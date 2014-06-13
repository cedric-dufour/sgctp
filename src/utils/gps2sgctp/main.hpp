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

// GPSD
#include "gps.h"

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
  /// Input GPSD data (actual)
  struct gps_data_t tGpsDataT;
  /// Input GPSD data pointer (link)
  struct gps_data_t * ptGpsDataT;
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
  /// Default GPS device ID
  string sID;
  /// Fix Time-To-Live (TTL), in seconds
  double fdFixTTL;
  /// 2D fix handling
  bool b2DFixOnly;


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
