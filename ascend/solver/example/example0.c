/* This example is outdated and non-functional
#include "base.h"
#include "var_normal.h"
#include "expr_normal.h"
#include "rel_normal.h"
#include "slv.h"
#include "eparse.h"
*/

/* $##LINK##
bind -bdir /usr/kw17/utilities/obj -bdir //irawaddy/usr10/newascend/solver/obj example_program.bin var.bin expr.bin rel.bin eparse_from_str.bin eparse_to_str.bin calc.bin relman.bin slv.bin slv0.bin slv1.bin //pelee/misc/ascend-2/minos51/bin/minos51.bin newminos_filename.bin slv_io.bin filter.bin mtx.bin part.bin reorder.bin linsol.bin ls_io.bin tm0.bin set.bin mem.bin readln.bin pl.bin -b example_program
*/

/* This example is used to illustrate how easy(???) it
   is to use the solver */

/* Try to solve:
   10000*x*y = 1
   exp(-x) + exp(-y) = 1.0001,
      x,y initially 0,1 */

static var_variable_t vars[3];
static rel_relation_t rels[3];
static slv_system_t sys;

static make_vars()
{
   vars[0] = var_create();
   vars[1] = var_create();
   vars[2] = NULL;
   var_set_value(vars[0],0.5);
   var_set_value(vars[1],0.5);
}

static var_variable_t name_to_var(rock,name)
POINTER rock;
char *name;
{
   switch( *name ) {
      case 'x' : return(vars[0]);
      case 'y' : return(vars[1]);
   }
   return(NULL);
}

static make_rels()
{
   int i;
   rels[0] = eparse_str_to_rel("10000*x*y = 1",eparse_INFIX,name_to_var,NULL);
   rels[1] = eparse_str_to_rel("exp(-x) + exp(-y) = 1.0001",eparse_INFIX,name_to_var,NULL);
   rels[2] = NULL;
   for( i=0 ; i<2 ; ++i )
      if( rels[i] == NULL )
         fprintf(stderr,"Relation %d invalid.\n",i);
}

static make_sys()
{
   sys = slv_create();
   slv_set_rel_list(sys,rels);
   slv_set_var_list(sys,vars);
}

static print_solution()
{
   printf("Solution\nx = %g\ny = %g\n",var_value(vars[0]),var_value(vars[1]));
}

static destroy_problem()
{
   var_destroy(vars[0]);
   var_destroy(vars[1]);
   rel_destroy(rels[0]);
   rel_destroy(rels[1]);
   slv_destroy(sys);
}

main()
{
   slv_status_t s;

   make_vars();
   make_rels();
   make_sys();

   slv_presolve(sys);
   slv_get_status(sys,&s);
   if( s.ready_to_solve )
      slv_solve(sys);
   slv_get_status(sys,&s);
   if( s.converged )
      print_solution();
   else
      printf("Not successful.\n");

   destroy_problem();
}
