#include <utilities/ascConfig.h>
#include <solver/mtx.h>
#include <solver/linsol.h>

#ifdef __WIN32__
#include <utilities/ascPrintType.h>
#include <utilities/ascPrint.h>
#define f_vtable_name "example_vtable"
static struct Asc_PrintVTable f_vtable = {f_vtable_name, vfprintf, fflush, NULL};
#endif

static linsol_system_t sys;
static mtx_matrix_t A;
static real64 b[10];

static make_coefficient_matrix()
{
   mtx_coord_t coord;

   printf("Create A\n");
   A = mtx_create();
   mtx_set_order(A,10);
   printf("\tA[0][0] = 2.0\n");
   coord.row = 0;
   coord.col = 0;
   mtx_set_value(A,&coord,2.);
   printf("\tA[0][1] = 4.0\n");
   coord.row = 0;
   coord.col = 1;
   mtx_set_value(A,&coord,4.);
   printf("\tA[1][0] = 1.0\n");
   coord.row = 1;
   coord.col = 0;
   mtx_set_value(A,&coord,1.);
   printf("\tA[1][1] = 4.0\n");
   coord.row = 1;
   coord.col = 1;
   mtx_set_value(A,&coord,4.);
   printf("\tA[2][1] = 1.0\n");
   coord.row = 2;
   coord.col = 1;
   mtx_set_value(A,&coord,1.);
   printf("\tA[2][2] = 1.0\n");
   coord.row = 2;
   coord.col = 2;
   mtx_set_value(A,&coord,1.);
   printf("\t{  2.0  4.0  0.0  }\n");
   printf("\t{  1.0  4.0  0.0  }\n");
   printf("\t{  0.0  1.0  1.0  }\n");
}

static make_rhs()
{
   printf("\tb[0] = 8.0\n");
   printf("\tb[1] = 6.0\n");
   printf("\tb[2] = 2.5\n");
   b[0] = 8.0;
   b[1] = 6.0;
   b[2] = 2.5;

   linsol_add_rhs(sys,b,FALSE);
}

static print_solution()
{
   printf("x[0] = %f\n",linsol_var_value(sys,b,0));
   printf("x[1] = %f\n",linsol_var_value(sys,b,1));
   printf("x[2] = %f\n",linsol_var_value(sys,b,2));
}

static print_residuals()
{
   printf("Residual[0] = %f\n",linsol_eqn_residual(sys,b,0));
   printf("Residual[1] = %f\n",linsol_eqn_residual(sys,b,1));
   printf("Residual[2] = %f\n",linsol_eqn_residual(sys,b,2));
}

int main(void)
{
   mtx_region_t region = {0,2,0,2};

#ifdef __WIN32__
   Asc_PrintPushVTable(&f_vtable);
#endif

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
   linsol_set_matrix(sys,A);
   printf("Reorder equations\n");
   linsol_reorder(sys,&region);
   printf("Invert matrix\n");
   linsol_invert(sys,&region);
   printf("Solve system\n");
   linsol_solve(sys,b);

   printf("Rank = %d\n",linsol_rank(sys));
  
   print_residuals();

   print_solution();

#ifdef __WIN32__
   Asc_PrintRemoveVTable(f_vtable_name);
#endif

  return 0;
}
