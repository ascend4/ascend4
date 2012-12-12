/*	ASCEND modelling environment
	Copyright (C) 2008 Carnegie Mellon University

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
*//** @file
	Wrapper for sunpos_grena.c to allow access to the calculation from ASCEND.
*/

/* include the external function API from libascend... */
#include <ascend/compiler/extfunc.h>

/* include error reporting API as well, so we can send messages to user */
#include <ascend/utilities/error.h>

/* for accessing the DATA instance */
#include <ascend/compiler/child.h>
#include <ascend/general/list.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/childinfo.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/slist.h>
#include <ascend/compiler/type_desc.h>
#include <ascend/compiler/packages.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/instmacro.h>
#include <ascend/compiler/instance_types.h>

/* the code that we're wrapping... */
#include "sunpos_grena.h"

ExtBBoxInitFunc sunpos_prepare;
ExtBBoxFunc sunpos_calc;

/* place to store symbols needed for accessing ASCEND's instance tree */
static symchar *sunpos_symbols[2];
#define LATITUDE_SYM sunpos_symbols[0]
#define LONGITUDE_SYM sunpos_symbols[1]

static const char *sunpos_help = "\
Calculate sun position (local zenith, azimuth angles) given time, pressure \n\
and temperature, using Grena algorithm. DATA member for this external relation \n\
is required to provide constants for latitude and longitude of the selected \n\
location.\n\
\n\
Time input to this calculation is an offset from 0h00 on 1 Jan 2003, which \n\
ASCEND will automatically convert to seconds if you use a variable of type \n\
'time'.";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from 'IMPORT "johnpye/grena/sunpos";'

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int sunpos_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"SUNPOS is still EXPERIMENTAL. Use with caution.\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, sunpos_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFN(sunpos,3,2);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}


/**
	This function is called when the black-box relation is being instantiated.

   'sunpos_prepare' just gets the data member and checks that it's
	valid, and stores it in the blackbox data field.
*/
int sunpos_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *inst;
	double latitude, longitude;

	LATITUDE_SYM = AddSymbol("latitude");
	LONGITUDE_SYM = AddSymbol("longitude");

	/* get the latitude */
	inst = ChildByChar(data,LATITUDE_SYM);
	if(!inst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'latitude' in DATA, please check usage of SUNPOS."
		);
		return 1;
	}
	if(InstanceKind(inst)!=REAL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'latitude' must be a real_constant");
		return 1;
	}
	latitude = RC_INST(inst)->value;
	CONSOLE_DEBUG("Latitude: %0.3f",latitude);
	if(latitude > PI/2 || latitude < -PI/2){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'latitude' is out of allowable range -PI/2 to PI/2.");
		return 1;
	}

	/* get the longitude */
	inst = ChildByChar(data,LONGITUDE_SYM);
	if(!inst){
		ERROR_REPORTER_HERE(ASC_USER_ERROR
			,"Couldn't locate 'longitude' in DATA, please check usage of SUNPOS."
		);
		return 1;
	}
	if(InstanceKind(inst)!=REAL_CONSTANT_INST){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member 'longitude' must be a real_constant");
		return 1;
	}
	longitude = RC_INST(inst)->value;
	CONSOLE_DEBUG("Longitude: %0.3f",longitude);

	if(longitude > PI || longitude < -PI){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'latitude' is out of allowable range -PI to PI.");
		return 1;
	}

	SunPos *S = ASC_NEW(SunPos);
	SunPos_set_lat_long(S, latitude, longitude);
	bbox->user_data = (void *)S;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Prepared position for sun position.\n");
	return 0;
}

#define CALCPREPARE(NIN,NOUT) \
	/* a few checks about the input requirements */ \
	if(ninputs != NIN)return -1; \
	if(noutputs != NOUT)return -2; \
	if(inputs==NULL)return -3; \
	if(outputs==NULL)return -4; \
	if(bbox==NULL)return -5; \
	\
	/* the 'user_data' in the black box object will contain the */\
	/* coefficients required for this fluid; cast it to the required form: */\
	const SunPos *sunpos1 = (const SunPos *)bbox->user_data


/**
	Evaluation function for 'sunpos'
	@return 0 on success
*/
int sunpos_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(3,2);

	double t, p, T;

	t = inputs[0]/ 86400. - 0.5 /* convert seconds to days, offset such that 0:00 1 Jan 2003 --> -0.5 */;
	p = inputs[1] / 101325. /* convert Pa to atm */;
	T = inputs[2] - 273.15 /* convert °C to K */;

	SunPos S = *sunpos1;

	SunPos_set_press_temp(&S, p, T);
	/* we ignore differences between universal time and terrestrial time.
	it seems that these differences are < 1 sec in general
	http://en.wikipedia.org/wiki/DUT1 */
	SunPos_set_time(&S, t, 0);

	double zenith, azimuth;

	SunPos_calc_zen_azi(&S, &zenith, &azimuth);

	/* returned values are in Radians, no offsets required */
	outputs[0] = zenith;
	outputs[1] = azimuth;

	return 0;
}



