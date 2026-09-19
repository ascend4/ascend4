/* Small runner for the IDA suite, including internal Jacobian comparisons.
   This suite only needs the traditional CUnit API. */
#include <CUnit/Basic.h>
#include <ascend/general/ospath.h>
char ASC_TEST_PATH[PATH_MAX] = ".";
extern CU_ErrorCode test_register_integrator_ida(void);
int main(void){
	unsigned failed;
	if(CU_initialize_registry() != CUE_SUCCESS) return 2;
	if(test_register_integrator_ida() != CUE_SUCCESS) return 2;
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	failed = CU_get_number_of_failures();
	CU_cleanup_registry();
	return failed ? 1 : 0;
}
