/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
*//** @defgroup system_graph System Graph output
	System graph output

	Use variable and relation filters to generate a graph between the selected
	vars and rels, by querying their incidence data. Write the graph in DOT
	format to the provided FILE*.

	by John Pye, March 2007
*/
#ifndef ASC_SYS_GRAPH_H
#define ASC_SYS_GRAPH_H

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include "slv_types.h"
#include "var.h"
#include "rel.h"

/**	@addtogroup system_graph
	@{
*/

/**
	Write graphviz output for the graph connecting the filter-selected relations
	and variables. The idea is that this graph should be creatable even before
	a solver has been selected, which is why it's prefixed with 'system_'.

	@todo FIXME can we move this to a dlopenable optional component?

	@return 0 on success, non-zero on error.
*/
ASC_DLLSPEC int system_write_graph(slv_system_t sys
	, FILE *fp
	, const char *format
);

#endif
