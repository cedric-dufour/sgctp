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

#ifndef SGCTP_CTRANSMIT_TCP_HPP
#define SGCTP_CTRANSMIT_TCP_HPP

// C
#include <stdint.h>

// SGCTP
#include "sgctp/transmit.hpp"


// SGCTP namespace
namespace SGCTP
{

  /// TCP transmission of SGCTP payload
  /**
   * This class allows the transmission of a SGCTP payload via TCP, including
   * additional definition of the required handshake (and underlying "session"
   * establishement).
   */
  class CTransmit_TCP: public CTransmit
  {

    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CTransmit_TCP()
    : CTransmit()
    {};
    virtual ~CTransmit_TCP()
    {};


    //----------------------------------------------------------------------
    // METHODS: CTransmit (implement/override)
    //----------------------------------------------------------------------

  public:
    virtual void setTimeout( int _iSocket,
                             double _fdTimeout );

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
      return TRANSMIT_TCP;
    };

    virtual int alloc();

    virtual int serialize( int _iSocket,
                           const CData &_roData );

    virtual int unserialize( int _iSocket,
                             CData *_poData,
                             int _iMaxSize = 0 );

    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

  public:
    /// Send the TCP handshake
    /**
     *  @param[in] _iSocket TCP socket descriptor
     *  @return Negative error code in case of error, zero otherwise
     */
    int sendHandshake( int _iSocket );
    /// Receive the TCP handshake (and initialize internal resources: principal/payload)
    /**
     *  @param[in] _iSocket TCP socket descriptor
     *  @return Negative error code in case of error, zero otherwise
     */
    int recvHandshake( int _iSocket );

  };

}

#endif // SGCTP_CTRANSMIT_TCP_HPP
