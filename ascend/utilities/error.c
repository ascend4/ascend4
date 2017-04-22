#include <string.h>

#include "error.h"

#define ERROR_REPORTER_TREE_ACTIVE

#ifdef ERROR_REPORTER_TREE_ACTIVE
# include <ascend/general/ascMalloc.h>
# include <ascend/general/panic.h>
#endif

/**
	Global variable which stores the pointer to the callback
	function being used.
*/
static error_reporter_callback_t g_error_reporter_callback;

/**
	Global variable which enables caching of *individual* error messages,
	as used with error_reporter_start() and error_reporter_end_flush().
	This is not the same as the caching performed by error_reporter_tree_start()
	which holds/records multiple error messages depending on the iscaching
	setting.
*/
static error_reporter_meta_t g_error_reporter_cache;

/**
	Default error reporter. This error reporter is used whenever the callback
	pointer is NULL, but can be replaced with another callback (eg for GUI
	reporting) using error_reporter_set_callback()
*/
int error_reporter_default_callback(ERROR_REPORTER_CALLBACK_ARGS){
	char *sevmsg="";
	enum ConsoleColor color = 0;
	char *endtxt="\n";
	int res=0;
	switch(sev){
		case ASC_PROG_FATAL:    color=ASC_FG_BRIGHTRED; sevmsg = "PROGRAM FATAL ERROR: "; break;
		case ASC_PROG_ERROR:    color=ASC_FG_RED; sevmsg = "PROGRAM ERROR: "; break;
		case ASC_PROG_WARNING:  color=ASC_FG_BROWN;sevmsg = "PROGRAM WARNING: "; break;
		case ASC_PROG_NOTE:     color=ASC_FG_BRIGHTGREEN; endtxt=""; break; /* default, keep unembellished for now */
		case ASC_USER_ERROR:    color=ASC_FG_BRIGHTRED; sevmsg = "ERROR: "; break;
		case ASC_USER_WARNING:  color=ASC_FG_BROWN; sevmsg = "WARNING: "; break;
		case ASC_USER_NOTE:     sevmsg = "NOTE: "; break;
		case ASC_USER_SUCCESS:  color=ASC_FG_BRIGHTGREEN; sevmsg = "SUCCESS: "; break;
	}
	color_on(ASCERR,color);
	res = ASC_FPRINTF(ASCERR,"%s",sevmsg);
	color_off(ASCERR);
	if(filename!=NULL)res += ASC_FPRINTF(ASCERR,"%s:",filename);
	if(line!=0)res += ASC_FPRINTF(ASCERR,"%d:",line);
	if(funcname!=NULL)res += ASC_FPRINTF(ASCERR,"%s:",funcname);
	if ((filename!=NULL) || (line!=0) || (funcname!=NULL))res += ASC_FPRINTF(ASCERR," ");
	res += ASC_VFPRINTF(ASCERR,fmt,args);
	res += ASC_FPRINTF(ASCERR,"%s",endtxt);
	return res;
}

/*---------------------------------------------------------------------------
  ERROR REPORTER TREE (BRANCHABLE ERROR STACK) FUNCTIONALITY
*/

#ifdef ERROR_REPORTER_TREE_ACTIVE

static error_reporter_tree_t *g_error_reporter_tree_current = NULL;

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

# define CURRENT g_error_reporter_tree_current

static int error_reporter_tree_write(error_reporter_tree_t *t);
static void error_reporter_tree_free(error_reporter_tree_t *t);

/// deprecated function for testing purpose only
error_reporter_tree_t *error_reporter_get_tree_current(){
	return CURRENT;
}

static error_reporter_tree_t *error_reporter_tree_new(int iscaching){
	error_reporter_tree_t *tnew = ASC_NEW(error_reporter_tree_t);
	tnew->iscaching = iscaching;
	tnew->next = NULL;
	tnew->head = NULL;
	tnew->tail = NULL;
	tnew->err = NULL;
	tnew->parent = NULL;
	return tnew;
}

static int error_reporter_tree_has_caching_parent(error_reporter_tree_t *t){
	assert(t);
	if(NULL == t->parent)return 0;
	error_reporter_tree_t *t1 = t->parent;
	int iscaching = 0;
	while(t1){
		iscaching = iscaching || t1->iscaching;
		assert(t1->parent != t1);
		t1 = t1->parent;
	}
	return iscaching;
}

int error_reporter_tree_start(int iscaching){
	error_reporter_tree_t *tnew = error_reporter_tree_new(iscaching);
	if(CURRENT == NULL){
		CONSOLE_DEBUG("creating tree (caching=%d)",iscaching);
		CURRENT = tnew;
	}else{
		CONSOLE_DEBUG("creating sub-tree (caching=%d)",iscaching);
		if(CURRENT->head == NULL){
			/* if the current tree has no elements, add it as the head */
			CURRENT->head = tnew;
		}else{
			/* link the new tree to the last in the child list */
			CURRENT->tail->next = tnew;
		}
		/* update the tail of the list */
		CURRENT->tail = tnew;
		/* now switch the context to the sub-tree */
		tnew->parent = CURRENT;
		CURRENT = tnew;
	}
	return 0;
}

int error_reporter_tree_end(){
	assert(CURRENT);
	if(CURRENT->iscaching){
		if(error_reporter_tree_has_caching_parent(CURRENT)){
			CONSOLE_DEBUG("no output; caching parent");
		}else{
			CONSOLE_DEBUG("outputting now, no caching parent");
			error_reporter_tree_write(CURRENT);
		}
	}
	if(CURRENT->parent){
		CONSOLE_DEBUG("ending sub-tree");
		CURRENT = CURRENT->parent;
	}else{
		CONSOLE_DEBUG("ending top tree");
		CURRENT = NULL;
	}
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
	/* recursively free anything beneath the CURRENT node, return to the parent context */
	error_reporter_tree_t *t1;
	if(!CURRENT){
		/* CONSOLE_DEBUG("NOTHING TO CLEAR!"); */
		return;
	}
	if(CURRENT->parent){
		t1 = CURRENT->parent;
	}else{
		t1 = NULL;
	}
	error_reporter_tree_free(CURRENT);
	CURRENT = t1;
}

static int error_reporter_tree_match_sev(error_reporter_tree_t *t, unsigned match){
	if(t->err && (t->err->sev & match)){
		/* CONSOLE_DEBUG("SEVERITY MATCH FOR t = %p",t); */
		return 1;
	}
	if(t->next && error_reporter_tree_match_sev(t->next, match)){
		/* CONSOLE_DEBUG("SEVERITY MATCH IN 'next' FOR t = %p",t); */
		return 1;
	}
	if(t->head && error_reporter_tree_match_sev(t->head, match)){
		/* CONSOLE_DEBUG("SEVERITTY MATCH IN 'head' FOR t = %p",t); */
		return 1;
	}
	/* CONSOLE_DEBUG("NO MATCH FOR t = %p",t); */
	return 0;
}

/**
	traverse the tree, looking for ASC_PROG_ERR, ASC_USER_ERROR, or ASC_PROG_FATAL
	@return 1 if errors found
*/
int error_reporter_tree_has_error(){
	int res;
	if(CURRENT){
		res = error_reporter_tree_match_sev(CURRENT,ASC_ERR_ERR);
		if(res){
			/* CONSOLE_DEBUG("ERROR(S) FOUND IN CURRENT %p",CURRENT); */
		}
		return res;
	}else{
		CONSOLE_DEBUG("NO TREE FOUND");
		return 0;
	}
}

static int error_reporter_tree_write(error_reporter_tree_t *t){
	int res = 0;
	assert(t != NULL);
	if(t->err){
		// an a simple node -- just an error
		assert(t->head == NULL);
		assert(t->tail == NULL);
		error_reporter_callback_t cb = g_error_reporter_callback ?
			g_error_reporter_callback : error_reporter_default_callback;
		res += (*cb)(t->err->sev,t->err->filename,t->err->line,t->err->func,t->err->msg,NULL);
	}else{
		// a parent node -- traverse the sub-nodes
		assert(t->head);
		assert(t->tail);
		error_reporter_tree_t *t1 = t->head;
		while(t1){
			res += error_reporter_tree_write(t1);
			t1 = t1->next;
		}
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
	ERROR_REPORTER_HERE(ASC_PROG_FATAL,"Attempt to check 'tree_has_error' when 'tree' turned off at compile time");
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
    , va_list args
){
	int res = 0;

#ifdef ERROR_REPORTER_TREE_ACTIVE
	error_reporter_tree_t *t;
	if(sev != ASC_PROG_FATAL){
		if(CURRENT){
			/* add the error to the tree, don't output anything now */
			t = error_reporter_tree_new(0);
			t->err = error_reporter_meta_new();
			res = vsnprintf(t->err->msg,ERROR_REPORTER_MAX_MSG,fmt,args);
			t->err->filename = errfile;
			t->err->func = errfunc;
			t->err->line = errline;
			t->err->sev = sev;
			if(!CURRENT->head){
				CURRENT->head = CURRENT->tail = t;
			}else{
				CURRENT->tail->next = t;
				CURRENT->tail = t;
			}
			CONSOLE_DEBUG("message '%s' added to tree",t->err->msg);

			if(CURRENT->iscaching || error_reporter_tree_has_caching_parent(CURRENT)){
				CONSOLE_DEBUG("caching; no output");
				return res;
			}
		}
	}
#endif

	CONSOLE_DEBUG("outputting message (format) '%s'",fmt);

	error_reporter_callback_t cb = g_error_reporter_callback;
	if(cb == NULL)cb = error_reporter_default_callback;

	res += (*cb)(sev,errfile,errline,errfunc,fmt,args);

	return res;
}

/*----------------------------
  DROP-IN replacements for stdio.h / ascPrint.h
*/

int vfprintf_error_reporter(FILE *file, const char *fmt, va_list args){
	char *msg;
	int len;
	int res;
	if(file==stderr){
		if(g_error_reporter_cache.iscaching){
			msg = g_error_reporter_cache.msg;
			len = strlen(msg);
			res = vsnprintf(msg+len,ERROR_REPORTER_MAX_MSG-len,fmt,args);
			if(len+res+1>=ERROR_REPORTER_MAX_MSG){
				SNPRINTF(msg+ERROR_REPORTER_MAX_MSG-16,15,"... (truncated)");
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
	return res;
}

/**
	This function performs caching of the error text if the flag is set
*/
int
fprintf_error_reporter(FILE *file, const char *fmt, ...){
	va_list args;
	int res;

	va_start(args,fmt);
	res = vfprintf_error_reporter(file,fmt,args);
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

int error_reporter_start(const error_severity_t sev, const char *filename
	, const int line, const char *func
){
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

/**
	This function will flush the output of a multi-line error message
	(as written with multiple calls to FPRINTF(stderr,...))
*/
int error_reporter_end_flush(){
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
void error_reporter_set_callback(
		const error_reporter_callback_t new_callback
){
	g_error_reporter_callback = new_callback;
}

/*-------------------------
  OPTIONAL code for systems not supporting variadic macros.
  Your system probably does support variadic macros, it's just
  a question of checking what your particular syntax is, and possibly
  adding some stuff in error.h.
*/

#ifdef NO_VARIADIC_MACROS
/** error reporter for compilers not supporting variadic macros */
int error_reporter_note_no_line(const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(ASC_PROG_NOTE,"unknown-file",0,NULL,fmt,args);
	va_end(args);

	return res;
}

/** error reporter 'here' for compilers not supporting variadic macros */
ASC_DLLSPEC int error_reporter_here(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,"unknown-file",0,NULL,fmt,args);
	va_end(args);

	return res;
}

/** error reporter for compilers not supporting variadic macros */
int error_reporter_noline(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,NULL,0,NULL,fmt,args);
	va_end(args);

	return res;
}

/** console debugging for compilers not supporting variadic macros */
int console_debug(const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = Asc_VFPrintf(ASCERR,fmt,args);
	va_end(args);

	return res;
}
#endif /* NO_VARIADIC_MACROS */
