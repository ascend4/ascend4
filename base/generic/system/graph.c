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

#ifdef HAVE_GRAPHVIZ_BOOLEAN
# define HAVE_BOOLEAN
#endif
#include <utilities/ascConfig.h>

#ifdef WITH_GRAPHVIZ
# ifdef __WIN32__
#  include <gvc.h>
# else
#  include <graphviz/gvc.h>
# endif
# define HAVE_BOOLEAN
#endif

boolean X;

#include "graph.h"
#include "slv_client.h"
#include "incidence.h"
#include <utilities/ascMalloc.h>
#include <utilities/ascPanic.h>

int system_write_graph(slv_system_t sys
	, FILE *fp
	, const char *format
){
	incidence_vars_t id;
	build_incidence_data(sys, &id);

#ifdef WITH_GRAPHVIZ
	Agraph_t *g;
	GVC_t *gvc;

	unsigned edgecount = 0;
	unsigned nodecount = 0;

	gvc = gvContext();
	g = agopen("g",AGDIGRAPH);
	agnodeattr(g,"shape","ellipse");
	agnodeattr(g,"label","");
	agnodeattr(g,"color","");
	agnodeattr(g,"style","");

	char temp[200];

	/* first create nodes for the relations */
	unsigned i;
	Agnode_t *n, *m;
	for(i=0; i < id.neqn; ++i){
		char *relname;
		relname = rel_make_name(sys,id.rlist[i]);
		sprintf(temp,"r%d",rel_sindex(id.rlist[i]));
		n = agnode(g,temp);
		agset(n,"label",relname);
		if(rel_satisfied(id.rlist[i])){
			agset(n,"style","filled");
			agset(n,"color","blue");
		}
		ASC_FREE(relname);
		nodecount++;
	}

	/* now create nodes for the variables */
	unsigned j;
	for(j=0; j < id.nvar; ++j){
		char *varname;
		varname = var_make_name(sys,id.vlist[j]);
		sprintf(temp,"v%d",var_sindex(id.vlist[j]));
		n = agnode(g,temp);
		agset(n,"label",varname);
		agset(n, "shape", "box");
		if(var_fixed(id.vlist[j])){
			CONSOLE_DEBUG("VAR '%s' IS FIXED",varname);
			agset(n,"style","filled");
			agset(n,"color","green");
		}
		if(!var_active(id.vlist[j])){
			CONSOLE_DEBUG("VAR '%s' IS FIXED",varname);
			agset(n,"style","filled");
			agset(n,"color","gray");
		}
		ASC_FREE(varname);
		nodecount++;
	}

	/* now create edges */
	const struct var_variable **ivars;
	unsigned niv;
	char reltemp[200];
	struct Agedge_t *e;
	for(i=0; i < id.nprow; ++i){
		ivars = rel_incidence_list(id.rlist[i]);
		niv = rel_n_incidences(id.rlist[i]);
		sprintf(reltemp,"r%d",rel_sindex(id.rlist[i]));
		char *relname;
		relname = rel_make_name(sys,id.rlist[i]);
		CONSOLE_DEBUG("rel = '%s'",relname);
		ASC_FREE(relname);
		for(j=0; j < niv; ++j){
			const struct var_variable *v;
			v = ivars[j];
			sprintf(temp,"v%d",var_sindex(v));
			n = agnode(g, reltemp);
			m = agnode(g, temp);

			if(id.v2pc[var_sindex(v)]==id.e2pr[rel_sindex(id.rlist[i])]){
				e = agedge(g,n,m); /* from rel to var */
			}else{
				e = agedge(g,m,n); /* from var to rel */
			}
			edgecount++;
		}
	}

	if(nodecount > 300 || edgecount > 300){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Graph is too complex, will not launch GraphViz (%d nodes, %d edges)", nodecount, edgecount);
		return 1;
	}

	gvLayout(gvc, g, "dot");
	gvRender(gvc, g, (char*)format, fp);

#else
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Function system_write_graph not available (GraphViz not present at build-time)");
	return 1; /* error */
#endif

#if 0
	int nr,nsr,nv,nsv,niv;
	int i,j;
	struct rel_relation **srels;
	struct var_variable **svars, **ivars;
	char *relname, *varname;

	CONSOLE_DEBUG("Writing graph...");
	asc_assert(fp!=NULL);

	CONSOLE_DEBUG("FP = %p",fp);
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
			if(var_fixed(svars[j])){
				fprintf(fp,"s\tv%d[label=\"%s\",style=filled,color=green]\n",j,varname);
			}else{
				fprintf(fp,"\tv%d[label=\"%s\"]\n",j,varname);
			}
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
			if(j==i){
				fprintf(fp,"\tr%d->v%d\n",i,var_sindex(ivars[j]));
			}else{
				fprintf(fp,"\tv%d->r%d\n",var_sindex(ivars[j]),i);
			}
		}
	}

	fprintf(fp,"}\n");
#endif
	CONSOLE_DEBUG("Completed graph output");
	return 0;
}




