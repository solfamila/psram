/*
 * Copyright 2024 NXP
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MCUX_PSA_S2XX_AEAD_H
#define MCUX_PSA_S2XX_AEAD_H

/** \file mcux_psa_s2xx_aead.h
 *
 * This file contains the declaration of the entry points associated to the
 * generic_aead capability (single-part and multipart) as described by the PSA
 * Cryptoprocessor Driver interface specification
 *
 */

#include "psa/crypto.h"
#include "mcux_psa_s2xx_common_init.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Encrypt and authenticate with an AEAD algorithm in one-shot
 *
 * \param[in]  attributes             Attributes of the key to use
 * \param[in]  key_buffer             Key material buffer
 * \param[in]  key_buffer_size        Size in bytes of the key
 * \param[in]  alg                    Algorithm to use
 * \param[in]  nonce                  Nonce buffer
 * \param[in]  nonce_length           Size in bytes of the nonce
 * \param[in]  additional_data        Data to authenticate buffer
 * \param[in]  additional_data_length Size in bytes of additional_data
 * \param[in]  plaintext              Data to encrypt buffer
 * \param[in]  plaintext_length       Size in bytes of the data to encrypt
 * \param[out] ciphertext             Buffer to hold the encrypted data
 * \param[in]  ciphertext_size        Size in bytes of the ciphertext buffer
 * \param[out] ciphertext_length      Size in bytes of the encrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t ele_s2xx_transparent_aead_encrypt(const psa_key_attributes_t *attributes,
                                               const uint8_t *key_buffer,
                                               size_t key_buffer_size,
                                               psa_algorithm_t alg,
                                               const uint8_t *nonce,
                                               size_t nonce_length,
                                               const uint8_t *additional_data,
                                               size_t additional_data_length,
                                               const uint8_t *plaintext,
                                               size_t plaintext_length,
                                               uint8_t *ciphertext,
                                               size_t ciphertext_size,
                                               size_t *ciphertext_length);

/*!
 * \brief Decrypt and verify tag with an AEAD algorithm in one-shot
 *
 * \param[in]  attributes             Attributes of the key to use
 * \param[in]  key_buffer             Key material buffer
 * \param[in]  key_buffer_size        Size in bytes of the key
 * \param[in]  alg                    Algorithm to use
 * \param[in]  nonce                  Nonce buffer
 * \param[in]  nonce_length           Size in bytes of the nonce
 * \param[in]  additional_data        Data to authenticate buffer
 * \param[in]  additional_data_length Size in bytes of additional_data
 * \param[in]  ciphertext             Data to decrypt buffer
 * \param[in]  ciphertext_length      Size in bytes of the data to decrypt
 * \param[out] plaintext              Buffer to hold the decrypted data
 * \param[in]  plaintext_size         Size in bytes of the plaintext buffer
 * \param[out] plaintext_length       Size in bytes of the decrypted data
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t ele_s2xx_transparent_aead_decrypt(const psa_key_attributes_t *attributes,
                                               const uint8_t *key_buffer,
                                               size_t key_buffer_size,
                                               psa_algorithm_t alg,
                                               const uint8_t *nonce,
                                               size_t nonce_length,
                                               const uint8_t *additional_data,
                                               size_t additional_data_length,
                                               const uint8_t *ciphertext,
                                               size_t ciphertext_length,
                                               uint8_t *plaintext,
                                               size_t plaintext_size,
                                               size_t *plaintext_length);

/* These are not implemented for now as ELE doesn't support multi-part operations */
#if 0

/*!
 * \brief Set the key for a multipart authenticated encryption operation.
 *
 * \param[in] operation       The operation object to set up.
 * \param[in] attributes      The attributes of the key to be set.
 * \param[in] key_buffer      The buffer containing the key material.
 * \param[in] key_buffer_size Size of \p key_buffer in bytes.
 * \param[in] alg             The AEAD algorithm
 *                            (#PSA_ALG_IS_AEAD(\p alg) must be true).
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_BAD_STATE
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_encrypt_setup(
    ele_s2xx_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg);

/*!
 * \brief Set the key for a multipart authenticated decryption operation
 *
 * \param[in] operation       The operation object to set up.
 * \param[in] attributes      The attributes of the key to be set.
 * \param[in] key_buffer      The buffer containing the key material.
 * \param[in] key_buffer_size Size of \p key_buffer in bytes.
 * \param[in] alg             The AEAD algorithm
 *                            (#PSA_ALG_IS_AEAD(\p alg) must be true).
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_BAD_STATE
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_decrypt_setup(
    ele_s2xx_aead_operation_t *operation,
    const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer, size_t key_buffer_size,
    psa_algorithm_t alg);

/*!
 * \brief Set the nonce for an authenticated encryption or decryption operation.
 *
 * \param[in] operation    Active AEAD operation.
 * \param[in] nonce        Buffer containing the nonce to use.
 * \param[in] nonce_length Size of the nonce in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_GENERIC_ERROR
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t ele_s2xx_transparent_aead_set_nonce(
    ele_s2xx_aead_operation_t *operation,
    const uint8_t *nonce,
    size_t nonce_length);

/*!
 * \brief Declare the lengths of the message and additional data for AEAD
 *
 * \param[in] operation        Active AEAD operation.
 * \param[in] ad_length        Size of the additional data in bytes.
 * \param[in] plaintext_length Size of the plaintext to encrypt in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_set_lengths(
    ele_s2xx_aead_operation_t *operation,
    size_t ad_length,
    size_t plaintext_length);

/*!
 * \brief Pass additional data to an active AEAD operation
 *
 * \param[in] operation    Active AEAD operation.
 * \param[in] input        Buffer containing the additional data.
 * \param[in] input_size   Size of the input buffer in bytes.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t ele_s2xx_transparent_aead_update_ad(
    ele_s2xx_aead_operation_t *operation,
    const uint8_t *input,
    size_t input_size);

/*!
 * \brief Encrypt or decrypt a message fragment in an active AEAD operation.
 *
 * \param[in]  operation     Active AEAD operation.
 * \param[in]  input         Buffer containing the message fragment.
 * \param[in]  input_length  Size of the input buffer in bytes.
 * \param[out] output        Buffer where the output is to be written.
 * \param[in]  output_size   Size of the output buffer in bytes.
 * \param[out] output_length The number of bytes that make up the output.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 * \retval #PSA_ERROR_DATA_INVALID
 */
psa_status_t ele_s2xx_transparent_aead_update(
    ele_s2xx_aead_operation_t *operation,
    const uint8_t *input,
    size_t input_length,
    uint8_t *output,
    size_t output_size,
    size_t *output_length);

/*!
 * \brief Finish encrypting a message in an AEAD operation.
 *
 * \param[in] operation          Active AEAD operation.
 * \param[out] ciphertext        Buffer containing the ciphertext.
 * \param[in] ciphertext_size    Size of the ciphertext buffer in bytes.
 * \param[out] ciphertext_length The number of bytes that make up the ciphertext
 * \param[out] tag               Buffer where the tag is to be written.
 * \param[in] tag_size           Size of the tag buffer in bytes.
 * \param[out] tag_length        The number of bytes that make up the tag.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_finish(
    ele_s2xx_aead_operation_t *operation,
    uint8_t *ciphertext,
    size_t ciphertext_size,
    size_t *ciphertext_length,
    uint8_t *tag,
    size_t tag_size,
    size_t *tag_length);

/*!
 * \brief Finish decrypting a message in an AEAD operation.
 *
 * \param[in] operation         Active AEAD operation.
 * \param[out] plaintext        Buffer containing the plaintext.
 * \param[in] plaintext_size    Size of the plaintext buffer in bytes.
 * \param[out] plaintext_length The number of bytes that make up the plaintext
 * \param[in] tag               Buffer containing the tag
 * \param[in] tag_size          Size of the tag buffer in bytes
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_INVALID_ARGUMENT
 * \retval #PSA_ERROR_CORRUPTION_DETECTED
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_verify(
    ele_s2xx_aead_operation_t *operation,
    uint8_t *plaintext,
    size_t plaintext_size,
    size_t *plaintext_length,
    const uint8_t *tag,
    size_t tag_size);
/*!
 * \brief Abort an AEAD operation
 *
 * \param[out] operation Initialized AEAD operation.
 *
 * \retval #PSA_SUCCESS
 * \retval #PSA_ERROR_NOT_SUPPORTED
 */
psa_status_t ele_s2xx_transparent_aead_abort(ele_s2xx_aead_operation_t *operation);

#endif // Not Supported

#ifdef __cplusplus
}
#endif
#endif /* MCUX_PSA_S2XX_AEAD_H */
