#include <string.h>

#include "error.h"

#define ERROR_REPORTER_TREE_ACTIVE

#ifdef ERROR_REPORTER_TREE_ACTIVE
# include <ascend/general/ascMalloc.h>
# include <ascend/general/panic.h>
#endif

//#define ERROR_DEBUG
#ifdef ERROR_DEBUG
# define MSG CONSOLE_DEBUG
# define TREE_PRINT error_reporter_tree_print
#else
# define MSG(ARGS...) ((void)0)
# define TREE_PRINT(ARGS...) ((void)0)
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
	if(filename!=NULL){
		//MSG("filename = '%s'",filename);
		res += ASC_FPRINTF(ASCERR,"%s:",filename);
	}
	if(line!=0)res += ASC_FPRINTF(ASCERR,"%d:",line);
	if(funcname!=NULL){
		//MSG("funcname = '%s'",funcname);
		res += ASC_FPRINTF(ASCERR,"%s:",funcname);
	}else{
		//MSG("funcname NULL");
	}
	if ((filename!=NULL) || (line!=0) || (funcname!=NULL))res += ASC_FPRINTF(ASCERR," ");
	res += ASC_VFPRINTF(ASCERR,fmt,*args);
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

#ifdef ERROR_DEBUG
static void error_reporter_tree_print(error_reporter_tree_t *t1);
#endif

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
	tnew->prev = NULL;
	tnew->head = NULL;
	tnew->tail = NULL;
	tnew->err = NULL;
	tnew->parent = NULL;
	TREE_PRINT(tnew);
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

#ifdef ERROR_DEBUG
static void error_reporter_tree_print1(error_reporter_tree_t *t,int level){
	if(t->head){
		assert(t->err == NULL);
		MSG("%*s+%p%s (head=%p,tail=%p)",2+2*level,"",t,t==CURRENT?" (CURRENT)":"",t->head,t->tail);
		error_reporter_tree_print1(t->head,level+1);
	}else if(t->err){
		assert(t->err != NULL);
		MSG("%*s-%p %s:%d: %s (parent=%p,next=%p)",2+2*level,"",t,t->err->filename, t->err->line, t->err->msg,t->parent,t->next);
	}else{
		MSG("%*s-%p EMPTY NODE",2+2*level,"",t);
	}

	if(t->next){
		error_reporter_tree_print1(t->next,level);
	}
}

static void error_reporter_tree_print(error_reporter_tree_t *t1){
	if(!t1){
		MSG("null tree");
	}else{
		MSG("finding top of tree, starting at %p",t1);
		while(t1->parent){
			t1 = t1->parent;
		}
		MSG("top of tree is %p",t1);
		error_reporter_tree_print1(t1,0);
	}
}
#endif

error_reporter_tree_t *error_reporter_tree_start(int iscaching){
	error_reporter_tree_t *tnew = error_reporter_tree_new(iscaching);
	if(CURRENT == NULL){
		MSG("creating tree (caching=%d)",iscaching);
		CURRENT = tnew;
	}else{
		MSG("creating sub-tree (caching=%d)",iscaching);
		if(CURRENT->head == NULL){
			MSG("adding at head");
			CURRENT->head = tnew;
		}else{
			MSG("adding at tail");
			/* link the new tree to the last in the child list */
			CURRENT->tail->next = tnew;
			tnew->prev = CURRENT->tail;
		}
		/* update the tail of the list */
		CURRENT->tail = tnew;
		/* now switch the context to the sub-tree */
		tnew->parent = CURRENT;
		CURRENT = tnew;
	}
	return CURRENT;
}

int error_reporter_tree_end(error_reporter_tree_t *tree){
	assert(CURRENT);
	assert(tree==CURRENT);
	if(CURRENT->iscaching){
		if(error_reporter_tree_has_caching_parent(CURRENT)){
			MSG("no output; caching parent");
		}else{
			MSG("outputting now, no caching parent");
			error_reporter_tree_write(CURRENT);
		}
	}
	if(CURRENT->parent){
		MSG("ending sub-tree, NOT freeing structures");
		CURRENT = CURRENT->parent;
	}else{
		MSG("ending top tree, freeing structures");
		TREE_PRINT(CURRENT);
		error_reporter_tree_free(CURRENT);
		CURRENT = NULL;
	}
	return 0;
}

/** recursive routine to deallocate error_reporter_tree_t structures. */
static void error_reporter_tree_free(error_reporter_tree_t *t){
	assert(t);
	error_reporter_tree_t *n=NULL;
	if(t->head){
		assert(t->err == NULL);
		MSG("freeing sub-nodes");
		error_reporter_tree_free(t->head);
		MSG("done freeing sub-nodes");
	}
	if(t->err){
		MSG("freeing error metadata ('%s')",t->err->msg);
		assert(t->head == NULL);
		assert(t->tail == NULL);
		ASC_FREE(t->err);
	}
	if(t->next)n = t->next;

	MSG("freeing node %p%s",t,t==CURRENT?" (CURRENT)":"");
	ASC_FREE(t);

	if(n){
		MSG("freeing next node %p",n);
		error_reporter_tree_free(n);
	}
}

/** clear all errors nested within the current tree -- they won't be visible
to any parent trees after this; they never happened. Must be called INSTEAD OF 
error_reporter_tree_end(). */
void error_reporter_tree_end_clear(error_reporter_tree_t *tree){
	assert(CURRENT);
	assert(tree==CURRENT);
	error_reporter_tree_t *t1 = NULL, *tp, *tn;
	if(CURRENT->parent){
		MSG("ending/clearing sub-tree %p",CURRENT);
		TREE_PRINT(CURRENT);
		t1 = CURRENT->parent;
		tn = CURRENT->next;
		tp = CURRENT->prev;
		assert(tn==NULL); /* actually, we can't imagine a case where tn != NULL */
		if(tn){
			MSG("%p->prev = %p",tn,tp);
			tn->prev = tp;
		}
		if(tp){
			MSG("%p->next = %p",tp,tn);
			tp->next = tn;
		}
		if(t1->head == CURRENT){
			MSG("fixing head");
			if(tp)t1->head = tp;
			else if(tn)t1->head = tn;
			else t1->head = NULL;
		}
		if(t1->tail == CURRENT){
			MSG("fixing tail");
			if(tp)t1->tail = tp;
			else t1->tail = NULL;
		}
		MSG("after pruning");
		TREE_PRINT(t1);
	}else{
		MSG("ending/clearing top tree");
	}
	error_reporter_tree_free(CURRENT);
	CURRENT = t1;
	MSG("completed tree_end_clear, new current:");
	TREE_PRINT(CURRENT);
}

static int error_reporter_tree_match_sev(error_reporter_tree_t *t, unsigned match){
	assert(t);
	if(t->err && (t->err->sev & match)){
		/* MSG("SEVERITY MATCH FOR t = %p",t); */
		return 1;
	}
	if(t->next && error_reporter_tree_match_sev(t->next, match)){
		/* MSG("SEVERITY MATCH IN 'next' FOR t = %p",t); */
		return 1;
	}
	if(t->head && error_reporter_tree_match_sev(t->head, match)){
		/* MSG("SEVERITTY MATCH IN 'head' FOR t = %p",t); */
		return 1;
	}
	/* MSG("NO MATCH FOR t = %p",t); */
	return 0;
}

/**
	traverse the tree, looking for ASC_PROG_ERR, ASC_USER_ERROR, or ASC_PROG_FATAL
	@return 1 if errors found
*/
int error_reporter_tree_has_error(error_reporter_tree_t *tree){
	int res;
	assert(CURRENT);
	assert(tree==CURRENT);
	res = error_reporter_tree_match_sev(CURRENT,ASC_ERR_ERR);
	if(res){
		MSG("ERROR(S) FOUND IN CURRENT %p",CURRENT);
	}
	return res;
}

static int error_reporter_tree_write(error_reporter_tree_t *t){
	int res = 0;
	assert(t != NULL);

	if(t->err){
		// an a simple node -- just an error
		assert(t->head == NULL);
		assert(t->tail == NULL);
		MSG("WRITING MSG FROM CACHE");
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
error_reporter_tree_t *error_reporter_tree_start(int iscaching){
	(void)iscaching;
	ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Error reporter 'tree' turned off at compile time");
	return NULL;
}
int error_reporter_tree_end(error_reporter_tree_t *tree){
	assert(tree==NULL);
	return 0;
}
void error_reporter_tree_end_clear(error_reporter_tree_t *tree){
	assert(tree==NULL);	
}
int error_reporter_tree_has_error(error_reporter_tree_t *tree){
	assert(tree==NULL);
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
    , va_list *args
){
	int res = 0;
	va_list args2;

	error_reporter_callback_t cb = g_error_reporter_callback;
	if(cb == NULL){
		//MSG("using default error reporter callback, errfunc = '%s'",errfunc);
		cb = error_reporter_default_callback;
	}

#ifdef ERROR_REPORTER_TREE_ACTIVE
	error_reporter_tree_t *t;
	if(sev != ASC_PROG_FATAL){
		if(CURRENT){
			va_copy(args2,*args);

			MSG("adding to tree");
			/* add the error to the tree, don't output anything now */
			t = error_reporter_tree_new(0);
			t->err = error_reporter_meta_new();
			vsnprintf(t->err->msg,ERROR_REPORTER_MAX_MSG,fmt,*args);
			MSG("t->err->msg = \"%s\"",t->err->msg);
			t->err->filename = errfile;
			t->err->func = errfunc;
			t->err->line = errline;
			t->err->sev = sev;
			if(!CURRENT->head){
				// current tree is empty
				CURRENT->head = CURRENT->tail = t;
				t->parent = CURRENT;
			}else{
				CURRENT->tail->next = t;
				t->prev = CURRENT->tail;
				CURRENT->tail = t;
				t->parent = CURRENT;
			}
			MSG("message '%s' added to tree",t->err->msg);
			//MSG("message has errfunc '%s'",t->err->func);

			if(CURRENT->iscaching || error_reporter_tree_has_caching_parent(CURRENT)){
				MSG("caching; no output");
				return res;
			}else{
				MSG("non-caching; outputting again directly");
				res = (*cb)(sev,errfile,errline,errfunc,fmt,&args2);
				MSG("res = %d",res);
				return res;
			}
		}else{
			MSG("no error tree; direct output");
			return (*cb)(sev,errfile,errline,errfunc,fmt,args);
		}
	}
#endif

	MSG("fatal error message format = '%s'",fmt);
	return (*cb)(sev,errfile,errline,errfunc,fmt,args);
}

/*----------------------------
  DROP-IN replacements for stdio.h / ascPrint.h
*/

int vfprintf_error_reporter(FILE *file, const char *fmt, va_list *args){
	char *msg;
	int len;
	int res;
	if(file==stderr){
		if(g_error_reporter_cache.iscaching){
			msg = g_error_reporter_cache.msg;
			len = strlen(msg);
			res = vsnprintf(msg+len,ERROR_REPORTER_MAX_MSG-len,fmt,*args);
			//MSG("Appended \"%s\" to message",msg+len);
			if(len+res+1>=ERROR_REPORTER_MAX_MSG){
				SNPRINTF(msg+ERROR_REPORTER_MAX_MSG-16,16,"... (truncated)");
				ASC_FPRINTF(stderr,"TRUNCATED MESSAGE, FULL MESSAGE FOLLOWS:\n----------START----------\n");
				ASC_VFPRINTF(stderr,fmt,*args);
				ASC_FPRINTF(stderr,"\n-----------END----------\n");
			}
		}else{
			MSG("Reporting msg directly, fmt = \"%s\"",fmt);
			/* Not caching: output all in one go as a ASC_PROG_NOTE */
			res = va_error_reporter(ASC_PROG_NOTE,NULL,0,NULL,fmt,args);
		}
	}else{
		MSG("Printing directly, fmt = \"%s\"",fmt);
		res = ASC_VFPRINTF(file,fmt,*args);
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
	res = vfprintf_error_reporter(file,fmt,&args);
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
		MSG("call to error_reporter_start before expected error_reporter_end_flush");
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
		/* MSG("IGNORING REPEATED CALL TO error_reporter_end_flush()"); */
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
	res = va_error_reporter(sev,errfile,errline,errfunc,fmt,&args);
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
	res = va_error_reporter(ASC_PROG_NOTE,"unknown-file",0,NULL,fmt,&args);
	va_end(args);

	return res;
}

/** error reporter 'here' for compilers not supporting variadic macros */
ASC_DLLSPEC int error_reporter_here(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,"unknown-file",0,NULL,fmt,&args);
	va_end(args);

	return res;
}

/** error reporter for compilers not supporting variadic macros */
int error_reporter_noline(const error_severity_t sev, const char *fmt,...){
	int res;
	va_list args;

	va_start(args,fmt);
	res = va_error_reporter(sev,NULL,0,NULL,fmt,&args);
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
