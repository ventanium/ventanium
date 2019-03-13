/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file string.h
 *
 * @brief String utils
 */

#ifndef VTM_CORE_STRING_H_
#define VTM_CORE_STRING_H_

#include <stdarg.h> /* va_list */
#include <vtm/core/api.h>
#include <vtm/core/list.h>
#include <vtm/core/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns a copy of the input string.
 *
 * @param in the string that should be copied
 * @return a copy of the string
 * @return NULL if not enough memory could be allocated
 */
VTM_API char* vtm_str_copy(const char *in);

/**
 * Returns a copy of the first n characters of the input string.
 *
 * @param in the input string
 * @param len the number of bytes that should be copied
 * @return a copy of the first n bytes
 * @return NULL if not enough memory could be allocated
 */
VTM_API char* vtm_str_ncopy(const char *in, size_t len);

/**
 * Compares s1 to s2 ignoring case.
 *
 * @param s1
 * @param s2
 * @return -1 if first character of s1 that does not match is lesser than
 *          the coresponding character from s2
 * @return  0 if the strings are equal
 * @return  1 if the first character of s1 that does not match is greater than
 *          the corresponding character from s2
 */
VTM_API int vtm_str_casecmp(const char *s1, const char *s2);

/**
 * Splits the given input string by given delimiter
 *
 * @param in the input string
 * @param delim the delimiter by that the input should be split
 * @param count the maximum number of result tokens
 * @return a list containing the tokens
 * @return NULL if an error occured
 */
VTM_API vtm_list* vtm_str_split(const char *in, const char *delim, unsigned int count);

/**
 * Splits the given input string by given delimiter.
 *
 * @param in NUL-terminated input string
 * @param delim NUL-terminated delimiter string
 * @param[out] out_tokens filled with array of pointers to tokens
 * @param[out] token_count filled with number of tokens in array
 * @return VTM_OK if the split operation was successful
 * @return VTM_E_MALLOC if not enough memory could be allocated
 */
VTM_API int vtm_str_split_ex(const char *in, const char *delim, char ***out_tokens, size_t *token_count);

/**
 * Removes whitespaces at begin and end of the input string.
 *
 * The result is a pointer to same string and does not need to be freed.
 *
 * @param in the input string
 * @return pointer to first non-whitespace character
 */
VTM_API char* vtm_str_trim(char *in);

/**
 * Prints fmt with arguments in returned string.
 *
 * @param fmt the format string
 * @return the formatted string
 * @return NULL if an error occured, for example if not enough memory could
 *         be allocated
 */
VTM_API char* vtm_str_printf(const char *fmt, ...);

/**
 * Prints fmt with arguments in returned string.
 *
 * @param fmt the format string
 * @param ap the variadic argument list
 * @return the formatted string
 * @return NULL if an error occured, for example if not enough memory could
 *         be allocated
 */
VTM_API char* vtm_str_vprintf(const char *fmt, va_list ap);

/**
 * Searches first occurence of given character.
 *
 * @param in the input string
 * @param c the searched character
 * @param len the length of the input string
 * @return >= 0 when character was found
 * @return -1 if not found
 * @return -2 when len > SSIZE_MAX
 */
VTM_API ssize_t vtm_str_index_of(const char *in, char c, size_t len);

/**
 * Searches first occurence of any of the given characters.
 *
 * @param in the input string
 * @param len the length of the input string
 * @param chars array of searched characters
 * @param char_count char array length
 * @param[out] out_found the found character
 * @return => 0 when one of the characters was found
 * @return -1 if no character was found
 */
VTM_API ssize_t vtm_str_index_chars(const char *in, size_t len, char *chars, size_t char_count, char *out_found);

/**
 * Searches the first occurence of the given pattern.
 *
 * @param in the input string
 * @param pattern the search pattern
 * @param input_len the length of the input string
 * @param pattern_len the length of the pattern
 * @return >= 0 when pattern was found
 * @return -1 if not found
 * @return -2 when input_len > SSIZE_MAX
 */
VTM_API ssize_t vtm_str_index_pattern(const char *in, const char *pattern, size_t input_len, size_t pattern_len);

/**
 * Checks if the list with given delimeter contains search string.
 *
 * @param in the input string
 * @param delim the list delimeter
 * @param search the search patttern
 * @param nocase true if comparison should be case-insensitive
 * @return true if the list contains the search string
 * @return false otherwise
 */
VTM_API bool vtm_str_list_contains(const char *in, const char *delim, const char *search, bool nocase);

/**
 * Evaluates how often the given pattern exists in the input string.
 *
 * @param in the input string
 * @param pattern the search pattern
 * @return the number of pattern occurences
 */
VTM_API size_t vtm_str_pattern_count(const char *in, const char *pattern);

/**
 * Checks if first string begins with the seconds string.
 *
 * @param the input string
 * @param the prefix
 * @return true if input starts with given prefix
 * @return false otherwise
 */
VTM_API bool vtm_str_starts_with(const char *in, const char *start);

#ifdef __cplusplus
}
#endif

#endif /* VTM_CORE_STRING_H_ */
