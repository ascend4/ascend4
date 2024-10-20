/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
	Solver API for ASCEND, for solving NLA, LP, NLP, MINLP problems (anything
	without derivatives).
*//*
	by John Pye, May 2006
	based on parts of slv_stdcalls.c, modified to provide dynamic loading
	support, and aiming to improve the separation of 'system' and 'solver',
	the former being a collection of equations and variables, and the latter
	being all the methods and data storage that allows a solution to be
	found and reported.
*/

#include "solver.h"

#include <ascend/system/system_impl.h>
#include <ascend/general/list.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/compiler/packages.h>
#include <ascend/general/ospath.h>

#ifdef WIN32
# include <windows.h>
#endif

//#define SOLVER_DEBUG
#ifdef SOLVER_DEBUG
# define MSG CONSOLE_DEBUG
# define ERRMSG CONSOLE_DEBUG
#else
# define MSG(...) 
# define ERRMSG CONSOLE_DEBUG
#endif

/**
	Local function that holds the list of available solvers. The value 
	returned is NOT owned by the called.

	@param free_space if 0, call as normal. if 1, free the list and maybe do some
	cleaning up etc. Should be called whenever a simulation is destroyed.
*/
static struct gl_list_t *solver_get_list(int free_space){
	static int init = 0;
	static struct gl_list_t *L;
	if(free_space){
		if(init && L) {
			 /*FIXME: ASC_FREE(L); free is never used on gl_list_t */
			gl_destroy(L);
		}
		init = 0;
		return NULL;
	}
	if(!init){
		L = gl_create(10);
		init = 1;
	}
	return L;
}

/**
	Return gl_list of SlvFunctionsT. C++ will use this to produce a
	nice little list of integrator names that can be used in Python :-/
*/
const struct gl_list_t *solver_get_engines(){
	return solver_get_list(0);
}
/** expansion use locally */
struct gl_list_t *solver_get_engines_growable(){
	return solver_get_list(0);
}

void solver_destroy_engines(){
	solver_get_list(1);
}

const SlvFunctionsT *solver_engine(const int number){
	const struct gl_list_t *L = solver_get_engines();
	int i;
	const SlvFunctionsT *S, *Sfound=NULL;
	MSG("Searching for solver #%d in list of %ld solvers",number,gl_length(L));
	for(i=1; i <= gl_length(L); ++i){
		S = gl_fetch(L,i);
		MSG("Looking at %s (%d)",S->name,S->number);
		if(S->number==number){
			MSG("Match!");
			Sfound = S;
			break;
		}
	}
	MSG("Returning %p",Sfound);
	return Sfound;
}

const SlvFunctionsT *solver_engine_named(const char *name){
	const struct gl_list_t *L = solver_get_engines();
	int i;
	const SlvFunctionsT *S, *Sfound=NULL;
	for(i=1; i <= gl_length(L); ++i){
		S = gl_fetch(L,i);
		if(strcmp(S->name,name)==0){
			Sfound = S;
			break;
		}
	}
	return Sfound;
}	

int slv_lookup_client(const char *name){
#if 0 
	int i;

	if(name == NULL)return -1;

	for(i = 0; i < NORC; i++) {
		if(strcmp( SCD(i).name, name)==0) {
		    return i;
		}
	}
	return -1;
#endif
	const struct gl_list_t *L = solver_get_engines();
	int i;
	const SlvFunctionsT *S, *Sfound=NULL;
	for(i=1; i <= gl_length(L); ++i){
		S = gl_fetch(L,i);
		if(strcmp(S->name,name)==0){
			Sfound = S;
			break;
		}
	}
	if(Sfound){
		return S->number;
	}else{
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Invalid engine name '%s'",name);
		return -1;
	}
}


/** 
	Register a new solver.
	
	@TODO This needs work still, particularly of the dynamic loading
	sort. it would be good if here we farmed out the dynamic loading
	to another file so we don't have to crap this one all up.

	old args:
(SlvRegistration registerfunc, CONST char *func
		,CONST char *file, int *new_client_id
*/
int solver_register(const SlvFunctionsT *solver){
#if 0
	int status;

	status = registerfunc(&( SlvClientsData[NORC]));
	if (!status) { /* ok */
		SlvClientsData[NORC].number = NORC;
		*new_client_id = NORC;
		NORC++;
	} else {
		*new_client_id = -2;
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR,"Client %d registration failure (%d)!",NORC,status);
	}
	return status;
#endif

	/* get the current list of registered engines */
	struct gl_list_t *L;
	L = solver_get_engines_growable();

	MSG("REGISTERING SOLVERS");
	MSG("There were %lu registered solvers", gl_length(solver_get_list(0)));

	int i;
	const SlvFunctionsT *S;
	for(i=1; i < gl_length(L); ++i){
		S = (const SlvFunctionsT *)gl_fetch(L,i);
		if(strcmp(solver->name,S->name)==0){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Solver with name '%s' is already registered",solver->name);
			return 0;
		}
		if(solver->number == S->number){
			ERROR_REPORTER_HERE(ASC_USER_WARNING,"Solver with ID '%d' is already registered",solver->number);
			return 0;
		}
	}

	MSG("Adding engine '%s'",solver->name);

	gl_append_ptr((struct gl_list_t *)L,(SlvFunctionsT *)solver);
	
	MSG("There are now %lu registered solvers", gl_length(solver_get_list(0)));
	return 0;
}


/*------------------------------------------------------------------------------
  SOLVER REGISTRATION
*/

/* rewrote this stuff to get rid of all the #ifdefs -- JP */

struct StaticSolverRegistration{
	const char *importname;
};

/*
	The names here are only used to provide information in the case where
	solver registration fails. The definitive solver names are in the slv*.c
	files.
*/
static const struct StaticSolverRegistration slv_reg[]={
	{"qrslv"}
	,{"ipopt"}
#if 0
	,{"conopt"}
	,{"lrslv"}
	,{"cmslv"}
#endif
	,{NULL}
/* 	{0,"SLV",&slv0_register} */
/*	,{0,"MINOS",&slv1_register} */
/*	,{0,"CSLV",&slv4_register} */
/*	,{0,"LSSLV",&slv5_register} */
/*	,{0,"MPS",&slv6_register} */
/*	,{0,"NGSLV",&slv7_register} */
/* 	,{0,"OPTSQP",&slv2_register} */
};

#if 0
/* this code can automate calls to AddDllDirectory in Windows. however, for now, we found it wasn't needed (surprisingly) */
#ifdef WIN32
# ifdef ASC_DLLDIRS
static FilePathTestFn add_solver_dll_dir;
static int add_solver_dll_dir(struct FilePath *fp,void *user_data){
	char *nm = ospath_str(fp);
	MSG("Adding DLL directory '%s'",nm);
	int l = MultiByteToWideChar(CP_UTF8, 0, nm, -1, NULL, 0);
	WCHAR *wstr = ASC_NEW_ARRAY(WCHAR,l);
	int res = MultiByteToWideChar(CP_UTF8,0,nm,-1,wstr,l);
	if(0==res){
		perror("MultiByteToWideChar");
		return 1;
	}
	AddDllDirectory(wstr);
	ASC_FREE(nm);
	ASC_FREE(wstr);
	return 0; // ensures we keep going through the whole list
}
static int add_source_dll_dir(struct FilePath *fp,void *user_data){
	struct FilePath *fp1 = ospath_new(ASC_SOURCE_ROOT);
	struct FilePath *fp2 = ospath_concat(fp1,fp);
	struct FilePath *fp3 = ospath_getabs(fp2);
	ospath_free(fp1); ospath_free(fp2);
	int res = add_solver_dll_dir(fp3,NULL);
	ospath_free(fp3);
	return res;
}
# endif
#endif
#endif

int SlvRegisterStandardClients(void){
	int nclients = 0;
	//int newclient=0;
	int error;
	int i;

#if 0
#ifdef WIN32
# ifdef ASC_DLLDIRS
	MSG("ADDING DLL DIRS");
	MSG("IPOPT...");

	//struct FilePath *fp1 = ospath_new("solvers/ipopt");
	//add_source_dll_dir(fp1, NULL);

	//struct FilePath *fp2 = ospath_new("c:/msys64/home/john/.local/bin");
	//add_solver_dll_dir(fp2, NULL);

	//struct FilePath *fp3 = ospath_new("c:/msys64/home/john/ascend");
	//add_solver_dll_dir(fp3, NULL);

	//struct FilePath *fp4 = ospath_new("c:/windows/system32");
	//add_solver_dll_dir(fp4, NULL);

	//struct FilePath *fp5 = ospath_new("c:/msys64/mingw64/bin");
	//add_solver_dll_dir(fp5, NULL);
	
#if 0
	MSG("Raw input: " ASC_DLLDIRS);
	struct FilePath **dlldirs = ospath_searchpath_new(ASC_DLLDIRS);
	void *res = ospath_searchpath_iterate(dlldirs,&add_solver_dll_dir,NULL);
	if(NULL != res){
		MSG("ERROR DLLDIRS");
	}	
#endif
# endif
#endif
#endif

	MSG("REGISTERING STANDARD SOLVER ENGINES");
	for(i=0; slv_reg[i].importname!=NULL;++i){
		MSG("Registering '%s'",slv_reg[i].importname);
		error = package_load(slv_reg[i].importname,NULL);
		if(error){
			ERROR_REPORTER_HERE(ASC_PROG_NOTE
				,"Unable to register solver '%s' (error %d).\n"
				,slv_reg[i].importname,error
			);
		}else{
			/* CONSOLE_DEBUG("Solver '%s' registered OK",slv_reg[i].importname); */
			nclients++;
		}
	}
  return nclients;
}

/*------------------------------------------------------*/

static void printwarning(const char * fname, slv_system_t sys)
{
  ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,
    "%s called with bad registered client (%s).",fname,
    slv_solver_name(slv_get_selected_solver(sys)));
}

static void printinfo(slv_system_t sys, const char *rname){
  asc_assert(sys->internals);
  if(sys->internals->name) {
    ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,
      "Client %s does not support function '%s'.",
      sys->internals->name,rname);
  }
}

/*-----------------------------------------------------------
	These macros do some more elimination of repetition. Here we're
	trying to replace some more complex 'method-like' calls on
	slv_system_t:

	These macros use macro-argument-concatenation and macro stringification.
	Verified that the former works with Visual C++.
	http://www.codeproject.com/macro/metamacros.asp
*/

/** Define a method like 'void slv_METHODNAME(sys)' */
#define DEFINE_SLV_PROXY_METHOD_VOID(METHOD) \
	void slv_ ## METHOD (slv_system_t sys){ \
		if(CF(sys,METHOD)==NULL){ \
			printwarning(#METHOD,sys); \
			return; \
		} \
		SF(sys,METHOD)(sys,sys->ct); \
	}

/** Define a method like 'RETURNTYPE slv_METHOD(sys)'; */
#define DEFINE_SLV_PROXY_METHOD(METHOD,PROP,RETTYPE,ERRVAL) \
	RETTYPE slv_ ## METHOD (slv_system_t sys){ \
		/* CONSOLE_DEBUG("slv_" #METHOD);*/ \
		asc_assert(sys->internals); \
		/*CONSOLE_DEBUG("internals OK");*/ \
		if(sys->internals->PROP==NULL){ \
			/*CONSOLE_DEBUG("method is NULL");*/ \
			printinfo(sys, #METHOD); \
			return ERRVAL; \
		} \
		/*CONSOLE_DEBUG("running method " #PROP " in solver %d",sys->internals->number);*/ \
		return (sys->internals->PROP)(sys,sys->ct); \
	}

/** Define a method like 'void slv_METHOD(sys,TYPE PARAMNAME)'; */
#define DEFINE_SLV_PROXY_METHOD_PARAM(METHOD,PROP,PARAMTYPE,PARAMNAME) \
	void slv_ ## METHOD (slv_system_t sys, PARAMTYPE PARAMNAME){ \
		if(!sys->internals || !sys->internals->PROP){ \
			printwarning(#METHOD,sys); \
			return; \
		} \
		(sys->internals->PROP)(sys,sys->ct, PARAMNAME); \
	}

DEFINE_SLV_PROXY_METHOD_PARAM(get_parameters,get_parameters,slv_parameters_t*,parameters) /*;*/

void slv_set_parameters(slv_system_t sys,slv_parameters_t *parameters)
{
  asc_assert(sys->internals);
  if(sys->internals->setparam == NULL ) {
    printwarning("slv_set_parameters",sys);
    return;
  }
  if (parameters->whose != sys->solver) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,
		"slv_set_parameters cannot pass parameters from one solver to a"
		" another.");
    return;
  }
  (sys->internals->setparam)(sys,sys->ct,parameters);
}

int slv_get_status(slv_system_t sys, slv_status_t *status){
	static const SlvFunctionsT *lastsolver = 0;
	if(!sys->internals)return -1;
	if(sys->internals->getstatus==NULL){
		if(lastsolver==0 || lastsolver != sys->internals){
			printinfo(sys,"get_status");
			lastsolver = sys->internals;
		}
		return -1;
	}
	return (sys->internals->getstatus)(sys,sys->ct,status);
}

DEFINE_SLV_PROXY_METHOD_PARAM(dump_internals,dumpinternals,int,level) /*;*/

DEFINE_SLV_PROXY_METHOD(get_linsolqr_sys, getlinsys, linsolqr_system_t, NULL) /*;*/

DEFINE_SLV_PROXY_METHOD(get_sys_mtx, get_sys_mtx, mtx_matrix_t, NULL) /*;*/

DEFINE_SLV_PROXY_METHOD(presolve,presolve,int,-1) /*;*/
DEFINE_SLV_PROXY_METHOD(resolve,resolve,int,-1) /*;*/
DEFINE_SLV_PROXY_METHOD(iterate,iterate,int,-1) /*;*/
DEFINE_SLV_PROXY_METHOD(solve,solve,int,-1) /*;*/

int slv_eligible_solver(slv_system_t sys)
{
  asc_assert(sys->internals);
  if(sys->internals->celigible == NULL ) {
    printwarning("slv_eligible_solver",sys);
    return 0;
  }
  return (sys->internals->celigible)(sys);
}

//-------------

	
int slv_select_solver(slv_system_t sys,int solver){

  int status_index;
  SlvClientDestroyF *destroy;
  const SlvFunctionsT *S;

  if(sys ==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver called with NULL system.");
    return -1;
  }

  /* CONSOLE_DEBUG("CHECKING FOR SOLVER %d", solver); */

  if(solver_engine(solver)){
	/* CONSOLE_DEBUG("SOLVER FOUND"); */
    if(sys->ct != NULL && solver != sys->solver){
	  /* CONSOLE_DEBUG("DIFFERENT SOLVER"); */
      //CONSOLE_DEBUG("g_SlvNumberOfRegisteredClients = %d, sys->solver = %d", g_SlvNumberOfRegisteredClients, sys->solver);
	  asc_assert(sys->solver >= -1);
	  //asc_assert(g_SlvNumberOfRegisteredClients > 0);
	  //asc_assert(sys->solver < g_SlvNumberOfRegisteredClients);
	  S = solver_engine(sys->solver);
      destroy = S->cdestroy;
      if(destroy!=NULL) {
        (destroy)(sys,sys->ct);
        sys->ct = NULL;
      }else{
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver: 'cdestroy' is undefined on solver '%s' (index %d).",
          S->name, sys->solver);
      }
    }

	/* CONSOLE_DEBUG("PREVIOUS SOLVER IS CLEAR"); */

    if(sys->ct != NULL) {
#ifdef SOLVER_DEBUG
	  CONSOLE_DEBUG("CURRENT SOLVER UNCHANGED");
#endif
      return sys->solver;
    }

    status_index = solver;
    sys->solver = solver;
    sys->internals = solver_engine(solver);
    if(sys->internals->ccreate != NULL){
      sys->ct = (sys->internals->ccreate)(sys,&status_index);
    }else{
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_select_solver create failed due to bad client '%s'.",
        slv_solver_name(sys->solver));
      return sys->solver;
    }
    if(sys->ct==NULL){
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate failed in slv_select_solver.");
      sys->solver = -1;
    }else{
      if (status_index){
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate succeeded with warning %d %s.",
          status_index," in slv_select_solver");
      }
      /* we could do a better job explaining the client warnings... */
      sys->solver = solver;
    }
  }else{
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_select_solver: invalid solver index '%d'.",
      solver);
    return -1;
  }
  return sys->solver;
}

/**
	@TODO looks buggy
*/
int slv_switch_solver(slv_system_t sys,int solver)
{
  int status_index;

  if(sys ==NULL){
    ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_switch_solver called with NULL system.");
    return -1;
  }

  /* CONSOLE_DEBUG("CHECKING FOR SOLVER %d", solver); */

  if(solver_engine(solver)){
    status_index = solver;
    sys->solver = solver;
	sys->internals = solver_engine(solver);
	CONSOLE_DEBUG("SWITCHING TO SOLVER '%s'",sys->internals->name);
    if(sys->internals->ccreate != NULL) {
      sys->ct = (sys->internals->ccreate)(sys,&status_index);
    } else {
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"slv_switch_solver create failed due to bad client '%s'.",
         slv_solver_name(sys->solver));
      return sys->solver;
    }
    if (sys->ct==NULL) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"SlvClientCreate failed in slv_switch_solver.");
      sys->solver = -1;
    }else{
      if (status_index) {
        ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"SlvClientCreate succeeded with warning %d %s.",
           status_index," in slv_switch_solver");
      }
      sys->solver = solver;
    }
  }else{
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Unknown client '%d'.",solver);
    return -1;
  }
  return sys->solver;
}

/*--------------------------------*/

int slv_get_selected_solver(slv_system_t sys){
  if (sys!=NULL) return sys->solver;
  return -1;
}


int32 slv_get_default_parameters(int sindex,
				slv_parameters_t *parameters)
{
  const SlvFunctionsT *S;
  S = solver_engine(sindex);
  if(S){
    if(S->getdefparam == NULL ) {
      ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_default_parameters called with parameterless index.");
      return 0;
    }else{
      /* send NULL system when setting up interface */
      (S->getdefparam)(NULL,NULL,parameters);
      return 1;
    }
  }else{
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_get_default_parameters called with unregistered index.");
    return 0;
  }
}


/*-----------------------------------------------------------*/

SlvClientToken slv_get_client_token(slv_system_t sys)
{
  if (sys==NULL) {
    FPRINTF(stderr,"slv_get_client_token called with NULL system.");
    return NULL;
  }
  return sys->ct;
}


void slv_set_client_token(slv_system_t sys, SlvClientToken ct)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_client_token called with NULL system.");
    return;
  }
  sys->ct = ct;
}

void slv_set_solver_index(slv_system_t sys, int solver)
{
  if (sys==NULL) {
    ERROR_REPORTER_NOLINE(ASC_PROG_ERROR,"slv_set_solver_index called with NULL system.");
    return;
  }
  sys->solver = solver;
  sys->internals = solver_engine(solver);
}


const char *slv_solver_name(int sindex){
  static char errname[] = "ErrorSolver";
  const SlvFunctionsT *S = solver_engine(sindex);
  if(S!=NULL){
    if(S->name == NULL) {
      ERROR_REPORTER_HERE(ASC_PROG_WARNING,"unnamed solver: index='%d'",sindex);
      return errname;
    }else{
	  return S->name;
    }
  }else{
    ERROR_REPORTER_HERE(ASC_PROG_ERROR,"invalid solver index '%d'", sindex);
    return errname;
  }
}

// vim: ts=4:noet:
