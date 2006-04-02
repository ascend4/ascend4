%{
/*
 *  TypeExtractor.l
 *  by Kirk Abbott and Ben Allan
 *  Created: 1/94
 *  Version: $Revision: 1.20 $
 *  Version control file: $RCSfile: typelex.l,v $
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

/*
 *  ASCEND Type Retriever 1.2
 *
 *  Purpose:
 *      To extract a type definition from an input file.
 *  Usage:
 *      See the header file for use from within ASCEND.
 *      For stand-alone use:
 *          typer [flags] ascend_type_name input_file output_file
 *              flags: -c  show comments
 *                         If no -c given, strip out comments.
 *  Features:
 *      --Does not descend into REQUIREd files. Only examines input_file
 *        for definitions of ascend_type_name.
 *      --This version DOES handle the UNIVERSAL quantifier.
 *
 */

#define MY_YYMORE() yymore()

#include <tcl.h>
#include <utilities/ascConfig.h>
#include <general/list.h>
#ifndef TYPER_TEST
/* for handling string modules and results in interp */
#include <compiler/compiler.h>
#include <compiler/symtab.h>
#include <compiler/fractions.h>
#include <compiler/dimen.h>
#include <compiler/child.h>
#include <compiler/type_desc.h>
#include <compiler/library.h>
#include <compiler/module.h>
#include "HelpProc.h" /* help is not necessary for test */
#endif /*!typertest*/

#include "typelex.h"


#ifndef lint
static const char TyperID[] = "$Id: typelex.l,v 1.20 2003/08/23 18:43:09 ballan Exp $";
#endif

#define YY_BREAK
/*
 *  Defining yybreak as above (empty) means that all of our matches must end
 *  in break or return because the normal flex supplied yybreak will
 *  be preempted by our empty one.
 *  In cases where matches contain a conditional return, make sure a
 *  break follows in the failure case.
 */

static int CommentNestLevel = 0;
/*
 *  Nesting level of (* comments *)
 */

static int BracesNestLevel = 0;
/*
 *  Nesting level of {braced} expressions
 */

enum keyword_tokens {
  NONE,       /* Nothing to match, we're in the initial state */
  MODEL,      /* We've just read a MODEL or ATOM keyword */
  MODEL_END,  /* We've just read an END keyword inside a MODEL,
               * an ATOM, or a DEFINITION. */
  CONSTANT    /* We've just read a CONSTANT keyword */
};
static enum keyword_tokens MatchedToken = NONE;
/*
 *  The recognized token we've most recently read.  This usually implies
 *  that we're either in that state or we need to check the next keyword
 *  to see if we should enter (or leave, in the case of MODEL_END) that
 *  state.  MODEL, ATOM, and DEFINITION state are equivalent.
 */

static int ShowComments = 0;
/*
 *  Should we print comments? (1 = yes, 0 = no)
 *  This is set to 1 with `-c' on the command line
 */

static unsigned long StartId = 0;
/*
 *  The location in the `typ_text' array where the MODEL, ATOM, or CONSTANT
 *  identifierbegins.  We save this value and call MY_YYMORE() when we find
 *  a MODEL, ATOM, or CONSTANT keyword; StartId tells us where the
 *  Identifier will go.
 */

static char *reqType = "";
/*
 *  The type we are looking for.  This is one of the arguments passed
 *  to Asc_ExtractType() in the argv vector.
 */

static void Asc_PutCode(char *s,FILE *fp);

static Tcl_Interp *g_typ__interp;
/* during parse, set to caller's interp if string result is wanted
 * instead of file.
 */

#ifdef TYPER_TEST
/*
 *  Specify ASCERR, ASCWAR when building stand alone.
 */
FILE *ASCERR = stderr;
FILE *ASCWAR = stderr;
#endif /* TYPER_TEST */

%}

ws	[\f\r\t\v ]
digit	[0-9]
letter	[a-zA-Z]

ident	{letter}({digit}|{letter}|_)*

model		MODEL{ws}+
atom		ATOM{ws}+
definition	DEFINITION{ws}+
end		END{ws}+
univ		UNIVERSAL{ws}+
constant	CONSTANT{ws}+

%x Comment
%x BracedText
%x Model
%x Constant
%x CheckID

%option yymore
%%

	/*
	 *  Code to handle (* Comments *)
	 *
	 *  "(*" puts us into the Comment state.  Comments nest, so in the
	 *  Comment state we need to look for "(*" that increases the nesting
	 *  level and "*)" that will lower it.
	 *  Do NOT try to match \(\*+ since that causes "(****)" to parse
	 *  incorrectly.
	 */

<INITIAL>\(\*			|
<Model>\(\*			|
<Constant>\(\* 			{
				  /*  Match "(" followed by "*" puts us into
				   *  the COMMENT state.  Don't use \*+ since
				   *  that will parse "(***)" incorrectly.
				   *  Initialize the nesting level.
				   *  Print if ShowComments is TRUE and
                                   *  MatchedToken is not NONE
				   */
				  BEGIN(Comment);
				  CommentNestLevel = 1;
				  if( ShowComments && MatchedToken!=NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<Comment>\(\*[^*(]*		{
				  /*  Match "(" followed "*" followed by
				   *  anything that's not "(" nor "*".
				   *  `anything' includes newlines.
				   *  Increase the commment nesting level.
				   *  Print if ShowComments is TRUE and
                                   *  MatchedToken is not NONE.
				   */
				  CommentNestLevel++;
				  if( ShowComments && MatchedToken!=NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<Comment>[^*(]*\*+\)		{
				  /*  Match anything not "*" or "(" followed
				   *  by one or more "*"s followed by ")".
				   *  `anything' includes newlines.
				   *  This decreases the comment nesting level
				   *  and kicks us out if we're back to zero.
				   *  Print if ShowComments is TRUE and
                                   *  MatchedToken is not NONE.
				   */
				  CommentNestLevel--;
				  if( CommentNestLevel > 0 ) {
                                    /* more comments */
                                    break;
                                  }
                                  if( MatchedToken == NONE ) {
                                    BEGIN(INITIAL);
                                    break;
                                  }
                                  if( ShowComments ) {
                                    Asc_PutCode( typ_text, typ_out );
                                  }
                                  switch( MatchedToken ) {
                                  case MODEL:
                                    BEGIN(Model);
                                    break;
                                  case CONSTANT:
                                    BEGIN(Constant);
                                    break;
                                  default:
                                    BEGIN(INITIAL);
                                    break;
                                  }
				  break;
                                }
<Comment>[^*(]*			|
<Comment>\(+[^*(]*		|
<Comment>\*+[^*()]*		{
				  /*
				   *  These three rules eat:
				   *    -- anything that's not a "*" or a "("
				   *    -- "("s not followed by "*"
				   *    -- "*"s not followed by ")"
				   *  `anything' includes newlines.
				   *  Print if ShowComments is TRUE and
                                   *  MatchedToken is not NONE
				   */
				  if( ShowComments && MatchedToken!=NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}


	/*
	 *  Code to handle { Text in Braces }
	 *
	 *  "{" puts us into the BracedText state.  Braces nest, so
	 *  in the BracedText state we need to look for "{" that increases
	 *  the nesting level and "}" that will lower it.
	 */

<INITIAL>\{			|
<Model>\{			|
<Constant>\{			{
				  /*  A "{" puts us into the BracedText state.
				   *  Initialize the nesting level.
				   *  Print if MatchedToken is not NONE.
				   *
				   */
				  BEGIN(BracedText);
				  BracesNestLevel = 1;
				  if( MatchedToken != NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<BracedText>[^\\{}]*\\.		{
				  /*  A backslash \ in the BracedText state
				   *  protects any character---even a
				   *  newline---and does not affect the
				   *  Nesting Level.
				   *  Print if MatchedToken is not NONE.
				   */
				  if( MatchedToken != NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<BracedText>[^\\{}]*\\\n	{
				  /*  A backslash \ in the BracedText state
				   *  protects any character---even a
				   *  newline---and does not affect the
				   *  Nesting Level.
				   *  Print if MatchedToken is not NONE.
				   */
				  if( MatchedToken != NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<BracedText>\{[^\\{}]*		{
				  /*  A "{" in the braces state gets added to
				   *  the text and increase the nesting level.
				   *  Match any non-brace character---including
				   *  newlines---that follows the open brace.
				   *  Print if MatchedToken is not NONE.
				   */
				  BracesNestLevel++;
				  if( MatchedToken != NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}
<BracedText>[^\\{}]*\}		{
				  /*  A "}" will reduce the nesting level.
				   *  If the nesting level is zero, go back to
				   *  the previous state.  Match any
				   *  non-brace character---including
				   *  newlines---that precedes the close brace.
				   *  Print if MatchedToken is not NONE.
				   */
				  BracesNestLevel--;
				  if (BracesNestLevel > 0) {
                                    /* more braced text */
                                    break;
                                  }
                                  if( MatchedToken == NONE ) {
                                    BEGIN(INITIAL);
                                    break;
                                  }
                                  Asc_PutCode( typ_text, typ_out );
                                  switch( MatchedToken ) {
				  case MODEL:
                                    BEGIN(Model);
                                    break;
                                  case CONSTANT:
                                    BEGIN(Constant);
                                    break;
                                  default:
                                    BEGIN(INITIAL);
                                    break;
                                  }
				  break;
				}
<BracedText>[^\\{}]*		{
				  /*  Match anything that is not "{" nor "}"
				   *  nor "\\"(backslash).
				   *  `anything' includes newlines.
				   *  Print if MatchedToken is not NONE.
				   */
				  if( MatchedToken != NONE ) {
				    Asc_PutCode( typ_text, typ_out );
				  }
				  break;
				}



	/*
	 *  Code to check an identifier.
	 *
	 *  When we are looking for a MODEL, ATOM, CONSTANT, or DEFIITION
	 *  and we find the text [UNIVERSAL]{MODEL|ATOM|CONSTANT|DEFINITION},
	 *  we enter this state.
         *
         *  When we are insided MODEL, ATOM, or DEFINITION and we find
	 *  the text END, we enter this state.
	 *
	 *  This state checks to see if the text following the MODEL,
	 *  ATOM, CONSTANT, DEFINITION, or END keyword matches
	 *  `reqType'---the type requested by the user.  If so, we are
	 *  at the beginning or end of a type, and should change states
	 *  accordingly.  If not, we should stay in our current state.
	 */

<CheckID>{ident}		{
				  /*
				   *  Found an identifier.
				   *  Identify the correct state.
				   */
				  if( MatchedToken == MODEL_END ) {
				    /*
				     *  We're in the Model state, print the
				     *  text and see if matches reqType, if
				     *  so, we're at the end of the MODEL
				     *  (or ATOM or DEFINITION)
				     *  and should exit typ_lex()---return to
				     *  our caller, else stay in the Model
				     *  state.
				     */
				    Asc_PutCode( typ_text, typ_out );
				    if(strcmp((typ_text+StartId),reqType) == 0) {
				      return 0;
				    } else {
                                      MatchedToken = MODEL;
				      BEGIN(Model);
				    }
                                    break;
				  }
                                  if(strcmp((typ_text+StartId),reqType)==0){
				    /*
				     *  We're in the INITIAL state, and we
				     *  found the beginning of the
				     *  requested MODEL, ATOM, CONSTANT,
				     *  or DEFINITION.
				     *  Print the text and set OutputState to
				     *  say we are printing.  Start the
				     *  appropriate State by checking
				     *  MatchedToken.
				     */
				    Asc_PutCode( typ_text, typ_out );
                                    switch( MatchedToken ) {
				    case MODEL:
				      BEGIN(Model);
                                      break;
				    case CONSTANT:
				      BEGIN(Constant);
                                      break;
                                    default:
				      /* something is wrong */
				      BEGIN(INITIAL);
                                      break;
				    }
                                    break;
				  }
                                  /*
                                   *  We're in the INITIAL state; we found
                                   *  an MODEL, ATOM, CONSTANT, or
                                   *  DEFINITION, but it wasn't the right
                                   *  one.  Stay in the INITIAL state.
                                   */
                                  MatchedToken = NONE;
                                  BEGIN(INITIAL);
				  break;
				}
<CheckID>.			{
				  /*
				   *  Some text other than an identifier
				   *  was found.  Print the text if
				   *  MatchedToken is not NONE,
				   *  and return to the correct state.
				   */
				  if( MatchedToken == NONE ) {
                                    BEGIN(INITIAL);
                                    break;
                                  }
                                  Asc_PutCode( typ_text, typ_out );
                                  switch( MatchedToken ) {
                                  case MODEL:
				    BEGIN(Model);
                                    break;
				  case CONSTANT:
				    BEGIN(Constant);
                                    break;
                                  default:
                                    /* something is wrong */
                                    BEGIN(INITIAL);
                                    break;
				  }
				  break;
				}



	/*
	 *  Code to handle the model/atom/definition we want.
	 *
	 *  Once we've found the start of the matching MODEL, ATOM, or
         *  DEFINITION, print the text until we find the END token.
         *  When we find END, save the current length of typ_text and use
         *  MY_YYMORE() to append the identifier, then check the identifier
         *  (in the CheckID state) to see if it is what we want.  If so,
	 *  this is the end of this MODEL, ATOM, or DEFINITION.
	 */

<Model>{end}			{
				  /*
				   *  Found the END keyword.  Save the
				   *  current location in the typ_text array,
				   *  then enter the CheckID state to see if
				   *  the identifier matches what we want.
				   */
				  StartId = typ_leng;
                                  MatchedToken = MODEL_END;
				  BEGIN(CheckID);
				  MY_YYMORE();
				  break;
				}
<Model>[^({A-Z]+		|
<Model>\(+[^*({A-Z]*		|
<Model>[A-Z_]+;?{ws}*		{
				  /*
				   *  These rules match
				   *    -- any character except that which
				   *       would cause us to change states
				   *    -- "("s not followed by "*"s
				   *    -- uppercase keywords and following
				   *       semicolon or whitespace.
				   *  Print the text.
				   */
				  Asc_PutCode( typ_text, typ_out );
				  break;
				}



	/*
	 *  Code to handle the constant definition we want.
	 *
	 *  Once we've found the start of the matching CONSTANT, print
	 *  the text until we find a semicolon ";".
	 */

<Constant>;			{
				  /*
				   * Found the ";" which ends the CONSTANT.
				   * Do NOT print it since that will be
				   * added below.  Return 0.
				   */
				   return 0;
				}
<Constant>[^({;]+		|
<Constant>\(+[^*({;]*		{
				  /*
				   *  These rules match
				   *    -- any character except that which
				   *       would cause us to change states
				   *    -- "("s not followed by "*"s
				   *  Print the text.
				   */
				  Asc_PutCode( typ_text, typ_out );
				  break;
				}



	/*
	 *  Code to look for [UNIVERSAL ]{MODEL|ATOM|CONSTANT|DEFINITION}.
	 *
	 *  If we find UNIVERSAL, use yymore() to append the next
	 *  keyword---probably MODEL, ATOM, or CONSTANT.
	 *  If we find MODEL, ATOM, CONSTANT, or DEFINITION save the
	 *  current length and use yymore() to append the identifier,
	 *  then check the identifier (in the CheckID state) to see if
	 *  it is what we're looking for.
	 */

<INITIAL>{univ}			{
				  /*
				   *  Found the UNIVERSAL keyword.  Append
				   *  the following ATOM/MODEL/CONSTANT
				   *  keyword.
				   */
				  MY_YYMORE();
				  break;
				}
<INITIAL>{model}		|
<INITIAL>{atom}			|
<INITIAL>{definition}		{
				  /*
				   *  Found an MODEL, ATOM, or DEFINITION
				   *  keyword--perhaps with a UNIVERSAL
				   *  modifier.  Save the current location
				   *  in the typ_text array, then enter the
				   *  CheckID state to see if the identifier
                                   *  matches what we want.
				   */
				  StartId = typ_leng;
				  MatchedToken = MODEL;
				  BEGIN(CheckID);
				  MY_YYMORE();
				  break;
				}
<INITIAL>{constant}		{
				  /*
				   *  Found a CONSTANT keyword--perhaps
				   *  with a UNIVERSAL modifier.  Save the
				   *  current location in the typ_text array,
				   *  then enter the CheckID state to see if
				   *  the identifier matches what we want.
				   */
				  StartId = typ_leng;
				  MatchedToken = CONSTANT;
				  BEGIN(CheckID);
				  MY_YYMORE();
				  break;
				}



	/*
         *  Rules to match other text.
         */


<INITIAL>[^({A-Z]+		|
<INITIAL>\(+[^*({A-Z]*		|
<INITIAL>[A-Z_]+;?{ws}*		{
				  /*
				   *  These rules match
				   *    -- any character except that which
				   *       would cause us to change states
				   *    -- "("s not followed by "*"s
				   *    -- uppercase keywords and following
				   *       semicolon or whitespace.
				   *  Do nothing.
				   */
				  break;
				}
<INITIAL><<EOF>>		{
				  /*
				   *  Reached End of file without a match
				   */
				  return 1;
				}


%%
/*
 *  int typ_wrap(void);
 *
 *  This returns 1 if the scanner should stop parsing, or 0 if
 *  the scanner should continue.  Flex requires this function
 *  unless the flex directive `%option nozzwrap' is given, but
 *  `%option' directives are a recent addition to flex and for
 *  maximum portability should not be used.
 */
int typ_wrap(void)
{
  return 1;
}


#ifndef TYPER_TEST
STDHLF(Asc_ExtractType,(Asc_ExtractTypeHL1,Asc_ExtractTypeHL2,HLFSTOP));
#endif

/*
 *  int Asc_ExtractType(cdata, interp, argc, argv)
 *      ClientData cdata;    --Tcl information, not used
 *      Tcl_Interp *interp;  --Tcl interpreter, not used in standalone use
 *      int argc;            --the number of arguments
 *      char **argv;         --the array of arguments
 *
 *  When running as part of ASCEND, returns TCL_OK or TCL_ERROR.
 *  When running as a stand alone tool (CPP macro TYPER_TEST is defined)
 *  we return:
 *    0  --success
 *    1  --problems with arguments
 *    2  --problems opening file
 *   -1  --no errors occurred but the requested type was not found
 *
 */
extern int Asc_ExtractType(ClientData cdata, Tcl_Interp *interp,
                           int argc, CONST84 char **argv)
{
  int ndx = 1;   /* index into the argv array */
  int result;    /* result from typ_lex and value to return to caller */
#ifndef TYPER_TEST
  struct TypeDescription *desc=NULL;
  struct module_t *mod=NULL;
  CONST char *scanstring=NULL;
  YY_BUFFER_STATE oldbuf=NULL;
  YY_BUFFER_STATE scanbuf=NULL;
#endif /*!typertest*/

  (void)cdata;   /* stop gcc whining about unused parameter */
#ifdef TYPER_TEST
  (void)interp;  /* stop gcc whining about unused parameter */
#endif


  /*
   *  Reset our global set
   */
  BEGIN( INITIAL );
  MatchedToken = NONE;
  ShowComments = 0;
  reqType = "";
  CommentNestLevel = 0;
  BracesNestLevel = 0;


  /*
   *  Process the arguments
   */
  if (( argc < 2 ) || ( argc > 5 )) {
#ifdef TYPER_TEST
    FPRINTF(ASCERR,
            "Wrong number of arguments\n"
            "Usage: %s [-c] type [source_file] [destination_file]\n",
            argv[0]);
    return 1;
#else /* ! TYPER_TEST */
    Tcl_AppendResult(interp, "Wrong number of arguments\nUsage: ",
		     argv[0], "[-c] type [source_file] "
                     "[-s,destination_file]",
		     (char*)NULL);
    return TCL_ERROR;
#endif /* TYPER_TYPER */
  }

  if(( argc > ndx ) && ( argv[ndx][0] == '-' )) {
    switch( argv[ndx][1] ) {
    case 'c':
      ShowComments = 1;
      ndx++;
      break;
    default:
#ifdef TYPER_TEST
      FPRINTF(ASCERR, "Unknown switch %s\n", argv[ndx]);
      return 1;
#else /* ! TYPER_TEST */
      Tcl_AppendResult(interp, "Unknown switch ", argv[ndx], (char*)NULL);
      return TCL_ERROR;
#endif /* TYPER_TEST */
    }
  }

  if( argc > ndx ) {
    /*  The type of MODEL/ATOM to get  */
    reqType = QUIET(argv[ndx++]);
  } else {
#ifdef TYPER_TEST
    FPRINTF(ASCERR,
            "Wrong number of arguments\n"
            "Usage: %s [-c] type [source_file] [destination_file]\n",
            argv[0]);
    return 1;
#else /* ! TYPER_TEST */
    Tcl_AppendResult(interp, "Wrong number of arguments\nUsage: ",
		     argv[0], "[-c] type [source_file] "
                     "[-s,destination_file]",
		     (char*)NULL);
      return TCL_ERROR;
#endif /* TYPER_TEST */
  }

  if( argc > ndx ) {
    /*  The source file; stdin if not specified. */
#ifndef TYPER_TEST
    /* in ascend, find source module if possible for string module name */
    desc = FindType(AddSymbol(reqType));
    if (desc != NULL) {
      mod = GetModule(desc);
      assert(mod!=NULL);
      scanstring = Asc_ModuleString(mod);
      if (scanstring == NULL) {
        mod = NULL;
      }
    }
#endif /*!typertest*/
    if(scanstring == NULL && (typ_in = fopen(argv[ndx],"r")) == NULL) {
#ifdef TYPER_TEST
      FPRINTF(ASCERR, "Error opening source file \'%s\'\n", argv[ndx]);
      return 2;
#else /* ! TYPER_TEST */
      Tcl_AppendResult(interp, "Error opening source file \'",
                       argv[ndx], "\'", (char*)NULL);
      return TCL_ERROR;
#endif /* TYPER_TEST */
    }
    ndx++;
  } else {
    typ_in = stdin;
  }

  if( argc > ndx ) {
#ifndef TYPER_TEST
    if (argv[ndx][0] == '-' && argv[ndx][1] == 's') {
      g_typ__interp = interp;
      typ_out = NULL;
    } else {
#endif /* !typertest*/
      g_typ__interp = NULL;
      /*  The destination file; stdout if not specified  */
      if((typ_out = fopen(argv[ndx],"a+")) == NULL) {
#ifdef TYPER_TEST
        FPRINTF(ASCERR, "Error opening destination file \'%s\'\n", argv[ndx]);
        return 2;
#else /* ! TYPER_TEST */
        Tcl_AppendResult(interp, "Error opening destination file \'",
                         argv[ndx], "\'", (char*)NULL);
        return TCL_ERROR;
#endif /* TYPER_TEST */
      }
#ifndef TYPER_TEST
    }
#endif /* !typertest*/
    ndx++;
  } else {
    typ_out = stdout;
  }


  /*
   *  Call typ_lex() to process the input
   */
#ifndef TYPER_TEST
  if (scanstring != NULL) {
    oldbuf = YY_CURRENT_BUFFER;
    scanbuf = typ__scan_string(scanstring);
    typ__switch_to_buffer(scanbuf);
    BEGIN(INITIAL);
    /* typ_restart((FILE *)NULL); */
  }
#else
  typ_restart(typ_in);
#endif /* !typertest */
  if( (result = typ_lex()) != 0 ) {
#ifdef TYPER_TEST
    FPRINTF(ASCERR, "Could not find type \'%s\'\n", reqType);
#else /* ! TYPER_TEST */
    if (g_typ__interp!=NULL) {
      Tcl_ResetResult(interp);
    }
    Tcl_AppendResult(interp, "Could not find type \'", reqType, "\'",
                     (char*)NULL);
    result = TCL_ERROR;
#endif /* TYPER_TEST */
  } else {
    /* add a closing semicolon and newline */
    Asc_PutCode( ";\n", typ_out );
#ifndef TYPER_TEST
    result = TCL_OK;
#endif /* ! TYPER_TEST */
  }

  /*
   *  Close any files/buffers we opened and exit.
   */
#ifndef TYPER_TEST
  if (scanstring != NULL) {
    typ__delete_buffer(YY_CURRENT_BUFFER);
    typ__switch_to_buffer(oldbuf);
    BEGIN(INITIAL);
  }
#endif
  if ( typ_in != stdin && typ_in != NULL) {
    fclose(typ_in);
  }
  if ( typ_out != stdout && typ_out != NULL) {
    fclose(typ_out);
  }

  return result;
}

static void Asc_PutCode(char *s,FILE *fp)
/** into string or screen */
{
#ifndef TYPER_TEST
  if (g_typ__interp != NULL) {
    Tcl_AppendResult(g_typ__interp,s,(char *)NULL);
  } else {
#endif /* typertest */
    fputs(s,fp);
#ifndef TYPER_TEST
  }
#endif /* typertest */
}

#ifdef TYPER_TEST
int main(int argc, char **argv)
{
  return Asc_ExtractType((ClientData)NULL, (Tcl_Interp*)NULL, argc, argv);
}
#endif /*  TYPER_TEST  */
