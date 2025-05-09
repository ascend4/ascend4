#!/usr/bin/env python3
import os, sys, subprocess, json, gzip, fnmatch
from collections import defaultdict
from gi.repository import Gio, Gtk, GObject, Gedit, PeasGtk

# GSettings for plugin preferences
t_settings = Gio.Settings.new("org.gnome.gcov-gedit")

def MSG(*args, **kwargs):
    if t_settings.get_boolean("debug"):
        print("[CoverageHighlighter]", *args, file=sys.stderr, **kwargs)

class CoverageHighlighter(GObject.Object, Gedit.ViewActivatable):
    __gtype_name__ = "CoverageHighlighter"
    view = GObject.Property(type=Gedit.View)

    def __init__(self):
        super().__init__()
        MSG("__init__ called")
        self._idle_id = None
        self._monitor = None
        self._debounce_id = None
        self._stem = None
        self._lcov_root = None
        self._baseline_cache = {}
        self._statusbar = None
        self._status_ctx = None

    def do_activate(self):
        MSG("Activating plugin")
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG("No TeplFile; plugin disabled")
            return
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG("No Gio.File; plugin disabled")
            return

        # set up status bar context
        win = self.view.get_toplevel()
        try:
            self._statusbar = win.get_statusbar()
            self._status_ctx = self._statusbar.get_context_id('CoverageHighlighter')
            MSG("Statusbar context created")
        except Exception as e:
            MSG(f"Statusbar init error: {e}")

        src = gfile.get_path()
        MSG(f"Loaded source file: {src}")
        base = os.path.basename(src)
        self._stem = os.path.splitext(base)[0]
        src_dir = os.path.dirname(src)

        # locate .lcov folder for baseline coverage file
        self._lcov_root = self._find_lcov_root(src_dir)
        MSG(f"LCOV root: {self._lcov_root}")

        # ensure coverage tags exist
        if not hasattr(buf, '_cov_tags'):
            MSG("Creating coverage tags")
            buf._cov_tags = (
                buf.create_tag('cov_covered',   background='#d0ffd0'),
                buf.create_tag('cov_uncovered', background='#ffd0d0'),
                buf.create_tag('cov_new',       background='#c0ffc0'),
                buf.create_tag('cov_lost',      background='#ffc0c0'),
                buf.create_tag('cov_common',    background='#fff8c0'),
            )
        self._tags = buf._cov_tags

        # schedule initial highlight
        MSG("Scheduling initial highlight")
        self._idle_id = GObject.idle_add(self._activate_highlight)

        # monitor .gcda/.gcno changes for re-highlighting
        try:
            file_obj = Gio.File.new_for_path(src_dir)
            self._monitor = file_obj.monitor_directory(
                Gio.FileMonitorFlags.NONE, None)
            self._monitor.connect('changed', self._on_dir_changed)
            MSG(f"Monitoring directory for changes: {src_dir}")
        except Exception as e:
            MSG(f"Failed to monitor directory: {e}")

    def do_deactivate(self):
        MSG("Deactivating plugin")
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
            MSG(f"Detected change for {name}, debouncing")
            if self._debounce_id:
                GObject.source_remove(self._debounce_id)
            self._debounce_id = GObject.timeout_add(500, self._on_debounce_timeout)

    def _on_debounce_timeout(self):
        MSG("Debounce elapsed, scheduling highlight")
        self._debounce_id = None
        GObject.idle_add(self._activate_highlight)
        return False

    def _activate_highlight(self):
        buf = self.view.get_buffer()
        tfile = getattr(buf, 'get_file', lambda: None)()
        if not tfile:
            MSG("No TeplFile; skip highlight")
            return False
        gfile = getattr(tfile, 'get_location', lambda: None)()
        if not gfile:
            MSG("No Gio.File; skip highlight")
            return False
        src = gfile.get_path()
        if not src.endswith('.c'):
            MSG(f"Not a C source: {src}")
            return False

        diff_mode = t_settings.get_boolean('differential-mode')
        mode = 'diff' if diff_mode else 'standard'
        MSG(f"Highlight mode: {mode} for {src}")

        # status bar notification
        if self._statusbar and self._status_ctx is not None:
            self._statusbar.push(self._status_ctx, f"Starting {mode} coverage highlight")

        if diff_mode and self._lcov_root:
            self._highlight_diff(src)
        else:
            self._highlight_standard(src)

        # final status bar message
        if self._statusbar and self._status_ctx is not None:
            self._statusbar.push(self._status_ctx, f"{mode.capitalize()} highlight complete for {os.path.basename(src)}")

        return False

    def _highlight_standard(self, src):
        MSG(f"Standard highlight: {src}")
        buf = self.view.get_buffer()
        src_dir = os.path.dirname(src)
        base = os.path.basename(src)
        stem = self._stem
        note = os.path.join(src_dir, f"{stem}.gcno")
        data = os.path.join(src_dir, f"{stem}.gcda")

        if not os.path.isfile(data):
            MSG('Data file missing; clearing highlights')
            start, end = buf.get_start_iter(), buf.get_end_iter()
            buf.remove_tag(self._tags[0], start, end)
            buf.remove_tag(self._tags[1], start, end)
            return

        latest = max(os.path.getmtime(p) for p in (note, data) if os.path.exists(p))
        json_path = None
        for fn in os.listdir(src_dir):
            if fn.startswith(stem) and fn.endswith('.gcov.json.gz'):
                path = os.path.join(src_dir, fn)
                if os.path.getmtime(path) >= latest:
                    json_path = path
                    break
        if not json_path:
            MSG('Generating gcov JSON')
            subprocess.run(['gcov','-i','-b','-r', base], cwd=src_dir,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            for fn in os.listdir(src_dir):
                if fn.startswith(stem) and fn.endswith('.gcov.json.gz'):
                    json_path = os.path.join(src_dir, fn)
                    break
        if not json_path:
            MSG('No JSON found; abort standard highlight')
            return

        try:
            with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f"JSON parse error: {e}")
            return

        coverage = {}
        for fentry in data.get('files', []):
            if fentry.get('file','').endswith(base):
                for lninfo in fentry.get('lines', []):
                    coverage[lninfo['line_number']-1] = lninfo['count']>0
                break

        start, end = buf.get_start_iter(), buf.get_end_iter()
        buf.remove_tag(self._tags[0], start, end)
        buf.remove_tag(self._tags[1], start, end)
        for ln, hit in coverage.items():
            it0 = buf.get_iter_at_line(ln)
            it1 = it0.copy(); it1.forward_to_line_end()
            buf.apply_tag(self._tags[0] if hit else self._tags[1], it0, it1)
        MSG(f"Standard highlight complete for {src}")

    def _highlight_diff(self, src):
        MSG(f"Differential highlight: {src}")
        buf = self.view.get_buffer()
        base_info = os.path.join(self._lcov_root, 'base.info')
        base_counts = self._baseline_cache.get(src) or self._load_baseline(base_info, src)
        self._baseline_cache[src] = base_counts
        curr_counts = self._load_current(src)
        if curr_counts is None:
            MSG('No current counts; abort diff')
            return

        start, end = buf.get_start_iter(), buf.get_end_iter()
        for tag in self._tags:
            buf.remove_tag(tag, start, end)

        for ln in sorted(set(base_counts) | set(curr_counts)):
            b = base_counts.get(ln,0)>0
            c = curr_counts.get(ln,0)>0
            if not b and c:
                tag = self._tags[2]
            elif b and not c:
                tag = self._tags[3]
            elif b and c:
                tag = self._tags[4]
            else:
                continue
            it0 = buf.get_iter_at_line(ln-1)
            it1 = it0.copy(); it1.forward_to_line_end()
            buf.apply_tag(tag, it0, it1)
        MSG(f"Differential highlight complete for {src}")

    def _find_lcov_root(self, start_dir):
        MSG(f"Searching for .lcov in {start_dir}")
        p = start_dir
        while p and p != os.path.dirname(p):
            cand = os.path.join(p, '.lcov')
            if os.path.isdir(cand):
                MSG(f"Found .lcov at {cand}")
                return cand
            p = os.path.dirname(p)
        MSG("No .lcov folder found")
        return None

    def _load_baseline(self, info_path, src):
        MSG(f"Loading baseline counts from {info_path} for {src}")
        counts = {}
        try:
            with open(info_path, 'r') as f:
                in_sec = False
                for raw in f:
                    line = raw.strip()
                    if line.startswith('SF:'):
                        in_sec = (line[3:].strip() == src)
                    elif in_sec and line.startswith('DA:'):
                        ln, cnt = line[3:].split(',',1)
                        counts[int(ln)] = int(cnt)
                    elif in_sec and line == 'end_of_record':
                        break
        except Exception as e:
            MSG(f"Error loading baseline: {e}")
        MSG(f"Baseline lines: {len(counts)}")
        return counts

    def _load_current(self, src):
        MSG(f"Loading current counts via gcov JSON for {src}")
        src_dir, base = os.path.dirname(src), os.path.basename(src)
        stem = os.path.splitext(base)[0]
        note = os.path.join(src_dir, f"{stem}.gcno")
        data = os.path.join(src_dir, f"{stem}.gcda")
        if not os.path.exists(data):
            MSG("No .gcda; skipping current load")
            return {}
        latest = max(os.path.getmtime(p) for p in (note, data) if os.path.exists(p))
        json_path = None
        for fn in os.listdir(src_dir):
            if fn.startswith(stem) and fn.endswith('.gcov.json.gz'):
                path = os.path.join(src_dir, fn)
                if os.path.getmtime(path) >= latest:
                    json_path = path
                    break
        if not json_path:
            try:
                MSG("Running gcov to generate JSON")
                subprocess.run(['gcov','-i','-b','-r', base], cwd=src_dir,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except Exception as e:
                MSG(f"gcov error: {e}")
                return {}
            for fn in os.listdir(src_dir):
                if fn.startswith(stem) and fn.endswith('.gcov.json.gz'):
                    json_path = os.path.join(src_dir, fn)
                    break
        if not json_path:
            MSG("No JSON found after gcov run")
            return {}
        try:
            with gzip.open(json_path, 'rt', encoding='utf-8') as gf:
                data = json.load(gf)
        except Exception as e:
            MSG(f"JSON parse error: {e}")
            return {}
        counts = {}
        for fentry in data.get('files', []):
            if fentry.get('file','').endswith(base):
                for lninfo in fentry.get('lines', []):
                    counts[lninfo['line_number']] = lninfo['count']
                break
        MSG(f"Current lines: {len(counts)}")
        return counts

class CoverageHighlighterPrefs(GObject.Object, PeasGtk.Configurable):
    __gtype_name__ = "CoverageHighlighterPrefs"
    object = GObject.Property(type=Gedit.Window)

    def do_create_configure_widget(self):
        builder = Gtk.Builder()
        # Load preferences UI from the plugin directory
        plugin_dir = os.path.dirname(__file__)
        ui_path = os.path.join(plugin_dir, 'gcov-prefs.ui')
        builder.add_from_file(ui_path)
        widget = builder.get_object("CoverageHighlighterPrefs")

        switch = builder.get_object("debug-switch")
        switch.set_active(t_settings.get_boolean("debug"))
        switch.connect(
            "toggled",
            lambda btn: t_settings.set_boolean("debug", btn.get_active())
        )
        diff_switch = builder.get_object("diff-switch")
        diff_switch.set_active(t_settings.get_boolean("differential-mode"))
        diff_switch.connect(
            "toggled",
            lambda btn: t_settings.set_boolean("differential-mode", btn.get_active())
        )
        return widget

    def do_update_configuration(self):
        pass

# vim:ts=4:et:sw=4

