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
	Data Reader implementation for the Australian Climate Data Bank .DAT format.

	These functions implement a reader interface for the Australian Climate
	Data Bank meteorological data as given in .DAT files from the Australian
	bureau of Meteorology.

	http://www.bom.gov.au/climate/averages/tables/supply.shtml

	The following comes from the README.txt file supplied by the BoM, to explain
	this data format:

  Characters             Item

   1 - 2  location identification (e.g.ME represents Melbourne)
   3 - 4  year (e.g. 67)
   5 - 6  month (i.e. 1 - 12)
   7 - 8  day (i.e. 1 - 31)
   9 - 10 hour standard (i.e. 0-23, 0 = midnight)
  11 - 14 dry bulb temperature (10-1 °C)
  15 - 17 absolute moisture content (10-1 g/kg)
  18 - 21 atmospheric pressure (10-1 kPa)
  22 - 24 wind speed (10-1 m/s)
  25 - 26 wind direction (0-16; 0 = CALM.  1 = NNE ,...,16 = N
  27      total cloud cover  (oktas, 0 - 8)
  28      flag relating to dry bulb temperature
  29      flag relating to absolute moisture content
  30      flag relating to atmospheric pressure
  31      flag relating to wind speed and direction
  32      flag relating to total cloud cover
  33      blank
  34 - 37 global solar irradiance on a horizontal plane (W/m2)
  38 - 40 diffuse solar irradiance on a horizontal plane (W/m2)
  41 - 44 direct solar irradiance on a plane normal to the beam ((W/m2)
  45 - 46 solar altitude (degrees, 0-90)
  47 - 49 solar azimuth (degrees, 0-360)
  50      flag relating to global and diffuse solar irradiance
  51      flag                                 }
  52 - 56 Australian Met Station Number        } Some locations only
  57 - 61 wet bulb temperature (10-1 °C)       }
  62 - 81 Station name (first line only)       }


  Values for flags relating to standard surface meteorological data
            columns 28 - 32)

  0  means that the value is measured value
  1  means that the value is estimated to replace a missing measurement
  2  means that the value is an interpolating between three-hourly measurements
  3  missing value

    Values for flag relating to solar radiation data (column 50)

  0  means that both global and diffuse irradiance values are based on measurements
  1  means that both global and diffuse irradiance values are estimated to
     replace a missing measurement
  2  means that the global irradiance value is based on measurement but the
     diffuse irradiance value is estimated to replace a missing measurement
  3  missing value or estimated value from cloud cover data
  4  interpolated value from three hourly data
*//*
	by John Pye, Aug 2006
*/

#include <stdio.h>
/* #include <libradtran/sun.h> */
#include "sun.h"

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>

#include "tmy.h"

#define ACDB_DEBUG 0

typedef struct AcdbPoint_struct{
	double t;
	float I;
	float Ibn;
	float Id;
	float T;
	float v_wind;
	
} AcdbPoint;

#define DATA(D) ((AcdbPoint *)(D->data))[D->i]

struct AcdbCity{
	char code[3];
	char name[40];
	char state[40];
	char dir[9];
};

static const struct AcdbCity acdb_city_info[] = {
	{"CN","Cairns AMO","QLD","CAIRNS"}
	,{"CV","Charleville AMO","QLD","CHARLEVI"}
	,{"CL","Cloncurry AMO","QLD","CLONCURR"}
	,{"GL","Gladstone MO","QLD","GLADSTON"}
	,{"LO","Longreach","QLD","LONGREAC"}
	,{"MK","Mackay MO","QLD","MACKAY"}
	,{"IS","Mt Isa AMO","QLD","MTISA"}
	,{"OA","Oakey Army Aviation","QLD","OAKEY"}
	,{"RO","Rockhampton AMO","QLD","ROCKHPTN"}
	,{"TO","Townsville AMO","QLD","TOWNSVL"}
	,{"WS","Wllis Island MO","QLD","WILLISIS"}
	,{"CO","Cobar AMO","NSW","COBAR"}
	,{"CH","Coffs Harbour MO","NSW","COFFSHAR"}
	,{"MA","Mascot (Sydney AMO)","NSW","MASCOT"}
	,{"MO","Moree MO","NSW","MOREE"}
	,{"NO","Nowra RAN AIR STN","NSW","NOWRA"}
	,{"OR","Orange AP","NSW","ORANGE"}
	,{"RI","Richmond","NSW","RICHMOND"}
	,{"SY","Sydney RO","NSW","SYDNEYRO"}
	,{"TM","Tamworth AMO","NSW","TAMWORTH"}
	,{"WA","Wagga Wagga AMO","NSW","WAGGA"}
	,{"WE","Williamtown AMO","NSW","WILLIAM"}
	,{"SE","East Sale AMO","VIC","EASTSALE"}
	,{"ES","Essendon AMO","VIC","ESSENDON"}
	,{"LA","Laverton AMO","VIC","LAVERTON"}
	,{"ME","Melbourne RO","VIC","MELBOURN"}
	,{"MI","Mildura AMO","VIC","MILDURA"}
	,{"NH","Nhill COMPOSITE","VIC","NHILL"}
	,{"TU","Tullamarine","VIC","TULLAMAR"}
	,{"HB","Hobart AMO","TAS","HOBRTAMO"}
	,{"HO","Hobart RO","TAS","HOBARTRO"}
	,{"LU","Launceston AP","TAS","LAUNCESN"}
	,{"AD","Adelaide AP","SA","ADELAP"}
	,{"AR","Adelaide RO","SA","ADELRO"}
	,{"AW","Adelaide West Terrace","SA","ADELWEST"}
	,{"CE","Ceduna","SA","CEDUNA"}
	,{"MG","Mt Gambier AMO","SA","MTGAMB"}
	,{"OO","Oodnadatta AMO","SA","OODNADAT"}
	,{"WO","Woomera AMO","SA","WOOMERA"}
	,{"AL","Alice Springs","NT","ALICESPR"}
	,{"DA","Darwin AP","NT","DARWINAP"}
	,{"DR","Darwin RO","NT","DARWINRO"}
	,{"TE","Tennant Crk MO","NT","TENNANT"}
	,{"AY","Albany (Eclipse Is)","WA","ALBANY"}
	,{"AB","Albany AMO","WA","ALBANAMO"}
	,{"BM","Broome AMO","WA","BROOME"}
	,{"CR","Carnarvon AMO","WA","CARNARVO"}
	,{"DE","Derby","WA","DERBY"}
	,{"EP","Esperance","WA","ESPERANC"}
	,{"FO","Forrest AMO","WA","FORREST"}
	,{"GE","Geraldton AMO","WA","GERALDTO"}
	,{"GI","Giles","WA","GILES"}
	,{"HA","Halls Creek","WA","HALLSCRK"}
	,{"KA","Kalgoorlie","WA","KALGOORL"}
	,{"LM","Learmonth MO","WA","LEARMTH"}
	,{"MT","Meekatharra","WA","MEEKATH"}
	,{"OW","Onslow","WA","ONSLOW"}
	,{"ON","Onslow PO","WA","ONSLOWPO"}
	,{"PR","Perth AMO","WA","PERTHAMO"}
	,{"PE","Perth RO","WA","PERTHRO"}
	,{"HE","Pt Hedland","WA","PORTHEDL"}
	,{"CB","Canberra AMO","ACT","CANBAMO"}
	,{"CA","Canberra CITY","ACT","CANBERRA"}
	,{"LE","Lae","New Guinea","LAE"}
	,{"PM","Pt Moresby Jackson","New Guinea","PTMORESB"}
	,{"RB","Rabual","New Guinea","RABAUL"}
	,{"LH","Lord Howe Island","Aust. Islands","LORDHOWE"}
	,{"NI","Norfolk Island AMO","Aust. Islands","NORFOLK"}
	,{"CI","Cocos Island COMP","Aust. Islands","COCOSIS"}
	,{"HH","Honiara Henderson AP","Solomon Islands","HONIARAP"}
	,{"HN","Honiara MO","Solomon Islands","HONIARMO"}
	,{"AN","Aneityum AERO","Vanuatu","ANEITYUM"}
	,{"VL","Vanua Lava","Vanuatu","VANUA"}
	,{"VI","Vila","Vanuatu","VILA"}
	,{"BT","Butterworth RAAF","Malaysia","BUTTRAAF"}
	,{"","","",""}
};

/**
	@return 0 on success
*/
int datareader_acdb_header(DataReader *d){

	char code[3];
	unsigned yr;
	unsigned data_rows;

	fscanf(d->f,"%2c%2ud",code,&yr);

	code[2] = '\0';

	const struct AcdbCity *i;
	unsigned found=0;
	for(i=acdb_city_info; i->code[0] != '\0'; ++i){
#if 0
		CONSOLE_DEBUG("Comparing code with '%s'",i->code);
#endif
		if(strcmp(i->code,code)==0){
			found=1;
			break;
		}
	}
	if(!found){
		CONSOLE_DEBUG("Unknown city '%s' in ACDB data file",code);
		ERROR_REPORTER_HERE(ASC_PROG_WARNING,"ACDB data file contains unrecognised city code '%s'",code);
	}else{
		CONSOLE_DEBUG("ACDB data file contains data for %s, %s.",i->name, i->state);
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"ACDB data file contains data for %s, %s.",i->name, i->state);
	}

	if(yr < 50)yr+=2000;
	else yr+=1900;

	if(is_leap_year(yr)){
		data_rows = 366 * 24;
	}else{
		data_rows = 365 * 24;
	}
	CONSOLE_DEBUG("ACDB data file is for year %u, expect %u data rows.",yr,data_rows);
	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"ACDB data file is for year %u, expect %u data rows.",yr,data_rows);

	d->ndata = data_rows;
	d->i = 0;
	d->ndata=8760;
	d->data = ASC_NEW_ARRAY(AcdbPoint,d->ndata);

	/* every line contains the city name, so rewind the file now */
	rewind(d->f);

	return 0;
}

int datareader_acdb_eof(DataReader *d){
	if(feof(d->f)){
		CONSOLE_DEBUG("REACHED END OF FILE");
		if(d->i < d->ndata){
			ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Incomplete data set found (%d rows < %d expected",d->i, d->ndata);
		}
		d->ndata=d->i;
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Read %d rows",d->ndata);
		return 1;
	}

	/* set the number of inputs and outputs */
	d->ninputs = 1;
	d->noutputs = 5;
	return 0;
}

#define MEAS(N) int N; char N##_source; int N##_uncert
#define READ(N) &N, &N##_source, &N##_uncert
	
/**
	Read a line of data and store in d.
	@return 0 on success
*/
int datareader_acdb_data(DataReader *d){
	/* static int lastmonth=-1;
	static int lastday=-1; */

	AcdbPoint *dat;

/* still need to implement...

  51      flag                                 }
  52 - 56 Australian Met Station Number        } Some locations only
  57 - 61 wet bulb temperature (10-1 °C)       }
  62 - 81 Station name (first line only)       }



   1 - 2  location identification (e.g.ME represents Melbourne)
   3 - 4  year (e.g. 67)
   5 - 6  month (i.e. 1 - 12)
   7 - 8  day (i.e. 1 - 31)
   9 - 10 hour standard (i.e. 0-23, 0 = midnight)

  11 - 14 dry bulb temperature (10-1 °C)
  15 - 17 absolute moisture content (10-1 g/kg)
  18 - 21 atmospheric pressure (10-1 kPa)
  22 - 24 wind speed (10-1 m/s)
  25 - 26 wind direction (0-16; 0 = CALM.  1 = NNE ,...,16 = N
  27      total cloud cover  (oktas, 0 - 8)
  28      flag relating to dry bulb temperature
  29      flag relating to absolute moisture content
  30      flag relating to atmospheric pressure
  31      flag relating to wind speed and direction
  32      flag relating to total cloud cover
  33      blank
  34 - 37 global solar irradiance on a horizontal plane (W/m2)
  38 - 40 diffuse solar irradiance on a horizontal plane (W/m2)
  41 - 44 direct solar irradiance on a plane normal to the beam ((W/m2)
  45 - 46 solar altitude (degrees, 0-90)
  47 - 49 solar azimuth (degrees, 0-360)
  50      flag relating to global and diffuse solar irradiance
  51      flag                                 }
  52 - 56 Australian Met Station Number        } Some locations only
  57 - 61 wet bulb temperature (10-1 °C)       }
  62 - 81 Station name (first line only)       }

*/

#define N_FIELDS 22
	const unsigned fieldsize[N_FIELDS] = {
		2,2,2,2, 4,3,4,3,2,1, 1,1,1,1,1, 1, 4,3,4,3,3,1
	};
	
	int data[N_FIELDS];

	enum {
		ACDB_YEAR
		,ACDB_MONTH
		,ACDB_DAY
		,ACDB_HOUR
		,ACDB_DRY_BULB_TEMP/* [0.1°C] */
		,ACDB_W /* ABSOLUTE MOISTURE CONTENT [0.1 G/KG] */
		,ACDB_P_ATM /* ATMOSPHERIC PRESSURE [0.1 KPA] */
		,ACDB_V_WIND /* WIND SPEED [0.1 M/S] */
		,ACDB_DIR_WIND /* DIR OF WIND: 0-16; 0 = CALM.  1 = NNE ,...,16 = N */
		,ACDB_CLOUD /* TOTAL CLOUD COVER (OKTAS, 0 - 8) */
		,ACDB_DRY_BULB_TEMP_FLAG
		,ACDB_W_FLAG
		,ACDB_P_ATM_FLAG
		,ACDB_WIND_FLAG
		,ACDB_CLOUD_FLAG
		,ACDB_WHITESPACE1
		,ACDB_GHI
		,ACDB_IHI /* indirect (diffuse) horizontal radiation */
		,ACDB_DNI
		,ACDB_ALTITUDE_DEG
		,ACDB_AZIMUTH_DEG
		,ACDB_SOLAR_FLAG
	};

	char field[10];
	char code[3];
	
	unsigned i;
	assert(N_FIELDS == sizeof(fieldsize) / sizeof(unsigned));

	fgets(code, 3, d->f);
	//CONSOLE_DEBUG("code = '%s'",code);
	//assert(strcmp(code,"CA")==0);

	/*
		this format includes spaces in the data file, so we can't use fscanf
		because of the way it soaks up leading spaces without counting them
		in the field width.
	*/
	for(i=0; i < N_FIELDS; ++i){
		fgets(field, fieldsize[i] + 1, d->f);
		//CONSOLE_DEBUG("field %d: size = %d, str = '%s'", i, fieldsize[i], field);
		if(i==ACDB_WHITESPACE1)continue;

		assert(field!=NULL);
		assert(strlen(field)!=0);

		data[i] = atoi(field);
	}

	fscanf(d->f," ");

#if 0
	CONSOLE_DEBUG("Time: %d/%d/%d %2d:00",data[ACDB_DAY],data[ACDB_MONTH],data[ACDB_YEAR],data[ACDB_HOUR]);
	CONSOLE_DEBUG("code = %s, dry_bulb_temp = %f, w = %f, p_atm = %f, v_wind = %f, dir_wind = %d"
		,code, data[ACDB_DRY_BULB_TEMP]/10., data[ACDB_W]/10., data[ACDB_P_ATM]/10., data[ACDB_V_WIND]/10., data[ACDB_DIR_WIND]
	);
#endif

	assert(strcmp(code,"CA")==0);
	assert(data[ACDB_DRY_BULB_TEMP]/10. <= 70.);
	assert(data[ACDB_DIR_WIND] >= 0 && data[ACDB_DIR_WIND] <= 16);

	/*
	if(month!=lastmonth || day!=lastday){
		CONSOLE_DEBUG("Reading data for %d/%d",day,month);
		lastmonth=month;
		lastday=day;
	}
	*/

	/* 
		for the moment, we only record global horizontal, direct normal,
		ambient temperature, wind speed.
	*/

	dat = &DATA(d);
	dat->t = ((day_of_year_specific(data[ACDB_DAY],data[ACDB_MONTH],data[ACDB_YEAR]) - 1)*24.0 + data[ACDB_HOUR])*3600.0;
	dat->I = data[ACDB_GHI]; /* average W/m2 for the hour in question */
	dat->Ibn = data[ACDB_DNI]; /* normal beam radiation, W/m2 */
	dat->Id = data[ACDB_IHI];
	dat->T = 0.1*data[ACDB_DRY_BULB_TEMP] + 273.15; /* temperature */
	dat->v_wind = data[ACDB_V_WIND] * 0.1;

#if 0
	if(d->i < 20){
		CONSOLE_DEBUG("ROW %d: year %d, month %d, day %d, hour %d (t = %.0f), Iegh %d, Iedn %d, Igh (%c) = %d --> I = %f"
			, d->i, year, month, day, hour, tmy->t, Iegh, Iedn, Igh_source, Igh, tmy->I
		);
	}
#endif

	d->i++;
	return 0;
}

int datareader_acdb_time(DataReader *d, double *t){
	*t = DATA(d).t;
	return 0;
}

int datareader_acdb_vals(DataReader *d, double *v){
#if ACDB_DEBUG
	CONSOLE_DEBUG("At t=%f h, T = %lf, I = %f J/m2"
		,(DATA(d).t/3600.0),DATA(d).T, DATA(d).I
	);
#endif
	v[0]=DATA(d).I;
	v[1]=DATA(d).Ibn;
	v[2]=DATA(d).Id;
	v[3]=DATA(d).T;
	v[4]=DATA(d).v_wind;
	return 0;
}
