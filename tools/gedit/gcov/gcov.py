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
        self.tag_hit = None
        self.tag_miss = None
        self._stem = None

    def do_activate(self):
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if tfile:
            gfile = getattr(tfile, 'get_location', lambda: None)()
            if gfile:
                src = gfile.get_path()
                base = os.path.basename(src)
                self._stem = os.path.splitext(base)[0]
                src_dir = os.path.dirname(src)
                try:
                    self._idle_id = GObject.idle_add(self._activate_highlight)
                    MSG('Scheduled initial idle highlight')
                except Exception as e:
                    MSG(f'Failed to schedule initial highlight: {e}')
                try:
                    file_obj = Gio.File.new_for_path(src_dir)
                    self._monitor = file_obj.monitor_directory(Gio.FileMonitorFlags.NONE, None)
                    self._monitor.connect('changed', self._on_dir_changed)
                    MSG(f'Monitoring {src_dir} for {self._stem}.gcda/.gcno changes')
                except Exception as e:
                    MSG(f'Failed to monitor directory: {e}')
                return
        MSG('Cannot determine TeplFile or path; plugin disabled')

    def do_deactivate(self):
        if self._idle_id:
            GObject.source_remove(self._idle_id)
            self._idle_id = None
        if self._monitor:
            try:
                self._monitor.cancel()
            except Exception:
                pass
            self._monitor = None
        if self._debounce_id:
            GObject.source_remove(self._debounce_id)
            self._debounce_id = None

    def _on_dir_changed(self, monitor, file, other, event_type):
        name = file.get_basename()
        if name == f"{self._stem}.gcda" or name == f"{self._stem}.gcno":
            MSG(f'Detected change for {name}, debouncing')
            if self._debounce_id:
                GObject.source_remove(self._debounce_id)
            self._debounce_id = GObject.timeout_add(500, self._on_debounce_timeout)

    def _on_debounce_timeout(self):
        self._debounce_id = None
        MSG('Debounce elapsed, scheduling highlight')
        GObject.idle_add(self._activate_highlight)
        return False

    def _activate_highlight(self):
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG('No TeplFile; skip highlight')
            return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG('No Gio.File; skip highlight')
            return False
        src = gfile.get_path()
        if not src.endswith('.c'):
            MSG('Not a C source; skip highlight')
            return False
        MSG(f'Highlighting coverage for {src}')
        self._highlight(src)
        return False

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)
        stem = self._stem or os.path.splitext(base)[0]

        note_path = os.path.join(src_dir, f"{stem}.gcno")
        data_path = os.path.join(src_dir, f"{stem}.gcda")
        buf = self.view.get_buffer()
        if not self.tag_hit or not self.tag_miss:
            self.tag_hit = buf.create_tag('cov_covered', background='#d0ffd0')
            self.tag_miss = buf.create_tag('cov_uncovered', background='#ffd0d0')
        # If data file missing, clear all highlights
        if not os.path.isfile(data_path):
            MSG('Data file missing; clearing highlights')
            start_iter = buf.get_start_iter()
            end_iter = buf.get_end_iter()
            buf.remove_tag(self.tag_hit, start_iter, end_iter)
            buf.remove_tag(self.tag_miss, start_iter, end_iter)
            return

        latest_data = 0
        for p in (note_path, data_path):
            if os.path.isfile(p):
                try:
                    latest_data = max(latest_data, os.path.getmtime(p))
                except Exception:
                    pass
        json_pattern = f"{stem}*.gcov.json.gz"
        json_path = None
        for fname in os.listdir(src_dir):
            if fnmatch.fnmatch(fname, json_pattern):
                candidate = os.path.join(src_dir, fname)
                try:
                    if os.path.getmtime(candidate) >= latest_data:
                        json_path = candidate
                        MSG('Using up-to-date JSON')
                    else:
                        MSG('JSON stale, regenerating')
                    break
                except Exception:
                    MSG('Error stat JSON, regenerating')
        if not json_path:
            MSG('Generating coverage JSON via gcov')
            cmd = ['gcov', '-i', '-b', '-r', base]
            try:
                subprocess.run(cmd, cwd=src_dir, check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
                MSG('gcov JSON generated')
            except subprocess.CalledProcessError as e:
                MSG(f'gcov error: {e.stderr.strip()}')
                return
            for fname in os.listdir(src_dir):
                if fnmatch.fnmatch(fname, json_pattern):
                    json_path = os.path.join(src_dir, fname)
                    break
        if not json_path:
            MSG('No JSON file found; aborting')
            return
        MSG(f'Parsing JSON: {json_path}')
        try:
            with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f'JSON parse error: {e}')
            return

        coverage = {}
        for fentry in data.get('files', []):
            if not fentry.get('file', '').endswith(base):
                continue
            for line in fentry.get('lines', []):
                ln = line.get('line_number', 0) - 1
                coverage[ln] = line.get('count', 0) > 0
            break

        MSG(f'Parsed {len(coverage)} lines of coverage')
        start_iter = buf.get_start_iter()
        end_iter = buf.get_end_iter()
        buf.remove_tag(self.tag_hit, start_iter, end_iter)
        buf.remove_tag(self.tag_miss, start_iter, end_iter)
        for ln, hit in coverage.items():
            try:
                start = buf.get_iter_at_line(ln)
                end = start.copy()
                end.forward_to_line_end()
                buf.apply_tag(self.tag_hit if hit else self.tag_miss, start, end)
            except Exception as e:
                MSG(f'Error tagging line {ln+1}: {e}')
        MSG('Highlight applied')

# vim:ts=4:et:sw=4

