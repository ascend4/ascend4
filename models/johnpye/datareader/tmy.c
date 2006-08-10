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
	Data Reader implementation for the TMY2 format.

	These functions implement a reader interface for meteorological data in the
	TMY2 format as specified at http://rredc.nrel.gov/solar/pubs/tmy2/tab3-2.html
*//*
	by John Pye, Aug 2006
*/

#include <stdio.h>
#include <libradtran/sun.h>

#include <utilities/ascMalloc.h>
#include <utilities/error.h>

#include "tmy.h"


typedef struct{
	double t;
	float G;
	float Gbn;
	float T;
	float v_wind;
	
} Tmy2Point;

#define DATA(D,I) ((Tmy2Point *)(D->data))[D->I]

/**
	@return 0 on success
*/
int datareader_tmy2_header(DataReader *d){
	char wban[-2 + 6 +2];
	char city[-8 +29 +2];
	char state[-31+32+2];
	int zone;
	char lathemi;
	int latdeg, latmin;
	char longhemi;
	int longdeg, longmin;
	int elev;

	fscanf(d->f,"%s %s %s %d" 
		" %c %d %d"
		" %c %d %d"
		" %d"
			,wban,city,state,&zone
			,&lathemi,&latdeg,&latmin
			,&longhemi,&longdeg,&longmin
			,&elev
	);

	double lat = latdeg + latmin/60;
	if(lathemi=='S')lat=-lat;
	double lng = longdeg + longmin/60;
	if(longhemi=='E')lng=-lng;
	CONSOLE_DEBUG( "TMY2 data for city '%s' (WBAN %s, time zone %+d) at lat=%.3f, long=%.3f, elev=%d m"
		, city, wban, zone, lat, lng, elev
	);

	d->i = 0;
	d->ndata=8760;
	d->data = ASC_NEW_ARRAY(Tmy2Point,d->ndata);
	return 0;
}

int datareader_tmy2_eof(DataReader *d){
	if(feof(d->f)){
		/* terminate the data array with a NULL */
		d->ndata=d->i;
		return 1;
	}
	return 0;
}

#define MEAS(N) int N; char N##_source; int N##_uncert
#define READ(N) &N, &N##_source, &N##_uncert
	
/**
	Read a line of data and store in d.
	@return 0 on success
*/
int datareader_tmy2_data(DataReader *d){
	Tmy2Point *tmy;
	int year,month,day,hour;
	int Iegh,Iedn; // Irradiation
	MEAS(Igh); /* Global horizontal irradiation in the specified interval / (Wh/m2) */
	MEAS(Idn); /* Direct normal irradiation in the specified interval / (Wh/m2) */
	MEAS(Idh); /* Diffuse horizontal irradiation in the specified interval / (Wh/m2) */

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
		/* 1 */ "%2d%2d%2d%2d" "%4d%4d" "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" /* =15 */
		/* 2 */ "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" /* +12=27 */
		/* 3 */ "%2d%1s%1d" "%2d%1s%1d" "%4d%1s%1d" "%4d%1s%1d" "%3d%1s%1d" "%4d%1s%1d" /* +18=45 */
		/* 4 */ "%3d%1s%1d" "%3d%1s%1d" "%4d%1s%1d" "%5drh = 0.01*(float)rh; /* relative humidity */%1s%1d" /* +12=57 */
		/* 5 */ "%1d%1d%1d%1d%1d%1d%1d%1d%1d%1d" /* +10=67 */
		/* 6 */ "%3d%1s%1d" "%3d%1s%1d" "%3d%1s%1d" "%2d%1s%1d" /* +12=79 */

	/* 1 */
		,&year, &month, &day, &hour
		,&Iegh, &Iedn
		,READ(Igh) /* I values in Wh/m2 */
		,READ(Idn)
		,READ(Idh)
	/* 2 */
		,READ(Lgh) /* L values in kCd/m2 */
		,READ(Ldn)
		,READ(Ldh)
		,READ(Lz)
	/* 3 */
		,READ(covtot)
		,READ(covopq)
		,READ(T)
		,READ(Tdew)
		,READ(rh)
		,READ(p)
	/* 4 */
		,READ(wdir)
		,READ(wvel)
		,READ(vis)
		,READ(ch)
	/* 5 */		
		,&obs, &storm, &precip, &drizz, &snowtype, &snowshower, &sleet, &fog, &smog, &hail
	/* 6 */
		,READ(rain)
		,READ(aer)
		,READ(snow)
		,READ(dsno)
	);

	/* 
		for the moment, we only record global horizontal, direct normal,
		ambient temperature, wind speed.
	*/

	tmy = &DATA(d,i);
	tmy->t = ((day_of_year_specific(day,month,year) - 1)*24 + hour)*3600;
	tmy->G = (float)Igh; /* average W/m2 for the hour in question */
	tmy->Gbn = (float)Idn; /* normal beam radiation */
	tmy->T = 0.1*(float)T + 273.15; /* temperature */
	tmy->v_wind = (float)wvel;
	d->i++;
	return 0;
}
