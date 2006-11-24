#include <string.h>

#include "error.h"

#define ERROR_REPORTER_TREE_ACTIVE

#ifdef ERROR_REPORTER_TREE_ACTIVE
# include "ascMalloc.h"
# include "ascPanic.h"
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

#ifdef ERROR_REPORTER_TREE_ACTIVE
static error_reporter_meta_t *error_reporter_meta_new(){
	error_reporter_meta_t *e;
	e = ASC_NEW(error_reporter_meta_t);
	e->sev = ASC_USER_SUCCESS;
	e->iscaching = 0;
	e->filename = NULL;
	e->func = NULL;
	e->line = 0;
	e->msg[0] = '\0';
	return e;
}
#endif /* ERROR_REPORTER_TREE_ACTIVE */
	
/**
	XTERM colour codes used to distinguish between errors of different types.
*/
#  define ERR_RED "31;1"
#  define ERR_GRN "32;2"
#  define ERR_BLU "34;1"
#  define ERR_BRN "33;1"
#  define ERR_BOLD "1"

/**
	Default error reporter. To use this error reporter, set
	the callback pointer to NULL.
*/
int error_reporter_default_callback(ERROR_REPORTER_CALLBACK_ARGS){
	char *sevmsg="";
	char *color=NULL;
	char *endtxt="\n";
	int res=0;
	switch(sev){
		case ASC_PROG_FATAL:    color=ERR_RED; sevmsg = "PROGRAM FATAL ERROR: "; break;
		case ASC_PROG_ERROR:    color=ERR_RED; sevmsg = "PROGRAM ERROR: "; break;
		case ASC_PROG_WARNING:  color=ERR_BOLD;sevmsg = "PROGRAM WARNING: "; break;
		case ASC_PROG_NOTE:     color=ERR_GRN; endtxt=""; break; /* default, keep unembellished for now */
		case ASC_USER_ERROR:    color=ERR_RED; sevmsg = "ERROR: "; break;
		case ASC_USER_WARNING:  color=ERR_BRN; sevmsg = "WARNING: "; break;
		case ASC_USER_NOTE:     sevmsg = "NOTE: "; break;
		case ASC_USER_SUCCESS:  color=ERR_GRN; sevmsg = "SUCCESS: "; break;
	}

	color_on(ASCERR,color);
	res = ASC_FPRINTF(ASCERR,sevmsg);
	color_off(ASCERR);

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

/*---------------------------------------------------------------------------
  ERROR REPORTER TREE (BRANCHABLE ERROR STACK) FUNCTIONALITY
*/

#ifdef ERROR_REPORTER_TREE_ACTIVE

static error_reporter_tree_t *g_error_reporter_tree = NULL;
static error_reporter_tree_t *g_error_reporter_tree_current = NULL;

# define TREECURRENT g_error_reporter_tree_current
# define TREE g_error_reporter_tree

static int error_reporter_tree_write(error_reporter_tree_t *t);
static void error_reporter_tree_free(error_reporter_tree_t *t);

static error_reporter_tree_t *error_reporter_tree_new(){
	error_reporter_tree_t *tnew = ASC_NEW(error_reporter_tree_t);
	tnew->next = NULL;
	tnew->head = NULL;
	tnew->tail = NULL;
	tnew->err = NULL;
	return tnew;
}

int error_reporter_tree_start(){
	error_reporter_tree_t *tnew;
	tnew = error_reporter_tree_new();

	CONSOLE_DEBUG("TREE = %p",TREE);
	CONSOLE_DEBUG("TREECURRENT = %p",TREECURRENT);

#if 0
	if(TREE != NULL && TREECURRENT == NULL){
		CONSOLE_DEBUG("CALLED WITH NULL TREECURRENT BUT NON-NULL TREE");
		error_reporter_tree_write(TREE);
		error_reporter_tree_free(TREE);
		TREE = NULL;
	}
#endif

	if(TREE == NULL){
		CONSOLE_DEBUG("CREATING ROOT");
		/* we're creating the root */
		tnew->parent = NULL;
		TREE = tnew;
		TREECURRENT = tnew;
		CONSOLE_DEBUG("TREECURRENT = %p",TREECURRENT);
	}else{
		asc_assert(TREECURRENT != NULL);
		CONSOLE_DEBUG("CREATING SUBTREE");
		if(TREECURRENT->head == NULL){
			/* if the current tree has no elements, add it as the head */
			TREECURRENT->head = tnew;
		}else{
			/* link the new tree to the last in the child list */
			TREECURRENT->tail->next = tnew;
		}
		/* update the tail of the list */
		TREECURRENT->tail = tnew;

		/* now switch the context to the sub-tree */
		tnew->parent = TREECURRENT;
		CONSOLE_DEBUG("SET TREECURRENT TO %p",TREECURRENT);
		TREECURRENT = tnew;
	}
	return 0;
}

int error_reporter_tree_end(){
	CONSOLE_DEBUG("TREE END");
	if(!TREECURRENT){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"'end' without TREECURRENT set");
		return 1;
	}
	TREECURRENT = TREECURRENT->parent;
	CONSOLE_DEBUG("SET TREECURRENT TO %p",TREECURRENT);
	return 0;
}	

static void error_reporter_tree_free(error_reporter_tree_t *t){
	if(t->head){
		error_reporter_tree_free(t->head);
	}
	if(t->next){
		error_reporter_tree_free(t->next);
	}
	if(t->err)ASC_FREE(t->err);
	ASC_FREE(t);
}

void error_reporter_tree_clear(){
	/* recursively free anything beneath the TREECURRENT node, return to the parent context */
	error_reporter_tree_t *t;
	if(!TREECURRENT){
		/* CONSOLE_DEBUG("NOTHING TO CLEAR!"); */
		return;
	}
	if(TREECURRENT->parent){
		t = TREECURRENT->parent;
	}else{
		TREE = NULL;
		t = NULL;
	}
	error_reporter_tree_free(TREECURRENT);
	TREECURRENT = t;
}

static int error_reporter_tree_match_sev(error_reporter_tree_t *t, unsigned match){
	if(t->err && (t->err->sev & match)){
		CONSOLE_DEBUG("SEVERITY MATCH FOR t = %p",t);
		return 1;
	}
	if(t->next && error_reporter_tree_match_sev(t->next, match)){
		CONSOLE_DEBUG("SEVERITY MATCH IN 'next' FOR t = %p",t);
		return 1;
	}
	if(t->head && error_reporter_tree_match_sev(t->head, match)){
		CONSOLE_DEBUG("SEVERITTY MATCH IN 'head' FOR t = %p",t);
		return 1;
	}
	CONSOLE_DEBUG("NO MATCH FOR t = %p",t);
	return 0;
}

/**
	traverse the tree, looking for ASC_PROG_ERR, ASC_USER_ERROR, or ASC_PROG_FATAL
	@return 1 if errors found
*/
int error_reporter_tree_has_error(){
	int res;
	if(TREECURRENT){
		res = error_reporter_tree_match_sev(TREECURRENT,ASC_ERR_ERR);
		if(res)CONSOLE_DEBUG("ERROR(S) FOUND IN TREECURRENT %p",TREECURRENT);
		return res;
	}else{
		CONSOLE_DEBUG("NO TREE FOUND");
		return 0;
	}
}

static int error_reporter_tree_write(error_reporter_tree_t *t){
	int res = 0;
	static int writecount = 0;

	if(++writecount > 30){
		CONSOLE_DEBUG("TOO MUCH WRITING");
		return 0;
	}

	if(t->err){
		res += error_reporter(t->err->sev, t->err->filename, t->err->line, t->err->func, t->err->msg);
	}else{
		/* CONSOLE_DEBUG("TREE HAS NO TOP-LEVEL ERROR"); */
	}

	if(t->head){
		res += error_reporter_tree_write(t->head);
	}
	if(t->next){
		res += error_reporter_tree_write(t->next);
	}
	return res;
}

#else /* ERROR_REPORTER_TREE_ACTIVE */
int error_reporter_tree_start(){
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Error reporter 'tree' turned off at compile time");
	return 0;
}
int error_reporter_tree_end(){return 0;}
void error_reporter_tree_clear(){}
int error_reporter_tree_has_error(){
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Attempt to check 'tree_has_error' when 'tree' turned off at compile time");
	return 0;
}
#endif /* ERROR_REPORTER_TREE_ACTIVE */

/*--------------------------
  REALLY WHERE THE REPORTING HAPPENS
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
	int res;

#ifdef ERROR_REPORTER_TREE_ACTIVE
	error_reporter_tree_t *t;
	if(sev != ASC_PROG_FATAL){
		if(TREECURRENT){
			/* add the error to the tree, don't output anything now */
			t = error_reporter_tree_new();
			t->err = error_reporter_meta_new();
			res = vsnprintf(t->err->msg,ERROR_REPORTER_MAX_MSG,fmt,args);
			t->err->filename = errfile;
			t->err->func = errfunc;
			t->err->line = errline;
			t->err->sev = sev;
			if(!TREECURRENT->head){
				TREECURRENT->head = TREECURRENT->tail = t;
			}else{
				TREECURRENT->tail->next = t;
				TREECURRENT->tail = t;
			}
			/* CONSOLE_DEBUG("Message (%d chars) added to tree",res); */
			return res;
		}else if(TREE){
			/* flush the tree before outputting current message */
			CONSOLE_DEBUG("WRITING OUT TREE CONTENTS");
			t = TREE;
			TREE = NULL;
			error_reporter_tree_write(t);
			CONSOLE_DEBUG("DONE WRITING TREE");
			TREECURRENT = t;
			error_reporter_tree_clear();
			CONSOLE_DEBUG("DONE FREEING TREE");
			CONSOLE_DEBUG("TREE = %p",TREE);
			CONSOLE_DEBUG("TREECURRENT = %p",TREECURRENT);
		}
	}
#endif

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
	if(g_error_reporter_cache.iscaching){
		error_reporter(
			g_error_reporter_cache.sev
			,g_error_reporter_cache.filename
			,g_error_reporter_cache.line
			,g_error_reporter_cache.func
			,g_error_reporter_cache.msg
		);
	}else{
		/* CONSOLE_DEBUG("IGNORING REPEATED CALL TO error_reporter_end_flush()"); */
	}
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
ASC_DLLSPEC(int) error_reporter_here(const error_severity_t sev, const char *fmt,...){
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
