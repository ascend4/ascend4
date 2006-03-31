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
static mtx_matrix_t N, NTN;
static real64 D1[10], D2[10];

static make_coefficient_matrix_1()
{
   mtx_coord_t coord;
 
   printf("Create N\n");
   N = mtx_create();
   mtx_set_order(N,10);

   /* eqn 0 */

   coord.row = 0;
   coord.col = 0;
   mtx_set_value(N,&coord,1.);

   /* eqn 1 */

   coord.row = 1;
   coord.col = 0;
   mtx_set_value(N,&coord,0.007);
   coord.col = 1;
   mtx_set_value(N,&coord,0.571);
   coord.col = 2;
   mtx_set_value(N,&coord,0.422);

   /* eqn 2 */

   coord.row = 2;
   coord.col = 1;
   mtx_set_value(N,&coord,0.125);
   coord.col = 2;
   mtx_set_value(N,&coord,0.75);
   coord.col = 3;
   mtx_set_value(N,&coord,0.125);

   /* eqn 3 */

   coord.row = 3;
   coord.col = 2;
   mtx_set_value(N,&coord,0.422);
   coord.col = 3;
   mtx_set_value(N,&coord,0.571);
   coord.col = 4;
   mtx_set_value(N,&coord,0.007);

   /* eqn 4 */

   coord.row = 4;
   coord.col = 4;
   mtx_set_value(N,&coord,1.);
}

static make_coefficient_matrix_2()
{
   mtx_coord_t coord;

   printf("Create N\n");
   N = mtx_create();
   mtx_set_order(N,10);

   /* eqn 0 */

   coord.row = 0;
   coord.col = 0;
   mtx_set_value(N,&coord,1.0);

   /* eqn 1 */

   coord.row = 1;
   coord.col = 0;
   mtx_set_value(N,&coord,0.15);
   coord.col = 1;
   mtx_set_value(N,&coord,0.662);
   coord.col = 2;
   mtx_set_value(N,&coord,0.188);

   /* eqn 2 */

   coord.row = 2;
   coord.col = 1;
   mtx_set_value(N,&coord,0.5);
   coord.col = 2;
   mtx_set_value(N,&coord,0.5);

   /* eqn 3 */

   coord.row = 3;
   coord.col = 1;
   mtx_set_value(N,&coord,0.188);
   coord.col = 2;
   mtx_set_value(N,&coord,0.662);
   coord.col = 3;
   mtx_set_value(N,&coord,0.15);

   /* eqn 4 */

   coord.row = 4;
   coord.col = 3;
   mtx_set_value(N,&coord,1.0);
}

static make_rhs()
{
   printf("\tD1 = { 0.0 }   D2 = { 0.0 }\n"
          "\t     { 1.5 }        { 2.0 }\n"
          "\t     { 3.0 }        { 2.5 }\n"
          "\t     { 4.5 }        { 2.0 }\n"
          "\t     { 6.0 }        { 0.0 }\n");
   D1[0] = 0.0;
   D1[1] = 1.5;
   D1[2] = 3.0;
   D1[3] = 4.5;
   D1[4] = 6.0;

   linsol_add_rhs(sys,D1,FALSE);

   D2[0] = 0.0;
   D2[1] = 2.0;
   D2[2] = 2.5;
   D2[3] = 2.0;
   D2[4] = 0.0;

   linsol_add_rhs(sys,D2,FALSE);
}

mtx_matrix_t ATA(mtx_matrix_t N)
{
   mtx_matrix_t NTN;
   int32 i;
   int32 v1,v2,v3;
   mtx_coord_t c1,c2,c3,c4;

   NTN = mtx_create();
   mtx_set_order(NTN,mtx_order(N));

   for (i = 0; i < mtx_order(N); i++) {
      c1.row = i;
      c1.col = mtx_FIRST;
      c2.row = c1.row;
      while (mtx_next_in_row(N,&c1,mtx_ALL_COLS)) {
         v1 = mtx_value(N,&c1);
         c2.col = mtx_FIRST;
         while (mtx_next_in_row(N,&c2,mtx_ALL_COLS)) {
            if (c2.col >= c1.col) {
               v2 = v1*mtx_value(N,&c2);
               c3.row = c2.col;
               c3.col = c1.col;
               v3 = v2 + mtx_value(NTN,&c3);
               mtx_set_value(NTN,&c3,v3);
               if (c3.row != c3.col) {
                  c4.row = c3.col;
                  c4.col = c3.row;
                  v3 = v2 + mtx_value(NTN,&c4);
                  mtx_set_value(NTN,&c4,v3);
               }
            }
         }
      }
   }
   return(NTN);
}

static ATv(mtx_matrix_t A, real64 v[], real64 r[])
{
   mtx_coord_t c;
   int32 i;

   for (i = 0; i < mtx_order(A); i++) r[i] = 0.0;

   for (i = 0; i < mtx_order(A); i++) {
      c.row = i;
      c.col = mtx_FIRST;
      while (mtx_next_in_row(A,&c,mtx_ALL_COLS)) {
         r[c.col] = r[c.col] + mtx_value(A,&c)*v[i];
      }
   }
}

/* print out non-zero elements of a matrix in text by row */
static print_matrix(mtx_matrix_t N, mtx_region_t region)
{
   mtx_coord_t c;
   int32 i;
   int has_row;

   for (i = region.col.low; i <= region.col.high; i++) {
      c.row = i;
      c.col = mtx_FIRST;
      has_row = FALSE;
      while (mtx_next_in_row(N,&c,mtx_ALL_COLS)) {
         has_row = TRUE;
         printf("\tmtx[%d][%d] = %f\n",c.row,c.col,mtx_value(N,&c));
      }
      if (has_row) printf("\n");
   }
}

static print_vector(real64 *v, mtx_range_t range)
{
    int32 i;

    for (i = range.low; i <= range.high; i++) {
      printf("\tvec[%d] = %f\n",i,v[i]);
    }
}

static print_solution(real64 *rhs, mtx_range_t range)
{
    int32 i;

    printf("Solution:\n");
    for (i = range.low; i <= range.high; i++) {
      printf("\ts[%d] = %f\n",i,linsol_var_value(sys,rhs,i));
    }
}

static print_residuals(real64 *rhs, mtx_range_t range)
{
    int32 i;

    printf("Residuals:\n");
    for (i = range.low; i <= range.high; i++) {
      printf("\tr[%d] = %f\n",i,linsol_eqn_residual(sys,rhs,i));
    }
}

int main()
{
   real64 rhs[10];
   mtx_range_t range;
   mtx_region_t region = {0,4,0,4};

#ifdef __WIN32__
   Asc_PrintPushVTable(&f_vtable);
#endif

   sys = linsol_create();

   printf("\n1st Case - Square\n");

   printf("Make coefficient matrix N\n");
   make_coefficient_matrix_1();
   print_matrix(N, region);
   printf("Make right hand side(s) D1 & D2\n");
   make_rhs();

   printf("Number of right hand sides = %d\n"
         ,linsol_number_of_rhs(sys));

   printf("Set coefficient matrix\n");
   linsol_set_matrix(sys,N);
   printf("Reorder equations\n");
   linsol_reorder(sys,&region);

   printf("Invert matrix\n");
   linsol_invert(sys,&region);

   printf("Solve D1\n");
   linsol_solve(sys,D1);
   printf("Rank = %d\n",linsol_rank(sys));
   range.low = 0;
   range.high = 4;
   print_residuals(D1,range);
   print_solution(D1,range);

   printf("Solve D2\n");
   linsol_solve(sys,D2);
   printf("Rank = %d\n",linsol_rank(sys));
   print_residuals(D2,range);
   print_solution(D2,range);

   mtx_destroy(N);
   linsol_destroy(sys);

   /* 2nd case */

   printf("\n2nd case - overspecified\n");
   printf("Make coefficient matrix N\n");
   make_coefficient_matrix_2();
   print_matrix(N, region);
   printf("Computing NTN\n");
   NTN = ATA(N);
   print_matrix(NTN, region);
   printf("Compute rhs\n");
   ATv(N,D1,rhs);
   range.low = 0;
   range.high = 3;
   print_vector(rhs,range);
   sys = linsol_create();
   linsol_set_matrix(sys,NTN);
   linsol_add_rhs(sys,rhs,FALSE);
   printf("Reorder matrix\n");
   linsol_reorder(sys, &region);
   printf("Invert matrix\n");
   linsol_invert(sys, &region);
   printf("Solve with inverse rhs\n");
   linsol_solve(sys,rhs);

   printf("Rank = %d\n",linsol_rank(sys));
   print_residuals(rhs,range);
   print_solution(rhs,range);

   printf("Solve with inverse rhs\n");
   ATv(N,D2,rhs);
   print_vector(rhs,range);
   linsol_rhs_was_changed(sys,rhs);
   linsol_solve(sys,rhs);
   printf("Rank = %d\n",linsol_rank(sys));
   print_residuals(rhs,range);
   print_solution(rhs,range);

#ifdef __WIN32__
   Asc_PrintRemoveVTable(f_vtable_name);
#endif

  return 0;
}
