#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ascend/general/env.h>
#include <ascend/general/ospath.h>
#include <ascend/general/list.h>
#include <ascend/general/ltmatrix.h>

#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/library.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/atomvalue.h>
#include <ascend/compiler/relation_io.h>
#include <ascend/compiler/reverse_ad.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/watchpt.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/name.h>
#include <ascend/compiler/visitinst.h>
#include <ascend/compiler/functype.h>
#include <ascend/compiler/safe.h>
#include <ascend/compiler/qlfdid.h>
#include <ascend/compiler/instance_io.h>

#include <ascend/compiler/slvreq.h>

#include <ascend/system/system.h>
#include <ascend/system/slv_client.h>
#include <ascend/solver/solver.h>
#include <ascend/solver/slvDOF.h>
#include <ascend/system/slv_server.h>
#include <ascend/system/var.h>

#include <test/common.h>

static void test_dof(const char *fname,int xstatus, int xdof){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	Asc_PutEnv(ASC_ENV_SOLVERS "=solvers/qrslv");

	/* load the file */
	char modelpath[PATH_MAX];
	strcpy((char *)modelpath,"test/slvdof/");
	strncat(modelpath, fname, PATH_MAX - strlen(modelpath));
	strncat(modelpath, ".a4c", PATH_MAX - strlen(modelpath));
	m = Asc_OpenModule(modelpath,&status);
	CU_ASSERT(status == 0);
	CU_TEST(NULL != m);

	/* parse it */
	CU_ASSERT(0 == zz_parse());

	/* find the model */
	CU_ASSERT(FindType(AddSymbol(fname))!=NULL);

	/* instantiate it */
	struct Instance *siminst = SimsCreateInstance(AddSymbol(fname), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(siminst!=NULL);

	/** initialise */
    //CONSOLE_DEBUG("RUNNING ON_LOAD");
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(siminst),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	/* 'build' the 'system' -- the flattened system of equations */
	slv_system_t sys = system_build(GetSimulationRoot(siminst));
	CU_ASSERT_FATAL(sys != NULL);

	/* assign the solver to the system */
	int dof;
	CU_TEST(1 == slvDOF_status(sys,&status,&dof));
    //CONSOLE_DEBUG("status = %d, dof = %d",status,dof);
	CU_TEST(status == xstatus); /* underspecified */
	CU_TEST(dof == xdof); /* 2 equations, 3 unknowns -> 1 dof */

	struct var_variable **vl = slv_get_solvers_var_list(sys);
	struct rel_relation **rl = slv_get_solvers_rel_list(sys);
#if 1
	int32 *vil, *ril, *fil; // for slvdof_structsing
	char *s;
	int32 *i;
	if(xstatus == 3 && status == 3){
		CU_TEST(NULL != sys);
		CU_TEST(0 == slvDOF_structsing(sys,mtx_FIRST,&vil,&ril,&fil));
		if(vil){
			for(i=vil; *i!=-1; ++i){
				s = var_make_name(sys,vl[*i]);
				CONSOLE_DEBUG("variable involved: %s",s);
				ASC_FREE(s);
			}
			ASC_FREE(vil);
			for(i=ril; *i!=-1; ++i){
				s = rel_make_name(sys,rl[*i]);
				CONSOLE_DEBUG("rel involved: %s",s);
				ASC_FREE(s);
			}
			ASC_FREE(ril);
			for(i=fil; *i!=-1; ++i){
				s = var_make_name(sys,vl[*i]);
				CONSOLE_DEBUG("should free var: %s",s);
				ASC_FREE(s);
			}
			ASC_FREE(fil);
		}

	}else if (xstatus == 1 && status == 1){
		CU_TEST(1 == slvDOF_eligible(sys, &vil));
		for(i=vil; *i!=-1; ++i){
			s = var_make_name(sys,vl[*i]);
			CONSOLE_DEBUG("try fixing var: %s",s);
			ASC_FREE(s);
		}
		ASC_FREE(vil);
	}	
#endif

	/* all sorts of destruction */
	//CONSOLE_DEBUG("DESTROYING NOW...");
	CU_ASSERT(NULL != siminst)
	if(sys)system_destroy(sys);

	system_free_reused_mem();
	sim_destroy(siminst);
	solver_destroy_engines();
	Asc_CompilerDestroy();
}

static void test_dof1(void){
	test_dof("dof1",1,1);
}

static void test_dof2(void){
	test_dof("dof2",2,0);
}

static void test_dof3(void){
	test_dof("dof3",3,0);
}

static void test_dof4(void){
	test_dof("dof4",4,0);
}

static void test_dof5(void){
	/* test a few NULL-checking cases */
	CU_TEST(1 == slvDOF_structsing(NULL,0,NULL,NULL,NULL));
	int x, y;
	CU_TEST(0 == slvDOF_status(NULL,&x,&y));
	int *z;
	CU_TEST(0 == slvDOF_eligible(NULL,&z));
}

/*===========================================================================*/
/* Registration information */

#define TESTS(T) \
	T(dof1) \
	T(dof2) \
	T(dof3) \
	T(dof4) \
	T(dof5)

REGISTER_TESTS_SIMPLE(solver_slvdof, TESTS)

