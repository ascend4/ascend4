The files in this directory support building the Tcl/Tk
version of ths Ascend modeling system using the jam* build tool.

The following files are included:

  Jamfile     - build instructions for the Ascend libs
  Jamrules.in - configuration & custom build rules used by Jamfile

The build also requires ../../jam/Jamrules_general and
../../jam/Jambase.  This system builds the Ascend executable
as well as a static library of Tcl/Tk interface functions.
Installation of the resulting program is still under construction.

The Ascend jam build system currently supports building the libraries
under Windows using the MSVC, Borland, MinGW, and Watcom** compilers
(extension to Linux/gcc is on the todo list).  With use of the
included Jambase file, it should work with both the original and ftjam
variants of jam.  The system was developed primarily using ftjam.

Automatic processing of Jamrules.in to Jamrules by configure is
not currently implemented.  Until it is, users will need to manually
copy Jamrules.in to Jamrules and edit Jamrules to match their
preferenes & configuration.

The library (libasc_tcltk98) is built in
./CONFIGTYPE/COMPILER/interface, while the executable
is built in ./CONFIGTYPE/COMPILER where
    CONFIGTYPE = Debug, Release
    COMPILER   = bcc, msvc, mingw, watcom, unix

There are several dependent libraries which must be provided to
successfully link the Ascend executable:

    libasc_compiler
    libasc_general
    libasc_packages
    libasc_solver
    libasc_utilities
    tcl80
    tk80

The default location for the Ascend base libraries is
../../base/jam/CONFIGTYPE/COMPILER.  The default location for the Tcl/Tk
libraries is ../../../Tcl/lib.  These may be modified by editing the
Jamrules file.
-------------------------------------------------------------------------

Build Instructions

1. jam must be installed and configured for the compiler in use.

2. Copy Jamrules.in to Jamrules.

3. Manually edit ../../jam/Jamrules_general, Jamrules and (if 
   necessary) Jamfile to set configuration and local directory 
   options.

4. Run jam from using the provided Jambase:
      jam -f ../../jam/Jambase ascend

-------------------------------------------------------------------------
Notes:

* jam is an open source, multi-platform build tool
which is available in several flavors.  These include
the original (http://www.perforce.com) and ftjam
(http://www.freetype.org/jam/index.html) variants,
among others.

** Currently, Open Watcom 1.3 can compile the Ascend sources but
cannot link the executable.  This is because Watcom does not supply
the required IEEE math functions isnan() and copysign().  Ascend does
not currently provide these support functions.

