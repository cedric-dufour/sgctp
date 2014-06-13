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

#ifndef SGCTP_CTRANSMIT_UDP_HPP
#define SGCTP_CTRANSMIT_UDP_HPP

// SGCTP
#include "sgctp/transmit.hpp"


// SGCTP namespace
namespace SGCTP
{

  /// UDP transmission of SGCTP payload
  /**
   * This class allows the transmission of a SGCTP payload via UDP.
   */
  class CTransmit_UDP: public CTransmit
  {

    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CTransmit_UDP()
      : CTransmit()
    {};
    virtual ~CTransmit_UDP()
    {};


    //----------------------------------------------------------------------
    // METHODS: CTransmit (implement/override)
    //----------------------------------------------------------------------

  protected:
    virtual int send( int _iSocket,
                      const void *_pBuffer,
                      int _iSize,
                      int _iFlags );

    virtual int recv( int _iSocket,
                      void *_pBuffer,
                      int _iSize,
                      int _iFlags );

  public:
    virtual ETransmitType getTransmitType()
    {
      return TRANSMIT_UDP;
    };

    virtual int alloc();

    virtual int unserialize( int _iDescriptor,
                             CData *_poData,
                             int _iMaxSize = 0 );

  };

}

#endif // SGCTP_CTRANSMIT_UDP_HPP
