/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h>
#include <vtm/core/format.h>

static void test_format(void)
{
	int n;
	char buf[32];
	unsigned char c;

	/* fmt int */
	n = vtm_fmt_int(buf, -12345);
	VTM_TEST_CHECK(n == 6, "fmt int length");
	VTM_TEST_CHECK(strncmp(buf, "-12345", n) == 0, "fmt int content");

	/* fmt uint */
	n = vtm_fmt_uint(buf, 12345);
	VTM_TEST_CHECK(n == 5, "fmt uint length");
	VTM_TEST_CHECK(strncmp(buf, "12345", n) == 0, "fmt uint content");

	/* fmt int64 */
	n = vtm_fmt_int64(buf, INT64_MIN);
	VTM_TEST_CHECK(n == VTM_FMT_CHARS_INT64, "fmt int64 length");
	VTM_TEST_CHECK(strncmp(buf, "-9223372036854775808", n) == 0, "fmt int64 content");

	/* fmt uint64 */
	n = vtm_fmt_uint64(buf, UINT64_MAX);
	VTM_TEST_CHECK(n == VTM_FMT_CHARS_INT64, "fmt uint64 length");
	VTM_TEST_CHECK(strncmp(buf, "18446744073709551615", n) == 0, "fmt uint64 content");

	/* fmt hex size */
	n = vtm_fmt_hex_size(buf, 123456);
	VTM_TEST_CHECK(n == 5, "fmt hex size length");
	VTM_TEST_CHECK(strncmp(buf, "1E240", n) == 0, "fmt hex size content");

	/* fmt hex mem */
	c = 199;
	n = vtm_fmt_hex_mem(buf, &c, 1, false);
	VTM_TEST_CHECK(n == 2, "fmt hex mem length");
	VTM_TEST_CHECK(strncmp(buf, "c7", n) == 0, "fmt hex mem content");
}

extern void test_vtm_core_format(void)
{
	VTM_TEST_LABEL("format");
	test_format();
}
