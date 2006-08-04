/*	ASCEND modelling environment
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
	This file is just a place-holder at the moment. It will be used to
	implement the DYNAMIC_CONOPT interface, which will allow the CONOPT
	solver to be dlopened at runtime.
*//*
	By John Pye
	Based on conopt.c by Vicente Rico Ramirez (created 05/97)
*/

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <utilities/error.h>
#include <utilities/ascEnvVar.h>
#include <general/env.h>
#include "conopt.h"

#ifndef ASC_WITH_CONOPT
# error "Shouldn't compile this file unless ASC_WITH_CONOPT set"
#endif

#ifndef ASC_LINKED_CONOPT
# include <ctype.h>
# include <utilities/ascMalloc.h>
# include <utilities/ascDynaLoad.h>

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
	This funciton will load the DLL and resolve all the required symbols
*/
int asc_conopt_load(){
	static int loaded=0;
	char *libpath;
	int status;
	char fnsymbol[400], *c;
	const char *libname=ASC_CONOPT_LIB;
	const char *envvar;

	if(loaded) {
		return 0; /* already loaded */
	}

	CONSOLE_DEBUG("LOADING CONOPT...");

	envvar  = ASC_CONOPT_ENVVAR;

	/* need to import this variable into the ascend 'environment' */
	if(-1!=env_import(ASC_CONOPT_ENVVAR,getenv,Asc_PutEnv)){
		CONSOLE_DEBUG("Using searchpath env var %s = %s",envvar,getenv(envvar));
	}else{
		CONSOLE_DEBUG("Using default conopt search path: %s", ASC_CONOPT_DLPATH);
	}

	libpath = SearchArchiveLibraryPath(libname, ASC_CONOPT_DLPATH, envvar);

	if(libpath==NULL){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR
			, "Library '%s' could not be located (check env var %s)"
			, libname, envvar
		);
		return 1;
	}

	status = Asc_DynamicLoad(libpath, NULL);
	if (status != 0) {
		ASC_FREE(libpath);
		return 1; /* failed to load */
	}

# if defined(FNAME_UCASE_NODECOR) || defined(FNAME_UCASE_DECOR) || defined(FNAME_UCASE_PREDECOR)
#  define FNCASE(C) C=toupper(C)
# elif defined(FNAME_LCASE_NODECOR) || defined(FNAME_LCASE_DECOR)
#  define FNCASE(C) C=tolower(C)
# else
#  error "CONOPT case rule not defined"
# endif

# if defined(FNAME_UCASE_DECOR) || defined(FNAME_LCASE_DECOR)
#  define FNDECOR(S,L) strcat(S,"_")
# elif defined(FNAME_UCASE_PREDECOR) /* on windows, precede with _ and append @L (integer value of L) */
#  define FNDECOR(S,L) strcat(S,L);for(c=S+strlen(S)+1;c>S;--c){*c=*(c-1);} *S='_';
# else
#  define FNDECOR(S,L) (void)0
# endif

# define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	for(c=fnsymbol;*c!='\0';++c){ \
		FNCASE(*c); \
	} \
	FNDECOR(fnsymbol,L); \
	conopt_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(conopt_fptrs.T##_ptr==NULL)status+=1;

	CONOPT_FNS(FN_PTR_GET,SPACE)

# undef FN_PTR_GET
# undef FNDECOR
# undef FNCASE

	ASC_FREE(libpath);

	if(status!=0){
		return 1; /* faile to result all symbols */
	}

    loaded = 1;
	return 0;
}

#endif

/*-----------------------------------------------------------------------------
   std.c

   This file has some 'standard' implementations for the mandatory
   callback routines Message, ErrMsg, Status, and Solution.
   The routines use global file pointers, so they are only intended
   as examples that can be used for further refinements.
*/

#define MAXLINE 133  /* maximum line length plus an extra character
                        for the null terminator                       */

int COI_CALL asc_conopt_progress( int* LEN_INT, int* INT
		, int* LEN_RL, double* RL, double* X, double* USRMEM
){
	/*(void)CONSOLE_DEBUG("Iteration %d, phase %d: %d infeasible, %d non-optimal; objective = %e"
		, INT[0], INT[1], INT[2], INT[3], RL[1]
	);*/
	/* NEED TO IMPLEMENT SOME KIND OF CALLBACK TO THE SOLVERREPORTER */
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
      (void)CONSOLE_DEBUG("%s", line);
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
      ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"%s\n", line);
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
	(void)CONSOLE_DEBUG("CONOPT has finished Optimizing");
	(void)CONSOLE_DEBUG("Model status    = %8d", *MODSTA);
	(void)CONSOLE_DEBUG("Solver status   = %8d", *SOLSTA);
	(void)CONSOLE_DEBUG("Iteration count = %8d", *ITER);
	(void)CONSOLE_DEBUG("Objective value = %10f", *OBJVAL);

	const char *modsta;
	error_severity_t t = ASC_USER_SUCCESS;
	switch(*MODSTA){
		case 1: modsta = "optimal"; break;
		case 2: modsta = "locally optimal"; break;
		case 3: t = ASC_USER_ERROR; modsta = "unbounded"; break;
		case 4: t = ASC_USER_ERROR; modsta = "infeasible"; break;
	}
	const char *solsta;
	switch(*SOLSTA){
		case 1: solsta = "normal completion"; break;
		case 2: t = ASC_USER_NOTE; solsta = "iteration interrupted"; break;
		case 3: t = ASC_PROG_NOTE; solsta = "time limit exceeded"; break;
		case 4: t = ASC_PROG_ERR; solsta = "failed (terminated by solver)"; break;
	}

	(void)CONSOLE_DEBUG("CONOPT %s: %s", solsta, modsta);
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
