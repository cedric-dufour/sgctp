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


  //----------------------------------------------------------------------
  // FIELDS
  //----------------------------------------------------------------------

  //
  // Resources
  //

private:
  /// Input transmission object (actual)
  CTransmit_TCP oTransmit_in;
  /// Input socket descriptor
  int sdInput;
  /// Input socket address info pointer
  struct addrinfo* ptAddrinfo_in;
  /// Output transmission object (actual)
  CTransmit_File oTransmit_out;
  /// Output file descriptor
  int fdOutput;

  //
  // Arguments
  //

private:
  /// Input host (IP/name)
  string sInputHost;
  /// Input host port
  string sInputPort;
  /// Output file path
  string sOutputPath;

  /// Filter data: reference latitude, in degrees
  double fdLatitude0;
  /// Filter data: reference longitude, in degrees
  double fdLongitude0;
  /// Filter data: 1st time limit, in seconds
  double fdTime1;
  /// Filter data: 1st latitude limit, in degrees
  double fdLatitude1;
  /// Filter data: 1st longitude limit, in degrees
  double fdLongitude1;
  /// Filter data: 1st elevation limit, in meters
  double fdElevation1;
  /// Filter data: 2nd time limit, in seconds
  double fdTime2;
  /// Filter data: 2nd latitude limit, in degrees
  double fdLatitude2;
  /// Filter data: 2nd longitude limit, in degrees
  double fdLongitude2;
  /// Filter data: 2nd elevation limit, in meters
  double fdElevation2;


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
