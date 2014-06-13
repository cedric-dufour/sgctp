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

#ifndef SGCTP_CTRANSMIT_HPP
#define SGCTP_CTRANSMIT_HPP

// C
#include <stdint.h>
#include <string.h>

// SGCTP
#include "sgctp/parameters.hpp"


// SGCTP namespace
namespace SGCTP
{

  // External
  class CData;
  class CPayload;

  /// Generic transmission of SGCTP payload
  /**
   * This class defines the generic aspects of the transmission of a SGCTP
   * payload.
   * WARNING: Different objects MUST be used for serialization and
   * unserialization!
   */
  class CTransmit: public CParameters
  {

    //----------------------------------------------------------------------
    // CONSTANTS / STATIC
    //----------------------------------------------------------------------

  public:
    /// Protocol version
    static const uint8_t PROTOCOL_VERSION = 1;

    /// Transmission types
    enum ETransmitType {
      TRANSMIT_UDP = 0,         ///< UDP socket
      TRANSMIT_TCP = 1,         ///< TCP socket
      TRANSMIT_FILE = 128,      ///< File socket
      TRANSMIT_UNDEFINED = 255  ///< undefined
    };

    /// Payload types
    enum EPayloadType {
      PAYLOAD_RAW = 0,         ///< raw payload
      PAYLOAD_AES128 = 1,      ///< AES128-encrypted payload
      PAYLOAD_UNDEFINED = 255  ///< undefined
    };


    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  protected:
    /// Transmission buffer
    unsigned char *pucBuffer;
    /// Transmission buffer size
    int iBufferSize;
    /// Transmission buffer actual data start offset
    int iBufferDataStart;
    /// Transmission buffer actual data end offset
    int iBufferDataEnd;

    /// Transmission timeout, in seconds
    double fdTimeout;

    /// Associated payload object
    CPayload *poPayload;
    /// Associated payload type
    EPayloadType ePayloadType;


    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CTransmit();
    virtual ~CTransmit();


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

  protected:
    /// Allocate resources required for data transmission
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    int allocBuffer();
    /// Receive data from the given descriptor (and push them on the data buffer)
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _iSize Size of data to receive
     *  @return (Positive) Quantity of data actually received; Negative error code in case of error
     */
    int recvBuffer( int _iDescriptor,
                    int _iSize );
    /// Pull data from the transmission buffer
    /**
     *  @param[in] _iSize Size of data to pull
     *  @return Pointer to the data (within the transmission buffer)
     */
    const unsigned char* pullBuffer( int _iSize );
    /// Flush the data buffer (from pulled data)
    void flushBuffer();
    /// Reset the data buffer (clear all data)
    void resetBuffer();

  public:
    /// Set the transmission (send/receive) timeout, in seconds
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _fdTimeout Transmission timeout, in seconds
     */
    virtual void setTimeout( int _iDescriptor,
                             double _fdTimeout )
    {
      fdTimeout = _fdTimeout;
    };
    /// Return whether data are available (in the transmission buffer)
    bool hasData()
    {
      return iBufferDataEnd > iBufferDataStart;
    };

  protected:
    /// Receive data from the given descriptor
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _pBuffer Pointer to buffer (to store received data)
     *  @param[in] _iSize Size of data to receive
     *  @param[in] _iFlags Descriptor-specific control flags
     *  @return (Positive) Quantity of data actually received; Negative error code in case of error
     */
    virtual int recv( int _iDescriptor,
                      void *_pBuffer,
                      int _iSize,
                      int _iFlags ) = 0;
    /// Send data to the given descriptor
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _pBuffer Pointer to buffer (containing data to be sent)
     *  @param[in] _iSize Size of data to send
     *  @param[in] _iFlags Descriptor-specific control flags
     *  @return (Positive) Quantity of data actually sent; Negative error code in case of error
     */
    virtual int send( int _iDescriptor,
                      const void *_pBuffer,
                      int _iSize,
                      int _iFlags ) = 0;

  public:
    /// Return the transmission type
    virtual ETransmitType getTransmitType() = 0;
    /// Return whether a payload object has been successfully associated/initialized
    virtual bool hasPayload()
    {
      return (bool)poPayload;
    };
    /// Reset (clear) the associated payload object
    virtual void resetPayload();
    /// Associate and initialize payload object
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    virtual int initPayload( uint8_t _ui8tPayloadType = PAYLOAD_RAW );
    /// Allocate resources required for payload (un-)serialization
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    virtual int allocPayload();
    /// Free resources required for payload (un-)serialization
    virtual void freePayload();
    /// Return the payload type
    virtual EPayloadType getPayloadType()
    {
      return ePayloadType;
    };

    /// Allocate resources required for data transmission (un-/serialization)
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    virtual int alloc() = 0;
    /// Serialize the given SGCTP data to the given descriptor
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _roData SGCTP data object (to be serialized/sent)
     *  @return (Positive) Quantity of data actually serialized/sent; Negative error code in case of error
     */
    virtual int serialize( int _iDescriptor,
                           const CData &_roData );
    /// Unserialize the SGCTP data from the given descriptor
    /**
     *  @param[in] _iDescriptor File/socket/... descriptor
     *  @param[in] _poData SGCTP data object (to store unserialized/received data)
     *  @param[in] _iMaxSize Maximum size of expected data (0 = no limit)
     *  @return (Positive) Quantity of data actually unserialized/received; Negative error code in case of error
     */
    virtual int unserialize( int _iDescriptor,
                             CData *_poData,
                             int _iMaxSize = 0 );
    /// Free resources required for data transmission (un-/serialization)
    virtual void free();

  };

}

#endif // SGCTP_CTRANSMIT_HPP
