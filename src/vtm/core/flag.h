/*
 * Copyright (C) 2018-2019 Matthias Benkendorf
 */

/**
 * @file flag.h
 *
 * @brief Helper macros for bitwise flag manipulation
 */

#ifndef VTM_CORE_FLAG_H_
#define VTM_CORE_FLAG_H_

/**
 * Set bits
 *
 * @param VAL the value where bits should be set
 * @param FLAG bits which should be set to 1
 */
#define vtm_flag_set(VAL,FLAG)      (VAL) |= (FLAG)

/**
 * Unset bits
 *
 * @param VAL the value where bits should unset
 * @param FLAG bits which should be set to 0
 */
#define vtm_flag_unset(VAL,FLAG)    (VAL) &= ~(FLAG)

/**
 * Check if bits are set
 *
 * @param VAL the value where bits should be checked
 * @param FLAG bits which should be tested
 */
#define vtm_flag_is_set(VAL,FLAG)   ((VAL) & (FLAG)) == (FLAG)

#endif /* VTM_CORE_FLAG_H_ */
