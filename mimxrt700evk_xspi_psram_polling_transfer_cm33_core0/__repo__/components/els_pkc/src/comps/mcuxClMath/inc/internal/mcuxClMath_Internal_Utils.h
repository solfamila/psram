/*--------------------------------------------------------------------------*/
/* Copyright 2022-2023 NXP                                                  */
/*                                                                          */
/* NXP Proprietary. This software is owned or controlled by NXP and may     */
/* only be used strictly in accordance with the applicable license terms.   */
/* By expressly accepting such terms or by downloading, installing,         */
/* activating and/or otherwise using the software, you are agreeing that    */
/* you have read, and that you agree to comply with and are bound by, such  */
/* license terms.  If you do not agree to be bound by the applicable        */
/* license terms, then you may not retain, install, activate or otherwise   */
/* use the software.                                                        */
/*--------------------------------------------------------------------------*/

/**
 * @file  mcuxClMath_Internal_Utils.h
 * @brief platform independent abstraction over math related builtin functions
 */

#ifndef MCUXCLMATH_INTERNAL_UTILS_H_
#define MCUXCLMATH_INTERNAL_UTILS_H_

#include <mcuxClCore_Platform.h>
#include <platform_specific_headers.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Count leading zeros of non-zero value.
 * If the value is 0, the result is undefined.
 */
MCUX_CSSL_FP_FUNCTION_DEF(mcuxClMath_CountLeadingZerosWord)
static inline uint32_t mcuxClMath_CountLeadingZerosWord(uint32_t value)
{
#ifdef __CLZ
    return __CLZ(value);
#else
    return (uint32_t)__builtin_clz(value);
#endif
}

/*
 * Count trailing zeros of non-zero value.
 * In case the value is 0,
 * the result might be undefined if using compiler built-in function;
 * or 32 if using the software implementation.
 */
/**
 * [DESIGN]
 * The software implementation counts the trailing 1 in inverse of input,
 * by checking the LSBit and right-shifting the inverse in a loop, until
 * any 0 bit is right-shifted to LSBit. Since the right-shifting will set
 * MSBit 0, the loop will execute not more than 32 iterations.
 */
MCUX_CSSL_FP_FUNCTION_DEF(mcuxClMath_CountTrailingZeroesWord)
static inline uint32_t mcuxClMath_CountTrailingZeroesWord(uint32_t value)
{
#if defined(__CLZ) && defined(__RBIT)
    return  __CLZ(__RBIT(value));
#else
    uint32_t zeroes = 0u;
    uint32_t inverseValue = ~value;

    while (1u == (1u & inverseValue))
    {
        MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_OVERFLOW("This loop executes up to 32 iterations, \
                because MSBit is cleared when right-shifting unsigned inverseValue.")
        zeroes++;
        MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_OVERFLOW()
        inverseValue >>= 1u;
    }

    return zeroes;
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*MCUXCLMATH_INTERNAL_UTILS_H_ */
