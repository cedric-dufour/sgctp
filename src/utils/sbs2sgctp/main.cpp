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
#include <unordered_map>
#include <vector>
using namespace std;

// Boost
#include <boost/algorithm/string.hpp>

// SGCTP
#define SGCTP_RECV_BUFFER_SIZE 131072
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
  : CSgctpUtilSkeleton( "sbs2sgctp", _iArgC, _ppcArgV )
  , sdInput( -1 )
  , ptAddrinfo_in( NULL )
  , fdOutput( STDOUT_FILENO )
  , sInputHost( "localhost" )
  , sInputPort( "30003" )
  , sOutputPath( "-" )
  , bGroundTraffic( false )
  , bCallsignLookup( false )
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
  cout << "  sbs2sgctp [options] [<hostname(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given SBS-1 host (default:localhost)." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -p, --port <port>" << endl;
  cout << "    Host port (defaut:30003)" << endl;
  cout << "  -g, --ground-traffic" << endl;
  cout << "    Include ground traffic" << endl;
  cout << "  -c, --callsign-lookup" << endl;
  cout << "    Use callsign instead of hexadecimal identification" << endl;
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
      else if( __sArg=="-g" || __sArg=="--ground-traffic" )
      {
        bGroundTraffic = true;
      }
      else if( __sArg=="-c" || __sArg=="--callsign-lookup" )
      {
        bCallsignLookup = true;
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

    // Open input (TCP socket)
    struct addrinfo* __ptAddrinfoActual_out;
    {

      // ... lookup socket address info
      struct addrinfo __tAddrinfoHints;
      memset( &__tAddrinfoHints, 0, sizeof( __tAddrinfoHints ) );
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

      for( __ptAddrinfoActual_out = ptAddrinfo_in;
           __ptAddrinfoActual_out != NULL;
           __ptAddrinfoActual_out = __ptAddrinfoActual_out->ai_next )
      {

        // ... create socket
        sdInput = socket( __ptAddrinfoActual_out->ai_family,
                          __ptAddrinfoActual_out->ai_socktype,
                          __ptAddrinfoActual_out->ai_protocol );
        if( sdInput < 0 )
        {
          SGCTP_LOG << SGCTP_WARNING << "Failed to create socket (" << sInputHost << ":" << sInputPort << ") @ socket=" << -errno << endl;
          continue;
        }
        break;
      }
      if( sdInput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to create socket (" << sInputHost << ":" << sInputPort << ") @ socket=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }

      // ... connect socket
      __iReturn = connect( sdInput,
                           ptAddrinfo_in->ai_addr,
                           ptAddrinfo_in->ai_addrlen );
      if( __iReturn )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to connect socket (" << sInputHost << ":" << sInputPort << ") @ connect=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }

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
    double __fdEpochCleanup = CData::epoch();
    unsigned char __pucBuffer[SGCTP_RECV_BUFFER_SIZE];
    int __iBufferRecvSize = 0;
    int __iBufferDataStart = 0;
    int __iBufferDataEnd = 0;
    int __iBufferDataEOL = 0;
    // Loop through socket RECV
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

      // Receive data
      __iReturn = recv( sdInput,
                        __pucBuffer+__iBufferDataEnd,
                        SGCTP_RECV_BUFFER_SIZE-__iBufferDataEnd,
                        0 );
      if( __iReturn <= 0 )
        break;
      __iBufferRecvSize = __iReturn;
      __iBufferDataEnd += __iBufferRecvSize;

      // Loop through buffer data
      while( __iBufferDataEnd-__iBufferDataStart > 0 )
      {
        // Read SBS message (line)
        char __pcSBS[SGCTP_RECV_BUFFER_SIZE+1];
        {
          unsigned char* __pucEOL =
            (unsigned char*)memchr( __pucBuffer+__iBufferDataEOL,
                                    '\n',
                                    __iBufferDataEnd-__iBufferDataEOL );
          if( __pucEOL == NULL )
          {
            __iBufferDataEOL = __iBufferDataEnd;
            break;
          }
          int __iLineLength = (int)(__pucEOL-__pucBuffer) - __iBufferDataStart;
          memcpy( __pcSBS, __pucBuffer+__iBufferDataStart, __iLineLength );
          __pcSBS[__iLineLength] = '\0';
          if( __iLineLength >= 1 && __pcSBS[__iLineLength-1] == '\r' )
            __pcSBS[__iLineLength-1] = '\0';
          __iBufferDataStart += __iLineLength+1;
          __iBufferDataEOL = __iBufferDataStart;
        }

        // Split SBS and check data
#ifdef __SGCTP_DEBUG__
        SGCTP_LOG << SGCTP_DEBUG << "SBS-1 data: " << __pcSBS << endl;
#endif // __SGCTP_DEBUG__
        // NOTE: SBS-1 protocol: http://www.homepages.mcb.net/bones/SBS/Article/Barebones42_Socket_Data.htm
        int __iMSG;
        vector<string> __vsSBSFields;
        boost::split( __vsSBSFields, __pcSBS, boost::is_any_of(",") );
        if( __vsSBSFields[0] != "MSG" )
          continue; // We process only SBS-1's "MSG" message type
        if( __vsSBSFields.size() != 22 )
          continue; // "MSG" message type ought to be 22-fields long
        if( ( __iMSG = atoi( __vsSBSFields[1].c_str() ) ) > 4 )
          continue; // Transmission types greater than "4" are useless to us
        if( atoi( __vsSBSFields[21].c_str() ) < 0 && !bGroundTraffic )
          continue; // Ignore ground traffic
        if( __vsSBSFields[4].empty() )
          continue; // We need a valid "HexIdent"
        double __fdEpochNow = CData::epoch();

        // Originating source (ID)
        string __sSource = __vsSBSFields[4];

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
            __poData->setSourceType( CData::SOURCE_ADSB );
          __poData_umap[__sSource] = __poData;
        }
        else
          __poData = __it->second;

        // ... ID
        if( bCallsignLookup )
        {
          if( !__vsSBSFields[10].empty() )
          {
            boost::trim( __vsSBSFields[10] );
            __poData->setID( __vsSBSFields[10].c_str() );
          }
        }
        else if( strlen( __poData->getID() ) == 0 )
          __poData->setID( __sSource.c_str() );
        if( __iMSG == 1 ) continue;

        // Parse SBS data
        bool __bDataAvailable = false;

        // ... time
        if( !__vsSBSFields[6].empty() && !__vsSBSFields[7].empty() )
        {
          struct tm __tmDateTime;
          char __pcDateTime[24], *__pcSubSecond;
          memcpy( __pcDateTime, __vsSBSFields[6].c_str(), 10 );
          __pcDateTime[10] = ' ';
          memcpy( __pcDateTime+11, __vsSBSFields[7].c_str(), 12 );
          __pcDateTime[23] = '\0';
          __pcSubSecond = strptime( __pcDateTime,
                                    "%Y/%m/%d %H:%M:%S.",
                                    &__tmDateTime );
          if( __pcSubSecond )
            __poData->setTime( (double)timegm( &__tmDateTime )
                               + (double)atoi( __pcSubSecond )/1000.0 );
        }
        else
        {
          // Let's default to use current system time (some SBS-1 devices do not set the date/time fields)
          __poData->setTime( __fdEpochNow );
        }

        // ... position/elevation
        if( !__vsSBSFields[14].empty() && !__vsSBSFields[15].empty() )
        {
          __poData->setLongitude( strtod( __vsSBSFields[15].c_str(), NULL ) );
          __poData->setLatitude( strtod( __vsSBSFields[14].c_str(), NULL ) );
          if( !__vsSBSFields[11].empty() )
            __poData->setElevation( strtod( __vsSBSFields[11].c_str(), NULL )
                                    * 0.3048 ); // feet
          __bDataAvailable = true;
        }

        // ... ground course
        if( !__vsSBSFields[12].empty() && !__vsSBSFields[13].empty() )
        {
          __poData->setBearing( strtod( __vsSBSFields[13].c_str(), NULL ) );
          __poData->setGndSpeed( strtod( __vsSBSFields[12].c_str(), NULL )
                                 / 1.94384449244 ); // knots
          if( !__vsSBSFields[16].empty() )
            __poData->setVrtSpeed( strtod( __vsSBSFields[16].c_str(), NULL )
                                   / 196.8503937 ); // feet/minute
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

      } // Loop through socket data

      // Flush buffer ?
      if( SGCTP_RECV_BUFFER_SIZE-__iBufferDataStart < 1024 )
      {
        // Non-overlapping memcpy
        int __iBufferLength = __iBufferDataEnd - __iBufferDataStart;
        int __iBufferCopied = 0;
        while( __iBufferCopied < __iBufferLength )
        {
          int __iBufferCopyLength = __iBufferLength - __iBufferCopied;
          if( __iBufferCopyLength > __iBufferDataStart )
            __iBufferCopyLength = __iBufferDataStart;
          memcpy( __pucBuffer + __iBufferCopied,
                  __pucBuffer + __iBufferCopied + __iBufferDataStart,
                  __iBufferCopyLength );
          __iBufferCopied += __iBufferCopyLength;
        }
        __iBufferDataEnd -= __iBufferDataStart;
        __iBufferDataEOL -= __iBufferDataStart;
        __iBufferDataStart = 0;
      }

      // Full buffer but no EOL ?
      if( __iBufferDataEOL == SGCTP_RECV_BUFFER_SIZE )
      {
        SGCTP_LOG << SGCTP_WARNING << "Buffer overflow" << endl;
        __iBufferDataStart = 0;
        __iBufferDataEnd = 0;
        __iBufferDataEOL = 0;
      }

    } // Loop through socket RECV

  }
  while( false ); // Error-catching block

  // Done
  if( sdInput >= 0 )
    close( sdInput );
  if( fdOutput >= 0 && fdOutput != STDOUT_FILENO )
    close( fdOutput );
  if( ptAddrinfo_in )
    free( ptAddrinfo_in );
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
