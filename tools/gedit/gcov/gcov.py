#!/usr/bin/env python3

from gi.repository import GObject, Gedit, Gio
import subprocess, os, re, sys

# Debug message helper
MSG = lambda *args, **kwargs: print(f"[CoverageHighlighter]", *args, file=sys.stderr, **kwargs)

class CoverageHighlighter(GObject.Object, Gedit.ViewActivatable):
    __gtype_name__ = "CoverageHighlighter"
    view = GObject.Property(type=Gedit.View)

    def __init__(self):
        super().__init__()

    def do_activate(self):
        # Try new API first, fallback if unavailable
        doc = None
        if hasattr(self.view, 'get_document'):
            try:
                doc = self.view.get_document()
            except Exception:
                MSG("get_document() exists but raised exception")
                doc = None
        else:
            MSG("get_document() not available on View")

        # Obtain TeplFile via Document or Buffer
        tfile = None
        if doc:
            try:
                tfile = doc.get_file()
            except Exception:
                MSG("doc.get_file() raised exception")
                tfile = None
        if not tfile:
            buf = self.view.get_buffer()
            try:
                tfile = buf.get_file()
            except Exception:
                MSG("buffer.get_file() raised exception")
                tfile = None

        if not tfile:
            MSG("No TeplFile for buffer, cannot determine file path")
            return

        # Get Gio.File and actual path
        try:
            gfile = tfile.get_location()
        except Exception:
            MSG("tfile.get_location() raised exception")
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

        cmd = ['gcov', '-r', '-j', '-b', base]
        MSG(f"Running {' '.join(cmd)} in {src_dir}")
        try:
            result = subprocess.run(
                cmd,
                cwd=src_dir,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            MSG(f"gcov completed for {base}")
            # Show initial output lines for debug
            for line in result.stdout.splitlines()[:5]:
                MSG(line)
        except subprocess.CalledProcessError as e:
            MSG(f"gcov error: {e.stderr.strip()}")
            return

        # Locate the .gcov file
        gcov_file = None
        for fname in (f"{base}.gcov", f"{stem}.gcov"):
            path = os.path.join(src_dir, fname)
            if os.path.isfile(path):
                gcov_file = path
                break
        if not gcov_file:
            MSG(f"No .gcov file found in {src_dir}")
            return
        MSG(f"Parsing {gcov_file}")

        # Parse coverage data
        pattern = re.compile(r'^\s*([-0-9#]+):\s*(\d+):')
        coverage = {}
        try:
            with open(gcov_file) as gf:
                for line in gf:
                    m = pattern.match(line)
                    if not m:
                        continue
                    count_str, lineno_str = m.groups()
                    lineno = int(lineno_str) - 1
                    if count_str in ('-', '#####'):
                        hit = False
                    else:
                        hit = count_str.isdigit() and int(count_str) > 0
                    coverage[lineno] = hit
        except Exception as e:
            MSG(f"Error reading gcov file: {e}")
            return

        total = len(coverage)
        covered = sum(1 for hit in coverage.values() if hit)
        pct = (covered / total * 100) if total else 0
        MSG(f"Coverage: {covered}/{total} lines ({pct:.1f}%)")

        # Apply highlighting
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

