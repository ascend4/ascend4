#include "dr.h"
#include <general/ospath.h>
#include <utilities/ascMalloc.h>
#include <utilities/error.h>

/*------------------------------------------------------------------------------
  DATA STRUCTURES (PRIVATE TO THIS FILE)
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
struct DataPoint{
	double t;
	double *outputs;
};

/**
	Need some kind of structure here to hold spline data for a particular
	interval.
*/

/**
	Top-level structure for the data reader. Keeps track of the file we're
	working with, the number of columns, etc.
*/
struct DataReader{
	const char *fn;
	struct FilePath *fp;
	FILE *f;
	int noutputs;
	
	InputFilterFn *iff;
	DataPoint *data; /**< null terminated */
};

/*------------------------------------------------------------------------------
  FORWARD DECLARATIONS
*/

DataReaderReadFn datareader_tmy2_read;
DataReaderHeaderFn datareader_tmy2_header;

/*------------------------------------------------------------------------------
  API FUNCTIONS
*/

/**
	Create a data reader object, with the filename specified. The filename
	will be searched for in a specified path, eg ASCENDLIBRARY.
*/		
int datareader_new(const char *fn){
	DataReader *d;
	
	d = ASC_NEW(DataReader);
	d->fn = fn;
	d->fp = NULL;
	d->f = NULL;
	d->noutputs = 0;
	d->iff = NULL;

	return d;
}

/**
	Set data file format
	@return 0 on success
*/
int datareader_set_file_format(DataReader *d, const datareader_file_format_t &format);
	switch(format){
		case DATAREADER_FORMAT_TMY2:
			d->headerfn=&datareader_tmy2_header;
			d->linefn=&datareader_tmy2_read;
			break;
		default:
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unknown file format specified");
			return 1;
	}
	return 0;
}

/**
	Assign an an on-file-open filter to the data reader. This will permit pre-processing
	of data from the file, and reading of different formats, eg CSV, TDV, TMY,
	fixed-width, etc.
*/
int datareader_set_on_open_action(DataReader *d, DataHeaderFunction *iff){
	d->iff = iff;
	return 0;
}

/**	
	Initialise the datareader: open the file, check the number of columns, etc.
	@return 0 on success
*/
int datareader_init(DataReader *d){
	FILE *f;

	d->fp = ospath_new(d->fn);
	if(d->fp==NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid filepath");
		return 1;
	}

	d->f = ospath_fopen(d->fp);
	if(d->f = NULL){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Unable to open file '%s'",d->fn);
		return 1;
	}

	if(datareader_process_header(d)){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Error processing file header in '%s'",d->fn);
		return 1;
	}
	
	if(datareader_read_data(d)){
		ERROR_REPORTER_HERE(ASC_PROG,ERR,"Error reading file data in '%s'",d->fn);
		return 1;
	}

	fclose(d->f);

	return 0;
}

/**
	Shut down the data reader and deallocate any memory owned by it, then
	free the memory at d.
*/
int datareader_delete(DataReader *d){
	if(d->fp){
		ospath_free(d->fp);
		d->fp = NULL;
	}
	if(d->f){
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
int datareader_num_inputs(const DataReader *d){
	return 1;
}

/**
	Return the number of outputs (dependent variables) found in the DataReader's
	current file. Should be one or more.
*/
int datareader_num_outputs(const DataReader *d){
	return d->noutputs;
}	d->f = NULL;


/**
	Return an interpolated set of output values for the given input values.
	This should be computed such that the output values are smooth in their 
	first derivatives.

	The required memory for the inputs and outputs must be allocated by the
	caller, and indicated by the pointers 'inputs' and 'outputs'.

	@see datareader_deriv
	@TODO implement this
*/
int datareader_func(DataReader *d, double *inputs, double *outputs){
	int i;
	double t;

	t = inputs[0];
	
}

/**
	Return an interpolated set of output derivatives for the given input
	values. These should be smooth.
	@see datareader_func
	@TODO implement this
*/
int datareader_deriv(DataReader *d, double *inputs, double *jacobian){
	CONSOLE_DEBUG("Not implemented");
	return 1;
}

/*------------------------------------------------------------------------------
  TMY2 READER FUNCTIONS
	
	These functions implement a reader interface for meteorological data in the
	TMY2 format as specified at http://rredc.nrel.gov/solar/pubs/tmy2/tab3-2.html
*/

/**
	@return 0 on success
*/
int datareader_tmy2_read(DataReader *d){
	char wban[-2 + 6 +2];
	char city[-8 +29 +2];
	char zone[-34+36 +2];
	char lathemi;
	int latdeg, latmin;
	char longhemi;
	int longdeg, longmin;

	fscanf(d->f,"%s %s %s %d %c %d %d %c %d %d %d"
		,wban,city,zone
		,lathemi,latdeg,latmin
		,longhemi,longdeg,longmin
	);

	double lat = latdeg + latmin/60;
	if(lathemi=='S')lat=-lat;
	double lng = longdeg + longmin/60;
	if(longhemi=='E')lng=-lng;
	CONSOLE_DEBUG( "TMY2 data for city '%s' (WBAN %s, time zone %s) at lat=%.3f, long=%.3f"
		city, wban, zone, lat, lng
	);
}

#define MEAS(N) int N; char N##_source; int N##_uncert

typedef struct Tmy2Point{
	double t;
	float G;
	float G
	
/**
	Read a line of data and store in d.
	@return 0 on success
*/
int datareader_tmy2_read(DataReader *d){
	int year,month,day,hour;
	int Iegh,Iedn ,Igh,Idn , Idh; // Irradiation
	char Igh_source, Idn_source, Idh_source;
	int Igh_uncert, Idn_uncert, Idh_uncert;

	MEAS(Lgh); /* Global horiz illuminance / (100 lux) */
	MEAS(Ldn); /* Direct normal illuminance / (100 lux) */
	MEAS(Ldh); /* Diffuse horiz illuminance / (100 lux) */
	MEAS(Lz); /* Zenith illuminance / (10 Cd/m2) */

	MEAS(covtot); /* Total sky cover / tenths */
	MEAS(covopq); /* Opaque sky cover / tenths */

	MEAS(T); /* temperature / (0.1degC) */
	MEAS(Tdew); /* dew point temperature / (0.1degC) */
	MEAS(p); /* pressure / mbar */
	MEAS(rh); /* rel humidity / % */
	MEAS(wdir); /* wind dir, N=0, E=90,... */
	MEAS(wvel); /* wind speed / (m/s) */
	MEAS(vis); /* visibility / (100m) */
	MEAS(ch); /* ceiling height / m (or special value) */
	
	MEAS(rain); /* preciptable water / mm */
	MEAS(aer); /* aerosol optical depth in thousandths (???) */
	MEAS(snow); /* snow depth on the specified day / cm (999=missing data) */
	MEAS(dsno); /* days since last snow (or special value) */

	/* weather observations from Appendix B */
	int obs, storm, precip, drizz, snowtype, snowshower, sleet, fog, smog, hail;

	/* brace yourself for this one... */

	fscanf(d->f, 
		/* 1 */ "%2d%2d%2d%2d" "%4d%4d" "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d"
		/* 2 */ "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d"
		/* 3 */ "%2d%1s%1d" "2d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" "%3d%1s%1d" "%4d%1s%1d" 
		/* 4 */ "%3d%1s%1d" "%3d%1s%1d" "%4d%1s%ld" "%5d%1s%1d" 
		/* 5 */ "%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d"
		/* 6 */ "%3d%1s%1d" "%3d%1s%1d" "%3d%1s%1d" "%2d%1s%1d"

	/* 1 */
		,year,month,day,hour
		,Iegh,Iedn
		,Igh,Igh_source,Igh_uncert /* I values in Wh/m2 */
		,Idn,Idn_source,Idn_uncert
		,Idh,Idh_source,Idh_uncert
	/* 2 */
		,Lgh,Lgh_source,Lgh_uncert /* L values in kCd/m2 */
		,Ldn,Ldn_source,Ldn_uncert
		,Ldh,Ldh_source,Ldh_uncert
		,Lz,Lz_source,Lz_uncert
	/* 3 */
		,covtot, covtot_source, covtot_uncert
		,covopq, covopq_source, covopq_uncert
		,T, T_source, T_uncert
		,Tdew, Tdew_source, Tdew_uncert
		,rh,rh_source, rh_uncert
		,p, p_source, p_uncert
	/* 4 */
		,wdir, wdir_source, wdir_uncert
		,wvel, wvel_source, wvel_uncert
		,vis, vis_source, vis_uncert
		,ch, ch_source, ch_uncert
	/* 5 */		
		,obs, storm, precip, drizz, snowtype, snowshower, sleet, fog, smog, hail
	/* 6 */
		,rain, rain_source, rain_uncert
		,aer, aer_source, aer_uncert
		,snow, snow_source, snow_uncert
		,dsno, dsno_source, dsno_uncert
	);

	/* 
		for the moment, we only record global horizontal, direct normal,
		ambient temperature, wind speed.
	*/

	struct Tmy2Point tmy;
	tmy.t = double(year
	tmy.G0 = float(Igh);
	tmy,
	
	
}
	
