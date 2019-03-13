/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_MEMORY_INTL_H_
#define VTM_NET_HTTP_HTTP_MEMORY_INTL_H_

#include <vtm/net/http/http_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

vtm_http_mem* vtm_http_mem_new(void);
void vtm_http_mem_free(vtm_http_mem *mem);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_MEMORY_INTL_H_ */
