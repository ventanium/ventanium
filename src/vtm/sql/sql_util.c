/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "sql_util_intl.h"

#include <stdlib.h> /* free() */
#include <ctype.h> /* isalnum() */
#include <vtm/core/error.h>

int vtm_sql_extract_query_params(struct vtm_buf *buf, const char *query, vtm_list *names)
{
	int rc;
	struct vtm_buf namebuf;
	char c, *name;
	bool quote;

	quote = false;

	vtm_list_set_free_func(names, free);
	vtm_buf_init(&namebuf, VTM_BYTEORDER_LE);

	while ( *query != '\0' ) {
		c = *query++;
		switch (c) {
			case '\'':
				quote = !quote;
				break;

			case ':':
				if (!quote) {
					vtm_buf_clear(&namebuf);
					while (*query != '\0' && (isalnum(*query) || *query == '_'))
						vtm_buf_putc(&namebuf, *query++);

					name = malloc(namebuf.used+1);
					if (!name) {
						vtm_err_oom();
						rc = vtm_err_get_code();
						goto end;
					}

					vtm_buf_getm(&namebuf, name, namebuf.used);
					name[namebuf.used] = '\0';

					vtm_list_add_va(names, name);
					c = '?';
				}
				break;

			default:
				break;
		}
		vtm_buf_putc(buf, c);
	}

	rc = VTM_OK;

end:
	vtm_buf_release(&namebuf);

	return rc;
}
