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
  : CSgctpUtilSkeleton( "sgctp2udp", _iArgC, _ppcArgV )
  , fdInput( STDIN_FILENO )
  , sdOutput( -1 )
  , ptAddrinfo_out( NULL )
  , sInputPath( "-" )
  , sOutputHost( "localhost" )
  , sOutputPort( "8947" )
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
  cout << "  sgctp2udp [options] [<hostname(out)>] [<path(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Send the SGCTP data of the given file (default:'-', standard input)" << endl;
  cout << "  to the given host (default:localhost), via UDP." << endl;
  displayOptionsHeader();
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:8947)" << endl;
  displayOptionPrincipal( true, true );
  displayOptionPayload( true, true );
  displayOptionPassword( true, true );
  displayOptionPasswordSalt( true, true );
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
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsDaemon( &__i ) );
      if( __sArg=="-p" || __sArg=="--port" )
      {
        if( ++__i<iArgC )
          sOutputPort = ppcArgV[__i];
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
        sOutputHost = __sArg;
        break;
      case 1:
        sInputPath = __sArg;
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
    if( sInputPath == "-" )
    {
      SGCTP_LOG << SGCTP_ERROR << "Input must not be standard input" << endl;
      return -EINVAL;
    }
    if( sInputPath[0] != '/' )
    {
      SGCTP_LOG << SGCTP_ERROR << "Input path must be absolute" << endl;
      return -EINVAL;
    }
  }
  daemonStart();

  // Catch signals
  sigCatch( CSgctpUtil::interrupt );

  // Error-catching block
  do
  {

    // Open input (POSIX file descriptor)
    if( sInputPath != "-" )
    {
      fdInput = open( sInputPath.c_str(),
                      O_RDONLY );
      if( fdInput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid/unreadable file (" << sInputPath << ") @ open=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }
    }

    // Prepare output (UDP socket)
    {

      // ... lookup socket address info
      struct addrinfo* __ptAddrinfoActual;
      struct addrinfo __tAddrinfoHints;
      memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
      __tAddrinfoHints.ai_family = AF_UNSPEC;
      __tAddrinfoHints.ai_socktype = SOCK_DGRAM;
      __iReturn = getaddrinfo( sOutputHost.c_str(),
                               sOutputPort.c_str(),
                               &__tAddrinfoHints,
                               &ptAddrinfo_out );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve socket address info (" << sOutputHost << ":" << sOutputPort << ") @ getaddrinfo=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

      for( __ptAddrinfoActual = ptAddrinfo_out;
           __ptAddrinfoActual != NULL;
           __ptAddrinfoActual = __ptAddrinfoActual->ai_next )
      {

        // ... create socket
        sdOutput = socket( __ptAddrinfoActual->ai_family,
                           __ptAddrinfoActual->ai_socktype,
                           __ptAddrinfoActual->ai_protocol );
        if( sdOutput < 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to create socket (" << sOutputHost << ":" << sOutputPort << ") @ socket=" << -errno << endl;
          continue;
        }
        break;

      }
      if( sdOutput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to create socket (" << sOutputHost << ":" << sOutputPort << ")" << endl;
        SGCTP_BREAK( -errno );
      }
      oTransmit_out.setSocketAddress( __ptAddrinfoActual->ai_addr,
                                      &__ptAddrinfoActual->ai_addrlen );

    }

    // Receive and dump data
    CData __oData;
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

      // ... unserialize
      __iReturn = oTransmit_in.unserialize( fdInput, &__oData );
      if( SGCTP_INTERRUPTED )
        break;
      if( __iReturn == 0 )
        SGCTP_BREAK( 0 );
      if( __iReturn < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to unserialize data @ unserialize=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

      // ... serialize
      sigBlock();
      __iReturn = oTransmit_out.serialize( sdOutput, __oData );
      sigUnblock();
      if( __iReturn == 0 )
        SGCTP_BREAK( 0 );
      if( __iReturn < 0 )
      {
        SGCTP_LOG << SGCTP_WARNING << "Failed to serialize data @ serialize=" << __iReturn << endl;
        continue;
      }

    }

  }
  while( false ); // Error-catching block

  // Done
  if( fdInput >= 0 && fdInput != STDIN_FILENO )
    close( fdInput );
  if( sdOutput >= 0 )
    close( sdOutput );
  if( ptAddrinfo_out )
    free( ptAddrinfo_out );
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
