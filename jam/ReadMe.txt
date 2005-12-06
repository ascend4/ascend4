
BUILDING ASCEND with JAM
------------------------

The files in this directory support building the Ascend
libraries and executables using the jam* build tool.

The following files are included:

  Jamrules_general.in - configuration & custom build rules used 
                        by Jamfiles
  Jambase             - lightly customized version of standard 
                        build rules (primarily to shore up support
                        for different compilers)

The Ascend jam build system currently supports building under Windows 
using the MSVC, Borland, MinGW, and Watcom** compilers, and under
Linux using gcc/g77.  With use of the included Jambase file, it should 
work with both the original and ftjam variants of jam. However, the
system was developed primarily using ftjam.

Automatic processing of Jamrules_general.in to Jamrules_general by
configure is not currently implemented.  A skeleton script
'configure' is provided which will copy the 'Jamrules_general.in' file
to 'Jamrules_general'. This file then needs to be hand-editied for
your local preferences and configuration.


Build Instructions - Linux
--------------------------

You can build the Jam static libraries on Linux at present:

1. jam must be installed and configured for the compiler in use.

2. Run ./configure 
   See above comments -- this script is just a skeleton at the moment

3. Manually edit Jamrules_general to set configuration and local
   directory options.  These settings are used by all the Jamfiles
   in the Ascend source tree.

4. Change directory to base/generic/jam, type 'jam libs'

The static libraries will end up in base/generic/jam/Release/linux

Build Instructions - Windows
----------------------------

As above, but instead of steam (2.) you need to manualy copy the
Jamrules_general.in file to Jamrules_general.


Notes
-----

* jam is an open source, multi-platform build tool
  which is available in several flavors.  These include
  the original (http://www.perforce.com) and ftjam
  (http://www.freetype.org/jam/index.html) variants,
  among others.

** Currently, Open Watcom 1.3 can compile the Ascend sources
  and build the libraries.  However, it does not supply the required
  IEEE math functions isnan() and copysign().  At present, Ascend does
  not supply these, so the user must provide these functions to use
  the libraries under Watcom.

Updated Dec 6 2005 -- johnpye
