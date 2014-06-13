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

#ifndef SGCTP_CTRANSMIT_FILE_HPP
#define SGCTP_CTRANSMIT_FILE_HPP

// SGCTP
#include "sgctp/transmit.hpp"


// SGCTP namespace
namespace SGCTP
{

  /// Save/load SGCTP payload to/from a file
  /**
   * This class allows to save/load a SGCTP payload to/from a file.
   */
  class CTransmit_File: public CTransmit
  {

    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CTransmit_File()
      : CTransmit()
    {};
    virtual ~CTransmit_File()
    {};


    //----------------------------------------------------------------------
    // METHODS: CTransmit (implement/override)
    //----------------------------------------------------------------------

  protected:
    virtual int send( int _iFileDescriptor,
                      const void *_pBuffer,
                      int _iSize,
                      int _iFlags );

    virtual int recv( int _iFileDescriptor,
                      void *_pBuffer,
                      int _iSize,
                      int _iFlags );

  public:
    virtual ETransmitType getTransmitType()
    {
      return TRANSMIT_FILE;
    };

    virtual int alloc();

  };

}

#endif // SGCTP_CTRANSMIT_FILE_HPP
