README for ASCEND on the OpenSUSE Build Service
===============================================

ASCEND can be built on the OpenSUSE Build Service. It has successfully been
built for Fedora 5, 6, and 7 as well as SUSE 10.0 and newer, and Ubuntu 7.04.

First, you will need create a distribution tarball. From the Subversion
repository, check out the sources.

Download the current ASCEND manual and save it as doc/book.pdf. Alternativaly,
if you have a recent LyX installed, run 'scons doc' to build your own copy 
of the manual. Currently, LyX 1.5.0 or 1.4.5+ are required for this.

Next, create the tarballs using 'scons dist'.

Now, upload the following files to the Build Service:

dist/ascend-<version>.tar.bz2
dist/debian.tar.gz
ascend.spec
debian/changelog (renamed to debian.changelog)
debian/control (renamed to debian.control)
debian/rules (renamed to debian.rules)

Finally, create a new file on the Build Service called ascend.dsc, containing
the following:

Format: 1.0
Source: ascend
Version: <version>-<release>
Binary: ascend
Maintainer: John Pye <john@curioussymbols.com>
Architecture: any
Standards-Version: 3.7.2
Build-Depends: debhelper, scons, gcc, flex, bison, python-dev, refblas3-dev, tcl8.4-dev, tk8.4-dev, debhelper, swig

Ensure that <version> and <release> corresponds to the first 'entry' in the
debian.changelog file that you uploaded. Unfortunately, the ascend.dsc file is
really a *derived* file rather than a *source* file, but this is still
something that the Build Service people are working out.

-- John Pye, Aug 2007.






