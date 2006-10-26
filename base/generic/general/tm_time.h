/*	ASCEND modelling environment
	Copyright (C) 1997 Carnegie Mellon University
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
	Time module

	Provide a function to monitor elapsed time during the
	course of execution of a program and standard ANSI
	primitives for performing calendar calculations.

	@TODO (Apology) We apologize for the existence of this file and for
	its name being the same as an ANSI C header which is not
	consistently available, unfortunately. We really
	ought not to need this header.

	06/90 - original version
	08/93 - removed CLOCKS_PER_SECOND which should be
			provided by the standard library, eliminated
			tm_TPS and changed tm_run_time() and
			tm_cpu_time() to return number of seconds
	06/94 - eliminated cute calendar functions for full
			ANSI compatibility.

	Requires:
	#include "utilities/ascConfig.h"
*//*
	by Karl Westerberg and Joseph Zaher
	Created: 6/90
	Version: $Revision: 1.1 $
	Version control file: $RCSfile: tm_time.h,v $
	Date last modified: $Date: 2000/01/25 02:21:26 $
	Last modified by: $Author: ballan $
*/

#ifndef ASC_TM_TIME_H
#define ASC_TM_TIME_H

#include <utilities/ascConfig.h>

#ifndef CLOCKS_PER_SEC

/* case linux */
#ifdef linux
#define CLOCKS_PER_SEC  100      /**< Typical clock ticks per sec. */
#endif /* linux */

/* case windoze */
#ifdef __WIN32__
#define CLOCKS_PER_SEC 1000      /**< Typical clock ticks per sec. */
#endif /* windoze */

/* default */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  1000000
/**<
 *  Typical clock ticks per sec.  Note that typically clocks
 *  have a minimum resolution of 16666, sigh.
 */
#endif /* default */

#endif /* CLOCKS_PER_SEC */

ASC_DLLSPEC(double) tm_cpu_time(void);
/**<
 *  Returns elapsed CPU time in seconds since the first call
 *  to a timing function.  The timing functions that, when
 *  initially called, set the start time for all others are:
 *    - tm_cpu_time()
 *    - tm_cpu_time_ftn_()
 *    - aftime_()
 *    - tm_cpu_time_ftn()
 *    - aftime()
 *    - TM_CPU_TIME_FTN()
 *    - AFTIME()
 *
 *  Users timing portions of the process should maintain their
 *  own start time and work by difference as the solvers do.
 *
 *  @return The elapsed CPU time since the 1st call to a timing function.
 */

extern double tm_reset_cpu_time(void);
/**<
 *  Resets the start time.  Use with caution if there are
 *  multiple callers depending on a constant start time stamp.
 *  This function is primarily for testing purposes.
 *
 *  @return The initiallized elapsed CPU time.
 */

ASC_DLLSPEC(void ) tm_cpu_time_ftn_(double *timef);
/**<
 *  Stores elapsed CPU time in seconds since the first call
 *  to a timing function in *t*.  The timing functions that, when
 *  initially called, set the start time for all others are:
 *    - tm_cpu_time()
 *    - tm_cpu_time_ftn_()
 *    - aftime_()
 *    - tm_cpu_time_ftn()
 *    - aftime()
 *    - TM_CPU_TIME_FTN()
 *    - AFTIME()
 *
 *  This function takes a (double *) to satisfy FORTRAN's call
 *  by reference semantics.
 *  <pre>
 *  f77 usage:  t is real*8 (double precision on most f77)
 *              external aftime, tm_cpu_time_ftn
 *              call tm_cpu_time_ftn(t)
 *              call aftime(t)
 *  </pre>
 *  On return t will have a time value (sec) stored in a double.
 *  The specified t may not be NULL (checked by assertion).<br><br>
 *
 *  If your F77 compiler doesn't morph all function calls to
 *  lower case (with or without trailing underbar), you will need
 *  to figure out its morphing convention and modify tm.[ch] so.
 *
 *  @param t Location to store the elapsed CPU time in seconds.
 */

ASC_DLLSPEC(void ) aftime_(double *timev);
/**< Short name for tm_cpu_time_ftn_() to satisfy FORTRAN's 6 char restriction. */
ASC_DLLSPEC(void ) tm_cpu_time_ftn(double *timev);
/**< Variant of tm_cpu_time_ftn_(). */
ASC_DLLSPEC(void ) aftime(double *timev);
/**< Short name for tm_cpu_time_ftn() to satisfy FORTRAN's 6 char restriction. */
ASC_DLLSPEC(void ) TM_CPU_TIME_FTN(double *timev);
/**< Variant of tm_cpu_time_ftn_(). */

ASC_DLLSPEC(void) AFTIME(double *timev);
/**< Short name for TM_CPU_TIME_FTN() to satisfy FORTRAN's 6 char restriction. */

#endif  /* ASC_TM_TIME_H */

