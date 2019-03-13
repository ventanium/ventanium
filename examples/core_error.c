/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include <vtm/core/error.h>

/*
 * This function simulates the occurrence of an error for
 * every input value except zero.
 */
int test_function(int input)
{
	if (input < 0)
		return vtm_err_set(VTM_E_INVALID_ARG);
	else if (input > 0)
		return vtm_err_setf(VTM_E_INVALID_ARG, "Argument was %d", input);

	return VTM_OK;
}

int main(void)
{
	int rc;

	rc = test_function(-1);
	if (rc != VTM_OK) {
		/*
		 * Display error code, description and location
		 * with built-in function
		 */
		vtm_err_print();
	}

	rc = test_function(1);
	if (rc != VTM_OK) {
		/* Display error details manually */
		fprintf(stderr, "Error code was: %d\n", vtm_err_get_code());
		fprintf(stderr, "Error name was: %s\n", vtm_err_get_name());
		fprintf(stderr, "Error msg was:  %s\n", vtm_err_get_msg());
		fprintf(stderr, "Error file was: %s\n", vtm_err_get_file());
		fprintf(stderr, "Error line was: %ld\n", vtm_err_get_line());
	}

	return 0;
}
