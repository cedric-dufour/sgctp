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
  : CSgctpUtilSkeleton( "tcp2sgctp", _iArgC, _ppcArgV )
  , sdInput( -1 )
  , ptAddrinfo_in( NULL )
  , fdOutput( STDOUT_FILENO )
  , sInputHost( "localhost" )
  , sInputPort( "8947" )
  , sOutputPath( "-" )
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
  cout << "  tcp2sgctp [options] [<hostname(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given TCP host (default:'localhost')." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:8947)" << endl;
  displayOptionPrincipal( false, true );
  displayOptionPayload( false, true );
  displayOptionPassword( false, true );
  displayOptionPasswordSalt( false, true );
  displayOptionDaemon();
  cout << endl << "WARNING:" << endl;
  cout << "  This program accepts only a *single* TCP client at a time." << endl;
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

    // Open input (TCP socket)
    {
      // ... lookup socket address info
      struct addrinfo* __ptAddrinfoActual;
      struct addrinfo __tAddrinfoHints;
      memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
      __tAddrinfoHints.ai_family = AF_UNSPEC;
      __tAddrinfoHints.ai_socktype = SOCK_STREAM;
      __tAddrinfoHints.ai_flags = AI_PASSIVE;
      __iReturn = getaddrinfo( sInputHost.c_str(),
                               sInputPort.c_str(),
                               &__tAddrinfoHints, &ptAddrinfo_in );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve socket address info (" << sInputHost << ":" << sInputPort << ") @ getaddrinfo=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }
      for( __ptAddrinfoActual = ptAddrinfo_in;
           __ptAddrinfoActual != NULL;
           __ptAddrinfoActual = __ptAddrinfoActual->ai_next )
      {
        // ... create socket
        sdInput = socket( __ptAddrinfoActual->ai_family,
                          __ptAddrinfoActual->ai_socktype,
                          __ptAddrinfoActual->ai_protocol );
        if( sdInput < 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to create socket (" << sInputHost << ":" << sInputPort << ") @ socket=" << -errno << endl;
          continue;
        }
        // ... configure socket
        int __iSO_REUSEADDR=1;
        __iReturn = setsockopt( sdInput,
                                SOL_SOCKET,
                                SO_REUSEADDR,
                                &__iSO_REUSEADDR,
                                sizeof( int ) );
        if( __iReturn )
        {
          close( sdInput );
          sdInput = -1;
          SGCTP_LOG << SGCTP_WARNING << "Failed to configure socket (" << sInputHost << ":" << sInputPort << ") @ setsockopt=" << -errno << endl;
          continue;
        }
        // ... bind socket
        __iReturn = bind( sdInput,
                          __ptAddrinfoActual->ai_addr,
                          __ptAddrinfoActual->ai_addrlen );
        if( __iReturn )
        {
          close( sdInput );
          sdInput = -1;
          SGCTP_LOG << SGCTP_WARNING << "Failed to bind socket (" << sInputHost << ":" << sInputPort << ") @ bind=" << -errno << endl;
          continue;
        }
        break;
      }
      if( sdInput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to create socket (" << sInputHost << ":" << sInputPort << ")" << endl;
        SGCTP_BREAK( -errno );
      }
      // ... listen on socket
      __iReturn = listen( sdInput, 1 );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to listen on socket (" << sInputHost << ":" << sInputPort << ") @ listen=" << -errno << endl;
        SGCTP_BREAK( -errno );
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

    // Loop through TCP connections
    int __sdClient;
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

      // Wait for client
      __sdClient = accept( sdInput, NULL, NULL );
      if( __sdClient < 0 )
        break;

      // Error-catching block
      do
      {
        // Receive handshake
        __iReturn = oTransmit_in.recvHandshake( __sdClient );
        if( __iReturn < 0 )
        {
          SGCTP_LOG << SGCTP_ERROR << "Failed to receive TCP handshake @ recvHandshake=" << __iReturn << endl;
          break;
        }

        // Receive and dump data
        CData __oData;
        for(;;)
        {
          if( SGCTP_INTERRUPTED )
            break;

          // ... unserialize
          __iReturn = oTransmit_in.unserialize( __sdClient, &__oData );
          if( SGCTP_INTERRUPTED )
            break;
          if( __iReturn == 0 )
            break;
          if( __iReturn < 0 )
          {
            SGCTP_LOG << SGCTP_WARNING << "Failed to unserialize data @ unserialize=" << __iReturn << endl;
            break;
          }

          // ... serialize
          sigBlock();
          __iReturn = oTransmit_out.serialize( fdOutput, __oData );
          sigUnblock();
          if( __iReturn == 0 )
            SGCTP_BREAK( 0 );
          if( __iReturn < 0 )
          {
            SGCTP_LOG << SGCTP_ERROR << "Failed to serialize data @ serialize=" << __iReturn << endl;
            SGCTP_BREAK( -1 );
          }

        }

      }
      while( false ); // Error-catching block

      // Close client and free resources
      close( __sdClient );
      oTransmit_in.freePayload();
      oTransmit_in.free();
    }

  }
  while( false ); // Error-catching block

  // Done
  if( sdInput >= 0 )
    close( sdInput );
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
