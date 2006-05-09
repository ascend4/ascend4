
BUILDING ASCEND WITH JAM
------------------------

The files in this directory support building the ASCEND
libraries and executables using the jam* build tool.

The following files are included:

  Jamfile     - top-level Jamfile for building ASCEND

  Jamrules.in - configuration & custom build rules used by Jamfiles

  Jambase     - lightly customized version of standard build rules
              - primarily to shore up support for different compilers
              - optional in most cases

Other subdirectories of the source trees contain additional files used
to build ASCEND with jam:

  ../base/generic/Jamfile    - builds the ASCEND base libraries
  ../blas/Jamfile            - builds the blas library
  ../linpack/Jamfile         - builds the linpack library
  ../lsod/Jamfile            - builds the LSODE library
  ../tcltk/generic/Jamfile - builds the tcltk executable
  ../tcltk/generic/Jamrules_tcltk.in
                             - configuration & custom build rules used
                               build the tcltk executable


The ASCEND jam build system intends to support building under Windows
using the MinGW/gcc, MSVC, Borland, and Watcom** compilers, and under
unix/linux using gcc/g77.  With use of the included custom Jambase file,
it should work with both the original and ftjam variants of jam.
However, the system was developed primarily using ftjam.  For best
results using your default Jambase, use of ftjam is recommended and
probably required.

Automatic processing of Jamrules.in and ../tcltk/generic/Jamrules_tcltk.in
by configure has now been implemented for systems supporting autoconf.
Configure should automatically set most necessary build options, although
some manual tweaking may still be necessary.  The configure.in file is
currently housed in ../base/unixAC213.  This may be relocated to a more
general location in the future.  For now, it is necessary to run autoconf &
configure from that directory, and then come back here to run jam.

Building ASCEND requires compatible C and FORTRAN compilers.  The official
distributions are built using gcc, which provides both of these.


Build Instructions - Linux/Unix
-------------------------------

You can build the Jam static libraries on Linux at present:

1. jam must be installed and configured for the compiler in use.

2. (a) If using autoconf, perform the following steps:

      - cd ../base/unixAC213
      - autoconf (if necessary)
      - ./configure {using the usual options such as --enable-gcc,
                     --with-tcl, --with-tk, --with-tktable}
      - cd ../../jam

   (b) If NOT using autoconf, manually copy the following files:
   
      - ./Jamrules.in  -->  ./Jamrules
      - ../tcltk/generic/Jamrules_tcltk.in  --> 
                    ../tcltk/generic/Jamrules_tcltk

3. If necessary or desired, manually edit the generated ./Jamrules and
   ../tcltk/generic/Jamrules_tcltk files to set configuration and local
   directory options.  These settings are used by all the Jamfiles
   in the ASCEND source tree.

4. Run jam to generate the desired output:

       jam all       builds all libraries, executable, test suite
       jam libs      builds base, blas, linpack, and lsode static libs
       jam ascend    builds the ASCEND executable (will build libs as needed)
       jam test      builds the test suite (will build libs as needed)
       jam blas      builds the blas library
       jam linpack   builds the linpack library
       jam lsode     builds the lsode library

   NOTE - if there is a problem with jam setting basic options (e.g. CC = gcc),
          there are 2 possible workarounds.  First, use the custom Jambase
          file (jam -f Jambase ...) which more reliably sets these basic
          compilation options.  Or, explicitly specify the symbol in
          question (jam -sCC=gcc ...).  We have not quite figured out how
          to make sure these get set when defined in other than Jambase.

   All built files will be placed in ./CONFIGTYPE/PLATFORM where
          CONFIGTYPE = Debug, Release
          PLATFORM   = linux, mingw, bcc, msvc, mingw, watcom


Installation of the executables, libraries and headers is under development.
Installation should be assumed not to work at this time.


Build Instructions - Windows
----------------------------

Same as for Linux/Unix above, except you will need to follow 2b since 
the autotools are not generally available.


Notes
-----

* jam is an open source, multi-platform build tool
  which is available in several flavors.  These include
  the original (http://www.perforce.com) and ftjam
  (http://www.freetype.org/jam/index.html) variants,
  among others.

** Currently, Open Watcom 1.3 can compile the ASCEND sources
  and build the libraries.  However, it does not supply the required
  IEEE math functions isnan() and copysign().  At present, ASCEND does
  not supply these, so the user must provide these functions to use
  the libraries under Watcom.


For more information on building ASCEND using JAM
please see the Wiki document at
https://pse.cheme.cmu.edu/wiki/view/Ascend/Jam

