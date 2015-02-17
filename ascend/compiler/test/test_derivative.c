#include <string.h>

#include <ascend/general/env.h>
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
#include <ascend/compiler/deriv.h>
#include <ascend/compiler/instance_io.h>
#include <ascend/compiler/value_type.h>
#include <ascend/compiler/initialize.h>
#include <ascend/compiler/cmpfunc.h>

#include <test/common.h>
#include <test/assertimpl.h>

static void test_test1(void){

	struct module_t *m;
	int status;
        struct Instance *sim, *deriv, *state, *inst, *root;
	struct Name *name;
	enum Proc_enum pe;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/derivative1.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);

        CU_ASSERT_FATAL(FindType(AddSymbol("test1"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test2"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test3"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test4"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test5"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test6"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test7"))!=NULL);
        CU_ASSERT_FATAL(FindType(AddSymbol("test8"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("der(distance,time)"))!=NULL);
        CU_ASSERT(FindType(AddSymbol("der(der(distance,time),time)"))!=NULL);

	struct gl_list_t *l = Asc_TypeByModule(m);
#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("%lu library entries loaded from %s",gl_length(l),Asc_ModuleName(m));
#endif
	gl_destroy(l);

	CONSOLE_DEBUG("Instantiating test1");

        sim = SimsCreateInstance(AddSymbol("test1"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars and rels */
	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("y"))) && InstanceKind(inst)==REAL_ATOM_INST); 
        WriteDerInfo(ASCERR,inst);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(x,t1)"))) && InstanceKind(inst)==REAL_ATOM_INST); 
        WriteDerInfo(ASCERR,inst);
	CU_ASSERT(RealAtomDims(inst)==RealAtomDims(ChildByChar(inst,AddSymbol("upper_bound"))));
        CU_ASSERT(inst == FindDerByArgs(ChildByChar(root,AddSymbol("x")),ChildByChar(root,AddSymbol("t1"))));
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(y,t1)"))) && InstanceKind(inst)==REAL_ATOM_INST); 
        CU_ASSERT(inst == FindDerByArgs(ChildByChar(root,AddSymbol("y")),ChildByChar(root,AddSymbol("t1"))));
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(der(x,t1),t1)"))) && InstanceKind(inst)==REAL_ATOM_INST);
        CU_ASSERT(inst == FindDerByArgs(ChildByChar(root,AddSymbol("der(x,t1)")),ChildByChar(root,AddSymbol("t1"))));

	/** Call on_load */
	name = CreateIdName(AddSymbol("on_load"));
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test2");

        sim = SimsCreateInstance(AddSymbol("test2"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars and rels */
	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(a.x,t1)"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	deriv = InstanceChild(InstanceChild(inst,1),1);
        inst = ChildByChar(root,AddSymbol("a"));
        inst = ChildByChar(inst,AddSymbol("x"));
	state = InstanceChild(InstanceChild(inst,1),1);
        CU_ASSERT(deriv == FindDerByArgs(state,ChildByChar(root,AddSymbol("t1"))));
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(y1,t1)"))) && InstanceKind(inst)==REAL_ATOM_INST); 

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test3");

        sim = SimsCreateInstance(AddSymbol("test3"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars and rels */
	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(b.a.x,t)"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	deriv = InstanceChild(InstanceChild(InstanceChild(inst,1),1),1);
        inst = ChildByChar(root,AddSymbol("b"));
        inst = InstanceChild(inst,1);
        inst = ChildByChar(inst,AddSymbol("a"));
        inst = ChildByChar(inst,AddSymbol("x"));
	state = InstanceChild(InstanceChild(inst,1),1);
	inst = ChildByChar(root,AddSymbol("b"));
        inst = InstanceChild(inst,1);
	inst = InstanceChild(InstanceChild(ChildByChar(inst,AddSymbol("der(a.x,t1)")),1),1);
        CU_ASSERT(deriv == FindDerByArgs(state,ChildByChar(root,AddSymbol("t"))) && deriv == inst);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(y,t)"))) && InstanceKind(inst)==REAL_ATOM_INST); 
        WriteDerInfo(ASCERR,inst);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(der(b.a.x,t),t)"))) != NULL && InstanceKind(inst)==ARRAY_INT_INST); 
        CU_ASSERT((inst = ChildByChar(root,AddSymbol("t"))) != NULL);
        WriteDerInfo(ASCERR,inst); 

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test4");

        sim = SimsCreateInstance(AddSymbol("test4"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);

	/* check for vars */
	root = GetSimulationRoot(sim);

	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(x,t)"))) && InstanceKind(inst)==ARRAY_INT_INST); 
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("x"))) && InstanceKind(inst)==ARRAY_INT_INST); 

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test5");

        sim = SimsCreateInstance(AddSymbol("test5"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test6");

        sim = SimsCreateInstance(AddSymbol("test6"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	root = GetSimulationRoot(sim);
	CU_ASSERT((inst = ChildByChar(root,AddSymbol("der(x,t)"))) && InstanceKind(inst)==REAL_ATOM_INST); 
	CU_ASSERT((inst = ChildByChar(inst,AddSymbol("lower_bound"))) != NULL); 
	pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);
	CU_ASSERT(RealAtomValue(inst)==0.0);

	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test7");

        sim = SimsCreateInstance(AddSymbol("test7"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	sim_destroy(sim);

	CONSOLE_DEBUG("Instantiating test8");

        sim = SimsCreateInstance(AddSymbol("test8"), AddSymbol("sim1"), e_normal, NULL);
	CU_ASSERT_FATAL(sim!=NULL);
	root = GetSimulationRoot(sim);
	inst = ChildByChar(root,AddSymbol("der(x1,t1)"));
	CU_ASSERT(CmpSymchar(GetName(InstanceTypeDesc(inst)),AddSymbol("der(distance,time)")) == 0);
	inst = ChildByChar(root,AddSymbol("der(y1,t1)"));
	CU_ASSERT(CmpSymchar(GetName(InstanceTypeDesc(inst)),AddSymbol("der(distance,time)")) == 0);
	inst = ChildByChar(root,AddSymbol("der(z1,t1)"));
	CU_ASSERT(CmpSymchar(GetName(InstanceTypeDesc(inst)),AddSymbol("der(distance,time)")) == 0);
	sim_destroy(sim);

	Asc_CompilerDestroy();
}

static void test_test2(void){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/noindep.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);
        CU_ASSERT(FindType(AddSymbol("noindep"))==NULL);

	Asc_CompilerDestroy();
}

static void test_test3(void){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/badderiv.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);
        CU_ASSERT(FindType(AddSymbol("badderiv"))==NULL);

	Asc_CompilerDestroy();
}

static void test_test4(void){
	struct module_t *m;
	int status;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");
	
	m = Asc_OpenModule("ksenija/badtype.a4c",&status);
	CU_ASSERT(status==0);

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("Beginning parse of %s",Asc_ModuleName(m));
#endif
	status = zz_parse();

#ifdef BASICS_DEBUG
	CONSOLE_DEBUG("zz_parse returns status=%d",status);
#endif
	CU_ASSERT(status==0);
        CU_ASSERT(FindType(AddSymbol("badtype"))==NULL);

	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2) \
	T(test3) \
	T(test4)

REGISTER_TESTS_SIMPLE(compiler_derivative, TESTS)

