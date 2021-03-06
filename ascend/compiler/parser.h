/*	ASCEND modelling environment
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASC_PARSER_H
#define ASC_PARSER_H

#include <ascend/general/platform.h>

/**	@addtogroup compiler_parse Compiler Scanner/Parser
	@{
*/

/* ascParse.y supplies this function. */
ASC_DLLSPEC int zz_parse();
extern int zz_lex();
	
#endif
