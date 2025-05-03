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

    def do_activate(self):
        # Initial highlight when idle
        try:
            self._idle_id = GObject.idle_add(self._activate_highlight)
            MSG('Scheduled initial idle highlight')
        except Exception as e:
            MSG(f'Failed to schedule initial highlight: {e}')

        # Setup directory monitor to watch for .gcda/.gcno changes
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if tfile:
            gfile = getattr(tfile, 'get_location', lambda: None)()
            if gfile:
                src_dir = os.path.dirname(gfile.get_path())
                dir_file = Gio.File.new_for_path(src_dir)
                try:
                    self._monitor = dir_file.monitor_directory(
                        Gio.FileMonitorFlags.NONE, None)
                    self._monitor.connect('changed', self._on_dir_changed)
                    MSG(f'Monitoring directory for coverage changes: {src_dir}')
                except Exception as e:
                    MSG(f'Failed to monitor directory: {e}')

    def do_deactivate(self):
        # Cancel idle callback if pending
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

    def _on_dir_changed(self, monitor, file, other, event_type):
        # React only to create or modify events on coverage files
        name = file.get_basename()
        if name.endswith('.gcda') or name.endswith('.gcno'):
            MSG(f'Coverage data changed: {name}, scheduling refresh')
            GObject.idle_add(self._activate_highlight)

    def _activate_highlight(self):
        # Perform highlight pass
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG('No TeplFile; skip')
            return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG('No Gio.File; skip')
            return False
        src = gfile.get_path()
        MSG(f'Running highlight for {src}')
        if not src.endswith('.c'):
            MSG('Not a C source; skip')
            return False
        self._highlight(src)
        return False  # single-run

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)

        # Determine latest data timestamp
        data_mtime = 0
        for f in os.listdir(src_dir):
            if f.endswith('.gcda') or f.endswith('.gcno'):
                try:
                    m = os.path.getmtime(os.path.join(src_dir, f))
                except OSError:
                    continue
                if m > data_mtime:
                    data_mtime = m

        # Check existing JSON
        stem = os.path.splitext(base)[0]
        json_pattern = f"{stem}*.gcov.json.gz"
        existing_json = None
        for f in os.listdir(src_dir):
            if fnmatch.fnmatch(f, json_pattern):
                existing_json = os.path.join(src_dir, f)
                break

        rerun = True
        if existing_json:
            try:
                json_mtime = os.path.getmtime(existing_json)
                if json_mtime >= data_mtime:
                    MSG('Coverage JSON up-to-date; skipping gcov')
                    rerun = False
                else:
                    MSG('Coverage JSON stale; rerunning gcov')
            except OSError:
                MSG('JSON stat error; rerunning gcov')
        else:
            MSG('No JSON found; running gcov')

        if rerun:
            cmd = ['gcov', '-i', '-b', '-r', base]
            MSG(f"Running {' '.join(cmd)} in {src_dir}")
            try:
                subprocess.run(cmd, cwd=src_dir, check=True,
                               stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
                MSG(f"gcov JSON generated for {base}")
            except subprocess.CalledProcessError as e:
                MSG(f"gcov error: {e.stderr.strip()}")
                return

        # Locate JSON file
        json_path = None
        for f in os.listdir(src_dir):
            if fnmatch.fnmatch(f, json_pattern):
                json_path = os.path.join(src_dir, f)
                break
        if not json_path:
            MSG(f"No JSON coverage file found in {src_dir}")
            return
        MSG(f"Parsing JSON file: {json_path}")

        try:
            with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f"JSON parse error: {e}")
            return

        coverage = {}
        for fentry in data.get('files', []):
            if not fentry.get('file', '').endswith(base):
                continue
            for line in fentry.get('lines', []):
                ln = line.get('line_number', 0) - 1
                coverage[ln] = (line.get('count', 0) > 0)
            break

        total = len(coverage)
        covered = sum(1 for hit in coverage.values() if hit)
        MSG(f"Coverage: {covered}/{total} lines ({covered*100/total if total else 0:.1f}%)")

        buf = self.view.get_buffer()
        tag_hit = buf.create_tag('cov_covered', background='#d0ffd0')
        tag_miss = buf.create_tag('cov_uncovered', background='#ffd0d0')
        for ln, hit in coverage.items():
            start = buf.get_iter_at_line(ln)
            end = start.copy()
            end.forward_to_line_end()
            buf.apply_tag(tag_hit if hit else tag_miss, start, end)
        MSG('Highlight applied')

# vim:ts=4:et:sw=4

