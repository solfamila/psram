/*
 * Copyright 2023 NXP
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef MCUX_PSA_ELS_PKC_OPAQUE_KEY_GENERATION_H
#define MCUX_PSA_ELS_PKC_OPAQUE_KEY_GENERATION_H

/** \file mcux_psa_opaque_els_pkc_key_generation.h
 *
 * This file contains the declaration of the entry points associated to the
 * key generation (i.e. random generation and extraction of public keys) as
 * described by the PSA Cryptoprocessor Driver interface specification
 *
 */

#include "psa/crypto.h"
#include <mcuxClKey.h>
#include <internal/mcuxClKey_Types_Internal.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief import opaque key
 */
psa_status_t els_pkc_opaque_import_key(const psa_key_attributes_t *attributes,
    const uint8_t *data, size_t data_length, uint8_t *key_buffer,
    size_t key_buffer_size, size_t *key_buffer_length,  size_t *bits);

/*!
 * \brief Generate a random key
 *
 * \param[in]  attributes        Attributes of the key to use
 * \param[out] key_buffer        Buffer to hold the generated key
 * \param[in]  key_buffer_size   Size in bytes of the key_buffer buffer
 * \param[out] key_buffer_length Size in bytes of the generated key
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_generate_key(const psa_key_attributes_t *attributes,
                                          uint8_t *key_buffer, size_t key_buffer_size,
                                          size_t *key_buffer_length);

/*!
 * \brief Destroy a random key
 *
 * \param[in]  attributes        Attributes of the key to destroy
 * \param[out] key_buffer        Buffer for the key
 * \param[in]  key_buffer_size   Size in bytes of the key_buffer buffer

 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_destroy_key(const psa_key_attributes_t *attributes,
                                         uint8_t *key_buffer, size_t key_buffer_size);

/*!
 * \brief Export the public key from a private key.
 *
 * \param[in]  attributes      Attributes of the key to use
 * \param[in]  key_buffer      Buffer to hold the generated key
 * \param[in]  key_buffer_size Size in bytes of the key_buffer buffer
 * \param[out] data            Buffer to hold the extracted public key
 * \param[in]  data_size       Size in bytes of the data buffer
 * \param[out] data_length     Size in bytes of the extracted public key
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
psa_status_t els_pkc_opaque_export_public_key(const psa_key_attributes_t *attributes,
                                               const uint8_t *key_buffer,
                                               size_t key_buffer_size, uint8_t *data,
                                               size_t data_size, size_t *data_length);

/*!
 * \brief Export the key from a private key.
 *
 * \param[in]  attributes      Attributes of the key to use
 * \param[in]  key_buffer      Buffer to hold the generated key
 * \param[in]  key_buffer_size Size in bytes of the key_buffer buffer
 * \param[out] data            Buffer to hold the extracted public key
 * \param[in]  data_size       Size in bytes of the data buffer
 * \param[out] data_length     Size in bytes of the extracted public key
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
 psa_status_t els_pkc_opaque_export_key(const psa_key_attributes_t *attributes,
                                               const uint8_t *key_buffer,
                                               size_t key_buffer_size, uint8_t *data,
                                               size_t data_size, size_t *data_length);

/*!
 * \brief Return the buffer size required by driver for storing key.
 *
 * \param[in] attributes defines the attributes associated with the input buffer
 * \param[in] data includes the input buffer as passed to the psa import function
 * \retval key_buffer_length is the required number of bytes required as 
 *         key_buffer. size_t on success. 0 on failure
 */
size_t els_pkc_opaque_size_function(const psa_key_attributes_t *attributes,
    const uint8_t *data,size_t data_length);

/*!
 * \brief Return the buffer size required by driver for storing key.
 *
 * \param[in]  attributes defines the attributes associated with the key
 * \retval key_buffer_length is the required number of bytes required as 
 *         key_buffer. size_t on success. 0 on failure
 */
size_t els_pkc_opaque_size_function_key_buff_size(const psa_key_attributes_t *attributes);


psa_status_t els_pkc_opaque_get_builtin_key(psa_drv_slot_number_t slot_number,
    psa_key_attributes_t *attributes,
    uint8_t *key_buffer, size_t key_buffer_size, size_t *key_buffer_length);

/*!
 * \brief Perform a key agreement and return the raw shared secret.
 *
 * \warning The raw result of a key agreement algorithm such as finite-field
 * Diffie-Hellman or elliptic curve Diffie-Hellman has biases and should
 * not be used directly as key material. It should instead be passed as
 * input to a key derivation algorithm. To chain a key agreement with
 * a key derivation, use psa_key_derivation_key_agreement() and other
 * functions from the key derivation interface.
 *
 * \param[in]  attributes      Attributes of the key to use
 * \param[in]  key_buffer      Buffer to hold the generated key
 * \param[in]  key_buffer_size Size in bytes of the key_buffer buffer
 * \param alg                        The key agreement algorithm to compute
 *                                    (\c PSA_ALG_XXX value such that
 *                                    #PSA_ALG_IS_RAW_KEY_AGREEMENT(\p alg)
 *                                    is true).
 * \param[in] peer_key                Public key of the peer. It must be
 *                                    in the same format that psa_import_key()
 *                                    accepts. The standard formats for public
 *                                    keys are documented in the documentation
 *                                    of psa_export_public_key().
 * \param peer_key_length             Size of \p peer_key in bytes.
 * \param[out] shared_secret          Buffer where the decrypted message is to
 *                                    be written.
 * \param shared_secret_size          Size of the \c output buffer in bytes.
 * \param[out] shared_secret_length   On success, the number of bytes
 *                                    that make up the returned output.
 *
 * \retval  PSA_SUCCESS on success. Error code from \ref psa_status_t on
 *          failure
 */
 psa_status_t els_pkc_opaque_key_agreement( const psa_key_attributes_t *attributes,
    const uint8_t *key_buffer,
    size_t key_buffer_size,
    psa_algorithm_t alg,
    const uint8_t *peer_key,
    size_t peer_key_length,
    uint8_t *shared_secret,
    size_t shared_secret_size,
    size_t *shared_secret_length);
#ifdef __cplusplus
}
#endif
#endif /* MCUX_PSA_ELS_PKC_OPAQUE_KEY_GENERATION_H */
