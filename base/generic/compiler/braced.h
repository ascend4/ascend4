/*
 *  Braced Text Module
 *  By Benjamin Allan
 *  March 20, 1998.
 *  Part of ASCEND
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: braced.h,v $
 *  Date last modified: $Date: 1998/06/16 16:38:39 $
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

/** @file
 *  Module for storing braced text of notes and
 *  some other BRACEDTEXT_T applications.
 *
 *  Not everything that is defined as BRACEDTEXT_T in
 *  the grammar is necessarily kept by using this module.
 *  For example, some units END up in the symbol table.<br><br>
 *
 *  struct bracechar defined first in compiler.h hides the details of
 *  whatever it is we do with braced text in composing
 *  NOTES and other persist forms.
 *  Braced text may be very long.
 *  <pre>
 *  When #including braced.h, make sure these files are #included first:
 *         #include "utilities/ascConfig.h"
 *         #include "compiler.h"
 *  </pre>
 */

#ifndef __BRACED_H_SEEN__
#define __BRACED_H_SEEN__

/**
 * <!--  bc = AddBraceChar(string,lang);                               -->
 * Creates a bracechar from a string. We do not keep the string.
 * If a lang is given, it is kept.
 */
extern struct bracechar *AddBraceChar(CONST char *string, symchar *lang);

/**
 * <!--  cbc = CopyBraceChar(bc);                                      -->
 * Increments a reference count and returns bc.
 */
extern struct bracechar *CopyBraceChar(struct bracechar *bc);

/**
 * <!--  DestroyBraceChar(bc);                                         -->
 * Frees memory associated with bc (subject to refcounting).
 */
extern void DestroyBraceChar(struct bracechar *bc);

/**
 * Returns a string of the bracechar content for READ-ONLY
 * use. Since we frequently want to use this inside
 * printf and the like, a short macro form is provided.
 */
ASC_DLLSPEC(CONST char *) BraceCharString(struct bracechar *sbc);
/** Shortcut to BraceCharString(). */
#define BCS(sbc) BraceCharString(sbc)

/**
 * Returns a 'language' of the bracechar content for READ-ONLY
 * use. Since we frequently want to use this inside
 * printf and the like, a short macro form is provided.
 * will not be NULL. To print, wrap in SCP.
 */
extern symchar *BraceCharLang(struct bracechar *sbc);
/** Shortcut to BraceCharLang(). */
#define BCLANG(sbc) BraceCharLang(sbc)

/**
 * Returns length of the bracechar content for READ-ONLY
 * use. Since we frequently want to use this inside
 * printf and the like, a short macro form is provided.
 * will not be NULL. (This is not the length of lang).
 */
ASC_DLLSPEC(int ) BraceCharLen(struct bracechar *sbc);
/** Shortcut to BraceCharLen(). */
#define BCL(sbc) BraceCharLen(sbc)

#endif  /* __BRACED_H_SEEN__ */

