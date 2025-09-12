# GCOV plugin for Gedito

This is a coverage highlighting tool for the Gedit text editor. It is 
completely independent of the ASCEND code and should really be split out
into a separate repository. 

## Installing gedit-gcov

In the current directoy, just use

```sh
scons install
```

and the plugin will be installed in your `~/.local` tree for use by the current
user (no root access required). It would be possible to also install in 
`/usr/local` with some small changes to `SConstruct`.

### Compiling GSettings schemas

The GSettings schema file (`org.gnome.gcov-gedit.gschema.xml`) is installed to `~/.local/share/glib-2.0/schemas` by `scons install`. You can compile the schemas manually by running:

```sh
glib-compile-schemas ~/.local/share/glib-2.0/schemas
```
This step is automatically included in the `scons install` step.

## Using the plugin

This tool makes use of .gcno and .gcda files that sit alongside corresponding
.c and .cpp files in the ASCEND source tree. These files are produced when
compiling ASCEND with a command such as 

```sh
scons GCOV=1 DEBUG=1 -j7 test
./a4 script test/test compiler_blackbox
```

Once you have run a test (as above), you can open a relevant `.c` file in 
Gedit, and it should appear with red and green highlighting of some lines.

* green lines: covered
* red lines: not covered in tests
* no highlighting: not counted as 'code', for example may be commented out or may relate to declarations rather than imperatives.

The plugin monitors the state of currently-displayed files and triggers `gcov`
to run in the background in relevant `.gcno` and `.gcda` files appear or 
disappear.

## Differential coverage analysis

The plugin also supports highlighting of differential coverage analysis. (BETA)
Providing the Preferences setting for differential coverage is turned on, the
plugin will search for a folder called `.lcov` in parent folders (assumed to be
the projec root, referred to here as `$PROJROOT`). If found, and if it contains a file 
called `base.info`, then the plugin will attempt to display differential coverage for the currently open 
file.

To create the `$PROJROOT/.lcov/base.info` file, build your code, clean out old
`.gcda` files, run your baseline test, then run `lcov` to build the `base.info`
file:

```sh
cd $PROJROOT
mkdir .lcov
scons GCOV=1 DEBUG=1 -j7 test # of course build your project first...
find . -name '*.gcda' -delete # remove any leftover coverage data
./a4 script test/test compiler_blackbox.pass1 compiler_blackbox.pass2
lcov --capture --directory . --output-file .lcov/base.info --rc check_data_consistency=0 
```

For the differential test, you can keep the existing coverage data, or you can 
reset it if you want to show 'no longer covered' lines and not just 
'newly covered' lines:

```sh
cd $PROJROOT
find . -name '*.gcda' -delete # remove any leftover coverage data
```

Then, run your your new test:
```sh
./a4 script test/test compiler_blackbox.pass1 compiler_blackbox.pass3
```

Having done that, any files with `$PROJROOT` will display differential coverage
if found":

* green lines: newly covered
* red lines: no longer covered
* orange: covered in both baseline and current
* no highlighting: not counted as 'code' (see above)

Below is what `rel_blackbox.c` looks like in Gedit after running the above tests:

<img width="1178" height="1037" alt="image" src="https://github.com/user-attachments/assets/f3e79bc9-e01d-4aea-8234-808aa37feb67" />




