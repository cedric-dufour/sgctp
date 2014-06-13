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
#include <fstream>
using namespace std;

// Boost
#include <boost/format.hpp>

// SGCTP
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


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtil::CSgctpUtil( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "sgctp2gpx", _iArgC, _ppcArgV )
  , fdInput( STDIN_FILENO )
  , sInputPath( "-" )
  , sOutputPath( "-" )
  , fdEpochReference( -1.0 )
{
  // Link actual transmission objects
  poTransmit_in = &oTransmit_in;
  poOutputStream = &cout;
}


//----------------------------------------------------------------------
// METHODS: CSgctpUtilSkeleton (implement/override)
//----------------------------------------------------------------------

void CSgctpUtil::displayHelp()
{
  // Display full help
  displayUsageHeader();
  cout << "  sgctp2gpx [options] [<path(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data of the given file (default:'-', standard input) as GPX." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -r, --date-reference <date>" << endl;
  cout << "    Date reference (default:-1; -1=no conversion; 0=current date)" << endl;
  displayOptionPrincipal( true, false );
  displayOptionPayload( true, false );
  displayOptionPassword( true, false );
  displayOptionPasswordSalt( true, false );
  displayOptionExtendedContent();
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
      SGCTP_PARSE_ARGS( parseArgsPrincipal( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPayload( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPassword( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsPasswordSalt( &__i, true, false ) );
      SGCTP_PARSE_ARGS( parseArgsExtendedContent( &__i ) );
      if( __sArg=="-o" || __sArg=="--output" )
      {
        if( ++__i<iArgC )
          sOutputPath = ppcArgV[__i];
      }
      else if( __sArg=="-r" || __sArg=="--date-reference" )
      {
        if( ++__i<iArgC )
          fdEpochReference = CData::fromIso8601( ppcArgV[__i] );
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
  __iReturn = principalLookup( true, false );
  if( __iReturn )
    return __iReturn;

  // Initialize transmission object(s)
  __iReturn = transmitInit( true, false );
  if( __iReturn )
    return __iReturn;

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

    // Open output
    if( sOutputPath != "-" )
    {
      ofstream __ofStreamOutput;
      __ofStreamOutput.open( sOutputPath.c_str(),
                             ofstream::trunc );
      if( !__ofStreamOutput.is_open() )
      {
        SGCTP_LOG << SGCTP_ERROR << "Invalid/unwritable file (" << sOutputPath << ") @ open=" << -errno << endl;
        SGCTP_BREAK( -errno );
      }
      poOutputStream = &__ofStreamOutput;
    }

    // Receive and dump data
    CData __oData;
    *poOutputStream << "<gpx version=\"1.1\" creator=\"SGCTP\">" << endl;
    *poOutputStream << "<trk>" << endl;
    *poOutputStream << "<trkseg>" << endl;
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
      if( __iReturn <= 0 )
      {
        SGCTP_LOG << SGCTP_ERROR << "Failed to unserialize data @ unserialize=" << __iReturn << endl;
        SGCTP_BREAK( __iReturn );
      }

      // ... serialize
      sigBlock();
      dumpGPX( __oData );
      sigUnblock();
    }
    *poOutputStream << "</trkseg>" << endl;
    *poOutputStream << "</trk>" << endl;
    *poOutputStream << "</gpx>" << endl;

  }
  while( false ); // Error-catching block

  // Done
  if( fdInput >= 0 && fdInput != STDIN_FILENO )
    close( fdInput );
  if( poOutputStream != &cout )
    ((ofstream*)poOutputStream)->close();
  return __iExit;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

void CSgctpUtil::dumpGPX( const CData& _roData )
{
  double __fdValue1, __fdValue2;
  if( !CData::isDefined( __fdValue1 = _roData.getLatitude() )
      || !CData::isDefined( __fdValue2 = _roData.getLongitude() ) )
    return;
  *poOutputStream << "<trkpt lat=\"" << (boost::format("%.7f")%__fdValue1).str()
                  << "\" lon=\"" << (boost::format("%.7f")%__fdValue2).str() << "\">" << endl;
  if( CData::isDefined( __fdValue1 = _roData.getElevation() ) )
    *poOutputStream << "<ele>" << (boost::format("%.1f")%__fdValue1).str() << "</ele>" << endl;
  if( CData::isDefined( __fdValue1 = _roData.getTime() ) )
  {
    if( fdEpochReference >= 0 )
      __fdValue1 = CData::toEpoch( __fdValue1, fdEpochReference );
    char __pcIso8601[24];
    CData::toIso8601( __pcIso8601, __fdValue1 );
    *poOutputStream << "<time>" << __pcIso8601 << "</time>" << endl;
  }
  if( bExtendedContent )
  {
    if( CData::isDefined( __fdValue1 = _roData.getLongitudeError() )
        && CData::isDefined( __fdValue2 = _roData.getLatitudeError() ) )
    {
      double __fdDopX = __fdValue1 / H_UERE;
      double __fdDopY = __fdValue2 / H_UERE;
      double __fdDopH = sqrt( __fdDopX*__fdDopX + __fdDopY*__fdDopY );
      *poOutputStream << "<hdop>" << (boost::format("%.1f")%__fdDopH).str() << "</hdop>" << endl;
      double __fdDopZ = 0;
      if( CData::isDefined( __fdValue1 = _roData.getElevationError() ) )
      {
        __fdDopZ = __fdValue1 / V_UERE;
        *poOutputStream << "<vdop>" << (boost::format("%.1f")%__fdDopZ).str() << "</vdop>" << endl;
      }
      double __fdDopP = sqrt( __fdDopX*__fdDopX
                              + __fdDopY*__fdDopY
                              + __fdDopZ*__fdDopZ );
      *poOutputStream << "<pdop>" << (boost::format("%.1f")%__fdDopP).str() << "</pdop>" << endl;
    }
  }
  *poOutputStream << "</trkpt>" << endl;
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpUtil __oSgctpUtil( argc, argv );
  return __oSgctpUtil.exec();
}
