/*--------------------------------------------------------------------------*/
/* Copyright 2020-2024 NXP                                                  */
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

/** @file  mcuxClRsa_Internal_Functions.h
 *  @brief Internal functions of the mcuxClRsa component
 */

#ifndef MCUXCLRSA_INTERNAL_FUNCTIONS_H_
#define MCUXCLRSA_INTERNAL_FUNCTIONS_H_

#include <mcuxClConfig.h> // Exported features flags header
#include <mcuxClSession.h>
#include <mcuxCsslFlowProtection.h>
#include <mcuxClCore_FunctionIdentifiers.h>
#include <mcuxClHash.h>
#include <mcuxClRsa_Types.h>
#include <mcuxClKey.h>

#if defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) || defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT)
#include <mcuxClCipher.h>
#endif /* defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) || defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT) */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mcuxClRsa_Internal_Functions mcuxClRsa_Internal_Functions
 * @brief Defines all internal functions of @ref mcuxClRsa
 * @ingroup mcuxClRsa
 * @{
 */

/**
 * \brief RSA public operation
 *
 * This function performs an RSA public key operation according to PKCS #1 v2.2.
 * The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8.
 * The public exponent is limited to 2 <= e < N.
 *
 * \param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * \param[in]  pKey                 Pointer to key structure of type @ref mcuxClRsa_Key
 * \param[in]  pInput               Buffer which contains the input
 * \param[out] pOutput              Pointer to result
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pKey:</dt>
 *          <dd>The key entries meet the following conditions:
 *              - Entry keytype must be set to #MCUXCLRSA_KEY_PUBLIC. In case of passing another key type, the function
 *                returns #MCUXCLRSA_STATUS_INVALID_INPUT. The functions checks, internally, whether the required key entries
 *                are not set to NULL. If so, the function returns #MCUXCLRSA_STATUS_INVALID_INPUT;
 *              - The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8;
 *              - It is required that e is greater or equal to 2 and smaller than n.
 *      <dt>pInput:</dt>
 *          <dd>The input must meet the following conditions:
 *               - It is provided in big-endian byte order;
 *               - The input length is determined by the modulus length.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - It is located in PKC RAM;
 *               - A buffer of MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(modulus length) + 2u * MCUXCLRSA_PKC_WORDSIZE bytes has to be allocated;
 *               - The result is stored in little-endian byte order in the buffer pointed to by pOutput with modulus length.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_public operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_KEYOP_OK    The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The input parameters are not valid.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_public, mcuxClRsa_PublicExpEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_public(
  mcuxClSession_Handle_t      pSession,
  const mcuxClRsa_Key * const pKey,
  mcuxCl_InputBuffer_t        pInput,
  uint8_t                   *pOutput
);

/**
 * \brief RSA Public key operation with blinding applied to the modulus
 *
 * \param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * \param[in]  iR_iX_iN_iT1         PKC operand index
 * \param[in]  iT2_iT3_iT4          PKC operand index
 * \param[in]  byteLenExp           length of the exponent
 * \param[in]  pExp                 Pointer to the exponent
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>iR_iX_iN_iT1:</dt>
 *        <dd><code>iR</code> (bits 24~31): index of result (PKC operand).
 *        <br>Its size shall be at least operandSize + 2u * MCUXCLRSA_PKC_WORDSIZE
 *        <dd><code>iN</code> (bits 16~23): index of modulus, will be destroy (PKC operand).
 *        <br>Its size shall be at least operandSize + MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4), reserve one MCUXCLRSA_PKC_WORDSIZE before it
 *        <dd><code>iX</code> (bits 8~15): index of base number in Normal representation (PKC operand).
 *        <br>Its size shall be at least operandSize + MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4)
 *        <dd><code>iT1</code> (bits 0~7):index of temp1 (PKC operand).
 *        <br>Its size shall be at least operandSize + MCUXCLRSA_PKC_WORDSIZE + MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4)
 *      <dt>iT2_iT3_iT4:</dt>
 *        <dd><code>iT2</code> (bits 16~23):index of temp2 (PKC operand).
 *        <br>Its size shall be at least operandSize + MCUXCLRSA_PKC_WORDSIZE + MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4)
 *        <dd><code>iT3</code> (bits 8~15): index of temp3(PKC operand).
 *        <br>Its size shall be at least operandSize + MCUXCLRSA_PKC_WORDSIZE + MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4)
 *        <dd><code>iT4</code> (bits 0~7): index of temp4 (PKC operand).
 *        <br>Its size shall be at least MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(blindingSize=4)
 *      <dt>byteLenExp:</dt>
 *        <dd>the length of exponent in byte.</dd>
 *      <dt>pExp:</dt>
 *        <dd>the exponent is a big-endian octet string and shall be non-zero.</dd>
 *  </dl></dd>
 * </dl>
 *
 * <dl>
 *   <dt>PKC properties</dt>
 *   <dd><dl>
 *     <dt>PS1 lengths</dt>
 *       <dd>PS1 OPLEN defines operandSize = nPkcByteLength (length of modulus N).</dd>
 *       <dd>PS1 OPLEN and MCLEN will be modified, and original values will be restored.</dd>
 *     <dt>PS2 lengths</dt>
 *       <dd>PS2 OPLEN and MCLEN will be modified, and original values will not be restored.</dd>
 *     <dt>ACTIV/GOANY</dt>
 *       <dd>#mcuxClPkc_WaitForFinish will be called before returning to caller.</dd>
 *   </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_publicExp operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_KEYOP_OK    The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_publicExp)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_publicExp(
  mcuxClSession_Handle_t      pSession,
  uint32_t iR_iX_iN_iT1,
  uint32_t iT2_iT3_iT4,
  uint32_t byteLenExp,
  const uint8_t* const pExp
);


/**
 * \brief RSA private plain operation
 *
 * This function performs an RSA private plain key operation according to PKCS #1 v2.2.
 * The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8.
 * The private exponent is limited to d < N.
 *
 * \param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * \param[in]  pKey                 Pointer to key structure of type @ref mcuxClRsa_Key
 * \param[in]  pInput               Pointer to input
 * \param[out] pOutput              Buffer to hold the result
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pKey:</dt>
 *          <dd>The key entries meet the following conditions:
 *              - Entry keytype must be set to #MCUXCLRSA_KEY_PRIVATEPLAIN. In case of passing another key type, the function
 *                returns #MCUXCLRSA_STATUS_INVALID_INPUT. The functions checks, internally, whether the required key entries
 *                are not set to NULL. If so, the function returns #MCUXCLRSA_STATUS_INVALID_INPUT;
 *              - The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8;
 *              - It is required that d is smaller than n.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is located in PKC RAM;
 *               - It is provided in little-endian byte order;
 *               - The input buffer length should be:
 *                 MCUXCLRSA_INTERNAL_PRIVATEPLAIN_INPUT_SIZE(modulus length) = MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(modulus length) + 2*MCUXCLRSA_PKC_WORDSIZE.
 *                 Inside this buffer, the input has the same byte length as the modulus, while upper bytes are used as temporary buffer for internal operations.
 *               - It is overwritten by the function.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated;
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_privatePlain operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_KEYOP_OK    The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The input parameters are not valid.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_privatePlain)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_privatePlain(
  mcuxClSession_Handle_t      pSession,
  const mcuxClRsa_Key * const pKey,
  uint8_t                   *pInput,
  mcuxCl_Buffer_t             pOutput
);


/**
 * \brief RSA private CRT operation
 *
 * This function performs an RSA private CRT key operation according to PKCS #1 v2.2.
 * The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8.
 * The length of the primes is limited to: size(p) = size(q) = 1/2 size(n).
 * The private exponent is limited to d < N.
 *
 * \param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * \param[in]  pKey                 Pointer to key structure of type @ref mcuxClRsa_Key
 * \param[in]  pInput               Pointer to input
 * \param[out] pOutput              Buffer to hold the result
 *
 ** <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pKey:</dt>
 *          <dd>The key entries meet the following conditions:
 *              - Entry keytype must be set to #MCUXCLRSA_KEY_PRIVATECRT or #MCUXCLRSA_KEY_PRIVATECRT_DFA. In case of passing
 *                another key type, the function returns #MCUXCLRSA_STATUS_INVALID_INPUT. The functions checks, internally,
 *                whether the required key entries are not set to NULL. If so, the function returns #MCUXCLRSA_STATUS_INVALID_INPUT;
 *              - The supported bit-lengths of the modulus range from 512 to 8192 in multiples of 8;
 *              - The length of the primes is limited to: size(p) = size(q) = 1/2 size(n);
 *              - It is required that d is smaller than n.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is located in PKC RAM;
 *               - It is provided in little-endian byte order;
 *               - The input length is determined by the modulus length.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated;
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_privateCRT operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_KEYOP_OK    The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The input parameters are not valid.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_privateCRT)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_privateCRT(
  mcuxClSession_Handle_t      pSession,
  const mcuxClRsa_Key * const pKey,
  uint8_t                   *pInput,
  mcuxCl_Buffer_t             pOutput
);

/**
 * @brief Function calculation modulo from the CRT key
 *
 * \param[in]      pKey           Pointer to key structure of type @ref mcuxClRsa_Key
 * \param[in/out]  pKeyBitLength  Pointer to return key bit lenght
 *
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_Calc_Modlen_From_CRTkey)
MCUX_CSSL_FP_PROTECTED_TYPE(void) mcuxClRsa_Calc_Modlen_From_CRTkey(
                        const mcuxClRsa_Key * const pKey,
                        uint32_t *pKeyBitLength);



/**
 * @brief No encoding message operation for signature
 *
 * This function is used to implement the RSASP1 primitive of PKCS #1 v2.2, i.e. signature
 * generation, without encoding message operation.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Buffer with input
 * @param[in]  inputLength          RFU: please set to zero
 * @param[in]  pVerificationInput   RFU: please set to NULL
 * @param[in]  pHashAlgo            RFU: please set to NULL
 * @param[in]  pLabel               RFU: please set to NULL
 * @param[in]  saltlabelLength      RFU: please set to zero
 * @param[in]  keyBitLength         Bit-length of modulus. Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              RFU: please set to zero
 * @param[out] pOutput              Buffer to hold the result
 * @param[out] pOutLength           RFU: please set to NULL
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order;
 *               - The input length is determined by the BYTE_LENGTH(keyBitLength).
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated in PKC RAM;
 *               - The result is stored in little-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_Sign_NoEMSA operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK    The function executed successfully.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_Sign_NoEMSA, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_Sign_NoEMSA(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);


/**
 * @brief No encoding message operation for verification
 *
 * This function is used to implement the RSAVP1 primitive of PKCS #1 v2.2, i.e. signature
 * a call to the public exponentiation, without encoding message operation.
 *
 * @param[in]  pSession                Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput                  RFU: please set to NULL
 * @param[in]  inputLength             RFU: please set to zero
 * @param[in]  pVerificationInput      Pointer to input
 * @param[in]  pHashAlgo               RFU: please set to NULL
 * @param[in]  pLabel                  RFU: please set to NULL
 * @param[in]  saltlabelLength         RFU: please set to zero
 * @param[in]  keyBitLength            Bit-length of modulus. Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options                 RFU: please set to zero
 * @param[out] pOutput                 Buffer to hold the result
 * @param[out] pOutLength              RFU: please set to NULL
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pVerificationInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is located in PKC RAM;
 *               - It is provided in little-endian byte order;
 *               - The input length is determined by the BYTE_LENGTH(keyBitLength).
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated;
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_Verify_NoEMSA operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_VERIFYPRIMITIVE_OK  The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_ERROR               An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_Verify_NoEMSA, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_Verify_NoEMSA(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);


/**
 * @brief RSA mask generation function
 *
 * This function is used to implement the mask generation function MGF1 of PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pInput               Pointer to seed, of which the mask is generated
 * @param[in]  inputLength          Length of seed, of which mask is generated
 * @param[in]  outputLength         Length of mask to be generated
 * @param[out] pOutput              Pointer to result
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is located in PKC RAM.
 *      <dt>inputLength:</dt>
 *          <dd>Byte-length of the seed, of which the mask is generated.
 *      <dt>outputLength:</dt>
 *          <dd>Byte-length of mask, which is generated.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of outputLength bytes has to be allocated.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_mgf1 operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_MGF_OK      The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_mgf1)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_mgf1(
  mcuxClSession_Handle_t       pSession,
  mcuxClHash_Algo_t            pHashAlgo,
  const uint8_t *             pInput,
  const uint32_t              inputLength,
  const uint32_t              outputLength,
  uint8_t *                   pOutput
);

/**
 * @brief RSA PSS Encoding operation
 *
 * This function performs an RSA PSS encoding operation according to EMSA-PSS-ENCODE of PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Input-Buffer, which contains the message or message digest to be encoded
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   RFU: please set to NULL
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               RFU: please set to NULL
 * @param[in]  saltlabelLength      Byte-length of salt
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message).  Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              Options field
 * @param[out] pOutput              Buffer to hold the result
 * @param[out] pOutLength           RFU: please set to NULL
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput in case of option MCUXCLRSA_OPTION_MESSAGE_PLAIN. In case of option MCUXCLRSA_OPTION_MESSAGE_DIGEST,
 *               this buffer size is specified by the targeted hash algorithm. Thus for MCUXCLRSA_OPTION_MESSAGE_DIGEST, please set inputLength to zero.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>saltlabelLength:</dt>
 *          <dd>Byte-length salt.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>options:</dt>
 *          <dd> This field is used to select options of the sign operation:
 *                - bits 31-8: RFU; please set to zero
 *                - bits 7-0:   Specify, whether pInput is provided as plain message (MCUXCLRSA_OPTION_MESSAGE_PLAIN) to be hashed
 *                                                    internally by the mcuxClRsa_pssEncode function, or as message digest (MCUXCLRSA_OPTION_MESSAGE_DIGEST).
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated in PKC RAM;
 *               - The result is stored in little-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pssEncode operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK   The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The input parameters are not valid.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pssEncode, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pssEncode(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);


/**
 * @brief RSA PSS verification operation
 *
 * This function performs an RSA PSS verification operation according to EMSA-PSS-VERIFY of PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Buffer, which contains the message or message digest to be verified
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   Pointer to buffer, which contains the encoded message digest to be verified. The content of the buffer is destroyed.
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               RFU: please set to NULL
 * @param[in]  saltlabelLength      Byte-length of salt
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message).  Note: This function only supports moduli, whose bit-length is a multiple of 8.
 * @param[in]  options              Options field
 * @param[out] pOutput              RFU: please set to NULL
 * @param[out] pOutLength           RFU: please set to NULL
 *
 *<dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput in case of option MCUXCLRSA_OPTION_MESSAGE_PLAIN. In case of option MCUXCLRSA_OPTION_MESSAGE_DIGEST,
 *               this buffer size is specified by the targeted hash algorithm. Thus for MCUXCLRSA_OPTION_MESSAGE_DIGEST, please set inputLength to zero.
 *     <dt>pVerificationInput:</dt>
 *          <dd>The padded message meets the following conditions:
 *               - It is located in PKC RAM;
 *               - It is provided in little-endian byte order;
 *               - The buffer size is aligned to PKC words;
 *               - The content of the buffer is destroyed.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>saltlabelLength:</dt>
 *          <dd>Byte-length salt.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>options:</dt>
 *          <dd> This field is used to select options of the sign operation:
 *                - bits 31-8: RFU; please set to zero
 *                - bits 7-0:   Specify, whether pInput is provided as plain message (MCUXCLRSA_OPTION_MESSAGE_PLAIN) to be hashed
 *                                                    internally by the mcuxClRsa_pssVerify function, or as message digest (MCUXCLRSA_OPTION_MESSAGE_DIGEST).
 *
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pssVerify operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_VERIFY_OK        The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_VERIFY_FAILED    The verification failed.
 * @retval #MCUXCLRSA_STATUS_ERROR            An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @if (MCUXCL_FEATURE_SESSION_SECURITYOPTIONS && MCUXCL_FEATURE_SESSION_SECURITYOPTIONS_DOUBLE_VERIFICATION_SIGNATURE)
 * @attention This function uses PKC which has to be initialized prior to calling the function.
 * @endif
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pssVerify, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pssVerify(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);


/**
 * @brief RSA PKCS1-v1_5 Encoding operation for signature generation
 *
 * This function performs an RSA PKCS1-v1_5 encoding operation according to EMSA-PKCS1-v1_5-ENCODE of PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Input-Buffer, which contains the message or message digest to be encoded
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   RFU: please set to NULL
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               RFU: please set to NULL
 * @param[in]  saltlabelLength      RFU: please set to zero
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message).  Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              Options field
 * @param[out] pOutput              Buffer to hold the result
 * @param[out] pOutLength           RFU: please set to NULL
 *
 *<dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput in case of option MCUXCLRSA_OPTION_MESSAGE_PLAIN. In case of option MCUXCLRSA_OPTION_MESSAGE_DIGEST,
 *               this buffer size is specified by the targeted hash algorithm. Thus for MCUXCLRSA_OPTION_MESSAGE_DIGEST, please set inputLength to zero.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>options:</dt>
 *          <dd> This field is used to select options of the sign operation:
 *                - bits 31-8: RFU; please set to zero
 *                - bits 7-0:   Specify, whether pInput is provided as plain message (MCUXCLRSA_OPTION_MESSAGE_PLAIN) to be hashed
 *                                                    internally by the mcuxClRsa_pkcs1v15Encode_sign function, or as message digest (MCUXCLRSA_OPTION_MESSAGE_DIGEST).
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - A buffer of modulus length bytes has to be allocated in PKC RAM;
 *               - The result is stored in little-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pkcs1v15Encode_sign operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK   The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The input parameters are not valid.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pkcs1v15Encode_sign, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pkcs1v15Encode_sign(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);

/**
 * @brief RSA PKCS1-v1_5 verification operation
 *
 * This function performs an RSA PKCS1-v1_5 verification operation according to PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Input-Buffer, which contains the message or message digest to be verified
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   Pointer to buffer, which contains the encoded message digest to be verified. The content of the buffer is destroyed.
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               RFU: please set to NULL.
 * @param[in]  saltlabelLength      RFU: please set to zero.
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message). Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              Options field
 * @param[out] pOutput              RFU: please set to NULL
 * @param[out] pOutLength           RFU: please set to NULL
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput in case of option MCUXCLRSA_OPTION_MESSAGE_PLAIN. In case of option MCUXCLRSA_OPTION_MESSAGE_DIGEST,
 *               this buffer size is specified by the targeted hash algorithm. Thus for MCUXCLRSA_OPTION_MESSAGE_DIGEST, please set inputLength to zero.
 *      <dt>pVerificationInput:</dt>
 *          <dd>The padded message meets the following conditions:
 *               - It is provided in little-endian byte order;
 *               - It is located in PKC RAM;
 *               - The buffer size is aligned to PKC words;
 *               - The content of the buffer is destroyed.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>options:</dt>
 *          <dd> This field is used to select options of the sign operation:
 *                - bits 31-8: RFU; please set to zero
 *                - bits 7-0:   Specify, whether pInput is provided as plain message (MCUXCLRSA_OPTION_MESSAGE_PLAIN) to be hashed
 *                                                    internally by the mcuxClRsa_pkcs1v15Verify function, or as message digest (MCUXCLRSA_OPTION_MESSAGE_DIGEST).
 *
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pkcs1v15Verify operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_VERIFY_OK        The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_VERIFY_FAILED    The verification failed.
 * @retval #MCUXCLRSA_STATUS_ERROR            An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @if (MCUXCL_FEATURE_SESSION_SECURITYOPTIONS && MCUXCL_FEATURE_SESSION_SECURITYOPTIONS_DOUBLE_VERIFICATION_SIGNATURE)
 * @attention This function uses PKC which has to be initialized prior to calling the function.
 * @endif
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pkcs1v15Verify, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pkcs1v15Verify(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);


/**
 * @brief Remove modulus blinding operation
 *
 * This function removes modulus blinding from the result of the exponentiation.
 *
 * @param[in] iR_iX_iNb_iB        indices of PKC operands
 * @param[in] iT2_iT1             indices of PKC operands
 * @param[in] nbPkcByteLength     length of Nb aligned to PKC word
 * @param[in] bPkcByteLength      length of B aligned to PKC word
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>iR_iX_iNb_iB:</dt>
 *        <dd><code>iB</code> (bits 0~7): index of blinding value with size bPkcByteLength (PKC operand).
 *        <br>Its size shall be at least bPkcByteLength.
 *        <br>The most significant PKC word of B shall be nonzero.
 *        <br><code>iNb</code> (bits 8~15): index of blinded modulus Nb (PKC operand).
 *        <br>Its size shall be at least nbPkcByteLength.
 *        <br>The NbDash of modulus shall be stored in the PKC word before modulus.
 *        <br><code>iX</code> (bits 16~23): index of input X in Montgomery representation (PKC operand).
 *        <br>Its size shall be at least nbPkcByteLength.
 *        <br><code>iR</code> (bits 16~23: index of result R in normal representation (PKC operand)
 *        <br>Its buffer size shall be at least (nbPkcByteLength - bPkcByteLength + 2 * MCUXCLRSA_PKC_WORDSIZE).
 *        <br>The result fits in size = (nbPkcByteLength - bPkcByteLength + MCUXCLRSA_PKC_WORDSIZE).</dd>
 *      <dt>iT2_iT1:</dt>
 *       <dd><code>iT1</code> (bits 0~7): index of temp1 (PKC operand).
 *       <br>Its buffer size shall be at least (nbPkcByteLength + MCUXCLRSA_PKC_WORDSIZE).
 *       <br><code>iT2</code> (bits 8~15): index of temp2 (PKC operand).
 *       <br>Its buffer size shall be at least MAX(nbPkcByteLength, 3 * MCUXCLRSA_PKC_WORDSIZE).</dd>
 *     <dt>@p nbPkcByteLength</dt>
 *       <dd>Length of modulus Nb. It shall be a multiple of MCUXCLRSA_PKC_WORDSIZE.</dd>
 *     <dt>@p bPkcByteLength</dt>
 *       <dd>Length of blinding value B. It shall be a multiple of MCUXCLRSA_PKC_WORDSIZE.</dd>
 *  </dl></dd>
 * </dl>
 *
 * <dl>
 *   <dt>PKC properties</dt>
 *   <dd><dl>
 *     <dt>PS1 lengths</dt>
 *       <dd>PS1 OPLEN = MCLEN defines operandSize = nbPkcByteLength (length of modulus Nb).</dd>
 *     <dt>PS2 lengths</dt>
 *       <dd>PS2 OPLEN and MCLEN will be modified, and original values will not be restored.</dd>
 *     <dt>ACTIV/GOANY</dt>
 *       <dd>#mcuxClPkc_WaitForReady will be called before returning to caller.
 *       <br>The PKC calculation might be still on-going,
 *           call #mcuxClPkc_WaitForFinish before CPU accesses to the result.</dd>
 *   </dl></dd>
 * </dl>
 *
 */
MCUX_CSSL_FP_FUNCTION_DEF(mcuxClRsa_RemoveBlinding)
MCUX_CSSL_FP_PROTECTED_TYPE(void) mcuxClRsa_RemoveBlinding(uint32_t iR_iX_iNb_iB,
     uint16_t iT2_iT1,
     uint32_t nbPkcByteLength,
     uint32_t bPkcByteLength);

#if defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_OAEP)
/**
 * @brief RSA OAEP Encoding operation
 *
 * This function performs an RSA OAEP Encoding operation according to PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Input-Buffer, which contains the message
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   RFU: please set to NULL
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               Buffer, which contains the input label
 * @param[in]  saltlabelLength      Length of the input label
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message). Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              RFU: please set to zero.
 * @param[out] pOutput              Buffer to hold the output encoded message
 * @param[out] pOutLength           RFU: please set to NULL
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput. It meets the following condition:
 *               - inputLength <= (8*keyBitLength) - (2*hLen) - 2
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the MGF1 and the lHash.
 *      <dt>pLabel:</dt>
 *          <dd>The label meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>saltlabelLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by label.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_oaepEncode operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK        The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT             The function failed due to invalid input parameters.
 * @retval #MCUXCLRSA_STATUS_ERROR                     An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_oaepEncode, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_oaepEncode(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);
#endif /* defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_OAEP) */

#if defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_OAEP)
/**
 * @brief RSA OAEP Decoding operation
 *
 * This function performs an RSA OAEP Decoding operation according to PKCS #1 v2.2.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pInput               Input-Buffer, which contains the encoded message to be decoded
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   RFU: please set to NULL.
 * @param[in]  pHashAlgo            Pointer to hash algorithm information
 * @param[in]  pLabel               Buffer, which contains the input label
 * @param[in]  saltlabelLength      Length of the input label
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message). Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              RFU: please set to zero.
 * @param[out] pOutput              Buffer to hold output decoded plaintext
 * @param[out] pOutLength           Length of output in bytes.
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput.
 *      <dt>pHashAlgo:</dt>
 *          <dd>Specifies the targeted hash algorithm, to be used for the mask generation function operation.
 *      <dt>pLabel:</dt>
 *          <dd>The label meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>saltlabelLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by label.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_oaepDecode operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK   The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The decoding failed due to invalid input parameters or invalid encoding.
 * @retval #MCUXCLRSA_STATUS_ERROR                An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_oaepDecode, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_oaepDecode(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);
#endif /* defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_OAEP) */

#if defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_PKCS1v15)
/**
 * @brief RSA PKCS1-v1_5 Encoding operation for encryption
 *
 * This function performs an RSA PKCS1-v1_5 Encoding operation for encryption (EME-PKCS1-v1_5 encoding
 * inlcuding length checking of the input message) according to PKCS #1 v2.2.
 *
 * @param[in]  pSession             Not used
 * @param[in]  pInput               Input-Buffer, which contains the message
 * @param[in]  inputLength          Size of Input
 * @param[in]  pVerificationInput   Not used
 * @param[in]  pHashAlgo            Not used
 * @param[in]  pLabel               Not used
 * @param[in]  saltlabelLength      Not used
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message). Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              Not used
 * @param[out] pOutput              Buffer to hold the output encoded message
 * @param[out] pOutLength           Not used
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>inputLength:</dt>
 *          <dd>Specifies the size of the buffer pointed to by pInput. It meets the following condition:
 *               - inputLength <= (8*keyBitLength) - 11
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pkcs1v15Encode_encrypt operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK        The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT             The function failed due to invalid input parameters.
 * @retval #MCUXCLRSA_STATUS_ERROR                     An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pkcs1v15Encode_encrypt, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pkcs1v15Encode_encrypt(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);
#endif /* defined(MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_PKCS1v15) */


#if defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_PKCS1v15)
/**
 * @brief RSA PKCS1-v1_5 Decoding operation for decryption
 *
 * This function performs an RSA PKCS1-v1_5 Decoding operation for decryption (EME-PKCS1-v1_5 decoding)
 * according to PKCS #1 v2.2.
 *
 * @param[in]  pSession             Not used
 * @param[in]  pInput               Input-Buffer, which contains the encoded message to be decoded
 * @param[in]  inputLength          Not used
 * @param[in]  pVerificationInput   Not used
 * @param[in]  pHashAlgo            Not used
 * @param[in]  pLabel               Not used
 * @param[in]  saltlabelLength      Not used
 * @param[in]  keyBitLength         Bit-length of key (bit-length of encoded message). Note: This function only supports moduli, whose bit-length is a multiple of 8
 * @param[in]  options              Not used
 * @param[out] pOutput              Buffer to hold the output decoded plaintext
 * @param[out] pOutLength           Length of output in bytes.
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pInput:</dt>
 *          <dd>The input meets the following conditions:
 *               - It is provided in big-endian byte order.
 *      <dt>keyBitLength:</dt>
 *          <dd>The key bit-length meets the following conditions:
 *               - This function only supports moduli, whose bit-length is a multiple of 8.
 *      <dt>pOutput:</dt>
 *          <dd>The output meets the following conditions:
 *               - The result is stored in big-endian byte order in the buffer pointed to by pOutput.
 *      <dt>pOutLength:</dt>
 *          <dd>Length of output in bytes.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_pkcs1v15Decode_decrypt operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_ENCODE_OK        The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT             The function failed due to invalid input parameters.
 * @retval #MCUXCLRSA_STATUS_ERROR                     An error occurred during the execution. In that case, expectations for the flow protection are not balanced.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_pkcs1v15Decode_decrypt, mcuxClRsa_PadVerModeEngine_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_pkcs1v15Decode_decrypt(
  mcuxClSession_Handle_t       pSession,
  mcuxCl_InputBuffer_t         pInput,
  const uint32_t              inputLength,
  uint8_t *                   pVerificationInput,
  mcuxClHash_Algo_t            pHashAlgo,
  mcuxCl_InputBuffer_t         pLabel,
  const uint32_t              saltlabelLength,
  const uint32_t              keyBitLength,
  const uint32_t              options,
  mcuxCl_Buffer_t              pOutput,
  uint32_t * const            pOutLength
);
#endif /* defined(MCUXCL_FEATURE_CIPHER_RSA_DECRYPT) && defined(MCUXCL_FEATURE_RSA_RSAES_PKCS1v15) */



/**
 * @brief RSA key generation of probable prime number p or q
 *
 * This function performs a generation of probable prime number based on the method specified in the FIPS 186-4, Appendix B.3.3.
 * The provided RNG (through the session) should be initialized (i.e., set the value of security_strength in accordance with
 * the key size as specified in SP 800-57, Part 1) before this function call.
 *
 * The main differences in comparison to method specified in the FIPS 186-4, Appendix B.3.3:
 * - Primes p and q are chosen to be congruent 3 mod 4 (this deviation has been approved).
 * - Check preformed in step 4.4 and 5.5 of this method is done using only 64 most significant bits
 *   of sqrt(2)(2^(nlen/2)–1) rounded up, this is 0xb504f333f9de6485 (this deviation has been approved).
 * - There is no check if (|p–q| <= 2((nlen/2)–100) when generating prime q (check preformed in
 *   step 5.4 of this method). Instead of this check shall be done after generating p and q.
 *   Rationale: This inequality occurs with a very small probability and it's usually treated
 *   as a hardware failure. As an alternative to generating new prime q number error code
 *   shall be returned (this deviation has been approved).
 * - The pre-check against products of small primes was added before the Miller-Rabin test.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pE                   Pointer to data, which contains public exponent e
 * @param[out] pProbablePrime       Pointer to data, which contains the generated probable prime number
 * @param[in]  keyBitLength         Bit-length of key
 *
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function. The RNG shall be initialized
 *              with the entropy level (security strength) in accordance with the value of keyBitLength, as specified in SP 800-57, Part 1.
 *      <dt>pE:</dt>
 *          <dd>The public exponent e meets the following conditions:
 *              - The public exponent e shall be FIPS 186-4 compliant;
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - The keyEntryLength shall be exact length of e (without leading zeros).
 *      <dt>pProbablePrime:</dt>
 *          <dd>Pointer to data, which contains the generated probable prime number. It meets the following conditions:
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - One additional PKC word shall be reserved before pPrimeCandidate->pKeyEntryData for the NDash calculated in the mcuxClRsa_GenerateProbablePrime
 *              - Probable prime number will be stored in little-endian byte order;
 *              - Length of the probable prime number will be keyBitLength/2.
 *      <dt>keyBitLength:</dt>
 *          <dd>Specifies the size of the generated key, it shall be even value.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_GenerateProbablePrime operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK                    The function executed successfully.
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_ITERATIONSEXCEEDED    The function exceeds the limit of iterations to generate a prime.
 * @retval #MCUXCLRSA_STATUS_RNG_ERROR                           An error occurred during random nubmer generation.
 *
 * @attention This function uses DRBG and PRNG (directly and indirectly) which have to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_GenerateProbablePrime)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_GenerateProbablePrime(
  mcuxClSession_Handle_t           pSession,
  mcuxClRsa_KeyEntry_t *           pE,
  mcuxClRsa_KeyEntry_t *           pPrimeCandidate,
  const uint32_t                  keyBitLength
);

/**
 * @brief Test prime candidate for RSA key generation
 *
 * This function performs a test of probable prime number based on the method specified in the FIPS 186-4, Appendix B.3.3.
 *
 * The main differences in comparison to method specified in the FIPS 186-4, Appendix B.3.3:
 * - Primes p and q are chosen to be congruent 3 mod 4 (this deviation has been approved).
 * - Check preformed in step 4.4 and 5.5 of this method is done using only 64 most significant bits
 *   of sqrt(2)(2^(nlen/2)–1) rounded up, this is 0xb504f333f9de6485 (this deviation has been approved).
 * - There is no check if |p - q| <= 2^(nlen/2 - 100) when generating prime q (check preformed in
 *   step 5.4 of this method). Instead of this check shall be done after generating p and q.
 *   Rationale: This inequality occurs with a very small probability and it's usually treated
 *   as a hardware failure. As an alternative to generating new prime q number error code
 *   shall be returned (this deviation has been approved).
 * - The pre-check against products of small primes was added before the Miller-Rabin test.
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pE                   Pointer to data, which contains public exponent e
 * @param[out] pProbablePrime       Pointer to data, which contains the generated probable prime number
 * @param[in]  keyBitLength         Bit-length of key
 * @param[in]  numberTestIterations Number of Miller-Rabin test iterations
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function. The RNG shall be initialized
 *              with the entropy level (security strength) in accordance with the value of keyBitLength, as specified in SP 800-57, Part 1.
 *      <dt>pE:</dt>
 *          <dd>The public exponent e meets the following conditions:
 *              - The public exponent e shall be FIPS 186-4 compliant;
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - The keyEntryLength shall be exact length of e (without leading zeros).
 *      <dt>pProbablePrime:</dt>
 *          <dd>Pointer to data, which contains the generated probable prime number. It meets the following conditions:
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - One additional PKC word shall be reserved before pPrimeCandidate->pKeyEntryData for the NDash calculated in this function;
 *              - Probable prime number will be stored in little-endian byte order;
 *              - Length of the probable prime number will be keyBitLength/2.
 *      <dt>keyBitLength:</dt>
 *          <dd>Specifies the size of the generated key, it shall be even value.
 *      <dt>iNumToCmp_iA0:</dt>
 *          <dd> iNumToCmp: index of the buffer to store 0xb504f333f9de6485, which is 64 most significant bits of sqrt(2)(2^(nlen/2)-1) rounded up.
 *          <br> iA0: index of the buffer to store 0xC0CFD797, which is the product of the first 9 prime numbers starting from 3.
 *      <dt>numberTestIterations:</dt>
 *          <dd>Specifies the number of Miller-Rabin test iterations.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_TestPrimeCandidate operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_INTERNAL_TESTPRIME_CMP_FAILED       prime candidate < sqrt(2)(2^((nlen/2)-1)), only 64 most significant bits are compared.
 * @retval #MCUXCLRSA_STATUS_INTERNAL_TESTPRIME_GCDA0_FAILED     prime candidate is not coprime to A0 - product of the first 9 prime numbers.
 * @retval #MCUXCLRSA_STATUS_INTERNAL_TESTPRIME_GCDE_FAILED      prime candidate - 1 is not coprime to the public exponent e.
 * @retval #MCUXCLRSA_STATUS_INTERNAL_TESTPRIME_MRT_FAILED       The prime candidate did not pass the Miller-Rabin test.
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK                    The prime candidate is probably prime.
 * @retval #MCUXCLRSA_STATUS_RNG_ERROR                           Random number (DRBG / PRNG) error (unexpected behavior).
 *
 * @attention This function uses DRBG and PRNG (indirectly) which have to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_TestPrimeCandidate)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_TestPrimeCandidate(
  mcuxClSession_Handle_t           pSession,
  mcuxClRsa_KeyEntry_t *           pE,
  mcuxClRsa_KeyEntry_t *           pPrimeCandidate,
  const uint32_t                  keyBitLength,
  const uint32_t                  iNumToCmp_iA0,
  const uint32_t                  numberTestIterations
);

/**
 * @brief RSA Miller-Rabin probabilistic primality test function
 *
 * This function is used to test prime candidate for primality using Miller-Rabin
 * probabilistic primality tests described in FIPS 186-4, Appendices C.3.1.
 *
 * <dt>Assumptions:</dt>
 * Prime candidate is congruent 3 mod 4. Taking this into account, the Miller-Rabin
 * algorithm gets simplified (due to fact that a=1 the step 4.5 is skipped);
 *
 * @param[in]  pSession                Pointer to #mcuxClSession_Descriptor
 * @param[in]  iP_iT                   Pointer table indices of parameters
 * @param[in]  keyBitLength            Bit-length of key
 * @param[in]  numberTestIterations    Number of Miller-Rabin test iterations
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function. The RNG shall be initialized
 *              with the entropy level (security strength) in accordance with the value of primeLength, as specified in SP 800-57, Part 1.
 *      <dt>iP_iT:</dt>
 *          <dd> iP: index of prime candidate buffer, of which the size should be at least MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(byteLenPrimeCandidate).
 *                   Additionally one PKC word shall be reserved before P for the NDash calculated in mcuxClRsa_MillerRabinTest.
 *                   Prime candidate length shall be keyBitLength/2.
 *          <br> iT: index of temp buffer, of which the size should be at least 9 * MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(byteLenPrimeCandidate) + 10 * PKC wordsize.
 *      <dt>keyBitLength:</dt>
 *          <dd>Specifies the size of the generated key, it shall be even value.
 *      <dt>numberTestIterations:</dt>
 *          <dd>Specifies the number of Miller-Rabin test iterations.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_MillerRabinTest operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK               The prime candidate is probably prime.
 * @retval #MCUXCLRSA_STATUS_INTERNAL_TESTPRIME_MRT_FAILED  The prime candidate did not pass the Miller-Rabin test.
 * @retval #MCUXCLRSA_STATUS_RNG_ERROR                      Random number (DRBG / PRNG) error (unexpected behavior)
 *
 * @attention This function uses DRBG and PRNG which have to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_MillerRabinTest)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_MillerRabinTest(
  mcuxClSession_Handle_t           pSession,
  uint32_t                        iP_iT,
  uint32_t                        keyBitLength,
  const uint32_t                  numberTestIterations
);


/**
 * @brief RSA function which computes private exponent d compliant with FIPS 186-4
 *
 * This function is used to compute private exponent d for given p, q and e.
 * The d is calculated as d = e^(–1) mod (LCM(p–1, q–1)),
 * where: LCM(a,b) = (ab)/gcd(a,b).
 *
 *
 * @param[in]  pSession             Pointer to #mcuxClSession_Descriptor
 * @param[in]  pE                   Pointer to buffer, which contains public exponent e
 * @param[in]  pP                   Pointer to buffer, which contains prime p
 * @param[in]  pQ                   Pointer to buffer, which contains prime q
 * @param[out] pD                   Pointer to buffer, which contains the computed private exponent d
 * @param[in]  keyBitLength         Bit-length of key
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 *      <dt>pE:</dt>
 *          <dd>The public exponent e meets the following conditions:
 *              - The public exponent e shall be FIPS 186-4 compliant;
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - The keyEntryLength shall be exact length of e (without leading zeros).
 *      <dt>pP:</dt>
 *          <dd>The prime p meets the following conditions:
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - Prime p length shall be keyBitLength/2.
 *      <dt>pQ:</dt>
 *          <dd>The prime q meets the following conditions:
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - Prime q length shall be keyBitLength/2.
 *      <dt>pD:</dt>
 *          <dd>The private exponent d meets the following conditions:
 *              - A buffer pointed by pKeyEntryData shall be located in PKC RAM, its address and length shall be aligned to PKC word;
 *                Size of the buffer should be at least keyBitLength/8 + PKC wordsize.
 *              - Data in this buffer shall be stored in little-endian byte order;
 *              - The private exponent d length is  not greater than keyBitLength.
 *      <dt>keyBitLength:</dt>
 *          <dd>Specifies the size of the generated key, it shall be even value.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_ComputeD operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK     The prime candidate is probably prime.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        For a given input (it means p, q and e) the computed D does not meet
 *                                               the requirements specified in the FIPS 186-4, Appendix B.3.1.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_ComputeD)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_ComputeD(
  mcuxClSession_Handle_t           pSession,
  mcuxClRsa_KeyEntry_t *           pE,
  mcuxClRsa_KeyEntry_t *           pP,
  mcuxClRsa_KeyEntry_t *           pQ,
  mcuxClRsa_KeyEntry_t *           pD,
  const uint32_t                  keyBitLength
);

/**
 * @brief RSA function which test if |p–q| <= 2^(nlen/2–100).
 *
 * This function is used to test if |p–q| <= 2^(nlen/2–100).
 * This is a verification required by FIPS 186-4 (Appendix B.3.3, step 5.4).
 *
 * @param[in]  iP_iQ_iT1          Pointer table indices of parameters
 * @param[in]  primeByteLength    Bytelength of parameters p and q
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>iP_iQ_iT1:</dt>
 *          <dd> iP: index of p prime buffer. The size shall be a multiple of PKC word and at least primeByteLength.
 *          <br> iQ: index of q prime buffer. The size shall be a multiple of PKC word and at least primeByteLength.
 *          <br> iT: index of temporary buffer. The size shall be at least 2 * MCUXCLRSA_ALIGN_TO_PKC_WORDSIZE(16).
 *      <dt>primeByteLength:</dt>
 *          <dd>The length of primes p and q. It must be a multiple of the PKC word.
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_TestPQDistance operation (see @ref MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t))
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK     The p and q primes meet the FIPS requirements.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The p and q primes pass this test, it means does not meet the FIPS requirements.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_TestPQDistance)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClRsa_Status_t) mcuxClRsa_TestPQDistance(uint32_t iP_iQ_iT, uint32_t primeByteLength);


/**
 * @brief RSA function which calculates modular inversion, X^(-1) mod N
 *
 * @param[in]  iR_iX_iN_iT       Pointer table indices of parameters
 *
 * <dt>Assumptions:</dt>
 * - If X and N are not coprime, the result will be incorrect.
 * - The N shall be congruent 2 mod 4
 * - Both ps1 (OP)LEN and MCLEN need to be initialized (MCLEN = OPLEN = operandSize).
 * - The size of X must be <= operandSize
 * - The offsets (UPTRT[iR] and UPTRT[iT]) must be initialized properly (equal to a multiple of PKC wordsize).
 * - The PKC calculation might be still on-going, call #mcuxClPkc_WaitForFinish before CPU accesses to the result.
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>iR_iX_iN_iT:</dt>
 *          <dd> iR: index of result buffer, of which the size should be at least operandSize + PKC wordsize.
 *          <br> iX: index of X buffer, of which the content (X) will be destroyed.
 *          <br> iN: index of modulus buffer, of which the content (N) will be destroyed. One PKC word shall be reserved before N,
 *                   for the NDash calculated in mcuxClRsa_ModInv.
 *          <br> iT: index of temp buffer, of which the size should be at least operandSize + PKC wordsize.
 *  </dl></dd>
 * </dl>
 *
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_ModInv)
MCUX_CSSL_FP_PROTECTED_TYPE(void) mcuxClRsa_ModInv(uint32_t iR_iX_iN_iT);


/**
 * @brief RSA function which verifies whether RSA public exponent is FIPS compliant (i.e., is odd value
 *  in the range 2^16 < e < 2^256) and determines its length without leading zeros.
 *
 * @param[in]  pE             Pointer to buffer, which contains public exponent e
 * @param[out] exactLength    Pointer to data were the exact length of public exponent e will be set
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>pE:</dt>
 *          <dd>The public exponent e in a big endian order
 *      <dt>exactLength:</dt>
 *          <dd>Exact length of public exponent e
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_VerifyE operation (see @ref mcuxClRsa_Status_Protected_t)
 * @retval #MCUXCLRSA_STATUS_KEYGENERATION_OK     The prime candidate is probably prime.
 * @retval #MCUXCLRSA_STATUS_INVALID_INPUT        The public exponent e does not meet the requirements
 *                                               specified in the FIPS 186-4, Appendix B.3.1.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_VerifyE)
mcuxClRsa_Status_Protected_t mcuxClRsa_VerifyE(mcuxClRsa_KeyEntry_t *pE, uint32_t *exactLength);

/**
 * @brief RSA function which returns the minimum number of Miller-Rabin test iterations for given
 * prime bit length and error probability lower than 2^(-125).
 *
 * The number of Miller-Rabin test iterations are determined for the prime lengths of 1024, 1536, 2048,
 * 3072, 4096 bits based on Table 1 of SOGIS Agreed Cryptographic Mechanisms version 1.2 (it is following
 * the Appendix F.1 of FIPS PUB 186-4).
 *
 * @param[in]  primeBitLength   The prime bit length
 *
 * <dl>
 *  <dt>Parameter properties</dt>
 *
 *  <dd><dl>
 *      <dt>primeBitLength:</dt>
 *          <dd>The prime bit length
 *  </dl></dd>
 * </dl>
 *
 * @return Minimum number of Miller-Rabin test iterations for given prime bit length
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_getMillerRabinTestIterations)
MCUX_CSSL_FP_PROTECTED_TYPE(uint32_t) mcuxClRsa_getMillerRabinTestIterations(const uint32_t primeBitLength);




#ifdef MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT
/**
 * @brief RSA Encrypt operation
 *
 * This function performs an RSA encryption according to RSAES-OAEP-ENCRYPT or RSAES-PKCS1-V1_5-ENCRYPT of PKCS #1 v2.2.
 * Based on the selection of the padding mode and hash function, one of the supported encoding operations is performed.
 *
 * @param[in]  pSession                  Pointer to #mcuxClSession_Descriptor
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 * @param[in]  pContext                  Pointer to the context. For RSA this is ignored.
 * @endif
 * @param[in]  key                       Key handle for the input key
 * @param[in]  mode                      Cipher mode descriptor
 * @param[in]  pLabel                    Pointer to the label.
 * @param[in]  labelLength               Byte length of the label.
 * @param[in]  pIn                       Pointer to the input buffer that contains the plain data that needs to be encrypted.
 * @param[in]  inLength                  Number of bytes of plain data in the pIn buffer.
 * @param[in]  pOut                      Pointer to the output buffer where the encrypted data needs to be written.
 * @param[out] pOutLength                Will contain the number of bytes of encrypted data that have been written to the pOut buffer.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 * @param[in]  steps                     Indicates the processing step. For RSA this is ignored.
 * @endif
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 *      <dt>pContext:</dt>
 *          <dd>Unused.
 * @endif
 *      <dt>key:</dt>
 *          <dd>The key handle should be properly set up before calling the RSA function.
 *              The following bit-lengths of the modulus are supported: 1024, 2048, 3072, 4096, 6144 and 8192.
 *      <dt>mode:</dt>
 *          <dd>The mode should be properly set up before calling the RSA function, and contain a pointer to a valid protocol descriptor.
 *      <dt>pLabel:</dt>
 *          <dd>The label is used only used for OAEP encoding.
 *      <dt>inLength:</dt>
 *          <dd>The input length should be compliant with the limitations of the selected encoding mode (PKCS#1v1.5 or OAEP)
 *      <dt>pOutLength:</dt>
 *          <dd>Always equal to the modulus byte length.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 *      <dt>steps:</dt>
 *          <dd>Unused.
 * @endif
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_Util_encrypt operation.
 * The return codes should match those of @ref mcuxClCipher
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_Util_encrypt, mcuxClCipher_CryptFunc_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClCipher_Status_t) mcuxClRsa_Util_encrypt(
    mcuxClSession_Handle_t           pSession,
    mcuxClCipher_Context_t * const   pContext,
    mcuxClKey_Handle_t               key,
    mcuxClCipher_Mode_t              mode,
    mcuxCl_InputBuffer_t             pLabel,
    uint32_t                        labelLength,
    mcuxCl_InputBuffer_t             pIn,
    uint32_t                        inLength,
    mcuxCl_Buffer_t                  pOut,
    uint32_t * const                pOutLength,
    uint32_t                        steps
);
#endif /* MCUXCL_FEATURE_CIPHER_RSA_ENCRYPT */

#ifdef MCUXCL_FEATURE_CIPHER_RSA_DECRYPT
/**
 * @brief RSA Decrypt operation
 *
 * This function performs an RSA decryption according to RSAES-OAEP-DECRYPT or RSAES-PKCS1-V1_5-DECRYPT of PKCS #1 v2.2.
 * Based on the selection of the padding mode and hash function, one of the supported decoding operations is performed.
 *
 * @param[in]  pSession                  Pointer to #mcuxClSession_Descriptor
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 * @param[in]  pContext                  Pointer to the context. For RSA this is ignored.
 * @endif
 * @param[in]  key                       Key handle for the input key
 * @param[in]  mode                      Cipher mode descriptor
 * @param[in]  pLabel                    Pointer to the label.
 * @param[in]  labelLength               Byte length of the label.
 * @param[in]  pIn                       Pointer to the input buffer that contains the encrypted data that needs to be decrypted.
 * @param[in]  inLength                  RFU
 * @param[in]  pOut                      Pointer to the output buffer where the plain data needs to be written.
 * @param[out] pOutLength                Will contain the number of bytes of encrypted data that have been written to the pOut buffer.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 * @param[in]  steps                     Indicates the processing step. For RSA this is ignored.
 * @endif
 *
 *  <dd><dl>
 *      <dt>pSession:</dt>
 *          <dd>The session pointed to by pSession has to be initialized prior to a call to this function.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 *      <dt>pContext:</dt>
 *          <dd>Unused.
 * @endif
 *      <dt>key:</dt>
 *          <dd>The key handle should be properly set up before calling the RSA function.
 *              The following bit-lengths of the modulus are supported: 1024, 2048, 3072, 4096, 6144 and 8192.
 *      <dt>mode:</dt>
 *          <dd>The mode should be properly set up before calling the RSA function, and contain a pointer to a valid protocol descriptor.
 *      <dt>pLabel:</dt>
 *          <dd>The label is used only used for OAEP encoding.
 *      <dt>inLength:</dt>
 *          <dd>RFU: Number of bytes of encrypted data in the pIn buffer. For RSA this is ignored, and it is considered that this
 *              is always equal to the modulus byte length.
 * @if (MCUXCL_FEATURE_CIPHER_CRYPT)
 *      <dt>steps:</dt>
 *          <dd>Unused.
 * @endif
 *  </dl></dd>
 * </dl>
 *
 * @return Status of the mcuxClRsa_Util_decrypt operation.
 * The return codes should match those of @ref mcuxClCipher
 *
 * @attention This function uses PRNG which has to be initialized prior to calling the function.
 */
MCUX_CSSL_FP_FUNCTION_DECL(mcuxClRsa_Util_decrypt, mcuxClCipher_CryptFunc_t)
MCUX_CSSL_FP_PROTECTED_TYPE(mcuxClCipher_Status_t) mcuxClRsa_Util_decrypt(
    mcuxClSession_Handle_t           pSession,
    mcuxClCipher_Context_t * const   pContext,
    mcuxClKey_Handle_t               key,
    mcuxClCipher_Mode_t              mode,
    mcuxCl_InputBuffer_t             pLabel,
    uint32_t                        labelLength,
    mcuxCl_InputBuffer_t             pIn,
    uint32_t                        inLength,
    mcuxCl_Buffer_t                  pOut,
    uint32_t * const                pOutLength,
    uint32_t                        steps
);
#endif /* MCUXCL_FEATURE_CIPHER_RSA_DECRYPT  */

/** Inline function for proper type casts*/

/**
 * @brief Cast a pointer to word-aligned data to a pointer to the mcuxClRsa_Key type.
 *
 * @param pKey    The pointer to cast to a proper key RSA handle. Must be aligned.
 */
MCUX_CSSL_FP_FUNCTION_DEF(mcuxClRsa_castToRsaKey)
static inline mcuxClRsa_Key* mcuxClRsa_castToRsaKey(uint32_t *pKey)
{
    MCUX_CSSL_ANALYSIS_START_PATTERN_REINTERPRET_MEMORY_OF_OPAQUE_TYPES()
    return (mcuxClRsa_Key *) pKey;
    MCUX_CSSL_ANALYSIS_STOP_PATTERN_REINTERPRET_MEMORY_OF_OPAQUE_TYPES()
}

/**
 * @}
 */ /* mcuxClRsa_Internal_Functions */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MCUXCLRSA_INTERNAL_FUNCTIONS_H_ */

