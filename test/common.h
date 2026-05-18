#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include "CUnit/CUnit.h"
#include "test_globals.h"

#define ASC_TESTING

/*
 * Record that a feature-specific portion of a test was skipped while allowing
 * the rest of the test body to keep running. Use this for secondary checks,
 * not for prerequisites that make the whole test invalid.
 */
#define ASC_TEST_PARTIAL_SKIP(REASON) \
	CU_skipImplementation(__LINE__, __FILE__, (REASON))

#define DEFINE_SUITE_SIMPLE(NAME) \
	int res = 0; \
	CU_pSuite s = CU_add_suite(#NAME,NULL,NULL); \
	if(NULL == s){ \
		return CUE_NOSUITE; \
	} \

#define ADD_TEST_SIMPLE(NAME) \
	res = res || (NULL == CU_add_test(s, #NAME, test_##NAME));

#define SUITE_DONE \
	if(res){ \
		return CUE_NOTEST; \
	} \
	return CUE_SUCCESS;

/* register tests that don't need Init/Clean/SetUp/TearDown routines */
#define REGISTER_TESTS_SIMPLE(SUITE,TESTS) \
	CU_ErrorCode test_register_##SUITE(void){ \
		DEFINE_SUITE_SIMPLE(SUITE); \
		TESTS(ADD_TEST_SIMPLE) \
		SUITE_DONE; \
	}

#define PROTO(SUITE,NAME) CU_ErrorCode test_register_##SUITE##_##NAME(void);

#define TESTREGISTER(SUITE,NAME) test_register_##SUITE##_##NAME()

#define PROTO_SUITE(SUITE) CU_ErrorCode test_register_##SUITE(void);

#define REGISTER_SUITE(SUITENAME,TESTS) \
	CU_ErrorCode test_register_##SUITENAME(void){ \
		CU_ErrorCode result = CUE_SUCCESS; \
		TESTS(REGISTER_TEST) \
		return result; \
	}	

#endif

