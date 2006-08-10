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

#include <stdio.h>

#ifndef ASCEX_DR_H
#define ASCEX_DR_H

/* DATA READER API DEFINITION */

struct DataReader_struct;
typedef struct DataReader_struct DataReader;

typedef enum {
	DATAREADER_FORMAT_TMY2
	,DATAREADER_FORMAT_UNKNOWN
} DataReaderFileFormat;

DataReader *datareader_new(const char *fn);
int datareader_init(DataReader *d);
int datareader_set_file_format(DataReader *d, DataReaderFileFormat format);
int datareader_delete(DataReader *d);

int datareader_num_inputs(const DataReader *d);
int datareader_num_outputs(const DataReader *d);

int datareader_func(DataReader *d, double *inputs, double *outputs);
int datareader_deriv(DataReader *d, double *inputs, double *jacobian);

/**
	Function that can read a single data point from the open file.
	Should return 0 on success.
*/
typedef int (DataReaderDataFn)(DataReader *d);

/**
	Function that can read the file header and allocate necessary memory
*/
typedef int (DataReaderHeaderFn)(DataReader *d);

/**
	Function that returns 1 when the end of file has been reached
*/
typedef int (DataReaderEofFn)(DataReader *d);


/**
	A function that should be called when a file is first opened, optional.
	This function can read header lines and determine the number of columns if
	necessary.

	Return 0 on success.
*/
typedef int (DataHeaderFn)(DataReader *d);

/*------------------------------------------------------------------------------
  DATA STRUCTURES

	Don't use these unless you're writing a new reader format (eg TMY, CSV etc)
*/

/**
	Record where in the file the data for a particular time can be found,
	for fast backtracking.
*/
struct IndexPoint{
	int pos;
	double t;
};

/**
	Structure to hold the data for a particular data point after being loaded.
*/
typedef struct{
	double t;
	double *outputs;
} DataPoint;

/**
	Need some kind of structure here to hold spline data for a particular
	interval.
*/

/**
	Top-level structure for the data reader. Keeps track of the file we're
	working with, the number of columns, etc.
*/
struct DataReader_struct{
	const char *fn;
	struct FilePath *fp;
	FILE *f;
	int noutputs;
	int ndata; /** number of data points in the raw data */
	int i; /** 'current location' in the data array */
	void *data; /**< stored data (form depends on what what loaded) */
	DataReaderHeaderFn *headerfn;
	DataReaderDataFn *datafn;
	DataReaderEofFn *eoffn;
};

#endif

