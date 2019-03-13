/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file common.h
 *
 * @brief Common network definitions
 */

#ifndef VTM_NET_COMMON_H_
#define VTM_NET_COMMON_H_

#include <vtm/core/api.h>
#include <vtm/core/system.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_NET_BYTEORDER      VTM_BYTEORDER_BE

enum vtm_net_recv_stat
{
	VTM_NET_RECV_STAT_ERROR,    /**< Error occured */
	VTM_NET_RECV_STAT_CLOSED,   /**< Input stream closed */
	VTM_NET_RECV_STAT_INVALID,  /**< Invalid data received */
	VTM_NET_RECV_STAT_AGAIN,    /**< Not enough data, retry again later */
	VTM_NET_RECV_STAT_COMPLETE  /**< Receiving operation complete */
};

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_COMMON_H_ */
