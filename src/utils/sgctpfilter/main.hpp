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

class CDataPrevious
{
public:
  double fdEpoch;
  double fdLatitude;
  double fdLongitude;
  double fdElevation;
  CDataPrevious()
    : fdEpoch( CData::UNDEFINED_VALUE )
    , fdLatitude( CData::UNDEFINED_VALUE )
    , fdLongitude( CData::UNDEFINED_VALUE )
    , fdElevation( CData::UNDEFINED_VALUE )
  {};
};

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
  CTransmit_File oTransmit_in;
  /// Input file descriptor
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

  /// Replay rate
  double fdReplayRate;
  /// Source ID
  string sID;
  /// Time throttling delay, in seconds
  double fdTimeThrottle;
  /// Time threshold, in meters
  double fdDistanceThreshold;
  /// Reference latitude, in degrees
  double fdLatitude0;
  /// Reference longitude, in degrees
  double fdLongitude0;
  /// 1st time limit, in seconds
  double fdTime1;
  /// 1st latitude limit, in degrees
  double fdLatitude1;
  /// 1st longitude limit, in degrees
  double fdLongitude1;
  /// 1st elevation limit, in meters
  double fdElevation1;
  /// 2nd time limit, in seconds
  double fdTime2;
  /// 2nd latitude limit, in degrees
  double fdLatitude2;
  /// 2nd longitude limit, in degrees
  double fdLongitude2;
  /// 2nd elevation limit, in meters
  double fdElevation2;

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


  //----------------------------------------------------------------------
  // METHODS
  //----------------------------------------------------------------------

private:
  /// Dumps SGCTP payload as XML
  void dumpXML( const CData& _roData );

};
