/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_HTTP_HTTP_H_
#define VTM_NET_HTTP_HTTP_H_

#include <vtm/core/api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ########## HTTP VERSION ########## */

/* http protocol version */
enum vtm_http_version
{
	VTM_HTTP_VER_1_0,
	VTM_HTTP_VER_1_1
};

VTM_API extern const char* VTM_HTTP_VERSIONS[];

VTM_API const char* vtm_http_get_version_string(enum vtm_http_version ver);
VTM_API int vtm_http_get_version(int major, int minor, enum vtm_http_version *ver);

/* ########## HTTP METHOD ########## */

/* request method */
enum vtm_http_method
{
	VTM_HTTP_METHOD_GET,
	VTM_HTTP_METHOD_POST,
	VTM_HTTP_METHOD_HEAD,
	VTM_HTTP_METHOD_PUT,
	VTM_HTTP_METHOD_PATCH,
	VTM_HTTP_METHOD_DELETE,
	VTM_HTTP_METHOD_TRACE,
	VTM_HTTP_METHOD_OPTIONS,
	VTM_HTTP_METHOD_CONNECT
};

VTM_API extern const char* VTM_HTTP_METHODS[];

VTM_API int vtm_http_get_method(const char *str, enum vtm_http_method *method);

#define VTM_HTTP_MAX_METHOD_LENGTH                7

/* ########## STATUS CODES ########## */

/* 1xx information */
#define VTM_HTTP_100_CONTINUE                   100
#define VTM_HTTP_101_SWITCHING_PROTOCOLS        101
#define VTM_HTTP_102_PROCESSING                 102

/* 2xx success */
#define VTM_HTTP_200_OK                         200

/* 3xx redirect */
#define VTM_HTTP_300_MULTIPLE_CHOICES           300
#define VTM_HTTP_301_MOVED_PERMANENTLY          301
#define VTM_HTTP_302_MOVED_TEMPORARILY          302
#define VTM_HTTP_303_SEE_OTHER                  303
#define VTM_HTTP_304_NOT_MODIFIED               304
#define VTM_HTTP_305_USE_PROXY                  305
#define VTM_HTTP_306_RESERVED                   306
#define VTM_HTTP_307_TEMPORARY_REDIRECT         307
#define VTM_HTTP_308_PERMANENT_REDIRECT         308

/* 4xx client error */
#define VTM_HTTP_400_BAD_REQUEST                400
#define VTM_HTTP_401_UNAUTHORIZED               401
#define VTM_HTTP_402_RESERVED                   402
#define VTM_HTTP_403_FORBIDDEN                  403
#define VTM_HTTP_404_NOT_FOUND                  404
#define VTM_HTTP_405_METHOD_NOT_ALLOWED         405

/* 5xx server error */
#define VTM_HTTP_500_INTERNAL_SERVER_ERROR      500
#define VTM_HTTP_501_NOT_IMPLEMENTED            501
#define VTM_HTTP_502_BAD_GATEWAY                502
#define VTM_HTTP_503_SERVICE_UNAVAILABLE        503
#define VTM_HTTP_504_GATEWAY_TIMEOUT            504
#define VTM_HTTP_505_HTTP_VER_NOT_SUPPORTED     505

VTM_API const char* vtm_http_get_status_phrase(int code);

/* ########## HEADER FIELDS ########## */

VTM_API extern const char* const VTM_HTTP_HEADER_AUTHORIZATION;
VTM_API extern const char* const VTM_HTTP_HEADER_CONNECTION;
VTM_API extern const char* const VTM_HTTP_HEADER_CONTENT_LENGTH;
VTM_API extern const char* const VTM_HTTP_HEADER_CONTENT_TYPE;
VTM_API extern const char* const VTM_HTTP_HEADER_DATE;
VTM_API extern const char* const VTM_HTTP_HEADER_EXPIRES;
VTM_API extern const char* const VTM_HTTP_HEADER_HOST;
VTM_API extern const char* const VTM_HTTP_HEADER_SERVER;
VTM_API extern const char* const VTM_HTTP_HEADER_TRANSFER_ENCODING;
VTM_API extern const char* const VTM_HTTP_HEADER_UPGRADE;
VTM_API extern const char* const VTM_HTTP_HEADER_USER_AGENT;
VTM_API extern const char* const VTM_HTTP_HEADER_WWW_AUTHENTICATE;

VTM_API extern const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT;
VTM_API extern const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_KEY;
VTM_API extern const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_PROTOCOL;
VTM_API extern const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_VERSION;

/* ########## HEADER VALUES ########## */

VTM_API extern const char* const VTM_HTTP_VALUE_CHUNKED;
VTM_API extern const char* const VTM_HTTP_VALUE_CLOSE;
VTM_API extern const char* const VTM_HTTP_VALUE_IDENTITY;
VTM_API extern const char* const VTM_HTTP_VALUE_KEEP_ALIVE;
VTM_API extern const char* const VTM_HTTP_VALUE_UPGRADE;
VTM_API extern const char* const VTM_HTTP_VALUE_WEBSOCKET;

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_HTTP_HTTP_H_ */
