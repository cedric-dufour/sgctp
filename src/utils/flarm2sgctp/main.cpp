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
#include <termios.h>
#include <unistd.h>

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
// GLOBAL VARIABLES
//----------------------------------------------------------------------

// NOTE: shamelessly copied from GPSD
#define H_UERE 15.0  /* meters, 95% confidence */
#define V_UERE 23.0  /* meters, 95% confidence */


//----------------------------------------------------------------------
// STATIC / CONSTANTS
//----------------------------------------------------------------------

void CSgctpUtil::interrupt( int _iSignal )
{
  SGCTP_INTERRUPTED = 1;
}


bool CSgctpUtil::nmeaValid( const string &_rsNMEA )
{
  // NMEA message MUST start with '$'
  if( _rsNMEA[0] != '$' )
    return false;

  // NMEA message MUST have 2-character talker ID and 3-character message type
  int __iLength = _rsNMEA.length();
  if( __iLength < 6 )
    return false;

  // NMEA message has not checksum; assume it is valid
  if( _rsNMEA[__iLength-3] != '*' )
    return true;

  // NMEA checksum
  int __iChecksum = 0;
  char __pcChecksum[2];
  for( int __i = __iLength-4; __i > 0; __i-- )
    __iChecksum ^= _rsNMEA[__i];
  sprintf( __pcChecksum, "%02X", __iChecksum );
  if( __pcChecksum[0] != _rsNMEA[__iLength-2]
      || __pcChecksum[1] != _rsNMEA[__iLength-1] )
    return false; // invalid NMEA checksum

  // Done
  return true;
}

double CSgctpUtil::nmeaTime( const string &_rsTime )
{
  size_t __iLength = _rsTime.length();
  size_t __iDecimal = _rsTime.find( '.' );
  if( __iDecimal == string::npos ) __iDecimal = __iLength;
  if( __iDecimal < 5 ) return CData::UNDEFINED_VALUE;
  double __fdTime =
    strtod( _rsTime.substr( 0, __iDecimal - 4 ).c_str(), NULL ) * 3600; // hours
  __fdTime +=
    strtod( _rsTime.substr( __iDecimal - 4, 2 ).c_str(), NULL ) * 60; // minutes
  __fdTime +=
    strtod( _rsTime.substr( __iDecimal - 2 ).c_str(), NULL ); // seconds
  return __fdTime;
}

double CSgctpUtil::nmeaLatitude( const string &_rsLatitude,
                                 const string &_rsQuadrant )
{
  size_t __iLength = _rsLatitude.length();
  size_t __iDecimal = _rsLatitude.find( '.' );
  if( __iDecimal == string::npos )
    __iDecimal = __iLength;
  if( __iDecimal < 3 )
    return CData::UNDEFINED_VALUE;
  double __fdLatitude =
    strtod( _rsLatitude.substr( 0, __iDecimal - 2 ).c_str(), NULL ); // degrees
  __fdLatitude +=
    strtod( _rsLatitude.substr( __iDecimal - 2 ).c_str(), NULL ) / 60.0; // minutes
  if( _rsQuadrant == "S" )
    __fdLatitude = -__fdLatitude;
  return __fdLatitude;
}

double CSgctpUtil::nmeaLongitude( const string &_rsLongitude,
                                  const string &_rsQuadrant )
{
  size_t __iLength = _rsLongitude.length();
  size_t __iDecimal = _rsLongitude.find( '.' );
  if( __iDecimal == string::npos )
    __iDecimal = __iLength;
  if( __iDecimal < 3 )
    return CData::UNDEFINED_VALUE;
  double __fdLongitude =
    strtod( _rsLongitude.substr( 0, __iDecimal - 2 ).c_str(), NULL ); // degrees
  __fdLongitude +=
    strtod( _rsLongitude.substr( __iDecimal - 2 ).c_str(), NULL ) / 60.0; // minutes
  if( _rsQuadrant == "W" )
    __fdLongitude = -__fdLongitude;
  return __fdLongitude;
}


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtil::CSgctpUtil( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "flarm2sgctp", _iArgC, _ppcArgV )
  , fdInput( STDIN_FILENO )
  , fdOutput( STDOUT_FILENO )
  , sInputPath( "-" )
  , sOutputPath( "-" )
  , iInputBaudRate( 4800 )
  , sID( "FLARM" )
  , bGpsOnly( false )
  , bFlarmOnly( false )
  , bBarometricElevation( false )
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
  cout << "  flarm2sgctp [options] [<path(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data received from the given FLARM file or serial port" << endl;
  cout << "  (default:'-', standard input)." << endl;
  cout << endl << "WARNING:" << endl;
  cout << "  Direct connection to FLARM serial port is untested! Please use with caution." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -r, --baud-rate <rate>" << endl;
  cout << "    Serial port baud rate (defaut:4800)" << endl;
  cout << "  -i, --gps-id <ID>" << endl;
  cout << "    FLARM (GPS) device ID, if none is sent by the device (default:FLARM)" << endl;
  cout << "  -g, --gps-only" << endl;
  cout << "    Output only (internal) GPS data (default:no)" << endl;
  cout << "  -f, --flarm-only" << endl;
  cout << "    Output only (remote aircraft) FLARM data (default:no)" << endl;
  cout << "  -b, --barometric-elevation" << endl;
  cout << "    Use barometric instead of GPS elevation (default:no)" << endl;
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
      else if( __sArg=="-r" || __sArg=="--baud-rate" )
      {
        if( ++__i<iArgC )
          iInputBaudRate = atoi( ppcArgV[__i] );
      }
      else if( __sArg=="-g" || __sArg=="--gps-only" )
      {
        bGpsOnly = true;
      }
      else if( __sArg=="-i" || __sArg=="--gps-id" )
      {
        if( ++__i<iArgC )
          sID = ppcArgV[__i];
      }
      else if( __sArg=="-f" || __sArg=="--flarm-only" )
      {
        bFlarmOnly = true;
      }
      else if( __sArg=="-b" || __sArg=="--barometric-elevation" )
      {
        bBarometricElevation = true;
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
    // Open input (POSIX file descriptor)
    bool __bInputIsSerial = false;
    if( sInputPath != "-" )
    {
      // ... check for (serial port) "/dev/"-ice
      if( sInputPath.substr( 0, 5 ) == "/dev/" ) __bInputIsSerial = true;

      // ... open file/port
      fdInput = open( sInputPath.c_str(),
                      __bInputIsSerial
                      ? O_RDWR | O_NOCTTY | O_SYNC
                      : O_RDONLY );
      if( fdInput < 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid/unreadable file (" << sInputPath << ") @ open=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }

      // ... configure serial port
      if( __bInputIsSerial )
      {
        const int __piBaudRate[5] = { B4800, B9600, B19200, B38400, B57600 };
        char __pcNmeaSentence[32];

        // ... check speed setting
        int __iBaudCode;
        switch( iInputBaudRate )
        {
        case 4800: __iBaudCode=0; break;
        case 9600: __iBaudCode=1; break;
        case 19200: __iBaudCode=2; break;
        case 38400: __iBaudCode=3; break;
        case 57600: __iBaudCode=4; break;
        default:
          SGCTP_LOG << SGCTP_ERROR << "Invalid baud rate (" << to_string( iInputBaudRate ) << ")" << endl;
          SGCTP_BREAK( -EINVAL );
        }

        // ... retrieve port attributes
        struct termios __tTermIO;
        memset( &__tTermIO, 0, sizeof( __tTermIO ) );
        if( tcgetattr( fdInput, &__tTermIO ) )
        {
          SGCTP_LOG << SGCTP_ERROR << "Failed to retrieve serial port attributes @ tcgetattr=" << -errno << endl;
          SGCTP_BREAK( -errno );
        }

        // ... set port speed
        cfsetispeed( &__tTermIO, __piBaudRate[__iBaudCode] );

        // ... set control flags
        __tTermIO.c_cflag = (__tTermIO.c_cflag & ~CSIZE) | CS8;
        __tTermIO.c_cflag &= ~(PARENB | PARODD);
        __tTermIO.c_cflag &= ~CSTOPB;
        __tTermIO.c_cflag &= ~CRTSCTS;
        __tTermIO.c_cflag |= (CLOCAL | CREAD);

        // ... set local flags
        __tTermIO.c_lflag = 0;

        // ... set input flags
        __tTermIO.c_iflag &= ~IGNBRK;
        __tTermIO.c_iflag &= ~(IXON | IXOFF | IXANY);

        // ... set output flags
        __tTermIO.c_oflag = 0;

        // ... set blocking flags
        __tTermIO.c_cc[VMIN]  = 1;
        __tTermIO.c_cc[VTIME] = 0;

        // ... set port attributes
        if( tcsetattr( fdInput, TCSANOW, &__tTermIO ) )
        {
          SGCTP_LOG << SGCTP_ERROR << "Failed to set serial port attributes @ tcsetattr=" << -errno << endl;
          SGCTP_BREAK( -errno );
        }

        // ... quick-'n-dirty "automatic" baud-rate setting
        {
          for( int __i=0; __i<5; __i++ )
          {
            cfsetospeed( &__tTermIO, __piBaudRate[__i] );
            tcsetattr( fdInput, TCSANOW, &__tTermIO );
            sprintf( __pcNmeaSentence, "$PFLAC,S,BAUD,%d\n", __iBaudCode );
            write( fdInput, __pcNmeaSentence, strlen( __pcNmeaSentence ) );
            usleep( 100000 ); // > ( strlen + 25 ) * 8 * 1000000 / slowest-baud-rate
          }
        }
        cfsetospeed( &__tTermIO, __piBaudRate[__iBaudCode] );
        tcsetattr( fdInput, TCSANOW, &__tTermIO );

        // ... enable NMEA output
        sprintf( __pcNmeaSentence, "$PFLAC,S,NMEAOUT,1\n" );
        write( fdInput, __pcNmeaSentence, strlen( __pcNmeaSentence ) );
        usleep( 500000000 / __piBaudRate[__iBaudCode] ); // > ( strlen + 25 ) * 8 * 1000000 / baud-rate

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
    CData* __poDataReference = NULL;
    double __fdEpochReference = 0.0;
    unordered_map<string, CData*> __poData_umap;
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
      __iReturn = read( fdInput,
                        __pucBuffer+__iBufferDataEnd,
                        SGCTP_RECV_BUFFER_SIZE-__iBufferDataEnd );
      if( __iReturn <= 0 )
        break;
      __iBufferRecvSize = __iReturn;
      __iBufferDataEnd += __iBufferRecvSize;

      // Loop through buffer data
      while( __iBufferDataEnd-__iBufferDataStart > 0 )
      {
        // Read FLARM message (line)
        char __pcFLARM[SGCTP_RECV_BUFFER_SIZE+1];
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
          memcpy( __pcFLARM, __pucBuffer+__iBufferDataStart, __iLineLength );
          __pcFLARM[__iLineLength] = '\0';
          if( __iLineLength >= 1 && __pcFLARM[__iLineLength-1] == '\r' )
            __pcFLARM[__iLineLength-1] = '\0';
          __iBufferDataStart += __iLineLength+1;
          __iBufferDataEOL = __iBufferDataStart;
        }

        // Split FLARM and check data
#ifdef __SGCTP_DEBUG__
        SGCTP_LOG << SGCTP_DEBUG << "FLARM data: " << __pcFLARM << endl;
#endif // __SGCTP_DEBUG__
        // NOTE: FLARM (data port) protocol: http://flarm.ch/support/manual/FLARM_DataportManual_v6.00E.pdf
        vector<string> __vsFLARMFields;
        boost::split( __vsFLARMFields, __pcFLARM, boost::is_any_of(",") );
        if( !nmeaValid( __vsFLARMFields[0] ) )
          continue; // invalid NMEA-0183 message
        double __fdEpochNow = CData::epoch();

        // Originating source (ID)
        string __sSource = "";
        bool __bSourceReference = false;
        if( __vsFLARMFields[0] == "$PFLAU" )
        {
          if( __vsFLARMFields.size() >= 11 && !__vsFLARMFields[10].empty() )
            sID = __vsFLARMFields[10];
          __sSource = sID;
          __bSourceReference = true;
        }
        else if( __vsFLARMFields[0] == "$PFLAA" )
        {
          if( !__vsFLARMFields[6].empty() )
            __sSource = __vsFLARMFields[6];
        }
        else if( __poDataReference )
        {
          __sSource = __poDataReference->getID();
        }
        if( __sSource.empty() ) continue; // we need a source ID!

        // Data map cleanup
        if( __fdEpochNow - __fdEpochCleanup > (double)300.0 )
        {
          __fdEpochCleanup = __fdEpochNow;

          // ... get keys
          vector<string> __vsKeys;
          __vsKeys.reserve( __poData_umap.size() );
          for( unordered_map<string, CData*>::const_iterator __it =
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
              if( __poData_umap[ *__it ] == __poDataReference )
                __poDataReference = NULL;
              delete __poData_umap[ *__it ];
              __poData_umap.erase( *__it );
            }
          }

        }

        // Data map lookup
        CData* __poData;
        unordered_map<string, CData*>::const_iterator __it =
          __poData_umap.find( __sSource );
        if( __it == __poData_umap.end() )
        {
          __poData = new CData();
          __poData->setID( __sSource.c_str() );
          if( bExtendedContent )
            __poData->setSourceType( __bSourceReference
                                     ? CData::SOURCE_GPS
                                     : CData::SOURCE_FLARM );
          __poData_umap[__sSource] = __poData;
        }
        else
          __poData = __it->second;

        // ... reference source
        if( __bSourceReference )
          __poDataReference = __poData;

        // Parse FLARM data
        if( !__poDataReference )
          continue; // we can't proceed without having reference data
        bool __bDataAvailable = false;
        if( __vsFLARMFields[0] == "$GPRMC" )
        {
          if( __vsFLARMFields[2] == "V" )
            continue; // let's ignore "warning" device data
          __bDataAvailable = false; // wait for elevation (GPGGA or PGRMZ) before sending data
          double __fdValue;

          // ... time
          __fdValue = nmeaTime( __vsFLARMFields[1] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setTime( __fdValue );
          __fdEpochReference = __fdEpochNow;

          // ... latitude
          __fdValue = nmeaLatitude( __vsFLARMFields[3], __vsFLARMFields[4] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setLatitude( __fdValue );

          // ... longitude
          __fdValue = nmeaLongitude( __vsFLARMFields[5], __vsFLARMFields[6] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setLongitude( __fdValue );

          // ... bearing
          if( !__vsFLARMFields[8].empty() )
            __poDataReference->setBearing( strtod( __vsFLARMFields[8].c_str(), NULL ) );

          // ... ground speed
          if( !__vsFLARMFields[7].empty() )
            __poDataReference->setGndSpeed( strtod( __vsFLARMFields[7].c_str(), NULL )
                                            * 1852.0 / 3600.0 ); // knots

        }
        else if( __vsFLARMFields[0] == "$GPGGA" )
        {
          if( __vsFLARMFields[6] == "0" )
            continue; // let's ignore invalid GPS fix data
          __bDataAvailable = !bFlarmOnly;
          double __fdValue;

          // ... time
          __fdValue = nmeaTime( __vsFLARMFields[1] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setTime( __fdValue );
          __fdEpochReference = __fdEpochNow;

          // ... latitude
          __fdValue = nmeaLatitude( __vsFLARMFields[2], __vsFLARMFields[3] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setLatitude( __fdValue );

          // ... longitude
          __fdValue = nmeaLongitude( __vsFLARMFields[4], __vsFLARMFields[5] );
          if( !CData::isDefined( __fdValue ) )
            continue; // corrupted NMEA data
          __poDataReference->setLongitude( __fdValue );

          // ... elevation (WGS-84)
          if( !bBarometricElevation )
          {
            if( __vsFLARMFields[10] == "M" )
            {
              if( __vsFLARMFields[12] == "M" )
                __poDataReference->setElevation( strtod( __vsFLARMFields[9].c_str(), NULL )
                                                 + strtod( __vsFLARMFields[11].c_str(), NULL ) );
              else
                __poDataReference->setElevation( strtod( __vsFLARMFields[9].c_str(), NULL ) );
            }
          }

        }
        else if( __vsFLARMFields[0] == "$PGRMZ" )
        {
          if( !bBarometricElevation )
            continue;
          __bDataAvailable = !bFlarmOnly;

          // ... elevation
          __poDataReference->setElevation( strtod( __vsFLARMFields[1].c_str(), NULL ) * 0.3048 ); // input = feet

        }
        else if( __vsFLARMFields[0] == "$GPGSA" )
        {
          if( !bExtendedContent )
            continue;

          // ... 2D error
          if( ( __vsFLARMFields[2] == "2" || __vsFLARMFields[2] == "3" )
              && !__vsFLARMFields[16].empty() )
          {
            double __fdHDOP = strtod( __vsFLARMFields[16].c_str(), NULL );

            // ... latitude
            __poDataReference->setLatitudeError( __fdHDOP * H_UERE );

            // ... longitude
            __poDataReference->setLongitudeError( __fdHDOP * H_UERE );

          }

          // ... 3D error
          if( __vsFLARMFields[2] == "3" && !__vsFLARMFields[17].empty() )
          {
            // ... elevation
            __poDataReference->setElevationError( strtod( __vsFLARMFields[17].c_str(), NULL )
                                                  * V_UERE );
          }

        }
        else if( __vsFLARMFields[0] == "$PFLAA" )
        {
          if( __fdEpochReference <= 0.0 )
            continue; // we need a valid time reference
          double __fdTimeReference = __poDataReference->getTime();
          if( !CData::isDefined( __fdTimeReference ) )
            continue; // we need a valid reference fix
          double __fdLatitudeReference = __poDataReference->getLatitude();
          if( !CData::isDefined( __fdLatitudeReference ) )
            continue; // we need a valid reference fix
          double __fdLongitudeReference = __poDataReference->getLongitude();
          if( !CData::isDefined( __fdLongitudeReference ) )
            continue; // we need a valid reference fix
          double __fdElevationReference = __poDataReference->getElevation();
          if( !CData::isDefined( __fdElevationReference ) )
            continue; // we need a valid reference fix
          __bDataAvailable = !bGpsOnly;

          // ... time
          __poData->setTime( __fdTimeReference
                             + __fdEpochNow - __fdEpochReference );

          // ... latitude
          __poData->setLatitude( __fdLatitudeReference
                                 + asin( strtod( __vsFLARMFields[2].c_str(), NULL ) / SGCTP_WGS84B ) * SGCTP_RAD2DEG );

          // ... longitude
          __poData->setLongitude( __fdLongitudeReference
                                  + asin( strtod( __vsFLARMFields[3].c_str(), NULL ) / SGCTP_WGS84A ) * SGCTP_RAD2DEG );

          // ... elevation
          __poData->setElevation( __fdElevationReference
                                  + strtod( __vsFLARMFields[4].c_str(), NULL ) );

          // ... bearing
          if( !__vsFLARMFields[7].empty() )
            __poData->setBearing( strtod( __vsFLARMFields[7].c_str(), NULL ) );

          // ... ground speed
          if( !__vsFLARMFields[9].empty() )
            __poData->setGndSpeed( strtod( __vsFLARMFields[9].c_str(), NULL ) );

          // ... vertical speed
          if( !__vsFLARMFields[10].empty() )
            __poData->setVrtSpeed( strtod( __vsFLARMFields[10].c_str(), NULL ) );

          // ... bearing variation over time
          if( !__vsFLARMFields[8].empty() )
            __poData->setBearingDt( strtod( __vsFLARMFields[8].c_str(), NULL ) );

          // ... errors
          if( bExtendedContent )
          {
            // NOTE: Let's assume FLARM relative elevation is computed based on barometric elevation.
            //       Thus, the error on the relative elevation can reasonably be assumed <1.0m.
            //       On the other hand, ground position is always based on GPS positioning (and DOP).
            //       Also, one must take into account (add) the errors of the reference fix.
            double __fdValue;

            // ... latitude
            __fdValue = __poDataReference->getLatitudeError();
            if( CData::isDefined( __fdValue ) )
              __poData->setLatitudeError( 2.0*__fdValue );

            // ... longitude
            __fdValue = __poDataReference->getLongitudeError();
            if( CData::isDefined( __fdValue ) )
              __poData->setLongitudeError( 2.0*__fdValue );

            // ... elevation
            __fdValue = __poDataReference->getElevationError();
            if( CData::isDefined( __fdValue ) )
              __poData->setElevationError( bBarometricElevation ? 1.0 : __fdValue );

          }

        }

        // Dump SGCTP data

        //  ... data available
        if( !__bDataAvailable )
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
  if( fdInput >= 0 && fdInput != STDIN_FILENO )
    close( fdInput );
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
