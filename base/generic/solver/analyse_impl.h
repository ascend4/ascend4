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
	Data structures used by the 'analyze.c' implementation. DO NOT include
	this file unless you are inside the implementation of problem analysis.
	(so, only analyse.c and diffvars.c, at this stage).
*/

#ifndef ASC_ANALYSE_IMPL_H
#define ASC_ANALYSE_IMPL_H

#include "slv_server.h"

/*-----------------------------------------------------------------------------
  DATA STRUCTURES FOR USE DURING PROBLEM ANALYSIS
*/

struct varip {
  struct var_variable *data;  /* ptr to destination of data */
  int index;		      /* master gl index */
  int incident;		      /* set 0 in classify_instance, 1 make_master_lists */
  int in_block;		      /* set 0 in classify_instance */
  int fixed;		      /* set in classify_instance */
  int solvervar;	      /* set in classify_instance */
  int active;             /* is this var a part of my problem */
  int basis;              /* set in classify_instance */

  int deriv;              /* set in classify_instance */
  int odeid;              /* value loaded from the ode_id child integer atom */
  int obsid;              /* value of obs_id from child integer atom */
};


struct disvarip {
  struct dis_discrete *data;	/* ptr to destination of data */
  int index;                    /* master gl index */
  int fixed;                    /* set in classify_instance */
  int isconst;                  /* is this dis var constant ? */
  int distype;                  /* 0 boolean, 1 int, -1 symbol */
  int value;                    /* integer value of the variable */
  int incident;                 /* is it incident in a logrel */
  int inwhen;                   /* is it in a when var list */
  int booleanvar;               /* Not sure if I need it */
  int active;                   /* is this disvar a part of my problem */
};


struct relip {
  struct rel_relation *data;    /* ptr to destination of data */
  long model;		/* relation is in this model in model gllist */
                    /* set in CollectRelsAndWhens. = 1.. nmodels */
                    /* rel_relation models = u.r.model-1 */
  int index; 		/* master gl list index */
  int obj;          /* is it an objective relation. set in classify_instance */
  int ext;	        /* is it e_blackbox. set in classify_instance */
  int included;     /* set in classify_instance */
  int cond;         /* is it a conditional relation. set in classify_instance */
  int inwhen;       /* is it in a when */
  int active;       /* is this rel a part of my problem */
};

struct logrelip {
  struct logrel_relation *data;    /* ptr to destination of data */
  long model;   /* logrelation is in this model in model gllist */
  int index;    /* master gllist index */
  int included; /* set in classify_instance */
  int cond;     /* is it a conditional logrelation.  */
  int inwhen;   /* is it in a when */
  int active;   /* is this logrel a part of my problem */
};

struct whenip{
  struct w_when *data;  /* ptr to destination of data */
  long model;           /* when is in this model in model gllist */
  int index;            /* master gllist index */
  int inwhen;           /* is it in a when */
};

struct modip {
  int index;        /* set in make master lists. 1..nmodels */
  int inwhen;       /* is it in a when */
};

/* we will decorate the ascend instance tree with these in the interface
	pointers, but ONLY for the system build process and not persistently.
	DO NOT EVER UNDER ANY CIRCUMSTANCES EXPORT THIS DATA STRUCTURE.
*/
struct solver_ipdata {
  struct Instance *i; /* the kind of instance is the enum for union */
  union {
    struct modip m;
    struct varip v;
    struct disvarip dv;
    struct relip r;
    struct logrelip lr;
    struct whenip w;
  } u;
};

/* a handy cast for fetching things off gllists */
#define SIP(x) ((struct solver_ipdata *)(x))

/*
	a bridge buffer used so much we aren't going to free it, just reuse it
*/
static struct reuse_t {
  size_t ipcap;			/* number of ips allocated in ipbuf */
  size_t ipused;		/* number of ips in use */
  struct solver_ipdata *ipbuf;
} g_reuse = {0,0,NULL};

/**
	a data structure for bridge-building only. hell of a scaffolding.
	all fields should be empty if construction is not in progress.
	In particular, do no operations that can throw an exception
	while manipulating a problem_t, as it is way too big to let leak.

	We are making the ANSI assumption that this will be init to 0/NULL 
	(K&R 2nd Ed, p 219)
	
	@TODO what about this one?:
	container for globals during assembly.
	At present, the mastervl and solvervl are of the same length. This
	is purely coincidental and the long run intent is that there is one
	master list and that a problem coordinator would set up the
	solver var/rel lists blockwise as we go along. We may want to put
	block information in the rel/var structures.
*/
struct problem_t {

  /* the following are established by CountStuffInTree */
  /** @TODO rename these for consistency with eg pars --> npars, vars --> nvars */
  long nv;              /* number of solvervar/solveratom */
  long np;              /* number of real ATOM instance parameters */
  long nu;              /* number of real ATOM instance uninteresting */
  long ndv;             /* number of discrete variables */
  long nud;             /* number of uninteresting discretes */
  long nc;              /* number of conditional relations */
  long ncl;              /* number of conditional logrelations */
  long nr;              /* number of algebraic relations */
  long no;              /* number of objective rels */
  long nl;              /* number of logical rels */
  long nw;              /* number of whens */
  long ne;              /* number of external rels subset overestimate*/
  long nm;              /* number of models */
  /*
  	The following gllists contain pointers to interface ptrs as
  	locally defined.
  	The lists will be in order found by a visit instance tree.
  */
  struct gl_list_t *vars;	/* solvervar/solveratom. varips */
  struct gl_list_t *pars;	/* real ATOM instance parameters */
  struct gl_list_t *unas;	/* real ATOM instance of no 'apparent' use */
  struct gl_list_t *models;	/* models in tree. modips */

  /*
  	The following gllists contain pointers to interface ptrs as
  	locally defined.
  	The lists will be in order found by running over the models list.
  */
  struct gl_list_t *dvars;	/* discrete variables */
  struct gl_list_t *dunas;	/* discrete variables of no use */
  struct gl_list_t *whens;	/* whens */
  struct gl_list_t *cnds;	/* conditional relations */
  struct gl_list_t *logcnds;	/* conditional logrelations */
  struct gl_list_t *rels;	/* ascend relations. relips */
  struct gl_list_t *objrels;	/* objective rels. relips */
  struct gl_list_t *logrels;	/* logical rels */

  struct gl_list_t *diffvars;  /* subset of vars: all vars with ode_id set!=0 */
  struct gl_list_t *algebvars; /* subset of vars: all vars with ode_id == 0 */
  struct gl_list_t *indepvars; /* subset of vars: all vars with ode_type == -1 */
  struct gl_list_t *obsvars; /* subset of vars: all vars with ode_type == -1 */

  /* bridge ip data */
  struct gl_list_t *oldips;	/* buffer of oldip crap we're protecting */

  /* misc stuff */
  struct gl_list_t *tmplist;	/* sort space */

  /* stuff that will end up in the slv_system_t */
  struct rel_relation *obj;	       /* DEFAULT objective relation, if any */
  struct Instance *root;	/* instance we construct system from */
  struct gl_list_t *extrels;	/* black box stub list */

  /* stuff that should move elsewhere, but end up in slv_system_t */
  mtx_region_t *blocks;		/* array of partitions in reordered matrix */
  int32 nblocks;		/* size of array of partitions */
  int nnz;	            /* free nonzeros in processed jacobian */
  int nnztot;			/* total nonzeros in processed relations */
  int nnzobj;			/* total nonzeros in objective gradients */
  int nnzcond;			/* total nonzeros in conditional relations */
  int relincsize;		/* total nonzeros in gradients */
  int relincinuse;		/* incidence given to relations so far */
  int varincsize;		/* total nonzeros in gradients (redundant) */
  int varincinuse;		/* incidence given to variables so far */
  int ncol;	            /* free and incident vars */
  int nrow;	            /* included relations */
  /* conditional stuff */
  int32 need_consistency;	/* Conistency analysis is required ? */

  /* logical relation stuff */
  int lognnz;           /* Summ of free boolean vars in inc logrels */
  int lognrow;          /* included logrelations */
  int logncol;          /* free and incident boolean vars */
  int lrelinc;          /* incident boolean vars */
  int lrelincsize;      /* Total summ of incidences (boolean vars)
                           in logrels*/
  int lrelincinuse;		/* incidence given to log relations so far */

  /* data to go to slv_system_t */
  /** @TODO rename these for consisttency, eg mastervl -> mastervars */
  struct rel_relation *reldata;      /* rel data space, mass allocated */
  struct rel_relation *objdata;      /* objrel data space, mass allocated */
  struct rel_relation *condata;      /* cond rel data space, mass allocated*/
  struct logrel_relation *lrdata;    /* logrel data space, mass allocated */
  struct logrel_relation *logcondata; /* cond logrel data space, allocated */
  struct var_variable *vardata;      /* svar data space, mass allocated */
  struct var_variable *pardata;      /* par data space, mass allocated */
  struct var_variable *undata;       /*  data space, mass allocated */
  struct dis_discrete *disdata;      /* dis var data space, mass allocated */
  struct dis_discrete *undisdata;    /*  data space, mass allocated */
  struct w_when *whendata;           /* when data space, mass allocated */
  struct bnd_boundary *bnddata;      /* boundaries data space, allocated */
  struct var_variable **mastervl;	/* master null-terminated list */
  struct var_variable **solvervl;	/* solvers null-terminated list */
  struct dis_discrete **masterdl;	/* master null-terminated list */
  struct dis_discrete **solverdl;	/* solvers null-terminated list */
  struct rel_relation **masterrl;	/* master null-terminated list */
  struct rel_relation **solverrl;	/* solvers null-terminated list */
  struct rel_relation **mastercl;	/* master null-terminated list */
  struct rel_relation **solvercl;	/* solvers null-terminated list */
  struct rel_relation **masterol;	/* master null-terminated list */
  struct rel_relation **solverol;	/* solvers null-terminated list */
  struct logrel_relation **masterll;	/* master null-terminated list */
  struct logrel_relation **solverll;	/* solvers null-terminated list */
  struct logrel_relation **mastercll;	/* master null-terminated list */
  struct logrel_relation **solvercll;	/* solvers null-terminated list */
  struct w_when **masterwl;     	/* master null-terminated list */
  struct w_when **solverwl;	        /* solvers null-terminated list */
  struct bnd_boundary **masterbl;     	/* master null-terminated list */
  struct bnd_boundary **solverbl;	/* solvers null-terminated list */
  struct var_variable **masterpl;	/* master null-terminated list */
  struct var_variable **solverpl;	/* solvers null-terminated list */
  struct var_variable **masterul;	/* master null-terminated list */
  struct var_variable **solverul;	/* solvers null-terminated list */
  struct dis_discrete **masterdul;	/* master null-terminated list */
  struct dis_discrete **solverdul;	/* solvers null-terminated list */
  struct var_variable **relincidence;	/* rel_relation incidence source */
  struct rel_relation **varincidence;	/* var_variable incidence source */
  struct dis_discrete **logrelinciden;	/* logrel_relation incidence source */
};

#endif
