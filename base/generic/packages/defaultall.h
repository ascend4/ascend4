/*	ASCEND modelling environment
	Copyright (C) 1998 Carnegie Mellon University
	Copyright (C) 2006, 2007 Carnegie Mellon University

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
*//**
	@file
	Variable default-value routines, for implementation of METHOD default_all.
*//*
	by John Pye, Feb 2007
*/

#ifndef ASC_DEFAULTALL_H
#define ASC_DEFAULTALL_H

#include <compiler/compiler.h>
#include <general/list.h>
#include <compiler/instance_enum.h>

int Asc_DefaultSelf(struct Instance *root, struct gl_list_t *arglist, void *userdata);
int Asc_DefaultAll(struct Instance *root, struct gl_list_t *arglist, void *userdata);

#endif /* ASC_DEFAULTALL_H */
