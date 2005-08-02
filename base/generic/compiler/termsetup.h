/*
 *  Interface Implementation - terminal setup
 *  by Tom Epperly
 *  Created: 1/17/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: termsetup.h,v $
 *  Date last modified: $Date: 1997/07/18 12:35:29 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1990, 1993, 1994 Thomas Guthrie Epperly
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.
 */

/** @file
 *  Interface Implementation - terminal setup.
 *  <pre>
 *  When #including termsetup.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 *  @todo Complete documentation of termsetup.h.
 */

#ifndef __TERMSETUP_H_SEEN__
#define __TERMSETUP_H_SEEN__

extern int  OutputChar(char c);               /**< Print c and return it. */
extern void DeleteBackOne(void);              /**< Print backspace. */
extern void ClearScreen(void);                /**< Clear the screen. */
extern void Bell(void);                       /**< Print bell. */
extern void ClearLine(void);                  /**< Clear the line. */
extern void SetupTermcapStuff(void);          /**< Setup terminal characteristics. */
extern void InterfaceError(void);             /**< Called when an error occurs. */
extern void SetupTerminal(void);              /**< Setup terminal. */
extern void RestoreTerminal(void);            /**< Restore terminal settings. */
extern void TermSetup_ResetTerminal(void);    /**< Reset the terminal. */
extern void ReadString(char *str, int *len);  /**< Read a string of length len. */

#endif /* __TERMSETUP_H_SEEN__ */

