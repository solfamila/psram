/*--------------------------------------------------------------------------*/
/* Copyright 2020-2021, 2023 NXP                                            */
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
 * @file  mcuxClEcc_Weier_Internal_PointCheck_FUP.c
 * @brief FUP program for Weierstrass curve internal point check
 */

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClEcc_Weier_Internal_PkcWaLayout.h>
#include <internal/mcuxClEcc_Weier_Internal_FUP.h>


/* FUP program: calculate (- y^2 + x^3 + a*x + b) mod p.               */
/* Input: (x,y) in (VX0,VY0), affine, normal representation.           */
/* Output: (- y^2 + x^3 + a*x + b) mod p in T0, in the range [0, p-1]. */
MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_Weier_PointCheckAffNR[10] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x53u,0xa2u,0xcfu,0x73u},{0x80u,0x00u,0x04u,0x16u,0x00u,0x19u},{0x80u,0x00u,0x19u,0x19u,0x00u,0x1bu},{0x80u,0x21u,0x10u,0x1bu,0x12u,0x1bu},{0x80u,0x00u,0x19u,0x1bu,0x00u,0x1du},{0x80u,0x00u,0x05u,0x16u,0x00u,0x19u},{0x80u,0x00u,0x19u,0x19u,0x00u,0x1bu},{0x80u,0x2au,0x10u,0x1bu,0x1du,0x1bu},{0x80u,0x33u,0x1bu,0x1bu,0x00u,0x19u},{0x80u,0x2au,0x00u,0x13u,0x19u,0x19u}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

