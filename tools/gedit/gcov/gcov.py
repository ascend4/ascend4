#!/usr/bin/env python3

import os, sys, subprocess, json, gzip, fnmatch
from gi.repository import Gio, Gtk, GObject, Gedit, PeasGtk

# GSettings for plugin preferences
settings = Gio.Settings.new("org.gnome.gcov-gedit")

def MSG(*args, **kwargs):
    """Print debug messages when 'debug' is true."""
    if settings.get_boolean("debug"):
        print("[CoverageHighlighter]", *args, file=sys.stderr, **kwargs)

class CoverageHighlighter(GObject.Object, Gedit.ViewActivatable):
    __gtype_name__ = "CoverageHighlighter"
    view = GObject.Property(type=Gedit.View)

    def __init__(self):
        super().__init__()
        self._idle_id     = None
        self._monitor     = None
        self._debounce_id = None
        self._stem        = None

    def do_activate(self):
        buf   = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG("No TeplFile; disabled")
            return
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG("No Gio.File; disabled")
            return

        src       = gfile.get_path()
        base      = os.path.basename(src)
        self._stem = os.path.splitext(base)[0]
        src_dir   = os.path.dirname(src)

        # Initial highlight once idle
        self._idle_id = GObject.idle_add(self._activate_highlight)

        # Watch only this file's .gcda/.gcno
        file_obj = Gio.File.new_for_path(src_dir)
        self._monitor = file_obj.monitor_directory(
            Gio.FileMonitorFlags.NONE, None)
        self._monitor.connect('changed', self._on_dir_changed)

    def do_deactivate(self):
        if self._idle_id:
            GObject.source_remove(self._idle_id)
            self._idle_id = None
        if self._monitor:
            self._monitor.cancel()
            self._monitor = None
        if self._debounce_id:
            GObject.source_remove(self._debounce_id)
            self._debounce_id = None

    def _on_dir_changed(self, monitor, file, other, etype):
        name = file.get_basename()
        if name in (f"{self._stem}.gcda", f"{self._stem}.gcno"):
            # debounce multiple rapid events
            if self._debounce_id:
                GObject.source_remove(self._debounce_id)
            self._debounce_id = GObject.timeout_add(
                500, self._on_debounce_timeout)

    def _on_debounce_timeout(self):
        self._debounce_id = None
        GObject.idle_add(self._activate_highlight)
        return False

    def _activate_highlight(self):
        buf   = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile: return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile: return False

        src = gfile.get_path()
        if not src.endswith('.c'): return False

        self._highlight(src)
        return False

    def _highlight(self, src):
        src_dir = os.path.dirname(src)
        base    = os.path.basename(src)
        stem    = self._stem

        note = os.path.join(src_dir, f"{stem}.gcno")
        data = os.path.join(src_dir, f"{stem}.gcda")
        buf  = self.view.get_buffer()

        # Attach or reuse the tag pair on the buffer itself
        if not hasattr(buf, '_cov_tags'):
            buf._cov_tags = (
                buf.create_tag('cov_covered',   background='#d0ffd0'),
                buf.create_tag('cov_uncovered', background='#ffd0d0')
            )
        tag_hit, tag_miss = buf._cov_tags

        # If no data file: clear and return
        if not os.path.isfile(data):
            start, end = buf.get_start_iter(), buf.get_end_iter()
            buf.remove_tag(tag_hit,  start, end)
            buf.remove_tag(tag_miss, start, end)
            return

        # Check timestamps to see if JSON needs regen
        latest_data = max(
            os.path.getmtime(p)
            for p in (note, data) if os.path.isfile(p)
        )

        pattern   = f"{stem}*.gcov.json.gz"
        json_path = None
        for fn in os.listdir(src_dir):
            if fnmatch.fnmatch(fn, pattern):
                cand = os.path.join(src_dir, fn)
                if os.path.getmtime(cand) >= latest_data:
                    json_path = cand
                break

        # Regenerate if missing or stale
        if not json_path:
            subprocess.run(
                ['gcov','-i','-b','-r',base],
                cwd=src_dir, check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE
            )
            for fn in os.listdir(src_dir):
                if fnmatch.fnmatch(fn, pattern):
                    json_path = os.path.join(src_dir, fn)
                    break

        if not json_path:
            return

        # Parse JSON
        with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
            data = json.load(gf)

        # Build per-line map
        coverage = {}
        for fentry in data.get('files', []):
            if not fentry.get('file','').endswith(base):
                continue
            for lninfo in fentry.get('lines', []):
                ln = lninfo.get('line_number',0)-1
                coverage[ln] = lninfo.get('count',0)>0
            break

        # Clear old tags
        start, end = buf.get_start_iter(), buf.get_end_iter()
        buf.remove_tag(tag_hit,  start, end)
        buf.remove_tag(tag_miss, start, end)

        # Apply new ones
        for ln, hit in coverage.items():
            try:
                it0 = buf.get_iter_at_line(ln)
                it1 = it0.copy(); it1.forward_to_line_end()
                buf.apply_tag(tag_hit if hit else tag_miss, it0, it1)
            except Exception:
                pass


class CoverageHighlighterPrefs(GObject.Object, PeasGtk.Configurable):
    __gtype_name__ = "CoverageHighlighterPrefs"
    object = GObject.Property(type=Gedit.Window)

    def do_create_configure_widget(self):
        builder = Gtk.Builder()
        ui_path = os.path.expanduser(
            "~/.local/share/gedit/plugins/gcov-prefs.ui"
        )
        builder.add_from_file(ui_path)
        widget = builder.get_object("CoverageHighlighterPrefs")

        switch = builder.get_object("debug-switch")
        switch.set_active(settings.get_boolean("debug"))
        switch.connect(
            "toggled",
            lambda btn: settings.set_boolean("debug", btn.get_active())
        )
        return widget

    def do_update_configuration(self):
        # nothing else to do
        pass

# vim:ts=4:et:sw=4

