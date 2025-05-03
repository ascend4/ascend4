#!/usr/bin/env python3

from gi.repository import GObject, Gedit, Gio
import subprocess, os, sys, json, gzip, fnmatch

# Debug message helper
def MSG(*args, **kwargs):
    print("[CoverageHighlighter]", *args, file=sys.stderr, **kwargs)

class CoverageHighlighter(GObject.Object, Gedit.ViewActivatable):
    __gtype_name__ = "CoverageHighlighter"
    view = GObject.Property(type=Gedit.View)

    def __init__(self):
        super().__init__()
        self._idle_id = None
        self._monitor = None
        self._debounce_id = None
        # Cache tags per view
        self.tag_hit = None
        self.tag_miss = None

    def do_activate(self):
        # Schedule initial highlight when idle
        try:
            self._idle_id = GObject.idle_add(self._activate_highlight)
            MSG('Scheduled initial idle highlight')
        except Exception as e:
            MSG(f'Failed to schedule initial highlight: {e}')

        # Set up directory monitor for .gcda/.gcno changes
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG('No TeplFile; directory monitor disabled')
            return
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG('No Gio.File; directory monitor disabled')
            return

        src_dir = os.path.dirname(gfile.get_path())
        try:
            file_obj = Gio.File.new_for_path(src_dir)
            self._monitor = file_obj.monitor_directory(Gio.FileMonitorFlags.NONE, None)
            self._monitor.connect('changed', self._on_dir_changed)
            MSG(f'Monitoring directory for coverage changes: {src_dir}')
        except Exception as e:
            MSG(f'Failed to monitor directory: {e}')

    def do_deactivate(self):
        # Cancel idle callback
        if self._idle_id:
            GObject.source_remove(self._idle_id)
            self._idle_id = None
        # Stop directory monitor
        if self._monitor:
            try:
                self._monitor.cancel()
            except Exception:
                pass
            self._monitor = None
        # Cancel debounce timer
        if self._debounce_id:
            GObject.source_remove(self._debounce_id)
            self._debounce_id = None

    def _on_dir_changed(self, monitor, file, other, event_type):
        name = file.get_basename()
        if not (name.endswith('.gcda') or name.endswith('.gcno')):
            return
        MSG(f'Detected change: {name}, debouncing')
        # debounce: reset timer
        if self._debounce_id:
            GObject.source_remove(self._debounce_id)
        self._debounce_id = GObject.timeout_add(500, self._on_debounce_timeout)

    def _on_debounce_timeout(self):
        self._debounce_id = None
        MSG('Debounce window elapsed, scheduling highlight')
        GObject.idle_add(self._activate_highlight)
        return False

    def _activate_highlight(self):
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG('No TeplFile; skipping highlight')
            return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG('No Gio.File; skipping highlight')
            return False
        src = gfile.get_path()
        if not src.endswith('.c'):
            MSG('Not a C source; skipping highlight')
            return False
        MSG(f'Applying coverage highlight for {src}')
        self._highlight(src)
        return False

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)
        stem, _ = os.path.splitext(base)

        # Determine freshness of coverage data
        latest_data = 0
        for fname in os.listdir(src_dir):
            if fname.endswith('.gcda') or fname.endswith('.gcno'):
                try:
                    m = os.path.getmtime(os.path.join(src_dir, fname))
                    latest_data = max(latest_data, m)
                except Exception:
                    pass
        # Check for existing JSON
        json_pattern = f"{stem}*.gcov.json.gz"
        json_path = None
        if latest_data:
            for fname in os.listdir(src_dir):
                if fnmatch.fnmatch(fname, json_pattern):
                    candidate = os.path.join(src_dir, fname)
                    try:
                        if os.path.getmtime(candidate) >= latest_data:
                            json_path = candidate
                            MSG('Existing JSON is up-to-date, using it')
                        else:
                            MSG('Existing JSON stale, regenerating')
                        break
                    except Exception:
                        MSG('Could not stat JSON, regenerating')
        if not json_path:
            MSG('Running gcov to generate JSON')
            cmd = ['gcov', '-i', '-b', '-r', base]
            try:
                subprocess.run(cmd, cwd=src_dir, check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
                MSG('gcov JSON generated')
            except subprocess.CalledProcessError as e:
                MSG(f'gcov error: {e.stderr.strip()}')
                return
            # locate again
            for fname in os.listdir(src_dir):
                if fnmatch.fnmatch(fname, json_pattern):
                    json_path = os.path.join(src_dir, fname)
                    break
        if not json_path:
            MSG('No JSON coverage file found; aborting highlight')
            return
        MSG(f'Parsing coverage JSON: {json_path}')
        try:
            with gzip.open(json_path, 'rt') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f'JSON parse error: {e}')
            return

        # Build line coverage map
        coverage = {}
        for fentry in data.get('files', []):
            if not fentry.get('file', '').endswith(base):
                continue
            for line in fentry.get('lines', []):
                ln = line.get('line_number', 0) - 1
                coverage[ln] = line.get('count', 0) > 0
            break

        MSG(f'Parsed {len(coverage)} lines of coverage')
        # Prepare tags
        buf = self.view.get_buffer()
        if not self.tag_hit or not self.tag_miss:
            self.tag_hit = buf.create_tag('cov_covered', background='#d0ffd0')
            self.tag_miss = buf.create_tag('cov_uncovered', background='#ffd0d0')

        # Clear all old tags first
        start_iter = buf.get_start_iter()
        end_iter = buf.get_end_iter()
        buf.remove_tag(self.tag_hit, start_iter, end_iter)
        buf.remove_tag(self.tag_miss, start_iter, end_iter)

        # Apply new tags
        for ln, hit in coverage.items():
            try:
                start = buf.get_iter_at_line(ln)
                end = start.copy()
                end.forward_to_line_end()
                buf.apply_tag(self.tag_hit if hit else self.tag_miss, start, end)
            except Exception as e:
                MSG(f'apply_tag error on line {ln+1}: {e}')
        MSG('Highlight applied')

# vim:ts=4:et:sw=4

