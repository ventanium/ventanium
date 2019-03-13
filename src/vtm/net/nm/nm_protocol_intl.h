/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

#ifndef VTM_NET_NM_NM_PROTOCOL_INTL_H_
#define VTM_NET_NM_NM_PROTOCOL_INTL_H_

#include <vtm/core/buffer.h>
#include <vtm/core/dataset.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_NM_PROTO_MAGIC         'V'
#define VTM_NM_PROTO_VER_1          1

#define VTM_NM_TNUM_INT8            1
#define VTM_NM_TNUM_UINT8           2
#define VTM_NM_TNUM_INT16           3
#define VTM_NM_TNUM_UINT16          4
#define VTM_NM_TNUM_INT32           5
#define VTM_NM_TNUM_UINT32          6
#define VTM_NM_TNUM_INT64           7
#define VTM_NM_TNUM_UINT64          8
#define VTM_NM_TNUM_BOOL            9
#define VTM_NM_TNUM_CHAR           10
#define VTM_NM_TNUM_SCHAR          11
#define VTM_NM_TNUM_UCHAR          12
#define VTM_NM_TNUM_FLOAT          13
#define VTM_NM_TNUM_DOUBLE         14
#define VTM_NM_TNUM_STRING         15

int vtm_nm_type_to_num(enum vtm_elem_type type, unsigned char *c);
int vtm_nm_type_from_num(enum vtm_elem_type *type, unsigned char c);

int vtm_nm_msg_to_buf(vtm_dataset *msg, struct vtm_buf *buf);
int vtm_nm_msg_from_buf(vtm_dataset *msg, struct vtm_buf *buf);

#ifdef __cplusplus
}
#endif

#endif /* VTM_NET_NM_NM_PROTOCOL_INTL_H_ */
