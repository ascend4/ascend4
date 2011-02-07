#ifndef MPS_TYPES_H
#define MPS_TYPES_H

#include <ascend/general/platform.h>
#include <ascend/linear/mtx.h>

/***
 ***       MPS matrix strucutre
 ***                                    v
 ***       min/max cx:  Ax<=b           u
 ***                                    s
 ***                                    e
 ***                       1            d
 ***
 ***                       |            |  
 ***                       |            |  
 ***                      \ /          \ /
 ***
 ***                      +-            -+ 
 ***       1          ->  |              |
 ***                      |              | 
 ***                      |      A       | 
 ***                      |              |
 ***       rused      ->  |              | 
 ***                      +-            -+
 ***
 ***       crow       ->  [      c       ]
 **/

typedef struct mps_data {   /**< see more detailed comments in calc_matrix */

   int32   rused;           /**< row of last relation (incident or not) */
   int32   rinc;            /**< number of incident relations */
   int32   crow;            /**< row of cost vector (rused+1)*/
   int32   vused;           /**< column of last variable (incident or not) */
   int32   vinc;            /**< number of incident variables */
   int32   cap;             /**< size of sparse square matrix=max(vused+2+1,rused+4+1) */
   int32   rank;            /**< Symbolic rank of problem */
   int32   bused;           /**< Included boundaries */

   int solver_var_used;     /**< values are calculated in calc_svtlist  */
   int solver_relaxed_used; /**< is cache of how many of each vars used */
   int solver_int_used;                    
   int solver_binary_used;   
   int solver_semi_used;     
   int solver_other_used; 
   int solver_fixed;        /**< number of fixed or non-incident vars */

   mtx_matrix_t  Ac_mtx;    /**< Matrix representation of the A matrix and c vector */

   real64  *lbrow;          /**< pointer to array of lower bounds */
   real64  *ubrow;          /**< pointer to array of upper bounds */
   real64  *bcol;           /**< pointer to array of RHS b vector */
   char    *typerow;        /**< pointer to array of variable types */
   char    *relopcol;       /**< pointer to array of relational operators i.e. <=, >=, =  */

} mps_data_t;



/**< _____________________________________________________________________ */

/**< define solver variable types */
/**< note: 0 not defined since default value for sparse matrix, and so a value of 
         0 will imply an invalid row/col is being accessed */
#define MPS_VAR     1          /**< original solver_var, or some other refinement */
#define MPS_RELAXED 2          /**< something else, but are working on a relaxed  */
                                  /**< problem, so treat it as a regular solver_var  */
#define MPS_INT     3          /**< integer var, refines solver_var */
#define MPS_BINARY  4          /**< binary var, refines solver_int */
#define MPS_SEMI    5          /**< semicontinuos solver_var, refines solver_var */
#define MPS_FIXED   6          /**< a fixed or nonincident var */

#define MPS_BINARY_STR "boolean_var"
#define MPS_INT_STR "solver_int"
#define MPS_SEMI_STR "solver_semi"
#define MPS_VAR_STR "solver_var"

#endif

