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

// C/C++
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

// SGCTP
#include "sgctp/data.hpp"
#include "sgctp/parameters.hpp"
#include "sgctp/payload.hpp"
#include "sgctp/payload_aes128.hpp"
#include "sgctp/principal.hpp"
#include "sgctp/transmit_file.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  // Data
  CData __oData_IN;
  __oData_IN.setTime( CData::epoch() );
  __oData_IN.setLongitude( 7.085140 );
  __oData_IN.setLatitude( 46.108600 );
  __oData_IN.setElevation( 473 );
  __oData_IN.setBearing( 55.5 );
  __oData_IN.setGndSpeed( 11.1 );
  __oData_IN.setVrtSpeed( 5.5 );
  __oData_IN.setBearingDt( 3.3 );
  __oData_IN.setGndSpeedDt( 2.2 );
  __oData_IN.setVrtSpeedDt( 1.1 );
  __oData_IN.setHeading( 50.5 );
  __oData_IN.setAppSpeed( 10.1 );
  __oData_IN.setID( "SGCTP" );
  char __pcData[] = "This is some very long data: 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789";
  __oData_IN.setData( (unsigned char*)__pcData, strlen( __pcData ) );

  // File dump
  int __fd;
  CTransmit_File __oTransmit_File;
  __oTransmit_File.usePrincipal()->setID( 10 );
  __oTransmit_File.usePrincipal()->setPassword( "password" );
  __oTransmit_File.setPasswordSalt( (unsigned char*)"1234567890123456" );
  // ... raw payload
  CData __oData_File_RAW;
  __oTransmit_File.initPayload( CTransmit::PAYLOAD_RAW );
  __fd = open( "sgctp-file-raw.dat", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR );
  __oTransmit_File.serialize( __fd, __oData_IN );
  __oTransmit_File.free();
  close( __fd );
  __fd = open( "sgctp-file-raw.dat", O_RDONLY );
  __oTransmit_File.unserialize( __fd, &__oData_File_RAW );
  __oTransmit_File.free();
  close( __fd );
  // ... AES128 payload
  CData __oData_File_AES128;
  __oTransmit_File.initPayload( CTransmit::PAYLOAD_AES128 );
  __fd = open( "sgctp-file-aes128.dat", O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR );
  __oTransmit_File.serialize( __fd, __oData_IN );
  __oTransmit_File.free();
  close( __fd );
  __fd = open( "sgctp-file-aes128.dat", O_RDONLY );
  __oTransmit_File.unserialize( __fd, &__oData_File_AES128 );
  __oTransmit_File.free();
  close( __fd );
  __oTransmit_File.freePayload();

  // Done
  return 0;
}
