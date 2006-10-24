/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
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
*//** @file
	Module for storing braced text of notes and some other 
	BRACEDTEXT_T applications.

	What we're calling "braced text" is just a regular C char* augmented with a 
	language attribute and string length, and with reference counting.
	
	Not everything that is defined as BRACEDTEXT_T in the grammar is necessarily
	kept by using this module. For example, some units END up in the symbol 
	table.
	
	'struct bracechar' is defined first in compiler.h, to hide the details of
	whatever it is we do with braced text in composing NOTES and other
	persistent forms.

	Braced text may be very long.
*//*
	By Benjamin Allan, March 20, 1998.
	Last in CVS:$Revision: 1.5 $ $Date: 1998/06/16 16:38:39 $ $Author: mthomas $
*/

#ifndef ASC_BRACED_H
#define ASC_BRACED_H

#include <utilities/ascConfig.h>
#include "compiler.h"

/**
 * Creates a bracechar from a string. We do not keep the string.
 * If a lang is given, it is kept.
 */
extern struct bracechar *AddBraceChar(CONST char *string, symchar *lang);

/**
 * Increments a reference count and returns bc.
 */
extern struct bracechar *CopyBraceChar(struct bracechar *bc);

/**
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
 * Will not be NULL. To print, wrap in SCP.
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

#endif  /* ASC_BRACED_H */
