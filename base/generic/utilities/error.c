#include <string.h>

#include "error.h"

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
		case ASC_PROG_ERROR:    sevmsg = ERR_RED "PROGRAM ERROR: " ERR_NORM; break;
		case ASC_PROG_WARNING:  sevmsg = "PROGRAM WARNING: "; break;
		case ASC_PROG_NOTE:     sevmsg = ERR_GRN; endtxt=ERR_NORM; break; /* default, keep unembellished for now */
		case ASC_USER_ERROR:    sevmsg = ERR_RED "ERROR: " ERR_NORM; break;
		case ASC_USER_WARNING:  sevmsg = ERR_BRN "WARNING: " ERR_NORM; break;
		case ASC_USER_NOTE:     sevmsg = "NOTE: "; break;
		case ASC_USER_SUCCESS:  sevmsg = ERR_GRN "SUCCESS: " ERR_NORM; break;
	}

	res = ASC_FPRINTF(ASCERR,sevmsg);
	if(filename!=NULL){
		res += ASC_FPRINTF(ASCERR,"%s:",filename);
	}
	if(line!=0){
		res += ASC_FPRINTF(ASCERR,"%d:",line);
	}
	if(funcname!=NULL){
		res += ASC_FPRINTF(ASCERR,"%s:",funcname);
	}
	if ((filename!=NULL) || (line!=0) || (funcname!=NULL)){
		res += ASC_FPRINTF(ASCERR," ");
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
    , const char *errfile
    , const int errline
    , const char *errfunc
    , const char *fmt
    , const va_list args
){
	extern error_reporter_callback_t g_error_reporter_callback;
	int res;

	if(g_error_reporter_callback==NULL){
		/* fprintf(stderr,"CALLING VFPRINTF\n"); */
		res = error_reporter_default_callback(sev,errfile,errline,errfunc,fmt,args);
	}else{
		/* fprintf(stderr,"CALLING G_ERROR_REPORTER_CALLBACK\n"); */
		res = g_error_reporter_callback(sev,errfile,errline,errfunc,fmt,args);
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
				snprintf(msg+ERROR_REPORTER_MAX_MSG-16,15,"... (truncated)");
				ASC_FPRINTF(stderr,"TRUNCATED MESSAGE, FULL MESSAGE FOLLOWS:\n----------START----------\n");
				ASC_VFPRINTF(stderr,fmt,args);
				ASC_FPRINTF(stderr,"\n-----------END----------\n");
			}
		}else{
			/* Not caching: output all in one go as a ASC_PROG_NOTE */
			res = va_error_reporter(ASC_PROG_NOTE,NULL,0,NULL,fmt,args);
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
error_reporter_start(const error_severity_t sev, const char *filename, const int line, const char *func){

	extern error_reporter_meta_t g_error_reporter_cache;
	if(g_error_reporter_cache.iscaching){
		error_reporter_end_flush();
	}
	g_error_reporter_cache.iscaching = 1;
	*(g_error_reporter_cache.msg) = '\0';
	g_error_reporter_cache.sev = sev;
	g_error_reporter_cache.filename = filename;
	g_error_reporter_cache.line = line;
	g_error_reporter_cache.func = func;

	return 1;
}

int
error_reporter_end_flush(){
	extern error_reporter_meta_t g_error_reporter_cache;

	error_reporter(
		g_error_reporter_cache.sev
		,g_error_reporter_cache.filename
		,g_error_reporter_cache.line
		,g_error_reporter_cache.func
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
    , const char *errfile
    , const int errline
    , const char *errfunc
    , const char *fmt
    , ...
){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,errfile,errline,errfunc,fmt,args);
	va_end(args);

	return res;
}

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

/*-------------------------
  OPTIONAL code for systems not supporting variadic macros.
  You know, your system probably does support variadic macros, it's just
  a question of checking what your particular syntax is...
*/

#ifdef NO_VARIADIC_MACROS
/* Following are only required on compilers without variadic macros: */

int error_reporter_note_no_line(const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(ASC_PROG_NOTE,"unknown-file",0,NULL,fmt,args);
	va_end(args);

	return res;
}

/**
	Error reporter 'here' function for compilers not supporting
	variadic macros.
*/
int error_reporter_here(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,"unknown-file",0,NULL,fmt,args);
	va_end(args);

	return res;
}


/**
	Error reporter 'no line' function for compilers not supporting
	variadic macros.
*/
int error_reporter_noline(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,NULL,0,NULL,fmt,args);
	va_end(args);

	return res;
}

int console_debug(const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = Asc_VFPrintf(ASCERR,fmt,args);
	va_end(args);

	return res;
}
#endif /* NO_VARIADIC_MACROS */
