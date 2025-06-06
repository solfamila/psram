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
 * @file  mcuxClEcc_WeierECC_Internal_BlindedSecretKeyGen_FUP.c
 * @brief FUP programs for Weierstrass curve internal key generation
 */

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClEcc_Weier_Internal_PkcWaLayout.h>
#include <internal/mcuxClEcc_Weier_Internal_FUP.h>


MCUX_CSSL_ANALYSIS_START_PATTERN_DESCRIPTIVE_IDENTIFIER()
/* Prerequisites: VT = number of bit(s) to be truncated;                      */
MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_WeierECC_BlindedSecretKeyGen_RandomWithExtraBits_Steps34[6] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0xbfu,0xe7u,0x45u,0xb0u},{0x40u,0x15u,0x1cu,0x1cu,0x0fu,0x1cu},{0x40u,0x15u,0x1eu,0x1eu,0x0fu,0x1eu},{0x40u,0x0au,0x1cu,0x1cu,0x1eu,0x1cu},{0x00u,0x1eu,0x18u,0x18u,0x03u,0x22u},{0x00u,0x1bu,0x01u,0x01u,0x02u,0x23u}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()



MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_WeierECC_BlindedSecretKeyGen_RandomWithExtraBits_Steps56[8] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0xe7u,0x43u,0xb0u,0xaau},{0x80u,0x13u,0x19u,0x1cu,0x19u,0x1eu},{0x80u,0x13u,0x19u,0x1au,0x19u,0x1cu},{0x80u,0x13u,0x19u,0x23u,0x19u,0x1au},{0xc0u,0x2au,0x1au,0x1cu,0x1eu,0x1cu},{0x40u,0x3eu,0x1au,0x1au,0x03u,0x1au},{0x00u,0x1eu,0x19u,0x19u,0x03u,0x1au},{0x40u,0x0au,0x1cu,0x1cu,0x1au,0x1cu}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()



MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClEcc_FUP_WeierECC_BlindedSecretKeyGen_RandomWithExtraBits_Step7[5] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x67u,0x49u,0x3cu,0x54u},{0xc0u,0x00u,0x1cu,0x17u,0x01u,0x1au},{0x80u,0x00u,0x1au,0x17u,0x01u,0x19u},{0x80u,0x33u,0x19u,0x19u,0x01u,0x1au},{0x80u,0x2au,0x01u,0x1au,0x01u,0x1au}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

MCUX_CSSL_ANALYSIS_STOP_PATTERN_DESCRIPTIVE_IDENTIFIER()
