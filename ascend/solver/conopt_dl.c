/*	ASCEND modelling environment
	Copyright (C) 2006, 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	This file allows CONOPT to be dlopened at runtime.
*//*
	By John Pye
	Based on conopt.c by Vicente Rico Ramirez (created 05/97)
*/

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/general/env.h>
#include <string.h>
#include "conopt_dl.h"

#ifndef ASC_WITH_CONOPT
#ifdef __GNUC__
# warning "Shouldn't compile this file unless ASC_WITH_CONOPT set"
#endif
#else

#ifndef ASC_LINKED_CONOPT
# include <ctype.h>
# include <ascend/general/ascMalloc.h>
# include <ascend/utilities/ascDynaLoad.h>

//#define ASC_CONOPT_DEBUG
#ifdef ASC_CONOPT_DEBUG
# define MSG(...) CONSOLE_DEBUG(__VA_ARGS__)
#else
# define MSG(...) (void)0
#endif

/*------------------------------------------------------------------------------
  DLOPENING CONOPT SUPPORT FUNCTIONS
*/

# define INTINT (int*cntvect,int*v)
# define INTINT1 (cntvect,v)
# define INTDOUBLE (int*cntvect,double*v)
# define INTDOUBLE1 (cntvect,v)
# define SEMICOLON ;
# define SPACE

/*
	Typedefs for the various function pointers
*/
# define FN_TYPE_DECL(T,A,V,L) \
	typedef int COI_CALL (T##_fn_t) A

CONOPT_FNS(FN_TYPE_DECL,SEMICOLON);
# undef FN_TYPE_DECL

/*
	Define a struct to hold all the function pointers, then
	declare it as a global variable.
*/
# define FN_PTR_DECL(T,A,V,L) \
	T##_fn_t* T##_ptr

typedef struct{
    CONOPT_FNS(FN_PTR_DECL,SEMICOLON);
} conopt_fptrs_t;
# undef FN_PTR_DECL

conopt_fptrs_t conopt_fptrs;
# ifdef ASC_CONOPT_API4
typedef void COI_CALL (COIGET_Version_fn_t)(int *major, int *minor, int *patch);
static COIGET_Version_fn_t *COIGET_Version_ptr = NULL;
# endif
static int conopt_loaded = 0;
static char *conopt_libpath = NULL;


/*
	Declare local functions to hook into the DLL
*/
# define FN_PTR_EXEC(T,A,V,L) \
	int COI_CALL T A{ \
		if(conopt_fptrs.T##_ptr==NULL){ \
			return 1; \
		} \
		return (* conopt_fptrs.T##_ptr) V ; \
	}

CONOPT_FNS(FN_PTR_EXEC,SPACE)

# undef FN_PTR_EXEC

/**
	This function will load the DLL and resolve all the required symbols
*/
int asc_conopt_load(){
# ifdef ASC_LINKED_CONOPT
#  error "We don't use this if we've got linked CONOPT!"
# endif
	char *libpath;
	int status;
	char fnsymbol[400], *c;
	const char *libname=ASC_CONOPT_LIB;
	const char *envvar;
# ifdef ASC_CONOPT_API4
	(void)c;
# endif

	if(conopt_loaded) {
		return 0; /* already loaded */
	}

	/* MSG("LOADING CONOPT..."); */

	envvar  = ASC_CONOPT_ENVVAR;

	/* need to import this variable into the ascend 'environment' */
	if(-1!=env_import(ASC_CONOPT_ENVVAR,getenv,Asc_PutEnv,0)){
		MSG("Searching in path '%s' (from env var '%s')",getenv(envvar),envvar);
	}/*else{
		MSG("Default conopt search path: %s", ASC_CONOPT_DLPATH);
	}*/

	/** @TODO replace with a direct call to ospath and/or importhandler? */
	libpath = SearchArchiveLibraryPath(libname, ASC_CONOPT_DLPATH, envvar);

	if(libpath==NULL){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR
			, "Library '%s' could not be located (check value of env var '%s' and/or default path '%s')"
			, libname, envvar, ASC_CONOPT_DLPATH
		);
		return 1;
	}

	status = Asc_DynamicLoad(libpath, NULL);
	if (status != 0) {
		ASC_FREE(libpath);
		return 1; /* failed to load */
	}

# ifdef ASC_CONOPT_API4
#  define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	conopt_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(conopt_fptrs.T##_ptr==NULL)status+=1;
# else
#  if defined(FNAME_UCASE_NODECOR) || defined(FNAME_UCASE_DECOR) || defined(FNAME_UCASE_PREDECOR)
#  define FNCASE(C) C=toupper(C)
#  elif defined(FNAME_LCASE_NODECOR) || defined(FNAME_LCASE_DECOR)
#  define FNCASE(C) C=tolower(C)
#  else
#  error "CONOPT case rule not defined"
#  endif

#  if defined(FNAME_UCASE_DECOR) || defined(FNAME_LCASE_DECOR)
#  define FNDECOR(S,L) strcat(S,"_")
#  elif defined(FNAME_UCASE_PREDECOR) /* on windows, precede with _ and append @L (integer value of L) */
#  define FNDECOR(S,L) strcat(S,L);for(c=S+strlen(S)+1;c>S;--c){*c=*(c-1);} *S='_';
#  else
#  define FNDECOR(S,L) (void)0
#  endif

# define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	for(c=fnsymbol;*c!='\0';++c){ \
		FNCASE(*c); \
	} \
	FNDECOR(fnsymbol,L); \
	conopt_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(conopt_fptrs.T##_ptr==NULL)status+=1;
# endif

	CONOPT_FNS(FN_PTR_GET,SPACE)

# undef FN_PTR_GET
# ifndef ASC_CONOPT_API4
# undef FNDECOR
# undef FNCASE
# else
	COIGET_Version_ptr = (COIGET_Version_fn_t *)Asc_DynamicFunction(libpath,"COIGET_Version");
# endif

	if(status!=0){
		Asc_DynamicUnLoad(libpath);
		ASC_FREE(libpath);
		memset(&conopt_fptrs,0,sizeof(conopt_fptrs));
# ifdef ASC_CONOPT_API4
		COIGET_Version_ptr = NULL;
# endif
		return 1; /* failed to resolve all symbols */
	}

	conopt_libpath = libpath;
	conopt_loaded = 1;
	return 0;
}

int asc_conopt_unload(){
	int status = 0;

	if(!conopt_loaded){
		return 0;
	}

	memset(&conopt_fptrs,0,sizeof(conopt_fptrs));
# ifdef ASC_CONOPT_API4
	COIGET_Version_ptr = NULL;
# endif
	if(conopt_libpath != NULL){
		status = Asc_DynamicUnLoad(conopt_libpath);
		ASC_FREE(conopt_libpath);
		conopt_libpath = NULL;
	}
	conopt_loaded = 0;
	return status;
}

# ifdef ASC_CONOPT_API4
int asc_conopt_get_version(int *major, int *minor, int *patch){
	if(!conopt_loaded || COIGET_Version_ptr == NULL){
		return 1;
	}
	COIGET_Version_ptr(major,minor,patch);
	return 0;
}
# endif

#endif

/*-----------------------------------------------------------------------------
   std.c (modified from the version provided with CONOPT)

   This file has some 'standard' implementations for the mandatory
   callback routines Message, ErrMsg, Status, and Solution.
   The routines use global file pointers, so they are only intended
   as examples that can be used for further refinements.

	FIXME this code could probably be moved to asc_conopt.c?
*/

#define MAXLINE 133  /* maximum line length plus an extra character
                        for the null terminator                       */

#ifdef ASC_CONOPT_API4

int COI_CALL asc_conopt_progress( int LEN_INT, const int INT[]
		, int LEN_RL, const double RL[], const double X[], void* USRMEM
){
	(void)LEN_INT;
	(void)LEN_RL;
	(void)X;
	(void)USRMEM;
	MSG("Iteration %d, phase %d: %d infeasible, %d non-optimal; objective = %e"
		, INT[0], INT[1], INT[2], INT[3], RL[1]
	);
	return 0;
}

int COI_CALL asc_conopt_message( int SMSG, int DMSG, int NMSG
		, char* MSGV[], void* USRMEM
){
	int i;
	(void)DMSG;
	(void)USRMEM;
	for(i = 0; i < SMSG; ++i){
		MSG("%s", MSGV[i]);
	}
	for(i = 0; i < NMSG; ++i){
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"(CONOPT) %s", MSGV[i]);
	}
	return 0;
}

int COI_CALL asc_conopt_errmsg( int ROWNO, int COLNO, int POSNO
		, const char* MSG, void* USRMEM
){
	(void)POSNO;
	(void)USRMEM;
	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
	if ( ROWNO == -1 ) {
		FPRINTF(ASCERR,"Variable %d : ",COLNO);
	}else if ( COLNO == -1 ) {
		FPRINTF(ASCERR,"Equation %d : ",ROWNO);
	}else{
		FPRINTF(ASCERR,"Variable %d appearing in Equation %d : ",COLNO, ROWNO);
	}
	FPRINTF(ASCERR,"%s\n", MSG);
	error_reporter_end_flush();
	return 0;
}

int COI_CALL asc_conopt_status(int MODSTA, int SOLSTA
		, int ITER, double OBJVAL, void* USRMEM
){
	int *modsta = &MODSTA;
	int *solsta = &SOLSTA;
	int *iter = &ITER;
	double *objval = &OBJVAL;
	(void)iter;
	(void)objval;
	(void)USRMEM;

	MSG("CONOPT has finished Optimizing");
	MSG("Model status    = %8d", *modsta);
	MSG("Solver status   = %8d", *solsta);
	MSG("Iteration count = %8d", *iter);
	MSG("Objective value = %10f", *objval);

	const char *modstatxt;
	error_severity_t t = ASC_USER_SUCCESS;
	switch(*modsta){
		case 1: modstatxt = "optimal"; break;
		case 2: modstatxt = "locally optimal"; break;
		case 3: t = ASC_USER_ERROR; modstatxt = "unbounded"; break;
		case 4: t = ASC_USER_ERROR; modstatxt = "infeasible"; break;
		case 5: modstatxt = "locally infeasible"; break;
		case 6: modstatxt = "intermediate infeasible"; break;
		case 7: modstatxt = "intermediate non-optimal"; break;
		case 12: modstatxt = "unknown type of error"; break;
		case 13: modstatxt = "error no solution"; break;
		case 15: modstatxt = "solved unique"; break;
		case 16: modstatxt = "solved"; break;
		case 17: modstatxt = "solved singular"; break;
		default: t = ASC_PROG_ERR; modstatxt = "UNKNOWN MODSTA";
	}
	const char *solstatxt;
	switch(*solsta){
		case 1: solstatxt = "normal completion"; break;
		case 2: t = ASC_USER_NOTE; solstatxt = "iteration interrupted"; break;
		case 3: t = ASC_PROG_NOTE; solstatxt = "time limit exceeded"; break;
		case 4: t = ASC_PROG_ERR; solstatxt = "failed (terminated by solver)"; break;
		case 5: t = ASC_PROG_ERR; solstatxt = "Error evaluation limit"; break;
		case 8: t = ASC_USER_NOTE; solstatxt = "User interrupt"; break;
		case 9: t = ASC_PROG_ERR; solstatxt = "Error: setup failure"; break;
		case 10:t = ASC_PROG_ERR; solstatxt = "Error: solver failure"; break;
		case 11:t = ASC_PROG_ERR; solstatxt = "Error: internal solver error"; break;
		case 15:t = ASC_PROG_ERR; solstatxt = "Terminated by Quick Mode"; break;
		default: t = ASC_PROG_ERR; solstatxt = "UNKNOWN SOLSTA";
	}

	MSG("CONOPT %s (%d): %s (%d)", solstatxt, *solsta, modstatxt, *modsta);
	ERROR_REPORTER_NOLINE(t,"CONOPT %s: %s", solstatxt, modstatxt);

	return 0;
}

int COI_CALL asc_conopt_solution( const double XVAL[], const double XMAR[]
		, const int XBAS[], const int XSTA[], const double YVAL[], const double YMAR[]
		, const int YBAS[], const int YSTA[], int N, int M, void* USRMEM
){
	int i;
	const char *status[4] = {"Lower","Upper","Basic","Super"};
	FILE *fd = stderr;
	(void)XSTA;
	(void)YVAL;
	(void)YMAR;
	(void)YSTA;
	(void)USRMEM;

	fprintf(fd,"\n Variable   Solution value    Reduced cost    Status\n\n");
	for ( i=0; i<N; i++ )
		fprintf(fd,"%6d%18f%18f%10s\n", i, XVAL[i], XMAR[i], status[XBAS[i]] );
	fprintf(fd,"\n Constrnt   Activity level    Marginal cost   Status\n\n");
	for ( i=0; i<M; i++ )
		fprintf(fd,"%6d%18f%18f%10s\n", i, YVAL[i], YMAR[i], status[YBAS[i]] );

	return 0;
}

#else

int COI_CALL asc_conopt_progress( int* LEN_INT, int* INT
		, int* LEN_RL, double* RL, double* X, double* USRMEM
){
	MSG("Iteration %d, phase %d: %d infeasible, %d non-optimal; objective = %e"
		, INT[0], INT[1], INT[2], INT[3], RL[1]
	);
	/* FIXME: NEED TO IMPLEMENT SOME KIND OF CALLBACK TO THE SOLVERREPORTER */
	return 0;
}

int COI_CALL asc_conopt_message( int* SMSG, int* DMSG, int* NMSG, int* LLEN
		,double* USRMEM, char* MSGV, int MSGLEN
){
/* This implementation is writing the screen file to stdout
   the documentation file to a file opened in main with the name
   document.txt and the status file to a file with the name
   status.txt.                                                        */
   int i,j,k,l;
   char line[MAXLINE];
   k = 0;
   for( i=0; i<*SMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      MSG("%s", line);
      k += MSGLEN;
   }
/*   k = 0;
   for( i=0; i<*DMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"%s\n", line);
      k += MSGLEN;
   }
*/
   k = 0;
   for( i=0; i<*NMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"(CONOPT) %s", line);
      k += MSGLEN;
   }
   return 0;
}

int COI_CALL asc_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
){
   /* Standard ErrMsg routine. Write to Documentation and Status file*/
   int j,l;
   char line[MAXLINE];
   ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
   if ( *ROWNO == -1 ) {
      FPRINTF(ASCERR,"Variable %d : ",*COLNO); }
   else if ( *COLNO == -1 ) {
      FPRINTF(ASCERR,"Equation %d : ",*ROWNO); }
   else  {
      FPRINTF(ASCERR,"Variable %d appearing in Equation %d : ",*COLNO, *ROWNO); }
   j = *MSGLEN;
   for( l= 0; l<j; l++ ) line[l] = MSG[l];
   line[j] = '\0';
   FPRINTF(ASCERR,"%s\n", line);
   error_reporter_end_flush();
   return 0;
}

int COI_CALL asc_conopt_status(int* MODSTA, int* SOLSTA
		, int* ITER, double* OBJVAL, double* USRMEM
){
	/* Standard Status routine. Write to all files */
	MSG("CONOPT has finished Optimizing");
	MSG("Model status    = %8d", *MODSTA);
	MSG("Solver status   = %8d", *SOLSTA);
	MSG("Iteration count = %8d", *ITER);
	MSG("Objective value = %10f", *OBJVAL);

	const char *modsta;
	error_severity_t t = ASC_USER_SUCCESS;
	switch(*MODSTA){
		case 1: modsta = "optimal"; break;
		case 2: modsta = "locally optimal"; break;
		case 3: t = ASC_USER_ERROR; modsta = "unbounded"; break;
		case 4: t = ASC_USER_ERROR; modsta = "infeasible"; break;
		case 5: modsta = "locally infeasible"; break;
		case 6: modsta = "intermediate infeasible"; break;
		case 7: modsta = "intermediate non-optimal"; break;
		case 12: modsta = "unknown type of error"; break;
		case 13: modsta = "error no solution"; break;
		case 15: modsta = "solved unique"; break;
		case 16: modsta = "solved"; break;
		case 17: modsta = "solved singular"; break;
		default: t = ASC_PROG_ERR; modsta = "UNKNOWN MODSTA";
	}
	const char *solsta;
	switch(*SOLSTA){
		case 1: solsta = "normal completion"; break;
		case 2: t = ASC_USER_NOTE; solsta = "iteration interrupted"; break;
		case 3: t = ASC_PROG_NOTE; solsta = "time limit exceeded"; break;
		case 4: t = ASC_PROG_ERR; solsta = "failed (terminated by solver)"; break;
		case 5: t = ASC_PROG_ERR; solsta = "Error evaluation limit"; break;
		case 8: t = ASC_USER_NOTE; solsta = "User interrupt"; break;
		case 9: t = ASC_PROG_ERR; solsta = "Error: setup failure"; break;
		case 10:t = ASC_PROG_ERR; solsta = "Error: solver failure"; break;
		case 11:t = ASC_PROG_ERR; solsta = "Error: internal solver error"; break;
		case 15:t = ASC_PROG_ERR; solsta = "Terminated by Quick Mode"; break;
		default: t = ASC_PROG_ERR; solsta = "UNKNOWN SOLSTA";
	}

	MSG("CONOPT %s (%d): %s (%d)", solsta, *SOLSTA, modsta, *MODSTA);
	ERROR_REPORTER_NOLINE(t,"CONOPT %s: %s", solsta, modsta);

	return 0;
}

int COI_CALL asc_conopt_solution( double* XVAL, double* XMAR, int* XBAS
		, int* XSTA, double* YVAL, double* YMAR, int* YBAS, int* YSTA
		, int* N, int* M, double* USRMEM
){
   /* Standard Solution routine */
   int i;
   char *status[4] = {"Lower","Upper","Basic","Super"};
   FILE *fd = stderr;

   fprintf(fd,"\n Variable   Solution value    Reduced cost    Status\n\n");
   for ( i=0; i<*N; i++ )
      fprintf(fd,"%6d%18f%18f%10s\n", i, XVAL[i], XMAR[i], status[XBAS[i]] );
   fprintf(fd,"\n Constrnt   Activity level    Marginal cost   Status\n\n");
   for ( i=0; i<*M; i++ )
      fprintf(fd,"%6d%18f%18f%10s\n", i, YVAL[i], YMAR[i], status[YBAS[i]] );

   return 0;
}

#endif

#endif /* ASC_WITH_CONOPT */
