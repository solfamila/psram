/*
 * FreeRTOS V202111.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file tls_freertos.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <string.h>

#include "lwip/netdb.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"

#include "core_pkcs11_config.h"

/* PKCS #11 includes. */
#include "core_pkcs11.h"

/* TLS transport header. */
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#include "using_mbedtls.h"

#include "pkcs11.h"
#include "core_pki_utils.h"


/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging related header files are required to be included in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define LIBRARY_LOG_NAME and  LIBRARY_LOG_LEVEL.
 * 3. Include the header file "logging_stack.h".
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Sockets. */
#ifndef LIBRARY_LOG_NAME
#define LIBRARY_LOG_NAME     "TlsTransport"
#endif
#ifndef LIBRARY_LOG_LEVEL
#define LIBRARY_LOG_LEVEL    LOG_ERROR
#endif

#include "logging.h"


/*-----------------------------------------------------------*/

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a high-level code.
 */
static const char * pNoHighLevelMbedTlsCodeStr = "<No-High-Level-Code>";

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a low-level code.
 */
static const char * pNoLowLevelMbedTlsCodeStr = "<No-Low-Level-Code>";

/**
 * @brief Utility for converting the high-level code in an mbedTLS error to string,
 * if the code-contains a high-level code; otherwise, using a default string.
 */
#define mbedtlsHighLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_high_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_high_level_strerr( mbedTlsCode ) : pNoHighLevelMbedTlsCodeStr

/**
 * @brief Utility for converting the level-level code in an mbedTLS error to string,
 * if the code-contains a level-level code; otherwise, using a default string.
 */
#define mbedtlsLowLevelCodeOrDefault( mbedTlsCode )       \
    ( mbedtls_low_level_strerr( mbedTlsCode ) != NULL ) ? \
    mbedtls_low_level_strerr( mbedTlsCode ) : pNoLowLevelMbedTlsCodeStr

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the mbed TLS structures in a network connection.
 *
 * @param[in] pSslContext The SSL context to initialize.
 */
static void sslContextInit( SSLContext_t * pSslContext );

/**
 * @brief Free the mbed TLS structures in a network connection.
 *
 * @param[in] pSslContext The SSL context to free.
 */
static void sslContextFree( SSLContext_t * pSslContext );

/*-----------------------------------------------------------*/

/**
 * @brief Callback that wraps PKCS#11 using secure element for random number generation.
 *
 * @param[in] pvCtx Caller context.
 * @param[in] pucRandom Byte array to fill with random data.
 * @param[in] xRandomLength Length of byte array.
 *
 * @return Zero on success.
 */
static int generateRandomBytes( void * pvCtx,
                                unsigned char * pucRandom,
                                size_t xRandomLength );

/**
 * @brief Helper for reading the specified certificate object, if present,
 * out of storage, into RAM, and then into an mbedTLS certificate context
 * object.
 *
 * @param[in] pSslContext Caller TLS context.
 * @param[in] pcLabelName PKCS #11 certificate object label.
 * @param[in] xClass PKCS #11 certificate object class.
 * @param[out] pxCertificateContext Certificate context.
 *
 * @return Zero on success.
 */
static CK_RV readCertificateIntoContext( SSLContext_t * pSslContext,
                                         const char * pcLabelName,
                                         CK_OBJECT_CLASS xClass,
                                         mbedtls_x509_crt * pxCertificateContext );

/**
 * @brief Helper for setting up potentially hardware-based cryptographic context
 * for the client TLS certificate and private key.
 *
 * @param[in] Caller context.
 * @param[in] PKCS11 label which contains the desired private key.
 *
 * @return Zero on success.
 */
static CK_RV initializeClientKeys( SSLContext_t * pxCtx,
                                   const char * pcLabelName );

/**
 * @brief Sign a cryptographic hash with the private key.
 *
 * @param[in] pvContext Crypto context.
 * @param[in] xMdAlg Unused.
 * @param[in] pucHash Length in bytes of hash to be signed.
 * @param[in] uiHashLen Byte array of hash to be signed.
 * @param[out] pucSig RSA signature bytes.
 * @param[in] pxSigLen Length in bytes of signature buffer.
 * @param[in] piRng Unused.
 * @param[in] pvRng Unused.
 *
 * @return Zero on success.
 */
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
static int privateKeySigningCallback( mbedtls_pk_context * pvContext,
#else
static int privateKeySigningCallback( void * pvContext,
#endif
                                      mbedtls_md_type_t xMdAlg,
                                      const unsigned char * pucHash,
                                      size_t xHashLen,
                                      unsigned char * pucSig,
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
                                      size_t sigSize,
#endif
                                      size_t * pxSigLen,
                                      int ( * piRng )( void *,
                                                       unsigned char *,
                                                       size_t ),
                                      void * pvRng );

/**
 * @brief Setup TLS by initializing contexts and setting configurations.
 *
 * @param[in] pNetworkContext Network context.
 * @param[in] pHostName Remote host name, used for server name indication.
 * @param[in] pNetworkCredentials TLS setup parameters.
 *
 * @return #TLS_TRANSPORT_SUCCESS, #TLS_TRANSPORT_INSUFFICIENT_MEMORY, #TLS_TRANSPORT_INVALID_CREDENTIALS,
 * or #TLS_TRANSPORT_INTERNAL_ERROR.
 */
static TlsTransportStatus_t tlsSetup( NetworkContext_t * pNetworkContext,
                                      const char * pHostName,
                                      const NetworkCredentials_t * pNetworkCredentials );

static int lwip_socket_connect( const char * pHostName,
                                uint16_t port,
                                uint32_t receiveTimeoutMs,
                                uint32_t sendTimeoutMs,
                                Socket_t * pSocket );

/**
 * @brief Sends data over LWIP sockets.
 *
 * @param[in] ctx The network context containing the socket handle.
 * @param[in] buf Buffer containing the bytes to send.
 * @param[in] len Number of bytes to send from the buffer.
 *
 * @return Number of bytes sent on success; else a negative value.
 */
int mbedtls_bio_lwip_send( void * ctx,
                           const unsigned char * buf,
                           size_t len );


/**
 * @brief Receives data from LWIP socket.
 *
 * @param[in] ctx The network context containing the socket handle.
 * @param[out] buf Buffer to receive bytes into.
 * @param[in] len Number of bytes to receive from the network.
 *
 * @return Number of bytes received if successful; Negative value on error.
 */
int mbedtls_bio_lwip_recv( void * ctx,
                           unsigned char * buf,
                           size_t len );
/*-----------------------------------------------------------*/

#if defined(__arm__) || defined(__REDLIB__)
size_t strnlen(const char *s, size_t maxLength)
{
    size_t length = 0;
    while ((length <= maxLength) && (*s))
    {
        s++;
        length++;
    }
    return length;
}
#endif
/*-----------------------------------------------------------*/

static void sslContextInit( SSLContext_t * pSslContext )
{
    configASSERT( pSslContext != NULL );

    mbedtls_ssl_config_init( &( pSslContext->config ) );
    mbedtls_x509_crt_init( &( pSslContext->rootCa ) );
    mbedtls_x509_crt_init( &( pSslContext->clientCert ) );
    mbedtls_ssl_init( &( pSslContext->context ) );

    xInitializePkcs11Session( &( pSslContext->xP11Session ) );
    C_GetFunctionList( &( pSslContext->pxP11FunctionList ) );
}
/*-----------------------------------------------------------*/

static void sslContextFree( SSLContext_t * pSslContext )
{
    configASSERT( pSslContext != NULL );

    mbedtls_ssl_free( &( pSslContext->context ) );
    mbedtls_x509_crt_free( &( pSslContext->rootCa ) );
    mbedtls_x509_crt_free( &( pSslContext->clientCert ) );
    mbedtls_ssl_config_free( &( pSslContext->config ) );

    pSslContext->pxP11FunctionList->C_CloseSession( pSslContext->xP11Session );
}
/*-----------------------------------------------------------*/

static CK_RV readCertificateIntoContext( SSLContext_t * pSslContext,
                                         const char * pcLabelName,
                                         CK_OBJECT_CLASS xClass,
                                         mbedtls_x509_crt * pxCertificateContext )
{
    CK_RV xResult = CKR_OK;
    CK_ATTRIBUTE xTemplate = { 0 };
    CK_OBJECT_HANDLE xCertObj = 0;

    /* Get the handle of the certificate. */
    xResult = xFindObjectWithLabelAndClass( pSslContext->xP11Session,
                                            ( char * ) pcLabelName,
                                            strnlen( pcLabelName,
                                                     pkcs11configMAX_LABEL_LENGTH ),
                                            xClass,
                                            &xCertObj );

    if( ( CKR_OK == xResult ) && ( xCertObj == CK_INVALID_HANDLE ) )
    {
        xResult = CKR_OBJECT_HANDLE_INVALID;
    }

    /* Query the certificate size. */
    if( CKR_OK == xResult )
    {
        xTemplate.type = CKA_VALUE;
        xTemplate.ulValueLen = 0;
        xTemplate.pValue = NULL;
        xResult = pSslContext->pxP11FunctionList->C_GetAttributeValue( pSslContext->xP11Session,
                                                                       xCertObj,
                                                                       &xTemplate,
                                                                       1 );
    }

    /* Create a buffer for the certificate. */
    if( CKR_OK == xResult )
    {
        xTemplate.pValue = pvPortMalloc( xTemplate.ulValueLen );

        if( NULL == xTemplate.pValue )
        {
            xResult = CKR_HOST_MEMORY;
        }
    }

    /* Export the certificate. */
    if( CKR_OK == xResult )
    {
        xResult = pSslContext->pxP11FunctionList->C_GetAttributeValue( pSslContext->xP11Session,
                                                                       xCertObj,
                                                                       &xTemplate,
                                                                       1 );
    }

    /* Decode the certificate. */
    if( CKR_OK == xResult )
    {
        xResult = mbedtls_x509_crt_parse( pxCertificateContext,
                                          ( const unsigned char * ) xTemplate.pValue,
                                          xTemplate.ulValueLen );
    }

    /* Free memory. */
    vPortFree( xTemplate.pValue );

    return xResult;
}

/*-----------------------------------------------------------*/

int canDoStub( mbedtls_pk_type_t type )
{
    return 1;
}

/*----------------------------------------------------------*/

/**
 * @brief Helper for setting up potentially hardware-based cryptographic context
 * for the client TLS certificate and private key.
 *
 * @param[in] Caller context.
 * @param[in] PKCS11 label which contains the desired private key.
 *
 * @return Zero on success.
 */
static CK_RV initializeClientKeys( SSLContext_t * pxCtx,
                                   const char * pcLabelName )
{
    CK_RV xResult = CKR_OK;
    CK_SLOT_ID * pxSlotIds = NULL;
    CK_ULONG xCount = 0;
    CK_ATTRIBUTE xTemplate[ 2 ];
    mbedtls_pk_type_t xKeyAlgo = ( mbedtls_pk_type_t ) ~0;

    /* Get the PKCS #11 module/token slot count. */
    if( CKR_OK == xResult )
    {
        xResult = ( BaseType_t ) pxCtx->pxP11FunctionList->C_GetSlotList( CK_TRUE,
                                                                          NULL,
                                                                          &xCount );
    }

    /* Allocate memory to store the token slots. */
    if( CKR_OK == xResult )
    {
        pxSlotIds = ( CK_SLOT_ID * ) pvPortMalloc( sizeof( CK_SLOT_ID ) * xCount );

        if( NULL == pxSlotIds )
        {
            xResult = CKR_HOST_MEMORY;
        }
    }

    /* Get all of the available private key slot identities. */
    if( CKR_OK == xResult )
    {
        xResult = ( BaseType_t ) pxCtx->pxP11FunctionList->C_GetSlotList( CK_TRUE,
                                                                          pxSlotIds,
                                                                          &xCount );
    }

    /* Put the module in authenticated mode. */
    if( CKR_OK == xResult )
    {
        xResult = ( BaseType_t ) pxCtx->pxP11FunctionList->C_Login( pxCtx->xP11Session,
                                                                    CKU_USER,
                                                                    ( CK_UTF8CHAR_PTR ) configPKCS11_DEFAULT_USER_PIN,
                                                                    sizeof( configPKCS11_DEFAULT_USER_PIN ) - 1 );
    }

    if( CKR_OK == xResult )
    {
        /* Get the handle of the device private key. */
        xResult = xFindObjectWithLabelAndClass( pxCtx->xP11Session,
                                                ( char * ) pcLabelName,
                                                strnlen( pcLabelName,
                                                         pkcs11configMAX_LABEL_LENGTH ),
                                                CKO_PRIVATE_KEY,
                                                &pxCtx->xP11PrivateKey );
    }

    if( ( CKR_OK == xResult ) && ( pxCtx->xP11PrivateKey == CK_INVALID_HANDLE ) )
    {
        xResult = CK_INVALID_HANDLE;
        LogError( ( "Could not find private key." ) );
    }

    /* Query the device private key type. */
    if( xResult == CKR_OK )
    {
        xTemplate[ 0 ].type = CKA_KEY_TYPE;
        xTemplate[ 0 ].pValue = &pxCtx->xKeyType;
        xTemplate[ 0 ].ulValueLen = sizeof( CK_KEY_TYPE );
        xResult = pxCtx->pxP11FunctionList->C_GetAttributeValue( pxCtx->xP11Session,
                                                                 pxCtx->xP11PrivateKey,
                                                                 xTemplate,
                                                                 1 );
    }

    /* Map the PKCS #11 key type to an mbedTLS algorithm. */
    if( xResult == CKR_OK )
    {
        switch( pxCtx->xKeyType )
        {
            case CKK_RSA:
                xKeyAlgo = MBEDTLS_PK_RSA;
                break;

            case CKK_EC:
                xKeyAlgo = MBEDTLS_PK_ECKEY;
                break;

            default:
                xResult = CKR_ATTRIBUTE_VALUE_INVALID;
                break;
        }
    }

    /* Map the mbedTLS algorithm to its internal metadata. */
    if( xResult == CKR_OK )
    {
        memcpy( &pxCtx->privKeyInfo, mbedtls_pk_info_from_type( xKeyAlgo ), sizeof( mbedtls_pk_info_t ) );

        /* Assign unimplemented function pointers to NULL */
        pxCtx->privKeyInfo.get_bitlen = NULL;
        pxCtx->privKeyInfo.verify_func = NULL;
#if defined( MBEDTLS_ECDSA_C ) && defined( MBEDTLS_ECP_RESTARTABLE )
        pxCtx->privKeyInfo.verify_rs_func = NULL;
        pxCtx->privKeyInfo.sign_rs_func = NULL;
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */
        pxCtx->privKeyInfo.decrypt_func = NULL;
        pxCtx->privKeyInfo.encrypt_func = NULL;
        pxCtx->privKeyInfo.check_pair_func = NULL;
        pxCtx->privKeyInfo.ctx_alloc_func = NULL;
        pxCtx->privKeyInfo.ctx_free_func = NULL;
#if defined( MBEDTLS_ECDSA_C ) && defined( MBEDTLS_ECP_RESTARTABLE )
        pxCtx->privKeyInfo.rs_alloc_func = NULL;
        pxCtx->privKeyInfo.rs_free_func = NULL;
#endif /* MBEDTLS_ECDSA_C && MBEDTLS_ECP_RESTARTABLE */
        pxCtx->privKeyInfo.debug_func = NULL;

        pxCtx->privKeyInfo.sign_func = privateKeySigningCallback;
        pxCtx->privKey.pk_info = &pxCtx->privKeyInfo;
        pxCtx->privKey.pk_ctx = pxCtx;
    }

    /* Free memory. */
    vPortFree( pxSlotIds );

    return xResult;
}

/*-----------------------------------------------------------*/

#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
static int privateKeySigningCallback( mbedtls_pk_context * pvContext,
#else
static int privateKeySigningCallback( void * pvContext,
#endif
                                      mbedtls_md_type_t xMdAlg,
                                      const unsigned char * pucHash,
                                      size_t xHashLen,
                                      unsigned char * pucSig,
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
                                      size_t sigSize,
#endif
                                      size_t * pxSigLen,
                                      int ( * piRng )( void *,
                                                       unsigned char *,
                                                       size_t ),
                                      void * pvRng )
{
    CK_RV xResult = CKR_OK;
    int iFinalResult = 0;
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    SSLContext_t * pxTLSContext = ( SSLContext_t * ) pvContext->pk_ctx;
#else
    SSLContext_t * pxTLSContext = ( SSLContext_t * ) pvContext;
#endif
    CK_MECHANISM xMech = { 0 };
    CK_BYTE xToBeSigned[ 256 ];
    CK_ULONG xToBeSignedLen = sizeof( xToBeSigned );

    /* Unreferenced parameters. */
    ( void ) ( piRng );
    ( void ) ( pvRng );
    ( void ) ( xMdAlg );

    /* Sanity check buffer length. */
    if( xHashLen > sizeof( xToBeSigned ) )
    {
        xResult = CKR_ARGUMENTS_BAD;
    }

    /* Format the hash data to be signed. */
    if( CKK_RSA == pxTLSContext->xKeyType )
    {
        xMech.mechanism = CKM_RSA_PKCS;

        /* mbedTLS expects hashed data without padding, but PKCS #11 C_Sign function performs a hash
         * & sign if hash algorithm is specified.  This helper function applies padding
         * indicating data was hashed with SHA-256 while still allowing pre-hashed data to
         * be provided. */
        xResult = vAppendSHA256AlgorithmIdentifierSequence( ( uint8_t * ) pucHash, xToBeSigned );
        xToBeSignedLen = pkcs11RSA_SIGNATURE_INPUT_LENGTH;
    }
    else if( CKK_EC == pxTLSContext->xKeyType )
    {
        xMech.mechanism = CKM_ECDSA;
        memcpy( xToBeSigned, pucHash, xHashLen );
        xToBeSignedLen = xHashLen;
    }
    else
    {
        xResult = CKR_ARGUMENTS_BAD;
    }

    if( CKR_OK == xResult )
    {
        /* Use the PKCS#11 module to sign. */
        xResult = pxTLSContext->pxP11FunctionList->C_SignInit( pxTLSContext->xP11Session,
                                                               &xMech,
                                                               pxTLSContext->xP11PrivateKey );
    }

    if( CKR_OK == xResult )
    {
        *pxSigLen = sizeof( xToBeSigned );
        xResult = pxTLSContext->pxP11FunctionList->C_Sign( ( CK_SESSION_HANDLE ) pxTLSContext->xP11Session,
                                                           xToBeSigned,
                                                           xToBeSignedLen,
                                                           pucSig,
                                                           ( CK_ULONG_PTR ) pxSigLen );
    }

    if( ( xResult == CKR_OK ) && ( CKK_EC == pxTLSContext->xKeyType ) )
    {
        /* PKCS #11 for P256 returns a 64-byte signature with 32 bytes for R and 32 bytes for S.
         * This must be converted to an ASN.1 encoded array. */
        if( *pxSigLen != pkcs11ECDSA_P256_SIGNATURE_LENGTH )
        {
            xResult = CKR_FUNCTION_FAILED;
        }

        if( xResult == CKR_OK )
        {
            PKI_pkcs11SignatureTombedTLSSignature( pucSig, pxSigLen );
        }
    }

    if( xResult != CKR_OK )
    {
        LogError( ( "Failed to sign message using PKCS #11 with error code %02X.", xResult ) );
        iFinalResult = -1;
    }

    return iFinalResult;
}


/*-----------------------------------------------------------*/

static int generateRandomBytes( void * pvCtx,
                                unsigned char * pucRandom,
                                size_t xRandomLength )
{
    /* Must cast from void pointer to conform to mbed TLS API. */
    SSLContext_t * pxCtx = ( SSLContext_t * ) pvCtx;
    CK_RV xResult;

    xResult = pxCtx->pxP11FunctionList->C_GenerateRandom( pxCtx->xP11Session, pucRandom, xRandomLength );

    if( xResult != CKR_OK )
    {
        LogError( ( "Failed to generate random bytes from the PKCS #11 module." ) );
    }

    return xResult;
}

/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsSetup( NetworkContext_t * pNetworkContext,
                                      const char * pHostName,
                                      const NetworkCredentials_t * pNetworkCredentials )
{
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;
    int32_t mbedtlsError = 0;
    CK_RV xResult = CKR_OK;

    configASSERT( pNetworkContext != NULL );
    configASSERT( pHostName != NULL );
    configASSERT( pNetworkCredentials != NULL );
    configASSERT( pNetworkCredentials->pRootCa != NULL );
    configASSERT( pNetworkCredentials->pClientCertLabel != NULL );
    configASSERT( pNetworkCredentials->pPrivateKeyLabel != NULL );

    /* Initialize the mbed TLS context structures. */
    sslContextInit( &( pNetworkContext->sslContext ) );

    mbedtlsError = mbedtls_ssl_config_defaults( &( pNetworkContext->sslContext.config ),
                                                MBEDTLS_SSL_IS_CLIENT,
                                                MBEDTLS_SSL_TRANSPORT_STREAM,
                                                MBEDTLS_SSL_PRESET_DEFAULT );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to set default SSL configuration: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

        /* Per mbed TLS docs, mbedtls_ssl_config_defaults only fails on memory allocation. */
        returnStatus = TLS_TRANSPORT_INSUFFICIENT_MEMORY;
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Set up the certificate security profile, starting from the default value. */
        pNetworkContext->sslContext.certProfile = mbedtls_x509_crt_profile_default;

        /* test.mosquitto.org only provides a 1024-bit RSA certificate, which is
         * not acceptable by the default mbed TLS certificate security profile.
         * For the purposes of this demo, allow the use of 1024-bit RSA certificates.
         * This block should be removed otherwise. */
        if( strncmp( pHostName, "test.mosquitto.org", strlen( pHostName ) ) == 0 )
        {
            pNetworkContext->sslContext.certProfile.rsa_min_bitlen = 1024;
        }

        /* Set SSL authmode and the RNG context. */
        mbedtls_ssl_conf_authmode( &( pNetworkContext->sslContext.config ),
                                   MBEDTLS_SSL_VERIFY_REQUIRED );
        mbedtls_ssl_conf_rng( &( pNetworkContext->sslContext.config ),
                              generateRandomBytes,
                              &pNetworkContext->sslContext );
        mbedtls_ssl_conf_cert_profile( &( pNetworkContext->sslContext.config ),
                                       &( pNetworkContext->sslContext.certProfile ) );

        /* Parse the server root CA certificate into the SSL context. */
        mbedtlsError = mbedtls_x509_crt_parse( &( pNetworkContext->sslContext.rootCa ),
                                               pNetworkCredentials->pRootCa,
                                               pNetworkCredentials->rootCaSize );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to parse server root CA certificate: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

            returnStatus = TLS_TRANSPORT_INVALID_CREDENTIALS;
        }
        else
        {
            mbedtls_ssl_conf_ca_chain( &( pNetworkContext->sslContext.config ),
                                       &( pNetworkContext->sslContext.rootCa ),
                                       NULL );
        }
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Setup the client private key. */
        xResult = initializeClientKeys( &( pNetworkContext->sslContext ),
                                        pNetworkCredentials->pPrivateKeyLabel );

        if( xResult != CKR_OK )
        {
            LogError( ( "Failed to setup key handling by PKCS #11." ) );

            returnStatus = TLS_TRANSPORT_INVALID_CREDENTIALS;
        }
        else
        {
            /* Setup the client certificate. */
            xResult = readCertificateIntoContext( &( pNetworkContext->sslContext ),
                                                  pNetworkCredentials->pClientCertLabel,
                                                  CKO_CERTIFICATE,
                                                  &( pNetworkContext->sslContext.clientCert ) );

            if( xResult != CKR_OK )
            {
                LogError( ( "Failed to get certificate from PKCS #11 module." ) );

                returnStatus = TLS_TRANSPORT_INVALID_CREDENTIALS;
            }
            else
            {
                ( void ) mbedtls_ssl_conf_own_cert( &( pNetworkContext->sslContext.config ),
                                                    &( pNetworkContext->sslContext.clientCert ),
                                                    &( pNetworkContext->sslContext.privKey ) );
            }
        }
    }

    if( ( returnStatus == TLS_TRANSPORT_SUCCESS ) && ( pNetworkCredentials->pAlpnProtos != NULL ) )
    {
        /* Include an application protocol list in the TLS ClientHello
         * message. */
        mbedtlsError = mbedtls_ssl_conf_alpn_protocols( &( pNetworkContext->sslContext.config ),
                                                        pNetworkCredentials->pAlpnProtos );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to configure ALPN protocol in mbed TLS: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

            returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
        }
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Initialize the mbed TLS secured connection context. */
        mbedtlsError = mbedtls_ssl_setup( &( pNetworkContext->sslContext.context ),
                                          &( pNetworkContext->sslContext.config ) );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to set up mbed TLS SSL context: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

            returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
        }
        else
        {
            /* Set the underlying IO for the TLS connection. */

            /* MISRA Rule 11.2 flags the following line for casting the second
             * parameter to void *. This rule is suppressed because
             * #mbedtls_ssl_set_bio requires the second parameter as void *.
             */
            /* coverity[misra_c_2012_rule_11_2_violation] */
            mbedtls_ssl_set_bio( &( pNetworkContext->sslContext.context ),
                                 ( void * ) pNetworkContext->tcpSocket,
                                 mbedtls_bio_lwip_send,
                                 mbedtls_bio_lwip_recv,
                                 NULL );
        }
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Enable SNI if requested. */
        if( pNetworkCredentials->disableSni == pdFALSE )
        {
            mbedtlsError = mbedtls_ssl_set_hostname( &( pNetworkContext->sslContext.context ),
                                                     pHostName );

            if( mbedtlsError != 0 )
            {
                LogError( ( "Failed to set server name: mbedTLSError= %s : %s.",
                            mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                            mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

                returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
            }
        }
    }

    /* Set Maximum Fragment Length if enabled. */
#ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Enable the max fragment extension. 4096 bytes is currently the largest fragment size permitted.
         * See RFC 8449 https://tools.ietf.org/html/rfc8449 for more information.
         *
         * Smaller values can be found in "mbedtls/include/ssl.h".
         */
        mbedtlsError = mbedtls_ssl_conf_max_frag_len( &( pNetworkContext->sslContext.config ), MBEDTLS_SSL_MAX_FRAG_LEN_4096 );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to maximum fragment length extension: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
            returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
        }
    }
#endif /* ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Perform the TLS handshake. */
        do
        {
            mbedtlsError = mbedtls_ssl_handshake( &( pNetworkContext->sslContext.context ) );
        } while( ( mbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
                 ( mbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to perform TLS handshake: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

            returnStatus = TLS_TRANSPORT_HANDSHAKE_FAILED;
        }
    }

    if( returnStatus != TLS_TRANSPORT_SUCCESS )
    {
        sslContextFree( &( pNetworkContext->sslContext ) );
    }
    else
    {
        LogInfo( ( "(Network connection %p) TLS handshake successful.",
                   pNetworkContext ) );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int lwip_socket_connect( const char * pHostName,
                                uint16_t port,
                                uint32_t receiveTimeoutMs,
                                uint32_t sendTimeoutMs,
                                Socket_t * pSocket )
{
    int socketStatus = 0;
    struct sockaddr_in serverAddr = { 0 };
    struct hostent * pDnsEntry = NULL;
    int socket = -1;

    socketStatus = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if( socketStatus >= 0 )
    {
        socket = socketStatus;
        socketStatus = 0;
    }
    else
    {
        LogError( ( "Failed to create TCP socket with error %d.", socketStatus ) );
    }

    if( socketStatus == 0 )
    {
        pDnsEntry = gethostbyname( pHostName );

        if( pDnsEntry != NULL )
        {
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons( port );
            memcpy( ( char * ) &serverAddr.sin_addr, ( char * ) ( pDnsEntry->h_addr_list[ 0 ] ), pDnsEntry->h_length );

            socketStatus = connect( socket, ( struct sockaddr * ) ( &serverAddr ), sizeof( serverAddr ) );

            if( socketStatus < 0 )
            {
                LogError( ( "Failed to establish TCP connection to %s, with error %d.", pHostName, socketStatus ) );
            }
        }
        else
        {
            LogError( ( "Failed to resolve IP address for host %s", pHostName ) );
            socketStatus = -1;
        }
    }

    if( socketStatus == 0 )
    {
        socketStatus = setsockopt( socket, SOL_SOCKET, SO_RCVTIMEO, &receiveTimeoutMs, sizeof( receiveTimeoutMs ) );
    }

    if( socketStatus == 0 )
    {
        socketStatus = setsockopt( socket, SOL_SOCKET, SO_SNDTIMEO, &sendTimeoutMs, sizeof( sendTimeoutMs ) );
    }

    if( socketStatus == 0 )
    {
        ( *pSocket ) = socket;
    }
    else
    {
        if( socket >= 0 )
        {
            closesocket( socket );
        }
    }

    return socketStatus;
}

/*-----------------------------------------------------------*/
int mbedtls_bio_lwip_send( void * ctx,
                           const unsigned char * buf,
                           size_t len )
{
    int sendStatus = 0;

    configASSERT( buf != NULL );
    sendStatus = send( ( Socket_t ) ctx, buf, len, 0 );

    return sendStatus;
}

/*-----------------------------------------------------------*/

int mbedtls_bio_lwip_recv( void * ctx,
                           unsigned char * buf,
                           size_t len )
{
    int recvStatus = 0;

    configASSERT( buf != NULL );
    recvStatus = recv( ( Socket_t ) ctx, buf, len, 0 );

    if( -1 == recvStatus )
    {
        /*
         * 1. EWOULDBLOCK if the socket is NON-blocking, but there is no data
         *    when recv is called.
         * 2. EAGAIN if the socket is blocking and have waited long enough but
         *    packet is not received.
         */
        if( ( errno == EWOULDBLOCK ) /*|| ( errno == EAGAIN )*/ )
        {
            recvStatus = MBEDTLS_ERR_SSL_WANT_READ; /* timeout or would block */
        }
        else
        {
            recvStatus = -errno;
        }
    }

    if( ( 0 == recvStatus ) && ( errno == ENOTCONN ) )
    {
        recvStatus = -ENOTCONN;
    }

    return recvStatus;
}

/*-----------------------------------------------------------*/
TlsTransportStatus_t TLS_FreeRTOS_Connect( NetworkContext_t * pNetworkContext,
                                           const char * pHostName,
                                           uint16_t port,
                                           const NetworkCredentials_t * pNetworkCredentials,
                                           uint32_t receiveTimeoutMs,
                                           uint32_t sendTimeoutMs )
{
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;

    if( ( pNetworkContext == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        LogError( ( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                    "pHostName=%p, pNetworkCredentials=%p.",
                    pNetworkContext,
                    pHostName,
                    pNetworkCredentials ) );
        returnStatus = TLS_TRANSPORT_INVALID_PARAMETER;
    }
    else if( ( pNetworkCredentials->pRootCa == NULL ) )
    {
        LogError( ( "pRootCa cannot be NULL." ) );
        returnStatus = TLS_TRANSPORT_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else for MISRA 15.7 compliance. */
    }

    /* Establish a TCP connection with the server. */
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        if( lwip_socket_connect( pHostName, port, receiveTimeoutMs, sendTimeoutMs, &pNetworkContext->tcpSocket ) < 0 )
        {
            returnStatus = TLS_TRANSPORT_CONNECT_FAILURE;
        }
        else
        {
            /* Empty else for MISRA 15.7 compliance. */
        }
    }

    /* Initialize TLS contexts and set credentials. */
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        returnStatus = tlsSetup( pNetworkContext, pHostName, pNetworkCredentials );
    }

#if 0
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        int opt = 1;

        if( lwip_ioctl( pNetworkContext->tcpSocket, FIONBIO, &opt ) != 0 )
        {
            returnStatus = TLS_TRANSPORT_CONNECT_FAILURE;
        }
    }
#endif

    /* Clean up on failure. */
    if( returnStatus != TLS_TRANSPORT_SUCCESS )
    {
        if( ( pNetworkContext != NULL ) && ( pNetworkContext->tcpSocket >= 0 ) )
        {
            ( void ) closesocket( pNetworkContext->tcpSocket );
        }
    }
    else
    {
        LogInfo( ( "(Network connection %p) Connection to %s established.",
                   pNetworkContext,
                   pHostName ) );
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

void TLS_FreeRTOS_Disconnect( NetworkContext_t * pNetworkContext )
{
    BaseType_t tlsStatus = 0;

    if( pNetworkContext != NULL )
    {
        /* Attempting to terminate TLS connection. */
        tlsStatus = ( BaseType_t ) mbedtls_ssl_close_notify( &( pNetworkContext->sslContext.context ) );

        /* Ignore the WANT_READ and WANT_WRITE return values. */
        if( ( tlsStatus != ( BaseType_t ) MBEDTLS_ERR_SSL_WANT_READ ) &&
            ( tlsStatus != ( BaseType_t ) MBEDTLS_ERR_SSL_WANT_WRITE ) )
        {
            if( tlsStatus == 0 )
            {
                LogInfo( ( "(Network connection %p) TLS close-notify sent.",
                           pNetworkContext ) );
            }
            else
            {
                LogError( ( "(Network connection %p) Failed to send TLS close-notify: mbedTLSError= %s : %s.",
                            pNetworkContext,
                            mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                            mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
            }
        }
        else
        {
            /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
            LogInfo( ( "(Network connection %p) TLS close-notify sent; received %s as the TLS status can be ignored for close-notify.",
                       pNetworkContext,
                       mbedtlsHighLevelCodeOrDefault( tlsStatus ) ) );
        }

        /* Close connection */
        closesocket( pNetworkContext->tcpSocket );

        /* Free mbed TLS contexts. */
        sslContextFree( &( pNetworkContext->sslContext ) );
    }
}
/*-----------------------------------------------------------*/

int32_t TLS_FreeRTOS_Recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t bytesToRecv )
{
    int32_t tlsStatus = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

    if( ( pNetworkContext != NULL ) && ( pBuffer != NULL ) && ( bytesToRecv > 0 ) )
    {
        tlsStatus = ( int32_t ) mbedtls_ssl_read( &( pNetworkContext->sslContext.context ),
                                                  pBuffer,
                                                  bytesToRecv );

        if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
            ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
            ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
        {
            LogDebug( ( "Failed to read data. However, a read can be retried on this error. "
                        "mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                        mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );

            /* Mark these set of errors as a timeout. The libraries may retry read
             * on these errors. */
            tlsStatus = 0;
        }
        else if( tlsStatus < 0 )
        {
            LogError( ( "Failed to read data: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                        mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
        }
        else
        {
            /* Empty else marker. */
        }
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t TLS_FreeRTOS_Send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t bytesToSend )
{
    int32_t tlsStatus = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

    if( ( pNetworkContext != NULL ) && ( pBuffer != NULL ) && ( bytesToSend > 0 ) )
    {
        tlsStatus = ( int32_t ) mbedtls_ssl_write( &( pNetworkContext->sslContext.context ),
                                                   pBuffer,
                                                   bytesToSend );

        if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
            ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
            ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
        {
            LogDebug( ( "Failed to send data. However, send can be retried on this error. "
                        "mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                        mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );

            /* Mark these set of errors as a timeout. The libraries may retry send
             * on these errors. */
            tlsStatus = 0;
        }
        else if( tlsStatus < 0 )
        {
            LogError( ( "Failed to send data:  mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                        mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
        }
        else
        {
            /* Empty else marker. */
        }
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/