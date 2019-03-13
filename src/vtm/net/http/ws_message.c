/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#include "ws_message_intl.h"

#include <stdlib.h> /* free() */

void vtm_ws_msg_init(struct vtm_ws_msg *msg, enum vtm_ws_msg_type type, void *data, size_t len)
{
	msg->type = type;
	msg->data = data;
	msg->len = len;
}

void vtm_ws_msg_release(struct vtm_ws_msg *msg)
{
	free(msg->data);
}
