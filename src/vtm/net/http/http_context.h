/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file http_context.h
 *
 * @brief Context for callback functions
 */

#ifndef VTM_NET_HTTP_HTTP_CONTEXT_H_
#define VTM_NET_HTTP_HTTP_CONTEXT_H_

#include <vtm/core/api.h>
#include <vtm/core/dataset.h>
#include <vtm/net/http/http_memory.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vtm_http_ctx
{
	vtm_http_mem   *mem;   /**< the per-server HTTP memory */
	vtm_dataset    *wd;    /**< the per-thread data storage */
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_CONTEXT_H_ */
