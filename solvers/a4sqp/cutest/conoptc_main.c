/*
 * CUTEst driver for CONOPT's C API.
 *
 * This file is built by CUTEst's package mechanism, not by the normal ASCEND
 * build. It keeps CUTEst/CONOPT comparison independent from ASCEND's CONOPT
 * slv_system_t adapter.
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
#include "conopt.h"

enum ConoptcRowKind {
	CONOPTC_ROW_EQ = 0,
	CONOPTC_ROW_GE = 1,
	CONOPTC_ROW_LE = 2,
	CONOPTC_ROW_OBJ = 3
};

struct ConoptcRow {
	integer cutest_index;
	int type;
	double rhs;
};

struct ConoptcContext {
	integer n;
	integer m_cutest;
	integer m_conopt;
	integer obj_row;
	integer jac_nnz_cutest;
	integer jac_nnz_conopt;
	integer obj_grad_nnz;
	logical constrained;
	logical noobj;
	struct ConoptcRow *rows;
	rp_ *c_work;
	rp_ *grad_work;
	rp_ *jac_work;
	integer *jac_var;
	integer *jac_fun;
	const rp_ *x_initial;
	const rp_ *x_l;
	const rp_ *x_u;
	rp_ *x_final;
	rp_ *x_margin;
	rp_ *y_margin;
	int modsta;
	int solsta;
	int iterations;
	double objective;
	int readmatrix_calls;
	int fdeval_calls;
	int error_count;
};

static void conoptc_json_string(const char *s){
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

static void conoptc_json_number(double value){
	if(isfinite(value)){
		printf("%.17g",value);
	}else{
		printf("null");
	}
}

static int conoptc_env_int(const char *name, int fallback){
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

static double conoptc_env_double(const char *name, double fallback){
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

static double conoptc_inf_norm(integer n, const rp_ *v){
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

static double conoptc_abs_inf_norm_int_count(int n, const double *v){
	double result = 0.0;
	int i;
	if(v == NULL){
		return NAN;
	}
	for(i = 0; i < n; ++i){
		double a = fabs(v[i]);
		if(a > result){
			result = a;
		}
	}
	return result;
}

static double conoptc_bound_projected_grad_inf(
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

static int conoptc_final_objective_gradient(
	struct ConoptcContext *ctx,
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

static void conoptc_print_x_if_requested(const char *env_name, integer n, const rp_ *x){
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
		conoptc_json_number((double)x[i]);
	}
	printf("]");
}

static int conoptc_is_finite_lower(double value){
	return value > -3.0e9;
}

static int conoptc_is_finite_upper(double value){
	return value < 3.0e9;
}

static integer conoptc_count_rows(integer m, const rp_ *g_l, const rp_ *g_u, const logical *equatn){
	integer count = 0;
	integer i;
	for(i = 0; i < m; ++i){
		if(equatn != NULL && equatn[i]){
			++count;
		}else{
			if(g_l != NULL && conoptc_is_finite_lower((double)g_l[i])){
				++count;
			}
			if(g_u != NULL && conoptc_is_finite_upper((double)g_u[i])){
				++count;
			}
		}
	}
	return count;
}

static void conoptc_build_rows(
	struct ConoptcContext *ctx,
	const rp_ *g_l,
	const rp_ *g_u,
	const logical *equatn
){
	integer i;
	integer row = 0;
	for(i = 0; i < ctx->m_cutest; ++i){
		if(equatn != NULL && equatn[i]){
			ctx->rows[row].cutest_index = i + 1;
			ctx->rows[row].type = CONOPTC_ROW_EQ;
			ctx->rows[row].rhs = g_l != NULL ? (double)g_l[i] : 0.0;
			++row;
		}else{
			if(g_l != NULL && conoptc_is_finite_lower((double)g_l[i])){
				ctx->rows[row].cutest_index = i + 1;
				ctx->rows[row].type = CONOPTC_ROW_GE;
				ctx->rows[row].rhs = (double)g_l[i];
				++row;
			}
			if(g_u != NULL && conoptc_is_finite_upper((double)g_u[i])){
				ctx->rows[row].cutest_index = i + 1;
				ctx->rows[row].type = CONOPTC_ROW_LE;
				ctx->rows[row].rhs = (double)g_u[i];
				++row;
			}
		}
	}
	ctx->obj_row = row;
	ctx->rows[row].cutest_index = -1;
	ctx->rows[row].type = CONOPTC_ROW_OBJ;
	ctx->rows[row].rhs = 0.0;
	ctx->m_conopt = row + 1;
}

static integer conoptc_row_count_for_cutest_constraint(const struct ConoptcContext *ctx, integer cutest_index){
	integer count = 0;
	integer row;
	for(row = 0; row < ctx->obj_row; ++row){
		if(ctx->rows[row].cutest_index == cutest_index){
			++count;
		}
	}
	return count;
}

static int COI_CALLCONV conoptc_readmatrix(
	double lower[],
	double curr[],
	double upper[],
	int vsta[],
	int typex[],
	double rhs[],
	int esta[],
	int colsta[],
	int rowno[],
	double value[],
	int nlflag[],
	int numvar,
	int numcon,
	int numnz,
	void *usrmem
){
	struct ConoptcContext *ctx = (struct ConoptcContext *)usrmem;
	integer row;
	integer col;
	integer k;
	integer count = 0;
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	(void)numcon;
	(void)numnz;
	if(ctx == NULL || numvar != ctx->n){
		return 1;
	}
	ctx->readmatrix_calls++;
	for(col = 0; col < ctx->n; ++col){
		lower[col] = ctx->x_l != NULL ? (double)ctx->x_l[col] : -1e20;
		curr[col] = ctx->x_initial != NULL ? (double)ctx->x_initial[col] : 0.0;
		upper[col] = ctx->x_u != NULL ? (double)ctx->x_u[col] : 1e20;
		vsta[col] = 0;
	}
	for(row = 0; row < ctx->m_conopt; ++row){
		typex[row] = ctx->rows[row].type;
		rhs[row] = ctx->rows[row].rhs;
		esta[row] = 0;
	}
	if(ctx->constrained && ctx->jac_nnz_cutest > 0){
		integer nnz = 0;
		CUTEST_ccfsg(
			&status,
			&ctx->n,
			&ctx->m_cutest,
			ctx->x_initial,
			ctx->c_work,
			&nnz,
			&ctx->jac_nnz_cutest,
			ctx->jac_work,
			ctx->jac_var,
			ctx->jac_fun,
			&grad
		);
		if(status != 0){
			return (int)status;
		}
	}
	if(!ctx->noobj){
		if(ctx->constrained){
			CUTEST_cofg(&status,&ctx->n,ctx->x_initial,&f,ctx->grad_work,&grad);
		}else{
			CUTEST_uofg(&status,&ctx->n,ctx->x_initial,&f,ctx->grad_work,&grad);
		}
		if(status != 0){
			return (int)status;
		}
	}
	for(col = 0; col < ctx->n; ++col){
		colsta[col] = (int)count;
		if(ctx->constrained && ctx->jac_nnz_cutest > 0){
			for(k = 0; k < ctx->jac_nnz_cutest; ++k){
				if(ctx->jac_var[k] == col + 1){
					for(row = 0; row < ctx->obj_row; ++row){
						if(ctx->rows[row].cutest_index == ctx->jac_fun[k]){
							rowno[count] = (int)row;
							value[count] = (double)ctx->jac_work[k];
							nlflag[count] = 1;
							++count;
						}
					}
				}
			}
		}
		if(!ctx->noobj){
			rowno[count] = (int)ctx->obj_row;
			value[count] = (double)ctx->grad_work[col];
			nlflag[count] = 1;
			++count;
		}
	}
	colsta[ctx->n] = (int)count;
	return count == ctx->jac_nnz_conopt ? 0 : 2;
}

static int COI_CALLCONV conoptc_fdeval(
	const double x[],
	double *g,
	double jac[],
	int rowno,
	const int jacnum[],
	int mode,
	int ignerr,
	int *errcnt,
	int numvar,
	int numjac,
	int thread,
	void *usrmem
){
	struct ConoptcContext *ctx = (struct ConoptcContext *)usrmem;
	integer status = 0;
	logical grad = TRUE_;
	rp_ f = 0.0;
	(void)ignerr;
	(void)thread;
	if(ctx == NULL || x == NULL || rowno < 0 || rowno >= ctx->m_conopt || numvar != ctx->n){
		if(errcnt != NULL){
			++(*errcnt);
		}
		return 1;
	}
	ctx->fdeval_calls++;
	if(mode == 1 || mode == 3){
		if(rowno == ctx->obj_row){
			if(ctx->noobj){
				*g = 0.0;
			}else if(ctx->constrained){
				CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->grad_work,&grad);
				*g = (double)f;
			}else{
				CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->grad_work,&grad);
				*g = (double)f;
			}
		}else{
			integer icon = ctx->rows[rowno].cutest_index;
			if(icon < 0){
				status = 1;
			}else{
				CUTEST_ccifg(&status,&ctx->n,&icon,(const rp_ *)x,(rp_ *)g,ctx->grad_work,&grad);
			}
		}
	}
	if(status == 0 && (mode == 2 || mode == 3)){
		if(jac == NULL){
			status = 1;
		}else if(rowno == ctx->obj_row){
			if(ctx->noobj){
				memset(jac,0,(size_t)numjac * sizeof(*jac));
			}else{
				if(mode != 3){
					if(ctx->constrained){
						CUTEST_cofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->grad_work,&grad);
					}else{
						CUTEST_uofg(&status,&ctx->n,(const rp_ *)x,&f,ctx->grad_work,&grad);
					}
				}
				if(status == 0){
					integer i;
					for(i = 0; i < numjac; ++i){
						int col = jacnum != NULL ? jacnum[i] : (int)i;
						jac[i] = (col >= 0 && col < ctx->n) ? (double)ctx->grad_work[col] : 0.0;
					}
				}
			}
		}else{
			integer icon = ctx->rows[rowno].cutest_index;
			if(icon < 0){
				status = 1;
			}else{
				rp_ ci = 0.0;
				CUTEST_ccifg(&status,&ctx->n,&icon,(const rp_ *)x,&ci,ctx->grad_work,&grad);
				if(status == 0){
					integer i;
					for(i = 0; i < numjac; ++i){
						int col = jacnum != NULL ? jacnum[i] : (int)i;
						jac[i] = (col >= 0 && col < ctx->n) ? (double)ctx->grad_work[col] : 0.0;
					}
				}
			}
		}
	}
	if(status != 0){
		ctx->error_count++;
		if(errcnt != NULL){
			++(*errcnt);
		}
	}
	return (int)status;
}

static int COI_CALLCONV conoptc_status(
	int modsta,
	int solsta,
	int iter,
	double objval,
	void *usrmem
){
	struct ConoptcContext *ctx = (struct ConoptcContext *)usrmem;
	if(ctx != NULL){
		ctx->modsta = modsta;
		ctx->solsta = solsta;
		ctx->iterations = iter;
		ctx->objective = objval;
	}
	return 0;
}

static int COI_CALLCONV conoptc_solution(
	const double xval[],
	const double xmar[],
	const int xbas[],
	const int xsta[],
	const double yval[],
	const double ymar[],
	const int ybas[],
	const int ysta[],
	int numvar,
	int numcon,
	void *usrmem
){
	struct ConoptcContext *ctx = (struct ConoptcContext *)usrmem;
	(void)xmar;
	(void)xbas;
	(void)xsta;
	(void)yval;
	(void)ybas;
	(void)ysta;
	if(ctx != NULL && ctx->x_final != NULL && xval != NULL && numvar == ctx->n){
		memcpy(ctx->x_final,xval,(size_t)ctx->n * sizeof(*ctx->x_final));
	}
	if(ctx != NULL && ctx->x_margin != NULL && xmar != NULL && numvar == ctx->n){
		memcpy(ctx->x_margin,xmar,(size_t)ctx->n * sizeof(*ctx->x_margin));
	}
	if(ctx != NULL && ctx->y_margin != NULL && ymar != NULL && numcon == ctx->m_conopt){
		memcpy(ctx->y_margin,ymar,(size_t)ctx->m_conopt * sizeof(*ctx->y_margin));
	}
	return 0;
}

static int COI_CALLCONV conoptc_message(
	int smsg,
	int dmsg,
	int nmsg,
	char *msgv[],
	void *usrmem
){
	int i;
	int print_level = conoptc_env_int("CONOPTC_PRINT_LEVEL",0);
	(void)smsg;
	(void)dmsg;
	(void)usrmem;
	if(print_level <= 0 || msgv == NULL){
		return 0;
	}
	for(i = 0; i < nmsg; ++i){
		if(msgv[i] != NULL){
			fprintf(stderr,"CONOPTC: %s\n",msgv[i]);
		}
	}
	return 0;
}

static int COI_CALLCONV conoptc_errmsg(
	int rowno,
	int colno,
	int posno,
	const char *msg,
	void *usrmem
){
	(void)posno;
	(void)usrmem;
	fprintf(stderr,"CONOPTC error row=%d col=%d: %s\n",rowno,colno,msg != NULL ? msg : "");
	return 0;
}

static int COI_CALLCONV conoptc_option(
	int ncall,
	double *rval,
	int *ival,
	int *lval,
	char *name,
	void *usrmem
){
	struct ConoptcOption {
		const char *name;
		double value;
		int active;
	};
	struct ConoptcOption options[] = {
		{"RTNWMA", conoptc_env_double("CONOPTC_FEAS_TOL",NAN), 0},
		{"RTREDG", conoptc_env_double("CONOPTC_OPT_TOL",NAN), 0},
		{"RTOBJR", conoptc_env_double("CONOPTC_OBJ_TOL",NAN), 0},
	};
	int count = 0;
	int i;
	(void)lval;
	(void)usrmem;
	for(i = 0; i < (int)(sizeof(options) / sizeof(options[0])); ++i){
		options[i].active = isfinite(options[i].value) && options[i].value >= 0.0;
		if(options[i].active){
			++count;
		}
	}
	if(ncall <= 0 || ncall > count || name == NULL || rval == NULL || ival == NULL){
		if(name != NULL){
			name[0] = '\0';
		}
		return 0;
	}
	count = 0;
	for(i = 0; i < (int)(sizeof(options) / sizeof(options[0])); ++i){
		if(options[i].active){
			++count;
			if(count == ncall){
				strcpy(name,options[i].name);
				*rval = options[i].value;
				*ival = 0;
				return 0;
			}
		}
	}
	name[0] = '\0';
	return 0;
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
	logical *equatn = NULL;
	logical *linear = NULL;
	rp_ calls[7] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
	rp_ cpu[4] = {0.0,0.0,0.0,0.0};
	char *pname = NULL;
	char *classification = NULL;
	struct ConoptcContext ctx;
	coiHandle_t conopt = NULL;
	int finite_lower = 0;
	int finite_upper = 0;
	double maxvio = 0.0;
	rp_ final_obj_check = 0.0;
	rp_ *final_grad = NULL;
	double final_objective_gradient_inf = NAN;
	double final_objective_projected_gradient_inf = NAN;
	double projected_gradient_inf = NAN;
	double conopt_x_margin_inf = NAN;
	double conopt_y_margin_inf = NAN;
	int conopt_status = -1;
	int retcode = 0;
	int driver_status = 0;

	memset(&ctx,0,sizeof(ctx));
	ctx.modsta = 0;
	ctx.solsta = 0;
	ctx.iterations = -1;
	FORTRAN_open(&funit,fname,&ierr);
	if(ierr != 0){
		fprintf(stderr,"CONOPTC-CUTEst: failed to open OUTSDIF.d\n");
		return 1;
	}
	CUTEST_cdimen(&status,&funit,&ctx.n,&ctx.m_cutest);
	if(status != 0){
		fprintf(stderr,"CONOPTC-CUTEst: CUTEST_cdimen status=%d\n",(int)status);
		return (int)status;
	}
	ctx.constrained = ctx.m_cutest > 0 ? TRUE_ : FALSE_;
	ctx.noobj = FALSE_;
	MALLOC(classification,FCSTRING_LEN + 1,char);
	MALLOC(pname,FSTRING_LEN + 1,char);
	MALLOC(x,ctx.n,rp_);
	MALLOC(x_l,ctx.n,rp_);
	MALLOC(x_u,ctx.n,rp_);
	MALLOC(ctx.grad_work,ctx.n,rp_);
	if(classification == NULL || pname == NULL || x == NULL || x_l == NULL || x_u == NULL
		|| ctx.grad_work == NULL
	){
		fprintf(stderr,"CONOPTC-CUTEst: allocation failure\n");
		return 2;
	}
	CUTEST_classification(&status,&funit,classification);
	if(status != 0){
		strcpy(classification,"unknown");
	}else{
		classification[FCSTRING_LEN] = '\0';
	}
	if(ctx.constrained){
		MALLOC(lambda,ctx.m_cutest,rp_);
		MALLOC(g_l,ctx.m_cutest,rp_);
		MALLOC(g_u,ctx.m_cutest,rp_);
		MALLOC(g,ctx.m_cutest,rp_);
		MALLOC(ctx.c_work,ctx.m_cutest,rp_);
		MALLOC(equatn,ctx.m_cutest,logical);
		MALLOC(linear,ctx.m_cutest,logical);
		if(lambda == NULL || g_l == NULL || g_u == NULL || g == NULL || ctx.c_work == NULL
			|| equatn == NULL || linear == NULL
		){
			fprintf(stderr,"CONOPTC-CUTEst: constrained allocation failure\n");
			return 2;
		}
		CUTEST_csetup(
			&status,
			&funit,
			&iout,
			&io_buffer,
			&ctx.n,
			&ctx.m_cutest,
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
		ctx.jac_nnz_cutest = nnzj_raw - ctx.n;
		if(ctx.jac_nnz_cutest < 0){
			ctx.jac_nnz_cutest = 0;
		}
		CUTEST_cstats(&status,&nonlin_obj,&nonlin_con,&neq,&nlinear);
		MALLOC(ctx.jac_work,ctx.jac_nnz_cutest > 0 ? ctx.jac_nnz_cutest : 1,rp_);
		MALLOC(ctx.jac_var,ctx.jac_nnz_cutest > 0 ? ctx.jac_nnz_cutest : 1,integer);
		MALLOC(ctx.jac_fun,ctx.jac_nnz_cutest > 0 ? ctx.jac_nnz_cutest : 1,integer);
		if(ctx.jac_work == NULL || ctx.jac_var == NULL || ctx.jac_fun == NULL){
			fprintf(stderr,"CONOPTC-CUTEst: derivative allocation failure\n");
			return 2;
		}
		ctx.m_conopt = conoptc_count_rows(ctx.m_cutest,g_l,g_u,equatn) + 1;
	}else{
		CUTEST_usetup(&status,&funit,&iout,&io_buffer,&ctx.n,x,x_l,x_u);
		ctx.m_conopt = 1;
	}
	if(status != 0){
		fprintf(stderr,"CONOPTC-CUTEst: setup status=%d\n",(int)status);
		return (int)status;
	}
	MALLOC(ctx.rows,ctx.m_conopt,struct ConoptcRow);
	MALLOC(ctx.x_final,ctx.n,rp_);
	MALLOC(ctx.x_margin,ctx.n,rp_);
	MALLOC(ctx.y_margin,ctx.m_conopt,rp_);
	if(ctx.rows == NULL || ctx.x_final == NULL || ctx.x_margin == NULL || ctx.y_margin == NULL){
		fprintf(stderr,"CONOPTC-CUTEst: row allocation failure\n");
		return 2;
	}
	memcpy(ctx.x_final,x,(size_t)ctx.n * sizeof(*ctx.x_final));
	memset(ctx.x_margin,0,(size_t)ctx.n * sizeof(*ctx.x_margin));
	memset(ctx.y_margin,0,(size_t)ctx.m_conopt * sizeof(*ctx.y_margin));
	ctx.x_initial = x;
	ctx.x_l = x_l;
	ctx.x_u = x_u;
	if(ctx.constrained){
		conoptc_build_rows(&ctx,g_l,g_u,equatn);
	}else{
		ctx.obj_row = 0;
		ctx.rows[0].cutest_index = -1;
		ctx.rows[0].type = CONOPTC_ROW_OBJ;
		ctx.rows[0].rhs = 0.0;
	}
	ctx.obj_grad_nnz = ctx.noobj ? 0 : ctx.n;
	ctx.jac_nnz_conopt = ctx.obj_grad_nnz;
	if(ctx.constrained){
		integer k;
		logical grad = TRUE_;
		integer nnz = 0;
		CUTEST_ccfsg(
			&status,
			&ctx.n,
			&ctx.m_cutest,
			(const rp_ *)x,
			ctx.c_work,
			&nnz,
			&ctx.jac_nnz_cutest,
			ctx.jac_work,
			ctx.jac_var,
			ctx.jac_fun,
			&grad
		);
		if(status != 0){
			fprintf(stderr,"CONOPTC-CUTEst: initial Jacobian status=%d\n",(int)status);
			return (int)status;
		}
		for(k = 0; k < ctx.jac_nnz_cutest; ++k){
			ctx.jac_nnz_conopt += conoptc_row_count_for_cutest_constraint(&ctx,ctx.jac_fun[k]);
		}
	}
	CUTEST_pname(&status,&funit,pname);
	if(status != 0){
		strcpy(pname,"unknown");
	}
	pname[FSTRING_LEN] = '\0';
	for(integer i = 0; i < ctx.n; ++i){
		if(conoptc_is_finite_lower((double)x_l[i])){
			++finite_lower;
		}else{
			x_l[i] = -3.1e9;
		}
		if(conoptc_is_finite_upper((double)x_u[i])){
			++finite_upper;
		}else{
			x_u[i] = 3.1e9;
		}
	}
	retcode = COI_Create(&conopt);
	if(retcode != 0 || conopt == NULL){
		fprintf(stderr,"CONOPTC-CUTEst: COI_Create failed retcode=%d\n",retcode);
		return 3;
	}
	COIDEF_NumVar(conopt,(int)ctx.n);
	COIDEF_NumCon(conopt,(int)ctx.m_conopt);
	COIDEF_NumNz(conopt,(int)ctx.jac_nnz_conopt);
	COIDEF_NumNlNz(conopt,(int)ctx.jac_nnz_conopt);
	COIDEF_OptDir(conopt,-1);
	COIDEF_ObjCon(conopt,(int)ctx.obj_row);
	COIDEF_ItLim(conopt,conoptc_env_int("CONOPTC_MAX_ITER",200));
	COIDEF_ErrLim(conopt,conoptc_env_int("CONOPTC_ERROR_LIMIT",200));
	COIDEF_ResLim(conopt,conoptc_env_double("CONOPTC_TIME_LIMIT",1.0e20));
	COIDEF_StdOut(conopt,conoptc_env_int("CONOPTC_PRINT_LEVEL",0) > 0 ? 1 : 0);
	COIDEF_UsrMem(conopt,&ctx);
	COIDEF_ReadMatrix(conopt,conoptc_readmatrix);
	COIDEF_FDEval(conopt,conoptc_fdeval);
	COIDEF_Status(conopt,conoptc_status);
	COIDEF_Solution(conopt,conoptc_solution);
	COIDEF_Message(conopt,conoptc_message);
	COIDEF_ErrMsg(conopt,conoptc_errmsg);
	COIDEF_Option(conopt,conoptc_option);
	conopt_status = COI_Solve(conopt);
	conopt_x_margin_inf = conoptc_abs_inf_norm_int_count((int)ctx.n,(const double *)ctx.x_margin);
	conopt_y_margin_inf = conoptc_abs_inf_norm_int_count((int)ctx.m_conopt,(const double *)ctx.y_margin);
	if(ctx.constrained && g != NULL){
		logical jtrans = FALSE_;
		logical grad = FALSE_;
		integer one = 1;
		rp_ scratch = 0.0;
		CUTEST_ccfg(&status,&ctx.n,&ctx.m_cutest,ctx.x_final,g,&jtrans,&one,&one,&scratch,&grad);
		if(status == 0){
			for(integer i = 0; i < ctx.m_cutest; ++i){
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
	}
	final_grad = (rp_ *)malloc(sizeof(*final_grad) * (size_t)ctx.n);
	if(final_grad != NULL && !conoptc_final_objective_gradient(&ctx,ctx.x_final,&final_obj_check,final_grad)){
		final_objective_gradient_inf = conoptc_inf_norm(ctx.n,final_grad);
		final_objective_projected_gradient_inf =
			conoptc_bound_projected_grad_inf(ctx.n,ctx.x_final,x_l,x_u,final_grad);
		if(!ctx.constrained){
			projected_gradient_inf = final_objective_projected_gradient_inf;
		}
	}
	driver_status = (ctx.solsta == 1 && (ctx.modsta == 1 || ctx.modsta == 2)) ? 0 :
		(ctx.solsta == 2 || ctx.solsta == 3 ? -1 : -3);
	if(ctx.constrained){
		CUTEST_creport(&status,calls,cpu);
		CUTEST_cterminate(&status);
	}else{
		CUTEST_ureport(&status,calls,cpu);
		CUTEST_uterminate(&status);
	}
	printf("{");
	printf("\"solver\":\"CONOPT\",");
	printf("\"problem\":");
	conoptc_json_string(pname);
	printf(",");
	printf("\"classification\":");
	conoptc_json_string(classification);
	printf(",");
	printf("\"n\":%d,\"m\":%d,\"conopt_rows\":%d,\"equalities\":%d,\"linear_constraints\":%d,",
		(int)ctx.n,
		(int)ctx.m_cutest,
		(int)ctx.m_conopt,
		(int)neq,
		(int)nlinear
	);
	printf("\"finite_var_lower\":%d,\"finite_var_upper\":%d,",finite_lower,finite_upper);
	printf("\"jac_nnz\":%d,\"conopt_jac_nnz\":%d,",(int)ctx.jac_nnz_cutest,(int)ctx.jac_nnz_conopt);
	printf("\"status\":%d,\"conopt_retcode\":%d,\"conopt_modsta\":%d,\"conopt_solsta\":%d,",
		driver_status,
		conopt_status,
		ctx.modsta,
		ctx.solsta
	);
	printf("\"objective\":");
	conoptc_json_number(ctx.objective);
	printf(",\"final_objective_check\":");
	conoptc_json_number((double)final_obj_check);
	printf(",\"objective_gradient_inf\":");
	conoptc_json_number(final_objective_gradient_inf);
	printf(",\"objective_projected_gradient_inf\":");
	conoptc_json_number(final_objective_projected_gradient_inf);
	printf(",");
	printf("\"max_constraint_violation\":%.17g,",maxvio);
	printf("\"projected_gradient_inf\":");
	conoptc_json_number(projected_gradient_inf);
	printf(",");
	printf("\"conopt_x_margin_inf\":");
	conoptc_json_number(conopt_x_margin_inf);
	printf(",\"conopt_y_margin_inf\":");
	conoptc_json_number(conopt_y_margin_inf);
	printf(",");
	printf("\"iterations\":%d,\"qp_solves\":null,\"qp_failures\":null,\"line_search_failures\":null,",
		ctx.iterations
	);
	printf("\"readmatrix_calls\":%d,\"fdeval_calls\":%d,\"eval_errors\":%d,",
		ctx.readmatrix_calls,
		ctx.fdeval_calls,
		ctx.error_count
	);
	printf("\"cutest_calls_obj\":%.17g,\"cutest_calls_objgrad\":%.17g,\"cutest_calls_con\":%.17g,\"cutest_calls_congrad\":%.17g,",
		(double)calls[0],
		(double)calls[1],
		(double)calls[4],
		(double)calls[5]
	);
	printf("\"cutest_setup_time\":%.17g,\"cutest_solve_time\":%.17g", (double)cpu[0], (double)cpu[1]);
	conoptc_print_x_if_requested("CONOPTC_DUMP_X",ctx.n,ctx.x_final);
	printf("}\n");
	COI_Free(&conopt);
	COI_Finalize();
	FREE(x);
	FREE(x_l);
	FREE(x_u);
	FREE(lambda);
	FREE(g_l);
	FREE(g_u);
	FREE(g);
	FREE(equatn);
	FREE(linear);
	FREE(final_grad);
	FREE(ctx.c_work);
	FREE(ctx.grad_work);
	FREE(ctx.jac_work);
	FREE(ctx.jac_var);
	FREE(ctx.jac_fun);
	FREE(ctx.rows);
	FREE(ctx.x_final);
	FREE(ctx.x_margin);
	FREE(ctx.y_margin);
	FREE(pname);
	FREE(classification);
	return driver_status;
}

#ifdef __cplusplus
}
#endif
