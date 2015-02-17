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
*//**
	@file
	Data Reader implementation for the CSV format.

	These functions implement a reader interface generic comma separated values
	ASCII files. Decimal points for numbers must not be denominated by commas and
	the number of columns in the file is assumed to be that of the first line.
*//*
	by Jose Zapata, Aug 2009
*/

#include <stdio.h>
#include <stdlib.h> //atof() doesnt work without this but gcc doesnt complain?
//#include <math.h>
#include <string.h>
/* #include <libradtran/sun.h> */
//#include "sun.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>

#include "csv.h"
#define CSV_DEBUG 0
int datareader_csv_header(DataReader *d) {
    char str[9999];
    char key[] = "abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVXYZ";//e is ommited
    //just in case data is IEEE floating point
    char *tok;
    int ncols = 0;
    boolean isHeader = FALSE; //assume first line has data
    int ndata = 0;
    while (!feof(d->f)) {
        if (fscanf (d->f, "%9998s",str) ==0) {
            CONSOLE_DEBUG("No Data reading CSV file");
            return 1;
        }
        if (!feof(d->f)) ndata++;
    }

    rewind(d->f);//return to start of the file for column number detection

    if (fscanf (d->f, "%9998s",str) == 0) { //gather the first data line
        CONSOLE_DEBUG("No Data reading CSV file");
        return 1;
    }

    tok = strtok(str,","); //parse for the number of columns
    while (tok !=NULL) {
        ncols++; //count the number of columns
        if (strpbrk(tok,key) != NULL) { //check for nonnumeric characters
            isHeader = TRUE; //if any letter chars are found, first line is of headers
        }
        tok = strtok(NULL, ",");
    }
    if (!isHeader) {
        rewind(d->f); //if first line has data return pointer to beginning of file
    }

    // set the value of some of the Data Reader parameters
    d->i = 0;
    d->ninputs = 1;
    d->ndata = ndata;
    d->nmaxoutputs = ncols-1; //as parsed

    typedef double csvPoint[d->nmaxoutputs+1]; //allocate array data
    d->data = ASC_NEW_ARRAY(csvPoint,d->ndata);
    return 0;
}

int datareader_csv_eof(DataReader *d) {
    if (feof(d->f)) {
        CONSOLE_DEBUG("REACHED END OF FILE");
        ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Read: %d rows",d->ndata);
        return 1;
    }
    return 0;
}
int datareader_csv_data(DataReader *d) {
    char str[9999];
    double *csv = ASC_NEW_ARRAY(double, d->nmaxoutputs+1);
    char *tok;
    int k = 0;
#if CSV_DEBUG
    CONSOLE_DEBUG("Reading row %d",d->i);
#endif

    if (fscanf (d->f, "%9998s",str) == 0) { //copy the csv line to str
        CONSOLE_DEBUG("No Data reading CSV file");
        return 1;
    }
    char *str2 = ASC_NEW_ARRAY(char,strlen(str)+1);
    strcpy(str2,str);//create a copy of the string, strtok is not a nice function

    tok = strtok(str2,","); //parse for data points
    while (tok !=NULL) {
        csv[k] = atof(tok);//assign values to data points
        k++;
        tok = strtok(NULL, ",");
    }

    ASC_FREE(str2); //done with the string copy, free that memory

    if (k != d->nmaxoutputs+1) { //check if the number of colums vary in the data set
        CONSOLE_DEBUG("Bad input data in data row %d, %d columns when expecting %d",d->i,k,d->nmaxoutputs+1);
        ASC_FREE(csv);
        return 1;
    }

    if (d->i >= d->ndata)return 0; //this handles an unsolved problem with feof(d->f)

    for (k = 0; k<= d->nmaxoutputs;k++) { //write values in datareader data array
        *((double *)d->data + d->i*(d->nmaxoutputs+1) + k) = csv[k];
#if CSV_DEBUG
        CONSOLE_DEBUG("[%d]-[%d]:%f",d->i,k,
                      *((double *)d->data + d->i*(d->nmaxoutputs+1) + k));
#endif
    }


    d->i++;//set index for next data row
    ASC_FREE(csv);
    return 0;
}

int datareader_csv_time(DataReader *d, double *t) {
    /*  Warining:

    	There is no check in place to verify the user has specified
    	a csv file whose first column is time format or that such value
    	is scaled to seconds.
    	@TODO handle different time formats, e.g. hh:mm:ss.ddd
    */

    *t = *((double *)d->data + d->i*(d->nmaxoutputs+1));
    return 0;
}

int datareader_csv_vals(DataReader *d, double *v) {
    int i;

    for (i = 1;i <= d->nmaxoutputs;i++) {
        v[i-1]=*((double *)d->data + d->i*(d->nmaxoutputs+1) + i) ;
#if CSV_DEBUG
        CONSOLE_DEBUG("At d->i=%d, v[%d] = %lf",d->i,i-1,
                      *((double *)d->data + d->i*(d->nmaxoutputs+1) + i));
#endif
    }
    return 0;
}
