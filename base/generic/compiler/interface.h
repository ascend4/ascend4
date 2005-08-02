/*
 *  Interface Include File
 *  by Tom Epperly
 *  Created: 1/17/90
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: interface.h,v $
 *  Date last modified: $Date: 1997/07/18 12:30:58 $
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
 *  Interface Include File.
 *  <pre>
 *  When #including interface.h.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *  </pre>
 */

#ifndef __INTERFACE_H_SEEN__
#define __INTERFACE_H_SEEN__

extern void Interface(void);
/**< 
 *  Initialize compiler interface.
 *  Takes no parameters.
 */

#endif /* __INTERFACE_H_SEEN__ */

