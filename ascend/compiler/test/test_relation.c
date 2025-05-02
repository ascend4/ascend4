/* Unit tests for relation compilation: simple equality */

#include <stdio.h>
#include <string.h>

#include <ascend/general/env.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/utilities/error.h>

#include <ascend/compiler/ascCompiler.h>
#include <ascend/compiler/module.h>
#include <ascend/compiler/parser.h>
#include <ascend/compiler/symtab.h>
#include <ascend/compiler/simlist.h>
#include <ascend/compiler/instquery.h>
#include <ascend/compiler/parentchild.h>
#include <ascend/compiler/mathinst.h>
#include <ascend/compiler/relation.h>
#include <ascend/compiler/relation_util.h>
#include <ascend/compiler/initialize.h>

#include <test/common.h>
#include <test/assertimpl.h>

static struct Instance *load_model(const char *modname)
{
    char path[256];
    int status;
    struct module_t *m;

    Asc_CompilerInit(1);
    Asc_PutEnv(ASC_ENV_LIBRARY "=models");
    snprintf(path, sizeof(path), "test/compiler/%s.a4c", modname);
    m = Asc_OpenModule(path, &status);
    CU_ASSERT(status == 0);
    CU_ASSERT(m != NULL);

    CU_ASSERT(0 == zz_parse());
    struct Instance *sim = SimsCreateInstance(AddSymbol(modname),
                                              AddSymbol("sim1"),
                                              e_normal,
                                              NULL);
    CU_ASSERT_FATAL(sim != NULL);
    return sim;
}

static void test_simple_eq(void)
{
    struct Instance *sim = load_model("simple_eq");
    struct Instance *root = GetSimulationRoot(sim);
    struct Instance *eq1 = ChildByChar(root, AddSymbol("eq1"));
    CU_ASSERT_FATAL(eq1 != NULL);

    const struct relation *rel = GetInstanceRelationOnly(eq1);
    CU_ASSERT_FATAL(rel != NULL);

    /* operator, variable count */
    CU_ASSERT(RelationRelop(rel) == e_equal);
    CU_ASSERT(NumberVariables(rel) == 2);

    /* variables a, b */
    struct Instance *a_inst = ChildByChar(root, AddSymbol("a"));
    struct Instance *b_inst = ChildByChar(root, AddSymbol("b"));
    CU_ASSERT(RelationVariable(rel, 1) == a_inst);
    CU_ASSERT(RelationVariable(rel, 2) == b_inst);

    /* token lengths: LHS "2 a * 3 b * +" -> 7, RHS "7" -> 1 */
    CU_ASSERT(RelationLength(rel, 1) == 7);
    CU_ASSERT(RelationLength(rel, 0) == 1);

    /* inspect first few tokens on LHS */
    {
        const struct relation_term *t;
        t = RelationTerm(rel, 1, 1); /* 2 */
        CU_ASSERT(RelationTermType(t) == e_int);
        CU_ASSERT(I_TERM(t)->ivalue == 2);

        t = RelationTerm(rel, 2, 1); /* a */
        CU_ASSERT(RelationTermType(t) == e_var);
        CU_ASSERT(V_TERM(t)->varnum == 1);

        t = RelationTerm(rel, 3, 1); /* * */
        CU_ASSERT(RelationTermType(t) == e_times);
    }

    sim_destroy(sim);
    Asc_CompilerDestroy();
}

#define TESTS(T) \
    T(simple_eq)

REGISTER_TESTS_SIMPLE(compiler_relation, TESTS)
