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
 * @file  mcuxClMath_SecModExp_FUP.h
 * @brief defines FUP program byte arrays for mcuxClMath_SecModExp
 */

#ifndef MCUXCLMATH_SECMODEXP_FUP_H_
#define MCUXCLMATH_SECMODEXP_FUP_H_

#include <mcuxClCore_Platform.h>
#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>

#ifdef __cplusplus
extern "C" {
#endif

MCUX_CSSL_ANALYSIS_START_PATTERN_EXTERNAL_LINKAGE_FUP()
/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_Init
 */
#define mcuxClMath_SecModExp_Fup_Init_LEN  3u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_Init[mcuxClMath_SecModExp_Fup_Init_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_Rerandomize
 */
#define mcuxClMath_SecModExp_Fup_Rerandomize_LEN  5u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_Rerandomize[mcuxClMath_SecModExp_Fup_Rerandomize_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_EuclideanSplit_1
 */
#define mcuxClMath_SecModExp_Fup_EuclideanSplit_1_LEN  8u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_EuclideanSplit_1[mcuxClMath_SecModExp_Fup_EuclideanSplit_1_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_EuclideanSplit_2
 */
#define mcuxClMath_SecModExp_Fup_EuclideanSplit_2_LEN  7u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_EuclideanSplit_2[mcuxClMath_SecModExp_Fup_EuclideanSplit_2_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_ExactDivideLoop
 */
#define mcuxClMath_SecModExp_Fup_ExactDivideLoop_LEN  9u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_ExactDivideLoop[mcuxClMath_SecModExp_Fup_ExactDivideLoop_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_CalcQAndInterleave
 */
#define mcuxClMath_SecModExp_Fup_CalcQAndInterleave_LEN  6u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_CalcQAndInterleave[mcuxClMath_SecModExp_Fup_CalcQAndInterleave_LEN];

/*
 * FUP program declaration mcuxClMath_SecModExp_Fup_PrepareFirstExp
 */
#define mcuxClMath_SecModExp_Fup_PrepareFirstExp_LEN  5u

extern const mcuxClPkc_FUPEntry_t mcuxClMath_SecModExp_Fup_PrepareFirstExp[mcuxClMath_SecModExp_Fup_PrepareFirstExp_LEN];
MCUX_CSSL_ANALYSIS_STOP_PATTERN_EXTERNAL_LINKAGE_FUP()

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCUXCLMATH_SECMODEXP_FUP_H_ */
