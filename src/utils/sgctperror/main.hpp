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
  // FIELDS
  //----------------------------------------------------------------------

  //
  // Arguments
  //

private:
  /// Error code
  int iError;


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
  /// Explains the error (corresponding to the saved error code)
  void explainError();

};
