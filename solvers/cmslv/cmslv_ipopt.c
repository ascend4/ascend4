#include <math.h>
#include <string.h>

#include <ascend/utilities/config.h>

#ifdef ASC_WITH_IPOPT

#include <IpStdCInterface.h>

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>
#include <ascend/system/var.h>

#include "cmslv.h"

struct slv9_ipopt_bnd_data {
  slv9_system_t sys;
  int32 num_vars;
  int32 num_eqns;
};

static Bool slv9_ipopt_eval_f(Index n, Number *x, Bool new_x,
    Number *obj_value, UserDataPtr user_data
){
  struct slv9_ipopt_bnd_data *D = (struct slv9_ipopt_bnd_data *)user_data;
  real64 obj = 0.0;
  int32 v;
  (void)n;
  (void)new_x;
  for(v=0; v<D->num_vars; ++v) {
    obj += x[v] * x[v];
  }
  *obj_value = obj;
  return TRUE;
}

static Bool slv9_ipopt_eval_grad_f(Index n, Number *x, Bool new_x,
    Number *grad_f, UserDataPtr user_data
){
  struct slv9_ipopt_bnd_data *D = (struct slv9_ipopt_bnd_data *)user_data;
  int32 v;
  (void)n;
  (void)new_x;
  for(v=0; v<D->num_vars; ++v) {
    grad_f[v] = 2.0 * x[v];
  }
  for(v=D->num_vars; v<n; ++v) {
    grad_f[v] = 0.0;
  }
  return TRUE;
}

static Bool slv9_ipopt_eval_g(Index n, Number *x, Bool new_x, Index m,
    Number *g, UserDataPtr user_data
){
  struct slv9_ipopt_bnd_data *D = (struct slv9_ipopt_bnd_data *)user_data;
  slv9_system_t sys = D->sys;
  int32 row, s;
  (void)n;
  (void)new_x;
  (void)m;

  for(row=0; row<D->num_vars; ++row) {
    g[row] = -x[row];
    for(s=0; s<sys->subregions; ++s) {
      g[row] -= sys->coeff_matrix->cols[s].element[row] * x[D->num_vars + s];
    }
  }
  g[D->num_vars] = 0.0;
  for(s=0; s<sys->subregions; ++s) {
    g[D->num_vars] += x[D->num_vars + s];
  }
  return TRUE;
}

static Bool slv9_ipopt_eval_jac_g(Index n, Number *x, Bool new_x, Index m,
    Index nele_jac, Index *iRow, Index *jCol, Number *values,
    UserDataPtr user_data
){
  struct slv9_ipopt_bnd_data *D = (struct slv9_ipopt_bnd_data *)user_data;
  slv9_system_t sys = D->sys;
  int32 nz = 0;
  int32 col, row, s;
  real64 deriv;
  (void)n;
  (void)x;
  (void)new_x;
  (void)m;
  (void)nele_jac;

  if(values == NULL) {
    for(col=0; col<D->num_vars; ++col) {
      iRow[nz] = col;
      jCol[nz] = col;
      nz++;
    }
    for(s=0; s<sys->subregions; ++s) {
      col = D->num_vars + s;
      for(row=0; row<D->num_eqns; ++row) {
        iRow[nz] = row;
        jCol[nz] = col;
        nz++;
      }
    }
  }else{
    for(col=0; col<D->num_vars; ++col) {
      values[nz++] = -1.0;
    }
    for(s=0; s<sys->subregions; ++s) {
      for(row=0; row<D->num_vars; ++row) {
        deriv = -sys->coeff_matrix->cols[s].element[row];
        if(deriv > RTMAXJ) {
          deriv = 0.5 * RTMAXJ;
        }else if(deriv < -RTMAXJ) {
          deriv = -0.5 * RTMAXJ;
        }
        values[nz++] = deriv;
      }
      values[nz++] = 1.0;
    }
  }
  return TRUE;
}

static Bool slv9_ipopt_eval_h(Index n, Number *x, Bool new_x,
    Number obj_factor, Index m, Number *lambda, Bool new_lambda,
    Index nele_hess, Index *iRow, Index *jCol, Number *values,
    UserDataPtr user_data
){
  struct slv9_ipopt_bnd_data *D = (struct slv9_ipopt_bnd_data *)user_data;
  int32 v;
  (void)n;
  (void)x;
  (void)new_x;
  (void)m;
  (void)lambda;
  (void)new_lambda;
  (void)nele_hess;

  if(values == NULL) {
    for(v=0; v<D->num_vars; ++v) {
      iRow[v] = v;
      jCol[v] = v;
    }
  }else{
    for(v=0; v<D->num_vars; ++v) {
      values[v] = 2.0 * obj_factor;
    }
  }
  return TRUE;
}

int32 slv9_bnd_iterate_ipopt(slv9_system_t sys, int32 num_opt_vars,
    int32 num_opt_eqns, int32 num_vars, real64 *obj_val
){
  struct slv9_ipopt_bnd_data D;
  IpoptProblem prob;
  enum ApplicationReturnStatus status;
  Number *x = NULL, *x_L = NULL, *x_U = NULL, *g_L = NULL, *g_U = NULL;
  struct var_variable **varlist;
  struct var_variable *var;
  int32 c, count, totvar;
  int32 nele_jac;
  real64 nominal, low, up, uplow, limit;
  static var_filter_t vfilter = {
      VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR | VAR_FIXED
     ,VAR_ACTIVE_AT_BND | VAR_INCIDENT | VAR_SVAR | 0
  };

  x = ASC_NEW_ARRAY(Number,num_opt_vars);
  x_L = ASC_NEW_ARRAY(Number,num_opt_vars);
  x_U = ASC_NEW_ARRAY(Number,num_opt_vars);
  g_L = ASC_NEW_ARRAY(Number,num_opt_eqns);
  g_U = ASC_NEW_ARRAY(Number,num_opt_eqns);
  if(x == NULL || x_L == NULL || x_U == NULL || g_L == NULL || g_U == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"CMSlv IPOPT boundary solve memory allocation failed.");
    goto fail;
  }

  varlist = sys->mvlist;
  totvar = sys->mvtot;
  limit = ASC_INFINITY;

  count = 0;
  for(c=0; c<totvar; ++c) {
    var = varlist[c];
    if(var_apply_filter(var,&vfilter)) {
      nominal = var_nominal(var);
      low = var_lower_bound(var);
      up = var_upper_bound(var);
      uplow = fabs(up - low);
      x_L[count] = (-uplow > -limit) ? -uplow : -0.5 * limit;
      x_U[count] = (uplow < limit) ? uplow : 0.5 * limit;
      x[count] = 0.5 * nominal;
      count++;
    }
  }
  for(c=count; c<num_opt_vars; ++c) {
    x_L[c] = 0.0;
    x_U[c] = 1.0;
    x[c] = sys->subregions > 0 ? 1.0 / sys->subregions : 1.0;
  }
  if(count != num_vars) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,
      "CMSlv IPOPT boundary variable count mismatch (%d != %d).",
      count,num_vars
    );
    goto fail;
  }
  for(c=0; c<num_opt_eqns; ++c) {
    g_L[c] = 0.0;
    g_U[c] = 0.0;
  }
  g_L[num_vars] = 1.0;
  g_U[num_vars] = 1.0;

  D.sys = sys;
  D.num_vars = num_vars;
  D.num_eqns = num_opt_eqns;
  nele_jac = num_vars + sys->subregions * num_opt_eqns;

  prob = CreateIpoptProblem(
    num_opt_vars, x_L, x_U,
    num_opt_eqns, g_L, g_U,
    nele_jac, num_vars, 0,
    slv9_ipopt_eval_f, slv9_ipopt_eval_g, slv9_ipopt_eval_grad_f,
    slv9_ipopt_eval_jac_g, slv9_ipopt_eval_h
  );
  if(prob == NULL) {
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"CMSlv IPOPT boundary problem creation failed.");
    goto fail;
  }

  AddIpoptIntOption(prob, "print_level", 0);
  AddIpoptIntOption(prob, "max_iter", OPT_ITER_LIMIT);
  AddIpoptNumOption(prob, "tol", OBJ_TOL);
  AddIpoptStrOption(prob, "mu_strategy", (char *)"adaptive");
  AddIpoptStrOption(prob, "fixed_variable_treatment", (char *)"make_constraint");
#ifdef ASC_WITH_IPOPT_HSLIB
  AddIpoptStrOption(prob, "hsllib", ASC_IPOPT_HSL_LIBRARY);
#endif

  status = IpoptSolve(prob, x, NULL, obj_val, NULL, NULL, NULL, (UserDataPtr)&D);
  FreeIpoptProblem(prob);

  if(status != Solve_Succeeded
      && status != Solved_To_Acceptable_Level
      && status != Feasible_Point_Found
  ) {
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,
      "CMSlv IPOPT boundary solve returned status %d.",(int)status
    );
  }

  for(c=0; c<num_opt_vars; ++c) {
    sys->opt_var_values->element[c] = x[c];
  }

  ASC_FREE(g_U);
  ASC_FREE(g_L);
  ASC_FREE(x_U);
  ASC_FREE(x_L);
  ASC_FREE(x);
  return 1;

fail:
  if(g_U) ASC_FREE(g_U);
  if(g_L) ASC_FREE(g_L);
  if(x_U) ASC_FREE(x_U);
  if(x_L) ASC_FREE(x_L);
  if(x) ASC_FREE(x);
  return 0;
}

#endif
