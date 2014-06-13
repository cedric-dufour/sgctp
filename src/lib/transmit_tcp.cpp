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
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

// SGCTP
#include "sgctp/data.hpp"
#include "sgctp/payload.hpp"
#include "sgctp/payload_aes128.hpp"
#include "sgctp/principal.hpp"
#include "sgctp/transmit_tcp.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// METHODS: CTransmit (implement/override)
//----------------------------------------------------------------------

void CTransmit_TCP::setTimeout( int _iSocket,
                                double _fdTimeout )
{
  // Set socket timeout
  struct timeval __tTimeval;
  double __fdSeconds;
  __tTimeval.tv_usec = 1000000 * (int)modf( _fdTimeout, &__fdSeconds );
  __tTimeval.tv_sec = (int)__fdSeconds;
  setsockopt( _iSocket,
              SOL_SOCKET, SO_RCVTIMEO,
              &__tTimeval, sizeof( __tTimeval ) );
  setsockopt( _iSocket,
              SOL_SOCKET, SO_SNDTIMEO,
              &__tTimeval, sizeof( __tTimeval ) );

  // Set transmission timeout
  CTransmit::setTimeout( _iSocket,
                         _fdTimeout );
}

int CTransmit_TCP::send( int _iSocket,
                         const void *_pBuffer,
                         int _iSize,
                         int _iFlags )
{
  int __iReturn = ::send( _iSocket, _pBuffer, _iSize, _iFlags );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_TCP::recv( int _iSocket,
                         void *_pBuffer,
                         int _iSize,
                         int _iFlags )
{
  int __iReturn = ::recv( _iSocket, _pBuffer, _iSize, _iFlags );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_TCP::alloc()
{
  return CTransmit::allocBuffer();
}

int CTransmit_TCP::serialize( int _iSocket,
                              const CData &_roData )
{
  int __iReturn;
  int __iExit;

  // Standard serialization
  __iReturn = CTransmit::serialize( _iSocket, _roData );
  if( __iReturn <= 0 )
    return __iReturn;
  __iExit = __iReturn;

  // Cryptographic key incrementation
  switch( ePayloadType )
  {

  case PAYLOAD_AES128:
    __iReturn = ((CPayload_AES128*)poPayload)->incrCryptoKey();
    if( __iReturn )
      return __iReturn;
    break;

  default:;

  }

  // Done
  return __iExit;
}

int CTransmit_TCP::unserialize( int _iSocket,
                                CData *_poData,
                                int _iMaxSize )
{
  int __iReturn;
  int __iExit;

  // Standard unserialization
  __iReturn = CTransmit::unserialize( _iSocket, _poData, _iMaxSize );
  if( __iReturn <= 0 )
    return __iReturn;
  __iExit = __iReturn;

  // Cryptographic key incrementation
  switch( ePayloadType )
  {

  case PAYLOAD_AES128:
    __iReturn = ((CPayload_AES128*)poPayload)->incrCryptoKey();
    if( __iReturn )
      return __iReturn;
    break;

  default:;

  }

  // Done
  return __iExit;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

/* Cryptographic handshake and data exchange:
 *
 * CLIENT            <->          SERVER
 *    setTimeout                           // prevent TCP-sockets-exhaustion DoS
 *  --->         ID, NonceC        --->
 *    Key0 = key( password(ID), NonceC )   // prevent rainbow-table attacks
 *  <---    crypt( NonceS, Key0 )  <---
 *    Key = key( NonceS, NonceC )          // prevent (out-of-band) replay attacks
 *  --->    crypt( "#AUTH", Key )   --->
 *    Key = key'()                         // prevent (in-band) replay attacks
 *  <---    crypt( "#OK", key )     <---
 *    Key = key'()
 *    unsetTimeout
 *  <-->   ... data exchange ...    <-->
 *    Key = key'()
 */

int CTransmit_TCP::sendHandshake( int _iSocket )
{
  int __iReturn;

  // Check resources
  if( !poPayload )
    return -ENODATA;
  if( !pucBuffer )
  {
    __iReturn = alloc();
    if( __iReturn < 0 )
      return __iReturn;
  }

  // Set socket timeout
  setTimeout( _iSocket, 3.0 ); // prevent DoS

  // Error-catching block
  int __iError = 1;
  int __iPayloadSize = 0;
  do
  {
    CData __oData;

    // Lookup principal
    if( pcPrincipalsPath )
    {
      __iReturn = usePrincipal()->read( pcPrincipalsPath, oPrincipal.getID() );
      if( __iReturn )
      {
        __iError = -EACCES;
        break;
      }
    }

    // Create handshake

    // ... signature
    memcpy( pucBuffer, "SGCTP", 5 );
    __iPayloadSize += 5;

    // ... protocol version
    *((uint8_t*)(pucBuffer+__iPayloadSize)) = PROTOCOL_VERSION;
    __iPayloadSize += 1;

    // ... payload type
    *((uint8_t*)(pucBuffer+__iPayloadSize)) = (uint8_t)ePayloadType;
    __iPayloadSize += 1;

    // ... principal ID
    *((uint32_t*)(pucBuffer+__iPayloadSize)) =
      htonl( (uint32_t)( oPrincipal.getID() >> 32 ) );
    __iPayloadSize += 4;
    *((uint32_t*)(pucBuffer+__iPayloadSize)) =
      htonl( (uint32_t)oPrincipal.getID() );
    __iPayloadSize += 4;

    // Send handshake
    __iReturn = send( _iSocket, pucBuffer, __iPayloadSize, MSG_EOR );
    if( __iReturn != __iPayloadSize )
    {
      __iError =
        ( __iReturn <= 0 )
        ? __iReturn
        : -EPROTO;
        break;
    }

    // Cryptographic session establishment
    bool __bSendAuthPayload = false;
    switch( ePayloadType )
    {

    case PAYLOAD_AES128:
      {
        CPayload_AES128 *__poPayload_AES128 = (CPayload_AES128*)poPayload;

        // ... send client nonce
        unsigned char __pucNonceClient[CPayload_AES128::CRYPTO_NONCE_SIZE];
        __iReturn = CPayload_AES128::makeCryptoNonce( __pucNonceClient );
        if( __iReturn )
        {
          __iError = __iReturn;
          break;
        }
        __iReturn = send( _iSocket,
                          __pucNonceClient,
                          CPayload_AES128::CRYPTO_NONCE_SIZE,
                          MSG_EOR );
        if( __iReturn != CPayload_AES128::CRYPTO_NONCE_SIZE )
        {
          __iError =
            ( __iReturn <= 0 )
            ? __iReturn
            : -EPROTO;
          break;
        }
        __poPayload_AES128->makeCryptoKey( (unsigned char*)oPrincipal.getPassword(),
                                           strlen( oPrincipal.getPassword() ),
                                           __pucNonceClient );

        // ... receive (decrypted) server nonce (session token)
        resetBuffer();
        __oData.reset();
        __iReturn = unserialize( _iSocket, &__oData, 48 );
        if( __iReturn < 0 )
        {
          __iError = __iReturn;
          break;
        }
        if( __oData.getDataSize() != CPayload_AES128::CRYPTO_NONCE_SIZE )
        {
          __iError = -EPROTO;
          break;
        }
        __poPayload_AES128->makeCryptoKey( __oData.getData(),
                                           CPayload_AES128::CRYPTO_NONCE_SIZE,
                                           __pucNonceClient );

        __bSendAuthPayload = true;
      }
      break;

    default:;
    }
    if( pcPrincipalsPath )
      usePrincipal()->erasePassword(); // clear password from memory
    if( __iError <= 0 )
      break;

    // Send AUTH payload
    if( __bSendAuthPayload )
    {
      __oData.reset();
      __oData.setID( "#AUTH" );
      __iReturn = serialize( _iSocket, __oData );
      if( __iReturn <= 0 )
      {
        __iError = __iReturn;
        break;
      }
    }

    // Receive OK payload
    resetBuffer();
    __oData.reset();
    __iReturn = unserialize( _iSocket, &__oData, 32 );
    if( __iReturn < 0 )
    {
      __iError = __iReturn;
      break;
    }
    if( strcmp( __oData.getID(), "#OK" ) )
    {
      __iError = -EPROTO;
      break;
    }

  }
  while( false ); // Error-catching block

  // Unset socket timeout
  setTimeout( _iSocket, 0.0 );

  // Reset buffer (for reception)
  resetBuffer();

  // Done
  return
    ( __iError <= 0 )
    ? __iError
    : __iPayloadSize;
}

int CTransmit_TCP::recvHandshake( int _iSocket )
{
  int __iReturn;

  // Check resources
  if( !pucBuffer )
  {
    __iReturn = alloc();
    if( __iReturn < 0 )
      return __iReturn;
  }

  // Set socket timeout
  setTimeout( _iSocket, 3.0 ); // prevent DoS

  // Error-catching block
  int __iError = 1;
  int __iPayloadSize = 0;
  do
  {
    CData __oData;

    // Receive handshake
    resetBuffer();
    __iReturn = recvBuffer( _iSocket, 15 );
    if( __iReturn != 15 )
    {
       __iError =
         ( __iReturn <= 0 )
         ? __iReturn
         : -EPROTO;
       break;
    }

    // Parse handshake
    const unsigned char *__pucHandshakeBuffer = pullBuffer( 15 );

    // ... signature
    if( memcmp( __pucHandshakeBuffer, "SGCTP", 5 ) )
    {
      __iError = -EPROTO;
      break;
    }
    __iPayloadSize += 5;

    // ... protocol version
    uint8_t __ui8tProtocolVersion =
      *((uint8_t*)(__pucHandshakeBuffer+__iPayloadSize));
    if( __ui8tProtocolVersion != PROTOCOL_VERSION )
    {
      __iError = -EPROTO;
      break;
    }
    __iPayloadSize += 1;

    // ... payload type
    uint8_t __ui8tPayloadType =
      *((uint8_t*)(__pucHandshakeBuffer+__iPayloadSize));
    __iPayloadSize += 1;

    // ... principal ID
    uint64_t __ui64tPrincipalID =
      ntohl( *((uint32_t*)(__pucHandshakeBuffer+__iPayloadSize)) );
    __iPayloadSize += 4;
    __ui64tPrincipalID |=
      ntohl( *((uint32_t*)(__pucHandshakeBuffer+__iPayloadSize)) );
    __iPayloadSize += 4;

    // Lookup principal
    if( pcPrincipalsPath )
    {
      __iReturn = usePrincipal()->read( pcPrincipalsPath, __ui64tPrincipalID );
      if( __iReturn )
      {
        __iError = -EACCES;
        break;
      }
    }

    // Initialize payload
    __iReturn = initPayload( __ui8tPayloadType );
    if( __iReturn < 0 )
    {
      __iError = __iReturn;
      break;
    }

    // Cryptographic session establishment
    bool __bRecvAuthPayload = false;
    switch( __ui8tPayloadType )
    {

    case PAYLOAD_AES128:
      {
        CPayload_AES128 *__poPayload_AES128 = (CPayload_AES128*)poPayload;

        // ... receive client nonce
        unsigned char __pucNonceClient[CPayload_AES128::CRYPTO_NONCE_SIZE];
        __iReturn = recvBuffer( _iSocket, CPayload_AES128::CRYPTO_NONCE_SIZE );
        if( __iReturn != CPayload_AES128::CRYPTO_NONCE_SIZE )
        {
          __iError =
            ( __iReturn <= 0 )
            ? __iReturn
            : -EPROTO;
          break;
        }
        memcpy( __pucNonceClient,
                pullBuffer( CPayload_AES128::CRYPTO_NONCE_SIZE ),
                CPayload_AES128::CRYPTO_NONCE_SIZE );
        __poPayload_AES128->makeCryptoKey( (unsigned char*)oPrincipal.getPassword(),
                                           strlen( oPrincipal.getPassword() ),
                                           __pucNonceClient );

        // ... send (encrypted) server nonce (session token)
        unsigned char __pucNonceServer[CPayload_AES128::CRYPTO_NONCE_SIZE];
        __iReturn = CPayload_AES128::makeCryptoNonce( __pucNonceServer );
        if( __iReturn )
        {
          __iError = __iReturn;
          break;
        }
        __oData.reset();
        __oData.setData( __pucNonceServer, CPayload_AES128::CRYPTO_NONCE_SIZE );
        __iReturn = serialize( _iSocket, __oData );
        if( __iReturn <= 0 )
        {
          __iError = __iReturn;
          break;
        }
        __poPayload_AES128->makeCryptoKey( __pucNonceServer,
                                           CPayload_AES128::CRYPTO_NONCE_SIZE,
                                           __pucNonceClient );

        // ... clean-up
        memset( __pucNonceServer, 0, CPayload_AES128::CRYPTO_NONCE_SIZE );

        __bRecvAuthPayload = true;
      }
      break;

    default:;
    }
    if( pcPrincipalsPath )
      usePrincipal()->erasePassword(); // clear password from memory
    if( __iError <= 0 )
      break;

    // Receive AUTH payload
    resetBuffer();
    if( __bRecvAuthPayload )
    {
      __oData.reset();
      __iReturn = unserialize( _iSocket, &__oData, 32 );
      if( __iReturn < 0 || strcmp( __oData.getID(), "#AUTH" ) )
      {
        __oData.reset();
        __oData.setID( "#KO" );
        serialize( _iSocket, __oData );
        __iError =
          ( __iReturn <= 0 )
          ? __iReturn
          : -EPROTO;
        break;
      }
    }

    // Send OK payload
    {
      __oData.reset();
      __oData.setID( "#OK" );
      __iReturn = serialize( _iSocket, __oData );
      if( __iReturn <= 0 )
      {
        __iError = __iReturn;
        break;
      }
    }

  }
  while( false ); // Error-catching block

  // Unset socket timeout
  setTimeout( _iSocket, 0.0 );

  // Reset buffer (for reception)
  resetBuffer();

  // Done
  return
    ( __iError <= 0 )
    ? __iError
    : __iPayloadSize;
}
