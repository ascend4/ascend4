/*
 *  anoncopy.h
 *  by Benjamin Allan
 *  September 08, 1997
 *  Part of ASCEND
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: anoncopy.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:35 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check
 *  the file named COPYING.
 */

#ifndef __ANONCOPY_H_SEEN__
#define __ANONCOPY_H_SEEN__
/* */
extern struct gl_list_t *Pass2CollectAnonProtoVars(struct Instance *);

/* */
extern void Pass2DestroyAnonProtoVars(struct gl_list_t *);

/* */
extern void Pass2CopyAnonProto(struct Instance *, struct BitList *,
                               struct gl_list_t *, struct Instance *);

#endif /* __ANONCOPY_H_SEEN__ */
