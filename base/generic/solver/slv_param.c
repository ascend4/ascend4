/*	ASCEND modelling environment
	Copyright (C) 1990 Karl Michael Westerberg
	Copyright (C) 1993 Joseph Zaher
	Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
	Copyright (C) 1996 Benjamin Andrew Allan
	Copyright (C) 2005-2007 Carnegie Mellon University

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
*/

#include "slv_param.h"

#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

void slv_set_char_parameter(char **cp, CONST char *newvalue)
{
  if (cp != NULL) {
    if (*cp != NULL) {
      ascfree(*cp);
    }
    *cp = ASC_STRDUP(newvalue);
  }
}

void slv_destroy_parms(slv_parameters_t *p) {
  int32 i/*, j */;
  for(i = 0; i < p->num_parms; i++){
    switch(p->parms[i].type) {
    case char_parm:
      /* ASC_FREE(p->parms[i].info.c.value);
      for (j = 0; j < p->parms[i].info.c.high; j++) {
        ASC_FREE(p->parms[i].info.c.argv[j]);
      }*/
      ASC_FREE(p->parms[i].info.c.argv);
      /* FALL THROUGH */
    case int_parm:
    case bool_parm:
    case real_parm:
      ASC_FREE(p->parms[i].name);
      ASC_FREE(p->parms[i].interface_label);
      ASC_FREE(p->parms[i].description);
      break;
    default:
      ERROR_REPORTER_NOLINE(ASC_PROG_WARNING,"Unrecognized parameter type in slv_destroy_parms.");
    }
  }
  if(p->parms && p->dynamic_parms){
    ASC_FREE(p->parms);
	p->parms = NULL;
  }
  CONSOLE_DEBUG("Destroyed slv_parameters_t");
}

/*------------------------------------------------------------------------------
	IMPROVED (says I) FUNCTIONS FOR DECLARING SOLVER PARAMETERS -- JP
*/
/** @page solver-parameters

	Additional info on new solver parameter routines. This routine attempts 
	to make declaration of new parameters possible with simple syntax, without
	requiring changes to the underlying data structure. Also aim to eliminate
	the extensive #defines used in the old approach, and eliminate the risk of
	messing up the parameter list by forgetting to update something.

	Usage:
		1. declare IDs for the parameters you'll be using via an 'enum'
			(last ID is XXXX_PARAMS_COUNT)
		2. allocate space for your slv_parameters_t::parms of size XXXX_PARAMS_COUNT
		3. for each parameter, call slv_param_* as follows:

			slv_param_int(p,XXXX_PARAM_NAME,(SlvParameterInitInt){
				{"codename","guiname",3 (==guipagenum) "description"}
				,1 (==default value) ,0 (==min), 100 (==max)
			});

		4. to access a value from your code, use SLV_PARAM_BOOL(p,XXX_PARAM_NAME) etc
			(as defined in slv_common.h)

	See example stuff in ida.c
*/

static void slv_define_param_meta(struct slv_parameter *p1, const SlvParameterInitMeta *meta, const int index){
	/* copy the codename, guiname and description */
	asc_assert(meta!=NULL);
	asc_assert(p1!=NULL);
	p1->name = ASC_STRDUP(meta->codename);
	p1->interface_label = ASC_STRDUP(meta->guiname);
	p1->description = ASC_STRDUP(meta->description);
	p1->display = meta->guipagenum;

	/* record the index of this parameter */		
	p1->number = index;
}

int slv_param_int(slv_parameters_t *p, const int index
	,const SlvParameterInitInt init
){
	struct slv_parameter *p1;
	if(p == NULL)return -1;
	p1 = &(p->parms[index]);

	p1->type = int_parm;
	p1->info.i.value = init.val;
	p1->info.i.low = init.low;
	p1->info.i.high = init.high;

	slv_define_param_meta(p1, &(init.meta), index);
	return ++(p->num_parms);
}

int slv_param_bool(slv_parameters_t *p, const int index
	,const SlvParameterInitBool init
){
	struct slv_parameter *p1;
	if(p == NULL)return -1;
	p1 = &(p->parms[index]);

	p1->type = bool_parm;
	p1->info.b.value = init.val;
	p1->info.b.low = 0;
	p1->info.b.high = 1;

	slv_define_param_meta(p1, &(init.meta), index);
	return ++(p->num_parms);
}

int slv_param_real(slv_parameters_t *p, const int index
	,const SlvParameterInitReal init
){
	struct slv_parameter *p1;

	if(p == NULL)return -1;
	p1 = &(p->parms[index]);

	p1->type = real_parm;
	p1->info.r.value = init.val;
	p1->info.r.low = init.low;
	p1->info.r.high = init.high;

	slv_define_param_meta(p1, &(init.meta), index);
	return ++(p->num_parms);
}

int slv_param_char(slv_parameters_t *p, const int index
	,const SlvParameterInitChar init
	,char *options[]
){
	int i, noptions;
	struct slv_parameter *p1;
	if(p == NULL)return -1;
	p1 = &(p->parms[index]);
	p1->type = char_parm;

	/* find the length by hunting for the NULL at the end */
	for(i=0; options[i]!=NULL; ++i){
		/* CONSOLE_DEBUG("FOUND init.options[%d]='%s'",i,options[i]); */
	}
	noptions = i;
	/* CONSOLE_DEBUG("THERE ARE %d CHAR OPTIONS IN PARAMETER '%s'", noptions, init.meta.codename); */

	p1->info.c.high = noptions;
	p1->info.c.value = ASC_STRDUP(init.val);
	p1->info.c.argv = ASC_NEW_ARRAY(char *,noptions);

	for(i = 0; i < noptions; ++i){
	    p1->info.c.argv[i] = strdup(options[i]);
		/* CONSOLE_DEBUG("Copied '%s' --> argv[%d] = '%s'",options[i],i,p1->info.c.argv[i]); */
	}

	slv_define_param_meta(p1, &(init.meta), index);
	return ++(p->num_parms);
}

int32 slv_define_parm(slv_parameters_t *p,
		   enum parm_type type,
		   char *name,
		   char *interface_label,
		   char *description,
		   union parm_arg value,
		   union parm_arg low,
		   union parm_arg high,
		   int32 display)
{
  int32 len,length,i, err=1;
  if (p == NULL) {
    return -1;
  }
  length = p->num_parms;

  switch (type) {
  case int_parm:
    err = 0;
    p->parms[length].info.i.value = value.argi;
    p->parms[length].info.i.low = low.argi;
    p->parms[length].info.i.high = high.argi;
    break;

  case bool_parm:
    err = 0;
    p->parms[length].info.b.value = value.argb;
    p->parms[length].info.b.low = low.argb;
    p->parms[length].info.b.high = high.argb;
    break;

  case real_parm:
    err = 0;
    p->parms[length].info.r.value = value.argr;
    p->parms[length].info.r.low = low.argr;
    p->parms[length].info.r.high = high.argr;
    break;

  case char_parm:
    err = 0;
    p->parms[length].info.c.argv =
      (char **)ascmalloc(high.argi*sizeof(char *));
    for (i = 0; i < high.argi; i++) {
      len = strlen(low.argv[i]);
      p->parms[length].info.c.argv[i] =ASC_NEW_ARRAY(char,len+1);
      strcpy(p->parms[length].info.c.argv[i],low.argv[i]);
    }

    p->parms[length].info.c.value =
      (char *)ascmalloc(strlen(value.argc)+1*sizeof(char));
    strcpy(p->parms[length].info.c.value,value.argc);

    p->parms[length].info.c.high = high.argi;
    break;

  default:
    return -1;
  }
  if (!err) {
    p->parms[length].type = type;
    p->parms[length].number = length;

    len = strlen(name);
    p->parms[length].name = ASC_NEW_ARRAY(char,len+1);
    strcpy(p->parms[length].name,name);

    len = strlen(interface_label);
    p->parms[length].interface_label = ASC_NEW_ARRAY(char,len+1);
    strcpy(p->parms[length].interface_label,interface_label);

    len = strlen(description);
    p->parms[length].description = ASC_NEW_ARRAY(char,len+1);
    strcpy(p->parms[length].description,description);

    p->parms[length].display = display;
  } else {
    p->parms[length].type = -1;
  }
  p->num_parms++;
  return p->num_parms;
}

