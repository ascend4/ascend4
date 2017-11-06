/*	ASCEND modelling environment
	Copyright (C) 1997, 2009 Carnegie Mellon University

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
*/
#ifndef ASC_MEM_H
#define ASC_MEM_H

#include <ascend/general/pool.h>

#define mem_address pool_address
#define mem_code_address pool_code_address
#define mem_move_cast pool_move_cast
#define mem_copy_cast pool_copy_cast
#define mem_repl_byte_cast pool_repl_byte_cast
#define mem_zero_byte_cast pool_zero_byte_cast
#define mem_repl_word_cast pool_repl_word_cast

#define mem_store_t pool_store_t
#define mem_statistics pool_statistics
#define mem_get_stats pool_get_stats
#define mem_create_store pool_create_store
#define mem_get_element pool_get_element
#define mem_get_element_list pool_get_element_list

#define MEM_DEBUG POOL_DEBUG
#define mem_LIGHTENING pool_LIGHTENING

#define mem_free_element pool_free_element
#define mem_clear_store pool_clear_store

#define mem_destroy_store pool_destroy_store
#define mem_print_store pool_print_store
#define mem_sizeof_store pool_sizeof_store

/* @} */

#endif  /* ASC_MEM_H */

