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

// SGCTP
#include "../skeleton.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------

class CSgctpUtil: protected CSgctpUtilSkeleton
{

  //----------------------------------------------------------------------
  // STATIC / CONSTANTS
  //----------------------------------------------------------------------

public:
  static void interrupt( int _iSignal );

private:
  /// Returns XML-encoded data
  static char* dataXML( const unsigned char* _pucData,
                        uint16_t _ui16tDataSize );
  /// Returns Base64-encoded data
  static char* dataBase64( const unsigned char* _pucData,
                           uint16_t _ui16tDataSize );


  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

  //
  // Resources
  //

private:
  /// Input transmission object (actual)
  CTransmit_File oTransmit_in;
  /// Input file descriptor
  int fdInput;
  /// Output stream
  ostream* poOutputStream;

  //
  // Arguments
  //

private:
  /// Input file path
  string sInputPath;
  /// Output file path
  string sOutputPath;
  /// Date/time reference epoch
  double fdEpochReference;


  //----------------------------------------------------------------------
  // CONSTRUCTORS / DESTRUCTOR
  //----------------------------------------------------------------------

public:
  CSgctpUtil( int _iArgC, char *_ppcArgV[] );

public:
  virtual ~CSgctpUtil() {};


  //----------------------------------------------------------------------
  // METHODS: CSgctpUtilSkeleton (implement/override)
  //----------------------------------------------------------------------

private:
  virtual void displayHelp();
  virtual int parseArgs();

public:
  virtual int exec();


  //----------------------------------------------------------------------
  // METHODS
  //----------------------------------------------------------------------

private:
  /// Dumps SGCTP payload as XML
  void dumpXML( const CData& _roData );

};
