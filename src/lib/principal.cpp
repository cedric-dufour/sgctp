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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C++
#include <set>
using namespace std;

// SGCTP
#include "sgctp/principal.hpp"
#include "sgctp/transmit.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

//
// SETTERS
//

void CPrincipal::reset()
{
  ui64tID = 0;
  memset( pcPassword, '\0', SGCTP_MAX_PASSWORD_SIZE );
  ui8tPayloadType_set.clear();
}

void CPrincipal::erasePassword()
{
  memset( pcPassword, '\0', SGCTP_MAX_PASSWORD_SIZE );
}

void CPrincipal::setPassword( const char *_pcPassword )
{
  uint8_t __ui8tPasswordLength =
    ( strlen( _pcPassword ) < SGCTP_MAX_PASSWORD_LENGTH )
    ? strlen( _pcPassword )
    : SGCTP_MAX_PASSWORD_LENGTH;
  memcpy( pcPassword, _pcPassword, __ui8tPasswordLength );
  pcPassword[__ui8tPasswordLength] = '\0';
}

void CPrincipal::addPayloadType( uint8_t _ui8tPayloadType )
{
  ui8tPayloadType_set.insert( _ui8tPayloadType );
}

//
// GETTERS
//

bool CPrincipal::hasPayloadTypes() const
{
  return !ui8tPayloadType_set.empty();
}

bool CPrincipal::hasPayloadType( uint8_t _ui8tPayloadType ) const
{
  return
    ui8tPayloadType_set.find( _ui8tPayloadType )
    != ui8tPayloadType_set.end();
}

//
// OTHER
//

int CPrincipal::read( const char *_pcPrincipalsPath, uint64_t _ui64tID )
{
  reset();

  // Open file
  FILE *__pFILE = fopen( _pcPrincipalsPath, "r" );
  if( __pFILE == NULL )
    return -errno;

  // Parse file
  char *__pcBuffer = NULL, *__pcDelim1, *__pcDelim2, *__pcDelim3;
  size_t __sizeBuffer;
  ssize_t __ssizeLine;
  for(;;)
  {
    // retrieve line
    __ssizeLine = getline( &__pcBuffer, &__sizeBuffer, __pFILE );
    if( __ssizeLine < 0 )
      break;

    // trim new-line
    *strchrnul( __pcBuffer, '\r' ) = '\0';
    *strchrnul( __pcBuffer, '\n' ) = '\0';

    // trim comments
    *strchrnul( __pcBuffer, '#' ) = '\0';
    *strchrnul( __pcBuffer, ' ' ) = '\0';

    // parse fields
    char *__pcFieldEnd;

    // ... first field: ID
    __pcDelim1 = strchr( __pcBuffer, ':' );
    if( __pcDelim1 == NULL )
      continue; // missing delimiter
    if( __pcDelim1 - __pcBuffer > 20 )
      continue; // invalid (too long) field
    *__pcDelim1 = '\0';
    unsigned long long int __ullID =
      strtoull( __pcBuffer, &__pcFieldEnd, 10 );
    if( *__pcFieldEnd != '\0' )
      continue; // invalid syntax
    if( __ullID != _ui64tID )
      continue; // non-matching ID
    ui64tID = __ullID;

    // ... second field: password
    __pcDelim2 = strchr( __pcDelim1+1, ':' );
    if( __pcDelim2 == NULL )
      continue; // missing delimiter
    if( __pcDelim2 - __pcDelim1 > SGCTP_MAX_PASSWORD_LENGTH )
      continue; // invalid (too long) field
    *__pcDelim2 = '\0';
    strcpy( pcPassword, __pcDelim1+1 );

    // ... third field: payload type(s)
    __pcDelim3 = __pcDelim2+1;
    while( *__pcDelim3 != '\0' )
    {
      if( *__pcDelim3 == ',' )
        __pcDelim3++; // skip comma
      unsigned long int __ulPayloadType =
        strtoul( __pcDelim3, &__pcDelim3, 10 );
      if( *__pcDelim3 != ',' && *__pcDelim3 != '\0' )
        continue; // invalid syntax
      if( __ulPayloadType > CTransmit::PAYLOAD_UNDEFINED )
        continue; // invalid value
      ui8tPayloadType_set.insert( __ulPayloadType );
    }
  }

  // Free resources
  if( __pcBuffer )
  {
    memset( __pcBuffer, '\0', __sizeBuffer );
    free( __pcBuffer );
  }
  fclose( __pFILE );

  // Done
  return
    ( ui64tID == 0 && _ui64tID != 0 )
    ? -EACCES
    : 0;
}
