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
*/
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "dr.h"
#include "tmy.h"
#include "acdb.h"
#include "csv.h"

#include <ascend/utilities/config.h>
#include <ascend/general/ospath.h>
#include <ascend/utilities/ascMalloc.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascPanic.h>
#include <ascend/utilities/ascEnvVar.h>

#define DR_DEBUG 0

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

/*
	declare the possible formats that we will accept ('format' child in the
	DATA instance of the external relation
*/

#define FMTS(D,X) D(TMY2) X D(ACDB) X D(CSV) X D(TDV)

#define ENUM(F_) DATAREADER_FORMAT_##F_
#define COMMA ,
typedef enum {
    FMTS(ENUM, COMMA),
    DATAREADER_INVALID_FORMAT
} DataReaderFormat;
#undef ENUM
#undef COMMA

/*------------------------------------------------------------------------------
  API FUNCTIONS
*/

/**
	Create a data reader object, with the filename specified. The filename
	will be searched for in a specified path, eg ASCENDLIBRARY.
*/
DataReader *datareader_new(const char *fn, int noutputs) {
    DataReader *d;
    int i;

    d = ASC_NEW(DataReader);
    d->fn = fn;
    d->fp = NULL;
    d->f = NULL;
    d->noutputs = noutputs; //maybe this is not the right place to put this!

    //create a data allocation for the parameter list
    d->cols = (int *)ascmalloc(noutputs*sizeof(int));
    d->interp_t = (interp_t *)ascmalloc(noutputs*sizeof(interp_t));
    //initialise param lists with default values. In case user doesn't declare params
    for (i=0;i<noutputs;i++) {
        d->cols[i] = i+1;
        d->interp_t[i] = default_interp;
    }
    d->a0 = (double *)ascmalloc(noutputs*sizeof(double));
    d->a1 = (double *)ascmalloc(noutputs*sizeof(double));
    d->a2 = (double *)ascmalloc(noutputs*sizeof(double));
    d->a3 = (double *)ascmalloc(noutputs*sizeof(double));

    d->datafn = NULL;
    d->headerfn = NULL;
    d->eoffn = NULL;

    CONSOLE_DEBUG("Datareader created...");
    return d;
}
/**read the parameter token and return an interpolation type.
@param interpToken the interpolation string, as declared in the model
@return interp_t the model type
**/

interp_t datareader_int_type(const char *interpToken) {
    interp_t type = default_interp; //set default interpolation, fallback case
    if (strcmp(interpToken,"default")==0) {
        return type; //user has declared default interpolation
    }
    if (strcmp(interpToken,"linear")==0) {
        type = linear;
        return type; //user has declared linear interpolation
    }
    if (strcmp(interpToken,"cubic")==0) {
        type = cubic;
        return type; //usen has declared cubic interpolation
    }
    if (strcmp(interpToken,"sun")==0) {
        type = sun;
        return type; //user has declared sun interpolation
    }
    //if we got here, used did not declare a valid interpolation type
    //return fallback case.

    //CONSOLE_DEBUG("token %s, type %d ", interpToken, type);
    return type;

}

/** Set datareader parameters.
This function reads the parameters as declared in the model file and
fills out the cols and interp_t fields in the datareader structure.

This process happens after datareader creation so that the right amount
of memory is allocated for cols and interp_t. The other reason to create
a separate function is that an integer with the result of this operation
can be returned
@param d the datareader object
@param par the parameter string passed from the drconfig model
@return 0 on sucess
**/
int datareader_set_parameters(DataReader *d, const char *parameters) {
    char *partok = NULL;
    int parcount = 0;
    boolean LastTokWasNumeric = FALSE;//keep track of token types
    partok = strtok(parameters,",:");
    while (partok !=NULL) { //cycle through the parameter string
        if (strpbrk(partok,"1234567890")!=NULL) { //if the token is numeric
            if (LastTokWasNumeric) parcount++; //last was numeric, no interp_t declared
            d->cols[parcount] = atoi(partok);//assign to corresponding element array
            if (d->cols[parcount] > d->nmaxoutputs) {
                CONSOLE_DEBUG("col %d, max %d",d->cols[parcount],d->nmaxoutputs);
                ERROR_REPORTER_HERE(ASC_USER_ERROR,
                "Requested Column out of range!,check your data file and model declaration");
                return 1; //failed due to column out of range
            }
            LastTokWasNumeric =TRUE; //keep track of numeric tokens
        }
        else {
            d->interp_t[parcount] = datareader_int_type(partok); //get type from token
            if (LastTokWasNumeric && (parcount +1 < d->noutputs)) parcount++;//last par was column number
            LastTokWasNumeric = FALSE;//keep track of numeric tokens
        }
        partok = strtok(NULL,",:"); //reread parameter string for next token
    }
CONSOLE_DEBUG("parcount: %d,noutoputs: %d",parcount,d->noutputs); 
    if (parcount+1 != d->noutputs) {
    	ERROR_REPORTER_HERE(ASC_USER_ERROR,
    	"Number of Columns in parameters and Model dont match, check model declaration");
    	return 1;
    }
    /*Check for model variables exceeding the allowable by data file.
    This condition will check that the user has declared a number of model
    variables less or equal to the variables allowed by the data file. This is
    different from the parameter check where the user is requesting a column out
    of range.
    
    One could argue that the user might like to declare several model variables
    linked to the same data column, but a decision has been made by the developer
    to restrict this, as it is thought as superfluous for most modelling scenarios.
    
    */
    if (d->noutputs > d->nmaxoutputs) {
    	ERROR_REPORTER_HERE(ASC_USER_ERROR,"Numbef of model variables exceeds number of data colums, check your model");
    	return 1;
    }
    return 0; //sucess.
}

/**
	Set data file format
	@return 0 on success
*/
int datareader_set_format(DataReader *d, const char *format) {

#define STR(N) #N
#define COMMA ,
    const char *fmts[] = { FMTS(STR, COMMA) };
#undef STR
#undef COMMA

    int i;

    CONSOLE_DEBUG("FORMAT '%s'", format);

    DataReaderFormat found = DATAREADER_INVALID_FORMAT;
    for (i = 0; i < DATAREADER_INVALID_FORMAT; ++i) {
        if (strcmp(format, fmts[i]) == 0) {
            found = (DataReaderFormat)i;
            break;
        }
    }

    CONSOLE_DEBUG("FOUND DATA FORMAT %d", found);

    switch (found) {
    case DATAREADER_FORMAT_TMY2:
        d->headerfn = &datareader_tmy2_header;
        d->datafn = &datareader_tmy2_data;
        d->eoffn = &datareader_tmy2_eof;
        d->indepfn = &datareader_tmy2_time;
        d->valfn = &datareader_tmy2_vals;
        break;
    case DATAREADER_FORMAT_ACDB:
        d->headerfn = &datareader_acdb_header;
        d->datafn = &datareader_acdb_data;
        d->eoffn = &datareader_acdb_eof;
        d->indepfn = &datareader_acdb_time;
        d->valfn = &datareader_acdb_vals;
        break;
    case DATAREADER_FORMAT_CSV:
        d->headerfn = &datareader_csv_header;
        d->datafn = &datareader_csv_data;
        d->eoffn = &datareader_csv_eof;
        d->indepfn = &datareader_csv_time;
        d->valfn = &datareader_csv_vals;
        break;
    case DATAREADER_FORMAT_TDV:
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Tab delimited values (TDV) format not yet implemenented.");
        return 1;
    default:
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Unknown file format '%s' specified", format);
        return 1;
    }

    return 0;
}

typedef struct DataFileSearchData_struct {
    struct FilePath *fp; /**< the relative path we're searching for */
    ospath_stat_t buf; /**< saves memory allocation in the 'test' fn */
    int error;           /**< error code (in case stat or fopen failed) */
    struct FilePath *fp_found; /**< the full path we found */
} DataFileSearchData;

FilePathTestFn datareader_searchpath_test;

/**
	@return 1 on success
*/
int datareader_searchpath_test(struct FilePath *path, void *searchdata) {
    struct FilePath *fp1;
    DataFileSearchData *sd;

    sd = (DataFileSearchData *)searchdata;
    assert(sd != NULL);
    assert(sd->fp != NULL);

    fp1 = ospath_concat(path, sd->fp);
    if (fp1 == NULL) {
        CONSOLE_DEBUG("Couldn't concatenate path");
        return 0;
    }

    if (ospath_stat(fp1, &sd->buf)) {
        sd->error = errno;
        ospath_free(fp1);
        return 0;
    }

    sd->fp_found = fp1;
    return 1;
};


/**
	Initialise the datareader: open the file, check the number of columns, etc.
	@return 0 on success

	@TODO search for the file in the ASCENDLIBRARY if not found immediately
*/
int datareader_init(DataReader *d) {
    ospath_stat_t s;
    char *tmp;
    struct FilePath **sp1, *fp2;
    DataFileSearchData sd;

    d->fp = ospath_new(d->fn);
    if (d->fp == NULL) {
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Invalid filepath");
        return 1;
    }

    if (ospath_stat(d->fp, &s)) {
        if (errno == ENOENT) {
            /* file doesn't exist: check the search path instead */
            tmp = Asc_GetEnv(ASC_ENV_LIBRARY);
            if (tmp == NULL) {
                ERROR_REPORTER_HERE(ASC_PROG_ERROR, "No paths to search (is env var '%s' set?)", ASC_ENV_LIBRARY);
                return 1;
            }

            sp1 = ospath_searchpath_new(tmp);
            if (sp1 == NULL) {
                ERROR_REPORTER_HERE(ASC_PROG_ERROR, "Unable to process %s value '%s'", ASC_ENV_LIBRARY, tmp);
                /* memory error */
                ascfree(tmp);
                return -3;
            }
            ascfree(tmp);

            sd.fp = d->fp;

            fp2 = ospath_searchpath_iterate(sp1, &datareader_searchpath_test, &sd);

            if (fp2 == NULL) {
                ERROR_REPORTER_HERE(ASC_USER_ERROR, "File '%s' not found in search path (error %d)", d->fn, sd.error);
                ospath_searchpath_free(sp1);
                return -1;
            }

            ospath_searchpath_free(sp1);

            /* replace our relative path with an absolute one */
            ospath_free(d->fp);
            d->fp = sd.fp_found;

        } else {
            ERROR_REPORTER_HERE(ASC_USER_ERROR, "The file '%s' cannot be accessed.\n"
                                "Check the file privileges, or try specifying an absolute path.", d->fn
                               );
            return 1;
        }
    }
#if DR_DEBUG
    CONSOLE_DEBUG("About to open the data file");
#endif
    d->f = ospath_fopen(d->fp, "r");
    if (d->f == NULL) {
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Unable to open file '%s' for reading.", d->fn);
        return 1;
    }
#if DR_DEBUG
    CONSOLE_DEBUG("Data file open ok");
#endif
    asc_assert(d->headerfn);
    asc_assert(d->eoffn);
    asc_assert(d->datafn);

    if ((*d->headerfn)(d)) {
        ERROR_REPORTER_HERE(ASC_PROG_ERR, "Error processing file header in '%s'", d->fn);
        fclose(d->f);
        return 1;
    }

    while (! (*d->eoffn)(d)) {
        if ((*d->datafn)(d)) {
            ERROR_REPORTER_HERE(ASC_PROG_ERR, "Error reading file data in '%s'", d->fn);
            fclose(d->f);
            return 1;
        }
    }
#if DR_DEBUG
    CONSOLE_DEBUG("Done retrieving data");
#endif
    fclose(d->f);
#if DR_DEBUG
    CONSOLE_DEBUG("Closed file");
#endif

    d->i = 0; /* set current position to zero */

    /*	these values are set to ensure that the polynomial coefficients
    	are at least computed once*/
    d->prev_i_val = -1;
    d->prev_i_der = -1;
    return 0;
}

/**
	Shut down the data reader and deallocate any memory owned by it, then
	free the memory at d.
*/
int datareader_delete(DataReader *d) {
    if (d->fp) {
        ospath_free(d->fp);
        d->fp = NULL;
    }
    if (d->f) {
        fclose(d->f);
        d->f = NULL;
    }
    ASC_FREE(d);
    return 0;
}

/**
	Return the number of inputs (independent variables) supplied in the
	DataReader's current file. Can only be 1 at this stage.
*/
int datareader_num_inputs(const DataReader *d) {
    return d->ninputs;
}

/**
	Return the number of outputs (dependent variables) found in the DataReader's
	current file. Should be one or more.
*/
int datareader_num_outputs(const DataReader *d) {
    return d->noutputs;
}


int datareader_locate(DataReader *d, double t, double *t1, double *t2) {
    (*d->indepfn)(d, t1);
    if (*t1 > t && d->i > 0) {
        /* start of current interval is too late */
        do {
            --(d->i);
            (*d->indepfn)(d, t1);
            /*CONSOLE_DEBUG("STEPPING BACK (d->i = %d currently), t1=%lf",d->i, *t1); */
        } while (*t1 > t && d->i > 0);
    }
    /* now either d->i==0 or t1 < t*/
    /* CONSOLE_DEBUG("d->i==%d, t1=%lf",d->i,*t1); */
    ++d->i;
    (*d->indepfn)(d, t2);
    if (*t2 <= t) {
        /* end of current interface is too early */
        do {
            /* CONSOLE_DEBUG("STEPPING FORWARD (d->i = %d currently), t1=%lf, t2=%lf",d->i,*t1,*t2); */
            (*d->indepfn)(d, t1);
            ++(d->i);
            (*d->indepfn)(d, t2);
        } while (*t2 < t && d->i < d->ndata);
    }
#if DR_DEBUG
    CONSOLE_DEBUG("d->i==%d, t1[0] = %lf, t2[0] = %lf", d->i, t1[0], t2[0]);
#endif

    if (d->i == d->ndata || d->i == 0) {
        return 1;
    }

    /* CONSOLE_DEBUG("INTERVAL OK"); */
    return 0;
}

/**
	Return an interpolated set of output values for the given input values.
	This is computed according to user defined parameters.

	The required memory for the inputs and outputs must be allocated by the
	caller, and indicated by the pointers 'inputs' and 'outputs'.

	@see datareader_deriv
	@TODO implement this
*/
int datareader_func(DataReader *d, double *inputs, double *outputs) {
    boolean AtStart, AtEnd; //keep track of dataset ends, they affect constrained spline calculations
    int i,j;
    double t1[1], t2[1];
    double v0[d->nmaxoutputs], v1[d->nmaxoutputs], v2[d->nmaxoutputs], v3[d->nmaxoutputs];
    /** this is a fixed size...
    the maximum size determined by the format handler, which is included in the datareader struct.
    @TODO the TmyDataPoint has the output variables declared as float, why?**/

    double t = inputs[0];

#if DR_DEBUG
    CONSOLE_DEBUG("EVALUATING AT t = %lf", inputs[0]);
#endif

    asc_assert(d->indepfn);

    if (datareader_locate(d, t, t1, t2)) {
        CONSOLE_DEBUG("LOCATION ERROR");
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Time value t=%f is out of range", t);
        return 1;
    }

#if DR_DEBUG
    CONSOLE_DEBUG("LOCATED AT t1 = %lf, t2 = %lf", t1[0], t2[0]);
#endif
    if (d->i < d->ndata-1) {
        ++d->i; //go one step forward
        (*d->valfn)(d, v3); //take a data sample at t1+2
        --d->i; //go back one step
        AtEnd = FALSE; //index is not at the end of the dataset
    }
    else AtEnd = TRUE;

    (*d->valfn)(d, v2);
    --d->i;
    (*d->valfn)(d, v1);

    if (d->i > 0) {
        --d->i; //go one step backward
        (*d->valfn)(d, v0); //take a data sample at t1-1
        ++d->i; //should be positioned at v1 t1
        AtStart = FALSE;
    }
    else AtStart = TRUE;

#if DR_DEBUG
    CONSOLE_DEBUG("LOCATED OK, d->i = %d, t1 = %lf, t2 = %lf, v1=%lf, v2=%lf", d->i, t1[0], t2[0], v1[0], v2[0]);
#endif

    for (i = 0;i < d->noutputs;++i) {
        j = d->cols[i]-1;
        switch (d->interp_t[i]) {
        case linear:
            outputs[i] = dr_linearinterp(t,t1,t2,v1[j],v2[j]);
            break;
        case default_interp:
        case sun:  //to be implemented as a refinement of the cubic spline
        case cubic:
            outputs[i] = dr_cubicinterp(d,i,t,t1,t2,v0[j],v1[j],v2[j],v3[j]);

            break;
        }
#if DR_DEBUG
        CONSOLE_DEBUG("[%d]: START = %lf, END = %lf, VALUE=%lf", i, v1[j],v2[j], outputs[i]);
#endif
    }

    return 0;
}

/**
	Return an interpolated set of output derivatives for the given input
	values. These can be smooth if the cubic interpolation method is selected.
*/
int datareader_deriv(DataReader *d, double *inputs, double *jacobian) {
    boolean AtStart, AtEnd; //keep track of dataset ends, they affect constrained spline calculations
    int i,j;
    double t1[1], t2[1];
    double v0[d->nmaxoutputs], v1[d->nmaxoutputs], v2[d->nmaxoutputs], v3[d->nmaxoutputs];
    /** this is a fixed size...
    the maximum size determined by the format handler, which is included in the datareader struct.
    @TODO the TmyDataPoint has the output variables declared as float, why?**/

    double t = inputs[0];

#if DR_DEBUG
    CONSOLE_DEBUG("EVALUATING AT t = %lf", inputs[0]);
#endif

    asc_assert(d->indepfn);

    if (datareader_locate(d, t, t1, t2)) {
        CONSOLE_DEBUG("LOCATION ERROR");
        ERROR_REPORTER_HERE(ASC_USER_ERROR, "Time value t=%f is out of range", t);
        return 1;
    }

#if DR_DEBUG
    CONSOLE_DEBUG("LOCATED AT t1 = %lf, t2 = %lf", t1[0], t2[0]);
#endif
    if (d->i < d->ndata-1) {
        ++d->i; //go one step forward
        (*d->valfn)(d, v3); //take a data sample at t1+2
        --d->i; //go back one step
        AtEnd = FALSE; //index is not at the end of the dataset
    }
    else AtEnd = TRUE;

    (*d->valfn)(d, v2);
    --d->i;
    (*d->valfn)(d, v1);

    if (d->i > 0) {
        --d->i; //go one step backward
        (*d->valfn)(d, v0); //take a data sample at t1-1
        ++d->i; //should be positioned at v1 t1
        AtStart = FALSE;
    }
    else AtStart = TRUE;

#if DR_DEBUG
    CONSOLE_DEBUG("LOCATED OK, d->i = %d, t1 = %lf, t2 = %lf, v1=%lf, v2=%lf", d->i, t1[0], t2[0], v1[0], v2[0]);
#endif

    for (i = 0;i < d->noutputs;++i) {
        j = d->cols[i]-1;
        switch (d->interp_t[i]) {
        case linear:
            jacobian[i] = dr_linearderiv(t,t1,t2,v1[j],v2[j]);
            break;
        case default_interp:
        case sun:  //to be implemented as a refinement of the cubic spline
        case cubic:
            jacobian[i] = dr_cubicderiv(d,i,t,t1,t2,v0[j],v1[j],v2[j],v3[j]);

            break;
        }
#if DR_DEBUG
        CONSOLE_DEBUG("[%d]: START = %lf, VALUE=%lf", i, v1[j], jacobian[i]);
#endif
    }

    return 0;
}

double dr_linearinterp(double t, double *t1, double *t2, double v1, double v2) {
    double g, dt;
    dt = (*t2)-(*t1);
    g = (v2 - v1) / dt;
    return v1 + g * (t - (*t1));
}
double dr_cubicinterp(DataReader *d, int j, double t, double *t1, double *t2, double v0, double v1, double v2, double v3) {
    //constrained cubic spline by CJC Kruger http://www.korf.co.uk
    boolean AtStart, AtEnd;
    double dt;
    double k0,k1,k2,k3; //gradients for cubic spline interpolation
    double a0,a1,a2,a3;//polynomial coefficients for second spline segment
    dt = t2[0] - t1[0];

    if (d->i != d->prev_i_val) {
        /*if we are at a new interval, still wait for all coefficients to be
          calculated before updating prev_i_val index */
        if (j == d->noutputs-1) d->prev_i_val = d->i;

        if (d->i < d->ndata-1) AtEnd = FALSE; //index is not at the end of the dataset
        else AtEnd = TRUE; //index is at the end of the dataset
        if (d->i > 0) AtStart = FALSE; //index is not at the beggining of the dataset
        else AtStart = TRUE; //index is at the begginintg of the dataset

        if ((dt/(v3-v2)+dt/(v2-v1))==0 || (v3-v2)*(v2-v1)<0) k2 = 0;
        else {
            if (AtEnd) k2 = 3*(v2-v1)/2-1/(dt/(v2-v1)+dt/(v1-v0));
            else k2 = 2/(dt/(v3-v2)+dt/(v2-v1));
        }
        if ((dt/(v2-v1)+dt/(v1-v0))==0 || (v2-v1)*(v1-v0)<0) k1 = 0;
        else {
            if (AtStart) k1 = 3*(v2-v1)/2 - k2/2;
            else k1 = 2/(dt/(v2-v1)+dt/(v1-v0));
        }
        k0 = -2*(k2+2*k1)/dt+6*(v2-v1)/pow(dt,2); //used as second derivative at t1
        k3 = 2*(2*k2+k1)/dt-6*(v2-v1)/pow(dt,2); //used as second derivative at t2

        //calculate polynomial coefficients
        a3 = (k3-k0)/(6*dt);
        a2 = (t2[0]*k0-t1[0]*k3)/(2*dt);
        a1 = ((v2-v1)-a2*(pow(t2[0],2)-pow(t1[0],2))-a3*(pow(t2[0],3)-pow(t1[0],3)))/dt;
        a0 = v1-a1*t1[0]-a2*pow(t1[0],2)-a3*pow(t1[0],3);

        //store coefficients in the datareader structure
        d->a0[j] = a0;
        d->a1[j] = a1;
        d->a2[j] = a2;
        d->a3[j] = a3;
#if DR_DEBUG
        if (j == 1 )CONSOLE_DEBUG("Cubic spline coefficients recalculated");
#endif
    }
    //calculate output value
#if DR_DEBUG
    CONSOLE_DEBUG("v[%d]:%lf",j,d->a0[j] + d->a1[j]*t + d->a2[j]*pow(t,2) + d->a3[j]*pow(t,3));
#endif
    return d->a0[j] + d->a1[j]*t + d->a2[j]*pow(t,2) + d->a3[j]*pow(t,3);

}

double dr_linearderiv(double t, double *t1, double *t2, double v1, double v2) {
    return (v2 - v1) / ((*t2)-(*t1));
}
double dr_cubicderiv(DataReader *d, int j, double t, double *t1, double *t2, double v0, double v1, double v2, double v3) {
    //constrained cubic spline by CJC Kruger http://www.korf.co.uk
    boolean AtStart, AtEnd;
    double dt;
    double k0,k1,k2,k3; //gradients for cubic spline interpolation
    double a0,a1,a2,a3;//polynomial coefficients for second spline segment
    dt = t2[0] - t1[0];

    if (d->i != d->prev_i_der) {
        /*if we are at a new interval, still wait for all coefficients to be
          calculated before updating prev_i_val index */
        if (j == d->noutputs-1) d->prev_i_der = d->i;

        if (d->i < d->ndata-1) AtEnd = FALSE; //index is not at the end of the dataset
        else AtEnd = TRUE; //index is at the end of the dataset
        if (d->i > 0) AtStart = FALSE; //index is not at the beggining of the dataset
        else AtStart = TRUE; //index is at the begginintg of the dataset

        if ((dt/(v3-v2)+dt/(v2-v1))==0 || (v3-v2)*(v2-v1)<0) k2 = 0;
        else {
            if (AtEnd) k2 = 3*(v2-v1)/2-1/(dt/(v2-v1)+dt/(v1-v0));
            else k2 = 2/(dt/(v3-v2)+dt/(v2-v1));
        }
        if ((dt/(v2-v1)+dt/(v1-v0))==0 || (v2-v1)*(v1-v0)<0) k1 = 0;
        else {
            if (AtStart) k1 = 3*(v2-v1)/2 - k2/2;
            else k1 = 2/(dt/(v2-v1)+dt/(v1-v0));
        }
        k0 = -2*(k2+2*k1)/dt+6*(v2-v1)/pow(dt,2); //used as second derivative at t1
        k3 = 2*(2*k2+k1)/dt-6*(v2-v1)/pow(dt,2); //used as second derivative at t2

        //calculate polynomial coefficients
        a3 = (k3-k0)/(6*dt);
        a2 = (t2[0]*k0-t1[0]*k3)/(2*dt);
        a1 = ((v2-v1)-a2*(pow(t2[0],2)-pow(t1[0],2))-a3*(pow(t2[0],3)-pow(t1[0],3)))/dt;
        a0 = v1-a1*t1[0]-a2*pow(t1[0],2)-a3*pow(t1[0],3);

        //store coefficients in the datareader structure
        d->a0[j] = a0;
        d->a1[j] = a1;
        d->a2[j] = a2;
        d->a3[j] = a3;
        #if DR_DEBUG
        if (j == 1 )CONSOLE_DEBUG("Cubic spline derivatives recalculated");
        #endif
    }
    //calculate output value
    return d->a1[j] + 2*d->a2[j]*t + 3*d->a3[j]*pow(t,2);

}
