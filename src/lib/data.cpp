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
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C++
#include <string>
using namespace std;

// SGCTP
#include "sgctp/data.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CONSTANTS / STATIC
//----------------------------------------------------------------------

const double CData::UNDEFINED_VALUE = NAN;
const double CData::OVERFLOW_VALUE = INFINITY;

string CData::stringSourceType( int _iSourceType )
{
  switch( _iSourceType )
  {
  case SOURCE_GPS: return "GPS";
  case SOURCE_AIS: return "AIS";
  case SOURCE_ADSB: return "ADS-B";
  case SOURCE_FLARM: return "FLARM";
  }
  return "";
}

double CData::epoch()
{
  timespec __tTimespec;
  clock_gettime( CLOCK_REALTIME, &__tTimespec ) ;
  return( (double)__tTimespec.tv_sec
          + (double)__tTimespec.tv_nsec/1000000000.0 );
}

double CData::toEpoch( double _fdTime,
                       double _fdEpochReference )
{
  if( __isnanl( _fdTime ) ) return 0.0;

  // Build "fully qualified" UNIX epoch
  double __fdEpoch;
  tm __tTm;
  timespec __tTimespecNow;
  if( _fdEpochReference == 0 )
  {
    clock_gettime( CLOCK_REALTIME, &__tTimespecNow ) ;
    gmtime_r( &(__tTimespecNow.tv_sec), &__tTm );
  }
  else
  {
    time_t __ttTime = (time_t)_fdEpochReference;
    gmtime_r( &__ttTime, &__tTm );
  }
  __tTm.tm_hour = 0; __tTm.tm_min = 0; __tTm.tm_sec = 0;
  __fdEpoch = timegm( &__tTm ) + _fdTime;

  // Fix midnight innacuracies
  if( _fdEpochReference == 0
      && _fdTime >= 43000
      && ( __tTimespecNow.tv_sec - (time_t)__fdEpoch ) >= 43000 )
    __fdEpoch -= 86400;

  // Done
  return __fdEpoch;
}

void CData::toIso8601( char *_pcIso8601, double _fdEpoch )
{
  double __fdSeconds;
  double __fdSubseconds = modf( _fdEpoch, &__fdSeconds );
  time_t __tTime = (int)__fdSeconds;
  tm __tTM;
  gmtime_r( &__tTime, &__tTM );
  if( _fdEpoch >= 86400 )
  {
    sprintf( _pcIso8601+18, "%.3fZ", __fdSubseconds );
    strftime( _pcIso8601, 20, "%Y-%m-%dT%H:%M:%S", &__tTM );
    _pcIso8601[19] = '.';
  }
  else
  {
    sprintf( _pcIso8601+7, "%.3fZ", __fdSubseconds );
    strftime( _pcIso8601, 9, "%H:%M:%S", &__tTM );
    _pcIso8601[8] = '.';
  }
}

double CData::fromIso8601( const char *_pcIso8601 )
{
  double __fdEpoch = 0.0;

  // Try potential ISO-8601 formats
  char *__pcReturn;
  tm __tTM;
  memset( &__tTM, 0, sizeof( struct tm ) );
  do
  {
    // ... date/time (1)
    __pcReturn = strptime( _pcIso8601, "%Y-%m-%dT%H:%M:%S", &__tTM );
    if( __pcReturn != NULL ) break;
    // ... date/time (2)
    __pcReturn = strptime( _pcIso8601, "%Y-%m-%d %H:%M:%S", &__tTM );
    if( __pcReturn != NULL ) break;
    // ... date
    __pcReturn = strptime( _pcIso8601, "%Y-%m-%d", &__tTM );
    if( __pcReturn != NULL ) break;
    // ... time
    char __pcTimeOnly[20];
    snprintf( __pcTimeOnly, 20, "1970-01-01T%s", _pcIso8601 );
    __pcReturn = strptime( __pcTimeOnly, "%Y-%m-%dT%H:%M:%S", &__tTM );
    if( __pcReturn != NULL ) break;
  }
  while( false );
  if( __pcReturn != NULL )
  {
    __fdEpoch = timegm(  &__tTM );
    if( *__pcReturn == '.' ) __fdEpoch += strtod( __pcReturn, NULL );
  }
  else
    __fdEpoch = strtod( _pcIso8601, NULL );

  // Done
  return __fdEpoch;
}



//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CData::~CData()
{
  if( pucData )
    free( pucData );
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

//
// SETTERS
//

void CData::reset( bool _bDataFree )
{
  pcID[0] = '\0';
  if( _bDataFree )
    freeData();
  ui32tTime = UNDEFINED_UINT32;
  ui32tLongitude = UNDEFINED_UINT32;
  ui32tLatitude = UNDEFINED_UINT32;
  ui32tElevation = UNDEFINED_UINT32;
  ui32tBearing = UNDEFINED_UINT32;
  ui32tGndSpeed = UNDEFINED_UINT32;
  ui32tVrtSpeed = UNDEFINED_UINT32;
  ui32tBearingDt = UNDEFINED_UINT32;
  ui32tGndSpeedDt = UNDEFINED_UINT32;
  ui32tVrtSpeedDt = UNDEFINED_UINT32;
  ui32tHeading = UNDEFINED_UINT32;
  ui32tAppSpeed = UNDEFINED_UINT32;
  ui32tSourceType = UNDEFINED_UINT32;
  ui32tLatitudeError = UNDEFINED_UINT32;
  ui32tLongitudeError = UNDEFINED_UINT32;
  ui32tElevationError = UNDEFINED_UINT32;
  ui32tBearingError = UNDEFINED_UINT32;
  ui32tGndSpeedError = UNDEFINED_UINT32;
  ui32tVrtSpeedError = UNDEFINED_UINT32;
  ui32tBearingDtError = UNDEFINED_UINT32;
  ui32tGndSpeedDtError = UNDEFINED_UINT32;
  ui32tVrtSpeedDtError = UNDEFINED_UINT32;
  ui32tHeadingError = UNDEFINED_UINT32;
  ui32tAppSpeedError = UNDEFINED_UINT32;
}

void CData::setID( const char *_pcID )
{
  uint8_t __ui8tIDLength =
    ( strlen( _pcID ) < 127 )
    ? strlen( _pcID )
    : 127;
  memcpy( pcID, _pcID, __ui8tIDLength );
  pcID[__ui8tIDLength] = '\0';
}

uint16_t CData::setData( const unsigned char *_pucData,
                         uint16_t _ui16tDataSize )
{
  allocData( _ui16tDataSize );
  if( ui16tDataSize > 0 )
    memcpy( pucData, _pucData, ui16tDataSize );
  return ui16tDataSize;
}

void CData::setTime( double _fdEpoch )
{
  tm __tTm;
  time_t __ttTime = (time_t)_fdEpoch;
  gmtime_r( &__ttTime, &__tTm );
  double __fd, __fdTime =
    (double)( 3600*__tTm.tm_hour + 60*__tTm.tm_min + __tTm.tm_sec )
    + modf( _fdEpoch, &__fd );
  ui32tTime = (uint32_t)( __fdTime * 10.0 + 0.5 );
}

void CData::setLongitude( double _fdLongitude )
{
  if( _fdLongitude < -180.0 )
    ui32tLongitude = 0;
  else if( _fdLongitude > 180.0 )
    ui32tLongitude = OVERFLOW_UINT32;
  else
    ui32tLongitude = (uint32_t)( _fdLongitude * 3600000.0 + 1073741824.5 );
}

void CData::setElevation( double _fdElevation )
{
  if( _fdElevation < -13107.15 )
    ui32tElevation = 0;
  else if( _fdElevation > 39321.35 )
    ui32tElevation = OVERFLOW_UINT32;
  else
    ui32tElevation = (uint32_t)( _fdElevation * 10.0 + 131072.5 );
}

void CData::setLatitude( double _fdLatitude )
{
  if( _fdLatitude < -90.0 )
    ui32tLatitude = 0;
  else if( _fdLatitude > 90.0 )
    ui32tLatitude = OVERFLOW_UINT32;
  else
    ui32tLatitude = (uint32_t)( _fdLatitude * 3600000.0 + 536870912.5 );
}

void CData::setBearing( double _fdBearing )
{
  if( _fdBearing < 0.0 )
    ui32tBearing = OVERFLOW_UINT32;
  else if( _fdBearing >= 360.0 )
    ui32tBearing = OVERFLOW_UINT32;
  else
    ui32tBearing = (uint32_t)( _fdBearing * 10.0 + 0.5 );
}

void CData::setGndSpeed( double _fdGndSpeed )
{
  if( _fdGndSpeed < 0.0 )
    ui32tGndSpeed = 0;
  else if( _fdGndSpeed > 6553.35 )
    ui32tGndSpeed = OVERFLOW_UINT32;
  else
    ui32tGndSpeed = (uint32_t)( _fdGndSpeed * 10.0 + 1.5 );
}

void CData::setVrtSpeed( double _fdVrtSpeed )
{
  if( _fdVrtSpeed < -409.55 )
    ui32tVrtSpeed = 0;
  else if( _fdVrtSpeed > 409.35 )
    ui32tVrtSpeed = OVERFLOW_UINT32;
  else
    ui32tVrtSpeed = (uint32_t)( _fdVrtSpeed * 10.0 + 4096.5 );
}

void CData::setBearingDt( double _fdBearingDt )
{
  if( _fdBearingDt < -51.15 )
    ui32tBearingDt = 0;
  else if( _fdBearingDt > 50.95 )
    ui32tBearingDt = OVERFLOW_UINT32;
  else
    ui32tBearingDt = (uint32_t)( _fdBearingDt * 10.0 + 512.5 );
}

void CData::setGndSpeedDt( double _fdGndSpeedDt )
{
  if( _fdGndSpeedDt < -204.75 )
    ui32tGndSpeedDt = 0;
  else if( _fdGndSpeedDt > 204.55 )
    ui32tGndSpeedDt = OVERFLOW_UINT32;
  else
    ui32tGndSpeedDt = (uint32_t)( _fdGndSpeedDt * 10.0 + 2048.5 );
}

void CData::setVrtSpeedDt( double _fdVrtSpeedDt )
{
  if( _fdVrtSpeedDt < -204.75 )
    ui32tVrtSpeedDt = 0;
  else if( _fdVrtSpeedDt > 204.55 )
    ui32tVrtSpeedDt = OVERFLOW_UINT32;
  else
    ui32tVrtSpeedDt = (uint32_t)( _fdVrtSpeedDt * 10.0 + 2048.5 );
}

void CData::setHeading( double _fdHeading )
{
  if( _fdHeading < 0.0 )
    ui32tHeading = OVERFLOW_UINT32;
  else if( _fdHeading > 360.0 )
    ui32tHeading = OVERFLOW_UINT32;
  else
    ui32tHeading = (uint32_t)( _fdHeading * 10.0 + 0.5 );
}

void CData::setAppSpeed( double _fdAppSpeed )
{
  if( _fdAppSpeed < 0.0 )
    ui32tAppSpeed = 0;
  else if( _fdAppSpeed > 6553.35 )
    ui32tAppSpeed = OVERFLOW_UINT32;
  else
    ui32tAppSpeed = (uint32_t)( _fdAppSpeed * 10.0 + 1.5 );
}

void CData::setLatitudeError( double _fdLatitudeError )
{
  _fdLatitudeError = fabs( _fdLatitudeError );
  if( _fdLatitudeError > 409.45 )
    ui32tLatitudeError = OVERFLOW_UINT32;
  else
    ui32tLatitudeError = (uint32_t)( _fdLatitudeError * 10.0 + 0.5 );
}

void CData::setLongitudeError( double _fdLongitudeError )
{
  _fdLongitudeError = fabs( _fdLongitudeError );
  if( _fdLongitudeError > 409.45 )
    ui32tLongitudeError = OVERFLOW_UINT32;
  else
    ui32tLongitudeError = (uint32_t)( _fdLongitudeError * 10.0 + 0.5 );
}

void CData::setElevationError( double _fdElevationError )
{
  _fdElevationError = fabs( _fdElevationError );
  if( _fdElevationError > 409.45 )
    ui32tElevationError = OVERFLOW_UINT32;
  else
    ui32tElevationError = (uint32_t)( _fdElevationError * 10.0 + 0.5 );
}

void CData::setBearingError( double _fdBearingError )
{
  _fdBearingError = fabs( _fdBearingError );
  if( _fdBearingError > 25.45 )
    ui32tBearingError = OVERFLOW_UINT32;
  else
    ui32tBearingError = (uint32_t)( _fdBearingError * 10.0 + 0.5 );
}

void CData::setGndSpeedError( double _fdGndSpeedError )
{
  _fdGndSpeedError = fabs( _fdGndSpeedError );
  if( _fdGndSpeedError > 25.45 )
    ui32tGndSpeedError = OVERFLOW_UINT32;
  else
    ui32tGndSpeedError = (uint32_t)( _fdGndSpeedError * 10.0 + 0.5 );
}

void CData::setVrtSpeedError( double _fdVrtSpeedError )
{
  _fdVrtSpeedError = fabs( _fdVrtSpeedError );
  if( _fdVrtSpeedError > 25.45 )
    ui32tVrtSpeedError = OVERFLOW_UINT32;
  else
    ui32tVrtSpeedError = (uint32_t)( _fdVrtSpeedError * 10.0 + 0.5 );
}

void CData::setBearingDtError( double _fdBearingDtError )
{
  _fdBearingDtError = fabs( _fdBearingDtError );
  if( _fdBearingDtError > 25.45 )
    ui32tBearingDtError = OVERFLOW_UINT32;
  else
    ui32tBearingDtError = (uint32_t)( _fdBearingDtError * 10.0 + 0.5 );
}

void CData::setGndSpeedDtError( double _fdGndSpeedDtError )
{
  _fdGndSpeedDtError = fabs( _fdGndSpeedDtError );
  if( _fdGndSpeedDtError > 25.45 )
    ui32tGndSpeedDtError = OVERFLOW_UINT32;
  else
    ui32tGndSpeedDtError = (uint32_t)( _fdGndSpeedDtError * 10.0 + 0.5 );
}

void CData::setVrtSpeedDtError( double _fdVrtSpeedDtError )
{
  _fdVrtSpeedDtError = fabs( _fdVrtSpeedDtError );
  if( _fdVrtSpeedDtError > 25.45 )
    ui32tVrtSpeedDtError = OVERFLOW_UINT32;
  else
    ui32tVrtSpeedDtError = (uint32_t)( _fdVrtSpeedDtError * 10.0 + 0.5 );
}

void CData::setHeadingError( double _fdHeadingError )
{
  _fdHeadingError = fabs( _fdHeadingError );
  if( _fdHeadingError > 25.45 )
    ui32tHeadingError = OVERFLOW_UINT32;
  else
    ui32tHeadingError = (uint32_t)( _fdHeadingError * 10.0 + 0.5 );
}

void CData::setAppSpeedError( double _fdAppSpeedError )
{
  _fdAppSpeedError = fabs( _fdAppSpeedError );
  if( _fdAppSpeedError > 25.45 )
    ui32tAppSpeedError = OVERFLOW_UINT32;
  else
    ui32tAppSpeedError = (uint32_t)( _fdAppSpeedError * 10.0 + 0.5 );
}


//
// GETTERS
//

void CData::getData( unsigned char *_pucData,
                     uint16_t *_pui16tDataSize ) const
{
  if( _pucData )
    memcpy( _pucData, pucData, ui16tDataSize );
  if( _pui16tDataSize )
    *_pui16tDataSize = ui16tDataSize;
}

double CData::getTime() const
{
  if( ui32tTime & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  return (double)ui32tTime * 0.1;
}

double CData::getLatitude() const
{
  if( ui32tLatitude & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tLatitude == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tLatitude == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tLatitude - 536870912.0 ) / 36.0 * 0.00001;
}

double CData::getLongitude() const
{
  if( ui32tLongitude & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tLongitude == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tLongitude == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tLongitude - 1073741824.0 ) / 36.0 * 0.00001;
}

double CData::getElevation() const
{
  if( ui32tElevation & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tElevation == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tElevation == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tElevation - 131072.0 ) * 0.1;
}

double CData::getBearing() const
{
  if( ui32tBearing & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tBearing == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tBearing * 0.1;
}

double CData::getGndSpeed() const
{
  if( ui32tGndSpeed & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tGndSpeed == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tGndSpeed == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tGndSpeed - 1.0 ) * 0.1;
}

double CData::getVrtSpeed() const
{
  if( ui32tVrtSpeed & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tVrtSpeed == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tVrtSpeed == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tVrtSpeed - 4096.0 ) * 0.1;
}

double CData::getBearingDt() const
{
  if( ui32tBearingDt & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tBearingDt == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tBearingDt == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tBearingDt - 512.0 ) * 0.1;
}

double CData::getGndSpeedDt() const
{
  if( ui32tGndSpeedDt & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tGndSpeedDt == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tGndSpeedDt == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tGndSpeedDt - 2048.0 ) * 0.1;
}

double CData::getVrtSpeedDt() const
{
  if( ui32tVrtSpeedDt & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tVrtSpeedDt == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tVrtSpeedDt == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tVrtSpeedDt - 2048.0 ) * 0.1;
}

double CData::getHeading() const
{
  if( ui32tHeading & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tHeading == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tHeading * 0.1;
}

double CData::getAppSpeed() const
{
  if( ui32tAppSpeed & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tAppSpeed == 0 )
    return -OVERFLOW_VALUE;
  if( ui32tAppSpeed == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return ( (double)ui32tAppSpeed - 1.0 ) * 0.1;
}

double CData::getLatitudeError() const
{
  if( ui32tLatitudeError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tLatitudeError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tLatitudeError * 0.1;
}

double CData::getLongitudeError() const
{
  if( ui32tLongitudeError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tLongitudeError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tLongitudeError * 0.1;
}

double CData::getElevationError() const
{
  if( ui32tElevationError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tElevationError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tElevationError * 0.1;
}

double CData::getBearingError() const
{
  if( ui32tBearingError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tBearingError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tBearingError * 0.1;
}

double CData::getGndSpeedError() const
{
  if( ui32tGndSpeedError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tGndSpeedError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tGndSpeedError * 0.1;
}

double CData::getVrtSpeedError() const
{
  if( ui32tVrtSpeedError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tVrtSpeedError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tVrtSpeedError * 0.1;
}

double CData::getBearingDtError() const
{
  if( ui32tBearingDtError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tBearingDtError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tBearingDtError * 0.1;
}

double CData::getGndSpeedDtError() const
{
  if( ui32tGndSpeedDtError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tGndSpeedDtError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tGndSpeedDtError * 0.1;
}

double CData::getVrtSpeedDtError() const
{
  if( ui32tVrtSpeedDtError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tVrtSpeedDtError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tVrtSpeedDtError * 0.1;
}

double CData::getHeadingError() const
{
  if( ui32tHeadingError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tHeadingError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tHeadingError * 0.1;
}

double CData::getAppSpeedError() const
{
  if( ui32tAppSpeedError & UNDEFINED_UINT32 )
    return UNDEFINED_VALUE;
  if( ui32tAppSpeedError == OVERFLOW_UINT32 )
    return OVERFLOW_VALUE;
  return (double)ui32tAppSpeedError * 0.1;
}


//
// OTHERS
//

uint16_t CData::allocData( uint16_t _ui16tDataSize )
{
  int __ui16tDataSize =
    ( _ui16tDataSize < 32767 )
    ? _ui16tDataSize
    : 32767;
  if( __ui16tDataSize != ui16tDataSize )
  {
    pucData =
      (unsigned char*)realloc( pucData,
                               __ui16tDataSize * sizeof( unsigned char ) );
    ui16tDataSize =
      pucData
      ? __ui16tDataSize
      : -1;
  }
  return ui16tDataSize;
}

void CData::freeData()
{
  if( pucData )
    free( pucData );
  pucData = NULL;
  ui16tDataSize = 0;
}

void CData::copy( const CData &_roData )
{
  if( &_roData == this )
    return;

  // Save pointers
  unsigned char *__pucData = this->pucData;

  // Copy all
  memcpy( this, &_roData, sizeof( CData ) );

  // Recover pointers
  this->pucData = __pucData;

  // Copy pointers content
  if( _roData.ui16tDataSize > 0 )
  {
    this->allocData( _roData.ui16tDataSize );
    memcpy( this->pucData, _roData.pucData, _roData.ui16tDataSize );
  }
  else
    this->freeData();
}

bool CData::sync( const CData &_roData )
{
  bool __bSynced = false;
  if( &_roData == this )
    return __bSynced;
  if( _roData.ui16tDataSize > 0 )
  {
    this->allocData( _roData.ui16tDataSize );
    memcpy( this->pucData, _roData.pucData, _roData.ui16tDataSize );
    __bSynced = true;
  }
  else if( this->ui16tDataSize > 0 )
  {
    this->freeData();
    __bSynced = true;
  }
  if( !( _roData.ui32tTime & UNDEFINED_UINT32 ) )
  {
    this->ui32tTime = _roData.ui32tTime;
    __bSynced = true;
  }
  if( !( _roData.ui32tLatitude & UNDEFINED_UINT32 ) )
  {
    this->ui32tLatitude = _roData.ui32tLatitude;
    __bSynced = true;
  }
  if( !( _roData.ui32tLongitude & UNDEFINED_UINT32 ) )
  {
    this->ui32tLongitude = _roData.ui32tLongitude;
    __bSynced = true;
  }
  if( !( _roData.ui32tElevation & UNDEFINED_UINT32 ) )
  {
    this->ui32tElevation = _roData.ui32tElevation;
    __bSynced = true;
  }
  if( !( _roData.ui32tBearing & UNDEFINED_UINT32 ) )
  {
    this->ui32tBearing = _roData.ui32tBearing;
    __bSynced = true;
  }
  if( !( _roData.ui32tGndSpeed & UNDEFINED_UINT32 ) )
  {
    this->ui32tGndSpeed = _roData.ui32tGndSpeed;
    __bSynced = true;
  }
  if( !( _roData.ui32tVrtSpeed & UNDEFINED_UINT32 ) )
  {
    this->ui32tVrtSpeed = _roData.ui32tVrtSpeed;
    __bSynced = true;
  }
  if( !( _roData.ui32tBearingDt & UNDEFINED_UINT32 ) )
  {
    this->ui32tBearingDt = _roData.ui32tBearingDt;
    __bSynced = true;
  }
  if( !( _roData.ui32tGndSpeedDt & UNDEFINED_UINT32 ) )
  {
    this->ui32tGndSpeedDt = _roData.ui32tGndSpeedDt;
    __bSynced = true;
  }
  if( !( _roData.ui32tVrtSpeedDt & UNDEFINED_UINT32 ) )
  {
    this->ui32tVrtSpeedDt = _roData.ui32tVrtSpeedDt;
    __bSynced = true;
  }
  if( !( _roData.ui32tHeading & UNDEFINED_UINT32 ) )
  {
    this->ui32tHeading = _roData.ui32tHeading;
    __bSynced = true;
  }
  if( !( _roData.ui32tAppSpeed & UNDEFINED_UINT32 ) )
  {
    this->ui32tAppSpeed = _roData.ui32tAppSpeed;
    __bSynced = true;
  }
  if( !( _roData.ui32tSourceType & UNDEFINED_UINT32 ) )
  {
    this->ui32tSourceType = _roData.ui32tSourceType;
    __bSynced = true;
  }
  if( !( _roData.ui32tLatitudeError & UNDEFINED_UINT32 ) )
  {
    this->ui32tLatitudeError = _roData.ui32tLatitudeError;
    __bSynced = true;
  }
  if( !( _roData.ui32tLongitudeError & UNDEFINED_UINT32 ) )
  {
    this->ui32tLongitudeError = _roData.ui32tLongitudeError;
    __bSynced = true;
  }
  if( !( _roData.ui32tElevationError & UNDEFINED_UINT32 ) )
  {
    this->ui32tElevationError = _roData.ui32tElevationError;
    __bSynced = true;
  }
  if( !( _roData.ui32tBearingError & UNDEFINED_UINT32 ) )
  {
    this->ui32tBearingError = _roData.ui32tBearingError;
    __bSynced = true;
  }
  if( !( _roData.ui32tGndSpeedError & UNDEFINED_UINT32 ) )
  {
    this->ui32tGndSpeedError = _roData.ui32tGndSpeedError;
    __bSynced = true;
  }
  if( !( _roData.ui32tVrtSpeedError & UNDEFINED_UINT32 ) )
  {
    this->ui32tVrtSpeedError = _roData.ui32tVrtSpeedError;
    __bSynced = true;
  }
  if( !( _roData.ui32tBearingDtError & UNDEFINED_UINT32 ) )
  {
    this->ui32tBearingDtError = _roData.ui32tBearingDtError;
    __bSynced = true;
  }
  if( !( _roData.ui32tGndSpeedDtError & UNDEFINED_UINT32 ) )
  {
    this->ui32tGndSpeedDtError = _roData.ui32tGndSpeedDtError;
    __bSynced = true;
  }
  if( !( _roData.ui32tVrtSpeedDtError & UNDEFINED_UINT32 ) )
  {
    this->ui32tVrtSpeedDtError = _roData.ui32tVrtSpeedDtError;
    __bSynced = true;
  }
  if( !( _roData.ui32tHeadingError & UNDEFINED_UINT32 ) )
  {
    this->ui32tHeadingError = _roData.ui32tHeadingError;
    __bSynced = true;
  }
  if( !( _roData.ui32tAppSpeedError & UNDEFINED_UINT32 ) )
  {
    this->ui32tAppSpeedError = _roData.ui32tAppSpeedError;
    __bSynced = true;
  }
  return __bSynced;
}
