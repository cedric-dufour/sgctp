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

#ifndef SGCTP_CPAYLOAD_AES128_HPP
#define SGCTP_CPAYLOAD_AES128_HPP

#ifdef __SGCTP_USE_OPENSSL__

// OpenSSL
#include "openssl/evp.h"

#else // __SGCTP_USE_OPENSSL__

// GCrypt
#include "gcrypt.h"

#endif // NOT __SGCTP_USE_OPENSSL__

// SGCTP
#include "sgctp/payload.hpp"


// SGCTP namespace
namespace SGCTP
{

  /// AES128-encrypted SGCTP payload
  /**
   * This class (un-)serializes SGCTP data from/to an AES128-encrypted
   * SGCTP payload.
   */
  class CPayload_AES128: public CPayload
  {

    //----------------------------------------------------------------------
    // CONSTANTS / STATIC
    //----------------------------------------------------------------------

  public:
#ifdef __SGCTP_USE_OPENSSL__
    static const EVP_CIPHER* CRYPTO_CIPHER;
#else //__SGCTP_USE_OPENSSL__
    static const int CRYPTO_CIPHER = GCRY_CIPHER_AES128;
    static const int CRYPTO_MODE = GCRY_CIPHER_MODE_CBC;
#endif //__SGCTP_USE_OPENSSL__
    static const uint16_t CRYPTO_BLOCK_SIZE = 16;
    static const uint16_t CRYPTO_NONCE_SIZE = 16;
    static const uint16_t CRYPTO_KEY_SIZE = 16;
    static const uint16_t CRYPTO_KEY_ITER = 16384;
    static const uint16_t CRYPTO_SEAL_SIZE = 4;

  public:
    /// Create cryptographic nonce
    /**
     *  @param[in] _pucNonce Nonce buffer (to store nonce into); it MUST have been allocated previously
     *  @return Negative error code in case of error, zero otherwise
     *  @see CRYPTO_NONCE_SIZE
     */
    static int makeCryptoNonce( unsigned char *_pucNonce );

  private:
    /// Initialize cryptographic engine
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    static int initCryptoEngine();

    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  private:
    /// Payload temporary import/export buffer
    unsigned char *pucBufferTmp;
    /// Cryptographic nonce (used for cryptographic hashing)
    unsigned char pucCryptoNonce[CRYPTO_NONCE_SIZE];
    /// Cryptographic key (used for encryption/decryption)
    unsigned char pucCryptoKey[CRYPTO_BLOCK_SIZE];
    /// Cryptographic seal (used to check valid decryption)
    unsigned char pucCryptoSeal[CRYPTO_SEAL_SIZE];


    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CPayload_AES128();
    virtual ~CPayload_AES128();


    //----------------------------------------------------------------------
    // METHODS: CPayload (implement/override)
    //----------------------------------------------------------------------

  public:
    virtual int alloc();

    virtual int serialize( unsigned char *_pucBuffer,
                           const CData &_roData );

    virtual int unserialize( CData *_poData,
                             const unsigned char *_pucBuffer,
                             uint16_t _ui16tBufferSize );

    virtual void free();


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

  public:
    /// Create cryptographic key (and seal)
    /**
     *  @param[in] _pucPassword User password
     *  @param[in] _iPasswordLength User password length
     *  @param[in] _pucNonce Nonce (salt) used for password hashing
     *  @return Negative error code in case of error, zero otherwise
     *  @see makeCryptoNonce()
     */
    int makeCryptoKey( const unsigned char *_pucPassword,
                       int _iPasswordLength,
                       const unsigned char *_pucNonce );

    /// Increment cryptographic key (and seal)
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    int incrCryptoKey();

  };

}

#endif // SGCTP_CPAYLOAD_AES128_HPP
