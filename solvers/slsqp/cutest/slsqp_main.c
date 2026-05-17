/*
 * CUTEst driver for NLopt SLSQP.
 *
 * This file is built by CUTEst's package mechanism, not by normal ASCEND
 * builds. It translates CUTEst problems into dense NLopt callbacks.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nlopt.h>

#define GENCMA

#ifdef __cplusplus
extern "C" {
#endif

#include "cutest.h"
#include "cutest_routines.h"

struct SlsqpCutestConstraint {
	integer row;
	double sign;
	double rhs;
};

struct SlsqpCutestContext {
	integer n;
	integer m;
	logical constrained;
	logical noobj;
	rp_ *c_work;
	integer jac_capacity;
	rp_ *jac_values;
	integer *jac_rows;
	integer *jac_cols;
	struct SlsqpCutestConstraint *eq;
	struct SlsqpCutestConstraint *ineq;
	integer n_eq;
	integer n_ineq;
	int obj_evals;
	int grad_evals;
	int con_evals;
	int jac_evals;
	nlopt_opt opt;
	const char *callback_error;
};

static void slsqp_json_string(const char *s){
	const unsigned char *p = (const unsigned char *)(s != NULL ? s : "");
	putchar('"');
	while(*p != '\0'){
		if(*p == '"' || *p == '\\'){
			putchar('\\');
			putchar((int)*p);
		}else if(*p >= 32 && *p < 127){
			putchar((int)*p);
		}else{
			printf("\\u%04x",(unsigned int)*p);
		}
		++p;
	}
	putchar('"');
}

static void slsqp_json_number(double value){
	if(isfinite(value)){
		printf("%.17g",value);
	}else{
		printf("null");
	}
}

static int slsqp_env_int(const char *name, int fallback){
	const char *value = getenv(name);
	char *end = NULL;
	long parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtol(value,&end,10);
	return end != value ? (int)parsed : fallback;
}

static double slsqp_env_double(const char *name, double fallback){
	const char *value = getenv(name);
	char *end = NULL;
	double parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtod(value,&end);
	return end != value && isfinite(parsed) ? parsed : fallback;
}

static double slsqp_objective_cb(unsigned n, const double *x, double *grad, void *data){
	struct SlsqpCutestContext *ctx = (struct SlsqpCutestContext *)data;
	integer status = 0;
	logical need_grad = grad != NULL ? TRUE_ : FALSE_;
	rp_ f = 0.0;
	if(ctx == NULL || (integer)n != ctx->n){
		return HUGE_VAL;
	}
	if(ctx->noobj){
		if(grad != NULL){
			memset(grad,0,(size_t)n * sizeof(*grad));
		}
		return 0.0;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)(grad != NULL ? grad : ctx->jac_values),&need_grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)(grad != NULL ? grad : ctx->jac_values),&need_grad);
	}
	++ctx->obj_evals;
	if(grad != NULL){
		++ctx->grad_evals;
	}
	if(status != 0){
		ctx->callback_error = "objective failed";
		nlopt_force_stop(ctx->opt);
		return HUGE_VAL;
	}
	return f;
}

static int slsqp_eval_constraints(struct SlsqpCutestContext *ctx, const double *x, double *dense_grad){
	integer status = 0;
	logical grad = dense_grad != NULL ? TRUE_ : FALSE_;
	integer nnz = 0;
	integer i;
	if(ctx == NULL || !ctx->constrained){
		return 0;
	}
	if(dense_grad != NULL){
		memset(dense_grad,0,(size_t)ctx->m * (size_t)ctx->n * sizeof(*dense_grad));
	}
	CUTEST_ccfsg(
		&status,
		&ctx->n,
		&ctx->m,
		(const rp_ *)x,
		ctx->c_work,
		&nnz,
		&ctx->jac_capacity,
		ctx->jac_values,
		ctx->jac_cols,
		ctx->jac_rows,
		&grad
	);
	if(status != 0 || nnz > ctx->jac_capacity){
		return 1;
	}
	if(dense_grad != NULL){
		for(i = 0; i < nnz; ++i){
			integer row = ctx->jac_rows[i] - 1;
			integer col = ctx->jac_cols[i] - 1;
			if(row >= 0 && row < ctx->m && col >= 0 && col < ctx->n){
				dense_grad[(size_t)row * (size_t)ctx->n + (size_t)col] = ctx->jac_values[i];
			}
		}
	}
	return 0;
}

static void slsqp_constraint_cb(
	unsigned m,
	double *result,
	unsigned n,
	const double *x,
	double *grad,
	void *data,
	const struct SlsqpCutestConstraint *map
){
	struct SlsqpCutestContext *ctx = (struct SlsqpCutestContext *)data;
	double *dense_grad = NULL;
	unsigned i;
	if(ctx == NULL || result == NULL || (integer)n != ctx->n){
		return;
	}
	if(grad != NULL){
		dense_grad = (double *)malloc(sizeof(double) * (size_t)ctx->m * (size_t)ctx->n);
		if(dense_grad == NULL){
			ctx->callback_error = "constraint dense gradient allocation failed";
			nlopt_force_stop(ctx->opt);
			return;
		}
	}
	if(slsqp_eval_constraints(ctx,x,dense_grad)){
		free(dense_grad);
		ctx->callback_error = "constraint evaluation failed";
		nlopt_force_stop(ctx->opt);
		return;
	}
	++ctx->con_evals;
	if(grad != NULL){
		memset(grad,0,(size_t)m * (size_t)n * sizeof(*grad));
		++ctx->jac_evals;
	}
	for(i = 0; i < m; ++i){
		const struct SlsqpCutestConstraint *ci = &map[i];
		integer row = ci->row;
		unsigned j;
		result[i] = ci->sign * (ctx->c_work[row] - ci->rhs);
		if(grad != NULL){
			for(j = 0; j < n; ++j){
				grad[(size_t)i * (size_t)n + j] = ci->sign * dense_grad[(size_t)row * (size_t)n + j];
			}
		}
	}
	free(dense_grad);
}

static void slsqp_eq_cb(unsigned m, double *result, unsigned n, const double *x, double *grad, void *data){
	struct SlsqpCutestContext *ctx = (struct SlsqpCutestContext *)data;
	slsqp_constraint_cb(m,result,n,x,grad,data,ctx != NULL ? ctx->eq : NULL);
}

static void slsqp_ineq_cb(unsigned m, double *result, unsigned n, const double *x, double *grad, void *data){
	struct SlsqpCutestContext *ctx = (struct SlsqpCutestContext *)data;
	slsqp_constraint_cb(m,result,n,x,grad,data,ctx != NULL ? ctx->ineq : NULL);
}

static double slsqp_max_violation(
	struct SlsqpCutestContext *ctx,
	const double *x,
	const double *x_l,
	const double *x_u
){
	double maxv = 0.0;
	integer i;
	if(ctx == NULL || x == NULL){
		return HUGE_VAL;
	}
	if(ctx->constrained && slsqp_eval_constraints(ctx,x,NULL)){
		return HUGE_VAL;
	}
	for(i = 0; i < ctx->n; ++i){
		if(x_l[i] > -1e19 && x[i] < x_l[i]){
			maxv = fmax(maxv,x_l[i] - x[i]);
		}
		if(x_u[i] < 1e19 && x[i] > x_u[i]){
			maxv = fmax(maxv,x[i] - x_u[i]);
		}
	}
	for(i = 0; i < ctx->n_eq; ++i){
		struct SlsqpCutestConstraint *ci = &ctx->eq[i];
		maxv = fmax(maxv,fabs(ctx->c_work[ci->row] - ci->rhs));
	}
	for(i = 0; i < ctx->n_ineq; ++i){
		struct SlsqpCutestConstraint *ci = &ctx->ineq[i];
		maxv = fmax(maxv,fmax(0.0,ci->sign * (ctx->c_work[ci->row] - ci->rhs)));
	}
	return maxv;
}

int MAINENTRY(void){
	char *fname = "OUTSDIF.d";
	integer funit = 42;
	integer iout = 6;
	integer io_buffer = 11;
	integer status = 0;
	integer ierr = 0;
	integer e_order = 1;
	integer l_order = 0;
	integer v_order = 0;
	integer nnzj_raw = 0;
	integer nonlin_obj = 0;
	integer nonlin_con = 0;
	integer neq = 0;
	integer nlinear = 0;
	rp_ *x = NULL;
	rp_ *x_l = NULL;
	rp_ *x_u = NULL;
	rp_ *lambda = NULL;
	rp_ *g_l = NULL;
	rp_ *g_u = NULL;
	logical *equatn = NULL;
	logical *linear = NULL;
	rp_ calls[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	rp_ cpu[4] = {0.0,0.0,0.0,0.0};
	char *pname = NULL;
	char *classification = NULL;
	struct SlsqpCutestContext ctx;
	nlopt_result result;
	double obj = 0.0;
	double maxv = 0.0;
	double constraint_tol = slsqp_env_double("SLSQP_CONSTRAINT_TOL",1e-8);
	int finite_lower = 0;
	int finite_upper = 0;
	integer i;

	memset(&ctx,0,sizeof(ctx));
	FORTRAN_open(&funit,fname,&ierr);
	if(ierr != 0){
		fprintf(stderr,"SLSQP-CUTEst: failed to open OUTSDIF.d\n");
		return 1;
	}
	CUTEST_cdimen(&status,&funit,&ctx.n,&ctx.m);
	if(status != 0){
		fprintf(stderr,"SLSQP-CUTEst: CUTEST_cdimen status=%d\n",(int)status);
		return (int)status;
	}
	ctx.constrained = ctx.m > 0 ? TRUE_ : FALSE_;
	CUTEST_cnoobj(&status,&funit,&ctx.noobj);
	if(status != 0){
		ctx.noobj = FALSE_;
	}
	MALLOC(classification,FCSTRING_LEN + 1,char);
	MALLOC(pname,FSTRING_LEN + 1,char);
	MALLOC(x,ctx.n,rp_);
	MALLOC(x_l,ctx.n,rp_);
	MALLOC(x_u,ctx.n,rp_);
	MALLOC(ctx.jac_values,ctx.n > 0 ? ctx.n : 1,rp_);
	if(classification == NULL || pname == NULL || x == NULL || x_l == NULL || x_u == NULL || ctx.jac_values == NULL){
		fprintf(stderr,"SLSQP-CUTEst: allocation failure\n");
		return 2;
	}
	CUTEST_classification(&status,&funit,classification);
	if(status != 0){
		strcpy(classification,"unknown");
	}else{
		classification[FCSTRING_LEN] = '\0';
	}
	if(ctx.constrained){
		MALLOC(lambda,ctx.m,rp_);
		MALLOC(g_l,ctx.m,rp_);
		MALLOC(g_u,ctx.m,rp_);
		MALLOC(ctx.c_work,ctx.m,rp_);
		MALLOC(equatn,ctx.m,logical);
		MALLOC(linear,ctx.m,logical);
		MALLOC(ctx.eq,ctx.m,struct SlsqpCutestConstraint);
		MALLOC(ctx.ineq,2 * ctx.m,struct SlsqpCutestConstraint);
		if(lambda == NULL || g_l == NULL || g_u == NULL || ctx.c_work == NULL
			|| equatn == NULL || linear == NULL || ctx.eq == NULL || ctx.ineq == NULL
		){
			fprintf(stderr,"SLSQP-CUTEst: constrained allocation failure\n");
			return 2;
		}
		CUTEST_csetup(&status,&funit,&iout,&io_buffer,&ctx.n,&ctx.m,x,x_l,x_u,lambda,g_l,g_u,equatn,linear,&e_order,&l_order,&v_order);
		CUTEST_cdimsj(&status,&nnzj_raw);
		ctx.jac_capacity = nnzj_raw - ctx.n;
		if(ctx.jac_capacity < 1){
			ctx.jac_capacity = 1;
		}
		FREE(ctx.jac_values);
		MALLOC(ctx.jac_values,ctx.jac_capacity,rp_);
		MALLOC(ctx.jac_rows,ctx.jac_capacity,integer);
		MALLOC(ctx.jac_cols,ctx.jac_capacity,integer);
		if(ctx.jac_values == NULL || ctx.jac_rows == NULL || ctx.jac_cols == NULL){
			fprintf(stderr,"SLSQP-CUTEst: Jacobian allocation failure\n");
			return 2;
		}
		CUTEST_cstats(&status,&nonlin_obj,&nonlin_con,&neq,&nlinear);
		for(i = 0; i < ctx.m; ++i){
			if(equatn[i]){
				ctx.eq[ctx.n_eq].row = i;
				ctx.eq[ctx.n_eq].sign = 1.0;
				ctx.eq[ctx.n_eq].rhs = g_l[i];
				++ctx.n_eq;
			}else{
				if(g_u[i] < 1e19){
					ctx.ineq[ctx.n_ineq].row = i;
					ctx.ineq[ctx.n_ineq].sign = 1.0;
					ctx.ineq[ctx.n_ineq].rhs = g_u[i];
					++ctx.n_ineq;
				}
				if(g_l[i] > -1e19){
					ctx.ineq[ctx.n_ineq].row = i;
					ctx.ineq[ctx.n_ineq].sign = -1.0;
					ctx.ineq[ctx.n_ineq].rhs = g_l[i];
					++ctx.n_ineq;
				}
			}
		}
	}else{
		CUTEST_usetup(&status,&funit,&iout,&io_buffer,&ctx.n,x,x_l,x_u);
	}
	if(status != 0){
		fprintf(stderr,"SLSQP-CUTEst: setup status=%d\n",(int)status);
		return (int)status;
	}
	CUTEST_pname(&status,&funit,pname);
	if(status != 0){
		strcpy(pname,"unknown");
	}
	pname[FSTRING_LEN] = '\0';
	for(i = 0; i < ctx.n; ++i){
		if(x_l[i] > -1e19){
			++finite_lower;
		}else{
			x_l[i] = -HUGE_VAL;
		}
		if(x_u[i] < 1e19){
			++finite_upper;
		}else{
			x_u[i] = HUGE_VAL;
		}
		if(isfinite(x_l[i]) && x[i] < x_l[i]){
			x[i] = x_l[i];
		}
		if(isfinite(x_u[i]) && x[i] > x_u[i]){
			x[i] = x_u[i];
		}
	}

	ctx.opt = nlopt_create(NLOPT_LD_SLSQP,(unsigned)ctx.n);
	if(ctx.opt == NULL){
		fprintf(stderr,"SLSQP-CUTEst: nlopt_create failed\n");
		return 3;
	}
	nlopt_set_min_objective(ctx.opt,slsqp_objective_cb,&ctx);
	nlopt_set_lower_bounds(ctx.opt,(const double *)x_l);
	nlopt_set_upper_bounds(ctx.opt,(const double *)x_u);
	nlopt_set_maxeval(ctx.opt,slsqp_env_int("SLSQP_MAX_ITER",200));
	nlopt_set_maxtime(ctx.opt,slsqp_env_double("SLSQP_MAX_TIME",0.0));
	nlopt_set_ftol_rel(ctx.opt,slsqp_env_double("SLSQP_FTOL_REL",1e-8));
	nlopt_set_ftol_abs(ctx.opt,slsqp_env_double("SLSQP_FTOL_ABS",0.0));
	nlopt_set_xtol_rel(ctx.opt,slsqp_env_double("SLSQP_XTOL_REL",1e-8));
	nlopt_set_xtol_abs1(ctx.opt,slsqp_env_double("SLSQP_XTOL_ABS",0.0));
	if(ctx.n_eq > 0){
		double *tol = (double *)malloc(sizeof(double) * (size_t)ctx.n_eq);
		for(i = 0; i < ctx.n_eq; ++i){
			tol[i] = constraint_tol;
		}
		nlopt_add_equality_mconstraint(ctx.opt,(unsigned)ctx.n_eq,slsqp_eq_cb,&ctx,tol);
		free(tol);
	}
	if(ctx.n_ineq > 0){
		double *tol = (double *)malloc(sizeof(double) * (size_t)ctx.n_ineq);
		for(i = 0; i < ctx.n_ineq; ++i){
			tol[i] = constraint_tol;
		}
		nlopt_add_inequality_mconstraint(ctx.opt,(unsigned)ctx.n_ineq,slsqp_ineq_cb,&ctx,tol);
		free(tol);
	}
	result = nlopt_optimize(ctx.opt,(double *)x,&obj);
	maxv = slsqp_max_violation(&ctx,(const double *)x,(const double *)x_l,(const double *)x_u);
	if(ctx.constrained){
		CUTEST_creport(&status,calls,cpu);
		CUTEST_cterminate(&status);
	}else{
		CUTEST_ureport(&status,calls,cpu);
		CUTEST_uterminate(&status);
	}

	printf("{");
	printf("\"solver\":\"SLSQP\",");
	printf("\"problem\":");
	slsqp_json_string(pname);
	printf(",\"classification\":");
	slsqp_json_string(classification);
	printf(",\"n\":%d,\"m\":%d,\"equalities\":%d,\"linear_constraints\":%d,",
		(int)ctx.n,(int)ctx.m,(int)neq,(int)nlinear
	);
	printf("\"finite_var_lower\":%d,\"finite_var_upper\":%d,",finite_lower,finite_upper);
	printf("\"status\":%d,\"status_name\":",(int)result);
	slsqp_json_string(nlopt_result_to_string(result));
	printf(",\"objective\":");
	slsqp_json_number(obj);
	printf(",\"max_constraint_violation\":");
	slsqp_json_number(maxv);
	printf(",\"iterations\":%d,",nlopt_get_numevals(ctx.opt));
	printf("\"obj_evals\":%d,\"grad_evals\":%d,\"con_evals\":%d,\"jac_evals\":%d,",
		ctx.obj_evals,ctx.grad_evals,ctx.con_evals,ctx.jac_evals
	);
	printf("\"cutest_calls_obj\":%.17g,\"cutest_calls_objgrad\":%.17g,\"cutest_calls_con\":%.17g,\"cutest_calls_congrad\":%.17g,",
		(double)calls[0],(double)calls[1],(double)calls[4],(double)calls[5]
	);
	printf("\"cutest_setup_time\":%.17g,\"cutest_solve_time\":%.17g,",(double)cpu[0],(double)cpu[1]);
	printf("\"callback_error\":");
	slsqp_json_string(ctx.callback_error != NULL ? ctx.callback_error : "");
	printf("}\n");

	nlopt_destroy(ctx.opt);
	FREE(x);
	FREE(x_l);
	FREE(x_u);
	FREE(lambda);
	FREE(g_l);
	FREE(g_u);
	FREE(equatn);
	FREE(linear);
	FREE(ctx.c_work);
	FREE(ctx.jac_values);
	FREE(ctx.jac_rows);
	FREE(ctx.jac_cols);
	FREE(ctx.eq);
	FREE(ctx.ineq);
	FREE(pname);
	FREE(classification);
	return result > 0 ? 0 : 1;
}

#ifdef __cplusplus
}
#endif
