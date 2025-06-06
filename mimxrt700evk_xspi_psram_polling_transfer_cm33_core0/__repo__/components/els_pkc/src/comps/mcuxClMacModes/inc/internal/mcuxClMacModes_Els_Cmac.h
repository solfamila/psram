/*--------------------------------------------------------------------------*/
/* Copyright 2020-2022, 2024 NXP                                            */
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

#ifndef MCUXCLMACMODES_ELS_CMAC_H_
#define MCUXCLMACMODES_ELS_CMAC_H_

#include <mcuxClConfig.h> // Exported features flags header
#include <mcuxClCore_Platform.h>
#include <mcuxClBuffer.h>
#include <mcuxClKey_Types.h>
#include <mcuxClSession_Types.h>

#include <mcuxClMac_Types.h>
#include <internal/mcuxClMacModes_Els_Ctx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal function which processes complete CMAC blocks.
 *
 * @Pre: @p pContext->cmac_options.bits must be configured to match the desired CMAC step
 *
 * @param[in]  pContext       CMAC context
 * @param[in]  pIn            CMAC input padded to full blocks if necessary
 * @param[in]  inputLen       Input length, length before padding if padding is applied
 * @param[in/out]  pOut       CMAC output
 *
 * @return mcuxClMac_Status_t  Status of the operation
 * @retval MCUXCLMAC_STATUS_FAILURE Operation failed
 * @retval MCUXCLMAC_STATUS_OK      Operation succeeded
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClMacModes_Engine_CMAC_ProcessBlocks)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClMac_Status_t) mcuxClMacModes_Engine_CMAC_ProcessBlocks(
    mcuxClMacModes_Context_t * const pContext, /*! CMAC context */
    const uint8_t *const pIn,                 /*! CMAC input */
    uint32_t inputLen,                        /*! Input size must be full blocks */
    uint8_t *const pOut);                     /*! CMAC output */

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClMacModes_Engine_CMAC_Oneshot)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClMac_Status_t) mcuxClMacModes_Engine_CMAC_Oneshot(
  mcuxClSession_Handle_t session,           /*! CMAC session handle */
  mcuxClMacModes_Context_t * const pContext,/*! CMAC context */
  const uint8_t *const pIn,                /*! CMAC input */
  uint32_t inLength,                       /*! Input size */
  uint8_t *const pOut,                     /*! CMAC output */
  uint32_t *const pOutLength               /*! Output size */
);

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClMacModes_Engine_CMAC_Init)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClMac_Status_t) mcuxClMacModes_Engine_CMAC_Init(
  mcuxClSession_Handle_t session,           /*! CMAC session handle */
  mcuxClMacModes_Context_t * const pContext /*! CMAC context */
);

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClMacModes_Engine_CMAC_Update)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClMac_Status_t) mcuxClMacModes_Engine_CMAC_Update(
  mcuxClSession_Handle_t session,           /*! CMAC session handle */
  mcuxClMacModes_Context_t * const pContext,/*! CMAC context */
  mcuxCl_InputBuffer_t pIn,                 /*! CMAC input */
  uint32_t inLength                        /*! Input size */
);

MCUX_CSSL_FP_FUNCTION_DECL(mcuxClMacModes_Engine_CMAC_Finalize)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClMac_Status_t) mcuxClMacModes_Engine_CMAC_Finalize(
  mcuxClSession_Handle_t session,           /*! CMAC session handle */
  mcuxClMacModes_Context_t * const pContext,/*! CMAC context */
  mcuxCl_Buffer_t pOut,                     /*! CMAC output */
  uint32_t *const pOutLength               /*! Output size */
);



#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCUXCLMACMODES_ELS_CMAC_H_ */
