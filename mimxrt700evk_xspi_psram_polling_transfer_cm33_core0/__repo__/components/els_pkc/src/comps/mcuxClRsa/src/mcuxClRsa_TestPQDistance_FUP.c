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

#include <mcuxCsslAnalysis.h>

#include <internal/mcuxClRsa_Internal_PkcDefs.h>
#include <internal/mcuxClPkc_FupMacros.h>
#include <internal/mcuxClRsa_TestPQDistance_FUP.h>

/* FUP to check the distance between P and Q */
MCUX_CSSL_ANALYSIS_START_PATTERN_FUP_PROGRAM()
const mcuxClPkc_FUPEntry_t mcuxClRsa_TestPQDistance_FUP[4] MCUX_FUP_ATTRIBUTE = {{0x10u,0x00u,0x27u,0x1eu,0x5fu,0x9eu},{0x00u,0x15u,0x00u,0x00u,0x04u,0x02u},{0x00u,0x15u,0x01u,0x01u,0x04u,0x03u},{0x00u,0x4bu,0x02u,0x02u,0x03u,0x02u}};
MCUX_CSSL_ANALYSIS_STOP_PATTERN_FUP_PROGRAM()

