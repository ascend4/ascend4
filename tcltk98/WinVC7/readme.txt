UNDER CONSTRUCTION

The VC7 projects in this folder build the Ascend Tcl/Tk executable.
The basic Tcl/Tk interface is first built into a static library
(libasc_tcltk98.lib), and then the executable is built.  In addition
to this interface library, the executable also depends on several
other static libraries which are not built here:

     libasc_compiler.lib   Ascend4 base library
     libasc_general.lib    "
     libasc_packages.lib   "
     libasc_solver.lib     "
     libasc_utilities.lib  "
     tk80.lib              Distributed with or built from Tk80
     tcl80.lib             Distributed with or built from Tcl80

Other dependencies are optional (FORTRAN libraries, special solvers,
etc.) and are not currently implemented.

The library and executable projects are collected in a Visual Studio 
2003 solution, WinVC7_tcltk98.sln.  This VS solution allows easy 
compilation of both the library and executable.

The current implementation does not perform generation of 
interface/typelex.c, which requires the unix lex/flex and sed tools.  
The pre-generated source file is currently are used.  If modification 
of this interface file is desired, it will be necessary to obtain 
Win32 versions of these tools and perform the source generation 
separately.  Both the Linux and jam build systems perform this 
generation - refer to the corresponding build files for guidance 
if needed.

These sources are dependent on the Tcl/Tk headers.  This implementation 
assumes that the Tcl headers are available in ..\..\..\Tcl\Include.  If 
the Tcl headers are located somewhere else, you will need to add the 
correct subdirectory to the search path.