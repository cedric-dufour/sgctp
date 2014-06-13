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
#include <stdlib.h>
#include <string.h>

// SGCTP
#include "sgctp/data.hpp"
#include "sgctp/payload.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CONSTANTS / STATIC
//----------------------------------------------------------------------

const uint8_t CPayload::BITMASK[9] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

void CPayload::putBits( uint8_t _ui8tBitsCount,
                        uint8_t _ui8tBits )
{
  // WARNING: code MUST be hardware-independent, so as to have most significant bits first!
  uint32_t __ui32tBufferByteOffset = ( ui32tBufferBitOffset >> 3 );
  uint16_t __ui16tData =
    ( _ui8tBits & BITMASK[_ui8tBitsCount] )
    << ( 16 - _ui8tBitsCount - ( ui32tBufferBitOffset & 0x7 ) );
  *(unsigned char*)(pucBufferPut+__ui32tBufferByteOffset) |=
    (unsigned char)( __ui16tData >> 8 );
  *(unsigned char*)(pucBufferPut+__ui32tBufferByteOffset+1) |=
    (unsigned char)( __ui16tData & 0xFF );
  ui32tBufferBitOffset += _ui8tBitsCount;
}

void CPayload::putBytes( uint16_t _ui16tBytesCount,
                         const unsigned char *_pucBytes )
{
  if( uint8_t __ui8tBufferBitAlign = ( ui32tBufferBitOffset & 0x7 ) )
    putBits( 8 - __ui8tBufferBitAlign, 0 );
  if( _ui16tBytesCount == 0 )
    return;
  uint32_t __ui32tBufferByteOffset = ( ui32tBufferBitOffset >> 3 );
  memcpy( pucBufferPut+__ui32tBufferByteOffset, _pucBytes, _ui16tBytesCount );
  ui32tBufferBitOffset += _ui16tBytesCount * 8;
}

uint8_t CPayload::getBits( uint8_t _ui8tBitsCount )
{
  // WARNING: code MUST be hardware-independent, so as to have most significant bits first!
  uint32_t __ui32tBufferByteOffset = ( ui32tBufferBitOffset >> 3 );
  uint16_t __ui16tBits =
    (uint16_t)( *(unsigned char*)(pucBufferGet+__ui32tBufferByteOffset) ) << 8
    | *(unsigned char*)(pucBufferGet+__ui32tBufferByteOffset+1);
  __ui16tBits >>= ( 16 - _ui8tBitsCount - ( ui32tBufferBitOffset & 0x7 ) );
  ui32tBufferBitOffset += _ui8tBitsCount;
  return __ui16tBits & BITMASK[_ui8tBitsCount];
}

void CPayload::getBytes( uint16_t _ui16tBytesCount,
                         unsigned char *_pucBytes )
{
  if( uint8_t __ui8tBufferBitAlign = ( ui32tBufferBitOffset & 0x7 ) )
    getBits( 8 - __ui8tBufferBitAlign );
  if( _ui16tBytesCount == 0 )
    return;
  uint32_t __ui32tBufferByteOffset = ( ui32tBufferBitOffset >> 3 );
  memcpy( _pucBytes, pucBufferGet+__ui32tBufferByteOffset, _ui16tBytesCount );
  ui32tBufferBitOffset += _ui16tBytesCount * 8;
}

int CPayload::serialize( unsigned char *_pucBuffer,
                         const CData &_roData )
{
  // Check buffer
  if( !_pucBuffer )
    return -EINVAL;
  pucBufferPut = _pucBuffer;

  // Zero buffer and reset bit counter
  zeroBuffer( pucBufferPut );
  ui32tBufferBitOffset = 0;

  // Payload parameters
  uint8_t __ui8tIDLength = strlen( _roData.pcID );
  bool __bTime =
    !( _roData.ui32tTime & CData::UNDEFINED_UINT32 );
  bool __b2DPosition =
    !( _roData.ui32tLatitude & CData::UNDEFINED_UINT32
       || _roData.ui32tLongitude & CData::UNDEFINED_UINT32 );
  bool __b3DPosition =
    __b2DPosition
    && !( _roData.ui32tElevation & CData::UNDEFINED_UINT32 );
  bool __b2DGndCourse =
    !( _roData.ui32tBearing & CData::UNDEFINED_UINT32
       || _roData.ui32tGndSpeed & CData::UNDEFINED_UINT32 );
  bool __b3DGndCourse =
    __b2DGndCourse
    && !( _roData.ui32tVrtSpeed & CData::UNDEFINED_UINT32 );
  bool __b2DGndCourseDt =
    !( _roData.ui32tBearingDt & CData::UNDEFINED_UINT32
       || _roData.ui32tGndSpeedDt & CData::UNDEFINED_UINT32 );
  bool __b3DGndCourseDt =
    __b2DGndCourseDt
    && !( _roData.ui32tVrtSpeedDt & CData::UNDEFINED_UINT32 );
  bool __b2DAppCourse =
    !( _roData.ui32tHeading & CData::UNDEFINED_UINT32
       || _roData.ui32tAppSpeed & CData::UNDEFINED_UINT32 );
  // ... extended content
  bool __bExtendedContent = false;
  bool __b2SourceType =
    !( _roData.ui32tSourceType & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b2SourceType;
  bool __b2DPositionError =
    !( _roData.ui32tLatitudeError & CData::UNDEFINED_UINT32
       || _roData.ui32tLongitudeError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b2DPositionError;
  bool __b3DPositionError =
    __b2DPositionError
    && !( _roData.ui32tElevationError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b3DPositionError;
  bool __b2DGndCourseError =
    !( _roData.ui32tBearingError & CData::UNDEFINED_UINT32
       || _roData.ui32tGndSpeedError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b2DGndCourseError;
  bool __b3DGndCourseError =
    __b2DGndCourseError
    && !( _roData.ui32tVrtSpeedError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b3DGndCourseError;
  bool __b2DGndCourseDtError =
    !( _roData.ui32tBearingDtError & CData::UNDEFINED_UINT32
       || _roData.ui32tGndSpeedDtError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b2DGndCourseDtError;
  bool __b3DGndCourseDtError =
    __b2DGndCourseDtError
    && !( _roData.ui32tVrtSpeedDtError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b3DGndCourseDtError;
  bool __b2DAppCourseError =
    !( _roData.ui32tHeadingError & CData::UNDEFINED_UINT32
       || _roData.ui32tAppSpeedError & CData::UNDEFINED_UINT32 );
  __bExtendedContent |= __b2DAppCourseError;

  // Content
  putBits( 1, __b2DPosition );
  putBits( 1, __b3DPosition );
  putBits( 1, __b2DGndCourse );
  putBits( 1, __b3DGndCourse );
  putBits( 1, __b2DGndCourseDt );
  putBits( 1, __b3DGndCourseDt );
  putBits( 1, __b2DAppCourse );
  putBits( 1, __bExtendedContent );

  // ID' + ID
  putBits( 7, __ui8tIDLength );
  putBits( 1, _roData.ui16tDataSize > 0 );
  if( __ui8tIDLength > 0 )
    putBytes( __ui8tIDLength, (unsigned char*)_roData.pcID );

  // Data' + Data" + Data
  if( _roData.ui16tDataSize > 0 )
  {
    if( _roData.ui16tDataSize > 127 )
    {
      putBits( 7, (uint8_t)( _roData.ui16tDataSize ) );
      putBits( 1, 1 );
      putBits( 8, (uint8_t)( _roData.ui16tDataSize >> 7 ) );
    }
    else
    {
      putBits( 7, (uint8_t)( _roData.ui16tDataSize ) );
      putBits( 1, 0 );
    }
  }
  if( _roData.ui16tDataSize > 0 )
    putBytes( _roData.ui16tDataSize, _roData.pucData );

  // Content

  // ... Time
  if( __bTime )
  {
    putBits( 7, (uint8_t)( _roData.ui32tTime >> 16 ) );
    putBits( 8, (uint8_t)( _roData.ui32tTime >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tTime ) );
  }
  else
  {
    putBits( 7, (uint8_t)( CData::OVERFLOW_UINT32 >> 16 ) );
    putBits( 8, (uint8_t)( CData::OVERFLOW_UINT32 >> 8 ) );
    putBits( 8, (uint8_t)( CData::OVERFLOW_UINT32 ) );
  }

  // ... Position
  if( __b2DPosition )
  {
    // ... Latitude
    putBits( 6, (uint8_t)( _roData.ui32tLatitude >> 24 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLatitude >> 16 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLatitude >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLatitude ) );
    // ... Longitude
    putBits( 7, (uint8_t)( _roData.ui32tLongitude >> 24 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLongitude >> 16 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLongitude >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLongitude ) );
  }
  if( __b3DPosition )
  {
    // ... Elevation
    putBits( 3, (uint8_t)( _roData.ui32tElevation >> 16 ) );
    putBits( 8, (uint8_t)( _roData.ui32tElevation >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tElevation ) );
  }

  // ... Ground course
  if( __b2DGndCourse )
  {
    // ... Bearing
    putBits( 4, (uint8_t)( _roData.ui32tBearing >> 8) );
    putBits( 8, (uint8_t)( _roData.ui32tBearing ) );
    // ... Ground speed
    putBits( 8, (uint8_t)( _roData.ui32tGndSpeed >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tGndSpeed ) );
  }
  if( __b3DGndCourse )
  {
    // ... Vertical speed
    putBits( 5, (uint8_t)( _roData.ui32tVrtSpeed >> 8) );
    putBits( 8, (uint8_t)( _roData.ui32tVrtSpeed ) );
  }

  // ... Ground course variation over time
  if( __b2DGndCourseDt )
  {
    // ... Bearing
    putBits( 2, (uint8_t)( _roData.ui32tBearingDt >> 8) );
    putBits( 8, (uint8_t)( _roData.ui32tBearingDt ) );
    // ... Ground speed
    putBits( 4, (uint8_t)( _roData.ui32tGndSpeedDt >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tGndSpeedDt ) );
  }
  if( __b3DGndCourseDt )
  {
    // ... Vertical speed
    putBits( 4, (uint8_t)( _roData.ui32tVrtSpeedDt >> 8) );
    putBits( 8, (uint8_t)( _roData.ui32tVrtSpeedDt ) );
  }

  // ... Apparent course
  if( __b2DAppCourse )
  {
    // ... Heading
    putBits( 4, (uint8_t)( _roData.ui32tHeading >> 8) );
    putBits( 8, (uint8_t)( _roData.ui32tHeading ) );
    // ... Apparent speed
    putBits( 8, (uint8_t)( _roData.ui32tAppSpeed >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tAppSpeed ) );
  }

  // Extended content
  if( __bExtendedContent )
  {
    putBytes( 0, NULL ); // re-align
    putBits( 1, __b2DPositionError );
    putBits( 1, __b3DPositionError );
    putBits( 1, __b2DGndCourseError );
    putBits( 1, __b3DGndCourseError );
    putBits( 1, __b2DGndCourseDtError );
    putBits( 1, __b3DGndCourseDtError );
    putBits( 1, __b2DAppCourseError );
    putBits( 1, 0 );
  }
  // ... Source type
  putBits( 8, (uint8_t)( _roData.ui32tSourceType ) );

  // ... Position error
  if( __b2DPositionError )
  {
    // ... Latitude
    putBits( 4, (uint8_t)( _roData.ui32tLatitudeError >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLatitudeError ) );
    // ... Longitude
    putBits( 4, (uint8_t)( _roData.ui32tLongitudeError >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tLongitudeError ) );
  }
  if( __b3DPositionError )
  {
    // ... Elevation
    putBits( 4, (uint8_t)( _roData.ui32tElevationError >> 8 ) );
    putBits( 8, (uint8_t)( _roData.ui32tElevationError ) );
  }

  // ... Ground course error
  if( __b2DGndCourseError )
  {
    // ... Bearing
    putBits( 8, (uint8_t)( _roData.ui32tBearingError ) );
    // ... Ground speed
    putBits( 8, (uint8_t)( _roData.ui32tGndSpeedError ) );
  }
  if( __b3DGndCourseError )
  {
    // ... Vertical speed
    putBits( 8, (uint8_t)( _roData.ui32tVrtSpeedError ) );
  }

  // ... Ground course variation over time error
  if( __b2DGndCourseDtError )
  {
    // ... Bearing
    putBits( 8, (uint8_t)( _roData.ui32tBearingDtError ) );
    // ... Ground speed
    putBits( 8, (uint8_t)( _roData.ui32tGndSpeedDtError ) );
  }
  if( __b3DGndCourseDtError )
  {
    // ... Vertical speed
    putBits( 8, (uint8_t)( _roData.ui32tVrtSpeedDtError ) );
  }

  // ... Apparent course error
  if( __b2DAppCourseError )
  {
    // ... Heading
    putBits( 8, (uint8_t)( _roData.ui32tHeadingError ) );
    // ... Apparent speed
    putBits( 8, (uint8_t)( _roData.ui32tAppSpeedError ) );
  }

  // Done
  return ( ui32tBufferBitOffset + 7 ) >> 3;
}

int CPayload::unserialize( CData *_poData,
                           const unsigned char *_pucBuffer,
                           uint16_t _ui16tPayloadSize )
{
  // Check buffer
  if( !_pucBuffer )
    return -EINVAL;
  pucBufferGet = _pucBuffer;

  // Reset bit counter
  ui32tBufferBitOffset = 0;

  // Zero data container
  _poData->reset( false );

  // Content
  // ... Geolocalization data
  bool __b2DPosition = (bool)getBits( 1 );
  bool __b3DPosition = (bool)getBits( 1 );
  bool __b2DGndCourse = (bool)getBits( 1 );
  bool __b3DGndCourse = (bool)getBits( 1 );
  bool __b2DGndCourseDt = (bool)getBits( 1 );
  bool __b3DGndCourseDt = (bool)getBits( 1 );
  bool __b2DAppCourse = (bool)getBits( 1 );
  bool __bExtendedContent = (bool)getBits( 1 );

  // ID' + ID
  uint8_t __ui8tIDLength = getBits( 7 );
  bool __bData = (bool)getBits( 1 );
  if( __ui8tIDLength > 0 )
    getBytes( __ui8tIDLength, (unsigned char*)_poData->pcID );
  _poData->pcID[__ui8tIDLength] = '\0';

  // Data' + Data" + Data
  if( __bData )
  {
    uint16_t __ui16tDataSize = getBits( 7 );
    if( (bool)getBits( 1 ) )
      __ui16tDataSize |= getBits( 8 ) << 7;
    _poData->allocData( __ui16tDataSize );
  }
  else
    _poData->freeData();
  if( _poData->ui16tDataSize > 0 )
    getBytes( _poData->ui16tDataSize, _poData->pucData );

  // Content
  uint32_t __ui32tData;

  // ... Time
  __ui32tData = getBits( 7 );
  __ui32tData <<= 8; __ui32tData |= getBits( 8 );
  __ui32tData <<= 8; __ui32tData |= getBits( 8 );
  _poData->ui32tTime =
    ( __ui32tData == ( 0xFFFFFFFF >> 9 ) )
    ? CData::UNDEFINED_UINT32
    : __ui32tData;

  // ... Position
  if( __b2DPosition )
  {
    // ... Latitude
    __ui32tData = getBits( 6 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tLatitude =
      ( __ui32tData == ( 0xFFFFFFFF >> 2 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Longitude
    __ui32tData = getBits( 7 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tLongitude =
      ( __ui32tData == ( 0xFFFFFFFF >> 1 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DPosition )
  {
    // ... Elevation
    __ui32tData = getBits( 3 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tElevation =
      ( __ui32tData == ( 0xFFFFFFFF >> 13 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Ground course
  if( __b2DGndCourse )
  {
    // ... Bearing
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tBearing =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Ground speed
    __ui32tData = getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tGndSpeed =
      ( __ui32tData == ( 0xFFFFFFFF >> 16 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DGndCourse )
  {
    // ... Vertical speed
    __ui32tData = getBits( 5 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tVrtSpeed =
      ( __ui32tData == ( 0xFFFFFFFF >> 19 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Ground course variation over time
  if( __b2DGndCourseDt )
  {
    // ... Bearing
    __ui32tData = getBits( 2 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tBearingDt =
      ( __ui32tData == ( 0xFFFFFFFF >> 22 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Ground speed
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tGndSpeedDt =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DGndCourseDt )
  {
    // ... Vertical speed
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tVrtSpeedDt =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Apparent course
  if( __b2DAppCourse )
  {
    // ... Heading
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tHeading =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Apparent speed
    __ui32tData = getBits( 8 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tAppSpeed =
      ( __ui32tData == ( 0xFFFFFFFF >> 16 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }


  // Extended content
  bool __b2DPositionError = false;
  bool __b3DPositionError = false;
  bool __b2DGndCourseError = false;
  bool __b3DGndCourseError = false;
  bool __b2DGndCourseDtError = false;
  bool __b3DGndCourseDtError = false;
  bool __b2DAppCourseError = false;
  if( __bExtendedContent )
  {
    getBytes( 0, NULL ); // re-align
    __b2DPositionError = (bool)getBits( 1 );
    __b3DPositionError = (bool)getBits( 1 );
    __b2DGndCourseError = (bool)getBits( 1 );
    __b3DGndCourseError = (bool)getBits( 1 );
    __b2DGndCourseDtError = (bool)getBits( 1 );
    __b3DGndCourseDtError = (bool)getBits( 1 );
    __b2DAppCourseError = (bool)getBits( 1 );
    getBits( 1 );
  }
  // ... Source type
  __ui32tData = getBits( 8 );
  _poData->ui32tSourceType =
    ( __ui32tData != CData::SOURCE_UNDEFINED )
    ? __ui32tData
    : CData::UNDEFINED_UINT32;

  // ... Position error
  if( __b2DPositionError )
  {
    // ... Latitude
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tLatitudeError =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Longitude
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tLongitudeError =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DPositionError )
  {
    // ... Elevation
    __ui32tData = getBits( 4 );
    __ui32tData <<= 8; __ui32tData |= getBits( 8 );
    _poData->ui32tElevationError =
      ( __ui32tData == ( 0xFFFFFFFF >> 20 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Ground course error
  if( __b2DGndCourseError )
  {
    // ... Bearing
    __ui32tData = getBits( 8 );
    _poData->ui32tBearingError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Ground speed
    __ui32tData = getBits( 8 );
    _poData->ui32tGndSpeedError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DGndCourseError )
  {
    // ... Vertical speed
    __ui32tData = getBits( 8 );
    _poData->ui32tVrtSpeedError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Ground course variation over time error
  if( __b2DGndCourseDtError )
  {
    // ... Bearing
    __ui32tData = getBits( 8 );
    _poData->ui32tBearingDtError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Ground speed
    __ui32tData = getBits( 8 );
    _poData->ui32tGndSpeedDtError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }
  if( __b3DGndCourseDtError )
  {
    // ... Vertical speed
    __ui32tData = getBits( 8 );
    _poData->ui32tVrtSpeedDtError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // ... Apparent course error
  if( __b2DAppCourseError )
  {
    // ... Heading
    __ui32tData = getBits( 8 );
    _poData->ui32tHeadingError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
    // ... Apparent speed
    __ui32tData = getBits( 8 );
    _poData->ui32tAppSpeedError =
      ( __ui32tData == ( 0xFFFFFFFF >> 24 ) )
      ? CData::OVERFLOW_UINT32
      : __ui32tData;
  }

  // Check size
  if( ( ui32tBufferBitOffset + 7 ) >> 3 != _ui16tPayloadSize )
    return -EBADMSG;

  // Done
  return ( ui32tBufferBitOffset + 7 ) >> 3;
}
