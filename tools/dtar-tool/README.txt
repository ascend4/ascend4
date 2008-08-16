DTAR
----

This is a fairly simple tool for building a Debian package
from a source code tarball. Following the convention of the
'rpm -ta' command, one can embed the necessary files for
building platform-dependent packages within a platform-independent
source-code package. This script implements the necessary
script for compiling this package on Debian platforms.

Usage:
dtar ~/ascend-0.9.5.115.tar.bz2

It works by first reading a few of the important files from
inside the tarball, including 'debian/control' and
'debian/changelog' to make sure that these are present and
readable, and that the debian changelog has a version number
that matches the source code tarball version number.

Next it unpacks the files to a temporary directory and
creates the source package. Finally, it creates the
binary package and deletes the temporarary files.

It is intended that some more command-line arguments will
be added to provide greater support over the build process.

It is also intended that this build script will provide
support for standard file locations as used by RPM, eg
RPMS/i386/mypackage_1.0-0.deb, so that the user can easily
locate the resulting packages once the command has
completed.

John Pye
16 Aug 2008.

