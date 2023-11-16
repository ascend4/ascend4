#include <string.h>

#include "error.h"

#define ERROR_REPORTER_TREE_ACTIVE

#ifdef ERROR_REPORTER_TREE_ACTIVE
# include <ascend/general/ascMalloc.h>
# include <ascend/general/panic.h>
#endif

//#define ERROR_REPORTER_DEBUG
#ifdef ERROR_REPORTER_DEBUG
# define MSG CONSOLE_DEBUG
# define ERRMSG CONSOLE_DEBUG
#else
# define MSG(...) 
# define ERRMSG CONSOLE_DEBUG
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
	Default error reporter. To use this error reporter, set
	the callback pointer to NULL.
*/
int error_reporter_default_callback(ERROR_REPORTER_CALLBACK_ARGS){
	char *sevmsg="";
	enum ConsoleColor color = 0;
	char *endtxt="\n";
	int res=0;
	switch(sev){
		case ASC_PROG_FATAL:    color=ASC_FG_BRIGHTRED; sevmsg = "PROGRAM FATAL ERROR: "; break;
		case ASC_PROG_ERROR:
		    color=ASC_FG_RED; sevmsg = "PROGRAM ERROR: ";
			break;
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
	res += ASC_FPRINTF(ASCERR,"%s",endtxt);

	return res;
}

/*---------------------------------------------------------------------------
  ERROR REPORTER TREE (BRANCHABLE ERROR STACK) FUNCTIONALITY
*/

#ifdef ERROR_REPORTER_TREE_ACTIVE


/* TREE will be a pointer to the start of the top layer of the tree, or NULL */
static error_reporter_tree_t *g_error_reporter_tree = NULL;
# define TREECURRENT g_error_reporter_tree_current

/* TREECURRENT will be a pointer to the most recently added entry, or NULL if empty (in which case TREE will also be null */
static error_reporter_tree_t *g_error_reporter_tree_current = NULL;
# define TREE g_error_reporter_tree

/* output the cached errors in their nested order */
static int error_reporter_tree_write(error_reporter_tree_t *t);
static void error_reporter_tree_free(error_reporter_tree_t *t);

/* create a new empty node for the tree */
static error_reporter_tree_t *error_reporter_tree_new(){
	error_reporter_tree_t *tnew = ASC_NEW(error_reporter_tree_t);
	tnew->parent = NULL;
	tnew->next = NULL;
	tnew->head = NULL;
	tnew->tail = NULL;
	tnew->err = NULL;
	MSG("new node at %p",tnew);
	return tnew;
}

// initialise a new sub-tree: it should be added after TREECURRENT->tail
int error_reporter_tree_start(){
	MSG("tree start...");
	if(TREE==NULL){
		if(TREECURRENT!=NULL){
			ERRMSG("called with non-null TREECURRENT but null TREE!");
			return 1;
		}
		MSG("whole new tree");
		TREE = error_reporter_tree_new();
		TREECURRENT = TREE;
	}else{
		MSG("new node");
		// tree already exists, add to tail of TREECURRENT
		if(TREECURRENT == NULL){
			ERRMSG("unexpected NULL TREECURRENT with TREE non-null");
			return 2;
		}
		error_reporter_tree_t *t = error_reporter_tree_new();
		t->parent = TREECURRENT;
		if(TREECURRENT->tail){
			TREECURRENT->tail->next = t;
		}else{
			TREECURRENT->head = t;
		}
		TREECURRENT->tail = t;
		TREECURRENT = t;
	}
	return 0;
}

// end and move up. TREECURRENT will be set to TREECURRENT->parent. leave 'closed' tree in place.
int error_reporter_tree_end(){
	if(TREECURRENT ==  NULL){
	// if(TREECURRENT == TREE){
		ERRMSG("attempting to end at parent node, not allowed");
		return 1;
	}
	if(TREECURRENT->parent == NULL){
		MSG("tree end; writing output");
		error_reporter_tree_write(TREECURRENT); // FIXME this is problematic. what if we are inside another tree?
		error_reporter_tree_free(TREECURRENT);
		TREECURRENT = NULL;
		TREE = NULL;
	}else{
		MSG("tree end; moving up");
		TREECURRENT= TREECURRENT->parent;
	}
	return 0;
}

// traverse and free all nodes, either 'next' or below.
static void error_reporter_tree_free(error_reporter_tree_t *t){
	MSG("tree free %p",t);
	if(!t){
		MSG("NULL tree");
		return;
	}
	if(t->err){
		MSG("freeing error at %p",t->err);
		ASC_FREE(t->err);
	}else if(t->head){
		MSG("freeing head at %p",t->head);
		error_reporter_tree_t *node = t->head;
		while(node){
			MSG("freeing node at %p",node);
			error_reporter_tree_t *next = node->next;
			error_reporter_tree_free(node);
			node = next;
		}
	}else{
		MSG("node at %p contains neither err nor head",t);
	}
	MSG("ok, freeing tree at %p",t);
	ASC_FREE(t);
}

//	free and discard all nodes at current level (TREECURRENT->parent->head), move up to parent.
void error_reporter_tree_clear(){
	MSG("tree clear");
#ifdef ERROR_REPORTER_DEBUG
	error_reporter_tree_dump(stderr);
#endif
	if(!TREE){
		MSG("null TREE");
		return;
	}
	if(!TREECURRENT){
		ERRMSG("null TREECURRENT");
		return;
	}
	error_reporter_tree_t *newcurrent = TREECURRENT->parent;
	if(newcurrent){
		// there is a parent tree. disconnect TREECURRENT from it:
		if(newcurrent->head == TREECURRENT){
			newcurrent->head = NULL;
		}
		if(newcurrent->tail == TREECURRENT){
			newcurrent->tail = NULL;
		}
	}
	error_reporter_tree_free(TREECURRENT);
	TREECURRENT = newcurrent;
	if(TREECURRENT == NULL){
		// in which case, there should be no remaining memory allocated
		TREE = NULL;
	}
	MSG("TREECURRENT = %p",TREECURRENT);
}


void error_reporter_tree_destroy_all(){
	TREECURRENT = TREE;
	if(TREE)error_reporter_tree_clear();
}


/**
	For this function, we need to look at messages 'inside' TREECURRENT.
*/
static int error_reporter_tree_match_sev(error_reporter_tree_t *t, unsigned match){
	// if it is a leaf:
	if(t->err && (t->err->sev & match)){
		//MSG("LEAF MATCH FOR t = %p",t);
		return 1;
	}else if(t->head){
		// if it is a node
		error_reporter_tree_t *node;
		node = t->head;
		//MSG("CHECKING NODE %p",t);
		while(node){
			if(error_reporter_tree_match_sev(node,match)){
				return 1;
			}
			node = node->next;
		}
	}
	//MSG("NO MATCH FOR t = %p",t);
	return 0;
}


int error_reporter_tree_has_error(){
	int res;
	if(TREECURRENT){
		res = error_reporter_tree_match_sev(TREECURRENT,ASC_ERR_ERR);
		if(res){
			MSG("MATCHING ERROR(S) FOUND IN TREECURRENT %p",TREECURRENT);
		}
		return res;
	}else{
		MSG("NO TREE FOUND");
		return 0;
	}
}

static int error_reporter_tree_write(error_reporter_tree_t *t){
	int res = 0;

	if(t->err){
		MSG("writing leaf %p, err %p",t, t->err);
		error_reporter_tree_t *current = TREECURRENT;
		// suppress the tree for a sec
		TREECURRENT = NULL;
		error_reporter(t->err->sev, t->err->filename, t->err->line, t->err->func, t->err->msg);
		TREECURRENT = current;
	}else if(t->head){
		MSG("writing branch at %p",t);
		error_reporter_tree_t *node = t->head;
		while(node){
			res += error_reporter_tree_write(node);
			node = node->next;
		}
	}else{
		MSG("invalid node");
	}
	return res;
}

static void error_reporter_tree_dump_impl(FILE *file,error_reporter_tree_t *t, int level){
	if(t->err){
		if(t->err->msg){
			fprintf(file,"%.*s- %s at %p\n",2*level," ",t->err->msg,t);
		}else{
			fprintf(file,"%.*s- %s at %p\n",2*level," ","[empty message]",t);
		}
	}else if(t->head){
		fprintf(file,"%.*s+ %s at %p\n",2*level," ",(t==TREE)?"TREE":"[branch]",t);
		error_reporter_tree_t *node = t->head;
		while(node){
			error_reporter_tree_dump_impl(file,node,level+1);
			node = node->next;
		}
	}else{
		fprintf(file,"%.*s* %s\n",2*level," ","[empty]");
	}
}

void error_reporter_tree_dump(FILE *file){
	if(!TREE){
		fprintf(stderr,"TREE: NULL\n");
		return;
	}
	error_reporter_tree_dump_impl(file,TREE,0);
}

#else /* ERROR_REPORTER_TREE_ACTIVE */
int error_reporter_tree_start(){
	ERRMSG("Error reporter 'tree' turned off at compile time");
	return 0;
}
int error_reporter_tree_end(){return 0;}
void error_reporter_tree_clear(){}
int error_reporter_tree_has_error(){
	E"Attempt to check 'tree_has_error' when 'tree' turned off at compile time");
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
	int res;

#ifdef ERROR_REPORTER_TREE_ACTIVE
	error_reporter_tree_t *t;
	if(sev != ASC_PROG_FATAL){
		if(TREECURRENT){
			/* add the error to the tree, don't output anything now */
			t = error_reporter_tree_new();
			t->err = error_reporter_meta_new();
			res = vsnprintf(t->err->msg,ERROR_REPORTER_MAX_MSG,fmt,args);
			MSG("store error '%s' at %p",t->err->msg,t->err);
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
		}
#if 0
		// this should never happen
		else if(TREE){
			/* flush the tree before outputting current message */
			/* CONSOLE_DEBUG("WRITING OUT TREE CONTENTS"); */
			t = TREE;
			TREE = NULL;
			error_reporter_tree_write(t);
			//CONSOLE_DEBUG("DONE WRITING TREE");
			TREECURRENT = t;
			error_reporter_tree_clear();
			/* CONSOLE_DEBUG("DONE FREEING TREE");
			CONSOLE_DEBUG("TREE = %p",TREE);
			CONSOLE_DEBUG("TREECURRENT = %p",TREECURRENT); */
		}
#endif
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
				SNPRINTF(msg+ERROR_REPORTER_MAX_MSG-16+1,16,"... (truncated)");
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
ASC_DLLSPEC int error_reporter_here(const error_severity_t sev, const char *fmt,...){
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
