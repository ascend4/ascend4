/*--------------------------------------------------------------------
 * $Id: sun.c,v 1.6 2004/07/05 06:57:56 pa2h Exp $
 * 
 * This file is part of libRadtran.
 * Copyright (c) 1997-2001 by Arve Kylling and Bernhard Mayer.
 * 
 * ######### Contact info: http://www.libradtran.org #########
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License   
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.        
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the   
 * GNU General Public License for more details.                    
 * 
 * You should have received a copy of the GNU General Public License          
 * along with this program; if not, write to the Free Software                
 * Foundation, Inc., 59 Temple Place - Suite 330, 
 * Boston, MA 02111-1307, USA.
 *--------------------------------------------------------------------*/


/* sun.a - Solar zenith and azimuth calculations                   @ti@ */
/*                                                                      */
/* Most of the formulas are adopted from:                               */
/*   Iqbal, Muhammad: "An Introduction to Solar Radiation",             */
/*   Academic Press, Inc., 1983                                         */
/* (page numbers in the comments refer to this book).                   */
/*                                                                      */
/* Time is specified in seconds from midnight,                          */
/* angles are specified in degrees.                                     */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "sun.h"



/* @42c@ */
/* @c42@ */
/****************************************************************************/
/* In order to use the functions provided by the sun library,     @42_10c@  */
/* #include <sun.h> in your source code and link with libRadtran_c.a.       */
/*                                                                          */
/* @strong{Example:}                                                        */
/* Example for a source file:                                               */
/* @example                                                                 */
/*                                                                          */
/*   ...                                                                    */
/*   #include "../src_c/sun.h"                                              */
/*   ...                                                                    */
/*                                                                          */
/* @end example                                                             */
/*                                                                          */
/* Linking of the executable, using the GNU compiler gcc:                   */
/* @example                                                                 */
/*                                                                          */
/*   gcc -o test test.c -lRadtran_c -L../lib                                */
/*                                                                          */
/* @end example                                                   @c42_10@  */
/****************************************************************************/


/****************************************************************************/
/* The sun library provides functions for solar zenith and azimuth @42_20c@ */
/* angle and sun-earth-distance calculations. All formulas have been taken  */
/* from Iqbal, "An introduction to solar radiation".                        */
/*                                                                 @c42_20@ */
/****************************************************************************/


/* define _PI_ */
#define _PI_ 3.1415926

/* internal functions */
static double dayangle      (int day);


/***********************************************************************************/
/* Function: dayangle                                                              */
/* Description:                                                                    */
/*  Internal function to calculate the dayangle according to Iqbal, pg. 3          */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int  day:  day of year.                                                        */
/*                                                                                 */
/* Return value:                                                                   */
/*  double dayangle, 0..2pi                                                        */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                                 */
/***********************************************************************************/

static double dayangle (int day)
{
  return 2.0 * _PI_ * (double) (day-1) / 365.0;
}



/***********************************************************************************/
/* Function: eccentricity                                                 @42_30i@ */
/* Description:                                                                    */
/*  Calculate the eccentricity correction factor E0 = (r0/r)**2 according to       */
/*  Iqbal, page 3. This factor, when multiplied with the irradiance, accounts      */
/*  for the annual variation of the sun-earth-distance.                            */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int day:  day of year (leap day is usually @strong{not} counted.               */
/*                                                                                 */
/* Return value:                                                                   */
/*  The eccentricity (double) for the specified day.                               */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

double eccentricity (int day)  
{
  double E0=0, angle=0;

  angle = dayangle (day); 

  E0 =   1.000110 + 0.034221 * cos(angle) + 0.001280 * sin(angle)
       + 0.000719 * cos(2*angle) + 0.000077 * sin(2*angle);

  return E0;
}



/***********************************************************************************/
/* Function: declination                                                  @42_30i@ */
/* Description:                                                                    */
/*  Calculate the declination for a specified day (Iqbal, page 7).                 */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int day:  day of year (leap day is usually @strong{not} counted.               */
/*                                                                                 */
/* Return value:                                                                   */
/*  The declination in degrees (double) for the specified day.                     */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

double declination (int day)
{
  double delta=0, angle=0;

  angle = dayangle (day); 

  delta =   0.006918 - 0.399912 * cos (angle) + 0.070257 * sin (angle) 
          - 0.006758 * cos (2*angle) + 0.000907 * sin (2*angle) 
          - 0.002697 * cos (3*angle) + 0.00148 * sin (3*angle);
  
  delta *= 180.0/_PI_;

  return delta;
}



/***********************************************************************************/
/* Function: equation_of_time                                             @42_30i@ */
/* Description:                                                                    */
/*  Calculate the equation of time for a specified day (Iqbal, page 11).           */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int day:  day of year (leap day is usually @strong{not} counted.               */
/*                                                                                 */
/* Return value:                                                                   */
/*  The equation of time in seconds (double) for the specified day.                */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int equation_of_time (int day)  
{
  double angle=0, et=0;

  angle = dayangle (day);

  et = (0.000075 + 0.001868 * cos(angle) - 0.032077 * sin(angle) 
        -0.014615 * cos(2*angle) - 0.04089 * sin(2*angle)) * 13750.8;

  return (int) (et+0.5);
}



/***********************************************************************************/
/* Function: LAT                                                          @42_30i@ */
/* Description:                                                                    */
/*  Calculate the local apparent time for a given standard time and location.      */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int time_std:     Standard time [seconds since midnight].                      */ 
/*  int day:          day of year (leap day is usually @strong{not} counted.       */
/*  double longitude: Longitude [degrees] (West positive).                         */
/*  double long_std:  Standard longitude [degrees].                                */
/*                                                                                 */
/* Return value:                                                                   */
/*  The local apparent time in seconds since midnight (double).                    */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int LAT (int time_std, int day, double longitude, double long_std) 
{
  int lat=0;

  lat = time_std + (int) (240.0 * (long_std-longitude)) 
                 + equation_of_time (day);

  return lat;
} 


/************************************************/
/* convert local apparent time to standard time */
/************************************************/

int standard_time (int lat, int day, double longitude, double long_std) 
{
  return lat - (int) (240.0 * (long_std-longitude)) - equation_of_time (day);
} 


/****************************/
/* hour angle omega; pg. 15 */
/****************************/

double hour_angle (int time)  
{
  double omega=0;

  omega = _PI_ * (1.0 - ((double) time) / 43200.0);

  omega *= 180.0/_PI_;
  return omega;
}



/***********************************************************************************/
/* Function: solar_zenith                                                 @42_30i@ */
/* Description:                                                                    */
/*  Calculate the solar zenith angle for a given time and location.                */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int time:         Standard time [seconds since midnight].                      */
/*  int day:          day of year (leap day is usually @strong{not} counted.       */
/*  double latitude:  Latitude [degrees] (North positive).                         */
/*  double longitude: Longitude [degrees] (West positive).                         */
/*  double long_std:  Standard longitude [degrees].                                */
/*                                                                                 */
/* Return value:                                                                   */
/*  The solar zenith angle [degrees].                                              */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

double solar_zenith (int time, int day, 
		     double latitude, double longitude, double long_std)
{
  double theta=0, omega=0, delta=0, phi = latitude*_PI_/180.0;
  int lat = LAT (time, day, longitude, long_std);

  delta = _PI_/180.0*declination (day);
  omega = _PI_/180.0*hour_angle (lat);
    
  theta = acos(sin(delta) * sin(phi) + cos(delta) * cos(phi) * cos(omega));

  theta *= (180.0/_PI_);   /* convert to degrees */
  return theta;
}




/***********************************************************************************/
/* Function: solar_azimuth                                                @42_30i@ */
/* Description:                                                                    */
/*  Calculate the solar azimuth angle for a given time and location.               */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int time:         Standard time [seconds since midnight].                      */
/*  int day:          day of year (leap day is usually @strong{not} counted.       */
/*  double latitude:  Latitude [degrees] (North positive).                         */
/*  double longitude: Longitude [degrees] (West positive).                         */
/*  double long_std:  Standard longitude [degrees].                                */
/*                                                                                 */
/* Return value:                                                                   */
/*  The solar azimuth angle [degrees].                                             */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

double solar_azimuth (int time, int day, 
		      double latitude, double longitude, double long_std)
{
  double theta=0, omega=0, delta=0, phi = latitude*_PI_/180.0, psi=0, cospsi=0;
  int lat = LAT (time, day, longitude, long_std);

  delta = _PI_/180.0*declination (day);
  omega = _PI_/180.0*hour_angle (lat);
  theta = _PI_/180.0*solar_zenith (time, day, latitude, longitude, long_std);


  cospsi = (cos(theta)*sin(phi) - sin(delta)) / sin(theta) / cos (phi);
  /* allow tiny roundoff errors */
  if (cospsi>1.0 && cospsi<1.0+1e-6)
    cospsi=1.0;

  if (cospsi<-1.0 && cospsi>-1.0-1e-6)
    cospsi=-1.0;

  psi = -acos(cospsi);

  /* adjust sign */
  if (lat>43200 || lat<0)
    psi=-psi;

  psi *= (180.0/_PI_);   /* convert to degrees */
  
  return psi;
}


/**
	Calculate the day of year for given data (ignoring leap years)

	Use this function when you don't know specifically what year you're dealing
	with.

	@param day Day of month (1..31)
	@param month Month (1..12)

	@return -1 on error, else the day of the year, with 1 = the first of January

	@note If your number of days exceeds the number in the month you've given,
	your value will overflow into the next month.
*/
int day_of_year (int day, int month)
{
  char mon =  0;
  int  doy = -1;

  if (month<1 || month > 12)
    return (-1);

  if (day<1 || day > 31)
    return (-1);
  
  mon = (char) month;

  switch (mon)  {
  case 1: doy = day + 0; break;
  case 2: doy = day + 31; break;
  case 3: doy = day + 59; break;
  case 4: doy = day + 90; break;
  case 5: doy = day + 120; break;
  case 6: doy = day + 151; break;
  case 7: doy = day + 181; break;

  case 8: doy = day + 212; break;
  case 9: doy = day + 243; break;
  case 10: doy = day + 273; break;
  case 11: doy = day + 304; break;
  case 12: doy = day + 334; break;
  default: doy = -1;
  }

  return (doy);
}

/**
	is the specified year a leap year?

	@param year the Gregorian year A.D. (no constraints are asserted)

	@see http://h71000.www7.hp.com/openvms/products/year-2000/leap.html
	@see http://www.mitre.org/tech/cots/LEAPCALC.html
	
	returns integer 1 if the year is a leap year, else 0.

	@note A leap year is a year which has 29 days in february, instead of the usual 28.
*/
char is_leap_year(int year){
	if(year % 4 != 0)return 0; /* use 28 for days in February */
	if(year % 400 == 0)return 1; /* use 29 for days in February */
	if(year % 100 == 0)return 0; /* use 28 for days in February */
	return 1; /* use 29 for days in February */
}

/**
	day of year, for a specified year (corrects for leap year)
*/
int day_of_year_specific(int day, int month, int year){
	int d;
	d=day_of_year(day,month);
	if(is_leap_year(year) && month>=3){
		d++;
	}
	return d;
}

/**************************************************************************/
/* convert time to string                                                 */
/* memory for timestr must be allocated by programmer (at least 10 bytes) */
/**************************************************************************/

char *time2str (char *timestr, int hour, int min, int sec)
{
  char hourstr[3] = "";
  char minstr[3]  = "";
  char secstr[3]  = "";

  strcpy (timestr, "");

  if (hour<0 || hour>24)  
    return NULL;

  if (min<0 || min>60)  
    return NULL;

  if (sec<0 || sec>60)  
    return NULL;



  if (hour<10)
    sprintf (hourstr, "0%d", hour);
  else 
    sprintf (hourstr,  "%d", hour);
    
  if (min<10)
    sprintf (minstr, "0%d", min);
  else 
    sprintf (minstr,  "%d", min);
      
  if (sec<10)
    sprintf (secstr, "0%d", sec);
  else 
    sprintf (secstr,  "%d", sec);
      
  sprintf (timestr, "%s:%s:%s", hourstr, minstr, secstr);

  return timestr;
}




/***********************************************************************************/
/* Function: zenith2time                                                  @42_30i@ */
/* Description:                                                                    */
/*  Calculate the times for a given solar zenith angle, day of year and location.  */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int day:              day of year                                              */
/*  double zenith_angle:  Solar zenith angle [degrees].                            */
/*  double latitude:      Latitude [degrees] (North positive).                     */
/*  double longitude:     Longitude [degrees] (West positive).                     */
/*  double long_std:      Standard longitude [degrees].                            */
/*  int *time1:           1st time of occurence.                                   */ 
/*  int *time2:           2nd time of occurence.                                   */ 
/*                                                                                 */
/* Return value:                                                                   */
/*  0  if o.k., <0 if error.                                                       */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int zenith2time (int day, 
		 double zenith_angle,
		 double latitude,
		 double longitude,
		 double long_std,
		 int *time1, 
		 int *time2)
{
  double delta = _PI_/180.0*declination (day);
  double phi   = _PI_/180.0*latitude;
  double theta = _PI_/180.0*zenith_angle;
  double cos_omega = (cos(theta) - sin(delta)*sin(phi)) / 
                      cos(delta) / cos(phi);
  double omega1=0, omega2=0;
  int lat1=0, lat2=0;

  if (fabs(cos_omega) > 1.0)
    return ERROR_NO_ZENITH;
  
  omega1 = acos (cos_omega);
  omega2 = 0.0-omega1;
  
  lat1 = 43200*(1-omega1/_PI_);
  lat2 = 43200*(1-omega2/_PI_);
  
  *time1 = standard_time (lat1, day, longitude, long_std);
  *time2 = standard_time (lat2, day, longitude, long_std);

  return 0;
}





/***********************************************************************************/
/* Function: Gregorian2Julian                                             @42_30i@ */
/* Description:                                                                    */
/*  Convert from Gregorian day (day, month, year) to Julian day (by the            */
/*  astronomical definition). This function, in combination with                   */
/*  Julian2Gregorian() is very useful to answer questions like "which date is      */
/*  666 days after December 31, 1999?" Algorithm from                              */
/*  H.F. Fliegel and T.C. Van Flandern, "A Machine Algorithm for Processing        */
/*  Calendar Dates", Communications of the Association for Computing Machinery     */ 
/*  (CACM), Vol. 11, No. 10, 657, 1968.                                            */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int d:            Day of month (1..31).                                        */
/*  int m:            Month (1..12).                                               */
/*  int y:            Year (attention: full year required, 1999 instead of 99)     */
/*  int *jd:          The Julian day, to be calculated.                            */
/*                                                                                 */
/* Return value:                                                                   */
/*  0  if o.k., <0 if error.                                                       */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int Gregorian2Julian (int d, int m, int y, int *jd) 
{
  char ystr[3]="";

  /* reset output */
  *jd =0;
  
  if (d<1||d>31)
    return -1;
  
  if (m<1||m>12)
    return -1;

  if (y>0&&y<100) {
    if (y<10)
      sprintf (ystr, "0%d", y);
    else 
      sprintf (ystr, "%d", y);

    fprintf (stderr, "Warning: Gregorian2Julian() has been called for year %s\n", ystr);
    fprintf (stderr, "Maybe you meant 19%s or 20%s?\n", ystr, ystr);
  }


  *jd = (1461 * (y + 4800 + (m - 14) / 12)) / 4 + 
    (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 -
    (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075;
  
  return 0;  /* if o.k. */
}



/***********************************************************************************/
/* Function: Julian2Gregorian                                             @42_30i@ */
/* Description:                                                                    */
/*  Convert from Julian day (by the astronomical definition) to Gregorian day      */
/*  (day, month, year) to . This function, in combination with                     */
/*  Gregorian2Julian() is very useful to answer questions like "which date is      */
/*  666 days after December 31, 1999?" Algorithm from                              */
/*  H.F. Fliegel and T.C. Van Flandern, "A Machine Algorithm for Processing        */
/*  Calendar Dates", Communications of the Association for Computing Machinery     */ 
/*  (CACM), Vol. 11, No. 10, 657, 1968.                                            */
/*                                                                                 */
/* Parameters:                                                                     */
/*  int *d:           Day of month (1..31), to be calculated.                      */
/*  int *m:           Month (1..12), to be calculated.                             */
/*  int *y:           Year, to be calculated.                                      */
/*  int jd:           The Julian day.                                              */
/*                                                                                 */
/* Return value:                                                                   */
/*  0  if o.k., <0 if error.                                                       */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int Julian2Gregorian(int *d, int *m, int *y, int jd) 
{
  int l=0, n=0, i=0, j=0;

  l  = jd + 68569;
  n  = ( 4 * l ) / 146097;
  l  = l - ( 146097 * n + 3 ) / 4;
  i  = ( 4000 * ( l + 1 ) ) / 1461001;
  l  = l - ( 1461 * i ) / 4 + 31;
  j  = ( 80 * l ) / 2447;
  *d = l - ( 2447 * j ) / 80;
  l  = j / 11;
  *m = j + 2 - ( 12 * l );
  *y = 100 * ( n - 49 ) + i + l;

  return 0;
}


/***********************************************************************************/
/* Function: location                                                     @42_30i@ */
/* Description:                                                                    */
/*  Return latitude, longitude, and standard longitude for a given location.       */
/*                                                                                 */
/* Parameters:                                                                     */
/*  double *latitude:  Latitude (North positive).                                  */
/*  double *longitude: Longitude (West positive).                                  */
/*  double *long_std:  Standard longitude (West positive).                         */
/*  char   *location:  String identifying the location.                            */
/*                                                                                 */
/* Return value:                                                                   */
/*  0  if o.k., <0 if error.                                                       */
/*                                                                                 */
/* Example:                                                                        */
/* Files:                                                                          */
/* Known bugs:                                                                     */
/* Author:                                                                         */
/*                                                                        @i42_30@ */
/***********************************************************************************/

int location (char *locstr, double *latitude, double *longitude, double *long_std) 
{

  if (!strcasecmp(locstr, "ifu"))  {
    *latitude  =  47.48;
    *longitude = -11.07;
    *long_std  = -15.00;

    return 0;
  }

  if (!strcasecmp(locstr, "dlrop"))  {
    *latitude  =  48.088;
    *longitude = -11.280;
    *long_std  = -15.000;

    return 0;
  }


  return -1;
}
