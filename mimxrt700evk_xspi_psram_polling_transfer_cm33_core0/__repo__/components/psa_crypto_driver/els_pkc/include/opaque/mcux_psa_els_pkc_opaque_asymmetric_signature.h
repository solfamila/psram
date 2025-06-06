/*
 * Copyright 2023 NXP
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MCUX_PSA_ELS_PKC_OPAQUE_ASYMMETRIC_SIGNATURE_H
#define MCUX_PSA_ELS_PKC_OPAQUE_ASYMMETRIC_SIGNATURE_H

/** \file mcux_psa_els_pkc_opaque_asymmetric_signature.h
 *
 * This file contains the declaration of the entry points associated to the
 * asymmetric signature capability as described by the PSA Cryptoprocessor
 * Driver interface specification
 *
 */

#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief Sign a message
 *
 * \param[in]  attributes       Attributes of the key to use
 * \param[in]  key_buffer       Key material buffer
 * \param[in]  key_buffer_size  Size in bytes of the key
 * \param[in]  alg              Algorithm to use
 * \param[in]  input            Data to sign buffer
 * \param[in]  input_length     Size in bytes of the data to sign
 * \param[out] signature        Buffer to hold the signature data
 * \param[in]  signature_size   Size in bytes of the signature buffer
 * \param[out] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_sign_message(const psa_key_attributes_t *attributes,
                                         const uint8_t *key_buffer,
                                         size_t key_buffer_size,
                                         psa_algorithm_t alg,
                                         const uint8_t *input,
                                         size_t input_length,
                                         uint8_t *signature,
                                         size_t signature_size,
                                         size_t *signature_length);
/*!
 * \brief Verify a message signature
 *
 * \param[in] attributes       Attributes of the key to use
 * \param[in] key_buffer       Key material buffer
 * \param[in] key_buffer_size  Size in bytes of the key
 * \param[in] alg              Algorithm to use
 * \param[in] input            Data to sign buffer
 * \param[in] input_length     Size in bytes of the data to sign
 * \param[in] signature        Signature to verify
 * \param[in] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_verify_message(const psa_key_attributes_t *attributes,
                                           const uint8_t *key_buffer,
                                           size_t key_buffer_size,
                                           psa_algorithm_t alg, const uint8_t *input,
                                           size_t input_length, const uint8_t *signature,
                                           size_t signature_length);
/*!
 * \brief Sign a precomputed hash of a message
 *
 * \param[in]  attributes       Attributes of the key to use
 * \param[in]  key_buffer       Key material buffer
 * \param[in]  key_buffer_size  Size in bytes of the key
 * \param[in]  alg              Algorithm to use
 * \param[in]  input            Hash to sign buffer
 * \param[in]  input_length     Size in bytes of the data to sign
 * \param[out] signature        Buffer to hold the signature data
 * \param[in]  signature_size   Size in bytes of the signature buffer
 * \param[out] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_sign_hash(const psa_key_attributes_t *attributes,
                                      const uint8_t *key_buffer,
                                      size_t key_buffer_size,
                                      psa_algorithm_t alg, const uint8_t *input,
                                      size_t input_length, uint8_t *signature,
                                      size_t signature_size, size_t *signature_length);
/*!
 * \brief Verify a message signature on a hash
 *
 * \param[in] attributes       Attributes of the key to use
 * \param[in] key_buffer       Key material buffer
 * \param[in] key_buffer_size  Size in bytes of the key
 * \param[in] alg              Algorithm to use
 * \param[in] hash            Hash to sign buffer
 * \param[in] hash_length     Size in bytes of the data to sign
 * \param[in] signature        Signature to verify
 * \param[in] signature_length Size in bytes of the signature
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_verify_hash(const psa_key_attributes_t *attributes,
                                        const uint8_t *key_buffer,
                                        size_t key_buffer_size,
                                        psa_algorithm_t alg, const uint8_t *hash,
                                        size_t hash_length, const uint8_t *signature,
                                        size_t signature_length);
#ifdef __cplusplus
}
#endif
#endif /* MCUX_PSA_ELS_PKC_OPAQUE_ASYMMETRIC_SIGNATURE_H */
