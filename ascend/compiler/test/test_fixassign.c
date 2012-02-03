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

#include <ascend/compiler/initialize.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *load_model(const char *filename, const char *name, int assert_parse_ok, int *parsestatus){
	struct module_t *m;

	Asc_CompilerInit(1);
	Asc_PutEnv(ASC_ENV_LIBRARY "=models");

	/* load the file */
	char path[PATH_MAX];
	strcpy((char *)path,"test/compiler/");
	strcat((char *)path,filename);
	int openmodulestatus;
	m = Asc_OpenModule(path,&openmodulestatus);
	CU_ASSERT(openmodulestatus == 0);
	CONSOLE_DEBUG("Parsing...");
	if(assert_parse_ok){
		CU_ASSERT((*parsestatus = zz_parse()) == 0);
	}else{
		*parsestatus = zz_parse();
		if(*parsestatus)return NULL;
	}
	CONSOLE_DEBUG("Instantiating...");
	struct Instance *sim = SimsCreateInstance(AddSymbol(name), AddSymbol("sim1"), e_normal, NULL);
	if(assert_parse_ok) {
		CU_ASSERT_FATAL(sim!=NULL);
	}else{
		CU_ASSERT_FATAL(sim==NULL);
	}
	return sim;
}

#define GET_FIXED(VAR) \
	inst = ChildByChar(root,AddSymbol(VAR)); \
	CU_ASSERT_FATAL(inst!=NULL); \
	CU_ASSERT((InstanceKind(inst)==REAL_ATOM_INST)); \
	inst = ChildByChar(inst,AddSymbol("fixed")); \
	CU_ASSERT_FATAL(inst!=NULL);

#define CHECK_FIXED(VAR) \
	GET_FIXED(VAR);\
	CU_ASSERT(GetBooleanAtomValue(inst));
#define CHECK_FREE(VAR) \
	GET_FIXED(VAR);\
	CU_ASSERT(!GetBooleanAtomValue(inst));

static void test_test1(void){
	int parsestatus;
	struct Instance *sim = load_model("fix_and_assign1.a4c", "fix_and_assign1", TRUE, &parsestatus);
	CU_ASSERT(parsestatus == 0);
	CU_ASSERT(sim != 0);
	CU_ASSERT(FindType(AddSymbol("fix_and_assign1"))!=NULL);

	struct Instance *root = GetSimulationRoot(sim);
	struct Instance *inst;
	CHECK_FREE("x");
	CHECK_FREE("y");
	/** Call on_load */
	struct Name *name = CreateIdName(AddSymbol("on_load"));
	enum Proc_enum pe = Initialize(GetSimulationRoot(sim),name,"sim1", ASCERR, WP_STOPONERR, NULL, NULL);
	CU_ASSERT(pe==Proc_all_ok);
	CHECK_FIXED("x");
	CHECK_FIXED("y");
	/* Check the values */
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("x"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT(RealAtomValue(inst)==2.0);
	CU_ASSERT((inst = ChildByChar(GetSimulationRoot(sim),AddSymbol("y"))) && InstanceKind(inst)==REAL_ATOM_INST);
	CU_ASSERT(RealAtomValue(inst)==1.0);
	Asc_CompilerDestroy();
}

static void test_test2(void){
	int parsestatus;
	struct Instance *sim = load_model("fix_and_assign2.a4c", "fix_and_assign2", FALSE, &parsestatus);
	/* fix-and-assign in the main model section should not trigger parse error */
	CU_ASSERT(parsestatus == 0);
	/* type for that model should have been ignored */
	CU_ASSERT(FindType(AddSymbol("fix_and_assign2"))==NULL);
	CU_ASSERT(sim==NULL);
	Asc_CompilerDestroy();
}

/*===========================================================================*/
/* Registration information */

/* the list of tests */

#define TESTS(T) \
	T(test1) \
	T(test2)

REGISTER_TESTS_SIMPLE(compiler_fixassign, TESTS)

