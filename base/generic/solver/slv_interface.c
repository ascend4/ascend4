/*
 *  SLV: Ascend Numeric Solver
 *  by Karl Michael Westerberg
 *  Created: 2/6/90
 *  Version: $Revision: 1.23 $
 *  Version control file: $RCSfile: slv_interface.c,v $
 *  Date last modified: $Date: 1998/01/11 17:19:09 $
 *  Last modified by: $Author: ballan $
 *
 *  This file is part of the SLV solver.
 *
 *  Copyright (C) 1990 Karl Michael Westerberg, Thomas Guthrie Epperly
 *  Copyright (C) 1993 Joseph Zaher
 *  Copyright (C) 1994 Joseph Zaher, Benjamin Andrew Allan
 *
 *  The SLV solver is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  The SLV solver is distributed in hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the program; if not, write to the Free Software Foundation, Inc., 675
 *  Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
 *  COPYING is found in ../compiler.
 */

/**
 ***  This file defines the function Solve(struct Instance *inst), which is
 ***  swiped from solver/chris.h.  This module will instead use Karl's
 ***  solver. (was newascend/compiler/chris.h on wetterhorn)
 ***  much of slv_interface.h is not yet implemented for SLV.
 **/

#include "utilities/ascConfig.h"
#include "utilities/ascSignal.h"
#include "compiler/instance_enum.h"
#include "compiler/fractions.h"
#include "compiler/compiler.h"
#include "utilities/ascMalloc.h"
#include "compiler/dimen.h"
#include "utilities/readln.h"
#include "solver/mtx.h"
#include "solver/slv_types.h"
#include "solver/var.h"
#include "solver/rel.h"
#include "solver/discrete.h"
#include "solver/conditional.h"
#include "solver/logrel.h"
#include "solver/bnd.h"
#include "solver/calc.h"
#include "solver/relman.h"
#include "solver/slv_common.h"
#include "solver/linsol.h"
#include "solver/linsolqr.h"
#include "solver/slv_client.h" /* should be client ... */
#include "solver/system.h"
#include "solver/slv_interface.h"   /* Implementation of this */
#include "solver/checkdim.h"
#include "compiler/plot.h"

#define prompt printf

#define yorn(b) ((b) ? "YES" : "NO")
#define torf(b) ((b) ? "TRUE" : "FALSE")

#define SAFE_FIX_ME 0

#define C_NOP                    0
#define C_RETURN                 1
#define C_HELP                   2
#define C_INPUT_PARAMETERS       3
#define C_OUTPUT_PARAMETERS      4
#define C_OUTPUT_STATUS          5
#define C_COUNT                  6
#define C_COUNT_IN_BLOCK         7
#define C_WRITE_VAR              8
#define C_WRITE_VARS             9
#define C_WRITE_VARS_IN_BLOCK   10
#define C_TOGGLE_SUPPRESS_REL   11
#define C_WRITE_REL             12
#define C_WRITE_RELS            13
#define C_WRITE_RELS_IN_BLOCK   14
#define C_WRITE_OBJ             15
#define C_CHECK_DIMENSIONS      16
#define C_ELIGIBLE_SOLVERS      17
#define C_SELECT_SOLVER         18
#define C_SELECTED_SOLVER       19
#define C_PRESOLVE              20
#define C_RESOLVE               21
#define C_ITERATE               22
#define C_SOLVE                 23
#define C_DUMP_SYSTEM           25
#define C_PLOT                  26

static struct command_list_t {
   char *name;
   char *descr;
   int num;
} commands[] = {
   { "", "", C_NOP } ,
   { "return", "Returns to ascend", C_RETURN } ,
   { "quit", "Returns to ascend", C_RETURN } ,
   { "bye", "Returns to ascend", C_RETURN } ,
   { "?", "Prints out this list", C_HELP } ,
   { "help", "Prints out this list", C_HELP } ,
   { "input parameters", "Input solver parameters", C_INPUT_PARAMETERS } ,
   { "output parameters", "Output solver parameters", C_OUTPUT_PARAMETERS } ,
   { "output status", "Output solver status", C_OUTPUT_STATUS } ,
   { "count", "Counts variables/relations", C_COUNT } ,
   { "count in block", "Counts variables/relations in block",
	C_COUNT_IN_BLOCK } ,
   { "write var" , "Writes one variable" , C_WRITE_VAR } ,
   { "write vars" , "Writes complete variable list" , C_WRITE_VARS } ,
   { "write vars in block" , "Writes variables in current block" ,
	C_WRITE_VARS_IN_BLOCK } ,
   { "toggle suppress rel" , "Toggles the suppress-relation-flag" ,
	C_TOGGLE_SUPPRESS_REL } ,
   { "write rel" , "Writes one relation" , C_WRITE_REL } ,
   { "write rels" , "Writes complete relation list" , C_WRITE_RELS } ,
   { "write rels in block" , "Writes relations in current block" ,
	C_WRITE_RELS_IN_BLOCK } ,
   { "write obj" , "Writes objective function" , C_WRITE_OBJ } ,
   { "check dimensions" , "Checks global dimensional consistency" ,
	C_CHECK_DIMENSIONS } ,
   { "eligible solvers" , "Indicates which solvers can solve the problem" ,
	C_ELIGIBLE_SOLVERS } ,
   { "select solver" , "Allows user to select a solver" , C_SELECT_SOLVER } ,
   { "selected solver" , "Indicates which solver has been selected" ,
	C_SELECTED_SOLVER } ,
   { "presolve" , "Pre-solves system" , C_PRESOLVE } ,
   { "resolve" , "Re-solves system" , C_RESOLVE } ,
   { "iterate" , "Perform one iteration" , C_ITERATE } ,
   { "solve" , "Attempts to solve entire system" , C_SOLVE } ,
   { "dump system" , "Dump solve system to a file" , C_DUMP_SYSTEM } ,
   { "plot" , "Plots the solve instance" , C_PLOT }
};
#define NCOMMANDS array_length(commands)

static void print_commands(void)
/**
 ***  Prints out all commands
 **/
{
   unsigned int ndx;
   for( ndx=0 ; ndx < NCOMMANDS ; ++ndx )
      PRINTF("%-30s%s\n",commands[ndx].name,commands[ndx].descr);
}

static int command_number(char *str)
/**
 ***  Returns number corresponding to command string.
 ***  If non-existent, -1 is returned.
 **/
{
   unsigned int i;
   for( i=0 ; i<NCOMMANDS ; ++i )
      if( strcmp(commands[i].name , str) == 0 )
         break; /* found it */
   return( i < NCOMMANDS ? commands[i].num : -1 );
}

static boolean input_boolean(boolean def)
/**
 ***  Inputs a boolean (true or false)
 **/
{
   char buf[10];
   readln(buf,sizeof(buf));
   switch( buf[0] ) {
      case 'y':
      case 'Y':
      case 't':
      case 'T':
         return(TRUE);
      case 'n':
      case 'N':
      case 'f':
      case 'F':
         return(FALSE);
      default:
         return(def);
   }
}

static int input_command(void)
/**
 ***  Inputs command, returns command number.
 **/
{
   char s[100];

   PRINTF("Solver> ");
   readln(s,sizeof(s));
   return( command_number(s) );
}

static void write_var(FILE *out, slv_system_t sys, struct var_variable *var)
/**
 ***  Writes a variable out.
 **/
{
   char *name;
   name = var_make_name(sys,var);
   if( *name == '?' ) FPRINTF(out,"%d",var_sindex(var));
   else FPRINTF(out,"%s",name);
   ascfree(name);
}

static void write_rel(FILE *out, slv_system_t sys, struct rel_relation *rel, boolean suppress)
/**
 ***  Writes a relation out.
 **/
{
   char *name, *str;
   name = rel_make_name(sys,rel);
   if( *name == '?' ) FPRINTF(out,"%d",rel_sindex(rel));
   else FPRINTF(out,"%s",name);
   ascfree(name);
   if( suppress ) return;
   PUTC('\n',out);
   str = relman_make_string_infix(sys,rel);
   FPRINTF(out,"\t%s\n",str);
   ascfree(str);
}

static void write_varlist(FILE *out, slv_system_t sys, var_filter_t *vfilter, boolean justwritestats)
/**
 ***  Writes the variable list for those variables
 ***  passing through filter.
 **/
{
   linsol_system_t lsys;
   mtx_matrix_t mtx;
   mtx_region_t reg;
   int32 col;
   struct var_variable **vp;
   int32 num[2][2];   /* num[?fixed][?incident] */
   int32 tnum[2];     /* tnum[?incident] */

   lsys = slv_get_linsol_sys(sys);
   mtx = linsol_get_matrix(lsys);
   FPRINTF(out,"#/block. value (nominal) [LB,UB] ");
   FPRINTF(out,"?fixed ?incident ?solved - name\n");
   num[0][0] = num[0][1] = num[1][0] = num[1][1] = 0;
   for( vp=slv_get_master_var_list(sys) ; *vp != NULL ; ++vp )
      if( var_apply_filter(*vp,vfilter) ) {
         ++num[(int)(var_fixed(*vp)&&var_active(*vp))]
	      [(int)(var_incident(*vp)&&var_active(*vp))];
         if( justwritestats ) continue;
	 col = mtx_org_to_col(mtx,var_sindex(*vp));
         FPRINTF(out,"%3d/%3d. %14e (%14e) [%14e,%14e] %2s %4s - ",
		 var_sindex(*vp) , mtx_block_containing_col(mtx,col,&reg) ,
		 var_value(*vp),var_nominal(*vp),
		 var_lower_bound(*vp) , var_upper_bound(*vp) ,
		 (var_fixed(*vp)&&var_active(*vp))?"FX":"FR",
		 (var_incident(*vp)&&var_active(*vp))?"INC":"!INC");
         write_var(out,sys,*vp);
         PUTC('\n',out);
      }

   tnum[0]=num[0][0]+num[1][0];
   tnum[1]=num[0][1]+num[1][1];
   FPRINTF(out,"# vars     free    fixed   total\n");
   FPRINTF(out,"incident   %4d     %4d    %4d\n",
	   num[0][1],num[1][1],tnum[1]);
   FPRINTF(out,"otherwise  %4d     %4d    %4d\n",
	   num[0][0],num[1][0],tnum[0]);
   FPRINTF(out,"total      %4d     %4d    %4d\n",
	   num[0][0]+num[0][1],num[1][0]+num[1][1],tnum[0]+tnum[1]);
}

static void write_rellist(FILE *out, slv_system_t sys, rel_filter_t *rfilter, 
                          boolean justwritestats, boolean suppress)
/**
 ***  Writes the relation list for those relations
 ***  passing through filter.
 **/
{
   linsol_system_t lsys;
   mtx_matrix_t mtx;
   mtx_region_t reg;
   int32 row;
   struct rel_relation **rp;
   boolean old_calc_ok;
   int32 num[2];   /* [?included] */

   old_calc_ok = calc_ok;
   lsys = slv_get_linsol_sys(sys);
   mtx = linsol_get_matrix(lsys);

   FPRINTF(out,"#/block. residual ?included - name\\n   relation\n");
   num[0] = num[1] = 0L;
   Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
   for( rp=slv_get_master_rel_list(sys) ; *rp != NULL ; ++rp ) {
      if( rel_apply_filter(*rp,rfilter) ) {
         real64 res;

         ++num[(int)(rel_included(*rp) && rel_active(*rp))];
         if( justwritestats ) continue;

         calc_ok = TRUE;
         res = relman_eval(*rp,&calc_ok,SAFE_FIX_ME);
	 row = mtx_org_to_row(mtx,rel_sindex(*rp));
         FPRINTF(out,"%3d/%3d. %c%14e %2s - ",
		 rel_sindex(*rp) , mtx_block_containing_row(mtx,row,&reg) ,
		 calc_ok?' ':'!',res,
		 (rel_included(*rp) && rel_active(*rp))?"IN":"IG");
         write_rel(out,sys,*rp,suppress);
      }
   }
   Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
   FPRINTF(out,"There are %d relations, %d included and %d ignored.\n",
	   num[0]+num[1],num[1],num[0]);
   calc_ok = old_calc_ok;
}

static void write_obj(FILE *out, slv_system_t sys)
/**
 ***  Writes out the objective function. THE SEMANTICS MAY BE WRONG HERE.
 **/
{
   struct rel_relation *obj = slv_get_obj_relation(sys);
   real64 val;

   if( obj == NULL )
      FPRINTF(out,"Objective: ----NONE---- ( = 0.0)\n");
   else {
      char *str = relman_make_string_infix(sys,obj);
      Asc_SignalHandlerPush(SIGFPE,SIG_IGN);
      val = relman_eval(obj,&calc_ok,SAFE_FIX_ME);
      Asc_SignalHandlerPop(SIGFPE,SIG_IGN);
      FPRINTF(out,"Objective: %s ( = %g)\n",str,val);
      ascfree(str);
   }
}

static void input_parameters(slv_system_t sys)
/* Inputs parameters for the given system. */
{
   slv_parameters_t p;

   slv_get_parameters(sys,&p);

   PRINTF("Print more important messages? [%s] ",
	  yorn(p.output.more_important!=NULL));
   p.output.more_important = input_boolean(p.output.more_important!=NULL) ?
      stdout : NULL;

   PRINTF("Print less important messages? [%s] ",
	  yorn(p.output.less_important!=NULL));
   p.output.less_important = input_boolean(p.output.less_important!=NULL) ?
      stdout : NULL;

   PRINTF("Cpu time limit (in seconds) [%g]: ",p.time_limit);
   p.time_limit = readdouble(p.time_limit);

   PRINTF("Iteration limit [%d]: ",p.iteration_limit);
   p.iteration_limit = readlong((long)p.iteration_limit);

   PRINTF("Tolerance, singular [%g]: ",p.tolerance.singular);
   p.tolerance.singular = readdouble(p.tolerance.singular);

   PRINTF("Tolerance, feasible [%g]: ",p.tolerance.feasible);
   p.tolerance.feasible = readdouble(p.tolerance.feasible);

   PRINTF("Partition problem? [%s] ",yorn(p.partition));
   p.partition = input_boolean(p.partition);

   PRINTF("Ignore bounds? [%s] ",yorn(p.ignore_bounds));
   p.ignore_bounds = input_boolean(p.ignore_bounds);

   slv_set_parameters(sys,&p);
}

static void  output_parameters(slv_system_t sys)
/**
 ***  Outputs parameters for the given system.
 **/
{
   slv_parameters_t p;
   slv_get_parameters(sys,&p);

   PRINTF("Print more important messages? = %s\n",
	  yorn(p.output.more_important!=NULL));
   PRINTF("Print less important messages? = %s\n",
	  yorn(p.output.less_important!=NULL));
   PRINTF("Cpu time limit                 = %g seconds\n",p.time_limit);
   PRINTF("Iteration limit                = %d\n",p.iteration_limit);
   PRINTF("Tolerance, singular            = %g\n",p.tolerance.singular);
   PRINTF("Tolerance, feasible            = %g\n",p.tolerance.feasible);
   PRINTF("Partition problem?             = %s\n",yorn(p.partition));
   PRINTF("Ignore bounds?                 = %s\n",yorn(p.ignore_bounds));
}

static void output_status(slv_system_t sys)
/**
 ***  Outputs status for the given system.
 **/
{
   slv_status_t s;
   slv_get_status(sys,&s);

   PRINTF("\nSolver status\n-------------\n");

   if( s.converged )
      PRINTF("System converged.\n");
   else
      PRINTF("System is %sready to solve.\n",s.ready_to_solve ? "" : "not ");

   PRINTF("Abnormalities -->");
   if( s.over_defined ) PRINTF("  over-defined");
   if( s.under_defined ) PRINTF("  under-defined");
   if( s.struct_singular ) PRINTF("  structurally singular");
   if( s.diverged ) PRINTF("  diverged");
   if( s.inconsistent ) PRINTF("  inconsistent");
   if( !s.calc_ok ) PRINTF("  calculation error");
   if( s.iteration_limit_exceeded ) PRINTF("  iteration limit exceeded");
   if( s.time_limit_exceeded ) PRINTF("  time limit exceeded");
   if( s.ok ) PRINTF("  NONE");
   PUTCHAR('\n');

   PRINTF("# of blocks = %d\n",s.block.number_of);
   PRINTF("current block = %d\n",s.block.current_block);
   PRINTF("its size = %d\n",s.block.current_size);
   PRINTF("# vars/rels solved already = %d\n",
	  s.block.previous_total_size);
   PRINTF("Iteration = %d (this block = %d)\n",
	  s.iteration,s.block.iteration);
   PRINTF("CPU time elapsed = %g sec (this block = %g sec)\n",
	  s.cpu_elapsed,s.block.cpu_elapsed);
   PRINTF("Residual norm in current block = %g\n",s.block.residual);
}



struct rock {
   struct var_variable **vlist;
   struct rel_relation **rlist;
   int32 nvars,nrels;
};

static void output_system(FILE *fp, slv_system_t sys)
{
   struct rock vr;
   int32 n;
   struct var_variable **vp;
   struct rel_relation **rp, *obj;
   slv_parameters_t p;

   vr.nvars = vr.nrels = 0;
   vr.vlist = slv_get_master_var_list(sys);
   vr.rlist = slv_get_master_rel_list(sys);

   for( vp=vr.vlist ; vp != NULL && *vp != NULL ; ++vp )
      ++vr.nvars;
   FPRINTF(fp,"nvars %d\n",vr.nvars);

   for( rp=vr.rlist ; rp != NULL && *rp != NULL ; ++rp )
      ++vr.nrels;
   FPRINTF(fp,"nrels %d\n\n",vr.nrels);

   for( n=0,vp=slv_get_master_var_list(sys);
        vp != NULL && *vp != NULL;
        ++vp,++n ) {
      FPRINTF(fp,";v%d\n",n);
      FPRINTF(fp,"   v%d = %g\n",n,var_value(*vp));
      FPRINTF(fp,"   v%d nominal %g\n",n,var_nominal(*vp));
      FPRINTF(fp,"   v%d LB %g\n",n,var_lower_bound(*vp));
      FPRINTF(fp,"   v%d UB %g\n",n,var_upper_bound(*vp));
      FPRINTF(fp,"   v%d %s\n",n,var_fixed(*vp)?"fixed":"free");
      FPRINTF(fp,"   v%d %s\n",n,var_active(*vp)?"active":"inactive");
   }

   for( n=0,rp=slv_get_master_rel_list(sys);
        rp != NULL && *rp != NULL;
        ++rp,++n ) {
      char *str = relman_make_string_infix(sys,*rp);
      FPRINTF(fp,"r%d: %s\n",n,str);
      FPRINTF(fp,"      r%d %s\n",n,(rel_included(*rp) && rel_active(*rp)) ?
	      "included" : "ignored");
      ascfree(str);
   }

   if( (obj=slv_get_obj_relation(sys)) != NULL ) {
      char *str = relman_make_string_infix(sys,obj);
      FPRINTF(fp,"\nobj %s\n",str);
      ascfree(str);
   }

   slv_get_parameters(sys,&p);
   FPRINTF(fp,"\ntime limit %g ;seconds\n",p.time_limit);
   FPRINTF(fp,"iteration limit %d\n",p.iteration_limit);
   FPRINTF(fp,"singular tolerance %g\n",p.tolerance.singular);
   FPRINTF(fp,"feasible tolerance %g\n",p.tolerance.feasible);
   FPRINTF(fp,"partition %s\n",p.partition?"yes":"no");
   FPRINTF(fp,"ignore bounds %s\n",p.ignore_bounds?"yes":"no");
   FPRINTF(fp,"output more important %s\n",(p.output.more_important!=NULL)?
	   "yes" :"no");
   FPRINTF(fp,"output less important %s\n",(p.output.less_important!=NULL)?
	   "yes" :"no");
   FPRINTF(fp,"\nend\n");
}



static slv_system_t sys = NULL;
/* The system in use */
static struct Instance *inst = NULL;
/* Instance in use */

static boolean do_command(int command)
/**
 ***  Executes given command: returns FALSE if terminate.
 **/
{
   static boolean suppress_rel_flag = TRUE;

   switch(command) {
      default:
         PRINTF("No such command.\n");
	 break;

      case C_NOP:
	 break;

      case C_RETURN:
         PRINTF("Keep system around for next time? [yes]: ");
         if( !input_boolean(TRUE) ) {
	    system_destroy(sys);
            sys = NULL;
         }
         return(FALSE);

      case C_HELP:
         print_commands();
         break;

      case C_INPUT_PARAMETERS:
         input_parameters(sys);
         break;

      case C_OUTPUT_PARAMETERS:
         output_parameters(sys);
         break;

      case C_OUTPUT_STATUS:
         output_status(sys);
         break;

      case C_COUNT: {
	 var_filter_t vfilter;
	 rel_filter_t rfilter;

         vfilter.matchbits = 0; /* allow all */
         rfilter.matchbits = 0; /* allow all */
	 FPRINTF(stdout,"There are %d variables and %d relations.\n",
		 slv_count_solvers_vars(sys,&vfilter),
		 slv_count_solvers_rels(sys,&rfilter));
         break;
      }

      case C_COUNT_IN_BLOCK: {
         slv_status_t s;
	 int32 bnum;
	 linsol_system_t lsys;
	 mtx_matrix_t mtx;
	 mtx_region_t reg;

         slv_get_status(sys,&s);
         PRINTF("Block number [%d]: ",s.block.current_block);
         bnum = (int32)readlong((long)s.block.current_block);
	 lsys = slv_get_linsol_sys(sys);
	 mtx = linsol_get_matrix(lsys);
	 mtx_block(mtx,bnum,&reg);
	 FPRINTF(stdout,"There are %d variables in block %d.\n",
		 (reg.col.high-reg.col.low+1),bnum);
	 FPRINTF(stdout,"There are %d relations in block %d.\n",
		 (reg.row.high-reg.row.low+1),bnum);
         break;
      }

      case C_WRITE_VAR: {
         int32 n;
	 struct var_variable *var;

         PRINTF("Which variable [0]: ");
         n = (int32)readlong(0L);
         var = slv_get_master_var_list(sys)[n];
         write_var(stdout,sys,var);
         break;
      }

      case C_WRITE_VARS: {
	 var_filter_t vfilter;
	 vfilter.matchbits = 0;
         write_varlist(stdout,sys,&vfilter,FALSE);
	 break;
      }

      case C_WRITE_VARS_IN_BLOCK: {
	 struct var_variable **vp = slv_get_solvers_var_list(sys);
         slv_status_t s;
	 int32 bnum;
	 linsol_system_t lsys;
	 mtx_matrix_t mtx;
	 mtx_region_t reg;

         slv_get_status(sys,&s);
         PRINTF("Block number [%d]: ",s.block.current_block);
         bnum = (int32)readlong((long)s.block.current_block);
	 lsys = slv_get_linsol_sys(sys);
	 mtx = linsol_get_matrix(lsys);
	 mtx_block(mtx,bnum,&reg);
	 for( ; reg.col.low <= reg.col.high; reg.col.low++ ) {
	    struct var_variable *var = vp[mtx_col_to_org(mtx,reg.col.low)];
	    write_var(stdout,sys,var);
	 }
         break;
      }

      case C_TOGGLE_SUPPRESS_REL:
         PRINTF("Relations will %sbe printed now.\n",
		(suppress_rel_flag = !suppress_rel_flag)?"not " : "");
         break;

      case C_WRITE_REL: {
	 struct rel_relation *rel;
         int32 n;

         PRINTF("Which relation [0]: ");
         n = (int32)readlong(0L);
         rel = slv_get_master_rel_list(sys)[n];
         write_rel(stdout,sys,rel,suppress_rel_flag);
         break;
      }

      case C_WRITE_RELS: {
	 rel_filter_t rfilter;
	 rfilter.matchbits = 0;
         write_rellist(stdout,sys,&rfilter,FALSE,suppress_rel_flag);
         break;
      }

      case C_WRITE_RELS_IN_BLOCK: {
	 struct rel_relation **rp = slv_get_solvers_rel_list(sys);
         slv_status_t s;
	 int32 bnum;
	 linsol_system_t lsys;
	 mtx_matrix_t mtx;
	 mtx_region_t reg;

         slv_get_status(sys,&s);
         PRINTF("Block number [%d]: ",s.block.current_block);
         bnum = (int32)readlong((long)s.block.current_block);
	 lsys = slv_get_linsol_sys(sys);
	 mtx = linsol_get_matrix(lsys);
	 mtx_block(mtx,bnum,&reg);
	 for( ; reg.row.low <= reg.row.high; reg.row.low++ ) {
	    struct rel_relation *rel = rp[mtx_row_to_org(mtx,reg.row.low)];
	    write_rel(stdout,sys,rel,suppress_rel_flag);
	 }
	 break;
      }

      case C_WRITE_OBJ:
         write_obj(stdout,sys);
         break;

      case C_CHECK_DIMENSIONS: {
/* broken 12/05 - checkdim not implemented */
        FPRINTF(ASCERR, "Error - checkdim not implemented.\n");
/*
         chkdim_system_t chk;
         slv_parameters_t p;
         / * expr_t obj; * /

         PRINTF("Creating system . . .\n");
         chkdim_create_system(&chk,slv_get_master_rel_list(sys));
         / * if( (obj=slv_get_obj_function(sys)) != NULL )
            chkdim_append_expr(&chk,obj); * /

         PRINTF(". . . done.  Check system . . .\n");
         slv_get_parameters(sys,&p);
         chkdim_check_dimensions(&chk,&p);

         PRINTF(". . . done.  Assign dimensions to variables? [no] ");
         if( input_boolean(FALSE) )
            chkdim_assign_dimensions(&chk,&p);
         chkdim_destroy_system(&chk);
*/
         break;
      }

      case C_ELIGIBLE_SOLVERS: {
         int cur,n;
/* broken 6/96 baa */
         cur = slv_get_selected_solver(sys);
         PRINTF("Solver   Name       ?Eligible\n");
         PRINTF("-----------------------------\n");
         for( n=0 ; n<slv_number_of_solvers ; ++n )
            PRINTF("%c%3d     %-11s    %s\n",
		   (n==cur?'*':' '),n,slv_solver_name(n),
		   yorn(slv_eligible_solver(sys)) );
         break;
      }

      case C_SELECT_SOLVER: {
         int n;

         PRINTF("Solver   Name\n");
         PRINTF("----------------------\n");
         for( n=0 ; n<slv_number_of_solvers ; ++n )
            PRINTF("%4d     %s\n",n,slv_solver_name(n));
         PRINTF("Which solver? [%d]: ",n=slv_get_selected_solver(sys));
         n = (int)readlong((long)n);

         slv_select_solver(sys,n);
         break;
      }

      case C_SELECTED_SOLVER: {
         int n;
         n = slv_get_selected_solver(sys);
         PRINTF("Selected solver is %d (%s).\n",n,slv_solver_name(n));
         break;
      }

      case C_PRESOLVE:
         slv_presolve(sys);
         output_status(sys);
         break;

      case C_RESOLVE:
         slv_resolve(sys);
         output_status(sys);
         break;

      case C_ITERATE:
         slv_iterate(sys);
         output_status(sys);
         break;

      case C_SOLVE:
         slv_solve(sys);
         output_status(sys);
         break;

      case C_DUMP_SYSTEM: {
         FILE *out;
         char fname[200];
         struct var_variable **vp;
         struct rel_relation **rp;

         PRINTF("Target filename [stdout]: ");
         readln(fname,sizeof(fname));
         out = (*fname=='\0') ? stdout : fopen(fname,"w");
         if( out == NULL ) {
            FPRINTF(stderr,"Unable to open %s for writing.\n",fname);
            break;
         }

         for( vp=slv_get_master_var_list(sys) ; *vp != NULL ; ++vp ) {
            FPRINTF(out,"; v%d <--> ",var_sindex(*vp));
            write_var(out,sys,*vp);
            PUTC('\n',out);
         }
         for( rp=slv_get_master_rel_list(sys) ; *rp != NULL ; ++rp ) {
            FPRINTF(out,"; r%d <--> ",rel_sindex(*rp));
            write_rel(out,sys,*rp,suppress_rel_flag);
            PUTC('\n',out);
         }
         output_system(out,sys);

         if( out != stdout )
            fclose(out);
         break;
      }

      case C_PLOT: {
         char args[50];
         static char plotfilename[] = "~/ascend.plot";

         if( !plot_allowed(inst) ) {
            PRINTF("Not allowed to plot instance: wrong type.\n");
            break;
         }

         PRINTF("Command line arguments: ");
         readln(args,sizeof(args));
         plot_prepare_file(inst,plotfilename);
         PRINTF("Plot left in %s\n",plotfilename);
         break;
      }
   }
   return(TRUE);
}

#define USER_SAYS_KEEP \
   (PRINTF("Keep previous system? [yes]: ") , input_boolean(TRUE))
/**
 ***  Asks user if system should be kept, and returns response.
 **/

void Solve(struct Instance *i)
{
   if( sys != NULL )
      if( inst != i || !USER_SAYS_KEEP ) {
	 system_destroy(sys);
         sys = NULL;
      }

   if( sys == NULL ) {
      sys = system_build(i);
      PRINTF("Presolving . . .\n");
      do_command(C_PRESOLVE);
   }

   inst = i;
   while( do_command(input_command()) )
      ;
}
