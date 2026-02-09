/*
 * ASCEND glob matching tests.
 */
#include <test/common.h>
#include <test/assertimpl.h>
#include <ascend/general/glob.h>

static void test_glob_star_basic(void){
	int err = 0;
	CU_TEST(asc_glob_match("abc", "abc", &err) == 1);
	CU_TEST(err == 0);
	CU_TEST(asc_glob_match("abc", "ab", &err) == 0);
	CU_TEST(asc_glob_match("*", "anything", &err) == 1);
	CU_TEST(asc_glob_match("a*", "abc", &err) == 1);
	CU_TEST(asc_glob_match("*c", "abc", &err) == 1);
	CU_TEST(asc_glob_match("a*c", "abc", &err) == 1);
	CU_TEST(asc_glob_match("a**c", "abc", &err) == 1);
	CU_TEST(asc_glob_match("a*c", "ab", &err) == 0);
}

static void test_glob_unsupported_tokens(void){
	int err = 0;
	CU_TEST(asc_glob_match("a?c", "abc", &err) == 0);
	CU_TEST(err == 1);
	err = 0;
	CU_TEST(asc_glob_match("a[b]c", "abc", &err) == 0);
	CU_TEST(err == 1);
}

#define TESTS(T) \
	T(glob_star_basic) \
	T(glob_unsupported_tokens)

REGISTER_TESTS_SIMPLE(general_glob, TESTS)
