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

static ExtBBoxInitFunc sunpos_nrel_prepare;
static ExtBBoxFunc sunpos_nrel_calc;
static ExtBBoxInitFunc julian_day_nrel_prepare;
static ExtBBoxFunc julian_day_nrel_calc;

static const char *sunpos_nrel_help = "\
Calculate sun position using NREL SPA code. Inputs are:\n\
  * time (relative to reference time)\n\
  * pressure (instantaneous atmospheric pressure)\n\
  * temperature (instantaneous absolute atmospheric temperature)\n\
  * reference time (Julian Day value expressed as seconds)\n\
The reference time allows this function to use the same time variable as the\
rest of your simulation; the reference time is expected to be pre-calculated\
from a year-month-day calculation (see 'julian_day_nrel' external relation).";

static const char *julian_day_nrel_help = "Calculate the Julian Day from "
	"year, month, day, hour, minute, second and timezone inputs. "
	"Intended for once-off use in ASCEND models to calculate the time offset "
	"eg for the start of a weather file. Acceptable dates are in the range "
	"of -2000 BC to AD 6000. All of the inputs should be as 'factor' type "
	"variables (to avoid needless time unit conversions), except for the "
	"timezone, which should be in time units eg '8{h}'.";

/*------------------------------------------------------------------------------
  REGISTRATION FUNCTION
*/

/**
	This is the function called from 'IMPORT "johnpye/nrel/sunpos_nrels";'

	It sets up the functions contained in this external library
*/
extern
ASC_EXPORT int sunpos_nrel_register(){
	int result = 0;

	ERROR_REPORTER_HERE(ASC_USER_WARNING,"SUNPOS_NREL is still EXPERIMENTAL. Use with caution.\n");

#define CALCFN(NAME,INPUTS,OUTPUTS) \
	result += CreateUserFunctionBlackBox(#NAME \
		, NAME##_prepare \
		, NAME##_calc /* value */ \
		, (ExtBBoxFunc*)NULL /* derivatives not provided yet*/ \
		, (ExtBBoxFunc*)NULL /* hessian not provided yet */ \
		, (ExtBBoxFinalFunc*)NULL /* finalisation not implemented */ \
		, INPUTS,OUTPUTS /* inputs, outputs */ \
		, NAME##_help /* help text */ \
		, 0.0 \
	) /* returns 0 on success */

	CALCFN(sunpos_nrel,4,2);
	CALCFN(julian_day_nrel,7,1);

#undef CALCFN

	if(result){
		ERROR_REPORTER_HERE(ASC_PROG_NOTE,"CreateUserFunction result = %d\n",result);
	}
	return result;
}

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

/**
	This function is called when the black-box relation is being instantiated.

	This just gets the data member and checks that it's valid, and stores
	it in the blackbox data field.
*/
static int sunpos_nrel_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	struct Instance *inst;
	double latitude, longitude, elevation;

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
static int sunpos_nrel_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(4,2);

	double t, p, T, t_offset;

	t = inputs[0]; /* convert from JD seconds to JD days */
	p = inputs[1] / 100. /* convert Pa to mbar */;
	T = inputs[2] - 273.15 /* convert °C to K */;
	t_offset = inputs[3];

	spa_data S = *sunpos1;
	S.pressure = p;
	S.temperature = T;
	S.jd = (t + t_offset) / 3600 / 24; /* convert to days */

	int res = spa_calculate(&S);
	CONSOLE_DEBUG("Sun position: t = %f JD, p  %f mbar, T = %f C: res = %d, az = %f, zen = %f",t, p, T, res, S.azimuth, S.zenith);

	/* returned values are in degrees, need to convert back to base SI: radians */
	outputs[0] = S.zenith * PI/180.;
	outputs[1] = S.azimuth * PI/180.;

	/* 0 on success, non-zero is error code from spa_calculate (would prob be input parameters out-of-range) */
	return res;
}

/*---------- SUNPOS_JULIAN_DAY ------------*/

static int julian_day_nrel_prepare(struct BBoxInterp *bbox,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	bbox->user_data = NULL;
	return 0;
}

static int julian_day_nrel_calc(struct BBoxInterp *bbox,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	CALCPREPARE(7,1);
	(void)sunpos1;

	int y,mon,d,h,m,s;
	double tz;

	y = inputs[0]; /* convert from seconds to years */
	mon = inputs[1]; /* convert from seconds to months */ 
	d = inputs[2]; /* convert from seconds to days */
	h = inputs[3]; /* seconds to hours */
	m = inputs[4]; /* seconds to minutes */
	s = inputs[5];
	tz = inputs[6] / 3600.; /* seconds to hours */

	double t = julian_day(y,mon,d, h,m,s, tz) * 3600 * 24;
	
	outputs[0] = t;
	return 0;
}


