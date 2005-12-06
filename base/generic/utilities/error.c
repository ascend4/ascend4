#include <string.h>

#include "error.h"

/* Don't use the XTERM colour coldes in Windows: */
#ifndef __WIN32__
#define TEST_DEFAULT_FPRINTF
#endif

#ifdef TEST_DEFAULT_FPRINTF
/** XTERM colour codes used to distinguish between errors of different types.

	@TODO some runtime testing to determine if these should be used or not
	depending on TERM env var.
*/
#  define ERR_RED "\033[31;1m"
#  define ERR_GRN "\033[32;2m"
#  define ERR_NORM "\033[0m"
#else
#  define ERR_RED ""
#  define ERR_GRN ""
#  define ERR_NORM ""
#endif

/**
	Global variable which stores the pointer to the callback
	function being used.
*/
static error_reporter_callback_t g_error_reporter_callback;

/**
	Global variable which holds cached error info for
	later output
*/
static error_reporter_meta_t g_error_reporter_cache;

/** 
	Default error reporter. To use this error reporter, set
	the callback pointer to NULL.
*/
int error_reporter_default_callback(ERROR_REPORTER_CALLBACK_ARGS){
	char *sevmsg="";
	char *endtxt="\n";
	int res=0;
	switch(sev){
		case ASC_PROG_FATAL:    sevmsg = ERR_RED "PROGRAM FATAL ERROR: " ERR_NORM; break;
		case ASC_PROG_ERROR:    sevmsg = "PROGRAM ERROR: "; break;
		case ASC_PROG_WARNING:  sevmsg = "PROGRAM WARNING: "; break;
		case ASC_PROG_NOTE:     sevmsg = ERR_GRN; endtxt=ERR_NORM; break; /* default, keep unembellished for now */
		case ASC_USER_ERROR:    sevmsg = "ERROR: "; break;
		case ASC_USER_WARNING:  sevmsg = "WARNING: "; break;
		case ASC_USER_NOTE:     sevmsg = "NOTE: "; break;
		case ASC_USER_SUCCESS:  sevmsg = "SUCCESS: "; break;
	}
	
	res = ASC_FPRINTF(ASCERR,sevmsg);
	if(filename!=NULL){
		res += ASC_FPRINTF(ASCERR,"%s:%d: ",filename,line);
	}
	res += ASC_VFPRINTF(ASCERR,fmt,args);
	res += ASC_FPRINTF(ASCERR,endtxt);
	
	return res;
}

/*--------------------------
  REPORT the error
*/
int
va_error_reporter(
		const error_severity_t sev
		, const char *errfile, const int errline
		, const char *fmt
		, va_list args
){
	extern error_reporter_callback_t g_error_reporter_callback;
	int res;

	if(g_error_reporter_callback==NULL){
		/* fprintf(stderr,"CALLING VFPRINTF\n"); */
		res = error_reporter_default_callback(sev,errfile,errline,fmt,args);
	}else{
		/* fprintf(stderr,"CALLING G_ERROR_REPORTER_CALLBACK\n"); */
		res = g_error_reporter_callback(sev,errfile,errline,fmt,args);
	}
	
	return res;
}

/*----------------------------
  DROP-IN replacements for stdio.h / ascPrint.h
*/

/**
	This function performs caching of the error text if the flag is set
*/
int
fprintf_error_reporter(FILE *file, const char *fmt, ...){
	va_list args;
	extern error_reporter_meta_t g_error_reporter_cache;
	char *msg;
	int len;
	int res;

	/* fprintf(stderr,"ENTERED FPRINTF_ERROR_REPORTER\n"); */

	va_start(args,fmt);
	if(file==stderr){
		if(g_error_reporter_cache.iscaching){
			msg = g_error_reporter_cache.msg;
			len = strlen(msg);
			res = vsnprintf(msg+len,ERROR_REPORTER_MAX_MSG-len,fmt,args);
			if(len+res+1>=ERROR_REPORTER_MAX_MSG){
				snprintf(msg+ERROR_REPORTER_MAX_MSG-15,14,"... (truncate)");
				ASC_FPRINTF(stderr,"TRUNCATED MESSAGE, FULL MESSAGE FOLLOWS:\n----------START----------\n");
				ASC_VFPRINTF(stderr,fmt,args);
				ASC_FPRINTF(stderr,"\n-----------END----------\n");
			}
		}else{
			/* Not caching: output all in one go as a ASC_PROG_NOTE */
			res = va_error_reporter(ASC_PROG_NOTE,NULL,0,fmt,args);
		}
	}else{
		res = ASC_VFPRINTF(file,fmt,args);
	}

	va_end(args);

	return res;
}

int
fputc_error_reporter(int c, FILE *file){
	if(file!=stderr){
		return ASC_FPUTC(c,file);
	}else if(fprintf_error_reporter(file,"%c",c) == 1){
		return c;
	}else{
		return EOF;
	}
}

int
fflush_error_reporter(FILE *file){
	if(file!=stderr){
		return ASC_FFLUSH(file);
	}else{
		return error_reporter_end_flush();
	}
}

/*----------------------------
  CACHING of multiple-FPRINTF error messages
*/

int
error_reporter_start(const error_severity_t sev, const char *filename, const int line){
	
	extern error_reporter_meta_t g_error_reporter_cache;
	if(g_error_reporter_cache.iscaching){
		error_reporter_end_flush();
	}
	g_error_reporter_cache.iscaching = 1;
	*(g_error_reporter_cache.msg) = '\0';
	g_error_reporter_cache.sev = sev;
	g_error_reporter_cache.filename = filename;
	g_error_reporter_cache.line = line;

	return 1;
}

int
error_reporter_end_flush(){
	extern error_reporter_meta_t g_error_reporter_cache;
	extern error_reporter_callback_t g_error_reporter_callback;

	error_reporter(
		g_error_reporter_cache.sev
		,g_error_reporter_cache.filename
		,g_error_reporter_cache.line
		,g_error_reporter_cache.msg
	);
	g_error_reporter_cache.iscaching = 0;

	return 0; /* output must be compatible with fflush */
}

/*--------------------------
  REPORT the error
*/
int
error_reporter(
		const error_severity_t sev
		, const char *errfile, const int errline
		, const char *fmt, ...
){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,errfile,errline,fmt,args);
	va_end(args);

	return res;
}

#ifndef HAVE_C99
int error_reporter_note_no_line(const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(ASC_PROG_NOTE,"unknown-file",0,fmt,args);
	va_end(args);

	return res;
}
#endif

/*-------------------------
  SET the callback function
*/
void
error_reporter_set_callback(
		const error_reporter_callback_t new_callback
){
	extern error_reporter_callback_t g_error_reporter_callback;
	g_error_reporter_callback = new_callback;
}

