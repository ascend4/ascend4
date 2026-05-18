#ifndef ASC_CMSLV_INTERNAL_H
#define ASC_CMSLV_INTERNAL_H

#include <ascend/general/platform.h>
#include <ascend/solver/solver.h>
#include <ascend/linear/mtx.h>

#ifdef ASC_WITH_CONOPT
# include <ascend/solver/conopt_dl.h>
#endif

typedef struct slv9_system_structure *slv9_system_t;

#define SOLVER_CMSLV 9

#define slv9_PA_SIZE 28 /* MUST INCREMENT WHEN ADDING PARAMETERS */
#define LOGSOLVER_OPTION_PTR (sys->parm_array[0])
#define LOGSOLVER_OPTION  ((*(char **)LOGSOLVER_OPTION_PTR))
#define NONLISOLVER_OPTION_PTR (sys->parm_array[1])
#define NONLISOLVER_OPTION  ((*(char **)NONLISOLVER_OPTION_PTR))
#define OPTSOLVER_OPTION_PTR (sys->parm_array[2])
#define OPTSOLVER_OPTION  ((*(char **)OPTSOLVER_OPTION_PTR))
#define TIME_LIMIT_PTR (sys->parm_array[3])
#define TIME_LIMIT     ((*(int32 *)TIME_LIMIT_PTR))
#define ITER_LIMIT_PTR (sys->parm_array[4])
#define ITER_LIMIT     ((*(int32 *)ITER_LIMIT_PTR))
#define ITER_BIS_LIMIT_PTR (sys->parm_array[5])
#define ITER_BIS_LIMIT  ((*(int32 *)ITER_BIS_LIMIT_PTR))
#define TOO_SMALL_PTR (sys->parm_array[6])
#define TOO_SMALL     ((*(real64 *)TOO_SMALL_PTR))
#define LINEAR_SEARCH_FACTOR_PTR (sys->parm_array[7])
#define LINEAR_SEARCH_FACTOR  ((*(real64 *)LINEAR_SEARCH_FACTOR_PTR))
#define SHOW_MORE_IMPT_PTR (sys->parm_array[8])
#define SHOW_MORE_IMPT     ((*(int32 *)SHOW_MORE_IMPT_PTR))
#define SHOW_LESS_IMPT_PTR (sys->parm_array[9])
#define SHOW_LESS_IMPT     ((*(int32 *)SHOW_LESS_IMPT_PTR))
#define AUTO_RESOLVE_PTR (sys->parm_array[10])
#define AUTO_RESOLVE     ((*(int32 *)AUTO_RESOLVE_PTR))
#define UNDEFINED_PTR (sys->parm_array[11])
#define UNDEFINED  ((*(real64 *)UNDEFINED_PTR))
#define DOMLIM_PTR (sys->parm_array[12])
#define DOMLIM     ((*(int32 *)DOMLIM_PTR))
#define OPT_ITER_LIMIT_PTR (sys->parm_array[13])
#define OPT_ITER_LIMIT     ((*(int32 *)OPT_ITER_LIMIT_PTR))
#define INFINITY_PTR (sys->parm_array[14])
#define ASC_INFINITY  ((*(real64 *)INFINITY_PTR))
#define OBJ_TOL_PTR (sys->parm_array[15])
#define OBJ_TOL  ((*(real64 *)OBJ_TOL_PTR))
#define RTMAXJ_PTR (sys->parm_array[16])
#define RTMAXJ     ((*(real64 *)RTMAXJ_PTR))
#define RHO_PTR (sys->parm_array[17])
#define RHO     ((*(real64 *)RHO_PTR))
#define PROGRESS_CALLBACKS_PTR (sys->parm_array[18])
#define PROGRESS_CALLBACKS ((*(int32 *)PROGRESS_CALLBACKS_PTR))
#define PROGRESS_LOG_PTR (sys->parm_array[19])
#define PROGRESS_LOG ((*(int32 *)PROGRESS_LOG_PTR))

#ifndef CONOPT_BOUNDLIMIT
# define CONOPT_BOUNDLIMIT 3.1e9
#endif
#ifndef MAX_INT
# define MAX_INT 20000
#endif
#ifndef MAX_REAL
# define MAX_REAL 10e300
#endif

struct opt_vector {
  real64 *element;
};

struct opt_matrix {
  struct opt_vector *cols;
};

struct boolean_values {
  int32 *pre_val;
  int32 *cur_val;
};

struct matching_cases {
  int32 *case_list;
  int32 ncases;
  int32 diff_subregion;
};

struct real_values {
  real64 *pre_values;
  real64 *cur_values;
};

struct subregionID {
  unsigned long ID_number;
  int32 *bool_values;
};

struct ds_subregion_list {
  int32 length,capacity;
  struct subregionID *sub_stack;
};

struct ds_subregions_visited {
  int32 length,capacity;
  unsigned long *visited;
};

struct slv9_system_structure {
  slv_system_t slv;

  struct rel_relation *obj;
  struct var_variable **vlist;
  struct rel_relation **rlist;
  struct dis_discrete **dvlist;
  struct logrel_relation **lrlist;
  struct bnd_boundary **blist;
  struct var_variable **mvlist;
  struct dis_discrete **mdvlist;

  struct opt_matrix *coeff_matrix;
  struct opt_vector *opt_var_values;
  int32 subregions;
  mtx_matrix_t lin_mtx;

  struct ds_subregion_list subregion_list;
  struct ds_subregions_visited subregions_visited;
  int32 *bool_mindex;
  int32 need_consistency_analysis;

  int32 integrity;
  int32 presolved;
  int32 solvers_ready;
  slv_parameters_t p;
  slv_status_t s;
  int32 cap;
  int32 rank;
  int32 vused;
  int32 vtot;
  int32 mvtot;
  int32 rused;
  int32 rtot;
  real64 clock;
  int32 nliter;

  void *parm_array[slv9_PA_SIZE];
  struct slv_parameter pa[slv9_PA_SIZE];

#ifdef ASC_WITH_CONOPT
  struct conopt_data con;
#endif
};

#ifdef ASC_WITH_IPOPT
int32 slv9_bnd_iterate_ipopt(slv9_system_t sys, int32 num_opt_vars,
  int32 num_opt_eqns, int32 num_vars, real64 *obj_val);
#endif

#endif
