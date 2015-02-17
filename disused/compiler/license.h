/*
 *  Text of the GNU Public License
 *  by Tom Epperly
 *  Created: 1 February 1994
 *  Version: $Revision: 1.4 $
 *  Version control file: $RCSfile: license.h,v $
 *  Date last modified: $Date: 1997/07/18 12:31:11 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1994 Thomas Guthrie Epperly
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
 *  Text of the GNU Public License.
 *  <pre>
 *  When #including .h, make sure these files are #included first:
 *         NO INCLUDES NEEDED
 *  </pre>
 */

#ifndef __LICENSE_H_SEEN__
#define __LICENSE_H_SEEN__

extern const char GPL1[];
/**< String containing part 1 of the GNU Public License, version 2 text. */
extern const char GPL2[];
/**< String containing part 2 of the GNU Public License, version 2 text. */
extern const char GPL3[];
/**< String containing part 3 of the GNU Public License, version 2 text. */
extern const char GPL4[];
/**< String containing part 4 of the GNU Public License, version 2 text. */
extern const char GPL5[];
/**< String containing part 5 of the GNU Public License, version 2 text. */

extern const char * const NO_WARRANTY;
/**< 
 *  String containing the no warranty section of the GNU Public License
 *  version 2.  This is a pointer to the last GPL.
 */

#endif /* __LICENSE_H_SEEN__ */

