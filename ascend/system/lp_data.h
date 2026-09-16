#ifndef ASC_LP_DATA_H
#define ASC_LP_DATA_H

#include <ascend/general/platform.h>
#include <ascend/linear/mtx.h>

typedef struct mps_data {
	int32 rused;
	int32 rinc;
	int32 crow;
	int32 vused;
	int32 vinc;
	int32 cap;
	int32 rank;
	int32 bused;

	int solver_var_used;
	int solver_relaxed_used;
	int solver_int_used;
	int solver_binary_used;
	int solver_semi_used;
	int solver_other_used;
	int solver_fixed;

	mtx_matrix_t Ac_mtx;

	real64 *lbrow;
	real64 *ubrow;
	real64 *bcol;
	real64 *col_scale;
	real64 *row_scale;
	char *typerow;
	char *relopcol;
} mps_data_t;

#define MPS_VAR 1
#define MPS_RELAXED 2
#define MPS_INT 3
#define MPS_BINARY 4
#define MPS_SEMI 5
#define MPS_FIXED 6

#define MPS_BINARY_STR "solver_binary"
#define MPS_INT_STR "solver_int"
#define MPS_SEMI_STR "solver_semi"
#define MPS_VAR_STR "solver_var"

#endif
