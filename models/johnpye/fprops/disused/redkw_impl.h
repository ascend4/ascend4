/*	ASCEND modelling environment
	Copyright (C) 2009 Carnegie Mellon University

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
*/
#ifndef REDKW_IMPL_H
#define REDKW_IMPL_H

#include "redkw.h"

/*
	This file contains the headers for the private code definedin 'redkw.c'.
	You shouldn't include this file in your programs, because the implementation
	of the helmholtz curves is 'secret business' of the fprops code.

	We provide this header file just the purpose of diagnostic testing.
*/

double get_alpha(double T, const EosData *d, const FpropsError *err);
double get_a(double T, const EosData *d, const FpropsError *err);
double get_b(const EosData *d, const FpropsError *err);

#endif //REDKW_IMPL_H
