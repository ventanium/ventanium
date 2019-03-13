/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file file.h
 *
 * @brief Helper functions about file handling
 */

#ifndef VTM_FS_FILE_H_
#define VTM_FS_FILE_H_

#include <stdio.h> /* FILE */
#include <vtm/core/api.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VTM_FILE_ATTR_REG       1  /**< regular file */
#define VTM_FILE_ATTR_DIR       2  /**< directory */

/**
 * Determines the file size.
 *
 * @return the file size in bytes
 */
VTM_API uint64_t vtm_file_get_fsize(FILE *fp);

/**
 * Reads one line delimited by \\n from file.
 *
 * The same buffer can be used in subsequent reads.
 * The buffer is resized if more space is needed.
 *
 * @param fp the file where to read from
 * @param[in,out] the allocated buffer with line content
 * @param[in,out] size of the buffer
 *
 * @return VTM_OK if read of line was successful
 * @return VTM_E_IO_EOF when end of file is reached
 */
VTM_API int vtm_file_getline(FILE *fp, char **buf, size_t *buf_len);

/**
 * Determines the extension of the given filename.
 *
 * @return pointer to extension in in input string
 * @return NULL if file has no extension
 */
VTM_API const char* vtm_file_get_ext(const char *filename);

/**
 * Reads the attributes of given file.
 *
 * @param fp the file where to read from
 * @param[out] attr attributes are stored here
 * @return VTM_OK if attributes were read successfuully
 * @return VTM_E_IO_UNKNOWN if an error occured
 */
VTM_API int vtm_file_get_fattr(FILE *fp, int *attr);

#ifdef __cplusplus
}
#endif

#endif /* VTM_FS_FILE_H_ */
