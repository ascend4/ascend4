/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/
#include <utilities/ascConfig.h>

ASC_IMPORT(int) AscDriver(int, CONST char **argv);

#ifdef __WIN32__
static void setargv(int*, char ***);
#endif /* __WIN32__ */

/*
 *  main or WinMain
 *
 *  The main entry point for a Unix or Windows application.
 *
 *  Each just calls AscDriver().
 *  These are based on functions from the Tk 8.0 distribution.
 *  See unix/tkAppInit.c and win/winMain.c in their sources.
 */
#ifndef __WIN32__

int main(int argc, CONST char *argv[])
{
  AscDriver(argc, argv);
  return 0;
}

#else /* __WIN32__ */

#include <windows.h>
#include <locale.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpszCmdLine, int nCmdShow)
{
  int argc;
  char **argv;
  char *p;
  char buffer[MAX_PATH];

  UNUSED_PARAMETER(hInstance);
  UNUSED_PARAMETER(hPrevInstance);
  UNUSED_PARAMETER(lpszCmdLine);
  UNUSED_PARAMETER(nCmdShow);

  /*
   * Set up the default locale to be standard "C" locale so parsing
   * is performed correctly.
   */
  setlocale(LC_ALL, "C");

  /*
   * Increase the application queue size from default value of 8.
   * At the default value, cross application SendMessage of WM_KILLFOCUS
   * will fail because the handler will not be able to do a PostMessage!
   * This is only needed for Windows 3.x, since NT dynamically expands
   * the queue.
   */
  /*
  	RIP WIndows 3.1
  	SetMessageQueue(64);
  */

  /*
   * Create the console channels and install them as the standard
   * channels.  All I/O will be discarded until TkConsoleInit is
   * called to attach the console to a text widget.
   */

  /*
   *  Windows expects us to parse our arguments ourselves.
   */
  setargv(&argc, &argv);

  /*
   * Replace argv[0] with full pathname of executable, and forward
   * slashes substituted for backslashes.
   */
  GetModuleFileName(NULL, buffer, sizeof(buffer));
  argv[0] = buffer;
  for (p = buffer; *p != '\0'; p++) {
    if (*p == '\\') {
      *p = '/';
    }
  }

  AscDriver(argc, argv);
  return 1;
}
#endif /* __WIN32__ */

#ifdef __WIN32__
/*
 *-------------------------------------------------------------------------
 *
 * setargv --
 *
 *	Parse the Windows command line string into argc/argv.  Done here
 *	because we don't trust the builtin argument parser in crt0.
 *	Windows applications are responsible for breaking their command
 *	line into arguments.
 *
 *	2N backslashes + quote -> N backslashes + begin quoted string
 *	2N + 1 backslashes + quote -> literal
 *	N backslashes + non-quote -> literal
 *	quote + quote in a quoted string -> single quote
 *	quote + quote not in quoted string -> empty string
 *	quote -> begin quoted string
 *
 * Results:
 *	Fills argcPtr with the number of arguments and argvPtr with the
 *	array of arguments.
 *
 * Side effects:
 *	Memory allocated.
 *
 * Parameters:
 *    argcptr  Filled with number of argument strings.
 *    argvptr  Filled with argument strings (malloc'd).
 *
 * This function is from the Tk 8.0 distribution.  See win/winMain.c in
 * their sources.
 *
 *--------------------------------------------------------------------------
 */
static void
setargv(int *argcPtr, char ***argvPtr)
{
    char *cmdLine, *p, *arg, *argSpace;
    char **argv;
    int argc, size, inquote, copy, slashes;

    cmdLine = GetCommandLine();

    /*
     * Precompute an overly pessimistic guess at the number of arguments
     * in the command line by counting non-space spans.
     */

    size = 2;
    for (p = cmdLine; *p != '\0'; p++) {
	if (isspace(*p)) {
	    size++;
	    while (isspace(*p)) {
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }
    argSpace = (char *) malloc((unsigned) (size * sizeof(char *) + strlen(cmdLine) + 1));
    argv = (char **) argSpace;
    argSpace += size * sizeof(char *);
    size--;

    p = cmdLine;
    for (argc = 0; argc < size; argc++) {
	argv[argc] = arg = argSpace;
	while (isspace(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    break;
	}

	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if ((inquote) && (p[1] == '"')) {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
                }
                slashes >>= 1;
            }

            while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }

	    if ((*p == '\0') || (!inquote && isspace(*p))) {
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
        }
	*arg = '\0';
	argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}

#endif /* __WIN32__ */
