#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include "CUnit/CUnit.h"

#define PROTO(SUITE,NAME) CU_ErrorCode test_register_##SUITE##_##NAME(void);

#define PROTO_SUITE(SUITE) CU_ErrorCode test_register_##SUITE(void);

#define REGISTER_SUITE(SUITENAME,TESTS) \
	CU_ErrorCode test_register_##SUITENAME(void){ \
		CU_ErrorCode result = CUE_SUCCESS; \
		TESTS(REGISTER_TEST) \
		return result; \
	}	

#endif

