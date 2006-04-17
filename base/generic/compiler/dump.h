/*
 *  Instance Garbage Dump
 *  by Tom Epperly
 *  10/24/89
 *  Version: $Revision: 1.7 $
 *  Version control file: $RCSfile: dump.h,v $
 *  Date last modified: $Date: 1998/02/05 16:35:51 $
 *  Last modified by: $Author: ballan $
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
 *
 *  This is a resting place for un-needed instances.  They may be stored in
 *  the dump in case they are needed later on.  Only pure instance should
 *  be stored in the dump, and the dumping procedure is responsible for
 *  making sure that the instance it "pure", and unconnected to anything
 *  else.  Typically, these restrictions are only met by atoms.
 */

/** @file
 *  Instance Garbage Dump
 *  <pre>
 *  When #including dump.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "instance_enum.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef ASC_DUMP_H
#define ASC_DUMP_H

#define MESSYTHRESH 10

extern void InitDump(void);
/**<
 *  Must be called to initialize the dump.
 */

extern void ASC_DLLSPEC EmptyTrash(void);
/**<
 *  Delete all the instances in the dump.  The dump can still be used
 *  after this call.
 */

extern void TendTrash(void);
/**<
 *  This is a less drastic version of EmptyTrash(), which only deletes
 *  instance with too many copies in the dump.  Too many is more than
 *  MESSYTHRES copies of a given type.
 */

extern void TrashType(symchar *str);
/**<
 *  Delete any copies of type 'str' from the dump.  This should be called
 *  if for instance the definition of 'str' is being changed.  In such
 *  cases, fetching copies of 'str' from the dump would return the old
 *  version.<br><br>
 *
 *  str should have no leading blanks and should be a valid identifier.
 */

extern void AddInstance(struct Instance *i);
/**<
 *  This adds instance i to the trash dump.
 */

extern struct Instance *FindInstance(symchar *str);
/**<
 *  This will return an instance if it one is available; otherwise, it
 *  returns NULL.
 */

#endif /* ASC_DUMP_H */

