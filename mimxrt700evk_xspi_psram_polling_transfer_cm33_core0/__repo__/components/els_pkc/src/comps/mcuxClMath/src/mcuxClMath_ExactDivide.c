/*--------------------------------------------------------------------------*/
/* Copyright 2021-2024 NXP                                                  */
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
 * @file  mcuxClMath_ExactDivide.c
 * @brief mcuxClMath: implementation of exact division
 */


#include <mcuxClCore_Platform.h>
#include <mcuxClCore_FunctionIdentifiers.h>
#include <mcuxCsslFlowProtection.h>

#include <mcuxClPkc.h>
#include <mcuxClMath_Functions.h>
#include <mcuxClMath_Types.h>

#include <internal/mcuxClPkc_Operations.h>
#include <internal/mcuxClPkc_Macros.h>


/**
 * [DESIGN]
 *
 * Since mcuxClMath_ExactDivideOdd only supports odd divisor, this function trims
 * trailing zero bits of Y and gets odd Y' = Y >> trailingZeros(Y). It also trims
 * X' = X >> ((trailingZeros(Y) / 8*MCUXCLPKC_WORDSIZE) * 8*MCUXCLPKC_WORDSIZE),
 * by shifting the address of X in UPTR table. It relies on mcuxClMath_ExactDivideOdd
 * to calculate R' = X'/Y', and then calculates the result,
 * R = R' >> (trailingZeros(Y) % (8*MCUXCLPKC_WORDSIZE)).
 *
 * Since mcuxClMath_ExactDivideOdd assumes there is no leading zero PKC word of Y',
 * this function counts the number of leading zeros of Y' and updates the length
 * of Y' if the number of leading zeros of Y' exceeds a PKC word.
 */
MCUX_CSSL_ANALYSIS_START_SUPPRESS_DECLARED_BUT_NEVER_DEFINED("It is indeed defined.")
MCUX_CSSL_ANALYSIS_START_PATTERN_SYMBOL_DEFINED_MORE_THAN_ONCE()
MCUX_CSSL_FP_FUNCTION_DEF(mcuxClMath_ExactDivide)
MCUX_CSSL_FP_PROTECTED_TYPE(void) mcuxClMath_ExactDivide(uint32_t iR_iX_iY_iT, uint32_t xPkcByteLength, uint32_t yPkcByteLength)
MCUX_CSSL_ANALYSIS_STOP_PATTERN_SYMBOL_DEFINED_MORE_THAN_ONCE()
MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_DECLARED_BUT_NEVER_DEFINED()
{
    MCUX_CSSL_FP_FUNCTION_ENTRY(mcuxClMath_ExactDivide);

    /* ASSERT: length of X >= length of Y. */
    MCUX_CSSL_ANALYSIS_ASSERT_PARAMETER_FP_VOID(xPkcByteLength, yPkcByteLength, MCUXCLPKC_RAM_SIZE)

    /* Backup PS1 length to restore in the end. */
    uint32_t backupPs1LenReg = MCUXCLPKC_PS1_GETLENGTH_REG();

    /***************************************************************************************/
    /* Step 1: Count trailing zeros of Y and make Y odd to be used in math_ExactDivideOdd  */
    /***************************************************************************************/

    MCUXCLPKC_WAITFORFINISH();
    MCUXCLPKC_PS1_SETLENGTH(0u, yPkcByteLength);  /* MCLEN on higher 16 bits is not used. */

    uint32_t uptrtIndexY = (iR_iX_iY_iT >> 8) & 0xFFu;

    MCUX_CSSL_FP_FUNCTION_CALL(noOfTrailingZeroBits, mcuxClMath_TrailingZeros((uint8_t)uptrtIndexY));
    uint32_t noOfTrailingZeroPkcWords = noOfTrailingZeroBits / (8u * MCUXCLPKC_WORDSIZE);

    /* If number of trailing zero bits exceeds a PKC word, shift pointer in UPTR table. */
    uint32_t noOfShiftBytes = noOfTrailingZeroPkcWords * MCUXCLPKC_WORDSIZE;

    uint16_t *pOperands = MCUXCLPKC_GETUPTRT();
    uint32_t uptrtIndexX = (iR_iX_iY_iT >> 16) & 0xFFu;

    MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("X > Y > 0, trailing zeros of Y will not exceed bit lengths of X and Y. \
            Offsets of X and Y after trimming trailing zero PKC word(s) will be still in the valid range.")
    const uint32_t offsetYTrimmed = (uint32_t) pOperands[uptrtIndexY] + noOfShiftBytes;
    const uint32_t offsetXTrimmed = (uint32_t) pOperands[uptrtIndexX] + noOfShiftBytes;
    MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()

    pOperands[uptrtIndexY] = (uint16_t) (offsetYTrimmed & 0xFFFFu);
    pOperands[uptrtIndexX] = (uint16_t) (offsetXTrimmed & 0xFFFFu);

    /* Shift number of bits, which are less than one PKC word. */
    uint32_t noOfShiftBits = noOfTrailingZeroBits % (8u * MCUXCLPKC_WORDSIZE);

    MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("Y > 0, length of Y after trimming trailing zeros is > 0.")
    uint32_t trimYPkcByteLen = yPkcByteLength - noOfShiftBytes;
    MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()

    MCUXCLPKC_PS1_SETLENGTH(0u, trimYPkcByteLen);  /* MCLEN on higher 16 bits is not used. */
    MCUXCLPKC_FP_CALC_OP1_SHR(uptrtIndexY, uptrtIndexY, noOfShiftBits);

    /* If number of leading zero bits after shift exceeds a PKC word, reduce length of Y. */
    MCUXCLPKC_WAITFORFINISH();
    MCUX_CSSL_FP_FUNCTION_CALL(leadingZeroBits, mcuxClMath_LeadingZeros((uint8_t)uptrtIndexY));

    if((8u * MCUXCLPKC_WORDSIZE) <= leadingZeroBits)
    {
        MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("Y > 0, length of Y after trimming leading and trailing zeros is > 0.")
        trimYPkcByteLen -= MCUXCLPKC_WORDSIZE;
        MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()
    }

    /***************************************************************************************/
    /* Step 2: Call math_ExactDivideOdd with odd Y                                         */
    /***************************************************************************************/

    uint32_t uptrtIndexT = iR_iX_iY_iT & 0xFFu;
    uint32_t uptrtIndexR = (iR_iX_iY_iT >> 24) & 0xFFu;

    MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("X > Y > 0, length of X after trimming (some) trailing zeros is > 0.")
    const uint32_t trimXPkcByteLen = xPkcByteLength - noOfShiftBytes;
    MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()
    MCUXCLMATH_FP_EXACTDIVIDEODD(uptrtIndexR,
                                uptrtIndexX,
                                uptrtIndexY,
                                uptrtIndexT,
                                trimXPkcByteLen,
                                trimYPkcByteLen);

    /***************************************************************************************/
    /* Step 3: Recover y and shift result to account for trailing zero bits of y           */
    /***************************************************************************************/

    MCUXCLPKC_FP_CALC_OP1_SHL(uptrtIndexY, uptrtIndexY, noOfShiftBits);

    /* If number of leading zero bits of Y' after shift does not exceed a PKC word,    */
    /* result R' is of the size (xPkcByteLength - yPkcByteLength + MCUXCLPKC_WORDSIZE). */
    /* An extra PKC word needs to be cleared before right shifting result R' with      */
    /* OPLEN = xPkcByteLength - yPkcByteLength + (2u * MCUXCLPKC_WORDSIZE).             */
    if((8u * MCUXCLPKC_WORDSIZE) > leadingZeroBits)
    {
        MCUXCLPKC_PS2_SETLENGTH_REG(MCUXCLPKC_WORDSIZE);  /* MCLEN on higher 16 bits is not used. */
        MCUXCLPKC_PS2_SETMODE(MCUXCLPKC_OP_CONST);
        MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("Length of X >= length of Y, and caller shall reserve enough space for R.")
        const uint32_t offsetExtraPkcWord = (uint32_t) pOperands[uptrtIndexR] + xPkcByteLength - yPkcByteLength + MCUXCLPKC_WORDSIZE;
        MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()
        MCUXCLPKC_PS2_SETZR(0u, offsetExtraPkcWord);
        MCUXCLPKC_WAITFORREADY();
        MCUXCLPKC_PS2_START_L0();
        MCUXCLPKC_WAITFORREADY();
    }

    MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("Length of X >= length of Y, and caller shall reserve enough space for R.")
    const uint32_t pkcLengthToShift = xPkcByteLength - yPkcByteLength + (2u * MCUXCLPKC_WORDSIZE);
    MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()
    MCUXCLPKC_PS2_SETLENGTH(0u, pkcLengthToShift);  /* MCLEN on higher 16 bits is not used. */
    MCUXCLPKC_FP_CALC_OP2_SHR(uptrtIndexR, uptrtIndexR, noOfShiftBits);

    /* Restore UPTR table and PKC settings. */
    MCUX_CSSL_ANALYSIS_START_SUPPRESS_INTEGER_WRAP("Restore offsets of X and Y.")
    const uint32_t offsetY = (uint32_t) pOperands[uptrtIndexY] - noOfShiftBytes;
    const uint32_t offsetX = (uint32_t) pOperands[uptrtIndexX] - noOfShiftBytes;
    MCUX_CSSL_ANALYSIS_STOP_SUPPRESS_INTEGER_WRAP()

    pOperands[uptrtIndexY] = (uint16_t) (offsetY & 0xFFFFu);
    pOperands[uptrtIndexX] = (uint16_t) (offsetX & 0xFFFFu);

    MCUXCLPKC_WAITFORREADY();
    MCUXCLPKC_PS1_SETLENGTH_REG(backupPs1LenReg);

    MCUX_CSSL_FP_FUNCTION_EXIT_VOID(mcuxClMath_ExactDivide,
            MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClMath_TrailingZeros),
            MCUXCLPKC_FP_CALLED_CALC_OP1_SHR,
            MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClMath_LeadingZeros),
            MCUX_CSSL_FP_FUNCTION_CALLED(mcuxClMath_ExactDivideOdd),
            MCUXCLPKC_FP_CALLED_CALC_OP1_SHL,
            MCUXCLPKC_FP_CALLED_CALC_OP1_SHR);
}
