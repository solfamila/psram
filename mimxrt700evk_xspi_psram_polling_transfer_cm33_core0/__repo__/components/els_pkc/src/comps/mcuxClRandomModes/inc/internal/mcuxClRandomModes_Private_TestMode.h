/*--------------------------------------------------------------------------*/
/* Copyright 2021-2023 NXP                                                  */
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

#ifndef MCUXCLRANDOM_PRIVATE_TESTMODE_H_
#define MCUXCLRANDOM_PRIVATE_TESTMODE_H_

#include <mcuxClConfig.h> // Exported features flags header

#include <mcuxClSession.h>
#include <mcuxClRandom_Types.h>
#include <internal/mcuxClRandom_Internal_Types.h>
#include <internal/mcuxClRandomModes_Private_NormalMode.h>


#ifdef __cplusplus
extern "C" {
#endif


/* Internal function prototypes */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRandomModes_TestMode_initFunction, mcuxClRandom_initFunction_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRandom_Status_t) mcuxClRandomModes_TestMode_initFunction(
        mcuxClSession_Handle_t pSession,
        mcuxClRandom_Mode_t mode,
        mcuxClRandom_Context_t context);

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRandomModes_TestMode_reseedFunction, mcuxClRandom_reseedFunction_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRandom_Status_t) mcuxClRandomModes_TestMode_reseedFunction(
        mcuxClSession_Handle_t pSession,
        mcuxClRandom_Mode_t mode,
        mcuxClRandom_Context_t context);

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRandomModes_TestMode_selftestFunction, mcuxClRandom_selftestFunction_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRandom_Status_t) mcuxClRandomModes_TestMode_selftestFunction(
        mcuxClSession_Handle_t pSession,
        mcuxClRandom_Mode_t mode);


#ifdef MCUXCL_FEATURE_RANDOMMODES_PR_DISABLED
extern const mcuxClRandom_OperationModeDescriptor_t mcuxClRandomModes_OperationModeDescriptor_TestMode_PrDisabled;
#endif /* MCUXCL_FEATURE_RANDOMMODES_PR_DISABLED */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCUXCLRANDOM_PRIVATE_TESTMODE_H_ */
