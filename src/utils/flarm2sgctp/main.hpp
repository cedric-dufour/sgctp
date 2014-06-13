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
  // Returns whether the given string is a valid NMEA sentence (including checksum)
  static bool nmeaValid( const string &_rsNMEA );
  // Returns the sub-second time corresponding to the given NMEA time field
  static double nmeaTime( const string &_rsTime );
  // Returns the latitude corresponding to the given NMEA latitude/quadrant fields
  static double nmeaLatitude( const string &_rsLatitude,
                              const string &_rsQuadrant );
  // Returns the longitude corresponding to the given NMEA longitude/quadrant fields
  static double nmeaLongitude( const string &_rsLongitude,
                               const string &_rsQuadrant );


  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

  //
  // Resources
  //

private:
  // Input file descriptor
  int fdInput;
  /// Output transmission object (actual)
  CTransmit_File oTransmit_out;
  /// Output file descriptor
  int fdOutput;

  //
  // Arguments
  //

private:
  /// Input file path
  string sInputPath;
  /// Output file path
  string sOutputPath;
  /// Input (serial port) baud rate
  int iInputBaudRate;
  /// FLARM (GPS) device ID
  string sID;
  /// GPS-only data inclusion flag
  bool bGpsOnly;
  /// FLARM-only data inclusion flag
  bool bFlarmOnly;
  /// Barometric elevation usage flag
  bool bBarometricElevation;
  /// Internal data Time-To-Live (TTL), in seconds
  int iDataTTL;


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

};
