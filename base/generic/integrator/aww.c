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
*//** @file
	C-code implementation of Art Westerberg's DAE integrator
*//*
	by John Pye, Dec 2006.
*/

#include <utilities/config.h>
#include <utilities/ascConfig.h>
#include <utilities/ascPanic.h>
#include "aww.h"

const IntegratorInternals integrator_aww_internals = {
	integrator_aww_create
	,integrator_aww_params_default
	,integrator_aww_analyse
	,integrator_aww_solve
	,NULL /* writematrixfn */
	,NULL /* debugfn */
	,integrator_aww_free
	,INTEG_AWW
	,"AWW"
};

typedef struct{
	unsigned nothing;
}  IntegratorAWWData;

/*-------------------------------------------------------------
  PARAMETERS FOR AWW INTEGRATOR
*/

enum aww_parameters{
	AWW_PARAM_METHOD
	,AWW_PARAM_RTOL
	,AWW_PARAMS_SIZE
};

enum aww_methods{
	AWW_BDF
	,AWW_AM
};
	
/**
	Here the full set of parameters is defined, along with upper/lower bounds,
	etc. The values are stuck into the blsys->params structure.

	@return 0 on success
*/
int integrator_aww_params_default(IntegratorSystem *blsys){
	asc_assert(blsys!=NULL);
	asc_assert(blsys->engine==INTEG_AWW);
	slv_parameters_t *p;
	p = &(blsys->params);

	slv_destroy_parms(p);

	if(p->parms==NULL){
		CONSOLE_DEBUG("params NULL");
		p->parms = ASC_NEW_ARRAY(struct slv_parameter, AWW_PARAMS_SIZE);
		if(p->parms==NULL)return -1;
		p->dynamic_parms = 1;
	}else{
		CONSOLE_DEBUG("params not NULL");
	}

	/* reset the number of parameters to zero so that we can check it at the end */
	p->num_parms = 0;

	slv_param_char(p,AWW_PARAM_METHOD
			,(SlvParameterInitChar){{"method"
			,"Stepping method",1
			,"See Art's notes, sec. 15.2"
		}, "BDF"}, (char *[]){"BDF","AM",NULL}
	);

	slv_param_real(p,AWW_PARAM_RTOL
			,(SlvParameterInitReal){{"rtol"
			,"Scalar relative error tolerance",1
			,"Value of the scalar relative error tolerance."
		}, 1e-4, 0, DBL_MAX }
	);

	asc_assert(p->num_parms == AWW_PARAMS_SIZE);

	CONSOLE_DEBUG("Created %d params", p->num_parms);

	return 0;
}

void integrator_aww_create(IntegratorSystem *blsys){
	CONSOLE_DEBUG("ALLOCATING AWW ENGINE DATA");
	IntegratorAWWData *enginedata;
	enginedata = ASC_NEW(IntegratorAWWData);
	enginedata->nothing = 0;
	blsys->enginedata = (void *)enginedata;
	integrator_aww_params_default(blsys);
}

void integrator_aww_free(void *enginedata){
	CONSOLE_DEBUG("DELETING AWW ENGINE DATA");
	IntegratorAWWData *d = (IntegratorAWWData *)enginedata;
	ASC_FREE(d);
}

/** return 1 on success */
int integrator_aww_analyse(IntegratorSystem *blsys){
	return 0;
}

/** return 0 on success */
int integrator_aww_solve(IntegratorSystem *blsys
		, unsigned long start_index, unsigned long finish_index
){
	double initstep, maxstep, rtol;
	unsigned maxsubsteps;
	const char *methodname;

	// solve initial point

	// run valuesForInit
	// run specifyForInit
	// solve with QRSlv
	// if not converged throw error

	// 'DELETE SYSTEM'
	// run valuesForStep
	// run specifyForStep
	
	initstep = integrator_get_stepzero(blsys);
	maxstep = integrator_get_maxstep(blsys);
	maxsubsteps = integrator_get_maxsubsteps(blsys);
	rtol = SLV_PARAM_REAL(&(blsys->params),AWW_PARAM_RTOL);

	methodname = SLV_PARAM_CHAR(&(blsys->params),AWW_PARAM_METHOD);
	CONSOLE_DEBUG("ASSIGNING STEPPING METHOD '%s'",methodname);
	
	enum aww_methods method;
	if(strcmp(methodname,"BDF")==0)method = AWW_BDF;
	else if(strcmp(methodname,"AM")==0)method = AWW_AM;
	else{
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Invalid method name '%s'",methodname);
	}

	/* initialise the integrator reporter */
	integrator_output_init(blsys);

	// while not too many solver calls {
	//    capture plotting points
	//    do something with polynomials
	//    for(i=0; i<= poly_oder; ++i){
	//		if(!in the final step){
	//			if(!have hit stopping configtion) UN 'stepX'
	//			SOLVE with QRSlv
	//			if(!converged)abort 'failed to convert
	//		}
	//		if(in the final step)abort 'STOP reached'
	//		RUN setStopConditions
	//		retrieve status of stop confitions
	//      retureve status of 'last step'
	//    }
	//    compute maximum steps for each variable
	// }

	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Not implemented");
	return 1;
}

