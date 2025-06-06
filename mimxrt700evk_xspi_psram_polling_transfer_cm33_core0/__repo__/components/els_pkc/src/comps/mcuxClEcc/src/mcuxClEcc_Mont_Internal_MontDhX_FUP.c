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
 * @file  mcuxClEcc_Mont_Internal_MontDhX_FUP.c
 * @brief FUP programs for MontDhX functions
 */

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClEcc_Mont_Internal_PkcWaLayout.h>
#include <internal/mcuxClEcc_Mont_Internal_FUP.h>


/* The algorithm implemented by this FUP program to decode an encoded Curve25519 or Curve448 private key
 * only works if:
 *    c < 8*pkcWordSize
 *    0 < 8*operandSize - t + c < 8*pkcWordSize-1
 *
 *  For the use cases Curve25519 and Curve448 these conditions are met. */
MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_MontDhDecodeScalar[5] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x72u,0xa5u,0xf5u,0xefu},{0x00u,0x15u,0x1cu,0x1cu,0x04u,0x1eu},{0x00u,0x14u,0x1eu,0x1eu,0x05u,0x1eu},{0x00u,0x1au,0x1eu,0x1eu,0x02u,0x1eu},{0x00u,0x17u,0x1eu,0x1eu,0x06u,0x1eu}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()



MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_MontDhX_CalcAffineX[5] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x94u,0x31u,0x49u,0x04u},{0x80u,0x00u,0x1bu,0x16u,0x00u,0x19u},{0x80u,0x00u,0x20u,0x19u,0x00u,0x1bu},{0x80u,0x33u,0x1bu,0x1bu,0x00u,0x20u},{0x80u,0x2au,0x00u,0x20u,0x00u,0x20u}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

