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
#include <unistd.h>

// C++
#include <unordered_map>
#include <vector>
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
  : CSgctpUtilSkeleton( "sgctpfilter", _iArgC, _ppcArgV )
  , fdInput( STDIN_FILENO )
  , fdOutput( STDOUT_FILENO )
  , sInputPath( "-" )
  , sOutputPath( "-" )
  , fdReplayRate( CData::UNDEFINED_VALUE )
  , sID( "" )
  , fdTimeThrottle( CData::UNDEFINED_VALUE )
  , fdDistanceThreshold( CData::UNDEFINED_VALUE )
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
  , iDataTTL( 3600 )
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
  cout << "  sgctpfilter [options] [<path(in)>] +<field> [+<field> ...]" << endl;
  displaySynopsisHeader();
  cout << "  Filter the SGCTP data from the given file (default:'-', standard input)." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -r, --replay-rate <rate>"   << endl;
  cout << "    Data replay rate (defaut:none, must be >= 1.0)"   << endl;
  cout << "  -i, --id <ID>"   << endl;
  cout << "    Source ID (defaut:none)"   << endl;
  cout << "  -t, --time-throttle <delay>"   << endl;
  cout << "    Throttle output (defaut:none, seconds)"   << endl;
  cout << "  -d, --distance-threshold <distance>"   << endl;
  cout << "    Distance variation threshold (defaut:none, meters)"   << endl;
  cout << "  -lat0, --latitude-reference <latitude>"   << endl;
  cout << "    Reference point latitude (default:none, degrees)"   << endl;
  cout << "  -lon0, --longitude-reference <longitude>"   << endl;
  cout << "    Reference point longitude (default:none, degrees)"   << endl;
  cout << "  -t1, --time-limit-1 <time>"   << endl;
  cout << "    1st limit time (default:none, seconds)"   << endl;
  cout << "  -lat1, --latitude-limit-1 <latitude>"   << endl;
  cout << "    1st limit latitude (default:none, degrees)"   << endl;
  cout << "  -lon1, --longitude-limit-1 <longitude>"   << endl;
  cout << "    1st limit longitude (default:none, degrees)"   << endl;
  cout << "  -ele1, --elevation-limit-1 <elevation>"   << endl;
  cout << "    1st limit elevation (default:none, meters)"   << endl;
  cout << "  -t2, --time-limit-2 <time>"   << endl;
  cout << "    2nd limit time (default:none, seconds)"   << endl;
  cout << "  -lat2, --latitude-limit-2 <latitude>"   << endl;
  cout << "    2nd limit latitude (default:none, degrees)"   << endl;
  cout << "  -lon2, --longitude-limit-2 <longitude>"   << endl;
  cout << "    2nd limit longitude (default:none, degrees)"   << endl;
  cout << "  -ele2, --elevation-limit-2 <elevation>"   << endl;
  cout << "    2nd limit elevation (default:none, meters)"   << endl;
  displayOptionPrincipal( true, true );
  displayOptionPayload( true, true );
  displayOptionPassword( true, true );
  displayOptionPasswordSalt( true, true );
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
      SGCTP_PARSE_ARGS( parseArgsPrincipal( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPayload( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPassword( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, true, true ) );
      SGCTP_PARSE_ARGS( parseArgsDaemon( &__i ) );
      if( __sArg=="-o" || __sArg=="--output" )
      {
        if( ++__i<iArgC )
          sOutputPath = ppcArgV[__i];
      }
      else if( __sArg=="-r" || __sArg=="--replay-rate" )
      {
        if( ++__i<iArgC ) {
          fdReplayRate = strtod( ppcArgV[__i], NULL );
          if( fdReplayRate < 1.0 ) fdReplayRate = CData::UNDEFINED_VALUE;
        }
      }
      else if( __sArg=="-i" || __sArg=="--id" )
      {
        if( ++__i<iArgC )
          sID = ppcArgV[__i];
      }
      else if( __sArg=="-t" || __sArg=="--time-throttle" )
      {
        if( ++__i<iArgC )
          fdTimeThrottle = strtod( ppcArgV[__i], NULL );
      }
      else if( __sArg=="-d" || __sArg=="--distance-threshold" )
      {
        if( ++__i<iArgC )
          fdDistanceThreshold = strtod( ppcArgV[__i], NULL );
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
        cerr << "DEBUG:" << to_string( fdTime2 ) << endl;
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
      else if( __sArg=="--ttl" )
      {
        if( ++__i<iArgC ) iDataTTL = atoi( ppcArgV[__i] );
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
  __iReturn = principalLookup( true, true );
  if( __iReturn )
    return __iReturn;

  // Initialize transmission object(s)
  __iReturn = transmitInit( true, true );
  if( __iReturn )
    return __iReturn;

  // Limits

  // ... time
  double __fdTimeMin = CData::UNDEFINED_VALUE;
  double __fdTimeMax = CData::UNDEFINED_VALUE;
  bool __bTimeLimit = false;
  if( CData::isDefined( fdTime1 ) )
    __fdTimeMin = fdTime1;
  if( CData::isDefined( fdTime2 ) )
    __fdTimeMax = fdTime2;
  if( CData::isDefined( __fdTimeMin )
      && CData::isDefined( __fdTimeMax ) )
  {
    __bTimeLimit = true;
    if( __fdTimeMin > __fdTimeMax )
    {
      double __fdValue = __fdTimeMin;
      __fdTimeMin = __fdTimeMax;
      __fdTimeMax = __fdValue;
    }
    if( __fdTimeMax - __fdTimeMin < 1.0 )
    {
      SGCTP_LOG << SGCTP_ERROR << "Invalid filter (null time range)" << endl;
      return -EINVAL;
    }
  }
  else
  {
    __fdTimeMin = CData::UNDEFINED_VALUE;
    __fdTimeMax = CData::UNDEFINED_VALUE;
  }

  // ... distance/azimuth
  double __fdDistanceMin = CData::UNDEFINED_VALUE;
  double __fdDistanceMax = CData::UNDEFINED_VALUE;
  double __fdAzimuthMin = CData::UNDEFINED_VALUE;
  double __fdAzimuthMax = CData::UNDEFINED_VALUE;
  if( CData::isDefined( fdLatitude0 )
      && CData::isDefined( fdLongitude0 ) )
  {
    if( CData::isDefined( fdLatitude1 )
        && CData::isDefined( fdLongitude1 ) )
    {
      __fdDistanceMin = distanceRL( fdLatitude0, fdLongitude0,
                                    fdLatitude1, fdLongitude1 );
      __fdAzimuthMin = azimuthRL( fdLatitude0, fdLongitude0,
                                  fdLatitude1, fdLongitude1 );
    }
    if( CData::isDefined( fdLatitude2 )
        && CData::isDefined( fdLongitude2 ) )
    {
      __fdDistanceMax = distanceRL( fdLatitude0, fdLongitude0,
                                    fdLatitude2, fdLongitude2 );
      __fdAzimuthMax = azimuthRL( fdLatitude0, fdLongitude0,
                                  fdLatitude2, fdLongitude2 );
    }
  }

  // ... azimuth
  bool __bAzimuthLimit = false;
  if( CData::isDefined( __fdAzimuthMin )
      && CData::isDefined( __fdAzimuthMax ) )
  {
    __bAzimuthLimit = true;
    if( __fdAzimuthMin > __fdAzimuthMax )
    {
      double __fdValue = __fdAzimuthMin;
      __fdAzimuthMin = __fdAzimuthMax;
      __fdAzimuthMax = __fdValue;
    }
    if( __fdAzimuthMax - __fdAzimuthMin < 1.0 ) // too-small a sector = 360-degree sector
    {
      __fdAzimuthMin = CData::UNDEFINED_VALUE;
      __fdAzimuthMax = CData::UNDEFINED_VALUE;
      __bAzimuthLimit = false;
    }
  }
  else
  {
    __fdAzimuthMin = CData::UNDEFINED_VALUE;
    __fdAzimuthMax = CData::UNDEFINED_VALUE;
  }

  // ... distance
  bool __bDistanceLimit = false;
  if( CData::isDefined( __fdDistanceMin )
      || CData::isDefined( __fdDistanceMax ) )
  {
    __bDistanceLimit = true;
    if( !CData::isDefined( __fdDistanceMax ) )
    {
      __fdDistanceMax = __fdDistanceMin;
      __fdDistanceMin = CData::UNDEFINED_VALUE;
    }
    if( CData::isDefined( __fdDistanceMin ) )
    {
      if( __fdDistanceMin > __fdDistanceMax )
      {
        double __fdValue = __fdDistanceMin;
        __fdDistanceMin = __fdDistanceMax;
        __fdDistanceMax = __fdValue;
      }
      if( __fdDistanceMax - __fdDistanceMin < 1.0 )
      {
        if( __bAzimuthLimit )
        {
          __fdDistanceMin = CData::UNDEFINED_VALUE;
          __fdDistanceMax = CData::UNDEFINED_VALUE;
          __bDistanceLimit = false;
        }
        else
        {
          SGCTP_LOG << SGCTP_ERROR << "Invalid filter (null distance range)" << endl;
          return -EINVAL;
        }
      }
    }
  }

  // ... elevation
  double __fdElevationMin = CData::UNDEFINED_VALUE;
  double __fdElevationMax = CData::UNDEFINED_VALUE;
  bool __bElevationLimit = false;
  if( CData::isDefined( fdElevation1 ) )
    __fdElevationMin = fdElevation1;
  if( CData::isDefined( fdElevation2 ) )
    __fdElevationMax = fdElevation2;
  if( CData::isDefined( __fdElevationMin )
      || CData::isDefined( __fdElevationMax ) )
  {
    __bElevationLimit = true;
    if( !CData::isDefined( __fdElevationMax ) )
    {
      __fdElevationMax = __fdElevationMin;
      __fdElevationMin = CData::UNDEFINED_VALUE;
    }
    if( CData::isDefined( __fdElevationMin ) )
    {
      if( __fdElevationMin > __fdElevationMax )
      {
        double __fdValue = __fdElevationMin;
        __fdElevationMin = __fdElevationMax;
        __fdElevationMax = __fdValue;
      }
      if( __fdElevationMax - __fdElevationMin < 1.0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid filter (null elevation range)" << endl;
        return -EINVAL;
      }
    }
  }

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
  unordered_map<string,CDataPrevious*> __poDataPrevious_umap;
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

    // Receive, filter and dump data
    CData __oData;
    double __fdEpochCleanup = CData::epoch();
    double __fdEpochNowPrevious = 0.0;
    double __fdEpochPrevious = 0.0;
    for(;;)
    {
      if( SGCTP_INTERRUPTED )
        break;

      // Unserialize input
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
      double __fdEpochNow = CData::epoch();

      // Originating source (ID)
      string __sSource = __oData.getID();

      // Data map cleanup
      if( __fdEpochNow - __fdEpochCleanup > (double)300.0 )
      {
        __fdEpochCleanup = __fdEpochNow;

        // ... get keys
        vector<string> __vsKeys;
        __vsKeys.reserve( __poDataPrevious_umap.size() );
        for( unordered_map<string,CDataPrevious*>::const_iterator __it =
               __poDataPrevious_umap.begin();
             __it != __poDataPrevious_umap.end();
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
          if( __fdEpochNow - __poDataPrevious_umap[ *__it ]->fdEpoch
              > (double)iDataTTL )
          {
            delete __poDataPrevious_umap[ *__it ];
            __poDataPrevious_umap.erase( *__it );
          }
        }

      }

      // Data map lookup
      CDataPrevious* __poDataPrevious;
      unordered_map<string,CDataPrevious*>::const_iterator __it =
        __poDataPrevious_umap.find( __sSource );
      if( __it == __poDataPrevious_umap.end() )
      {
        __poDataPrevious = new CDataPrevious();
        __poDataPrevious_umap[__sSource] = __poDataPrevious;
      }
      else
        __poDataPrevious = __it->second;
      bool __b2DPrev =
        CData::isDefined( __poDataPrevious->fdLatitude )
        && CData::isDefined( __poDataPrevious->fdLongitude );
      bool __b3DPrev =
        __b2DPrev
        && CData::isDefined( __poDataPrevious->fdElevation );

      // Filter

      // ... ID
      if( !sID.empty() && sID != __oData.getID() )
        continue;

      // ... time
      double __fdTime = __oData.getTime();
      if( __bTimeLimit )
      {
        if( CData::isDefined( __fdTimeMin ) && __fdTime < __fdTimeMin )
          continue;
        if( CData::isDefined( __fdTimeMax ) && __fdTime > __fdTimeMax )
          continue;
      }

      // ... time (throttle)
      double __fdEpoch = CData::toEpoch( __fdTime );
      if( CData::isDefined( fdTimeThrottle )
          && ( __fdEpoch - __poDataPrevious->fdEpoch ) < fdTimeThrottle )
        continue;

      // ... distance (threshold)
      double __fdLatitude = __oData.getLatitude();
      double __fdLongitude = __oData.getLongitude();
      double __fdElevation = __oData.getElevation();
      bool __b2D =
        CData::isDefined( __fdLatitude )
        && CData::isDefined( __fdLongitude );
      bool __b3D =
        __b2D
        && CData::isDefined( __fdElevation );
      if( CData::isDefined( fdDistanceThreshold ) && __b2D && __b2DPrev )
      {
        double __fdDistance = distanceRL( __poDataPrevious->fdLatitude,
                                          __poDataPrevious->fdLongitude,
                                          __fdLatitude,
                                          __fdLongitude );
        if( __b3D && __b3DPrev )
        {
          double __fdElevationDelta =
            __poDataPrevious->fdElevation - __fdElevation;
          __fdDistance =
            sqrt( __fdDistance*__fdDistance
                  + __fdElevationDelta*__fdElevationDelta );
        }
        if( __fdDistance < fdDistanceThreshold )
          continue;
      }

      // ... distance
      if( __bDistanceLimit && __b2D )
      {
        double __fdDistance = distanceRL( fdLatitude0, fdLongitude0,
                                          __fdLatitude, __fdLongitude );
        if( CData::isDefined( __fdDistanceMin )
            && __fdDistance < __fdDistanceMin )
          continue;
        if( CData::isDefined( __fdDistanceMax )
            && __fdDistance > __fdDistanceMax )
          continue;
      }

      // ... azimuth
      if( __bAzimuthLimit && __b2D )
      {
        double __fdAzimuth = azimuthRL( fdLatitude0, fdLongitude0,
                                        __fdLatitude, __fdLongitude );
        if( CData::isDefined( __fdAzimuthMin )
            && __fdAzimuth < __fdAzimuthMin )
          continue;
        if( CData::isDefined( __fdAzimuthMax )
            && __fdAzimuth > __fdAzimuthMax )
          continue;
      }

      // ... elevation
      if( __bElevationLimit && __b3D )
      {
        if( CData::isDefined( __fdElevationMin )
            && __fdElevation < __fdElevationMin )
          continue;
        if( CData::isDefined( __fdElevationMax )
            && __fdElevation > __fdElevationMax )
          continue;
      }

      // Replay rate
      if( CData::isDefined( fdReplayRate ) )
      {
        if( __fdEpochPrevious > 0 )
        {
          double __fdDelay =
            ( __fdEpoch - __fdEpochPrevious ) / fdReplayRate
            - ( __fdEpochNow - __fdEpochNowPrevious );
          if( __fdDelay > 0 ) usleep( __fdDelay * 1000000.0 );
        }
        __fdEpochPrevious = __fdEpoch;
        __fdEpochNowPrevious = CData::epoch();
      }

      // Store "previous" data
      __poDataPrevious->fdEpoch = __fdEpoch;
      if( CData::isDefined( __fdLatitude ) )
        __poDataPrevious->fdLatitude = __fdLatitude;
      if( CData::isDefined( __fdLongitude ) )
        __poDataPrevious->fdLongitude = __fdLongitude;
      if( CData::isDefined( __fdElevation ) )
        __poDataPrevious->fdElevation = __fdElevation;

      // Serialize output
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

    }

  }
  while( false ); // Error-catching block

  // Done
  if( fdInput >= 0 && fdInput != STDIN_FILENO )
    close( fdInput );
  if( fdOutput >= 0 && fdOutput != STDOUT_FILENO )
    close( fdOutput );
  for( unordered_map<string,CDataPrevious*>::const_iterator __it =
         __poDataPrevious_umap.begin();
       __it != __poDataPrevious_umap.end();
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
