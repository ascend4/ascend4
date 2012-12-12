/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @addtogroup general_hash General String Hashing
	Hashing functions for strings.
*//*
	by Tom Epperly
	10/24/89
	Last in CVS: $Revision: 1.1 $ $Date: 1997/07/18 11:38:39 $ $Author: mthomas $
*/


#ifndef ASC_HASHPJW_H
#define ASC_HASHPJW_H

#include <ascend/general/platform.h>

/**	@addtogroup general_hash General String Hashing
	@{
*/

ASC_DLLSPEC unsigned long hashpjw(CONST char *str, unsigned long size);
/**<
 *  Return a hash value base on str.  The value will be >= 0 and < size.
 *  The specified string may not be NULL, and size must be greater than 0 
 *  (checked by assertion).
 *
 *  @param str  String to use as base for hash (non-NULL).
 *  @param size Maximum value for hash (0 <= hash < size).
 *  @return Returns the hash value.
 */

ASC_DLLSPEC unsigned long hashpjw_int(int id, unsigned long size);
/**<
 *  Return a hash value base on id.  The value will be >= 0 and < size.
 *  The requested size must be greater than 0 (checked by assertion).
 *  This function at the moment does a conversion of the integer to a string
 *  and so is relatively expensive. This needs to be cleaned up, with a
 *  dedicated integer hashing function.
 *
 *  @param id   Integer to use as base for hash.
 *  @param size Maximum value for hash (0 <= hash < size).
 *  @return Returns the hash value.
 *
 *  @todo general/hashpjw.h - Provide integer hashing function (or
 *        possibly remove function as hashpjw_int is not used in ASCEND).
 */

/* @} */

#endif /* ASC_HASHPJW_H */

