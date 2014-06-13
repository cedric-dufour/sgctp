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
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>

// C++
#include <cmath>
using namespace std;

// GPSD
#include "gps.h"
#if GPSD_API_MAJOR_VERSION < 4
#error "Unsupported GPSD API version (<4)"
#endif

// SGCTP
#include "main.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// STATIC / CONSTANTS
//----------------------------------------------------------------------

void CSgctpUtil::interrupt( int _iSignal )
{
  SGCTP_INTERRUPTED = 1;
}


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtil::CSgctpUtil( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "gps2sgctp", _iArgC, _ppcArgV )
  , ptGpsDataT( NULL )
  , fdOutput( STDOUT_FILENO )
  , sInputHost( "localhost" )
  , sInputPort( "2947" )
  , sOutputPath( "-" )
  , sID( "" )
  , fdFixTTL( 0.1 )
  , b2DFixOnly( false )
{
  // Link actual transmission objects
  poTransmit_out = &oTransmit_out;
}


//----------------------------------------------------------------------
// METHODS: CSgctpUtilSkeleton (implement/override)
//----------------------------------------------------------------------

void CSgctpUtil::displayHelp()
{
  // Display full help
  displayUsageHeader();
  cout << "  gps2sgctp [options] [<hostname(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given GPSD host (default:localhost)." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:2947)" << endl;
  cout << "  -i, --gpsd-id <ID>" << endl;
  cout << "    GPSD device ID (default:GPSD device name)" << endl;
  cout << "  -t, --fix-ttl <seconds>" << endl;
  cout << "    Fix data time-to-live/TTL (default:0.1)" << endl;
  cout << "  -2, --2d-only" << endl;
  cout << "    Do not wait for consistent 3D fix" << endl;
  displayOptionPrincipal( false, true );
  displayOptionPayload( false, true );
  displayOptionPassword( false, true );
  displayOptionPasswordSalt( false, true );
  displayOptionExtendedContent();
  displayOptionDaemon();
}

int CSgctpUtil::parseArgs()
{
  // Parse all arguments
  int __iArgCount = 0;
  for( int __i=1; __i<iArgC; __i++ )
  {
    string __sArg = ppcArgV[__i];
    if( __sArg[0] == '-' )
    {
      SGCTP_PARSE_ARGS( parseArgsHelp( &__i ) );
      SGCTP_PARSE_ARGS( parseArgsPrincipal( &__i, false, true ) );
      SGCTP_PARSE_ARGS( parseArgsPayload( &__i, false, true ) );
      SGCTP_PARSE_ARGS( parseArgsPassword( &__i, false, true ) );
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, false, true ) );
      SGCTP_PARSE_ARGS( parseArgsExtendedContent( &__i ) );
      SGCTP_PARSE_ARGS( parseArgsDaemon( &__i ) );
      if( __sArg=="-o" || __sArg=="--output" )
      {
        if( ++__i<iArgC )
          sOutputPath = ppcArgV[__i];
      }
      else if( __sArg=="-p" || __sArg=="--port" )
      {
        if( ++__i<iArgC )
          sInputPort = ppcArgV[__i];
      }
      else if( __sArg=="-i" || __sArg=="--gpsd-id" )
      {
        if( ++__i<iArgC )
          sID = ppcArgV[__i];
      }
      else if( __sArg=="-t" || __sArg=="--fix-ttl" )
      {
        if( ++__i<iArgC )
          fdFixTTL = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-2" || __sArg=="--2d-only" )
      {
        b2DFixOnly = true;
      }
      else if( __sArg[0] == '-' )
      {
        displayErrorInvalidOption( __sArg );
        return -EINVAL;
      }
      if( __i == iArgC )
      {
        displayErrorMissingArgument ( __sArg );
        return -EINVAL;
      }
    }
    else
    {
      switch( __iArgCount++ )
      {
      case 0:
        sInputHost = __sArg;
        break;
      default:
        displayErrorExtraArgument( __sArg );
        return -EINVAL;
      }
    }
  }

  // Done
  return 0;
}

#define SGCTP_BREAK( __iExit__ ) { SGCTP_INTERRUPTED=1; __iExit = __iExit__; break; }
int CSgctpUtil::exec()
{
  int __iExit = 0;
  int __iReturn;

  // Parse arguments
  __iReturn = parseArgs();
  if( __iReturn )
    return __iReturn;

  // Lookup principal(s)
  __iReturn = principalLookup( false, true );
  if( __iReturn )
    return __iReturn;

  // Initialize transmission object(s)
  __iReturn = transmitInit( false, true );
  if( __iReturn )
    return __iReturn;

  // Daemonize
  if( bDaemon )
  {
    if( sOutputPath == "-" )
    {
      SGCTP_LOG << SGCTP_ERROR << "Output must not be standard output" << endl;
      return -EINVAL;
    }
    if( sOutputPath[0] != '/' )
    {
      SGCTP_LOG << SGCTP_ERROR << "Output path must be absolute" << endl;
      return -EINVAL;
    }
  }
  daemonStart();

  // Catch signals
  sigCatch( CSgctpUtil::interrupt );

  // Error-catching block
  do
  {

    // Open input (GPSD)

#if GPSD_API_MAJOR_VERSION >= 5

    __iReturn = gps_open( sInputHost.c_str(), sInputPort.c_str(), &tGpsDataT );
    ptGpsDataT = &tGpsDataT;

#else

    ptGpsDataT = gps_open( sInputHost.c_str(), sInputPort.c_str() );
    if( !ptGpsDataT ) __iReturn = -1;

#endif

    if( __iReturn < 0 )
    {
      ptGpsDataT = NULL;
      SGCTP_LOG << SGCTP_ERROR << "Failed to connect to GPSD (" << sInputHost << ":" << sInputPort << ") @ gps_open=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }
    __iReturn = gps_stream( ptGpsDataT, WATCH_ENABLE|WATCH_NEWSTYLE, NULL );
    if( __iReturn < 0 )
    {
      SGCTP_LOG << SGCTP_ERROR << "Failed to connect to GPSD (" << sInputHost << ":" << sInputPort << ") @ gps_stream=" << __iReturn << endl;
      SGCTP_BREAK( __iReturn );
    }

    // Open output (POSIX file descriptor)
    if( sOutputPath != "-" )
    {
      fdOutput = open( sOutputPath.c_str(),
                       O_CREAT|O_TRUNC|O_WRONLY,
                       S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH );
      if( fdOutput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid/unwritable file (" << sOutputPath << ") @ open=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }
    }

    // Receive and dump data
    CData __oData;
    double __fdEpoch2DFix = 0.0;
    double __fdEpoch3DFix = 0.0;
    double __fdEpochFix = 0.0;
    if( bExtendedContent ) __oData.setSourceType( CData::SOURCE_GPS );
    fd_set __tFdSet_read;
    FD_SET( ptGpsDataT->gps_fd, &__tFdSet_read );
    for(;;)
    {
      if( SGCTP_INTERRUPTED ) break;

      // Wait for data
      __iReturn = select( ptGpsDataT->gps_fd+1, &__tFdSet_read,
                          NULL, NULL, NULL );
      if( __iReturn <= 0 )
        break;

      // Retrieve data

#if GPSD_API_MAJOR_VERSION >= 5

      __iReturn = gps_read( ptGpsDataT );

#else

      __iReturn = gps_poll( ptGpsDataT );

#endif

      if( __iReturn == 0 )
        continue; // No data available
      if( __iReturn < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve GPSD data @ gps_read=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

      // Check data status
      if( !( ptGpsDataT->set & PACKET_SET ) )
        continue; // We need a valid packet

      // Check device data
      // NOTE: ( ptGpsDataT->set & DEVICE_SET ) is unreliable; let's use an alternate method
      if( ptGpsDataT->dev.path[0] == '\0' )
        continue; // We MUST have a device/source name
      double __fdEpochNow = CData::epoch();

      // Originating source (ID)
      string __sSource = ptGpsDataT->dev.path;
      __oData.setID( sID.empty()
                     ? __sSource.c_str()
                     : sID.c_str() );

      // Check position/course data
      bool __bDataAvailable = false;
      if( ptGpsDataT->set
          & (TIME_SET|LATLON_SET|ALTITUDE_SET|TRACK_SET|SPEED_SET|CLIMB_SET) )
      {
        // ... time
        if( !std::isnan( ptGpsDataT->fix.time )
            && ptGpsDataT->fix.time > 0.0 )
          __oData.setTime( ptGpsDataT->fix.time );

        // ... position/elevation
        if( !std::isnan( ptGpsDataT->fix.longitude )
            && !std::isnan( ptGpsDataT->fix.latitude ) )
        {
          __oData.setLongitude( ptGpsDataT->fix.longitude );
          __oData.setLatitude( ptGpsDataT->fix.latitude );
          if( !std::isnan( ptGpsDataT->fix.altitude ) )
          {
            __oData.setElevation( ptGpsDataT->fix.altitude );
            __fdEpoch3DFix = __fdEpochNow;
          }
          __bDataAvailable = true;
        }

        // ... ground course
        if( !std::isnan( ptGpsDataT->fix.speed )
            && fabs( ptGpsDataT->fix.speed ) > 0.0 )
        {
          __oData.setGndSpeed( ptGpsDataT->fix.speed );
          if( !std::isnan( ptGpsDataT->fix.track ) )
          {
            __oData.setBearing( ptGpsDataT->fix.track );
            __fdEpoch2DFix = __fdEpochNow;
          }
          __bDataAvailable = true;
        }
        if( !std::isnan( ptGpsDataT->fix.climb )
            && fabs( ptGpsDataT->fix.climb ) > 0.0 )
        {
          __oData.setVrtSpeed( ptGpsDataT->fix.climb );
          __bDataAvailable = true;
        }

        // ... errors
        if( bExtendedContent )
        {
          // ... latitude
          if( !std::isnan( ptGpsDataT->fix.epy ) && ptGpsDataT->fix.epy > 0.0 )
            __oData.setLatitudeError( ptGpsDataT->fix.epy );
          // ... longitude
          if( !std::isnan( ptGpsDataT->fix.epx ) && ptGpsDataT->fix.epx > 0.0 )
            __oData.setLongitudeError( ptGpsDataT->fix.epx );
          // ... elevation
          if( !std::isnan( ptGpsDataT->fix.epv ) && ptGpsDataT->fix.epv > 0.0 )
            __oData.setElevationError( ptGpsDataT->fix.epv );
          // ... bearing
          if( !std::isnan( ptGpsDataT->fix.epd ) && ptGpsDataT->fix.epd > 0.0 )
            __oData.setBearingError( ptGpsDataT->fix.epd );
          // ... ground speed
          if( !std::isnan( ptGpsDataT->fix.eps ) && ptGpsDataT->fix.eps > 0.0 )
            __oData.setGndSpeedError( ptGpsDataT->fix.eps );
          // ... vertical speed
          if( !std::isnan( ptGpsDataT->fix.epc ) && ptGpsDataT->fix.epc > 0.0 )
            __oData.setVrtSpeedError( ptGpsDataT->fix.epc );
        }

      }

      // Dump SGCTP data

      // ... data available
      if( !__bDataAvailable )
        continue;

      // ... 2D/3D fix
      if( !b2DFixOnly && fabs( __fdEpoch3DFix - __fdEpoch2DFix ) > fdFixTTL )
        continue;

      // ... fix TTL
      if( __fdEpochNow - __fdEpochFix <= fdFixTTL )
        continue;
      __fdEpochFix = __fdEpochNow;

      // ... serialize
      sigBlock();
      __iReturn = oTransmit_out.serialize( fdOutput, __oData );
      sigUnblock();
      if( __iReturn == 0 )
        SGCTP_BREAK( 0 );
      if( __iReturn < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to serialize data @ serialize=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

    } // Loop through GPSD data

  }
  while( false ); // Error-catching block

  // Done
  if( ptGpsDataT )
  {
    gps_stream( ptGpsDataT, WATCH_DISABLE, NULL );
    gps_close( ptGpsDataT );
  }
  if( fdOutput >= 0 && fdOutput != STDOUT_FILENO )
    close( fdOutput );
  daemonEnd();
  return __iExit;
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpUtil __oSgctpUtil( argc, argv );
  return __oSgctpUtil.exec();
}
