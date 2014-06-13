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
#include <arpa/inet.h>
#include <sys/socket.h>

// SGCTP
#include "sgctp/data.hpp"
#include "sgctp/payload.hpp"
#include "sgctp/payload_aes128.hpp"
#include "sgctp/principal.hpp"
#include "sgctp/transmit.hpp"
#include "sgctp/transmit_file.hpp"
#include "sgctp/transmit_udp.hpp"
#include "sgctp/transmit_tcp.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CTransmit::CTransmit()
  : CParameters()
  , pucBuffer( NULL )
  , iBufferSize( 0 )
  , iBufferDataStart( 0 )
  , iBufferDataEnd( 0 )
  , fdTimeout( 0.0 )
  , poPayload( NULL )
  , ePayloadType( PAYLOAD_UNDEFINED )
{}

CTransmit::~CTransmit()
{
  if( pucBuffer )
    CPayload::freeBuffer( pucBuffer );
  if( poPayload )
    delete poPayload;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

int CTransmit::allocBuffer()
{
  if( !pucBuffer )
  {
    pucBuffer = CPayload::allocBuffer();
    if( !pucBuffer )
      return -ENOMEM;
    iBufferSize = CPayload::BUFFER_SIZE;
    iBufferDataStart = 0;
    iBufferDataEnd = 0;
  }
  return 0;
}

int CTransmit::recvBuffer( int _iDescriptor,
                           int _iSize )
{
  int __iReturn;

  // Check available data
  if( iBufferDataEnd - iBufferDataStart >= _iSize )
    return _iSize;

  // Check available space
  if( _iSize > iBufferSize )
    return -EOVERFLOW;
  if( iBufferSize - iBufferDataEnd < _iSize )
    flushBuffer();

  // Receive data
  double __fdTimeoutStart =
    ( fdTimeout > 0.0 )
    ? CData::epoch()
    : 0.0;
  do
  {
    if( fdTimeout > 0.0 )
    {
      if( CData::epoch() - __fdTimeoutStart > fdTimeout )
        return -ETIMEDOUT;
    }
    __iReturn = recv( _iDescriptor,
                      pucBuffer+iBufferDataEnd,
                      iBufferSize-iBufferDataEnd,
                      0 );
    if( __iReturn <= 0 )
      return __iReturn;
    iBufferDataEnd += __iReturn;
  }
  while( iBufferDataEnd-iBufferDataStart < _iSize );

  // Done
  return _iSize;
}

const unsigned char* CTransmit::pullBuffer( int _iSize )
{
  iBufferDataStart += _iSize;
  return pucBuffer+(iBufferDataStart-_iSize);
}

void CTransmit::flushBuffer()
{
  // Non-overlapping memcpy
  int __iBufferLength = iBufferDataEnd - iBufferDataStart;
  int __iBufferCopied = 0;
  while( __iBufferCopied < __iBufferLength )
  {
    int __iBufferCopyLength = __iBufferLength - __iBufferCopied;
    if( __iBufferCopyLength > iBufferDataStart )
      __iBufferCopyLength = iBufferDataStart;
    memcpy( pucBuffer + __iBufferCopied,
            pucBuffer + __iBufferCopied + iBufferDataStart,
            __iBufferCopyLength );
    __iBufferCopied += __iBufferCopyLength;
  }
  iBufferDataEnd -= iBufferDataStart;
  iBufferDataStart = 0;
}

void CTransmit::resetBuffer()
{
  iBufferDataStart = 0;
  iBufferDataEnd = 0;
}

void CTransmit::resetPayload()
{
  if( poPayload )
    delete poPayload;
  poPayload = NULL;
  ePayloadType = PAYLOAD_UNDEFINED;
}

int CTransmit::initPayload( uint8_t _ui8tPayloadType )
{
  // Authorize payload
  if( CParameters::oPrincipal.hasPayloadTypes()
      && !CParameters::oPrincipal.hasPayloadType( _ui8tPayloadType ) )
    return -EPERM;

  // (Re-)Initialize
  resetPayload();
  switch( _ui8tPayloadType )
  {

  case PAYLOAD_RAW:
    poPayload = new CPayload();
    break;

  case PAYLOAD_AES128:
    poPayload = new CPayload_AES128();
    // ... default key (using user provided password salt)
    if( pucPasswordSalt )
    {
      ((CPayload_AES128*)poPayload)->makeCryptoKey( (unsigned char*)oPrincipal.getPassword(),
                                                    strlen( oPrincipal.getPassword() ),
                                                    pucPasswordSalt );
    }
    break;

  default:
    return -EINVAL;
  }
  if( !poPayload )
    return -ENOMEM;
  ePayloadType = (EPayloadType)_ui8tPayloadType;

  // Done
  return ePayloadType;
}

int CTransmit::allocPayload()
{
  if( !poPayload )
    return -ENODATA;
  return poPayload->alloc();
}

void CTransmit::freePayload()
{
  if( !poPayload )
    return;
  poPayload->free();
}

int CTransmit::serialize( int _iDescriptor,
                          const CData &_roData )
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

  // Create payload
  __iReturn = poPayload->serialize( pucBuffer, _roData );
  if( __iReturn < 0 )
    return __iReturn;
  int __iPayloadSize = __iReturn;

  // Send payload
  uint16_t __ui16tPayloadSize_NS = htons( __iPayloadSize );

  // ... size
  __iReturn = send( _iDescriptor, &__ui16tPayloadSize_NS, 2, MSG_MORE );
  if( __iReturn != 2 )
    return
      ( __iReturn <= 0 )
      ? __iReturn
      : -EIO;

  // ... content
  __iReturn = send( _iDescriptor, pucBuffer, __iPayloadSize, MSG_EOR );
  if( __iReturn != __iPayloadSize )
    return
      ( __iReturn <= 0 )
      ? __iReturn
      : -EIO;

  // Done
  return __iPayloadSize+2;
}

int CTransmit::unserialize( int _iDescriptor,
                            CData *_poData,
                            int _iMaxSize )
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

  // Receive payload

  // ... size
  __iReturn = recvBuffer( _iDescriptor, 2 );
  if( __iReturn != 2 )
    return
      ( __iReturn <= 0 )
      ? __iReturn
      : -EPROTO;
  uint16_t __ui16tPayloadSize = ntohs( *((uint16_t*)pullBuffer( 2 )) );
  if( _iMaxSize && __ui16tPayloadSize > _iMaxSize )
    return -EMSGSIZE;

  // ... content
  __iReturn = recvBuffer( _iDescriptor, __ui16tPayloadSize );
  if( __iReturn != __ui16tPayloadSize )
    return
      ( __iReturn <= 0 )
      ? __iReturn
      : -EPROTO;

  // Parse payload
  __iReturn = poPayload->unserialize( _poData,
                                      pullBuffer( __ui16tPayloadSize ),
                                      __ui16tPayloadSize );
  if( __iReturn <= 0 )
    return __iReturn;

  // Done
  return __ui16tPayloadSize+2;
}

void CTransmit::free()
{
  if( pucBuffer )
  {
    CPayload::freeBuffer( pucBuffer );
    pucBuffer = NULL;
    iBufferSize = 0;
    iBufferDataStart = 0;
    iBufferDataEnd = 0;
  }
}
