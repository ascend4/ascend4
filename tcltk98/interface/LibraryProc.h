/*
 *  LibraryProc.h
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.22 $
 *  Version control file: $RCSfile: LibraryProc.h,v $
 *  Date last modified: $Date: 2003/08/23 18:43:07 $
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

#ifndef LibraryProc_module_loaded
#define LibraryProc_module_loaded


/*
 *  To include this header, you must include the following:
 *      #include <stdio.h>
 *      #include "tcl.h"
 *      #include "interface/LibraryProc.h"
 */


extern int Asc_FileIDCopy(FILE *, FILE *);
/*
 *  int Asc_FileIDCopy
 *  FILE *filein;
 *  FILE *fileout;
 *  Copies the input from a file with specified file id to the other.
 *  It is the responsibility of the user to ensure that the files are
 *  open and writable.
 */


STDHLF_H(Asc_LibrParseCmd);
extern int Asc_LibrParseCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/*  Registered as: */
#define Asc_LibrParseCmdHN "libr_parsestring"
/*  Usage: */
#define Asc_LibrParseCmdHU \
  Asc_LibrParseCmdHN " <ascend code>"
#define Asc_LibrParseCmdHS \
  "Opens and parses ascend code, a string containing ASCEND statements"
#define Asc_LibrParseCmdHL "\
 * Opens and parses a string.  If the open is successful,\n\
 * the name of the module is returned, otherwise an error is reported.\n\
 * it is impossible to detect if there were parse errors from the exit\n\
 * status of this command, though this may be fixed in the future.\n\
"

STDHLF_H(Asc_LibrReadCmd);
extern int Asc_LibrReadCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/*  Registered as: */
#define Asc_LibrReadCmdHN "librread"
/*  Usage: */
#define Asc_LibrReadCmdHU \
  Asc_LibrReadCmdHN " <filename> [parse_relations]"
#define Asc_LibrReadCmdHS \
  "Opens and parses filename, a file containing ASCEND type definitions"
#define Asc_LibrReadCmdHL "\
 * Opens and parses filename; relations will be parsed unless the\n\
 * optional argument parse_relations is zero.  If the open is successful,\n\
 * the name of the module is returned, otherwise an error is reported.\n\
 * it is impossible to detect if there were parse errors from the exit\n\
 * status of this command.\n\
"

STDHLF_H(Asc_LibrOptionsCmd);
extern int Asc_LibrOptionsCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/*  Registered as: */
#define Asc_LibrOptionsCmdHN "asc_compiler_option"
/*  Usage: */
#define Asc_LibrOptionsCmdHU \
  Asc_LibrOptionsCmdHN " [-<option> [value]]"
#define Asc_LibrOptionsCmdHS \
  "gets and sets global options for parsing and compiling"
#define Asc_LibrOptionsCmdHL "\
 * If given no arguments, returns list of current options.\n\
 * Given an option and value, sets the option accordingly.\n\
 * Given just the option name, returns the value.\n\
 * Processes specific options one at a time only.\n\
 * Options are:\n\
 *   parserWarnings: values > 1 suppress increasingly bad parser messages\n\
 *   compilerWarnings: 0 (do not), >0 (do) issue instantiation warnings\n\
 *   simplifyRelations: 0 (do not), 1 (do) do trivial simplifications\n\
 *   useCopyAnon: 0 (do not), 1 (do) share relations automatically\n\
"

STDHLF_H(Asc_LibrTypeListCmd);
extern int Asc_LibrTypeListCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/*  Registered as:  */
#define Asc_LibrTypeListCmdHN "libr_types_in_module"
/*  Usage:  */
#define Asc_LibrTypeListCmdHU \
  Asc_LibrTypeListCmdHN " <module>"
#define Asc_LibrTypeListCmdHS \
  "Returns a list of types defined in the module named \"module\""
#define Asc_LibrTypeListCmdHL "\
  * Returns a list of types defined in the module named \"module\"\n\
"


STDHLF_H(Asc_LibrModuleInfoCmd);
extern int Asc_LibrModuleInfoCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrModuleInfoCmdHN "libr_moduleinfo"
/*  Usage:  */
#define Asc_LibrModuleInfoCmdHU \
  Asc_LibrModuleInfoCmdHN " <module> [<module>...]"
#define Asc_LibrModuleInfoCmdHS \
  "For each module, returns  name, file or buffer, time or index, {} or string"
#define Asc_LibrModuleInfoCmdHL "\
 *  For each file module, returns the name, filename, time modified and {}.\n\
 *  For each string module, returns the name, name, string parse index, and\n\
 *  string parsed.\n\
"



STDHLF_H(Asc_LibrDestroyTypesCmd);
extern int Asc_LibrDestroyTypesCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrDestroyTypesCmdHN "libr_destroy_types"
/*  Usage:  */
#define Asc_LibrDestroyTypesCmdHU \
  Asc_LibrDestroyTypesCmdHN " takes no arguments"
#define Asc_LibrDestroyTypesCmdHS \
  "Reinitializes all base types and the module list"
#define Asc_LibrDestroyTypesCmdHL "\
 *  calls DestroyLibrary from the compiler, redefines basetypes,\n\
 *  and reinitializes interface module list.\n\
 *  Empties prototype library.\n\
"


extern int Asc_GNUTextCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/*
 *  Asc_GNUTextCmd
 *  Registered as : gnutext
 *  gnutext [l,w] returns the string requested from
 *  Tom's license.c. defaults to warranty
 */



STDHLF_H(Asc_LibrHideTypeCmd);
extern int Asc_LibrHideTypeCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrHideTypeCmdHN "libr_hide_type"
/*  Usage:  */
#define Asc_LibrHideTypeCmdHU \
  Asc_LibrHideTypeCmdHN " <type> [part]"
#define Asc_LibrHideTypeCmdHS \
  "Causes the instances of type not to be shown when browsing"
#define Asc_LibrHideTypeCmdHL "\
 *  Will set to FALSE the bit TYPESHOW of the Type Description type.\n\
 *  This will cause that the instances of type will not be shown for\n\
 *  browsing purposes.\n\
 *  If part is given, the part named will be hidden when browsing objects\n\
 *  the type given. The part name must be simple, that is no [] or . are\n\
 *  allowed. Parts of arrays cannot be hidden by subscript.\n\
"


STDHLF_H(Asc_LibrUnHideTypeCmd);
extern int Asc_LibrUnHideTypeCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrUnHideTypeCmdHN "libr_unhide_type"
/*  Usage:  */
#define Asc_LibrUnHideTypeCmdHU \
  Asc_LibrUnHideTypeCmdHN " <type> [part]"
#define Asc_LibrUnHideTypeCmdHS \
  "Causes the instances of type to be shown when browsing"
#define Asc_LibrUnHideTypeCmdHL "\
 *  Will set to TRUE the bit TYPESHOW of the Type Description type.\n\
 *  This will cause that the instances of type will be shown for\n\
 *  browsing purposes.\n\
 *  If part is given, the part named will be shown when browsing objects\n\
 *  the type given.\n\
"

STDHLF_H(Asc_LibrTypeIsShownCmd);
extern int Asc_LibrTypeIsShownCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrTypeIsShownCmdHN "libr_type_is_shown"
/*  Usage:  */
#define Asc_LibrTypeIsShownCmdHU \
  Asc_LibrTypeIsShownCmdHN " <type>"
#define Asc_LibrTypeIsShownCmdHS \
  "Returns 1 if type is being shown, or 0 if not."
#define Asc_LibrTypeIsShownCmdHL "\
 *  Returns 1 is the TYPESHOWN bit of a type description is ON. 0 if is\n\
 *  OFF. Query required for updating the Library Display Buttons.\n\
"


STDHLF_H(Asc_LibrQueryTypeCmd);
extern int Asc_LibrQueryTypeCmd(ClientData, Tcl_Interp*, int, CONST84 char**);
/* Registered as: */
#define Asc_LibrQueryTypeCmdHN "libr_query"
/*  Usage:  */
#define Asc_LibrQueryTypeCmdHU \
  Asc_LibrQueryTypeCmdHN " -H for more details."
#define Asc_LibrQueryTypeCmdHS \
  "Returns list of all children of type or info on child or info on info"
#define Asc_LibrQueryTypeCmdHL1 "\
 *  Should be expanded to handle many other queries and eliminate\n\
 *  most of the libr_ functions. Some of the following are unimplemented\n\
"
#define Asc_LibrQueryTypeCmdHL10 "\
 *  -ancestors -type typename\n\
 *    Returns the names of types that type REFINES from most to least.\n\
"
#define Asc_LibrQueryTypeCmdHL20 "\
 *  -basemethods\n\
 *    Returns the names of methods defined on DEFINITION MODEL.\n\
"
#define Asc_LibrQueryTypeCmdHL30 "\
 *  -catalog\n\
 *     Returns list of all formal types ATOMic and MODEL.\n\
"
#define Asc_LibrQueryTypeCmdHL40 "\
 *  -childnames -type typename\n\
 *     Returns list of children of type.\n\
"
#define Asc_LibrQueryTypeCmdHL50 "\
 *  -childinfo [-type typename [-child childname]]\n\
 *     Returns metasyntactic info if type nor child given.\n\
 *     Returns info for all children if type given but childname omitted.\n\
 *     Returns info for childname if type and child given.\n\
"
#define Asc_LibrQueryTypeCmdHL60 "\
 *  -definition -type typename [-method methodname]\n\
 *     Returns definition of type in standard string form,unless\n\
 *     methodname is given, in which case just gives method.\n\
"
#define Asc_LibrQueryTypeCmdHL70 "\
 *  -exists -type typename\n\
 *     Returns 1 if typename exists, or 0 if not.\n\
"
#define Asc_LibrQueryTypeCmdHL80 "\
 *  -externalfunctions\n\
 *     Returns list of all external functions loaded.\n\
"
#define Asc_LibrQueryTypeCmdHL85 "\
 *  -findtype -type typename\n\
 *     Returns the module defining type.\n\
"
#define Asc_LibrQueryTypeCmdHL90 "\
 *  -filetypes\n\
 *     Returns list of expected ASCEND source code filename suffixes.\n\
 *     Extensions other than these are probably not ascend MODEL/ATOM/unit\n\
 *     code. The compiler will dutifully attempt to eat any file of any\n\
 *     name; it is not restricted to any particular extension, or lack of\n\
 *     extension.\n\
"
#define Asc_LibrQueryTypeCmdHL100 "\
 *  -fundamentals\n\
 *     Returns a list containing the names of the fundamental types.\n\
"
#define Asc_LibrQueryTypeCmdHL110 "\
 *  -language -type typename [-child childname] [-method methodname]\n\
 *     Returns languages of NOTES on typename if no option other than -type\n\
 *     Returns languages of NOTES on childname if given.\n\
 *     Returns languages of NOTES on methodname if given.\n\
"
#define Asc_LibrQueryTypeCmdHL115 "\
 *  -methods -type typename\n\
 *     Returns a list containing the names of the methods on type given.\n\
"
#define Asc_LibrQueryTypeCmdHL120 "\
 *  -modulelist -mtype mt\n\
 *    Returns a list of modules that have types defined if mt == 0\n\
 *    Returns a list of interactive modules if mt == 1\n\
 *    Returns a list of interactive modules with global statements if mt == 2\n\
"
#define Asc_LibrQueryTypeCmdHL130 "\
 *  -notes -dbid $db -record $noteid\n\
 *     Returns data of noteid given, if it is a known record of db.\n\
 *     Data is a tuple {type language child method text}.\n\
"
#define Asc_LibrQueryTypeCmdHL131 "\
 *  -notesdblist\n\
 *     Returns known dbids in a list.\n\
"
#define Asc_LibrQueryTypeCmdHL132 "\
 *  -notes -dbid db -type typename -language keyword \n\
 *     [-child childname] [-method methodname]\n\
 *     Returns token to notes on typename in language keyword\n\
 *       if no modifiers given,\n\
 *       or on childname in type if childname given,\n\
 *       or on methodname in type if methodname given, etc.\n\
 *     The returned token should be destroyed when done with it by\n\
"
#define Asc_LibrQueryTypeCmdHL133 "\
 *  -notes -dbid $db -destroytoken $token\n\
 *     If you forget, destroytoken will be done automagically when\n\
 *     next you delete all types in the library or exit.\n\
"
#define Asc_LibrQueryTypeCmdHL135 "\
 *  -notesdump -dbid db [-textwidth n] [-notestoken token]\n\
 *     Returns the list of all notes, with texts abbreviated,\n\
 *     unless -notestoken $token given in which case returns just those.\n\
 *     If a string is empty in some field, it becomes ~\n\
 *     If n < 5, n=5 is assumed. If n not specified, n = 15.\n\
 *     List is a tuple of identically sorted lists:\n\
 *     {types} {langs} {names} {methods} {textabbrs} {filelines} {recnums}\n\
"
#define Asc_LibrQueryTypeCmdHL136 "\
 *  -notekinds -dbid db\n\
 *     Returns the list of languages known in the database.\n\
"
#define Asc_LibrQueryTypeCmdHL137 "\
 *  -notesmatch -dbid -db -pattern string [-notestoken token]\n\
 *     Returns token to the list of notes with text matching string.\n\
 *     If notestoken is not given, searches whole database, else searches\n\
 *     just those notes in the token.\n\
"
#define Asc_LibrQueryTypeCmdHL140 "\
 *  -roottypes\n\
 *    Returns a list of all types (base or otherwise) that are not\n\
 *    refinements of other types. These may be the roots of hierarchies.\n\
"

#endif /*LibraryProc_module_loaded*/
