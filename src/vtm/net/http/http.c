/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

#include "http.h"

#include <stdlib.h> /* NULL */
#include <string.h> /* strcmp() */
#include <vtm/core/error.h>
#include <vtm/core/macros.h>

/* ########## HTTP VERSION ########## */

const char* VTM_HTTP_VERSIONS[] = {
	"HTTP/1.0",
	"HTTP/1.1",
	NULL
};

const char* vtm_http_get_version_string(enum vtm_http_version ver)
{
	switch (ver) {

		case VTM_HTTP_VER_1_0:
			return VTM_HTTP_VERSIONS[0];

		case VTM_HTTP_VER_1_1:
			return VTM_HTTP_VERSIONS[1];

		default:
			break;
	}

	return NULL;
}

int vtm_http_get_version(int major, int minor, enum vtm_http_version *ver)
{
	switch (major) {
		case 1:
			switch (minor) {
				case 0:    *ver = VTM_HTTP_VER_1_0;   return VTM_OK;
				case 1:    *ver = VTM_HTTP_VER_1_1;   return VTM_OK;
				default:   break;
			}
			break;

		default:
			break;
	}

	return VTM_E_NOT_FOUND;
}

/* ########## HTTP METHOD ########## */

const char* VTM_HTTP_METHODS[] = {
	"GET",
	"POST",
	"HEAD",
	"PUT",
	"PATCH",
	"DELETE"
	"TRACE"
	"OPTIONS",
	"CONNECT",
	NULL
};

int vtm_http_get_method(const char *str, enum vtm_http_method *method)
{
	size_t i;

	for (i=0; i < VTM_ARRAY_LEN(VTM_HTTP_METHODS); i++) {
		if (VTM_HTTP_METHODS[i] == NULL)
			break;

		if (strcmp(str, VTM_HTTP_METHODS[i]) == 0) {
			*method = (enum vtm_http_method) i;
			return VTM_OK;
		}
	}

	return VTM_E_NOT_FOUND;
}

/* ########## STATUS CODES ########## */

/* unknown */
static const char* const VTM_HTTP_PHRASE_000 = "Unknown";

/* 1xx information */
static const char* const VTM_HTTP_PHRASE_100 = "Continue";
static const char* const VTM_HTTP_PHRASE_101 = "Switching Protocols";
static const char* const VTM_HTTP_PHRASE_102 = "Processing";

/* 2xx success */
static const char* const VTM_HTTP_PHRASE_200 = "OK";

/* 3xx redirect */
static const char* const VTM_HTTP_PHRASE_300 = "Multiple Choices";
static const char* const VTM_HTTP_PHRASE_301 = "Moved Permanently";
static const char* const VTM_HTTP_PHRASE_302 = "Moved Temporarily";
static const char* const VTM_HTTP_PHRASE_303 = "See other";
static const char* const VTM_HTTP_PHRASE_304 = "Not modified";
static const char* const VTM_HTTP_PHRASE_305 = "Use proxy";
static const char* const VTM_HTTP_PHRASE_306 = "Reserved";
static const char* const VTM_HTTP_PHRASE_307 = "Temporary redirect";
static const char* const VTM_HTTP_PHRASE_308 = "Permanent redirect";

/* 4xx client error */
static const char* const VTM_HTTP_PHRASE_400 = "Bad request";
static const char* const VTM_HTTP_PHRASE_401 = "Unauthorized";
static const char* const VTM_HTTP_PHRASE_402 = "Reserved";
static const char* const VTM_HTTP_PHRASE_403 = "Forbidden";
static const char* const VTM_HTTP_PHRASE_404 = "Not found";
static const char* const VTM_HTTP_PHRASE_405 = "Method not allowed";

/* 5xx server error */
static const char* const VTM_HTTP_PHRASE_500 = "Internal server error";
static const char* const VTM_HTTP_PHRASE_501 = "Not implemented";
static const char* const VTM_HTTP_PHRASE_502 = "Bad gateway";
static const char* const VTM_HTTP_PHRASE_503 = "Service unavailable";
static const char* const VTM_HTTP_PHRASE_504 = "Gateway timeout";
static const char* const VTM_HTTP_PHRASE_505 = "HTTP Version not supported";

const char* vtm_http_get_status_phrase(int code)
{
	switch (code) {
		case VTM_HTTP_100_CONTINUE:					return VTM_HTTP_PHRASE_100;
		case VTM_HTTP_101_SWITCHING_PROTOCOLS:		return VTM_HTTP_PHRASE_101;
		case VTM_HTTP_102_PROCESSING:				return VTM_HTTP_PHRASE_102;

		case VTM_HTTP_200_OK:						return VTM_HTTP_PHRASE_200;

		case VTM_HTTP_300_MULTIPLE_CHOICES:			return VTM_HTTP_PHRASE_300;
		case VTM_HTTP_301_MOVED_PERMANENTLY:		return VTM_HTTP_PHRASE_301;
		case VTM_HTTP_302_MOVED_TEMPORARILY:		return VTM_HTTP_PHRASE_302;
		case VTM_HTTP_303_SEE_OTHER:				return VTM_HTTP_PHRASE_303;
		case VTM_HTTP_304_NOT_MODIFIED:				return VTM_HTTP_PHRASE_304;
		case VTM_HTTP_305_USE_PROXY:				return VTM_HTTP_PHRASE_305;
		case VTM_HTTP_306_RESERVED:					return VTM_HTTP_PHRASE_306;
		case VTM_HTTP_307_TEMPORARY_REDIRECT:		return VTM_HTTP_PHRASE_307;
		case VTM_HTTP_308_PERMANENT_REDIRECT:		return VTM_HTTP_PHRASE_308;

		case VTM_HTTP_400_BAD_REQUEST:				return VTM_HTTP_PHRASE_400;
		case VTM_HTTP_401_UNAUTHORIZED:				return VTM_HTTP_PHRASE_401;
		case VTM_HTTP_402_RESERVED:					return VTM_HTTP_PHRASE_402;
		case VTM_HTTP_403_FORBIDDEN:				return VTM_HTTP_PHRASE_403;
		case VTM_HTTP_404_NOT_FOUND:				return VTM_HTTP_PHRASE_404;
		case VTM_HTTP_405_METHOD_NOT_ALLOWED:		return VTM_HTTP_PHRASE_405;

		case VTM_HTTP_500_INTERNAL_SERVER_ERROR:	return VTM_HTTP_PHRASE_500;
		case VTM_HTTP_501_NOT_IMPLEMENTED:			return VTM_HTTP_PHRASE_501;
		case VTM_HTTP_502_BAD_GATEWAY:				return VTM_HTTP_PHRASE_502;
		case VTM_HTTP_503_SERVICE_UNAVAILABLE:		return VTM_HTTP_PHRASE_503;
		case VTM_HTTP_504_GATEWAY_TIMEOUT:			return VTM_HTTP_PHRASE_504;
		case VTM_HTTP_505_HTTP_VER_NOT_SUPPORTED:	return VTM_HTTP_PHRASE_505;

		default:									return VTM_HTTP_PHRASE_000;
	}
}

/* ########## HEADER FIELDS ########## */

const char* const VTM_HTTP_HEADER_AUTHORIZATION = "Authorization";
const char* const VTM_HTTP_HEADER_CONNECTION = "Connection";
const char* const VTM_HTTP_HEADER_CONTENT_LENGTH = "Content-Length";
const char* const VTM_HTTP_HEADER_CONTENT_TYPE = "Content-Type";
const char* const VTM_HTTP_HEADER_DATE = "Date";
const char* const VTM_HTTP_HEADER_EXPIRES = "Expires";
const char* const VTM_HTTP_HEADER_HOST = "Host";
const char* const VTM_HTTP_HEADER_SERVER = "Server";
const char* const VTM_HTTP_HEADER_TRANSFER_ENCODING = "Transfer-Encoding";
const char* const VTM_HTTP_HEADER_UPGRADE = "Upgrade";
const char* const VTM_HTTP_HEADER_USER_AGENT = "User-Agent";
const char* const VTM_HTTP_HEADER_WWW_AUTHENTICATE = "WWW-Authenticate";

const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT = "Sec-WebSocket-Accept";
const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_KEY = "Sec-WebSocket-Key";
const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_PROTOCOL = "Sec-WebSocket-Protocol";
const char* const VTM_HTTP_HEADER_SEC_WEBSOCKET_VERSION = "Sec-WebSocket-Version";

/* ########## HEADER VALUES ########## */

const char* const VTM_HTTP_VALUE_CHUNKED = "chunked";
const char* const VTM_HTTP_VALUE_CLOSE = "close";
const char* const VTM_HTTP_VALUE_IDENTITY = "identity";
const char* const VTM_HTTP_VALUE_KEEP_ALIVE = "keep-alive";
const char* const VTM_HTTP_VALUE_UPGRADE = "Upgrade";
const char* const VTM_HTTP_VALUE_WEBSOCKET = "websocket";
