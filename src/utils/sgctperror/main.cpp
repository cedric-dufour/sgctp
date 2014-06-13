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
#include <unistd.h>

// SGCTP
#include "main.hpp"
using namespace SGCTP;


//----------------------------------------------------------------------
// CONSTRUCTORS / DESTRUCTOR
//----------------------------------------------------------------------

CSgctpUtil::CSgctpUtil( int _iArgC, char *_ppcArgV[] )
  : CSgctpUtilSkeleton( "sgctperror", _iArgC, _ppcArgV )
{}


//----------------------------------------------------------------------
// METHODS: CSgctpUtilSkeleton (implement/override)
//----------------------------------------------------------------------

void CSgctpUtil::displayHelp()
{
  // Display full help
  displayUsageHeader();
  cout << "  sgctperror [options] <error-code>" << endl;
  displaySynopsisHeader();
  cout << "  Explains the SGCTP error corresponding to the given error code." << endl;
  displayOptionsHeader();
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
      if( __sArg[0] == '-' )
      {
        switch( __iArgCount++ )
        {
        case 0:
          iError = atoi( __sArg.c_str() );
          break;
        default:
          displayErrorExtraArgument( __sArg );
          return -EINVAL;
        }
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
        iError = atoi( __sArg.c_str() );
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

int CSgctpUtil::exec()
{
  int __iReturn;

  // Parse arguments
  __iReturn = parseArgs();
  if( __iReturn )
    return __iReturn;

  // Explain error
  explainError();

  // Done
  return 0;
}


//----------------------------------------------------------------------
// METHODS
//----------------------------------------------------------------------

void CSgctpUtil::explainError()
{
  switch( iError )
  {

  case 0:
    cout << "NO ERROR (" << to_string( iError ) << ") - No error" << endl;
    cout << endl;
    cout << "Everything is fine! Isn't it?" << endl;
    break;

  case -EPERM:
    cout << "EPERM (" << to_string( iError ) << ") - Permission denied" << endl;
    cout << endl;
    cout << "The principal is listed in the principal database but the payload type does not" << endl;
    cout << "match any of its authorized payload types." << endl;
    break;

  case -EIO:
    cout << "EIO (" << to_string( iError ) << ") - Transmission (TX) error" << endl;
    cout << endl;
    cout << "An error occured while sending data (fewer data than expected could be sent)." << endl;
    break;

  case -ENOMEM:
    cout << "ENOMEM (" << to_string( iError ) << ") - Memory allocation failure" << endl;
    cout << endl;
    cout << "Dynamic content memory could not be allocated (out of memory?)." << endl;
    break;

  case -EACCES:
    cout << "EACCES (" << to_string( iError ) << ") - Permission denied" << endl;
    cout << endl;
    cout << "The principal could not be found in the principals databases. Or the principals" << endl;
    cout << "database could not be read (correct path/permissions?)." << endl;
    break;

  case -EINVAL:
    cout << "EINVAL (" << to_string( iError ) << ") - Invalid argument" << endl;
    cout << endl;
    cout << "An invalid argument was passed to the program or within the library." << endl;
    break;

  case -EPIPE:
    cout << "EPIPE (" << to_string( iError ) << ") - Broken pipe" << endl;
    cout << endl;
    cout << "The destination of the data transmission (TX) has been disconnected/interrupted." << endl;
    break;

  case -ENOSYS:
    cout << "ENOSYS (" << to_string( iError ) << ") - Function not implemented" << endl;
    cout << endl;
    cout << "Internal components of the program or the library could not be found or" << endl;
    cout << "properly initialized." << endl;
    break;

  case -EWOULDBLOCK:
    cout << "EWOULDBLOCK (" << to_string( iError ) << ") - Transmission timeout" << endl;
    cout << endl;
    cout << "Data transmission (RX or TX) timed out (no data received)." << endl;
    break;

  case -EBADE:
    cout << "EBADE (" << to_string( iError ) << ") - Bad exchange" << endl;
    cout << endl;
    cout << "Transmitted (RX) data are corrupted. This most likely indicates wrong password" << endl;
    cout << "for encrypted/authenticated payload types." << endl;
    break;

  case -ENODATA:
    cout << "ENODATA (" << to_string( iError ) << ") - No data" << endl;
    cout << endl;
    cout << "The program or library attempted to use unitialized data. This indicates a" << endl;
    cout << "programming error in the program or the library (Ouch!)." << endl;
    break;

  case -EPROTO:
    cout << "EPROTO (" << to_string( iError ) << ") - Protocol error" << endl;
    cout << endl;
    cout << "Transmitted (RX) data violates the protocol specifications (missing or" << endl;
    cout << "unexpected data)." << endl;
    break;

  case -EBADMSG:
    cout << "EBADMSG (" << to_string( iError ) << ") - Bad message" << endl;
    cout << endl;
    cout << "Transmitted (RX) data could not be decoded (corrupted data). This may indicated" << endl;
    cout << "a protocol version mismatch between parties." << endl;
    break;

  case -EOVERFLOW:
    cout << "EOVERFLOW (" << to_string( iError ) << ") - Data overflow" << endl;
    cout << endl;
    cout << "Data size exceed what the program or library can handle." << endl;
    break;

  case -EMSGSIZE:
    cout << "EMSGSIZE (" << to_string( iError ) << ") - Bad message size" << endl;
    cout << endl;
    cout << "Transmitted (RX) data are bigger than expected." << endl;
    break;

  case -ETIMEDOUT:
    cout << "ETIMEDOUT (" << to_string( iError ) << ") - Transmission timeout" << endl;
    cout << endl;
    cout << "Data transmission (RX or TX) timed out (not enough data received)." << endl;
    break;

  default:
    cout << "UNKNOWN (" << to_string( iError ) << ")" << endl;
    cout << endl;
    cout << "The given error is not used/recognized by the program or library." << endl;
    cout << "Please refer to your platform's 'errno.h' for potential further information." << endl;
  }
}


//----------------------------------------------------------------------
// MAIN
//----------------------------------------------------------------------

int main( int argc, char* argv[] )
{
  CSgctpUtil __oSgctpUtil( argc, argv );
  return __oSgctpUtil.exec();
}
