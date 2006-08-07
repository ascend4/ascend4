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

typedef enum datareader_file_format_enum{
	DATAREADER_FORMAT_TMY2
	,DATAREADER_FORMAT_UNKNOWN
} datareader_file_format_t;

int datareader_new(const char *fn);
int datareader_init(DataReader *d);
int datareader_set_file_format(DataReader *d, const datareader_file_format_t &format);
int datareader_delete(DataReader *d);

int datareader_num_inputs(const DataReader *d);
int datareader_num_outputs(const DataReader *d);

int datareader_func(DataReader *d, double *inputs, double *outputs);
int datareader_deriv(DataReader *d, double *inputs, double *jacobian);


/**
	Function that can read a single data point from the open file.
	Should return 0 on success.
*/
typedef int (DataReaderReadFn)(DataReader *d);

/**
	Function that can read the file header and allocate necessary memory
*/
typedef int (DataReaderHeaderFn)(DataReader *d);


/**
	A function that should be called when a file is first opened, optional.
	This function can read header lines and determine the number of columns if
	necessary.

	Return 0 on success.
*/
typedef int (DataHeaderFn)(DataRader *d);

#endif
