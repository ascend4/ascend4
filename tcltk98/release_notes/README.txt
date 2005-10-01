This file contains release notes for version 0.9.5 of ascend4,
binary for Windows.

Topics-
0. Documentation for the ASCEND language and how to use it
1. Installing under Win32 (95, 98, ME, NT4, XP)
2. Installing under Windows 3.1.1
3. Tcl/Tk used by ASCEND under Windows
4. Installing debugger version of ASCEND under Windows
5. Building ASCEND under Dev Studio from its sources
6. Modeling libraries from CMU

0. Documentation for the ASCEND language and how to use it.

   Howtos for mathematical modeling are available from
      http://ascend.cheme.cmu.edu/pdfhelp.htm.

   Bibliography and online publications relating to ASCEND and
   ASCEND applications are available from
      http://ascend.cheme.cmu.edu/ascend_documentation.htm.

   General information on ASCEND including free registration and
   bug report/technical support information is available from
      http://ascend.cheme.cmu.edu/Home.html

   All information is available in Adobe PDF format. Some information
   is available in alternate formats.


1. To install this software for Windows 95, 98, ME, NT 4, or XP:

   Unzip the distribution file. If your unzip program has an
   option that turns folder information off and on, turn it ON.
   ASCEND will not work without being arranged in its folders.

   Find ascopt4.exe in new folder <INSTALL_DIR>\ascend4-0.9.5\ascend4\bin\,
   then	double-click to start ASCEND.
      or
   Right click the executable to create a shortcut and move the
   shortcut to your desktop or menu folder.

   Do NOT move the ascopt4.exe. It will not work except from
   the bin directory.

   The current release has only been tested on Windows XP.  If you
   encounter difficulties installing ASCEND, please let us know.


2. To install this software for Windows 3.1.1:

   ASCEND may work if you have installed Win32s.
   Verify that you have win32s and then follow the instructions for
   installing under Win32. We have not tested this theory as we
   have no obsolete Windows boxes available.


3. Tcl/Tk Notes:

   This release of ASCEND relies on Tcl/Tk 8.0p2, the portable GUI
   system from http://www.scriptics.com.  The ASCEND IV zip file is
   entirely self-contained.  You do not need to install Tcl/Tk
   separately. ASCEND will not interfere with any other Tcl/Tk
   version you may have installed.


4. To install the debugger version of ASCEND under windows:

   ASCEND IV is compiled with Mingw gcc 3.2.3 available from
   http://www.mingw.org.  The unoptimized executable file with
   debugger symbols is available.
     - Install the normal version of ASCEND IV.
     - Obtain a4debug.zip from the same download area
       where you obtained the normal version.
     - Unzip a4debug.zip.
     - Move ascend4.exe to <INSTALL_DIR>\ascend4-0.9.0/ascend4/bin.
       It will be in the same location as ascopt4.exe.
     - Run ascend4.exe just as you would ascopt.exe.
   Otherwise, you'll have to build ASCEND yourself in debug mode.


5. Building ascend4 from the source distribution.

   ASCEND IV is compiled from a single set of sources that
   works under most known ANSI C/C++ compilers for UNIX/X11R6
   or Windows. It is in principle portable to Macintosh,
   though this has not been done.

   Building ASCEND under Linux or other UNIX is trivial
   assuming you have matched ansi compliant C and F77
   compilers available. The configure and make programs
   take care of nearly everything automagically.

   Under Windows, life is not so simple.
   Compiling the complete system requires a matched set of
   C and FORTRAN compilers.  Known combinations that work
   include Mingw gcc/g77 and Visual C++/Digital FORTRAN.
   The Open Watcom tools almost work, but lack a couple of
   needed ieee math functions at this point.  The standard
   distribution is built using jam (ft variant preferred,
   see http://freetype.sourceforge.net/jam) and Mingw.  At
   this point, it is necessary to manually set options in
   <TOP>\jam\Jamrules_general.  Then, run jam in each of the
   jam subdirectories of base, blas, linpack, lsod, and
   tcltk98 (last).  A sporadically-maintained Visual Studio 7
   project set is also provided in the source distribution.
   You're on your own for an alternate FORTRAN compiler.


6. Modeling libraries from CMU

   A variety of libraries are included in the ascend4\models\
   directory. See ascend4\models\README.txt or README.html for
   details. Additional contributed models may be available from the
   ASCEND web sites above.

