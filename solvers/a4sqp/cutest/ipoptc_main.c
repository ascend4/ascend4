/*
 * CUTEst driver for IPOPT's C API.
 *
 * This avoids the legacy CUTEst IPOPT Fortran interface, which is not reliable
 * with the local installed IPOPT build.
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
#include "IpStdCInterface.h"

struct IpoptCutestContext {
	integer n;
	integer m;
	integer nele_jac;
	integer nele_hess;
	logical constrained;
	logical noobj;
	rp_ *c_work;
	rp_ *g_work;
	rp_ *jac_work;
	rp_ *hess_work;
	rp_ *hess_x_work;
	rp_ *lambda_work;
	integer *jac_var;
	integer *jac_fun;
	integer *hess_row;
	integer *hess_col;
};

static void ipoptc_json_string(const char *s){
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

static void ipoptc_json_number(double value){
	if(isfinite(value)){
		printf("%.17g",value);
	}else{
		printf("null");
	}
}

static double ipoptc_inf_norm(integer n, const rp_ *v){
	double result = 0.0;
	integer i;
	if(v == NULL){
		return NAN;
	}
	for(i = 0; i < n; ++i){
		double a = fabs((double)v[i]);
		if(a > result){
			result = a;
		}
	}
	return result;
}

static double ipoptc_bound_projected_grad_inf(
	integer n,
	const rp_ *x,
	const rp_ *x_l,
	const rp_ *x_u,
	const rp_ *grad
){
	double result = 0.0;
	integer i;
	const double active_tol = 1e-8;
	if(x == NULL || grad == NULL){
		return NAN;
	}
	for(i = 0; i < n; ++i){
		double g = (double)grad[i];
		double lower = x_l != NULL ? (double)x_l[i] : -INFINITY;
		double upper = x_u != NULL ? (double)x_u[i] : INFINITY;
		int at_lower = isfinite(lower) && (double)x[i] <= lower + active_tol;
		int at_upper = isfinite(upper) && (double)x[i] >= upper - active_tol;
		if((at_lower && g > 0.0) || (at_upper && g < 0.0)){
			g = 0.0;
		}
		if(fabs(g) > result){
			result = fabs(g);
		}
	}
	return result;
}

static int ipoptc_final_objective_gradient(
	struct IpoptCutestContext *ctx,
	const rp_ *x,
	rp_ *obj,
	rp_ *grad_out
){
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	if(ctx == NULL || x == NULL || obj == NULL || grad_out == NULL){
		return 1;
	}
	if(ctx->noobj){
		memset(grad_out,0,(size_t)ctx->n * sizeof(*grad_out));
		*obj = 0.0;
		return 0;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,x,&f,grad_out,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,x,&f,grad_out,&grad);
	}
	if(status != 0){
		return 1;
	}
	*obj = f;
	return 0;
}

static void ipoptc_print_x_if_requested(const char *env_name, integer n, const rp_ *x){
	integer i;
	const char *dump = getenv(env_name);
	if(dump == NULL || *dump == '\0' || strcmp(dump,"0") == 0){
		return;
	}
	printf(",\"final_x\":[");
	for(i = 0; i < n; ++i){
		if(i > 0){
			printf(",");
		}
		ipoptc_json_number((double)x[i]);
	}
	printf("]");
}

static int ipoptc_env_int(const char *name, int fallback){
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

static double ipoptc_env_double(const char *name, double fallback){
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

static const char *ipoptc_env_string(const char *name, const char *fallback){
	const char *value = getenv(name);
	return (value != NULL && *value != '\0') ? value : fallback;
}

static Bool ipoptc_eval_f(Index n, Number *x, Bool new_x, Number *obj_value, UserDataPtr user_data){
	struct IpoptCutestContext *ctx = (struct IpoptCutestContext *)user_data;
	integer status = 0;
	logical grad = FALSE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || obj_value == NULL){
		return FALSE;
	}
	if(ctx->noobj){
		*obj_value = 0.0;
		return TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->g_work,&grad);
	}
	if(status != 0){
		return FALSE;
	}
	*obj_value = f;
	return TRUE;
}

static Bool ipoptc_eval_grad_f(Index n, Number *x, Bool new_x, Number *grad_f, UserDataPtr user_data){
	struct IpoptCutestContext *ctx = (struct IpoptCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	(void)n;
	(void)new_x;
	if(ctx == NULL || grad_f == NULL){
		return FALSE;
	}
	if(ctx->noobj){
		memset(grad_f,0,(size_t)ctx->n * sizeof(*grad_f));
		return TRUE;
	}
	if(ctx->constrained){
		CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}else{
		CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,(rp_ *)grad_f,&grad);
	}
	return status == 0 ? TRUE : FALSE;
}

static Bool ipoptc_eval_g(Index n, Number *x, Bool new_x, Index m, Number *g, UserDataPtr user_data){
	struct IpoptCutestContext *ctx = (struct IpoptCutestContext *)user_data;
	integer status = 0;
	logical jtrans = FALSE_;
	logical grad = FALSE_;
	integer one = 1;
	rp_ scratch = 0.0;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL){
		return FALSE;
	}
	if(ctx->m <= 0){
		return TRUE;
	}
	if(g == NULL){
		return FALSE;
	}
	CUTEST_ccfg(&status,&ctx->n,&ctx->m,(const rp_ *)x,(rp_ *)g,&jtrans,&one,&one,&scratch,&grad);
	return status == 0 ? TRUE : FALSE;
}

static Bool ipoptc_eval_jac_g(
	Index n,
	Number *x,
	Bool new_x,
	Index m,
	Index nele_jac,
	Index *iRow,
	Index *jCol,
	Number *values,
	UserDataPtr user_data
){
	struct IpoptCutestContext *ctx = (struct IpoptCutestContext *)user_data;
	integer status = 0;
	logical grad = TRUE_;
	integer nnzj = 0;
	rp_ *x_use;
	(void)n;
	(void)new_x;
	(void)m;
	if(ctx == NULL || nele_jac < ctx->nele_jac){
		return FALSE;
	}
	if(ctx->m <= 0 || ctx->nele_jac <= 0){
		return TRUE;
	}
	x_use = x != NULL ? (rp_ *)x : ctx->hess_x_work;
	if(values == NULL){
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x_use,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			ctx->jac_work,
			ctx->jac_var,
			ctx->jac_fun,
			&grad
		);
		if(status != 0){
			return FALSE;
		}
		for(integer k = 0; k < ctx->nele_jac; ++k){
			iRow[k] = (Index)ctx->jac_fun[k];
			jCol[k] = (Index)ctx->jac_var[k];
		}
	}else{
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m,
			(const rp_ *)x_use,
			ctx->c_work,
			&nnzj,
			&ctx->nele_jac,
			(rp_ *)values,
			ctx->jac_var,
			ctx->jac_fun,
			&grad
		);
	}
	return status == 0 ? TRUE : FALSE;
}

static Bool ipoptc_eval_h(
	Index n,
	Number *x,
	Bool new_x,
	Number obj_factor,
	Index m,
	Number *lambda,
	Bool new_lambda,
	Index nele_hess,
	Index *iRow,
	Index *jCol,
	Number *values,
	UserDataPtr user_data
){
	struct IpoptCutestContext *ctx = (struct IpoptCutestContext *)user_data;
	integer status = 0;
	integer nnzh = 0;
	rp_ *x_use;
	(void)n;
	(void)new_x;
	(void)m;
	(void)new_lambda;
	if(ctx == NULL || nele_hess < ctx->nele_hess){
		return FALSE;
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
			return FALSE;
		}
		if(values == NULL){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				iRow[k] = (Index)ctx->hess_row[k];
				jCol[k] = (Index)ctx->hess_col[k];
			}
		}else if(obj_factor != 1.0){
			for(integer k = 0; k < ctx->nele_hess; ++k){
				values[k] *= obj_factor;
			}
		}
		return TRUE;
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
			return FALSE;
		}
		for(integer k = 0; k < ctx->nele_hess; ++k){
			iRow[k] = (Index)ctx->hess_row[k];
			jCol[k] = (Index)ctx->hess_col[k];
		}
		return TRUE;
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
	return status == 0 ? TRUE : FALSE;
}

static void ipoptc_apply_options(IpoptProblem problem){
	double tol = ipoptc_env_double("IPOPTC_TOL",1e-7);
	int max_iter = ipoptc_env_int("IPOPTC_MAX_ITER",300);
	int print_level = ipoptc_env_int("IPOPTC_PRINT_LEVEL",0);
	const char *hess = ipoptc_env_string("IPOPTC_HESSIAN","limited-memory");
	AddIpoptNumOption(problem,"tol",tol);
	AddIpoptNumOption(problem,"constr_viol_tol",tol);
	AddIpoptNumOption(problem,"dual_inf_tol",tol);
	AddIpoptNumOption(problem,"compl_inf_tol",tol);
	AddIpoptIntOption(problem,"max_iter",max_iter);
	AddIpoptIntOption(problem,"print_level",print_level);
	AddIpoptStrOption(problem,"hessian_approximation",(char *)hess);
	AddIpoptStrOption(problem,"mu_strategy",(char *)"adaptive");
	AddIpoptNumOption(problem,"bound_relax_factor",0.0);
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
	rp_ *g = NULL;
	rp_ *mult_x_l = NULL;
	rp_ *mult_x_u = NULL;
	logical *equatn = NULL;
	logical *linear = NULL;
	rp_ obj = 0.0;
	rp_ calls[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	rp_ cpu[4] = {0.0,0.0,0.0,0.0};
	char *pname = NULL;
	char *classification = NULL;
	struct IpoptCutestContext ctx;
	IpoptProblem problem = NULL;
	enum ApplicationReturnStatus solve_status;
	int finite_lower = 0;
	int finite_upper = 0;
	double maxvio = 0.0;
	rp_ final_obj_check = 0.0;
	rp_ *final_grad = NULL;
	double final_objective_gradient_inf = NAN;
	double final_objective_projected_gradient_inf = NAN;

	memset(&ctx,0,sizeof(ctx));
	FORTRAN_open(&funit,fname,&ierr);
	if(ierr != 0){
		fprintf(stderr,"IPOPTC-CUTEst: failed to open OUTSDIF.d\n");
		return 1;
	}
	CUTEST_cdimen(&status,&funit,&ctx.n,&ctx.m);
	if(status != 0){
		fprintf(stderr,"IPOPTC-CUTEst: CUTEST_cdimen status=%d\n",(int)status);
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
	MALLOC(mult_x_l,ctx.n,rp_);
	MALLOC(mult_x_u,ctx.n,rp_);
	if(classification == NULL || pname == NULL || x == NULL || x_l == NULL || x_u == NULL
		|| ctx.g_work == NULL || mult_x_l == NULL || mult_x_u == NULL
	){
		fprintf(stderr,"IPOPTC-CUTEst: allocation failure\n");
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
		MALLOC(equatn,ctx.m,logical);
		MALLOC(linear,ctx.m,logical);
		if(lambda == NULL || g_l == NULL || g_u == NULL || g == NULL || ctx.c_work == NULL
			|| equatn == NULL || linear == NULL
		){
			fprintf(stderr,"IPOPTC-CUTEst: constrained allocation failure\n");
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
		MALLOC(ctx.jac_work,ctx.nele_jac > 0 ? ctx.nele_jac : 1,rp_);
		MALLOC(ctx.jac_var,ctx.nele_jac > 0 ? ctx.nele_jac : 1,integer);
		MALLOC(ctx.jac_fun,ctx.nele_jac > 0 ? ctx.nele_jac : 1,integer);
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		MALLOC(ctx.lambda_work,ctx.m > 0 ? ctx.m : 1,rp_);
		if(ctx.jac_work == NULL || ctx.jac_var == NULL || ctx.jac_fun == NULL
			|| ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL
			|| ctx.hess_x_work == NULL || ctx.lambda_work == NULL
		){
			fprintf(stderr,"IPOPTC-CUTEst: derivative allocation failure\n");
			return 2;
		}
		for(integer i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}else{
		CUTEST_usetup(&status,&funit,&iout,&io_buffer,&ctx.n,x,x_l,x_u);
		CUTEST_udimsh(&status,&ctx.nele_hess);
		ctx.nele_jac = 0;
		MALLOC(ctx.hess_work,ctx.nele_hess > 0 ? ctx.nele_hess : 1,rp_);
		MALLOC(ctx.hess_row,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_col,ctx.nele_hess > 0 ? ctx.nele_hess : 1,integer);
		MALLOC(ctx.hess_x_work,ctx.n > 0 ? ctx.n : 1,rp_);
		if(ctx.hess_work == NULL || ctx.hess_row == NULL || ctx.hess_col == NULL
			|| ctx.hess_x_work == NULL
		){
			fprintf(stderr,"IPOPTC-CUTEst: unconstrained Hessian allocation failure\n");
			return 2;
		}
		for(integer i = 0; i < ctx.n; ++i){
			ctx.hess_x_work[i] = 0.0;
		}
	}
	if(status != 0){
		fprintf(stderr,"IPOPTC-CUTEst: setup status=%d\n",(int)status);
		return (int)status;
	}
	CUTEST_pname(&status,&funit,pname);
	if(status != 0){
		strcpy(pname,"unknown");
	}
	pname[FSTRING_LEN] = '\0';
	for(integer i = 0; i < ctx.n; ++i){
		if(x_l[i] > -1e19){
			++finite_lower;
		}
		if(x_u[i] < 1e19){
			++finite_upper;
		}
	}
	problem = CreateIpoptProblem(
		(Index)ctx.n,
		(Number *)x_l,
		(Number *)x_u,
		(Index)ctx.m,
		(Number *)g_l,
		(Number *)g_u,
		(Index)ctx.nele_jac,
		(Index)ctx.nele_hess,
		1,
		ctx.noobj ? NULL : ipoptc_eval_f,
		ipoptc_eval_g,
		ctx.noobj ? NULL : ipoptc_eval_grad_f,
		ipoptc_eval_jac_g,
		ctx.noobj ? NULL : ipoptc_eval_h
	);
	if(problem == NULL){
		fprintf(stderr,"IPOPTC-CUTEst: CreateIpoptProblem failed\n");
		return 3;
	}
	ipoptc_apply_options(problem);
	solve_status = IpoptSolve(
		problem,
		(Number *)x,
		(Number *)g,
		(Number *)&obj,
		(Number *)lambda,
		(Number *)mult_x_l,
		(Number *)mult_x_u,
		&ctx
	);
	if(ctx.constrained && g != NULL){
		for(integer i = 0; i < ctx.m; ++i){
			double vio = 0.0;
			if(g[i] < g_l[i]){
				vio = (double)(g_l[i] - g[i]);
			}
			if(g[i] > g_u[i] && (double)(g[i] - g_u[i]) > vio){
				vio = (double)(g[i] - g_u[i]);
			}
			if(vio > maxvio){
				maxvio = vio;
			}
		}
	}
	final_grad = (rp_ *)malloc(sizeof(*final_grad) * (size_t)ctx.n);
	if(final_grad != NULL && !ipoptc_final_objective_gradient(&ctx,x,&final_obj_check,final_grad)){
		final_objective_gradient_inf = ipoptc_inf_norm(ctx.n,final_grad);
		final_objective_projected_gradient_inf =
			ipoptc_bound_projected_grad_inf(ctx.n,x,x_l,x_u,final_grad);
	}
	if(ctx.constrained){
		CUTEST_creport(&status,calls,cpu);
		CUTEST_cterminate(&status);
	}else{
		CUTEST_ureport(&status,calls,cpu);
		CUTEST_uterminate(&status);
	}
	printf("{");
	printf("\"solver\":\"IPOPT\",");
	printf("\"problem\":");
	ipoptc_json_string(pname);
	printf(",");
	printf("\"classification\":");
	ipoptc_json_string(classification);
	printf(",");
	printf("\"n\":%d,\"m\":%d,\"equalities\":%d,\"linear_constraints\":%d,",
		(int)ctx.n,
		(int)ctx.m,
		(int)neq,
		(int)nlinear
	);
	printf("\"finite_var_lower\":%d,\"finite_var_upper\":%d,",finite_lower,finite_upper);
	printf("\"jac_nnz\":%d,\"hess_nnz\":%d,",(int)ctx.nele_jac,(int)ctx.nele_hess);
	printf("\"status\":%d,\"objective\":%.17g,",(int)solve_status,(double)obj);
	printf("\"final_objective_check\":");
	ipoptc_json_number((double)final_obj_check);
	printf(",\"objective_gradient_inf\":");
	ipoptc_json_number(final_objective_gradient_inf);
	printf(",\"objective_projected_gradient_inf\":");
	ipoptc_json_number(final_objective_projected_gradient_inf);
	printf(",");
	printf("\"max_constraint_violation\":%.17g,",maxvio);
	printf("\"projected_gradient_inf\":null,");
	printf("\"iterations\":null,\"qp_solves\":null,\"qp_failures\":null,\"line_search_failures\":null,");
	printf("\"final_step_norm\":null,\"final_trust_radius\":null,\"final_elastic_max\":null,");
	printf("\"cutest_calls_obj\":%.17g,\"cutest_calls_objgrad\":%.17g,\"cutest_calls_con\":%.17g,\"cutest_calls_congrad\":%.17g,",
		(double)calls[0],
		(double)calls[1],
		(double)calls[4],
		(double)calls[5]
	);
	printf("\"cutest_setup_time\":%.17g,\"cutest_solve_time\":%.17g", (double)cpu[0], (double)cpu[1]);
	ipoptc_print_x_if_requested("IPOPTC_DUMP_X",ctx.n,x);
	printf("}\n");
	FreeIpoptProblem(problem);
	FREE(x);
	FREE(x_l);
	FREE(x_u);
	FREE(lambda);
	FREE(g_l);
	FREE(g_u);
	FREE(g);
	FREE(mult_x_l);
	FREE(mult_x_u);
	FREE(equatn);
	FREE(linear);
	FREE(final_grad);
	FREE(ctx.c_work);
	FREE(ctx.g_work);
	FREE(ctx.jac_work);
	FREE(ctx.jac_var);
	FREE(ctx.jac_fun);
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
