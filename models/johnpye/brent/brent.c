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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	Brent solver callable as an ASCEND 'EXTERNAL' method.
*//*
	by John Pye, Mar 2007
*/

#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include <utilities/error.h>
#include <general/ospath.h>
#include <compiler/extfunc.h>
#include <compiler/symtab.h>
#include <compiler/instquery.h>
#include <compiler/parentchild.h>
#include <compiler/atomvalue.h>
#include <compiler/initialize.h>
#include <compiler/type_desc.h>
#include <compiler/rootfind.h>
#include <compiler/instance_io.h>

/* #define BRENT_DEBUG */

ExtMethodRun brent_eval;

/**
	This is the function called from "IMPORT brent"

	It sets up the functions in this external function library
*/
extern ASC_EXPORT int brent_register(){
	int result=0;
	result += CreateUserFunctionMethod("brent",
		brent_eval,
		4,NULL,NULL,NULL
	);
	return result;
}

/**
	Gather instance arguments from the arglist and check that they are the right
	kinds of instances. Check that the 'by changing' var is 'fixed'.

	@param arglist List of arguments
	@param arg Returned instances list, length 5: model,seekto,bychanging,lb,ub,nom

	Argument list contains the following:
	  . arg1 - model inst for which the sensitivity analysis is to be done.
	  . arg2 - goal variable (we want its value to go to zero)
	  . arg3 - 'by changing' variable.
*/
int brent_check_args(struct gl_list_t *arglist,struct Instance **arg){
	unsigned long len;
	unsigned long i;
	struct Instance *c;
	symchar *fixed,*lb,*ub,*nom;

	fixed = AddSymbol("fixed");
	lb = AddSymbol("lower_bound");
	ub = AddSymbol("upper_bound");
	nom = AddSymbol("nominal");

	len = gl_length(arglist);
	if(len != 3){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Three arguments required for 'brent' EXTERNAL method.");
		return 1;
	}

	for(i=0;i<len;++i){
		if(gl_fetch(arglist,i+1)==NULL){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"NULL in argument list at position %d",i);
			return 1;
		}
		if(gl_length((struct gl_list_t*)gl_fetch(arglist,i+1))!=1){
			ERROR_REPORTER_HERE(ASC_USER_ERROR,"Three simple arguments (not lists) required for 'brent' EXTERNAL method.");
			return 1;
		}
		arg[i] = (struct Instance *)gl_fetch((struct gl_list_t*)gl_fetch(arglist,i+1),1);
	}

	if(InstanceKind(arg[0])!=MODEL_INST){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"First argument of 'brent' EXTERNAL method should be a MODEL instance");
		return 1;
	}

	if(InstanceKind(arg[1])!=REAL_ATOM_INST){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Second argument of 'brent' EXTERNAL method should be real-valued atom instance");
		return 1;
	}		

	c = ChildByChar(arg[2],nom);
	if(c==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Nominal value not found in second argument");
		return 1;
	}
	arg[5] = c;

	if(InstanceKind(arg[2])!=REAL_ATOM_INST){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Third argument of 'brent' EXTERNAL method should be real-valued atom instance");
		return 1;
	}

	c = ChildByChar(arg[2],lb);
	if(c==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Lower bound not found in third argument");
		return 1;
	}
	arg[3] = c;

	c = ChildByChar(arg[2],ub);
	if(c==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Upper bound not found in third argument");
		return 1;
	}
	arg[4] = c;	


	c = ChildByChar(arg[2],fixed);
	if(c==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Third argument of 'brent' EXTERNAL method needs attribute 'fixed'");
		return 1;
	}
	asc_assert(InstanceKind(c)==BOOLEAN_INST);
	if(GetBooleanAtomValue(c)!=TRUE){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"The third ('by changing') argument of 'brent' EXTERNAL method should first be fixed");
		return 1;
	}		

	return 0;
}

struct InitProcedure *brent_find_solve_method(struct Instance **arg){
	symchar *name;
	name = AddSymbol("solve");
	struct TypeDescription *type;
	type = InstanceTypeDesc(arg[0]);
	asc_assert(type);
	return FindMethod(type,name);	
}

struct BrentProblem{
	struct Instance *model;
	struct Instance *seekto;
	double seekvalue;
	double seeknominal;
	struct Instance *bychanging;
	double lower_bound;
	double upper_bound;
	struct InitProcedure *solvemethod;
};

/**
	ExtEvalFunc type is a function pointer.

	@see rootfind.c:51

	@param mode 'to pass to the eval function' (?)
	@param m    relation index
	@param n    variable index
	@param x    'x' vector (?)
	@param u    'u' vector (?)
	@param f    vector of residuals
	@param g    vector of gradients
*/
static ExtEvalFunc brent_resid;

static int brent_resid(int *mode, int *m, int *n, double *x, double *u, double *f, double *g){
	struct BrentProblem *prob;
	(void)m;
	(void)n;
	(void)u;
	(void)g;
	prob = (struct BrentProblem *)mode; /* sneaky, eh! */
	

	SetRealAtomValue(prob->bychanging,x[0],0);
	enum Proc_enum pe;	
	pe = Initialize(prob->model, CreateIdName(ProcName(prob->solvemethod))
		,(char *)SCP(prob->solvemethod->name)
		,ASCERR
		,0, NULL, NULL
	);
	f[0] = RealAtomValue(prob->seekto) - prob->seekvalue;
	ERROR_REPORTER_HERE(ASC_USER_NOTE,"x = %f --> u = %f",x[0],f[0]);
	if(pe!=Proc_all_ok)return 1;
	return 0;
};		
	
int brent_solve(struct BrentProblem *prob){
	int iserr;

	double x[1];
	double u[1];
	double tol[1];
	double f[1], g[1];
	int m=0, n=0;

	tol[0] = 1e-7 * prob->seeknominal;

	char *name1, *name2;
	name1 = WriteInstanceNameString(prob->seekto,prob->model);
	name2 = WriteInstanceNameString(prob->bychanging,prob->model);
	CONSOLE_DEBUG("Solving '%s' to value %f by changing '%s'",name1, prob->seekvalue, name2);
	ASC_FREE(name1); ASC_FREE(name2);

	zbrent(&brent_resid
		,&(prob->lower_bound)
		,&(prob->upper_bound)
		,(int *)(prob)
		,&m, &n
		,x, u, f, g, tol
		,&iserr
	);

	if(iserr){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Brent solver failed");
		return 1;
	}
	return 0;
}

int brent_eval(struct Instance *context, struct gl_list_t *arglist, void *user_data){
	struct Instance *arg[6];
	struct InitProcedure *solvemethod;
	struct BrentProblem prob;
	int res;
	res = brent_check_args(arglist,arg);
	if(res)return res;

	solvemethod = brent_find_solve_method(arg);
	if(!solvemethod){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"No 'solve' method in model!");
		return 1;
	}

	prob.model = arg[0];
	prob.seekto = arg[1];
	prob.seekvalue = 0;
	prob.seeknominal = RealAtomValue(arg[5]);
	prob.bychanging = arg[2];
	prob.lower_bound = RealAtomValue(arg[3]);
	prob.upper_bound = RealAtomValue(arg[4]);
	prob.solvemethod = solvemethod;

	CONSOLE_DEBUG("Seeking solution between lower bound %f and upper bound %f",prob.lower_bound,prob.upper_bound);

	return brent_solve(&prob);
}
