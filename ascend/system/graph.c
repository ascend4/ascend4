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
#include <ascend/general/platform.h>

#ifdef WITH_GRAPHVIZ
# include <gvc.h>
#ifdef WITH_CGRAPH
# include <cgraph.h>
#endif
#endif

#define ASC_GV_LIBNAME "libgvc.so"
#ifdef WITH_CGRAPH
# define ASC_CGRAPH_LIBNAME "libcgraph.so"
#else
# define ASC_CGRAPH_LIBNAME ASC_GV_LIBNAME
#endif

#ifdef WITH_CGRAPH
# error "Not ready for this yet"
#endif

#include "./graph.h"
#include "slv_client.h"
#include "incidence.h"
#include <ascend/general/ascMalloc.h>
#include <ascend/general/panic.h>
#include <ascend/utilities/ascDynaLoad.h>

int system_write_graph(slv_system_t sys
	, FILE *fp
	, const char *format
){
#ifdef WITH_GRAPHVIZ
	incidence_vars_t id;
	build_incidence_data(sys, &id);

	int res;

	/* first create nodes for the relations */
	unsigned i;
	Agnode_t *n, *m;
	void (*ags) (Agnode_t* , char* , char*);

	Agraph_t *g;

	unsigned edgecount = 0;
	unsigned nodecount = 0;

	GVC_t *gvc;

	const struct var_variable **ivars;
	unsigned niv;
	char reltemp[200];
	//Agedge_t *e;

	char temp[200];

	/* dynamic loading of stuff from the graphviz libraries... */
#ifdef WITH_CGRAPH
	/* gvc */
	GVC_t *(*gvConte)(void);
	void (*gvLayo)(GVC_t* , graph_t*, const char*engine);
	void (*gvRend)(GVC_t* , graph_t*, const char*format, FILE*out);
	/* cgraph */
	Agdesc_t Agdirected = { 1, 0, 0, 1};
	Agraph_t *(*agop)(char* , Agdesc_t type, Agdisc_t *disc);
	Agsym_t *(*agatt)(Agraph_t *g, int kind, char *name, char *value);
	Agnode_t *(*agno)(Agraph_t *g, char *name, int cflag);
	Agedge_t *(*aged)(Agraph_t *g, Agnode_t *t, Agnode_t *h, char *name,int cflag);
# define ASC_AGNODE(GRAPH,NAME) (*agno)(GRAPH,NAME,1)
# define ASC_AGEDGE(GRAPH,N1,N2) (*aged)(GRAPH,N1,N2,NULL,1)
#else // agraph
	GVC_t *(*gvConte)();
	Agnode_t *(*agno)(Agraph_t* , char*);
	Agraph_t *(*agop)(char* , int);
	void (*agatt) (Agraph_t* , char* , char*);
	Agedge_t *(*aged)(Agraph_t* , Agnode_t*, Agnode_t*);
	void (*gvLayo)(GVC_t* , Agraph_t*, char*);
	void (*gvRend)(GVC_t* , Agraph_t*, char*, FILE*);
# define ASC_AGNODE(GRAPH,NAME) (*agno)(GRAPH,NAME)
# define ASC_AGEDGE(GRAPH,N1,N2) (*aged)(GRAPH,N1,N2)
#endif

	res = Asc_DynamicLoad(ASC_GV_LIBNAME,NULL);
	if(res){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Unable to load GraphViz (gvc) on your system. Is it installed?");
		return 1;
	}
#ifdef WITH_CGRAPH
	res = Asc_DynamicLoad(ASC_CGRAPH_LIBNAME,NULL);
	if(res){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Unable to access GraphViz (cgraph) on your system. Is it installed?");
		return 1;
	}
#endif

	*(void **) (&gvConte) = Asc_DynamicFunction(ASC_GV_LIBNAME,"gvContext");
	*(void **) (&gvLayo) = Asc_DynamicFunction(ASC_GV_LIBNAME,"gvLayout");
	*(void **) (&gvRend) = Asc_DynamicFunction(ASC_GV_LIBNAME,"gvRender");
	*(void **) (&agop) = Asc_DynamicFunction(ASC_CGRAPH_LIBNAME,"agopen");
#ifdef WITH_CGRAPH
	*(void **) (&agatt) = Asc_DynamicFunction(ASC_CGRAPH_LIBNAME,"agattr");
#else
	*(void **) (&agatt) = Asc_DynamicFunction(ASC_GV_LIBNAME,"agnodeattr");
#endif

	*(void **) (&agno) = Asc_DynamicFunction(ASC_CGRAPH_LIBNAME,"agnode");
	*(void **) (&ags) = Asc_DynamicFunction(ASC_CGRAPH_LIBNAME,"agset");
	*(void **) (&aged) = Asc_DynamicFunction(ASC_CGRAPH_LIBNAME,"agedge");

	if(!gvConte || !gvLayo || !gvRend || !agop || !agatt | !agno | !ags || !aged){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,"Unable to access find required "
			"functions in Graphviz dynamically-loaded library/ies Do you have the "
			"correct version installed?"
		);
		return 1;
	}

	CONSOLE_DEBUG("Dynamically loaded GraphViz OK");

	/* create the graph and its style details */
	gvc = (*gvConte)();
#ifdef WITH_CGRAPH
	g = (*agop)("g",Agdirected,NULL);
	(*agatt)(g,AGNODE,"shape","ellipse");
	(*agatt)(g,AGNODE,"label","");
	(*agatt)(g,AGNODE,"color","");
	(*agatt)(g,AGNODE,"style","");
#else
	g = (*agop)("g",AGDIGRAPH);
	(*agatt)(g,"shape","ellipse");
	(*agatt)(g,"label","");
	(*agatt)(g,"color","");
	(*agatt)(g,"style","");
#endif

	/* create notes for the relations */
	for(i=0; i < id.neqn; ++i){
		char *relname;
		relname = rel_make_name(sys,id.rlist[i]);
		sprintf(temp,"r%d",rel_sindex(id.rlist[i]));
		n = ASC_AGNODE(g,temp);
		(*ags)(n,"label",relname);
		if(rel_satisfied(id.rlist[i])){
			(*ags)(n,"style","filled");
			(*ags)(n,"color","blue");
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
		n = ASC_AGNODE(g,temp);
		(*ags)(n,"label",varname);
		(*ags)(n, "shape", "box");
		if(var_fixed(id.vlist[j])){
			CONSOLE_DEBUG("VAR '%s' IS FIXED",varname);
			(*ags)(n,"style","filled");
			(*ags)(n,"color","green");
		}
		if(!var_active(id.vlist[j])){
			CONSOLE_DEBUG("VAR '%s' IS FIXED",varname);
			(*ags)(n,"style","filled");
			(*ags)(n,"color","gray");
		}
		ASC_FREE(varname);
		nodecount++;
	}

	/* now create edges */
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
			n = ASC_AGNODE(g, reltemp);
			m = ASC_AGNODE(g, temp);

			if(id.v2pc[var_sindex(v)]==id.e2pr[rel_sindex(id.rlist[i])]){
				ASC_AGEDGE(g,n,m); /* from rel to var */
			}else{
				ASC_AGEDGE(g,m,n); /* from var to rel */
			}
			edgecount++;
		}
	}

	/* we won't try to plot it if it's way too complex */

	if(nodecount > 300 || edgecount > 300){
		ERROR_REPORTER_HERE(ASC_USER_ERROR,"Graph is too complex, will not launch GraphViz (%d nodes, %d edges)", nodecount, edgecount);
		return 1;
	}

	(*gvLayo)(gvc, g, "dot");
	(*gvRend)(gvc, g, (char*)format, fp);

	//printf("\nErrors encountered %s\n",(*dlerror)());

	Asc_DynamicUnLoad(ASC_GV_LIBNAME);

	CONSOLE_DEBUG("Completed graph output");
	return 0;

#else
	ERROR_REPORTER_HERE(ASC_PROG_ERR,"Function system_write_graph not available (GraphViz not present at build-time)");
	return 1;
#endif
}




