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
#include <unistd.h>

// SGCTP
#include "sgctp/payload.hpp"
#include "sgctp/transmit_file.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// METHODS: CTransmit (implement/override)
//----------------------------------------------------------------------


int CTransmit_File::send( int _iFileDescriptor,
                          const void *_pBuffer,
                          int _iSize,
                          int _iFlags )
{
  int __iReturn = ::write( _iFileDescriptor, _pBuffer, _iSize );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_File::recv( int _iFileDescriptor,
                          void *_pBuffer,
                          int _iSize,
                          int _iFlags )
{
  int __iReturn = ::read( _iFileDescriptor, _pBuffer, _iSize );
  return
    ( __iReturn >= 0 )
    ? __iReturn
    : -errno;
}

int CTransmit_File::alloc()
{
  return CTransmit::allocBuffer();
}
