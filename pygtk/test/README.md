# PyGTK tests

These pytest tests exercise the ASCEND PyGTK browser against the in-tree
ASCEND libraries. Run them through the development launcher so that
`PYTHONPATH`, the shared-library path, `ASCENDLIBRARY`, and `ASCENDSOLVERS` are
set correctly:

```sh
./a4 pytest pygtk/test
```

On a headless machine, install `pytest-xvfb` as a test dependency. The plugin
starts Xvfb automatically:

```sh
python3 -m pip install pytest pytest-xvfb
./a4 pytest pygtk/test
```

Without the plugin, the equivalent explicit command is:

```sh
xvfb-run -a ./a4 pytest pygtk/test
```

Use `./a4 pytest pygtk/test -m gui` for all GUI tests, or add `-k blocks` to
select the model-tree block tests. When a test fails after creating the main
window, its last rendered image is saved below `.pytest_cache` in the
`gui-screenshots` cache directory.

The tests normally activate GTK signals directly. This exercises the same
callbacks as a click while avoiding fragile screen coordinates. Tests for
coordinate-sensitive behaviour, such as selecting a tree row before opening a
context menu, can use `Gdk.test_simulate_button` after the row has been realised
under Xvfb.
