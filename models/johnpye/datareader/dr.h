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

DataReader *datareader_new(const char *fn, int noutputs);
int datareader_init(DataReader *d);
int datareader_set_parameters(DataReader *d, const char *parameters);
int datareader_set_format(DataReader *d, const char *format);
int datareader_delete(DataReader *d);

int datareader_num_inputs(const DataReader *d);
int datareader_num_outputs(const DataReader *d);

int datareader_func(DataReader *d, double *inputs, double *outputs);
int datareader_deriv(DataReader *d, double *inputs, double *jacobian);

double dr_linearinterp(double t, double *t1, double *t2, double v1, double v2);
double dr_cubicinterp(DataReader *d, int j, double t, double *t1, double *t2, double v0, double v1, double v2, double v3);
double dr_linearderiv(double t, double *t1, double *t2, double v1, double v2);
double dr_cubicderiv(DataReader *d, int j, double t, double *t1, double *t2, double v0, double v1, double v2, double v3);
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
	Function that returns current value of dependent variables
	Return 0 on success.
*/
typedef int (DataReaderValFn)(DataReader *d,double *dep);

/**
	A function that returns the current value of independent variables.
	Return 0 on success.
*/
typedef int (DataReaderIndepFn)(DataReader *d,double *indep);

/**
	A function that calculates the cubic spline for all datapoints in the
	data array struct
	@return 0 on sucess.	
*/
typedef int (DataReaderCalcGradFn)(DataReader *d);

/**
	A function that returns the value of the current gradients
	@return 0 on sucess.	
*/
typedef int (DataReaderGradFn)(DataReader *d, double *gradients);


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
	Top-level structure for the data reader. Keeps track of the file we're
	working with, the number of columns, etc.
*/

/* type of interpolation to use at each datareader sample */
typedef enum {
	default_interp, //default interpolation, allow the format handler to decide
	linear, //straight line between sample points
	cubic, //cubic spline sample based on clamped spline algorithm
	sun //special cubic spline for sun data (includes sun position algorithms)
} interp_t;
/** this function parses the parameter format tokens and returns an interpolation type
*/
interp_t datareader_int_type(const char *interpToken);

struct DataReader_struct{
	const char *fn;
	struct FilePath *fp;
	FILE *f;
	int ninputs;
	int noutputs;
	int nmaxoutputs;//maximum number of columns, as per format settings.
	int ndata; /** number of data points in the raw data */
	int i; /** 'current location' in the data array */
	int prev_i_val; //previous location of the data array. For cubic interp value function
	int prev_i_der; //previous location of the data array. For cubic interp deriv function
	void *data; /**< stored data (form depends on what what loaded) */
	//void *grad; /** stored gradients, for each data point a cubic spline gradient is calculated**/
	int *cols; //columns required, as declared in parameter file
	interp_t *interp_t; //interpolation types, as tokenised
	double *a0; //cubic spline coefficient a0 array for all outputs;
	double *a1; //cubic spline coefficient a0 array for all outputs;
	double *a2; //cubic spline coefficient a0 array for all outputs;
	double *a3; //cubic spline coefficient a0 array for all outputs;
	DataReaderHeaderFn *headerfn;
	DataReaderDataFn *datafn;
	DataReaderEofFn *eoffn;
	DataReaderIndepFn *indepfn;
	DataReaderValFn *valfn;
};

#endif

