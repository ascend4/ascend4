/*
 *  Typelex.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.6 $
 *  Version control file: $RCSfile: typelex.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:09 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the ASCEND Tcl/Tk interface
 *
 *  Copyright 1997, Carnegie Mellon University
 *
 *  The ASCEND Tcl/Tk interface is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The ASCEND Tcl/Tk interface is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is found in ../compiler.
 */

/** @file
 *  Lexical scanner.
 *  <pre>
 *  Requires:     #include "tcl.h"
 *                #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef ASCTK_TYPELEX_H
#define ASCTK_TYPELEX_H

STDHLF_H(Asc_ExtractType);
/**< Help long string function. */

extern int Asc_ExtractType(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char **argv);
/**<
 *  <!--  int Asc_ExtractType(cdata, interp, argc, argv)               -->
 *  <!--      ClientData cdata;   --client data, not used              -->
 *  <!--      Tcl_Interp *interp; --tcl interpreter, not used          -->
 *  <!--      int argc;           --the number of arguments            -->
 *  <!--      char **argv;        --the array of arguments             -->
 *
 *  This is the Tcl callback for the type extractor command.
 */
/**  Registered as */
#define Asc_ExtractTypeHN "libr_extract_type"
/** Usage */
#define Asc_ExtractTypeHU \
    Asc_ExtractTypeHN " [-c] <type> [source_file] [-s,destination_file]"
/** Short help text */
#define Asc_ExtractTypeHS \
    "Returns the ASCEND code that defines an ATOM or MODEL"
/** Long help text part 1 */
#define Asc_ExtractTypeHL1 "\
 *  Extracts the definition of the ATOM or MODEL <type> from the specified\n\
 *  source file (or stdin if not specified) and writes it to the specified\n\
 *  destination file (or stdout if not specified).  Comments are stripped\n\
"
/** Long help text part 2 */
#define Asc_ExtractTypeHL2 "\
 *  from the code unless the -c flag is given.  Returns TCL_OK if successful\n\
 *  If unsuccessful, returns TCL_ERROR and puts the error message into\n\
 *  interp->result. If -s is given instead of destination, output to interp.\n\
 *  source_file may be an interactive string module when in ASCEND.\n\
"

/**<
extern void Asc_PutCode(char *s, FILE *fp);
 * Intended for internal use only. Puts the echoed code out to
 * file or string as determined by -s instead of output file.
 *
 *  NOTES:
 *  Maybe we should consider returning the type definition in
 *  interp->result.  Assumes source-file contains valid ASCEND code.
 *  When looking for the type `foo', the tokens ``END foo'' inside of a
 *  singlely or doubly quoted string will be registered as the end of the
 *  MODEL or ATOM.  The tokens ``END foo'' may safely appear in a comment
 *  or the text of a NOTE.
 */

#endif  /* ASCTK_TYPELEX_H */

