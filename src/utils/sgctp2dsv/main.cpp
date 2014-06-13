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
  : CSgctpUtilSkeleton( "sgctp2dsv", _iArgC, _ppcArgV )
  , fdInput( STDIN_FILENO )
  , sInputPath( "-" )
  , sOutputPath( "-" )
  , sOutputDelimiter( "," )
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
  cout << "  sgctp2dsv [options] [<path(in)>] +<field> [+<field> ...]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data of the given file (default:'-', standard input) as DSV." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -d, --delimiter <string>" << endl;
  cout << "    Delimiter string (default:',')" << endl;
  cout << "  -r, --date-reference <date>" << endl;
  cout << "    Date reference (default:-1; -1=no conversion; 0=current date)" << endl;
  cout << endl << "FIELDS:" << endl;
  cout << "  time, id, lat, lon, ele, bng, gsd, vsd, rtn, gac, vac, hng, asd," << endl;
  cout << "  srct, elat, elon, eele, ebng, egsd, evsd, ertn, egac, evac, ehng, easd" << endl;
  displayOptionPrincipal( true, false );
  displayOptionPayload( true, false );
  displayOptionPassword( true, false );
  displayOptionPasswordSalt( true, false );
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
      if( __sArg=="-o" || __sArg=="--output" )
      {
        if( ++__i<iArgC )
          sOutputPath = ppcArgV[__i];
      }
      else if( __sArg=="-d" || __sArg=="--delimiter" )
      {
        if( ++__i<iArgC )
          sOutputDelimiter = ppcArgV[__i];
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
    else if( __sArg[0] == '+' )
    {
      string __sFld = __sArg.substr( 1 );
      sOutputFields_vector.push_back( __sFld );
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
  if( sOutputFields_vector.empty() )
  {
    SGCTP_LOG << SGCTP_ERROR << "At least one output field must be specified" << endl;
    return -EINVAL;
  }

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
      dumpDSV( __oData );
      sigUnblock();
    }

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

void CSgctpUtil::dumpDSV( const CData& _roData )
{
  double __fdValue;
  int __iValue;
  for( unsigned int __i=0; __i<sOutputFields_vector.size(); __i++ )
  {
    string __sFld = sOutputFields_vector[__i];
    if( __i )
    {
      *poOutputStream << sOutputDelimiter;
    }
    if( __sFld == "time" )
    {
      if( CData::isDefined( __fdValue = _roData.getTime() ) )
      {
        if( fdEpochReference >= 0 )
        {
          __fdValue = CData::toEpoch( __fdValue, fdEpochReference );
          char __pcIso8601[24];
          CData::toIso8601( __pcIso8601, __fdValue );
          *poOutputStream << __pcIso8601;
        }
        else
          *poOutputStream << (boost::format("%.1f")%__fdValue).str();
      }
    }
    else if( __sFld == "id" )
    {
      *poOutputStream << _roData.getID();
    }
    else if( __sFld == "lat" )
    {
      if( CData::isDefined( __fdValue = _roData.getLatitude() ) )
        *poOutputStream << (boost::format("%.7f")%__fdValue).str();
    }
    else if( __sFld == "lon" )
    {
      if( CData::isDefined( __fdValue = _roData.getLongitude() ) )
        *poOutputStream << (boost::format("%.7f")%__fdValue).str();
    }
    else if( __sFld == "ele" )
    {
      if( CData::isDefined( __fdValue = _roData.getElevation() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "bng" )
    {
      if( CData::isDefined( __fdValue = _roData.getBearing() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "gsd" )
    {
      if( CData::isDefined( __fdValue = _roData.getGndSpeed() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "vsd" )
    {
      if( CData::isDefined( __fdValue = _roData.getVrtSpeed() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "rtn" )
    {
      if( CData::isDefined( __fdValue = _roData.getBearingDt() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "gac" )
    {
      if( CData::isDefined( __fdValue = _roData.getGndSpeedDt() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "vac" )
    {
      if( CData::isDefined( __fdValue = _roData.getVrtSpeedDt() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "hng" )
    {
      if( CData::isDefined( __fdValue = _roData.getHeading() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "asd" )
    {
      if( CData::isDefined( __fdValue = _roData.getAppSpeed() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "srct" )
    {
      if( ( __iValue = _roData.getSourceType() ) != CData::SOURCE_UNDEFINED )
        *poOutputStream << CData::stringSourceType( __iValue );
    }
    else if( __sFld == "elat" )
    {
      if( CData::isDefined( __fdValue = _roData.getLatitudeError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "elon" )
    {
      if( CData::isDefined( __fdValue = _roData.getLongitudeError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "eele" )
    {
      if( CData::isDefined( __fdValue = _roData.getElevationError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "ebng" )
    {
      if( CData::isDefined( __fdValue = _roData.getBearingError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "egsd" )
    {
      if( CData::isDefined( __fdValue = _roData.getGndSpeedError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "evsd" )
    {
      if( CData::isDefined( __fdValue = _roData.getVrtSpeedError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "ertn" )
    {
      if( CData::isDefined( __fdValue = _roData.getBearingDtError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "egac" )
    {
      if( CData::isDefined( __fdValue = _roData.getGndSpeedDtError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "evac" )
    {
      if( CData::isDefined( __fdValue = _roData.getVrtSpeedDtError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "ehng" )
    {
      if( CData::isDefined( __fdValue = _roData.getHeadingError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
    else if( __sFld == "easd" )
    {
      if( CData::isDefined( __fdValue = _roData.getAppSpeedError() ) )
        *poOutputStream << (boost::format("%.1f")%__fdValue).str();
    }
  }
  *poOutputStream << endl;
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpUtil __oSgctpUtil( argc, argv );
  return __oSgctpUtil.exec();
}
