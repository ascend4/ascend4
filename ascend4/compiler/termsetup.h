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

#ifndef __TERMSETUP_H_SEEN__
#define __TERMSETUP_H_SEEN__

extern int OutputChar();
extern void DeleteBackOne();
extern void ClearScreen();
extern void Bell();
extern void ClearLine();
extern void SetupTermcapStuff();
extern void InterfaceError();
extern void SetupTerminal();
extern void RestoreTerminal();
extern void TermSetup_ResetTerminal();
extern void ReadString();

#endif /* __TERMSETUP_H_SEEN__ */
