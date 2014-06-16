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
#include <sys/socket.h>

// C++
#include <fstream>
using namespace std;

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
  : CSgctpUtilSkeleton( "hub2sgctp", _iArgC, _ppcArgV )
  , sdInput( -1 )
  , ptAddrinfo_in( NULL )
  , fdOutput( STDOUT_FILENO )
  , sInputHost( "localhost" )
  , sInputPort( "8948" )
  , sOutputPath( "-" )
  , fdLatitude0( CData::UNDEFINED_VALUE )
  , fdLongitude0( CData::UNDEFINED_VALUE )
  , fdTime1( CData::UNDEFINED_VALUE )
  , fdLatitude1( CData::UNDEFINED_VALUE )
  , fdLongitude1( CData::UNDEFINED_VALUE )
  , fdElevation1( CData::UNDEFINED_VALUE )
  , fdTime2( CData::UNDEFINED_VALUE )
  , fdLatitude2( CData::UNDEFINED_VALUE )
  , fdLongitude2( CData::UNDEFINED_VALUE )
  , fdElevation2( CData::UNDEFINED_VALUE )
{
  // Link actual transmission objects
  poTransmit_in = &oTransmit_in;
  poTransmit_out = &oTransmit_out;
}


//----------------------------------------------------------------------
// METHODS: CSgctpUtilSkeleton (implement/override)
//----------------------------------------------------------------------

void CSgctpUtil::displayHelp()
{
  // Display full help
  displayUsageHeader();
  cout << "  hub2sgctp [options] [<hostname(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given SGCTP hub (default:localhost)." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:8948)" << endl;
  cout << "  -lat0, --latitude-reference <latitude>"   << endl;
  cout << "    Filter reference point latitude (default:none, degrees)"   << endl;
  cout << "  -lon0, --longitude-reference <longitude>"   << endl;
  cout << "    Filter reference point longitude (default:none, degrees)"   << endl;
  cout << "  -t1, --time-limit-1 <time>"   << endl;
  cout << "    Filter 1st limit time (default:none, seconds)"   << endl;
  cout << "  -lat1, --latitude-limit-1 <latitude>"   << endl;
  cout << "    Filter 1st limit latitude (default:none, degrees)"   << endl;
  cout << "  -lon1, --longitude-limit-1 <longitude>"   << endl;
  cout << "    Filter 1st limit longitude (default:none, degrees)"   << endl;
  cout << "  -ele1, --elevation-limit-1 <elevation>"   << endl;
  cout << "    Filter 1st limit elevation (default:none, meters)"   << endl;
  cout << "  -t2, --time-limit-2 <time>"   << endl;
  cout << "    Filter 2nd limit time (default:none, seconds)"   << endl;
  cout << "  -lat2, --latitude-limit-2 <latitude>"   << endl;
  cout << "    Filter 2nd limit latitude (default:none, degrees)"   << endl;
  cout << "  -lon2, --longitude-limit-2 <longitude>"   << endl;
  cout << "    Filter 2nd limit longitude (default:none, degrees)"   << endl;
  cout << "  -ele2, --elevation-limit-2 <elevation>"   << endl;
  cout << "    Filter 2nd limit elevation (default:none, meters)"   << endl;
  displayOptionPrincipal( true, true );
  displayOptionPayload( true, true );
  displayOptionPassword( true, true );
  displayOptionPasswordSalt( false, true );
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
      SGCTP_PARSE_ARGS( parseArgsPrincipal( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPayload( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPassword( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, false, true ) );
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
      else if( __sArg=="-lat0" || __sArg=="--latitude-reference" )
      {
        if( ++__i<iArgC )
          fdLatitude0 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-lon0" || __sArg=="--longitude-reference" )
      {
        if( ++__i<iArgC )
          fdLongitude0 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-t1" || __sArg=="--time-limit-1" )
      {
        if( ++__i<iArgC )
          fdTime1 = CData::fromIso8601( ppcArgV[__i] );
      }
      else if( __sArg=="-lat1" || __sArg=="--latitude-limit-1" )
      {
        if( ++__i<iArgC )
          fdLatitude1 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-lon1" || __sArg=="--longitude-limit-1" )
      {
        if( ++__i<iArgC )
          fdLongitude1 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-ele1" || __sArg=="--elevation-limit-1" )
      {
        if( ++__i<iArgC )
          fdElevation1 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-t2" || __sArg=="--time-limit-2" )
      {
        if( ++__i<iArgC )
          fdTime2 = CData::fromIso8601( ppcArgV[__i] );
      }
      else if( __sArg=="-lat2" || __sArg=="--latitude-limit-2" )
      {
        if( ++__i<iArgC )
          fdLatitude2 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-lon2" || __sArg=="--longitude-limit-2" )
      {
        if( ++__i<iArgC ) fdLongitude2 = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-ele2" || __sArg=="--elevation-limit-2" )
      {
        if( ++__i<iArgC ) fdElevation2 = strtod( ppcArgV[__i], NULL );
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
  __iReturn = principalLookup( true, true );
  if( __iReturn )
    return __iReturn;

  // Initialize transmission object(s)
  __iReturn = transmitInit( true, true );
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

    // Open input (TCP socket)
    struct addrinfo* __ptAddrinfoActual_in;
    {

      // ... lookup scoket address info
      struct addrinfo __tAddrinfoHints;
      memset( &__tAddrinfoHints, 0, sizeof( addrinfo ) );
      __tAddrinfoHints.ai_family = AF_UNSPEC;
      __tAddrinfoHints.ai_socktype = SOCK_STREAM;
      __iReturn = getaddrinfo( sInputHost.c_str(),
                               sInputPort.c_str(),
                               &__tAddrinfoHints,
                               &ptAddrinfo_in );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve socket address info (" << sInputHost << ":" << sInputPort << ") @ getaddrinfo=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

    }

    // Open output
    if( sOutputPath != "-" )
    {
      fdOutput = open( sOutputPath.c_str(),
                       O_CREAT|O_TRUNC|O_WRONLY,
                       S_IRUSR|S_IWUSR );
      if( fdOutput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid/unwritable file (" << sOutputPath << ") @ open=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }
    }

    // Loop through TCP (re-)connections
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

      // Error-catching block
      do
      {

        // Connect to host

        // ... create socket
        for( __ptAddrinfoActual_in = ptAddrinfo_in;
             __ptAddrinfoActual_in != NULL;
             __ptAddrinfoActual_in = __ptAddrinfoActual_in->ai_next )
        {
          sdInput = socket( __ptAddrinfoActual_in->ai_family,
                             __ptAddrinfoActual_in->ai_socktype,
                             __ptAddrinfoActual_in->ai_protocol );
          if( sdInput < 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to create socket (" << sInputHost << ":" << sInputPort << ") @ socket=" << -errno << endl;
            continue;
          }
          break;
        }
        if( sdInput < 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to create socket (" << sInputHost << ":" << sInputPort << ")" << endl;
          break;
        }

        // ... connect socket
        __iReturn = connect( sdInput,
                             ptAddrinfo_in->ai_addr,
                             ptAddrinfo_in->ai_addrlen );
        if( __iReturn )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to connect socket (" << sInputHost << ":" << sInputPort << ") @ connect=" << -errno << endl;
          break;
        }

        // Send handshake
        __iReturn = oTransmit_in.sendHandshake( sdInput );
        if( __iReturn <= 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to send TCP handshake @ sendHandshake=" << __iReturn << endl;
          break;
        }

        // Send filter options
        CData __oData;

        // ... #FLT0
        if( CData::isDefined( fdLatitude0 )
            && CData::isDefined( fdLongitude0 ) )
        {
          __oData.reset();
          __oData.setID( "#FLT0" );
          // ... set filter data
          if( CData::isDefined( fdLatitude0 )
              && CData::isDefined( fdLongitude0 ) )
          {
            __oData.setLatitude( fdLatitude0 );
            __oData.setLongitude( fdLongitude0 );
          }
          // ... send filter data
          __iReturn = oTransmit_in.serialize( sdInput, __oData );
          if( __iReturn <= 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to send filter data (#0) @ serialize=" << __iReturn << endl;
            break;
          }
        }

        // ... #FLT1
        if( CData::isDefined( fdTime1 )
            || ( CData::isDefined( fdLatitude1 )
                 && CData::isDefined( fdLongitude1 ) )
            || CData::isDefined( fdElevation1 ) )
        {
          __oData.reset();
          __oData.setID( "#FLT1" );
          // ... set filter data
          if( CData::isDefined( fdTime1 ) )
            __oData.setTime( fdTime1 );
          if( CData::isDefined( fdLatitude1 )
              && CData::isDefined( fdLongitude1 ) )
          {
            __oData.setLatitude( fdLatitude1 );
            __oData.setLongitude( fdLongitude1 );
          }
          if( CData::isDefined( fdElevation1 ) )
            __oData.setTime( fdElevation1 );
          // ... send filter data
          __iReturn = oTransmit_in.serialize( sdInput, __oData );
          if( __iReturn <= 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to send filter data (#1) @ serialize=" << __iReturn << endl;
            break;
          }
        }

        // ... #FLT2
        if( CData::isDefined( fdTime2 )
            || ( CData::isDefined( fdLatitude2 )
                 && CData::isDefined( fdLongitude2 ) )
            || CData::isDefined( fdElevation2 ) )
        {
          __oData.reset();
          __oData.setID( "#FLT2" );
          // ... set filter data
          if( CData::isDefined( fdTime2 ) )
            __oData.setTime( fdTime2 );
          if( CData::isDefined( fdLatitude2 )
              && CData::isDefined( fdLongitude2 ) )
          {
            __oData.setLatitude( fdLatitude2 );
            __oData.setLongitude( fdLongitude2 );
          }
          if( CData::isDefined( fdElevation2 ) )
            __oData.setTime( fdElevation2 );
          // ... send filter data
          __iReturn = oTransmit_in.serialize( sdInput, __oData );
          if( __iReturn <= 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to send filter data (#2) @ serialize=" << __iReturn << endl;
            break;
          }
        }

        // Start feed
        __oData.reset();
        __oData.setID( "#START" );
        __iReturn = oTransmit_in.serialize( sdInput, __oData );
        if( __iReturn <= 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to start data feed @ serialize=" << __iReturn << endl;
          break;
        }

        // Receive and dump data
        for(;;)
        {
          if( SGCTP_INTERRUPTED )
            break;

          // ... unserialize
          __iReturn = oTransmit_in.unserialize( sdInput, &__oData );
          if( SGCTP_INTERRUPTED )
            break;
          if( __iReturn == 0 )
            break;
          if( __iReturn < 0 )
          {
            SGCTP_LOG << SGCTP_ERROR << "Failed to unserialize data @ unserialize=" << __iReturn << endl;
            SGCTP_BREAK( __iReturn );
          }

          // ... serialize
          sigBlock();
          __iReturn = oTransmit_out.serialize( fdOutput, __oData );
          sigUnblock();
          if( __iReturn <= 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to serialize data @ serialize=" << __iReturn << endl;
            break;
          }

        }

      }
      while( false ); // Error-catching block

      if( sdInput >= 0 )
        close( sdInput );
      if( !SGCTP_INTERRUPTED )
        sleep( 5 );

    }

  }
  while( false ); // Error-catching block

  // Done
  if( ptAddrinfo_in )
    free( ptAddrinfo_in );
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
