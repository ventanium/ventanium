/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include <vtf.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vtm/core/error.h>
#include <vtm/fs/file.h>

#define VTM_FS_TEST_FILE    "./test/data/fs/test.size"

static void test_file_size(void)
{
	FILE *fp;
	uint64_t filesize;

	fp = fopen(VTM_FS_TEST_FILE, "r");
	VTM_TEST_CHECK(fp != NULL, "test file opened");

	filesize = vtm_file_get_fsize(fp);
	VTM_TEST_CHECK(filesize == 5, "file size");

	fclose(fp);
}

static void test_file_getline(void)
{
	FILE *fp;
	char *buf;
	size_t len;
	int rc;

	buf = NULL;
	len = 0;

	fp = fopen("./test/data/fs/test_getline.txt", "r");
	VTM_TEST_ASSERT(fp != NULL, "test getline file opened");

	rc = vtm_file_getline(fp, &buf, &len);
	VTM_TEST_CHECK(rc == VTM_OK, "getline first line");
	VTM_TEST_CHECK(buf != NULL, "getline buffer alloc");
	VTM_TEST_CHECK(strcmp(buf, "ABC") == 0, "first line comparison");

	rc = vtm_file_getline(fp, &buf, &len);
	VTM_TEST_CHECK(rc == VTM_OK, "getline second line");
	VTM_TEST_CHECK(strlen(buf) == 0, "empty line comparison");

	rc = vtm_file_getline(fp, &buf, &len);
	VTM_TEST_CHECK(rc == VTM_E_IO_EOF, "getline eof");

	fclose(fp);
	free(buf);
}

static void test_file_ext(void)
{
	const char *ext;

	ext = vtm_file_get_ext(VTM_FS_TEST_FILE);
	VTM_TEST_CHECK(strcmp(ext, "size") == 0, "file ext");
}

extern void test_vtm_fs_file(void)
{
	VTM_TEST_LABEL("file");
	test_file_size();
	test_file_getline();
	test_file_ext();
}
