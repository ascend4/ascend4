/*
 * CUTEst driver for the IPOPT-like A4SQP C API.
 *
 * This file is intended to be built by CUTEst's package mechanism. It does not
 * participate in normal ASCEND builds.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GENCMA

#ifdef __cplusplus
extern "C" {
#endif

#include "cutest.h"
#include "cutest_routines.h"
#include "solvers/a4sqp/a4sqp_c.h"

struct A4SqpCutestContext {
	integer n;
	integer m;
	integer nele_jac;
	integer nele_hess;
	logical constrained;
	logical noobj;
	rp_ *c_work;
	rp_ *g_work;
	rp_ *hess_work;
	rp_ *hess_x_work;
	rp_ *lambda_work;
	integer *hess_row;
	integer *hess_col;
};

static void a4sqp_cutest_json_string(const char *s){
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

static void a4sqp_cutest_json_number(double value){
	if(isfinite(value)){
		printf("%.17g",value);
	}else{
		printf("null");
	}
}

static int a4sqp_cutest_env_int(const char *name, int fallback){
	const char *value = getenv(name);
	char *end = NULL;
	long parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtol(value,&end,10);
	if(end == value){
		return fallback;
	}
	return (int)parsed;
}

static double a4sqp_cutest_env_double(const char *name, double fallback){
	const char *value = getenv(name);
	char *end = NULL;
	double parsed;
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	parsed = strtod(value,&end);
	if(end == value || !isfinite(parsed)){
		return fallback;
	}
	return parsed;
}

static const char *a4sqp_cutest_env_string(const char *name, const char *fallback){
	const char *value = getenv(name);
	if(value == NULL || *value == '\0'){
		return fallback;
	}
	return value;
}

static A4SqpBool a4sqp_cutest_eval_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *obj_value,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = FALSE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || obj_value == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->noobj){
		*obj_value = 0.0;
		return A4SQP_TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}
	if(status != 0){
		return A4SQP_FALSE;
	}
	*obj_value = f;
	return A4SQP_TRUE;
}

static A4SqpBool a4sqp_cutest_eval_grad_f(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber *grad_f,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || grad_f == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->noobj){
		memset(grad_f,0,(size_t)ctx->n * sizeof(*grad_f));
		return A4SQP_TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
}

static A4SqpBool a4sqp_cutest_eval_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpNumber *g,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical jtrans = FALSE_;
	logical grad = FALSE_;
	integer one = 1;
	rp_ scratch = 0.0;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL || g == NULL){
		return A4SQP_FALSE;
	}
	if(ctx->m <= 0){
		return A4SQP_TRUE;
	}
	CUTEST_ccfg(&status,&ctx->n,&ctx->m,(const rp_ *)x,(rp_ *)g,&jtrans,&one,&one,&scratch,&grad);
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
}

static A4SqpBool a4sqp_cutest_eval_jac_g(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpIndex m,
	A4SqpIndex nele_jac,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	integer nnzj = 0;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL || ctx->m <= 0 || nele_jac < ctx->nele_jac){
		return A4SQP_FALSE;
	}
	if(values == NULL){
		rp_ *jac = NULL;
		MALLOC(jac,ctx->nele_jac > 0 ? ctx->nele_jac : 1,rp_);
		if(jac == NULL){
			return A4SQP_FALSE;
		}
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			jac,
			(integer *)jCol,
			(integer *)iRow,
			&grad
		);
		FREE(jac);
	}else{
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			(rp_ *)values,
			(integer *)jCol,
			(integer *)iRow,
			&grad
		);
	}
	if(status != 0 || nnzj > ctx->nele_jac){
		return A4SQP_FALSE;
	}
	return A4SQP_TRUE;
}

static A4SqpBool a4sqp_cutest_eval_h(
	A4SqpIndex n,
	A4SqpNumber *x,
	A4SqpBool new_x,
	A4SqpNumber obj_factor,
	A4SqpIndex m,
	A4SqpNumber *lambda,
	A4SqpBool new_lambda,
	A4SqpIndex nele_hess,
	A4SqpIndex *iRow,
	A4SqpIndex *jCol,
	A4SqpNumber *values,
	A4SqpUserDataPtr user_data
){
	struct A4SqpCutestContext *ctx = (struct A4SqpCutestContext *)user_data;
	integer status = 0;
	integer nnzh = 0;
	rp_ *x_use;
	(void)n;
	(void)new_x;
	(void)m;
	(void)new_lambda;
	if(ctx == NULL || nele_hess < ctx->nele_hess){
		return A4SQP_FALSE;
	}
	x_use = x != NULL ? (rp_ *)x : ctx->hess_x_work;
	if(!ctx->constrained){
		CUTEST_ush(
			&status,
			&ctx->n,
			x_use,
			&nnzh,
			&ctx->nele_hess,
			values == NULL ? ctx->hess_work : (rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status != 0){
			return A4SQP_FALSE;
		}
		if(values == NULL){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				iRow[k] = (A4SqpIndex)ctx->hess_row[k];
				jCol[k] = (A4SqpIndex)ctx->hess_col[k];
			}
		}else if(obj_factor != 1.0){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				values[k] *= obj_factor;
			}
		}
		return A4SQP_TRUE;
	}
	if(values == NULL){
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = 0.0;
		}
		CUTEST_csh(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			ctx->hess_work,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status != 0){
			return A4SQP_FALSE;
		}
		for(integer k = 0; k < ctx->nele_hess; ++k){
			iRow[k] = (A4SqpIndex)ctx->hess_row[k];
			jCol[k] = (A4SqpIndex)ctx->hess_col[k];
		}
		return A4SQP_TRUE;
	}
	if(obj_factor == 0.0){
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = lambda != NULL ? (rp_)lambda[i] : 0.0;
		}
		CUTEST_cshc(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			(rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
	}else{
		for(integer i = 0; i < ctx->m; ++i){
			ctx->lambda_work[i] = lambda != NULL ? (rp_)(lambda[i] / obj_factor) : 0.0;
		}
		CUTEST_csh(
			&status,
			&ctx->n,
			&ctx->m,
			x_use,
			ctx->lambda_work,
			&nnzh,
			&ctx->nele_hess,
			(rp_ *)values,
			ctx->hess_row,
			ctx->hess_col
		);
		if(status == 0 && obj_factor != 1.0){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				values[k] *= obj_factor;
			}
		}
	}
	return status == 0 ? A4SQP_TRUE : A4SQP_FALSE;
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
	rp_ obj = 0.0;
	rp_ *g = NULL;
	rp_ calls[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	rp_ cpu[4] = {0.0,0.0,0.0,0.0};
	char *pname = NULL;
	char *classification = NULL;
	struct A4SqpCutestContext ctx;
	A4SqpProblem problem = NULL;
	struct A4SqpSolveStats stats;
	enum A4SqpApplicationReturnStatus solve_status;
	int finite_lower = 0;
	int finite_upper = 0;
	int i;

	memset(&ctx,0,sizeof(ctx));
	FORTRAN_open(&funit,fname,&ierr);
	if(ierr != 0){
		fprintf(stderr,"A4SQP-CUTEst: failed to open OUTSDIF.d\n");
		return 1;
	}
	CUTEST_cdimen(&status,&funit,&ctx.n,&ctx.m);
	if(status != 0){
		fprintf(stderr,"A4SQP-CUTEst: CUTEST_cdimen status=%d\n",(int)status);
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
	MALLOC(ctx.g_work,ctx.n,rp_);
	if(classification == NULL || pname == NULL || x == NULL || x_l == NULL || x_u == NULL || ctx.g_work == NULL){
		fprintf(stderr,"A4SQP-CUTEst: allocation failure\n");
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
		MALLOC(g,ctx.m,rp_);
		MALLOC(ctx.c_work,ctx.m,rp_);
		MALLOC(ctx.lambda_work,ctx.m,rp_);
		MALLOC(equatn,ctx.m,logical);
		MALLOC(linear,ctx.m,logical);
		if(lambda == NULL || g_l == NULL || g_u == NULL || g == NULL || ctx.c_work == NULL
			|| ctx.lambda_work == NULL || equatn == NULL || linear == NULL
		){
			fprintf(stderr,"A4SQP-CUTEst: constrained allocation failure\n");
			return 2;
		}
		CUTEST_csetup(
			&status,
			&funit,
			&iout,
			&io_buffer,
			&ctx.n,
			&ctx.m,
			x,
			x_l,
			x_u,
			lambda,
			g_l,
			g_u,
			equatn,
			linear,
			&e_order,
			&l_order,
			&v_order
		);
		CUTEST_cdimsj(&status,&nnzj_raw);
		ctx.nele_jac = nnzj_raw - ctx.n;
		if(ctx.nele_jac < 0){
			ctx.nele_jac = 0;
		}
		CUTEST_cdimsh(&status,&ctx.nele_hess);
		CUTEST_cstats(&status,&nonlin_obj,&nonlin_con,&neq,&nlinear);
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		if(ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL || ctx.hess_x_work == NULL){
			fprintf(stderr,"A4SQP-CUTEst: Hessian allocation failure\n");
			return 2;
		}
		for(i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}else{
		CUTEST_usetup(&status,&funit,&iout,&io_buffer,&ctx.n,x,x_l,x_u);
		ctx.nele_jac = 0;
		CUTEST_udimsh(&status,&ctx.nele_hess);
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		if(ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL || ctx.hess_x_work == NULL){
			fprintf(stderr,"A4SQP-CUTEst: unconstrained Hessian allocation failure\n");
			return 2;
		}
		for(i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}
	if(status != 0){
		fprintf(stderr,"A4SQP-CUTEst: setup status=%d\n",(int)status);
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
		}
		if(x_u[i] < 1e19){
			++finite_upper;
		}
	}
	problem = CreateA4SqpProblem(
		(A4SqpIndex)ctx.n,
		(double *)x_l,
		(double *)x_u,
		(A4SqpIndex)ctx.m,
		(double *)g_l,
		(double *)g_u,
		(A4SqpIndex)ctx.nele_jac,
		(A4SqpIndex)ctx.nele_hess,
		1,
		ctx.noobj ? NULL : a4sqp_cutest_eval_f,
		ctx.constrained ? a4sqp_cutest_eval_g : NULL,
		ctx.noobj ? NULL : a4sqp_cutest_eval_grad_f,
		ctx.constrained ? a4sqp_cutest_eval_jac_g : NULL,
		ctx.noobj ? NULL : a4sqp_cutest_eval_h
	);
	if(problem == NULL){
		fprintf(stderr,"A4SQP-CUTEst: CreateA4SqpProblem failed\n");
		return 3;
	}
	AddA4SqpIntOption(problem,"max_iter",a4sqp_cutest_env_int("A4SQP_MAX_ITER",200));
	AddA4SqpIntOption(problem,"max_backtrack",a4sqp_cutest_env_int("A4SQP_MAX_BACKTRACK",20));
	AddA4SqpIntOption(problem,"trust_qp_retries",a4sqp_cutest_env_int("A4SQP_TRUST_QP_RETRIES",5));
	AddA4SqpIntOption(problem,"print_level",a4sqp_cutest_env_int("A4SQP_PRINT_LEVEL",0));
	AddA4SqpNumOption(problem,"tol",a4sqp_cutest_env_double("A4SQP_TOL",1e-7));
	AddA4SqpNumOption(problem,"elastic_penalty",a4sqp_cutest_env_double("A4SQP_ELASTIC_PENALTY",100.0));
	AddA4SqpNumOption(problem,"trust_radius_init",a4sqp_cutest_env_double("A4SQP_TRUST_RADIUS_INIT",1.0));
	AddA4SqpNumOption(problem,"hess_reg",a4sqp_cutest_env_double("A4SQP_HESS_REG",1e-8));
	AddA4SqpStrOption(problem,"hessian",(char *)a4sqp_cutest_env_string("A4SQP_HESSIAN","BFGS"));
	solve_status = A4SqpSolve(
		problem,
		(double *)x,
		(double *)g,
		(double *)&obj,
		(double *)lambda,
		NULL,
		NULL,
		&ctx
	);
	GetA4SqpSolveStatistics(problem,&stats);
	if(ctx.constrained){
		CUTEST_creport(&status,calls,cpu);
		CUTEST_cterminate(&status);
	}else{
		CUTEST_ureport(&status,calls,cpu);
		CUTEST_uterminate(&status);
	}
	printf("{");
	printf("\"solver\":\"A4SQP\",");
	printf("\"problem\":");
	a4sqp_cutest_json_string(pname);
	printf(",");
	printf("\"classification\":");
	a4sqp_cutest_json_string(classification);
	printf(",");
	printf("\"n\":%d,\"m\":%d,\"equalities\":%d,\"linear_constraints\":%d,",
		(int)ctx.n,
		(int)ctx.m,
		(int)neq,
		(int)nlinear
	);
	printf("\"finite_var_lower\":%d,\"finite_var_upper\":%d,",finite_lower,finite_upper);
	printf("\"jac_nnz\":%d,\"hess_nnz\":%d,",(int)ctx.nele_jac,(int)ctx.nele_hess);
	printf("\"status\":%d,\"objective\":",(int)solve_status);
	a4sqp_cutest_json_number((double)obj);
	printf(",");
	printf("\"max_constraint_violation\":");
	a4sqp_cutest_json_number(stats.max_constraint_violation);
	printf(",");
	printf("\"projected_gradient_inf\":");
	a4sqp_cutest_json_number(stats.projected_gradient_inf);
	printf(",");
	printf("\"iterations\":%d,\"qp_solves\":%d,\"qp_failures\":%d,\"line_search_failures\":%d,",
		stats.iterations,
		stats.qp_solves,
		stats.qp_failures,
		stats.line_search_failures
	);
	printf("\"final_step_norm\":");
	a4sqp_cutest_json_number(stats.final_step_norm);
	printf(",\"final_trust_radius\":");
	a4sqp_cutest_json_number(stats.final_trust_radius);
	printf(",\"final_elastic_max\":");
	a4sqp_cutest_json_number(stats.final_elastic_max);
	printf(",\"regularization_size\":");
	a4sqp_cutest_json_number(stats.regularization_size);
	printf(",");
	printf("\"cutest_calls_obj\":%.17g,\"cutest_calls_objgrad\":%.17g,\"cutest_calls_con\":%.17g,\"cutest_calls_congrad\":%.17g,",
		(double)calls[0],
		(double)calls[1],
		(double)calls[4],
		(double)calls[5]
	);
	printf("\"cutest_setup_time\":%.17g,\"cutest_solve_time\":%.17g", (double)cpu[0], (double)cpu[1]);
	printf("}\n");
	FreeA4SqpProblem(problem);
	FREE(x);
	FREE(x_l);
	FREE(x_u);
	FREE(lambda);
	FREE(g_l);
	FREE(g_u);
	FREE(g);
	FREE(equatn);
	FREE(linear);
	FREE(ctx.c_work);
	FREE(ctx.g_work);
	FREE(ctx.hess_work);
	FREE(ctx.hess_x_work);
	FREE(ctx.lambda_work);
	FREE(ctx.hess_row);
	FREE(ctx.hess_col);
	FREE(pname);
	FREE(classification);
	return (int)solve_status;
}

#ifdef __cplusplus
}
#endif
