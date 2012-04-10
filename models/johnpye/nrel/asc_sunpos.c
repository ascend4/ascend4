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
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//** @file
	Wrapper for sunpos_nrel.c to allow access to the calculation from ASCEND.
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
#include "spa.h"

#ifndef PI
# define PI 3.14159265358979
#endif

ExtBBoxInitFunc sunpos_nrel_prepare;
ExtBBoxFunc sunpos_nrel_calc;

static const char *sunpos_nrel_help = "Calculate sun position (local zenith, azimuth "
	"angles) given time, pressure and temperature, using NREL algorithm. DATA "
	"member for this external relation is required to provide constants for "
	"latitude and longitude of the selected location.\n\n"
	"Time is required to be in the form of Julian Date. ASCEND will convert the "
	"Julian Date into seconds automatically. The JD should be in the range"
	"-2000 BC to 6000 AD (12:00pm 1 Jan 2000 GMT is 2451545.0 JD)";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from 'IMPORT "johnpye/grena/sunpos";'

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int sunpos_nrel_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"SUNPOS is still EXPERIMENTAL. Use with caution.\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, sunpos_nrel_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFN(sunpos_nrel,3,2);

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
int sunpos_nrel_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *inst;
	double latitude, longitude, elevation;

	/* fetch DATA items for geographical location, timezone etc */
#define GET_CHILD_VAL(NAME) \
	inst = ChildByChar(data,AddSymbol(#NAME)); \
	if(!inst){ \
		ERROR_REPORTER_HERE(ASC_USER_ERROR \
			,"Couldn't locate '" #NAME "' in DATA, please check usage of SUNPOS."\
		);\
		return 1;\
	}\
	if(InstanceKind(inst)!=REAL_CONSTANT_INST){\
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"DATA member '" #NAME "' must be a real_constant");\
		return 1;\
	}\
	NAME = RC_INST(inst)->value;

	/* get the latitude */
	GET_CHILD_VAL(latitude);
	CONSOLE_DEBUG("Latitude: %0.3f",latitude);
	if(latitude > PI/2 || latitude < -PI/2){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'latitude' is out of allowable range -PI/2 to PI/2.");
		return 1;
	}

	/* get the longitude */
	GET_CHILD_VAL(longitude);
	CONSOLE_DEBUG("Longitude: %0.3f",longitude);
	if(longitude > PI || longitude < -PI){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'latitude' is out of allowable range -PI to PI.");
		return 1;
	}

	/* get the elevation */
	GET_CHILD_VAL(elevation);
	CONSOLE_DEBUG("Elevation: %0.3f m",elevation);
	if(elevation < -6500000){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"'elevation' is out of allowable range (must be > -6,500 km)");
		return 1;
	}

#undef GET_CHILD_VAL

	spa_data *S = ASC_NEW(spa_data);
	S->latitude = latitude * 180/PI;
	S->longitude = longitude * 180/PI;
	S->elevation = elevation;
	S->function = SPA_ZA_JD;

	ERROR_REPORTER_HERE(ASC_PROG_NOTE,"Prepared position for sun position.\n");
	bbox->user_data = (void *)S;
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
	const spa_data *sunpos1 = (const spa_data *)bbox->user_data


/**
	Evaluation function for 'sunpos'
	@return 0 on success
*/
int sunpos_nrel_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(3,2);

	double t, p, T;

	t = inputs[0]/ 86400.; /* convert from JD seconds to JD days */
	p = inputs[1] / 100. /* convert Pa to mbar */;
	T = inputs[2] - 273.15 /* convert °C to K */;

	spa_data S = *sunpos1;
	S.pressure = p;
	S.temperature = T;
	S.jd = t;

	int res = spa_calculate(&S);
	CONSOLE_DEBUG("Sun position: t = %f JD, p  %f mbar, T = %f C: res = %d, az = %f, zen = %f",t, p, T, res, S.azimuth, S.zenith);

	/* returned values are in degrees, need to convert back to base SI: radians */
	outputs[0] = S.zenith * PI/180.;
	outputs[1] = S.azimuth * PI/180.;

	/* 0 on success, non-zero is error code from spa_calculate (would prob be input parameters out-of-range) */
	return res;
}



