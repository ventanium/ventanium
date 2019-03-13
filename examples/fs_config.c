/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <stdio.h>
#include <vtm/core/error.h>
#include <vtm/fs/config.h>

#define EXAMPLE_CONF	"./example.conf"

int create_example_config(void)
{
	FILE *fp;

	fp = fopen(EXAMPLE_CONF, "w");
	if (!fp)
		return VTM_ERROR;

	fprintf(fp, "# This is a comment\n");
	fprintf(fp, "; This is also a comment\n");
	fprintf(fp, "PORT = 512\n");
	fprintf(fp, "LOGGING = TRUE\n");
	fprintf(fp, "PATH = /example/path\n");

	fclose(fp);

	return VTM_OK;
}

int main(int argc, char **argv)
{
	vtm_dataset *conf;

read:
	/* try to read the example config file */
	conf = vtm_config_file_read(EXAMPLE_CONF);
	if (!conf) {
		/* if the example config has not been created yet, try to create it */
		if (vtm_err_get_code() == VTM_E_IO_FILE_NOT_FOUND) {
			printf("Trying to generate example config file..\n");
			if (create_example_config() == VTM_OK)
				goto read;
		}
		vtm_err_print();
		EXIT_FAILURE;
	}

	/* read config content */
	printf("Port as int is %d\n", vtm_dataset_get_int(conf, "PORT"));
	printf("Port as string is %s\n", vtm_dataset_get_string(conf, "PORT"));

	printf("Logging is: %s\n",
		vtm_dataset_get_bool(conf, "LOGGING") ? "enabled" : "disabled");

	printf("Path is %s\n", vtm_dataset_get_string(conf, "PATH"));

	/* release resources */
	vtm_dataset_free(conf);

	return 0;
}
