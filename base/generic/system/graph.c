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
*//** @file
	System graph output

	Use variable and relation filters to generate a graph between the selected
	vars and rels, by querying their incidence data. Write the graph in DOT
	format to the provided FILE*.

	by John Pye, March 2007
*/
#include "graph.h"
#include "slv_client.h"
#include <utilities/ascMalloc.h>

int system_write_graph(slv_system_t sys
	, FILE *fp
	, const rel_filter_t *rfilter, const var_filter_t *vfilter
){
	int nr,nsr,nv,nsv,niv;
	int i,j;
	struct rel_relation **srels;
	struct var_variable **svars, **ivars;
	char *relname, *varname;

	fprintf(fp,"digraph G{\n");

	/* first create nodes for the rels */
	nr = slv_count_solvers_rels(sys,rfilter);
	nsr = slv_get_num_solvers_rels(sys);
	srels = slv_get_solvers_rel_list(sys);
	fprintf(fp,"\n\n\t/* %d relations */\n\n",nr);
	for(i=0; i<nsr; ++i){
		if(rel_apply_filter(srels[i],rfilter)){
			relname = rel_make_name(sys,srels[i]);
			fprintf(fp,"\tr%d[shape=box,label=\"%s\"]\n",i,relname);
			ASC_FREE(relname);
		}
	}

	/* and the vars */
	nsv = slv_get_num_solvers_vars(sys);
	nv = slv_count_solvers_vars(sys,vfilter);
	svars = slv_get_solvers_var_list(sys);
	fprintf(fp,"\n\n\t/* %d variables */\n\n",nv);
	for(j=0; j<nsv; ++j){
		if(var_apply_filter(svars[j],vfilter)){
			varname = var_make_name(sys,svars[j]);
			fprintf(fp,"\tv%d[label=\"%s\"]\n",j,varname);
			ASC_FREE(varname);
		}
	}

	/* now output the edges between them */
	fprintf(fp,"\n\n\t/* incidences */\n\n");
	for(i=0; i<nsr; ++i){
		if(!rel_apply_filter(srels[i],rfilter))continue;
		
		ivars = rel_incidence_list(srels[i]);
		niv = rel_n_incidences(srels[i]);

		for(j=0; j<niv; ++j){
			if(!var_apply_filter(ivars[j],vfilter))continue;

			fprintf(fp,"\tv%d->r%d\n",var_sindex(ivars[j]),i);
		}
	}

	fprintf(fp,"}\n");

	CONSOLE_DEBUG("Completed graph output");
}
		

	

