/*	ASCEND modelling environment
	Copyright (C) 1997 Benjamin Andrew Allan
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
*//* @file
	Some nice syntax for exceptions in C.
	See http://www.swig.org/Doc1.1/HTML/Exceptions.html#n3

	Note that you will enter a world of pain if you attempt to nest these
	try...catch statements.

	Usage example from the above link
	@CODE
	try {
		$function
	} catch(RangeError) {
		croak("Range Error");
	} catch(DivisionByZero) {
		croak("Division by zero");
	} catch(OutOfMemory) {
		croak("Out of memory");
	} finally {
		croak("Unknown exception");
	}
	@ENDCODE

*/
#ifndef ASC_EXCEPT_H
#define ASC_EXCEPT_H

/* File : except.h */
#include <setjmp.h>
extern jmp_buf exception_buffer;
extern int exception_status;

#define TRY        if((exception_status = setjmp(exception_buffer)) == 0)
#define CATCH(val) else if(exception_status == val)
#define THROW(val) longjmp(exception_buffer,val)
#define FINALLY    else

/* Exception codes */

#define RangeError     1
#define DivisionByZero 2
#define OutOfMemory    3

/* #define ASC_JMP_INFO */
/**< Whether to store additional information before making a setjmp call */

#ifndef ASC_JMP_INFO
# define SETJMP setjmp
# define LONGJMP longjmp
# define SIGNAL signal
# define JMP_BUF jmp_buf
#else
# define SETJMP(ENV) (\
		CONSOLE_DEBUG("SETJMP at %s:%d (%s=%p)",__FILE__,__LINE__,#ENV,ENV.jmp)\
		,ENV.filename = __FILE__, ENV.line = __LINE__, ENV.func = __FUNCTION__\
		,ENV.varname = #ENV\
		, setjmp(ENV.jmp)\
	)
# define LONGJMP(ENV,VAL) (\
		CONSOLE_DEBUG("LONGJMP to %s:%d (%s) (%s=%p)",ENV.filename,ENV.line,ENV.func,ENV.varname,ENV.jmp)\
		, longjmp(ENV.jmp, VAL)\
	)
typedef struct{
	jmp_buf jmp;
	const char *filename;
	int line;
	const char *func;
	const char *varname;
} asc_jmp_buf;
#define JMP_BUF asc_jmp_buf
#define SIGNAL(SIG,HANDLER) (CONSOLE_DEBUG("SIGNAL(%d,%s)",SIG,#HANDLER),signal(SIG,HANDLER))
#endif

#endif
