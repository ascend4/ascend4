/**< 
 *  Time module
 *  by Karl Westerberg
 *  Created: 6/90
 *  Version: $Revision: 1.1 $
 *  Version control file: $RCSfile: tm_time.h,v $
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


/**< 
 *  Contents:     Time module
 *
 *  Authors:      Karl Westerberg
 *                Joseph Zaher
 *
 *  Dates:        06/90 - original version
 *                08/93 - removed CLOCKS_PER_SECOND which should be
 *                        provided by the standard library, eliminated
 *                        tm_TPS and changed tm_run_time() and
 *                        tm_cpu_time() to return number of seconds
 *                06/94 - eliminated cute calendar functions for full
 *                        ANSI compatibility.
 *
 *  Description:  Provide a function to monitor elapsed time during the
 *                course of execution of a program and standard ANSI
 *                primitives for performing calendar calculations.
 * 
 *                We apologize for the existence of this file and for
 * 		  its name being the same as an ANSI C header which is not
 *		  consistently available, unfortunately. We really
 *		  ought not to need this header.
 */
#ifndef tm_module_loaded

#ifndef CLOCKS_PER_SEC

/**< case linux */
#ifdef linux
#define CLOCKS_PER_SEC  100
#endif /**< linux */

/**< case windoze */
#ifdef WIN
#define CLOCKS_PER_SEC 1000
#endif /**< windoze */

/**< default */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC  1000000         /**< typical clock ticks per sec */
/**< note that typically clocks have a minimum resolution of 16666, sigh */
#endif /**< default */

#endif /**< CLOCKS_PER_SEC */

extern double tm_cpu_time();
/**< 
 *  Returns the number of seconds used during
 *  processing by the CPU since any part of
 *  ASCEND first called tm_cpu_time().
 *  Users timing portions of the process should maintain
 *  their own start time and work by difference as the
 *  solvers do.
 */


extern void tm_cpu_time_ftn_();
extern void aftime_();
extern void tm_cpu_time_ftn();
extern void aftime();
extern void TM_CPU_TIME_FTN();
extern void AFTIME();
/**< 
 *  double *time;
 *  Does the same thing as tm_cpu_time, with the exception
 *  that this function takes a double * arg, to satisfy
 *  fortran's call by reference semantics. The short name is
 *  to satisfy fortrans 6 char restriction.
 *
 *  f77 usage:  t is real*8 (double precision on most f77)
 *  external aftime, tm_cpu_time_ftn
 *  call tm_cpu_time_ftn(t)
 *  call aftime(t)
 *  On return t will have a time value (sec) stored in a double.
 *  If your F77 compiler doesn't morph all function calls to
 *  lower case (with or without trailing underbar), you will need
 *  to figure out its morphing convention and modify tm.[ch] so.
 */

#endif
