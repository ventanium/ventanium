/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTF_H_
#define VTF_H_

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_TEST_LABEL(LABEL) vtm_test_set_label(LABEL, __FILE__)
#define VTM_TEST_CHECK(COND, DESC) vtm_test_check((COND), DESC, __FILE__, __LINE__)
#define VTM_TEST_PASSED(DESC) vtm_test_passed(DESC, __FILE__, __LINE__)
#define VTM_TEST_FAILED(DESC) vtm_test_failed(DESC, __FILE__, __LINE__)
#define VTM_TEST_ASSERT(COND, DESC) vtm_test_assert((COND), DESC, __FILE__, __LINE__)
#define VTM_TEST_ABORT(DESC) vtm_test_abort(DESC, __FILE__, __LINE__)

enum vtm_test_opt
{
	VTM_TEST_OPT_PRINT_PASSED
};

typedef void(*vtm_test_func)(void);

void vtm_test_set_option(enum vtm_test_opt opt, int val);

void vtm_test_begin(void);
void vtm_test_set_module(const char *name);
void vtm_test_set_label(const char *name, const char *file);
void vtm_test_run(vtm_test_func fn);
void vtm_test_summary(void);
void vtm_test_end(void);

void vtm_test_check(int result, const char *desc, const char *file, unsigned int line);
void vtm_test_passed(const char *desc, const char *file, unsigned int line);
void vtm_test_failed(const char *desc, const char *file, unsigned int line);
void vtm_test_assert(int result, const char *desc, const char *file, unsigned int line);
void vtm_test_abort(const char *desc, const char *file, unsigned int line);

#ifdef __cplusplus
}
#endif

#endif /* VTF_H_ */
