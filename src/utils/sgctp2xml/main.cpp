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

// 3rd-party
extern "C" {
#include "base64.h"
}

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

char* CSgctpUtil::dataXML( const unsigned char* _pucData,
                           uint16_t _ui16tDataSize )
{
  static char __pcDataString[CData::MAX_DATA_SIZE+1];

  // Test data for non-ASCII symbols
  for( uint16_t __i = 0; __i < _ui16tDataSize; __i++ )
    if( _pucData[__i] < 32 || _pucData[__i] > 126 )
      return NULL;
  memcpy( __pcDataString, _pucData, _ui16tDataSize );
  __pcDataString[_ui16tDataSize] = '\0';

  // Test data for XML CDATA terminating stanza
  if( strstr( __pcDataString, "]]>" ) )
    return NULL;

  // Done
  return __pcDataString;
}

char* CSgctpUtil::dataBase64( const unsigned char* _pucData,
                              uint16_t _ui16tDataSize )
{
  static const size_t __iDataBase64_size=BASE64_LENGTH(CData::MAX_DATA_SIZE)+1;
  static char __pcDataBase64[__iDataBase64_size];

  // Base64 encoding
  base64_encode( (const char*)_pucData, _ui16tDataSize,
                 __pcDataBase64, __iDataBase64_size );

  // Done
  return __pcDataBase64;
}


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtil::CSgctpUtil( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "sgctp2xml", _iArgC, _ppcArgV )
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
  cout << "  sgctp2xml [options] [<path(in)>]" << endl;
  displaySynopsisHeader();
  cout << "  Dump the SGCTP data of the given file (default:'-', standard input) as XML." << endl;
  displayOptionsHeader();
  cout << "  -o, --output <path>" << endl;
  cout << "    Output file (default:'-', standard output)" << endl;
  cout << "  -r, --date-reference <date>" << endl;
  cout << "    Date reference (default:-1; -1=no conversion; 0=current date)" << endl;
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
    *poOutputStream << "<SGCTP>" << endl;
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
      dumpXML( __oData );
      sigUnblock();
    }
    *poOutputStream << "</SGCTP>" << endl;

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

void CSgctpUtil::dumpXML( const CData& _roData )
{
  double __fdValue;
  int __iValue;
  *poOutputStream << "  <Payload>" << endl;
  if( strlen( _roData.getID() ) )
    *poOutputStream << "    <ID>" << _roData.getID() << "</ID>" << endl;
  if( _roData.getDataSize() )
  {
    char* __pcDataString = dataXML( _roData.getData(), _roData.getDataSize() );
    if( __pcDataString )
      *poOutputStream << "    <Data><![CDATA[" << __pcDataString << "]]></Data>" << endl;
    else
      *poOutputStream << "    <Data64>" << dataBase64( _roData.getData(), _roData.getDataSize() ) << "</Data64>" << endl;
  }
  if( CData::isDefined( __fdValue = _roData.getTime() ) )
  {
    if( fdEpochReference >= 0 )
      __fdValue = CData::toEpoch( __fdValue, fdEpochReference );
    char __pcIso8601[24];
    CData::toIso8601( __pcIso8601, __fdValue );
    *poOutputStream << "    <Time>" << __pcIso8601 << "</Time>" << endl;
  }
  if( CData::isDefined( __fdValue = _roData.getLatitude() ) )
    *poOutputStream << "    <Latitude>" << (boost::format("%.7f")%__fdValue).str() << "</Latitude>" << endl;
  if( CData::isDefined( __fdValue = _roData.getLongitude() ) )
    *poOutputStream << "    <Longitude>" << (boost::format("%.7f")%__fdValue).str() << "</Longitude>" << endl;
  if( CData::isDefined( __fdValue = _roData.getElevation() ) )
    *poOutputStream << "    <Elevation>" << (boost::format("%.1f")%__fdValue).str() << "</Elevation>" << endl;
  if( CData::isDefined( __fdValue = _roData.getBearing() ) )
    *poOutputStream << "    <Bearing>" << (boost::format("%.1f")%__fdValue).str() << "</Bearing>" << endl;
  if( CData::isDefined( __fdValue = _roData.getGndSpeed() ) )
    *poOutputStream << "    <GndSpeed>" << (boost::format("%.1f")%__fdValue).str() << "</GndSpeed>" << endl;
  if( CData::isDefined( __fdValue = _roData.getVrtSpeed() ) )
    *poOutputStream << "    <VrtSpeed>" << (boost::format("%.1f")%__fdValue).str() << "</VrtSpeed>" << endl;
  if( CData::isDefined( __fdValue = _roData.getBearingDt() ) )
    *poOutputStream << "    <BearingDt>" << (boost::format("%.1f")%__fdValue).str() << "</BearingDt>" << endl;
  if( CData::isDefined( __fdValue = _roData.getGndSpeedDt() ) )
    *poOutputStream << "    <GndSpeedDt>" << (boost::format("%.1f")%__fdValue).str() << "</GndSpeedDt>" << endl;
  if( CData::isDefined( __fdValue = _roData.getVrtSpeedDt() ) )
    *poOutputStream << "    <VrtSpeedDt>" << (boost::format("%.1f")%__fdValue).str() << "</VrtSpeedDt>" << endl;
  if( CData::isDefined( __fdValue = _roData.getHeading() ) )
    *poOutputStream << "    <Heading>" << (boost::format("%.1f")%__fdValue).str() << "</Heading>" << endl;
  if( CData::isDefined( __fdValue = _roData.getAppSpeed() ) )
    *poOutputStream << "    <AppSpeed>" << (boost::format("%.1f")%__fdValue).str() << "</AppSpeed>" << endl;
  if( ( __iValue = _roData.getSourceType() ) != CData::SOURCE_UNDEFINED )
    *poOutputStream << "    <SourceType>" << CData::stringSourceType( __iValue ) << "</SourceType>" << endl;
  if( CData::isDefined( __fdValue = _roData.getLatitudeError() ) )
    *poOutputStream << "    <LatitudeError>" << (boost::format("%.1f")%__fdValue).str() << "</LatitudeError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getLongitudeError() ) )
    *poOutputStream << "    <LongitudeError>" << (boost::format("%.1f")%__fdValue).str() << "</LongitudeError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getElevationError() ) )
    *poOutputStream << "    <ElevationError>" << (boost::format("%.1f")%__fdValue).str() << "</ElevationError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getBearingError() ) )
    *poOutputStream << "    <BearingError>" << (boost::format("%.1f")%__fdValue).str() << "</BearingError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getGndSpeedError() ) )
    *poOutputStream << "    <GndSpeedError>" << (boost::format("%.1f")%__fdValue).str() << "</GndSpeedError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getVrtSpeedError() ) )
    *poOutputStream << "    <VrtSpeedError>" << (boost::format("%.1f")%__fdValue).str() << "</VrtSpeedError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getBearingDtError() ) )
    *poOutputStream << "    <BearingDtError>" << (boost::format("%.1f")%__fdValue).str() << "</BearingDtError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getGndSpeedDtError() ) )
    *poOutputStream << "    <GndSpeedDtError>" << (boost::format("%.1f")%__fdValue).str() << "</GndSpeedDtError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getVrtSpeedDtError() ) )
    *poOutputStream << "    <VrtSpeedDtError>" << (boost::format("%.1f")%__fdValue).str() << "</VrtSpeedDtError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getHeadingError() ) )
    *poOutputStream << "    <HeadingError>" << (boost::format("%.1f")%__fdValue).str() << "</HeadingError>" << endl;
  if( CData::isDefined( __fdValue = _roData.getAppSpeedError() ) )
    *poOutputStream << "    <AppSpeedError>" << (boost::format("%.1f")%__fdValue).str() << "</AppSpeedError>" << endl;
  *poOutputStream << "  </Payload>" << endl;
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpUtil __oSgctpUtil( argc, argv );
  return __oSgctpUtil.exec();
}
