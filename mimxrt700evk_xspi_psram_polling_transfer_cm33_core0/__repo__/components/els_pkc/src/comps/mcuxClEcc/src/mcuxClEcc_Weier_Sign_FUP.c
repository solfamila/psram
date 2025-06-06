/*--------------------------------------------------------------------------*/
/* Copyright 2020-2023 NXP                                                  */
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
 * @file  mcuxClEcc_Weier_Sign_FUP.c
 * @brief FUP program for Weierstrass curve ECDSA signature generation
 */

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClEcc_Weier_Internal_PkcWaLayout.h>
#include <internal/mcuxClEcc_Weier_Internal_FUP.h>

MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_Weier_Sign_CalculateS[13] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x5au,0x69u,0x3fu,0x97u},{0x80u,0x00u,0x20u,0x17u,0x01u,0x21u},{0x80u,0x21u,0x01u,0x22u,0x23u,0x22u},{0x80u,0x00u,0x21u,0x22u,0x01u,0x1du},{0x80u,0x21u,0x11u,0x1cu,0x1du,0x1du},{0x80u,0x00u,0x1au,0x1du,0x01u,0x19u},{0x80u,0x00u,0x21u,0x23u,0x01u,0x1fu},{0x80u,0x00u,0x1au,0x1fu,0x01u,0x1bu},{0x80u,0x2au,0x11u,0x19u,0x1bu,0x19u},{0x80u,0x33u,0x19u,0x19u,0x01u,0x1bu},{0x80u,0x33u,0x18u,0x18u,0x01u,0x19u},{0x80u,0x00u,0x1au,0x1au,0x01u,0x1fu},{0x80u,0x00u,0x19u,0x1fu,0x01u,0x1du}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

