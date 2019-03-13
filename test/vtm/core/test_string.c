/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/error.h>
#include <vtm/core/string.h>

#define TEST_STRING    "1234 ABCD"
#define TRIM_STRING    "  ABCD  "

static void test_string_copy(void)
{
	char *copy;

	/* copy */
	copy = vtm_str_copy(TEST_STRING);
	VTM_TEST_ASSERT(copy != NULL, "string copy");
	VTM_TEST_CHECK(strcmp(TEST_STRING, copy) == 0, "string copy content");
	free(copy);

	/* copy n characters */
	copy = vtm_str_ncopy(TEST_STRING, 4);
	VTM_TEST_ASSERT(copy != NULL, "string ncopy");
	VTM_TEST_CHECK(strlen(copy) == 4, "string ncopy length");
	VTM_TEST_CHECK(strncmp(TEST_STRING, copy, 4) == 0, "string ncopy content");
	free(copy);
}

static void test_string_misc(void)
{
	int n;
	char *copy, *trimmed;

	/* case insensitive compare */
	n = vtm_str_casecmp(TEST_STRING, "1234 abcd");
	VTM_TEST_CHECK(n == 0, "string casecmp");

	/* trim */
	copy = vtm_str_copy(TRIM_STRING);
	VTM_TEST_ASSERT(copy != NULL, "string copy");

	trimmed = vtm_str_trim(copy);
	VTM_TEST_CHECK(strlen(trimmed) == 4, "string trimmed length");
	VTM_TEST_CHECK(strcmp(trimmed, "ABCD") == 0, "string trimmed content");
	free(copy);

	/* starts with */
	VTM_TEST_CHECK(vtm_str_starts_with(TEST_STRING, "1234"), "string starts with");
}

static void test_string_index(void)
{
	ssize_t index;

	/* index of char */
	index = vtm_str_index_of(TEST_STRING, '1', strlen(TEST_STRING));
	VTM_TEST_CHECK(index == 0, "string index begin");

	index = vtm_str_index_of(TEST_STRING, '5', strlen(TEST_STRING));
	VTM_TEST_CHECK(index == -1, "string index not found");

	index = vtm_str_index_of(TEST_STRING, 'D', strlen(TEST_STRING) / 2);
	VTM_TEST_CHECK(index == -1, "string index not found in span");

	/* index of pattern */
	index = vtm_str_index_pattern(TEST_STRING, "ABCD", strlen(TEST_STRING), 4);
	VTM_TEST_CHECK(index == 5, "string pattern found");
}

static void test_string_list_contains(void)
{
	bool rc;

	rc = vtm_str_list_contains("deflate, gzip ", ",", "flate", false);
	VTM_TEST_CHECK(rc == false, "string list contains partial");

	rc = vtm_str_list_contains("deflate, gzip ", ",", "gzip", false);
	VTM_TEST_CHECK(rc == true, "string list contains");
}

static void test_string_patterns(void)
{
	int rc;
	size_t i, n;
	char **tokens;

	/* pattern count */
	n = vtm_str_pattern_count("ABC BC CB", "BC");
	VTM_TEST_CHECK(n == 2, "string pattern count");

	/* split extended */
	rc = vtm_str_split_ex("AB, CD ,EF , ", ",", &tokens, &n);
	VTM_TEST_CHECK(rc == VTM_OK, "string split ex result");
	VTM_TEST_CHECK(n == 3, "string split ex token count");

	VTM_TEST_CHECK(strcmp(tokens[0], "AB") == 0, "string tok1");
	VTM_TEST_CHECK(strcmp(tokens[1], "CD") == 0, "string tok2");
	VTM_TEST_CHECK(strcmp(tokens[2], "EF") == 0, "string tok3");

	for (i=0; i < n; i++)
		free(tokens[i]);
	free(tokens);
}

extern void test_vtm_core_string(void)
{
	VTM_TEST_LABEL("string");
	test_string_copy();
	test_string_misc();
	test_string_index();
	test_string_list_contains();
	test_string_patterns();
}
