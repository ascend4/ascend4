/*	ASCEND modelling environment
	Copyright (C) 1997 Carnegie Mellon University
	Copyright (C) 2015 John Pye

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
*//*
	Time module by Karl Westerberg, created: 6/1990
	Last in CVS: $Date: 2000/01/25 02:21:26 $ $Author: ballan $
*/

#include <sys/time.h>
#include "platform.h"
#include "panic.h"
#include "tm_time.h"

#ifdef __WIN32__
# include "Windows.h"
#endif

#include <stdio.h>

static boolean f_first = TRUE;

double tm_cpu_time(void){
#ifndef __WIN32__
	static struct timespec ref;
	struct timespec now;

	if( f_first ) {
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ref);
		f_first = FALSE;
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);
	return (now.tv_sec - ref.tv_sec) + 1e-9*(now.tv_nsec - ref.tv_nsec);
#else /* WIN32 */
	static LARGE_INTEGER ref, f;
	LARGE_INTEGER now;
	if(f_first){
		QueryPerformanceFrequency(&f);
		//fprintf(stderr,"\nQueryPerformanceFrequency returns: %lld clocks/s\n",f.QuadPart);
		QueryPerformanceCounter(&ref);
		//fprintf(stderr,"ref = %lld, f = %lld\n",ref.QuadPart,f.QuadPart);
		f_first = FALSE;
		return 0;
	}
	QueryPerformanceCounter(&now);
	//fprintf(stderr,"we got now=%lld, ref=%lld, f = %lld\n",now.QuadPart,ref.QuadPart,f.QuadPart);
	double dt = ((double)(now.QuadPart - ref.QuadPart)/f.QuadPart);
	//fprintf(stderr,"dt = %lf\n",dt);
	return dt;
#endif
}

double tm_reset_cpu_time(void){
  //fprintf(stderr,"RESET CLOCK\n");
  f_first = TRUE;
  return tm_cpu_time();
}

void tm_cpu_time_ftn_(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}

void aftime_(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}

void tm_cpu_time_ftn(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}

void aftime(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}

void TM_CPU_TIME_FTN(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}

void AFTIME(double *t)
{
  asc_assert(NULL != t);
  *t = tm_cpu_time();
}
