// INDENTING (emacs/vi): -*- mode:c++; tab-width:2; c-basic-offset:2; intent-tabs-mode:nil; -*- ex: set tabstop=2 expandtab:

/*
 * Simple Geolocalization and Course Transmission Protocol (SGCTP)
 * Copyright (C) 2014 Cedric Dufour <http://cedric.dufour.name>
 *
 * The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 * free software:
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, Version 3.
 *
 * The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 */

// C
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef __SGCTP_USE_OPENSSL__

// OpenSSL
#include "openssl/evp.h"
#include "openssl/rand.h"

#else // __SGCTP_USE_OPENSSL__

// GCrypt
#include "gcrypt.h"

#endif // NOT __SGCTP_USE_OPENSSL__

// SGCTP
#include "sgctp/data.hpp"
#include "sgctp/parameters.hpp"
#include "sgctp/payload_aes128.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CONSTANTS / STATIC
//----------------------------------------------------------------------

#ifdef __SGCTP_USE_OPENSSL__
const EVP_CIPHER* CPayload_AES128::CRYPTO_CIPHER = EVP_aes_128_cbc();
#endif // __SGCTP_USE_OPENSSL__

#ifdef __SGCTP_USE_OPENSSL__

int CPayload_AES128::makeCryptoNonce( unsigned char *_pucNonce )
{
  // Create crypto nonce
  RAND_pseudo_bytes( _pucNonce, CRYPTO_NONCE_SIZE );

  // Done
  return 0;
}

int CPayload_AES128::initCryptoEngine()
{
  return 0;
};

#else // __SGCTP_USE_OPENSSL__

int CPayload_AES128::makeCryptoNonce( unsigned char *_pucNonce )
{
  int __iReturn;

  // Initialize crypto engine
  __iReturn = initCryptoEngine();
  if( __iReturn < 0 )
    return __iReturn;

  // Create crypto nonce
  gcry_create_nonce( _pucNonce, CRYPTO_NONCE_SIZE );

  // Done
  return 0;
}

int CPayload_AES128::initCryptoEngine()
{
  if( gcry_control( GCRYCTL_INITIALIZATION_FINISHED_P ) )
    return 0;
  if( !gcry_check_version( "1.5.0" ) )
    return -ENOSYS;
  gcry_control( GCRYCTL_SUSPEND_SECMEM_WARN );
  gcry_control( GCRYCTL_INIT_SECMEM, 16384, 0 );
  gcry_control( GCRYCTL_RESUME_SECMEM_WARN );
  gcry_control( GCRYCTL_INITIALIZATION_FINISHED, 0 );
  return 0;
}

#endif // NOT __SGCTP_USE_OPENSSL__


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CPayload_AES128::CPayload_AES128()
  : CPayload()
  , pucBufferTmp( NULL )
{}

CPayload_AES128::~CPayload_AES128()
{
  if( pucBufferTmp )
    freeBuffer( pucBufferTmp );
}


//----------------------------------------------------------------------
// METHODS: CPayload (implement/override)
//----------------------------------------------------------------------

int CPayload_AES128::alloc()
{
  if( !pucBufferTmp )
  {
    pucBufferTmp = allocBuffer();
    if( !pucBufferTmp )
      return -ENOMEM;
  }
  return 0;
}

int CPayload_AES128::serialize( unsigned char *_pucBuffer,
                                const CData &_roData )
{
  int __iReturn;

  // Check buffer
  if( !pucBufferTmp )
  {
    __iReturn = alloc();
    if( __iReturn < 0 )
      return __iReturn;
  }

  // Create crypto payload

  // ... serialize raw payload
  __iReturn = CPayload::serialize( pucBufferTmp, _roData );
  if( __iReturn < 0 )
    return __iReturn;
  int __iPayloadSize_RAW = __iReturn;

  // ... add seal
  memcpy( pucBufferTmp+__iPayloadSize_RAW, pucCryptoSeal, CRYPTO_SEAL_SIZE );
  __iPayloadSize_RAW += CRYPTO_SEAL_SIZE;

#ifndef __SGCTP_USE_OPENSSL__

  // ... PKCS#7 padding
  unsigned char __ucPadLength =
    CRYPTO_BLOCK_SIZE - __iPayloadSize_RAW % CRYPTO_BLOCK_SIZE;
  memset( pucBufferTmp+__iPayloadSize_RAW, __ucPadLength, __ucPadLength );
  __iPayloadSize_RAW += __ucPadLength;

#endif // NOT __SGCTP_USE_OPENSSL__

  // Encrypt
  int __iPayloadSize = 0;
  unsigned char __pucCryptoIV[CRYPTO_BLOCK_SIZE];

#ifdef __SGCTP_USE_OPENSSL__

  // ... IV
  RAND_pseudo_bytes( __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  memcpy( _pucBuffer, __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  __iPayloadSize += CRYPTO_BLOCK_SIZE;

  // ... cipher
  int __iLength;
  EVP_CIPHER_CTX __evpCipherCtx;
  EVP_CIPHER_CTX_init( &__evpCipherCtx );
  EVP_EncryptInit_ex( &__evpCipherCtx,
                      CRYPTO_CIPHER, NULL,
                      pucCryptoKey, __pucCryptoIV );
  EVP_EncryptUpdate( &__evpCipherCtx,
                     _pucBuffer+__iPayloadSize, &__iLength,
                     pucBufferTmp, __iPayloadSize_RAW );
  __iPayloadSize += __iLength;
  EVP_EncryptFinal_ex( &__evpCipherCtx,
                       _pucBuffer+__iPayloadSize, &__iLength );
  __iPayloadSize += __iLength;
  EVP_CIPHER_CTX_cleanup( &__evpCipherCtx );

#else // __SGCTP_USE_OPENSSL__

  // ... IV
  gcry_create_nonce( __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  memcpy( _pucBuffer, __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  __iPayloadSize += CRYPTO_BLOCK_SIZE;

  // ... cipher
  gcry_cipher_hd_t __gcryCipherHd;
  gcry_cipher_open( &__gcryCipherHd, CRYPTO_CIPHER, CRYPTO_MODE, 0 );
  gcry_cipher_setkey( __gcryCipherHd,
                      pucCryptoKey, CRYPTO_KEY_SIZE );
  gcry_cipher_setiv( __gcryCipherHd,
                     __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  gcry_cipher_encrypt( __gcryCipherHd,
                       _pucBuffer+__iPayloadSize, BUFFER_SIZE-__iPayloadSize,
                       pucBufferTmp, __iPayloadSize_RAW );
  __iPayloadSize += __iPayloadSize_RAW;
  gcry_cipher_close( __gcryCipherHd );

#endif // NOT __SGCTP_USE_OPENSSL__

  // Done
  return __iPayloadSize;
}

int CPayload_AES128::unserialize( CData *_poData,
                                  const unsigned char *_pucBuffer,
                                  uint16_t _ui16tBufferSize )
{
  int __iReturn;

  // Check buffer
  if( _ui16tBufferSize-CRYPTO_BLOCK_SIZE > BUFFER_SIZE )
    return -EOVERFLOW;
  if( !pucBufferTmp )
  {
    __iReturn = alloc();
    if( __iReturn < 0 )
      return __iReturn;
  }

  // Decrypt
  int __iPayloadSize = 0;
  unsigned char __pucCryptoIV[CRYPTO_BLOCK_SIZE];

  // ... IV
  memcpy( __pucCryptoIV, _pucBuffer, CRYPTO_BLOCK_SIZE );
  __iPayloadSize += CRYPTO_BLOCK_SIZE;

#ifdef __SGCTP_USE_OPENSSL__

  // ... cipher
  int __iLength;
  EVP_CIPHER_CTX __evpCipherCtx;
  EVP_CIPHER_CTX_init( &__evpCipherCtx );
  EVP_DecryptInit_ex( &__evpCipherCtx,
                      CRYPTO_CIPHER, NULL,
                      pucCryptoKey, __pucCryptoIV );
  EVP_DecryptUpdate( &__evpCipherCtx,
                     pucBufferTmp, &__iLength,
                     _pucBuffer+__iPayloadSize,
                     _ui16tBufferSize-CRYPTO_BLOCK_SIZE );
  __iPayloadSize += __iLength;
  EVP_DecryptFinal_ex( &__evpCipherCtx,
                       pucBufferTmp+__iPayloadSize, &__iLength );
  __iPayloadSize += __iLength;
  EVP_CIPHER_CTX_cleanup( &__evpCipherCtx );

#else // __SGCTP_USE_OPENSSL__

  // ... cipher
  gcry_cipher_hd_t __gcryCipherHd;
  gcry_cipher_open( &__gcryCipherHd, CRYPTO_CIPHER, CRYPTO_MODE, 0 );
  gcry_cipher_setkey( __gcryCipherHd,
                      pucCryptoKey, CRYPTO_KEY_SIZE );
  gcry_cipher_setiv( __gcryCipherHd,
                     __pucCryptoIV, CRYPTO_BLOCK_SIZE );
  gcry_cipher_decrypt( __gcryCipherHd,
                       pucBufferTmp, BUFFER_SIZE,
                       _pucBuffer+__iPayloadSize,
                       _ui16tBufferSize-CRYPTO_BLOCK_SIZE );
  __iPayloadSize += _ui16tBufferSize-CRYPTO_BLOCK_SIZE;
  gcry_cipher_close( __gcryCipherHd );

#endif // NOT __SGCTP_USE_OPENSSL__

  // Extract crypto payload
  int __iPayloadSize_RAW = __iPayloadSize-CRYPTO_BLOCK_SIZE;

#ifndef __SGCTP_USE_OPENSSL__

  // ... PKCS#7 padding
  unsigned char __ucPadLength = pucBufferTmp[__iPayloadSize_RAW-1];
  __iPayloadSize_RAW -= __ucPadLength;

#endif // __SGCTP_USE_OPENSSL__

  // ... check seal
  __iPayloadSize_RAW -= CRYPTO_SEAL_SIZE;
  __iReturn = memcmp( pucBufferTmp+__iPayloadSize_RAW,
                      pucCryptoSeal,
                      CRYPTO_SEAL_SIZE );
  if( __iReturn )
    return -EBADE;

  // ... unserialize raw payload
  __iReturn = CPayload::unserialize( _poData,
                                     pucBufferTmp,
                                     __iPayloadSize_RAW );
  if( __iReturn <= 0 )
    return __iReturn;

  // Done
  return __iPayloadSize;
}

void CPayload_AES128::free()
{
  if( pucBufferTmp )
    freeBuffer( pucBufferTmp );
  pucBufferTmp = NULL;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

int CPayload_AES128::makeCryptoKey( const unsigned char *_pucPassword,
                                    int _iPasswordLength,
                                    const unsigned char *_pucNonce )
{
  int __iReturn;

  // Initialize crypto engine
  __iReturn = initCryptoEngine();
  if( __iReturn < 0 )
    return __iReturn;

  // Save nonce
  memcpy( pucCryptoNonce,
          _pucNonce,
          CRYPTO_NONCE_SIZE );

  // Create crypto key and seal
  unsigned char __pucCryptoMaterial[CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE];

#ifdef __SGCTP_USE_OPENSSL__

  PKCS5_PBKDF2_HMAC_SHA1( _pucPassword, _iPasswordLength,
                          pucCryptoNonce, CRYPTO_NONCE_SIZE,
                          CRYPTO_KEY_ITER,
                          CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE,
                          __pucCryptoMaterial );

#else // __SGCTP_USE_OPENSSL__

  gcry_kdf_derive( _pucPassword, _iPasswordLength,
                   GCRY_KDF_PBKDF2, GCRY_MD_SHA1,
                   pucCryptoNonce, CRYPTO_NONCE_SIZE,
                   CRYPTO_KEY_ITER,
                   CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE,
                   __pucCryptoMaterial );

#endif // NOT __SGCTP_USE_OPENSSL__

  // Save key and seal
  memcpy( pucCryptoKey,
          __pucCryptoMaterial,
          CRYPTO_KEY_SIZE );
  memcpy( pucCryptoSeal,
          __pucCryptoMaterial+CRYPTO_KEY_SIZE,
          CRYPTO_SEAL_SIZE );

  // Done
  return 0;
}

int CPayload_AES128::incrCryptoKey()
{
  // Initialize crypto engine
  // -> let's skip this; it MUST have been done in makeCryptoKey

  // Create crypto key and seal
  unsigned char __pucCryptoMaterial[CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE];

#ifdef __SGCTP_USE_OPENSSL__

  PKCS5_PBKDF2_HMAC_SHA1( pucCryptoKey, CRYPTO_KEY_SIZE,
                          pucCryptoNonce, CRYPTO_NONCE_SIZE,
                          1,
                          CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE,
                          __pucCryptoMaterial );

#else // __SGCTP_USE_OPENSSL__

  gcry_kdf_derive( pucCryptoKey, CRYPTO_KEY_SIZE,
                   GCRY_KDF_PBKDF2, GCRY_MD_SHA1,
                   pucCryptoNonce, CRYPTO_NONCE_SIZE,
                   1,
                   CRYPTO_KEY_SIZE+CRYPTO_SEAL_SIZE,
                   __pucCryptoMaterial );

#endif // NOT __SGCTP_USE_OPENSSL__

  // Save key and seal
  memcpy( pucCryptoKey,
          __pucCryptoMaterial,
          CRYPTO_KEY_SIZE );
  memcpy( pucCryptoSeal,
          __pucCryptoMaterial+CRYPTO_KEY_SIZE,
          CRYPTO_SEAL_SIZE );

  // Done
  return 0;
}
