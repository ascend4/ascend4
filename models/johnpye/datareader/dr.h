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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Data Reader implementation

	Although the API allows for functions of more than one variable, the
	implementation I'll build here will be for functions of just one indep
	variable, since the application here is time series, eg weather data.
*//*
	by John Pye, 3 Aug 2006
*/

#ifndef DATAREADER_H
#define DATAREADER_H

/* DATA READER API DEFINITION */

struct DataReader;

int datareader_new(const char *fn);
int datareader_init(DataReader *d);
int datareader_set_input_filter(DataReader *d, InputFilterFn *iff);
int datareader_delete(DataReader *d);

int datareader_num_inputs(const DataReader *d);
int datareader_num_outputs(const DataReader *d);

int datareader_func(DataReader *d, double *inputs, double *outputs);
int datareader_deriv(DataReader *d, double *inputs, double *jacobian);


/**
	An input filter function must be able to return a row of data by reading the
	file at the current point. Returns 0 on success.
*/
typedef int (InputFilterFn)(FILE *f, double *inputs, double *ouputs);


#endif
