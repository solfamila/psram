/*--------------------------------------------------------------------------*/
/* Copyright 2021, 2023 NXP                                                 */
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
 * @file  mcuxClMath_ExactDivideOdd_FUP.h
 * @brief defines FUP program byte arrays for mcuxClMath_ExactDivideOdd
 */

#ifndef MCUXCLMATH_EXACTDIVIDEODD_FUP_H_
#define MCUXCLMATH_EXACTDIVIDEODD_FUP_H_

#include <mcuxClCore_Platform.h>
#include <mcuxCsslAnalysis.h>
#include <internal/mcuxClPkc_FupMacros.h>

#ifdef __cplusplus
extern "C" {
#endif

MCUX_CSSL_ANALYSIS_START_PATTERN_EXTERNAL_LINKAGE_FUP()
/*
 * FUP program declaration mcuxClMath_Fup_ExactDivideOdd_DivideLoop
 */
#define mcuxClMath_Fup_ExactDivideOdd_DivideLoop_LEN  5u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_Fup_ExactDivideOdd_DivideLoop[mcuxClMath_Fup_ExactDivideOdd_DivideLoop_LEN];

/*
 * FUP program declaration mcuxClMath_Fup_ExactDivideOdd_NDashY
 */
#define mcuxClMath_Fup_ExactDivideOdd_NDashY_LEN  4u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_Fup_ExactDivideOdd_NDashY[mcuxClMath_Fup_ExactDivideOdd_NDashY_LEN];
MCUX_CSSL_ANALYSIS_STOP_PATTERN_EXTERNAL_LINKAGE_FUP()

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCUXCLMATH_EXACTDIVIDEODD_FUP_H_ */
