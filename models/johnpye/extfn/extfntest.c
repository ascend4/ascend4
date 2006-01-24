#include <stdio.h>

#include <utilities/ascConfig.h>
#include <compiler/fractions.h>
#include <compiler/compiler.h>
#include <compiler/dimen.h>
#include <compiler/child.h>
#include <general/list.h>
#include <compiler/module.h>
#include <compiler/childinfo.h>
#include <compiler/slist.h>
#include <compiler/type_desc.h>
#include <compiler/packages.h>

int addone_prepare(struct Slv_Interp *slv_interp, struct Instance *data, struct gl_list_t *arglist);
int addone_calc(struct Slv_Interp *slv_interp, int ninputs, int noutputs, double *inputs, double *outputs, double *jacobian);

/**
	This is the function called from "IMPORT extfntest_register FROM extfntest"

	It sets up the functions in this external function library
*/
extern int
DLEXPORT extfntest_register(struct Slv_Interp *dummy1,
                      struct Instance *root,
                      struct gl_list_t *arglist,
                      unsigned long dummy4
){
	const char *addone_help = "This is a test of the dynamic user packages functionality";
	int result = 0;

	(void)dummy1;(void)root;(void)arglist;(void)dummy4;

	error_reporter(ASC_PROG_NOTE,__FILE__,__LINE__,"Initialising EXTFNTEST...\n");

	result += CreateUserFunction("add_one",
                  (ExtEvalFunc *)addone_prepare,
			      (ExtEvalFunc **)addone_calc,
			      (ExtEvalFunc **)NULL,
			      (ExtEvalFunc **)NULL,
			      1,1,addone_help);

	FPRINTF(ASCERR,"CreateUserFunction result = %d\n",result);
	return result;
}

int addone_prepare(struct Slv_Interp *slv_interp,
	   struct Instance *data,
	   struct gl_list_t *arglist
){
	FPRINTF(ASCERR,"PREPARING PKG EXTFNTEST...\n");
	const char *mystring = "MY STRING IS HERE";
	slv_interp->user_data = (void *)mystring;
}

int addone_calc(struct Slv_Interp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian
){
	char *mystring = (char *)slv_interp->user_data;

	FPRINTF(ASCERR,"ADDONE_CALC: mystring = %s\n",mystring);

	FPRINTF(ASCERR,"NINPUTS = %d, NOUTPUTS = %d\n",ninputs, noutputs);

	double *x = &(inputs[0]);
	
	double *y = &(outputs[0]);

	y = x + 1;

	return 1; /* success */
}
