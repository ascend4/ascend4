/*	ASCEND modelling environment
	Copyright (C) 2012 Carnegie Mellon University

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
	Data Reader implementation for the TMY3 weather data format.

	File format description:
	http://www.nrel.gov/docs/fy08osti/43156.pdf

	Data available for download:
	http://rredc.nrel.gov/solar/old_data/nsrdb/1991-2005/tmy3/

	Header fields (first line):
	1. site USAF code
	2. station name (quoted)
	3. station US state (two-letter abbrev)
	4. site timezone (hours from greenwich, west negative)
	5. site latitude (deg, with decimal fraction)
	6. site longitude (deg, with decimal fraction)
	7. site elevation (m)
	
	Second line of the file contains the field names with units of measurement.

	Key fields (not all listed here):
	 1. date (MM/DD/YYYY -- NB american style!)
	 2. time (HH:MM in local STANDARD time -- no daylight savings applied)
	 5. GHI (Wh/m^2) integrated for the hour leading UP TO the timestamp
     7. % uncertainty in GHI
	 8. DNI (Wh/m^2) integrated for the hour leading UP TO the timestamp
	10. % uncertainty in DNI
	11. diffuse horizontal irradiance (Wh/m2) in hour leading to timestamp
	26. fraction of sky totally covered with cloud (tenths) at indicated time
	32. dry bulb temperature (0.1 °C) at the indicated time
	38. relative humidity (%) at the indicated time
	41. air pressure (mbar) at the indicated time
	44. wind direction (10°) assumed to be measured eastward? from north=0 at indic time.
	47. wind speed (0.1m/s)
	65. amount of rain (mm), see next field
	66. duration over which rain was measured (1hr), -9900 if missing.
*//*
	by John Pye, April 2012.
*/

#include "tmy3.h"
#include "sun.h"

#include <stdio.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>

#include "parse/parse.h"

#define TMY3_DEBUG 1

/**
	Data extracted from the E/E data file, doesn't have to included everything,
	only the stuff we actually think we will want to use.
*/
typedef struct Tmy3Point_struct{
	double t;
	float T; ///< dry bulb temperature, K.
	float p; ///< atmospheric pressure, Pa.
	float rh; /// relative humidity, fraction 0<rh<1.
	float DNI; ///< direct normal irradiance Wh/m²
	float GHI; ///< diffuse horizontal irradiance Wh/m²
	float d_wind; ///< wind direction, rad, N = 0, E = pi/2, W = 3pi/2, S = pi. Range 0<=d<2pi
	float v_wind; ///< wind speed, m/s.
} Tmy3Point;

typedef struct Tmy3Data_struct{
	Tmy3Point *rows;
	parse *p; /* parse object, non-null during file read */
} Tmy3Data;

#define DATA(D) ((Tmy3Data *)(D->data))

typedef struct Tmy3Location_struct{
	char stationcode[101];
	char stationname[101];
	char state[20];
	double timezone; // in hours, rel to GMT
	double latitude; // in degrees, + is north
	double longitude; // in degrees, + is east
	double elevation; // in metres
} Tmy3Location;

//example header line:
//723815,"DAGGETT BARSTOW-DAGGETT AP",CA,-8.0,34.850,-116.800,586
int parseLocation(parse *p, Tmy3Location *loc){
	return
		(parseStrExcept(p,",",loc->stationcode,100)
		&& parseThisString(p,",\"")
		&& parseStrExcept(p,"\"",loc->stationname,100)
		&& parseThisString(p,"\",")
		&& parseStrExcept(p,",",loc->state,100)
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->timezone))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->latitude))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->longitude))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->elevation))
		&& parseEOL(p)
		) || parseError(p,"Error in header line (line 1)")
	;
}

/**
	@return 0 on success
*/
int datareader_tmy3_header(DataReader *d){
	Tmy3Location loc;
	d->data = ASC_NEW(Tmy3Data);
	DATA(d)->p = parseCreateFile(d->f);
	parse *p = DATA(d)->p;
	char rubbish[2049];

	if(!(
		parseLocation(p,&loc) 
		&& parseStrExcept(p,"\r\n",rubbish,2048)
		&& parseEOL(p)
	)){
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Parser error in header part of file");
	}

	CONSOLE_DEBUG("TMY3 file for '%s' at (%.2fN,%.2fE)",loc.stationname,loc.latitude,loc.longitude);

    // set the value of some of the Data Reader parameters
    d->i = 0;
    d->ninputs = 1;
    d->ndata = 8760; // FIXME
    d->nmaxoutputs = 7; // FIXME

    DATA(d)->rows = ASC_NEW_ARRAY(Tmy3Point,d->ndata);

	/* set the number of inputs and outputs */
	d->ninputs = 1;
	d->noutputs = 7;
	return 0;
}

int datareader_tmy3_eof(DataReader *d){
	parse *p = DATA(d)->p;
	if(parseEnd(p)){
		CONSOLE_DEBUG("REACHED END OF FILE");
		if(d->i < d->ndata){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Incomplete data set found (%d rows < %d expected",d->i, d->ndata);
		}
		d->ndata=d->i;
		int i;double tmin = +0.5*DBL_MAX,tmax = -0.5*DBL_MAX;
		for(i=0; i<d->ndata; ++i){
			double t = DATA(d)->rows[i].t;
			if(t < tmin)tmin = t;
			if(t > tmax)tmax = t;
		}
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Read %d rows, t in range [%f,%f] d",d->ndata,tmin/3600./24.,tmax/3600./24.);
		return 1; /* yes, end of file */
	}
	return 0; /* no, more data still */
}
	
/**
	Read a line of data and store in d.
	@return 0 on success
*/
int datareader_tmy3_data(DataReader *d){
	//CONSOLE_DEBUG("Reading data, i = %d",d->i);
	unsigned year,month,day,hour,minute;
	Tmy3Point row;

	// in the following 'C' are char fiels, 'I' are integer fields;
	// the suffix 'e' means error/uncertainty and 's' means source of data.
#define NUMFIELDS(C,I,X) I(3_EGHI) X I(4_EDNI) \
	X I(5_GHI) X C(6_GHIs) X I(7_GHIe) \
	X I(8_DNI) X C(9_DNIs) X I(10_DNIe) \
	X I(11_DiffHI) X C(12_DiffHIs) X I(13_DiffHIe) \
	X I(14_GHIll) X C(15_GHIlls) X I(16_GHIlle) \
	X I(17_DNIll) X C(18_DNIlls) X I(19_DNIlle) \
	X I(20_DiffHIll) X C(21_DiffHIlls) X I(22_DiffHIlle) \
	X I(23_ZenLum) X C(24_ZenLums) X I(25_ZenLume) \
	X I(26_TotCov) X C(27_TotCovs) X C(28_TotCove) \
	X I(29_OpaqCov) X C(30_OpaqCovs) X C(31_OpenCove) \
	X I(32_T) X C(33_Ts) X C(34_Te) \
	X I(35_Tdew) X C(36_Tdews) X C(37_Tdewe) \
	X I(38_RH) X C(39_RHs) X C(40_RHe) \
	X I(41_P) X C(42_Ps) X C(43_Pe) \
	X I(44_WD) X C(45_WDs) X C(46_WDe) \
	X I(47_WS) X C(48_WSs) X C(49_WSe) \
	X I(50_HViz) X C(51_HVizs) X C(52_HVize) \
	X I(53_HViz) X C(54_HVizs) X C(55_HVize) \
	X I(56_HViz) X C(57_HVizs) X C(58_HVize) \
	X I(59_HViz) X C(60_HVizs) X C(61_HVize) \
	X I(62_HViz) X C(63_HVizs) X C(64_HVize) \
	X I(65_RainDepth) X I(66_RainDuration) X C(67_Rains) X C(68_Raine)

#define CHARDECL(NAME) char tmy3_field_##NAME;
#define NUMDECL(NAME) double tmy3_field_##NAME;
#define NUTHIN_HERE
	NUMFIELDS(CHARDECL,NUMDECL,NUTHIN_HERE);
#undef CHARDECL
#undef NUMDECL
#undef NUTHIN_HERE

	// TODO what to do with 'missing' values??
	parse *p = DATA(d)->p;

#define PARSEINT(NAME) parseDouble(p,&tmy3_field_##NAME)
#define PARSECHAR(NAME) parseAChar(p,&tmy3_field_##NAME)
#define ANDTHEN && parseThisString(p,",") &&

// example row:
// 01/25/1988,12:00,821,1411,530,1,13,580,1,9,192,1,13,565,1,13,593,1,9,219,1,13,442,1,21,10,A,7,4,A,7,13.3,A,7,-8.9,A,7,21,A,7,960,A,7,60,A,7,3.6,A,7,80500,A,7,77777,A,7,0.5,E,8,0.030,F,8,-9900.000,?,0,-9900,-9900,?,0

	if(!(
		(( /* parse the date and time first... */
		parseNumber(p,&month)
		&& parseThisString(p,"/")
		&& parseNumber(p,&day)
		&& parseThisString(p,"/")
		&& parseNumber(p,&year)
		&& parseThisString(p,",")
		&& parseNumber(p,&hour)
		&& parseThisString(p,":")
		&& parseNumber(p,&minute)
		&& parseThisString(p,",")
		/* then come the data fields */
		&& NUMFIELDS(PARSECHAR,PARSEINT,ANDTHEN)
		) || parseError(p,"Missing/incorrect data field")
		) && (
			parseEOL(p) || parseError(p,"Expected end-of-line")
		)
	)){
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Failed to parse E/E data file");
		return 1;
	};
#undef ANDTHEN
#undef PARSEINT
#undef PARSECHAR

	// TODO add check for data for Feb 29... just in case?
	row.t = ((day_of_year_specific(day,month,year) - 1)*24.0 + (hour - 1))*3600.0 + minute*60.;
	row.T = tmy3_field_32_T * 0.1;
	row.p = tmy3_field_41_P * 100.;
	row.rh = tmy3_field_38_RH * 0.01;
	row.DNI = tmy3_field_8_DNI * 1.;
	row.GHI = tmy3_field_5_GHI * 1.;
	row.v_wind = tmy3_field_47_WS * 0.1;
	row.d_wind = tmy3_field_44_WD * 3.14159265358 / 180. ;

	DATA(d)->rows[d->i] = row;

	//CONSOLE_DEBUG("Read i = %d, t = %f d, T = %.1f°C, rh = %.1f %",d->i,row.t / 3600. / 24., T, row.rh*100);
	
	d->i++;
	return 0;
}

int datareader_tmy3_time(DataReader *d, double *t){
	*t = DATA(d)->rows[d->i].t;
	return 0;
}

#define ROW DATA(d)->rows[d->i]

int datareader_tmy3_vals(DataReader *d, double *v){
#if TMY3_DEBUG
	CONSOLE_DEBUG("At t=%f d, T = %lf, DNI = %f Wh/m2"
		,(ROW.t / 3600. / 24.),ROW.T, ROW.DNI
	);
#endif
	v[0]=ROW.T;
	v[1]=ROW.p;
	v[2]=ROW.rh;
	v[3]=ROW.DNI;
	v[4]=ROW.GHI;
	v[5]=ROW.d_wind;
	v[6]=ROW.v_wind;
	return 0;
}


