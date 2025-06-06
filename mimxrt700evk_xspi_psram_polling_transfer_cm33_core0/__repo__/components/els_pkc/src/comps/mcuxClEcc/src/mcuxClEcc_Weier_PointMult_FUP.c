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
 * @file  mcuxClEcc_Weier_PointMult_FUP.c
 * @brief FUP program for Weierstrass curve point multiplication
 */

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClEcc_Weier_Internal_PkcWaLayout.h>
#include <internal/mcuxClEcc_Weier_Internal_FUP.h>


MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_Weier_PointMult_SplitScalar_ConvertPoint2MR[10] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0xc0u,0xf6u,0x49u,0x3au},{0x80u,0x00u,0x19u,0x17u,0x01u,0x1au},{0xc0u,0x00u,0x1cu,0x1au,0x01u,0x19u},{0xc0u,0x00u,0x1eu,0x1au,0x01u,0x1bu},{0x80u,0x00u,0x26u,0x16u,0x00u,0x24u},{0x80u,0x00u,0x27u,0x16u,0x00u,0x25u},{0x00u,0x09u,0x00u,0x00u,0x00u,0x23u},{0x80u,0x2au,0x11u,0x1bu,0x19u,0x1bu},{0x80u,0x33u,0x1bu,0x1bu,0x01u,0x19u},{0x80u,0x2au,0x01u,0x19u,0x01u,0x1au}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

