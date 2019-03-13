/*
 * Copyright (C) 2018 Matthias Benkendorf
 */

/**
 * @file macros.h
 *
 * @brief Basic macros
 */

#ifndef VTM_CORE_MACRO_H_
#define VTM_CORE_MACRO_H_

/**
 * Calculates the number of items in an array.
 *
 * @param x the array
 */
#define VTM_ARRAY_LEN(x)    sizeof(x) / sizeof(x[0])

/**
 * Suppresses unused variable compiler warnings.
 *
 * @param VAR the variable for which warnings should be supressed.
 */
#define VTM_UNUSED(VAR)     (void)VAR

/**
 * Evaluates to a string representation of the input.
 *
 * @param X the input
 */
#define VTM_TO_STR(X)       VTM_TO_STR_INTL(X)
#define VTM_TO_STR_INTL(X)  #X

#endif /* VTM_CORE_MACRO_H_ */
