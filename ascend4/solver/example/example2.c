#include "base.h"
#include "linsol.h"

static linsol_system_t sys;
static mtx_matrix_t N,NTN;
static mtx_number_t D1[10],D2[10];

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
   D1[0] = 0.0;
   D1[1] = 1.5;
   D1[2] = 3.0;
   D1[3] = 4.5;
   D1[4] = 6.0;

   linsol_add_rhs(sys,D1);

   D2[0] = 0.0;
   D2[1] = 2.0;
   D2[2] = 2.5;
   D2[3] = 2.0;
   D2[4] = 0.0;

   linsol_add_rhs(sys,D2);
}

mtx_matrix_t ATA(N)
mtx_matrix_t N;
{
    mtx_matrix_t NTN;
    mtx_index_t i;
    mtx_number_t v1,v2,v3;
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

static ATv(A,v,r)
mtx_matrix_t A;
mtx_number_t v[],r[];
{
   mtx_coord_t c;
   mtx_index_t i;

   for (i = 0; i < mtx_order(A); i++) r[i] = 0.0;

   for (i = 0; i < mtx_order(A); i++) {
      c.row = i;
      c.col = mtx_FIRST;
      while (mtx_next_in_row(A,&c,mtx_ALL_COLS)) {
	  r[c.col] = r[c.col] + mtx_value(A,&c)*v[i];
      }
   }
}

static print_matrix(N)
mtx_matrix_t N;
{
   mtx_coord_t c;
   mtx_index_t i;

   for (i = 0; i < mtx_order(N); i++) {
      c.row = i;
      c.col = mtx_FIRST;
      while (mtx_next_in_row(N,&c,mtx_ALL_COLS)) {
         printf("x[%d][%d] = %f\n",c.row,c.col,mtx_value(N,&c));
      }
      printf("\n");
   }
}

static print_vector(v, range)
mtx_number_t *v;
mtx_range_t range;

{
    mtx_index_t i;

    for (i = range.low; i <= range.high; i++) {
      printf("x[%d] = %f\n",i,v[i]);
    }
}

static print_solution(rhs, range)
mtx_number_t *rhs;
mtx_range_t range;
{
    mtx_index_t i;

    for (i = range.low; i <= range.high; i++) {
      printf("x[%d] = %f\n",i,linsol_var_value(sys,rhs,i));
    }
}

static print_residuals(rhs, range)
mtx_number_t *rhs;
mtx_range_t range;
{
    mtx_index_t i;

    for (i = range.low; i <= range.high; i++) {
      printf("Residual[%d] = %f\n",i,linsol_eqn_residual(sys,rhs,i));
    }
}

main()
{
   mtx_number_t rhs[10];
   mtx_range_t range;

   sys = linsol_create();

   printf("Make coefficient matrix N\n");
   make_coefficient_matrix_1();
   printf("Make right hand side(s) D1 & D2\n");
   make_rhs();

   printf("Number of right hand sides = %d\n"
         ,linsol_number_of_rhs(sys));

   printf("Set coefficient matrix\n");
   linsol_set_coef_matrix(sys,N);
   printf("Reorder equations\n");
   linsol_reorder(sys);

   printf("Solve with inverse D1\n");
   linsol_solve(sys,D1);
   printf("Rank = %d\n",linsol_rank(sys));
   range.low = 0;
   range.high = 4;
   print_residuals(D1,range);
   print_solution(D1,range);

   printf("Solve with inverse D2\n");
   linsol_solve(sys,D2);
   printf("Rank = %d\n",linsol_rank(sys));
   print_residuals(D2,range);
   print_solution(D2,range);

   mtx_destroy(N);
   linsol_destroy(sys);

   /* 2nd case */

   printf("\n2nd case - overspecified\n");
   make_coefficient_matrix_2();
   print_matrix(N);
   printf("Computing NTN\n");
   NTN = ATA(N);
   printf("NTN\n\n");
   print_matrix(NTN);
   printf("Compute rhs\n");
   ATv(N,D1,rhs);
   range.low = 0;
   range.high = 3;
   print_vector(rhs,range);
   sys = linsol_create();
   linsol_set_coef_matrix(sys,NTN);
   linsol_add_rhs(sys,rhs);
   linsol_reorder(sys);
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
}






