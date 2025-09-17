# GCOV plugin for Gedit

This is a coverage highlighting tool for the Gedit text editor. It is 
completely independent of the ASCEND code and should really be split out
into a separate repository. 

## Installing gedit-gcov

In the current directoy, just use

```sh
scons install
```

and the plugin will be installed in your `~/.local` tree for use by the
current user (no root access required). It would be possible to also 
install in `/usr/local` with some small changes to `SConstruct`.

### Compiling GSettings schemas

The GSettings schema file (`org.gnome.gcov-gedit.gschema.xml`) is 
installed to `~/.local/share/glib-2.0/schemas` by `scons install`. 
You can compile the schemas manually by running:

```sh
glib-compile-schemas ~/.local/share/glib-2.0/schemas
```
This step is automatically included in the `scons install` step.

## Using the plugin

This tool makes use of .gcno and .gcda files that sit alongside 
corresponding .c and .cpp files in the ASCEND source tree. These 
files are produced when compiling ASCEND with a command such as 

```sh
scons GCOV=1 DEBUG=1 -j7 test
./a4 script test/test compiler_blackbox
```

Once you have run a test (as above), you can open a relevant `.c`
file in Gedit, and it should appear with red and green highlighting 
of some lines.

* green lines: covered
* red lines: not covered in tests
* no highlighting: not counted as 'code', for example may be commented out or may relate to declarations rather than imperatives.

The plugin monitors the state of currently-displayed files and triggers `gcov`
to run in the background in relevant `.gcno` and `.gcda` files appear or 
disappear.

## Differential coverage analysis

The plugin also supports some limited support for differential coverage analysis. This means that you can run a baseline coverage analysis, then a 'variant' coverage analysis (for example, two different unit tests) and you can find out all of the differences in the lines of code covered between the baseline and the variant: some lines will be covered in neither, some will be covered in the variant and not in the baseline or vice versa, and some will be covered in both. This is very useful in localising the source of hard-to-fix errors. 

(The important limitation of the current  implementation is that it does not handle source code changes between the baseline and variant (for more details see the paper by Henry Cox, https://doi.org/10.48550/arXiv.2008.07947) -- suggestions are welcome for how to add support for that!)

Providing the Preferences setting for differential coverage is turned on in Gedit, the plugin will search for a folder called `.lcov` in parent folders (assumed to be the project root, referred to here as `$PROJROOT`). If found, and if it contains a file called `base.info`, then the plugin will attempt to display differential coverage for the currently open file. Otherwise the plugin will do normal coverage analysis as noted above.

To create the `$PROJROOT/.lcov/base.info` file, build your code, clean out old `.gcda` files, run your baseline test, then run `lcov` to build the `base.info` file:

```sh
cd $PROJROOT
mkdir .lcov
scons GCOV=1 DEBUG=1 -j7 test # of course build your project first...
find . -name '*.gcda' -delete # remove any leftover coverage data
./a4 script test/test compiler_blackbox.pass1 compiler_blackbox.pass2
lcov --capture --directory . --output-file .lcov/base.info --rc check_data_consistency=0 
```
For the differential test, you can keep the existing coverage data, or you can reset it if you want to show 'no longer covered' lines and not just 'newly covered' lines:

```sh
cd $PROJROOT
find . -name '*.gcda' -delete # remove any leftover coverage data
```

Then, run your your new test:
```sh
./a4 script test/test compiler_blackbox.pass1 compiler_blackbox.pass3
```

Having done that, any files with `$PROJROOT` will display differential coverage if found, comparing in this example the 'pass1 and pass2' baseline with the 'pass1 and pass3' variant, with text highlighting colours in Gedit as follows:

* green lines: newly covered (in variant but not baseline)
* red: no longer covered (in baseline but not in variant)
* orange: covered in both baseline and variant
* gery: covered in neither
* no highlighting: lines not counted as 'code' (see above)

Below is what `rel_blackbox.c` looks like in Gedit after running the above tests:

<img width="1178" height="1037" alt="image" src="https://github.com/user-attachments/assets/1c5ba675-5233-41f0-bc17-e9af187ad118" />





