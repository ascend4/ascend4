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
 *  along with the program; if not, write to the Free Software Foundation,
 *  Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named
 *  COPYING.  COPYING is in ../compiler.
 */

#include <time.h>
#include "utilities/ascConfig.h"
#include "general/tm_time.h"

double tm_cpu_time()
{
   static boolean first = TRUE;
   static clock_t ref;
   static double dref;
   static double dcps;
   clock_t now;
   double dnow;

   if( first ) {
      dcps = (double) CLOCKS_PER_SEC;
      ref = clock();
      dref = (double) ref;
      first = FALSE;
   }
   now = clock();
   dnow = (double) now;

   return( (dnow - dref)/dcps );
}

void tm_cpu_time_ftn_(double *t)
{
  *t = tm_cpu_time();
}

void aftime_(double *t)
{
  *t = tm_cpu_time();
}

void tm_cpu_time_ftn(double *t)
{
  *t = tm_cpu_time();
}

void aftime(double *t)
{
  *t = tm_cpu_time();
}

void TM_CPU_TIME_FTN(double *t)
{
  *t = tm_cpu_time();
}

void AFTIME(double *t)
{
  *t = tm_cpu_time();
}
