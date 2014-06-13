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
#include <unordered_map>
#include <vector>
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
  : CSgctpUtilSkeleton( "ais2sgctp", _iArgC, _ppcArgV )
  , ptGpsDataT( NULL )
  , fdOutput( STDOUT_FILENO )
  , sInputHost( "localhost" )
  , sInputPort( "2947" )
  , sOutputPath( "-" )
  , bCallsignLookup( false )
  , bShipnameLookup( false )
  , iDataTTL( 3600 )
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
  cout << "  ais2sgctp [options] [<hostname(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given GPSD (AIS) host (default:localhost)." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:2947)" << endl;
  cout << "  -c, --callsign-lookup" << endl;
  cout << "    Use callsign instead of MMSI number" << endl;
  cout << "  -s, --shipname-lookup" << endl;
  cout << "    Use shipname instead of MMSI number" << endl;
  displayOptionPrincipal( false, true );
  displayOptionPayload( false, true );
  displayOptionPassword( false, true );
  displayOptionPasswordSalt( false, true );
  displayOptionExtendedContent();
  displayOptionDaemon();
  cout << "  --ttl <seconds>" << endl;
  cout << "    Internal data Time-To-Live/TTL (default:3600)" << endl;
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
      else if( __sArg=="-c" || __sArg=="--callsign-lookup" )
      {
        bCallsignLookup = true;
        bShipnameLookup = false;
      }
      else if( __sArg=="-s" || __sArg=="--shipname-lookup" )
      {
        bShipnameLookup = true;
        bCallsignLookup = false;
      }
      else if( __sArg=="--ttl" )
      {
        if( ++__i<iArgC )
          iDataTTL = atoi( ppcArgV[__i] );
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
  unordered_map<string,CData*> __poData_umap;
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

    // Loop through GPSD data
    double __fdEpochCleanup = CData::epoch();
    fd_set __tFdSet_read;
    FD_SET( ptGpsDataT->gps_fd, &__tFdSet_read );
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

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

      // Check AIS data status
      if( !( ptGpsDataT->set & AIS_SET ) )
        continue; // We need a valid AIS packet
      if( !ptGpsDataT->ais.mmsi )
        continue; // We need a valid MMSI number
      double __fdEpochNow = CData::epoch();

      // Originating source (ID)
      string __sSource = to_string( ptGpsDataT->ais.mmsi );

      // Data map cleanup
      if( __fdEpochNow - __fdEpochCleanup > (double)300.0 )
      {
        __fdEpochCleanup = __fdEpochNow;

        // ... get keys
        vector<string> __vsKeys;
        __vsKeys.reserve( __poData_umap.size() );
        for( unordered_map<string,CData*>::const_iterator __it =
               __poData_umap.begin();
             __it != __poData_umap.end();
             ++__it )
        {
          __vsKeys.push_back( __it->first );
        }

        // ... loop through keys
        for( vector<string>::const_iterator __it = __vsKeys.begin();
             __it != __vsKeys.end();
             ++__it )
        {
          // ... cleanup stale entries (older than data TTL)
          if( __fdEpochNow - CData::toEpoch( __poData_umap[ *__it ]->getTime() )
              > (double)iDataTTL )
          {
            delete __poData_umap[ *__it ];
            __poData_umap.erase( *__it );
          }
        }

      }

      // Data map lookup
      CData* __poData;
      unordered_map<string,CData*>::const_iterator __it =
        __poData_umap.find( __sSource );
      if( __it == __poData_umap.end() )
      {
        __poData = new CData();
        if( bExtendedContent )
          __poData->setSourceType( CData::SOURCE_AIS );
        __poData_umap[__sSource] = __poData;
      }
      else
        __poData = __it->second;

      // Parse AIS data
      // NOTE: we handle only message types: 1, 2, 3, 5, 18, 19, 24
      bool __bDataAvailable = false;
      int __iSecond = AIS_SEC_NOT_AVAILABLE;
      int __iLongitude = AIS_LON_NOT_AVAILABLE;
      int __iLatitude = AIS_LAT_NOT_AVAILABLE;
      int __iCourse = AIS_COURSE_NOT_AVAILABLE;
      int __iSpeed = AIS_SPEED_NOT_AVAILABLE;
      string __sShipname;
      string __sCallsign;
      switch( ptGpsDataT->ais.type )
      {

      case 1:
      case 2:
      case 3:
        if( ptGpsDataT->ais.type1.second < 60 )
          __iSecond = ptGpsDataT->ais.type1.second;
        if( ptGpsDataT->ais.type1.lon != AIS_LON_NOT_AVAILABLE )
          __iLongitude = ptGpsDataT->ais.type1.lon;
        if( ptGpsDataT->ais.type1.lat != AIS_LAT_NOT_AVAILABLE )
          __iLatitude = ptGpsDataT->ais.type1.lat;
        if( ptGpsDataT->ais.type1.course < 3600 )
          __iCourse = ptGpsDataT->ais.type1.course;
        if( ptGpsDataT->ais.type1.speed < 1000 )
          __iSpeed = ptGpsDataT->ais.type1.speed;
        break;

      case 5:
        __sShipname =  ptGpsDataT->ais.type5.shipname;
        __sCallsign =  ptGpsDataT->ais.type5.callsign;
        break;

      case 18:
        if( ptGpsDataT->ais.type18.second < 60 )
          __iSecond = ptGpsDataT->ais.type18.second;
        if( ptGpsDataT->ais.type18.lon != AIS_LON_NOT_AVAILABLE )
          __iLongitude = ptGpsDataT->ais.type18.lon;
        if( ptGpsDataT->ais.type18.lat != AIS_LAT_NOT_AVAILABLE )
          __iLatitude = ptGpsDataT->ais.type18.lat;
        if( ptGpsDataT->ais.type18.course < 3600 )
          __iCourse = ptGpsDataT->ais.type18.course;
        if( ptGpsDataT->ais.type18.speed < 1000 )
          __iSpeed = ptGpsDataT->ais.type18.speed;
        break;

      case 19:
        if( ptGpsDataT->ais.type19.second < 60 )
          __iSecond = ptGpsDataT->ais.type19.second;
        if( ptGpsDataT->ais.type19.lon != AIS_LON_NOT_AVAILABLE )
          __iLongitude = ptGpsDataT->ais.type19.lon;
        if( ptGpsDataT->ais.type19.lat != AIS_LAT_NOT_AVAILABLE )
          __iLatitude = ptGpsDataT->ais.type19.lat;
        if( ptGpsDataT->ais.type19.course < 3600 )
          __iCourse = ptGpsDataT->ais.type19.course;
        if( ptGpsDataT->ais.type19.speed < 1000 )
          __iSpeed = ptGpsDataT->ais.type19.speed;
        break;

      case 24:
        __sShipname =  ptGpsDataT->ais.type24.shipname;
        __sCallsign =  ptGpsDataT->ais.type24.callsign;
        break;

      default:
        continue;

      }

      // ... ID
      if( bCallsignLookup )
      {
        if( !__sCallsign.empty() )
          __poData->setID( __sCallsign.c_str() );
      }
      else if( bShipnameLookup )
      {
        if( !__sShipname.empty() )
          __poData->setID( __sShipname.c_str() );
      }
      else if( !strlen( __poData->getID() ) )
        __poData->setID( __sSource.c_str() );
      if( ptGpsDataT->ais.type == 5 || ptGpsDataT->ais.type == 24 )
        continue;

      // ... time
      if( __iSecond != AIS_SEC_NOT_AVAILABLE )
      {
        uint __iCurrentTime = __fdEpochNow;
        int __iCurrentSecond = __iCurrentTime % 60;
        __iCurrentTime -= __iCurrentSecond;
        __iCurrentTime += __iSecond;
        if( __iSecond > 45 && __iCurrentSecond < 15 )
          __iCurrentTime -= 60;
        __poData->setTime( __iCurrentTime );
      }

      // ... position
      if( __iLongitude != AIS_LON_NOT_AVAILABLE
          && __iLatitude != AIS_LAT_NOT_AVAILABLE )
      {
        __poData->setLongitude( (double)__iLongitude / 600000.0 );
        __poData->setLatitude( (double)__iLatitude / 600000.0 );
        __bDataAvailable = true;
      }

      // ... course
      if( __iCourse != AIS_COURSE_NOT_AVAILABLE )
      {
        __poData->setBearing( (double)__iCourse / 10.0 ); // decidegrees
        __bDataAvailable = true;
      }

      // ... speed
      if( __iSpeed != AIS_SPEED_NOT_AVAILABLE && __iSpeed > 0 )
      {
        __poData->setGndSpeed( (double)__iSpeed / 19.4384449244 ); // deciknots
        __bDataAvailable = true;
      }

      // Dump SGCTP data

      //  ... data available
      if( !__bDataAvailable )
        continue;

      // ... ID ?
      if( !strlen( __poData->getID() ) )
        continue;

      // ... serialize
      sigBlock();
      __iReturn = oTransmit_out.serialize( fdOutput, *__poData );
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
  for( unordered_map<string,CData*>::const_iterator __it =
         __poData_umap.begin();
       __it != __poData_umap.end();
       ++__it )
    delete __it->second;
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
