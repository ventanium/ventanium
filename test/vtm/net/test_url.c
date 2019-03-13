/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtf.h>

#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/net/url.h>

static void test_url(void)
{
	int rc;
	struct vtm_url url;

	rc = vtm_url_parse("https://www.example.com:12345/path", &url);
	VTM_TEST_CHECK(rc == VTM_OK, "url parse");

	VTM_TEST_CHECK(url.scheme == VTM_URL_SCHEME_HTTPS, "check url scheme");
	VTM_TEST_CHECK(url.port == 12345, "check url port");
	VTM_TEST_CHECK(strcmp(url.host, "www.example.com") == 0, "check url host");
	VTM_TEST_CHECK(strcmp(url.path, "/path") == 0, "check url path");

	vtm_url_release(&url);
	VTM_TEST_PASSED("url release");
}

static void test_encoding(void)
{
	int rc;
	char buf[64];

	rc = vtm_url_encode("ABC abc 123 %+!", buf, sizeof(buf));
	VTM_TEST_CHECK(rc == VTM_OK, "url encode");

	rc = strcmp(buf, "ABC%20abc%20123%20%25%2B%21");
	VTM_TEST_CHECK(rc == 0, "url encode check");
}

static void test_decoding(void)
{
	int rc;
	char buf[64];

	rc = vtm_url_decode("ABC%20abc%20123%20%25%2B%21", buf, sizeof(buf));
	VTM_TEST_CHECK(rc == VTM_OK, "url decode");

	rc = strcmp(buf, "ABC abc 123 %+!");
	VTM_TEST_CHECK(rc == 0, "url decode check");
}

extern void test_vtm_net_url(void)
{
	VTM_TEST_LABEL("url");
	test_url();
	test_encoding();
	test_decoding();
}
