/*
 * tkConsole.h --
 *
 *	This file implements a Tcl console for systems that may not
 *	otherwise have access to a console.  It uses the Text widget
 *	and provides special access via a console command.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "TclTk.license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkConsole.c 1.51 97/04/25 16:52:39
 */

/** @file
 *	This file implements a Tcl console for systems that may not
 *	otherwise have access to a console.  It uses the Text widget
 *	and provides special access via a console command.<br><br>
 *
 *  TkConsoleCreate() and TkConsoleInit() are not exported functions
 *  in the Tcl/Tk 8.0 distribution.  We've pulled them into our
 *  application because we need some sort of command line when running
 *  ASCEND under Windows.<br><br>
 *
 *  To build with the TkConsole, you'll need to
 *    - define the CPP macro ASC_USE_TK_CONSOLE
 *    - make sure the ``generic'' subdirectory in the
 *      Tk source distribution is on your include path.
 */

#ifndef _TKCONSOLE_H
# define _TKCONSOLE_H

# ifndef ASC_USE_TK_CONSOLE
#  ifdef __WIN32__
/* #   define ASC_USE_TK_CONSOLE */
#  endif
# endif

extern void TkConsoleCreate(void);
/**<
 * <!--  TkConsoleCreate --                                            -->
 *
 *  Create the console channels and install them as the standard
 *  channels.  All I/O will be discarded until TkConsoleInit is
 *  called to attach the console to a text widget.<br><br>
 *
 *  Results: None.
 *
 *  Side effects:  Creates the console channel and installs it as
 *                 the standard channels.
 */

extern int TkConsoleInit(Tcl_Interp *interp);
/**<
 * <!--  int TkConsoleInit(interp);                                    -->
 * <!--      Tcl_Interp *interp;	// Interpreter to use for prompting.  -->
 *
 *  Initialize the console.  This code actually creates a new
 *  application and associated interpreter.  This effectivly hides
 *  the implementation from the main application.
 *
 *  Results:  None.
 *
 *  Side effects:  A new console it created.
 */

#endif  /* _TKCONSOLE_H */

