#include "base.h"
#include "linsol.h"

static linsol_system_t sys;
static mtx_matrix_t A;
static mtx_number_t b[10];

static make_coefficient_matrix()
{
   mtx_coord_t coord;
 
   printf("Create A\n");
   A = mtx_create();
   mtx_set_order(A,10);
   printf("A[0][0]\n");
   coord.row = 0;
   coord.col = 0;
   mtx_set_value(A,&coord,2.);
   printf("A[0][1]\n");
   coord.row = 0;
   coord.col = 1;
   mtx_set_value(A,&coord,4.);
   printf("A[1][0]\n");
   coord.row = 1;
   coord.col = 0;
   mtx_set_value(A,&coord,1.);
   printf("A[1][1]\n");
   coord.row = 1;
   coord.col = 1;
   mtx_set_value(A,&coord,4.);

   coord.row = 2;
   coord.col = 1;
   mtx_set_value(A,&coord,1.);
   coord.row = 2;
   coord.col = 2;
   mtx_set_value(A,&coord,1.);
}

static make_rhs()
{
   b[0] = 8.0;
   b[1] = 6.0;
   b[2] = 2.5;

   linsol_add_rhs(sys,b);
}

static print_solution()
{
   printf("x[0] = %f\n",linsol_var_value(sys,b,0));
   printf("x[1] = %f\n",linsol_var_value(sys,b,1));
}

static print_residuals()
{
   printf("Residual[0] = %f\n",linsol_eqn_residual(sys,b,0));
   printf("Residual[1] = %f\n",linsol_eqn_residual(sys,b,1));
   printf("Residual[2] = %f\n",linsol_eqn_residual(sys,b,2));
}

main()
{
   sys = linsol_create();

   printf("Make coefficient matrix\n");
   make_coefficient_matrix();
   printf("Non-zeros in row 0 = %d\n",
          mtx_nonzeros_in_row(A,0,mtx_ALL_COLS));
   printf("Make right hand side\n");
   make_rhs();

   printf("Number of right hand sides = %d\n"
         ,linsol_number_of_rhs(sys));
   printf("Set coefficient matrix\n");
   linsol_set_coef_matrix(sys,A);
   printf("Reorder equations\n");
   linsol_reorder(sys);
   printf("Solve without inverse\n");
   linsol_solve_wo_inverse(sys,b);

   printf("Rank = %d\n",linsol_rank(sys));
  
   print_residuals();

   print_solution();
}




