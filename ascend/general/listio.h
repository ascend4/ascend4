/*
 *  List I/O Routines
 *  by Ben Allan
 *  Created: 12/97
 *  Version: $Revision: 1.2 $
 *  Version control file: $RCSfile: listio.h,v $
 *  Date last modified: $Date: 1998/06/16 15:47:42 $
 *  Last modified by: $Author: mthomas $
 *
 *  This file is part of the ASCEND Language Interpreter
 *
 *  Copyright (C) 1998 Carnegie Mellon University
 *
 *  The ASCEND Language Interpreter is free software; you can
 *  redistribute it and/or modify it under the terms of the GNU
 *  General Public License as published by the Free Software
 *  Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  The ASCEND Language Interpreter is distributed in hope that it
 *  will be useful, but WITHOUT ANY WARRANTY; without even the implied
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __listio_h_seen__
#define __listio_h_seen__

/**	@addtogroup general_list General List
	@{
*/

/** @file
 *  List I/O Routines.
 *  <pre>
 *  Requires:
 *        #include "utilities/ascConfig.h"
 *        #include "general/list.h"
 *  </pre>
 */

/**
 *  Write the data in a list (as ints/pointers) to a file stream.
 *  If fp is NULL, the the listing is to stderr.  The list pointer
 *  may not be NULL (checked by assertion).
 *
 *  @param fp File stream to receive listing (stderr if fp == NULL).
 *  @param l  The gl_list_t to write to fp (non-NULL).
 */
ASC_DLLSPEC void gl_write_list(FILE *fp, struct gl_list_t *l);


/**
	Output a list item. Casts the item to the type required for the list
	in question (it's the user's job to get that right). Then outputs the item.
	@param fp File stream to output to.
	@param item Pointer to the particular item.
	@return As for fprintf: positive on success, equal to the number of chars 
	output. Negative if an error occurred.
*/
typedef int WriteItemFn(FILE *fp, void *item);


/**
	Output an char* list-item. If the pointer is null, "(NULL)" is printed,
	and an error code of -1 is returned. Otherwise the integer is printed.
*/
ASC_DLLSPEC int gl_write_list_item_str(FILE *fp, void *item);


/**
	Write a compact text representation of a list, eg as
	
	  (item1, item2, item3)
	  
	using a specified function to format each item.
	
	each item.
	@param fp File stream for output
	@param l The list to output (non-NULL)
	@param write_item A function of type WriteItemFn that will convert each list item to a string and write it.
*/
ASC_DLLSPEC void gl_write_list_str(FILE *fp, struct gl_list_t *l, WriteItemFn *write_item);

/* @} */

#endif  /* __listio_h_seen__ */

