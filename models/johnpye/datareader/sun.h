/*--------------------------------------------------------------------
 * $Id: sun.h,v 1.4 2004/01/12 17:18:04 arve Exp $
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

#ifndef __sun_h
#define __sun_h

#if defined (__cplusplus)
extern "C" {
#endif


/* error messages */
#define ERROR_NO_ZENITH     -1


/* prototypes */

double eccentricity  (int day);            /* day of year          */

double declination   (int day);            /* day of year          */

int equation_of_time (int day);            /* day of year          */

int LAT              (int time,            /* standard time        */       
		      int day,             /* day of year          */
		      double longitude,    /* longitude            */
		      double long_std);    /* standard longitude   */

double hour_angle    (int time);           /* local apparent time  */

double solar_zenith  (int time,            /* standard time        */
		      int day,             /* day of year          */
		      double latitude,     /* latitude             */
		      double longitude,    /* longitude            */  
		      double long_std);    /* standard longitude   */

double solar_azimuth (int time,            /* standard time        */
		      int day,             /* day of year          */
		      double latitude,     /* latitude             */
		      double longitude,    /* longitude            */
		      double long_std);	   /* standard longitude   */

int zenith2time      (int day,             /* day of year          */
		      double zenith_angle, /* zenith angle [deg]   */
		      double latitude,     /* latitude [deg]       */
		      double longitude,    /* longitude [deg]      */
		      double long_std,     /* standard longitude   */
		      int *time1,          /* first time           */
		      int *time2);         /* second time          */

int day_of_year (int day,                  /* day of month (1..31) */ 
		 int month);               /* month        (1..12) */       

char *time2str  (char *timestr,            /* buffer for timestr   */
		 int hour,                 /* hour                 */
		 int min,                  /* minute               */
		 int sec);                 /* second               */

int Gregorian2Julian (int d,  int m,  int y,  int *jd); 
int Julian2Gregorian (int *d, int *m, int *y, int jd);

int location (char *locstr, double *a, double *o, double *s);

#if defined (__cplusplus)
}
#endif


#endif







