/*	ASCEND modelling environment
	Copyright (C) 2011 Carnegie Mellon University

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
	Data Reader implementation for the ESP-r/EnergyPlus weather data format.

	(WARNING: outdated, see newer file embedded in the latest EnergyPlus pkg)
	http://apps1.eere.energy.gov/buildings/energyplus/weatherdata_format.cfm

The data format for the main data rows is shown here (see 'Input Output' document
in EnergyPlus's Documentation folder (you need to install the software to get
these files, unfortunately, but it is free (gratis) to do so)

N1, \field Year
N2, \field Month
N3, \field Day
N4, \field Hour
N5, \field Minute
A1, \field Data Source and Uncertainty Flags
\note Initial day of weather file is checked by EnergyPlus for validity (as shown below)
\note Each field is checked for "missing" as shown below. Reasonable values, calculated
\note values or the last "good" value is substituted.
N6, \field Dry Bulb Temperature
\units C
\minimum> -70
\maximum< 70
\missing 99.9
N7, \field Dew Point Temperature
\units C
\minimum> -70
\maximum< 70
\missing 99.9
N8, \field Relative Humidity
\missing 999.
\minimum 0
\maximum 110
N9, \field Atmospheric Station Pressure
\units Pa
\missing 999999.
\minimum> 31000
\maximum< 120000
N10, \field Extraterrestrial Horizontal Radiation
\units Wh/m2
\missing 9999.
\minimum 0
N11, \field Extraterrestrial Direct Normal Radiation
\units Wh/m2
\missing 9999.
\minimum 0
N12, \field Horizontal Infrared Radiation Intensity
\units Wh/m2
\missing 9999.
\minimum 0
N13, \field Global Horizontal Radiation
\units Wh/m2
\missing 9999.
\minimum 0
N14, \field Direct Normal Radiation
\units Wh/m2
\missing 9999.
\minimum 0
N15, \field Diffuse Horizontal Radiation
\units Wh/m2
\missing 9999.
\minimum 0
N16, \field Global Horizontal Illuminance
\units lux
\missing 999999.
\note will be missing if >= 999900
\minimum 0
N17, \field Direct Normal Illuminance
\units lux
\missing 999999.
\note will be missing if >= 999900
\minimum 0
N18, \field Diffuse Horizontal Illuminance
\units lux
\missing 999999.
\note will be missing if >= 999900
\minimum 0
N19, \field Zenith Luminance
\units Cd/m2
\missing 9999.
\note will be missing if >= 9999
\minimum 0
N20, \field Wind Direction
\units degrees
\missing 999.
\minimum 0
\maximum 360
N21, \field Wind Speed
\units m/s
\missing 999.
\minimum 0
\maximum 40
N22, \field Total Sky Cover
\missing 99
\minimum 0
\maximum 10
N23, \field Opaque Sky Cover (used if Horizontal IR Intensity missing)
\missing 99
\minimum 0
\maximum 10
N24, \field Visibility
\units km
\missing 9999
N25, \field Ceiling Height
\units m
\missing 99999
N26, \field Present Weather Observation
N27, \field Present Weather Codes
N28, \field Precipitable Water
\units mm 
\missing 999
N29, \field Aerosol Optical Depth
\units thousandths
\missing .999
N30, \field Snow Depth
\units cm
\missing 999
N31, \field Days Since Last Snowfall
\missing 99
N32, \field Albedo
\missing 999
N33, \field Liquid Precipitation Depth
\units mm
\missing 999
N34; \field Liquid Precipitation Quantity
\units hr
\missing 99

*//*
	by John Pye, Oct 2011.
*/

#include "ee.h"
#include "sun.h"

#include <stdio.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>

#include "parse/parse.h"

#define EE_DEBUG 0

/**
	Data extracted from the E/E data file, doesn't have to included everything,
	only the stuff we actually think we will want to use.
*/
typedef struct EePoint_struct{
	double t;
	float T; ///< dry bulb temperature, K.
	float p; ///< atmospheric pressure, Pa.
	float rh; /// relative humidity, fraction 0<rh<1.
	float DNI; ///< direct normal irradiance Wh/m²
	float Gd; ///< diffuse horizontal irradiance Wh/m²
	float d_wind; ///< wind direction, rad, N = 0, E = pi, W = 3pi, S = 2pi. Range 0<=d<2pi
	float v_wind; ///< wind speed, m/s.
} EePoint;

typedef struct EeData_struct{
	EePoint *rows;
	parse *p; /* parse object, non-null during file read */
} EeData;

#define DATA(D) ((EeData *)(D->data))

struct EeLocation{
	char city[101];
	char state[101];
	char country[101];
	char source[101];
	int WMO;
	double latitude; // in degrees, + is north
	double longitude; // in degrees, + is east
	double timezone; // in hours, rel to GMT
	double elevation; // in metres, -1000.0 to 9999.9 m.	
};

int parseLOCATION(parse *p, struct EeLocation *loc){
	return
		(parseThisString(p,"LOCATION")
		&& parseThisString(p,",")
		&& parseStrExcept(p,",",loc->city,100)
		&& parseThisString(p,",")
		&& parseStrExcept(p,",",loc->state,100)
		&& parseThisString(p,",")
		&& parseStrExcept(p,",",loc->country,100)
		&& parseThisString(p,",")
		&& parseStrExcept(p,",",loc->source,100)
		&& parseThisString(p,",")
		&& parseSignedNumber(p,&(loc->WMO))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->latitude))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->longitude))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->timezone))
		&& parseThisString(p,",")
		&& parseDouble(p,&(loc->elevation))
		&& parseEOL(p)
		) || parseError(p,"Error in LOCATION line")
	;
}

int parseIgnoreLineWith(parse *p, const char *label){
	char rubbish[1024];
	return
		parseThisString(p,label)
		&& parseStrExcept(p,"\r\n",rubbish,1023)
		&& parseEOL(p)
		;//&& assign(CONSOLE_DEBUG("Ignore: %s%s", label,rubbish));
}

/**
	@return 0 on success
*/
int datareader_ee_header(DataReader *d){
	struct EeLocation loc;
	d->data = ASC_NEW(EeData);
	DATA(d)->p = parseCreateFile(d->f);
	parse *p = DATA(d)->p;

	if(!(
		parseLOCATION(p,&loc) 
		&& parseIgnoreLineWith(p,"DESIGN CONDITIONS")
		&& parseIgnoreLineWith(p,"TYPICAL/EXTREME PERIODS")
		&& parseIgnoreLineWith(p,"GROUND TEMPERATURES")
		&& parseIgnoreLineWith(p,"HOLIDAYS/DAYLIGHT SAVINGS")
		&& many(parseIgnoreLineWith(p,"COMMENTS"))
		&& parseIgnoreLineWith(p,"DATA PERIODS")
	)){
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Parser error in data file");
	}

	CONSOLE_DEBUG("Loaded data for '%s' at (%.2fN,%.2fE)",loc.city,loc.latitude,loc.longitude);

    // set the value of some of the Data Reader parameters
    d->i = 0;
    d->ninputs = 1;
    d->ndata = 8760; // FIXME
    d->nmaxoutputs = 5; // FIXME

    DATA(d)->rows = ASC_NEW_ARRAY(EePoint,d->ndata);

	/* set the number of inputs and outputs */
	d->ninputs = 1;
	d->noutputs = 6;

	return 0;
}

int datareader_ee_eof(DataReader *d){
	//CONSOLE_DEBUG("Checking for EOF");
	parse *p = DATA(d)->p;
	if(parseEnd(p)){
		CONSOLE_DEBUG("REACHED END OF FILE");
		if(d->i < d->ndata){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Incomplete data set found (%d rows < %d expected",d->i, d->ndata);
		}
		d->ndata=d->i;
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Read %d rows",d->ndata);
		return 1;
	}

	return 0;
}
	
/**
	Read a line of data and store in d.
	@return 0 on success
*/
int datareader_ee_data(DataReader *d){
	//CONSOLE_DEBUG("Reading data, i = %d");
	unsigned year,month,day,hour,minute;
	char uncerts[101];
	EePoint *row = &(DATA(d)->rows[d->i]);

#define NUMFIELDS(D,X) D(T) X D(Tdew) X D(rh) X D(pres) X D(EHI) X D(EDNI) X D(HII) X D(GHI) X D(DNI) X D(DiffHI) X D(GHIll) X D(DNIll) X D(DiffHIll) X D(ZL) X D(winddir) X D(windspeed) X D(skycover) X D(opaqueskycover) X D(visibility) X D(ceilingheight) X D(weatherobs) X D(weathercodes) X D(precip) X D(aerosol) X D(snowdepth) X D(dayssincesnow) X D(albedo) X D(liquidprecipdepth) X D(liquidprecipqty)

#define DECL(D) D
#define COMMA ,
	double NUMFIELDS(DECL,COMMA);
#undef COMMA
#undef DECL

	// TODO what to do with 'missing' values??
	parse *p = DATA(d)->p;

#define PARSENUM(D) parseDouble(p,&D)
#define ANDTHEN && parseThisString(p,",") &&

	if(!(
		((parseNumber(p,&year)
		&& parseThisString(p,",")
		&& parseNumber(p,&month)
		&& parseThisString(p,",")
		&& parseNumber(p,&day)
		&& parseThisString(p,",")
		&& parseNumber(p,&hour)
		&& parseThisString(p,",")
		&& parseNumber(p,&minute)
		&& parseThisString(p,",")
		&& parseStrExcept(p,",",uncerts,100)
		&& parseThisString(p,",")
		&& NUMFIELDS(PARSENUM,ANDTHEN)
		) || parseError(p,"Missing/incorrect data field")
		) && (
			parseEOL(p) || parseError(p,"Expected end-of-line")
		)
	)){
		ERROR_REPORTER_HERE(ASC_PROG_ERROR,"Failed to parse E/E data file");
		return 1;
	};
#undef ANDTHEN
#undef PARSENUM

	row->t = ((day_of_year_specific(day,month,year) - 1)*24.0 + hour)*3600.0 + minute*60.;
	row->T = T + 273.15;
	row->p = pres;
	row->rh = rh / 100.;
	row->DNI = DNI; // FIXME need to allow for difference in dt, which might be other than 1h.
	row->Gd = DiffHI; // FIXME need to allow for difference in dt, which might be other than 1h.
	row->d_wind = winddir * 2*3.141592653589 / 360; // perhaps for variable continuity we should return sin and cos of this?
	row->v_wind = windspeed;

	d->i++;
	return 0;
}

int datareader_ee_time(DataReader *d, double *t){
	//*t = DATA(d).t;
	return 0;
}

#define ROW DATA(d)->rows[d->i]

int datareader_ee_vals(DataReader *d, double *v){
#if EE_DEBUG
	CONSOLE_DEBUG("At t=%f h, T = %lf, DNI = %f Wh/m2"
		,(ROW.t / 3600.),ROW.T, ROW.DNI
	);
#endif

	v[0]=ROW.T;
	v[1]=ROW.p;
	v[2]=ROW.rh;
	v[3]=ROW.DNI;
	v[4]=ROW.Gd;
	v[5]=ROW.d_wind;
	v[6]=ROW.v_wind;
	return 0;
}
