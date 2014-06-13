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
#include <sys/socket.h>

// SGCTP
#include "sgctp/payload.hpp"
#include "sgctp/transmit_udp.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// METHODS: CTransmit (implement/override)
//----------------------------------------------------------------------

int CTransmit_UDP::send( int _iSocket,
                         const void *_pBuffer,
                         int _iSize,
                         int _iFlags )
{
  int __iReturn = ::sendto( _iSocket, _pBuffer, _iSize, _iFlags,
                            ptSockaddr, *ptSocklenT );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_UDP::recv( int _iSocket,
                         void *_pBuffer,
                         int _iSize,
                         int _iFlags )
{
  int __iReturn = ::recvfrom( _iSocket, _pBuffer, _iSize, _iFlags,
                              ptSockaddr, ptSocklenT );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_UDP::alloc()
{
  return CTransmit::allocBuffer();
}

int CTransmit_UDP::unserialize( int _iDescriptor,
                                CData *_poData,
                                int _iMaxSize )
{
  // Unserialize
  int __iReturn = CTransmit::unserialize( _iDescriptor,
                                          _poData,
                                          _iMaxSize );

  // Discard remaining buffer data (NOTE: help recover from corrupted data)
  resetBuffer();

  // Done
  return __iReturn;
}

