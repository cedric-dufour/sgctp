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

#ifndef SGCTP_CPAYLOAD_HPP
#define SGCTP_CPAYLOAD_HPP

// C
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// SGCTP namespace
namespace SGCTP
{

  // External
  class CData;


  /// (Raw) SGCTP payload
  /**
   * This class (un-)serializes data from/to a (raw) SGCTP payload.
   */
  class CPayload
  {

    //----------------------------------------------------------------------
    // CONSTANTS / STATIC
    //----------------------------------------------------------------------

  public:
    /// Buffer size required for payload (un-)serialization
    static const uint16_t BUFFER_SIZE = 33024; // Actually, it is 32939... but let's keep a few extra bytes available

  private:
    /// Bit masks used to (un-)serialize data bit-per-bit
    static const uint8_t BITMASK[9];

  public:
    /// Allocate a buffer for payload (un-)serialization
    /**
     *  @return Pointer to the allocated buffer; NULL in case of error
     */
    static unsigned char* allocBuffer()
    {
      return (unsigned char*)malloc( BUFFER_SIZE * sizeof( unsigned char ) );
    };
    /// Zero-out a buffer required for payload (un-)serialization
    /**
     *  @param[in] _pucBuffer Allocated buffer
     *  @see allocBuffer()
     */
    static void zeroBuffer( unsigned char *_pucBuffer )
    {
      memset( _pucBuffer, 0, BUFFER_SIZE * sizeof( unsigned char ) );
    };
    /// Free a buffer allocated for payload (un-)serialization
    /**
     *  @param[in] _pucBuffer Allocated buffer
     *  @see allocBuffer()
     */
    static void freeBuffer( unsigned char *_pucBuffer )
    {
      ::free( _pucBuffer );
    };

    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  private:
    /// SGCTP payload export buffer pointer
    unsigned char *pucBufferPut;
    /// SGCTP payload import buffer pointer
    const unsigned char *pucBufferGet;
    /// SGCTP payload bit position
    uint32_t ui32tBufferBitOffset;


    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CPayload()
    {};
    virtual ~CPayload()
    {};


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

  private:
    /// Add the given bits to the payload
    /**
     *  @param[in] _ui8tBitsSize Data size (bits quantity)
     *  @param[in] _ui8tBits Data (bits)
     */
    void putBits( uint8_t _ui8tBitsSize,
                  uint8_t _ui8tBits );
    /// Add the given bytes to the payload
    /**
     *  @param[in] _ui16tBytesSize Data size (bytes quantity)
     *  @param[in] _pucBytes Data buffer (to read the data from)
     */
    void putBytes( uint16_t _ui16tBytesSize,
                   const unsigned char *_pucBytes );
    /// Retrieve the given bits from the payload
    /**
     *  @param[in] _ui8tBitsSize Data size (bits quantity)
     *  @return Data (bits)
     */
    uint8_t getBits( uint8_t _ui8tBitsSize );
    /// Retrieve the given bytes from the payload
    /**
     *  @param[in] _ui16tBytesSize Data size (bytes quantity)
     *  @param[in] _pucBytes Data buffer (to write the data to)
     */
    void getBytes( uint16_t _ui16tBytesSize,
                   unsigned char *_pucBytes );

  public:
    /// Allocate resources for payload (un-)serialization
    /**
     *  @return Negative error code in case of error, zero otherwise
     */
    virtual int alloc()
    {
      return 0;
    };
    /// Serialize the given SGCTP data into the given payload buffer
    /**
     *  @param[in] _pucBuffer Payload buffer (to write the serialization data to)
     *  @param[in] _roData SGCTP data object (to be serialized)
     *  @return (Positive) Quantity of data actually serialized; Negative error code in case of error
     */
    virtual int serialize( unsigned char *_pucBuffer,
                           const CData &_roData );
    /// Unserialize the SGCTP data from the given payload buffer
    /**
     *  @param[in] _poData SGCTP data object (to store unserialized data)
     *  @param[in] _pucBuffer Payload buffer (to read the serialization data from)
     *  @return (Positive) Quantity of data actually unserialized; Negative error code in case of error
     */
    virtual int unserialize( CData *_poData,
                             const unsigned char *_pucBuffer,
                             uint16_t _ui16tBufferSize );
    /// Free resources for payload (un-)serialization
    virtual void free() {};

  };

}

#endif // SGCTP_CPAYLOAD_HPP
