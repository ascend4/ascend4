The VC7 projects in this folder build the Ascend base libraries.  
Each project builds a single library corresponding to the
source files in each of the base/generic subdirectories.

The projects are collated in a Visual Studio 2003 solution,
WinVC7_base_libs.sln.  This VS solution allows easy compilation
of all of the base libraries.

The current implementation does not perform generation of:
     compiler/ascParse.c
     compiler/ascParse.h
     compiler/scanner.c
which requires the unix lex/flex and sed tools.  The pre-generated
source files are used.  If modification of these compiler files
is desired, it will be necessary to obtain Win32 versions of these
tools and perform the source generation separately.  Both the Linux
and jam build systems perform this generation - refer to the
corresponding build files for guidance if needed.

A couple of sources in the generic\solver library still are dependent
on the Tcl/Tk headers.  This implementation assumes that the Tcl headers
are available in ..\..\..\Tcl\Include.  If the Tcl headers are located 
somewhere else, you will need to add the correct subdirectory to the
search path.