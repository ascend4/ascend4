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

    def do_activate(self):
        # Schedule coverage highlight once UI is idle (buffer content loaded)
        try:
            self._idle_id = GObject.idle_add(self._activate_highlight)
            MSG('Scheduled idle highlight')
        except Exception as e:
            MSG(f'Failed to schedule idle highlight: {e}')

    def do_deactivate(self):
        # Remove idle callback if still pending
        if self._idle_id:
            GObject.source_remove(self._idle_id)
            self._idle_id = None

    def _activate_highlight(self):
        buf = self.view.get_buffer()
        # Get TeplFile -> Gio.File
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG('No TeplFile; skipping highlight')
            return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG('No Gio.File; skipping highlight')
            return False
        src = gfile.get_path()
        MSG(f'Idle highlight for {src}')
        if not src.endswith('.c'):
            MSG(f'{src} is not a C source; skipping gcov')
            return False
        self._highlight(src)
        return False

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)

        # Run gcov to produce JSON
        cmd = ['gcov', '-i', '-b', '-r', base]
        MSG(f"Running {' '.join(cmd)} in {src_dir}")
        try:
            subprocess.run(cmd, cwd=src_dir, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
            MSG(f"gcov JSON completed for {base}")
        except subprocess.CalledProcessError as e:
            MSG(f"gcov error: {e.stderr.strip()}")
            return

        # Find the generated .gcov.json.gz file
        pattern = f"{os.path.splitext(base)[0]}*.gcov.json.gz"
        json_path = None
        for fname in os.listdir(src_dir):
            if fnmatch.fnmatch(fname, pattern):
                json_path = os.path.join(src_dir, fname)
                break
        if not json_path:
            MSG(f"No .gcov.json.gz file found in {src_dir}")
            return
        MSG(f"Parsing JSON file: {json_path}")

        # Load and parse JSON
        try:
            with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f"Failed to read/parse JSON: {e}")
            return

        # Extract per-line counts
        coverage = {}
        for fentry in data.get('files', []):
            fname = fentry.get('file', '')
            # Match by ending with the base filename
            if not fname.endswith(base):
                continue
            for line in fentry.get('lines', []):
                ln = line.get('line_number', 0) - 1
                count = line.get('count', 0)
                coverage[ln] = (count > 0)
            break

        total = len(coverage)
        covered = sum(1 for hit in coverage.values() if hit)
        percent = covered * 100 / total if total else 0
        MSG(f"Coverage: {covered}/{total} lines ({percent:.1f}%)")

        # Apply highlighting tags
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

