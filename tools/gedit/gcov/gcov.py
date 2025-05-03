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

    def do_activate(self):
        # Determine TeplFile via Document or Buffer
        tfile = None
        if hasattr(self.view, 'get_document'):
            try:
                doc = self.view.get_document()
                tfile = doc.get_file() if doc else None
            except Exception:
                MSG("get_document()/get_file() error, falling back")
        if not tfile:
            buf = self.view.get_buffer()
            try:
                tfile = buf.get_file()
            except Exception:
                MSG("buffer.get_file() error")
        if not tfile:
            MSG("No TeplFile for buffer, cannot determine file path")
            return

        # Get filesystem path
        try:
            gfile = tfile.get_location()
        except Exception:
            MSG("tfile.get_location() error")
            return
        if not gfile:
            MSG("No Gio.File for buffer, cannot determine file path")
            return

        src = gfile.get_path()
        MSG(f"Opening {src}")
        if not src.endswith('.c'):
            MSG("Not a C source, skipping coverage highlight")
            return

        # Perform coverage highlighting
        self._highlight(src)

    def do_deactivate(self):
        # No cleanup required
        pass

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)
        stem, _ = os.path.splitext(base)

        # Run gcov to produce JSON
        cmd = ['gcov', '-i', '-b', '-r', base]
        MSG(f"Running {' '.join(cmd)} in {src_dir}")
        try:
            subprocess.run(
                cmd,
                cwd=src_dir,
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True
            )
            MSG(f"gcov JSON completed for {base}")
        except subprocess.CalledProcessError as e:
            MSG(f"gcov error: {e.stderr.strip()}")
            return

        # Find the generated .gcov.json.gz file
        pattern = f"{stem}*.gcov.json.gz"
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
            fname = fentry.get('file')
            if fname == base or fname.endswith('/' + base):
                for line in fentry.get('lines', []):
                    ln = line.get('line_number', 0) - 1
                    count = line.get('count', 0)
                    coverage[ln] = (count > 0)
                break

        total = len(coverage)
        covered = sum(1 for hit in coverage.values() if hit)
        pct = (covered / total * 100) if total else 0
        MSG(f"Coverage: {covered}/{total} lines ({pct:.1f}%)")

        # Apply highlighting tags
        buf = self.view.get_buffer()
        tag_hit = buf.create_tag('cov_covered', background='#d0ffd0')
        tag_miss = buf.create_tag('cov_uncovered', background='#ffd0d0')
        for ln, hit in coverage.items():
            start = buf.get_iter_at_line(ln)
            end = start.copy()
            end.forward_to_line_end()
            buf.apply_tag(tag_hit if hit else tag_miss, start, end)
        MSG("Highlight applied")

# vim:ts=4:et:sw=4

