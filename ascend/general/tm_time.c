/*
 *  Time module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: tm_time.c,v $
 *  Date last modified: $Date: 2000/01/25 02:21:26 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the Ascend Language Interpreter.
 *
 *  Copyright (C) 1997 Carnegie Mellon University
 *
 *  The Ascend Language Interpreter is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The Ascend Language Interpreter is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <time.h>
#include "platform.h"
#include "panic.h"
#include "tm_time.h"

static boolean f_first = TRUE;

double tm_cpu_time(void)
{
   static clock_t ref;
   static double dref;
   clock_t now;
   double dnow;

   if( f_first ) {
      ref = clock();
      dref = (double) ref;
      f_first = FALSE;
   }
   now = clock();
   dnow = (double) now;

   return((dnow - dref)/CLOCKS_PER_SEC);
}

double tm_reset_cpu_time(void)
{
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
