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

#ifndef SGCTP_CDATA_HPP
#define SGCTP_CDATA_HPP

// C
#include <math.h>
#include <stdint.h>
#include <string.h>

// C++
#include <string>
using namespace std;

// SGCTP namespace
namespace SGCTP
{

  /// SGCTP data container
  /**
   * This class encapsulates geolocalization and motion data (in integer/no-
   * precision-loss format).
   * It provides the necessary setters/getters to convert all geolocalization
   * and motion data from/to their SI-standardized form.
   * See the PROTOCOL document to see the meaning of each data field.
   */
  class CData
  {
    friend class CPayload;

    //----------------------------------------------------------------------
    // CONSTANTS / STATIC
    //----------------------------------------------------------------------

  public:
    /// Data source types
    enum ESourceType {
      SOURCE_UNDEFINED = 0,   ///< undefined
      SOURCE_GPS = 1,         ///< Global Positioning System (GPS)
      SOURCE_AIS = 2,         ///< Automatic Identification System (AIS)
      SOURCE_ADSB = 3,        ///< Automatic Dependent Surveillance - Broadcast (ADS-B)
      SOURCE_FLARM = 4        ///< FLight ALarM (FLARM)
    };

  private:
    /// Internal (integer) undefined value
    static const uint32_t UNDEFINED_UINT32 = 0x80000000;
    /// Internal (integer) positive overflow value
    static const uint32_t OVERFLOW_UINT32 = 0x7FFFFFFF;

  public:
    /// Undefined value
    static const double UNDEFINED_VALUE;
    /// Overflow value
    static const double OVERFLOW_VALUE;
    /// Maximum ID length
    static const uint8_t MAX_ID_LENGTH = 127;
    /// Maximum ID size
    static const uint8_t MAX_ID_SIZE = 128;
    /// Maximum data size
    static const uint16_t MAX_DATA_SIZE = 32767;
    /// Return whether the given value is defined
    inline static bool isDefined( double _fdValue )
    {
      return !__isnanl( _fdValue );
    };
    /// Return whether the given value is valid (does not overflow)
    inline static bool isValid( double _fdValue ) {
      return !__isinfl( _fdValue );
    };
    /// Return the human-readable string corresponding to the given source type
    /**
     *  @param[in] _iSourceType Source type (code)
     *  @see ESourceType
     */
    static string stringSourceType( int _iSourceType );
    /// Return the current (sub-second) UNIX epoch
    static double epoch();
    /// Return the UNIX epoch (including sub-second decimals) corresponding to the given SGCTP time (relative to the given epoch)
    /**
     *  @param[in] _fdTime SGCTP time (REMINDER: does NOT contain date information)
     *  @param[in] _fdEpochReference Reference UNIX epoch/data (to obtain date information)
     *  @return UNIX epoch (time AND date)
     */
    static double toEpoch( double _fdTime,
                           double _fdEpochReference = 0 );
    /// Return the ISO-8601 date/time string corresponding to the given UNIX epoch
    /**
     *  @param[in] _pcIso8601 Pointer to the characters string to store the ISO-8601 date/time into
     *  @param[in] _fdEpoch UNIX epoch (time AND date)
     */
    static void toIso8601( char *_pcIso8601,
                           double _fdEpoch );
    /// Return the UNIX epoch corresponding to the given ISO-8601 date/time string
    /**
     *  @param[in] _pcIso8601 Pointer to the characters string containing the ISO-8601 date/time
     *  @return _fdEpoch UNIX epoch (time AND date)
     */
    static double fromIso8601( const char *_pcIso8601 );


    //----------------------------------------------------------------------
    // FIELDS
    //----------------------------------------------------------------------

  private:
    /// ID string (max. 127 characters)
    char pcID[MAX_ID_SIZE];
    /// Data (max. 32767 symbols)
    unsigned char *pucData;
    /// Data size
    uint16_t ui16tDataSize;
    /// Time
    uint32_t ui32tTime;
    /// Latitude
    uint32_t ui32tLatitude;
    /// Longitude
    uint32_t ui32tLongitude;
    /// Elevation
    uint32_t ui32tElevation;
    /// Bearing
    uint32_t ui32tBearing;
    /// Ground speed
    uint32_t ui32tGndSpeed;
    /// Vertical speed
    uint32_t ui32tVrtSpeed;
    /// Bearing variation over time (rate of turn)
    uint32_t ui32tBearingDt;
    /// Ground speed variation over time (acceleration)
    uint32_t ui32tGndSpeedDt;
    /// Vertical speed variation over time (acceleration)
    uint32_t ui32tVrtSpeedDt;
    /// Heading
    uint32_t ui32tHeading;
    /// Apparent speed
    uint32_t ui32tAppSpeed;
    /// Source type
    uint32_t ui32tSourceType;
    /// Latitude error
    uint32_t ui32tLatitudeError;
    /// Longitude error
    uint32_t ui32tLongitudeError;
    /// Elevation error
    uint32_t ui32tElevationError;
    /// Bearing error
    uint32_t ui32tBearingError;
    /// Ground speed error
    uint32_t ui32tGndSpeedError;
    /// Vertical speed error
    uint32_t ui32tVrtSpeedError;
    /// Bearing variation over time (rate of turn) error
    uint32_t ui32tBearingDtError;
    /// Ground speed variation over time (acceleration) error
    uint32_t ui32tGndSpeedDtError;
    /// Vertical speed variation over time (acceleration) error
    uint32_t ui32tVrtSpeedDtError;
    /// Heading error
    uint32_t ui32tHeadingError;
    /// Apparent speed error
    uint32_t ui32tAppSpeedError;


    //----------------------------------------------------------------------
    // CONSTRUCTORS / DESTRUCTOR
    //----------------------------------------------------------------------

  public:
    CData()
      : pucData( NULL )
    {
      reset();
    }
    ~CData();


    //----------------------------------------------------------------------
    // METHODS
    //----------------------------------------------------------------------

    //
    // SETTERS
    //

  public:
    /// Reset (undefine) all data
    /**
     *  @param[in] _bDataFree Whether to reset (undefine) the "data" field
     */
    void reset( bool _bDataFree = true );
    /// Set the ID string (max. 127 characters)
    void setID( const char *_pcID );
    /// Set the data (max. 32767 symbols)
    uint16_t setData( const unsigned char *_pucData,
                      uint16_t _ui16tDataSize );
    /// Set the time, in seconds (UTC)
    /**
     *  @param[in] _fdEpoch UNIX epoch; date part will be stripped out
     */
    void setTime( double _fdEpoch );
    /// Set the latitude, in degrees
    void setLatitude( double _fdLatitude );
    /// Set the longitude, in degrees
    void setLongitude( double _fdLongitude );
    /// Set the elevation, in meters
    void setElevation( double _fdElevation );
    /// Set the bearing, in degrees
    void setBearing( double _fdBearing );
    /// Set the ground speed, in meters per second
    void setGndSpeed( double _fdGndSpeed );
    /// Set the vertical speed, in meters per second
    void setVrtSpeed( double _fdVrtSpeed );
    /// Set the bearing variation over time (rate of turn), in degrees per second
    void setBearingDt( double _fdBearingDt );
    /// Set the ground speed variation over time (acceleration), in meters per second^2
    void setGndSpeedDt( double _fdGndSpeedDt );
    /// Set the vertical speed variation over time (acceleration), in meters per second^2
    void setVrtSpeedDt( double _fdVrtSpeedDt );
    /// Set the heading, in degrees
    void setHeading( double _fdHeading );
    /// Set the apparent speed, in meters per second
    void setAppSpeed( double _fdApparentSpeed );
    /// Set the source type
    void setSourceType( ESourceType _eSourceType )
    { ui32tSourceType =
        ( _eSourceType != SOURCE_UNDEFINED )
        ? _eSourceType
        : UNDEFINED_UINT32;
    };
    /// Set the latitude error, in meters
    void setLatitudeError( double _fdLatitudeError );
    /// Set the longitude error, in meters
    void setLongitudeError( double _fdLongitudeError );
    /// Set the elevation error, in meters
    void setElevationError( double _fdElevationError );
    /// Set the bearing error, in degrees
    void setBearingError( double _fdBearingError );
    /// Set the ground speed error, in meters per second
    void setGndSpeedError( double _fdGndSpeedError );
    /// Set the vertical speed error, in meters per second
    void setVrtSpeedError( double _fdVrtSpeedError );
    /// Set the bearing variation over time (rate of turn) error, in degrees per second
    void setBearingDtError( double _fdBearingDtError );
    /// Set the ground speed variation over time (acceleration) error, in meters per second^2
    void setGndSpeedDtError( double _fdGndSpeedDtError );
    /// Set the vertical speed variation over time (acceleration) error, in meters per second^2
    void setVrtSpeedDtError( double _fdVrtSpeedDtError );
    /// Set the heading error, in degrees
    void setHeadingError( double _fdHeadingError );
    /// Set the apparent speed error, in meters per second
    void setAppSpeedError( double _fdApparentSpeedError );


    //
    // GETTERS
    //

  public:
    /// Return the ID string
    const char* getID() const
    {
      return pcID;
    };
    /// Return the data (max. 32767 symbols)
    void getData( unsigned char *_pucData,
                  uint16_t *_pui16tDataSize ) const;
    /// Return the data
    const unsigned char* getData() const
    {
      return pucData;
    };
    /// Return the data size
    const uint16_t getDataSize() const
    {
      return ui16tDataSize;
    };
    /// Return the time (inclusing sub-second decimals), in seconds from 00:00:00 (UTC)
    double getTime() const;
    /// Return the latitude, in degrees
    double getLatitude() const;
    /// Return the longitude, in degrees
    double getLongitude() const;
    /// Return the elevation, in meters
    double getElevation() const;
    /// Return the bearing, in degrees
    double getBearing() const;
    /// Return the ground speed, in meters per second
    double getGndSpeed() const;
    /// Return the vertical speed, in meters per second
    double getVrtSpeed() const;
    /// Return the bearing variation over time (rate of turn), in degrees per second
    double getBearingDt() const;
    /// Return the ground speed variation over time (acceleration), in meters per second^2
    double getGndSpeedDt() const;
    /// Return the vertical speed variation over time (acceleration), in meters per second^2
    double getVrtSpeedDt() const;
    /// Return the heading, in degrees
    double getHeading() const;
    /// Return the apparent speed, in meters per second
    double getAppSpeed() const;
    /// Return the source type
    ESourceType getSourceType() const
    {
      return (ESourceType)(ui32tSourceType & 0xFF);
    };
    /// Return the latitude error, in meters
    double getLatitudeError() const;
    /// Return the longitude error, in meters
    double getLongitudeError() const;
    /// Return the elevation error, in meters
    double getElevationError() const;
    /// Return the bearing error, in degrees
    double getBearingError() const;
    /// Return the ground speed error, in meters per second
    double getGndSpeedError() const;
    /// Return the vertical speed error, in meters per second
    double getVrtSpeedError() const;
    /// Return the bearing variation over time (rate of turn) error, in degrees per second
    double getBearingDtError() const;
    /// Return the ground speed variation over time (acceleration) error, in meters per second^2
    double getGndSpeedDtError() const;
    /// Return the vertical speed variation over time (acceleration) error, in meters per second^2
    double getVrtSpeedDtError() const;
    /// Return the heading error, in degrees
    double getHeadingError() const;
    /// Return the apparent speed error, in meters per second
    double getAppSpeedError() const;


    //
    // OTHERS
    //

  private:
    /// Allocate the memory for the data container (max. 32767 symbols)
    uint16_t allocData( uint16_t _ui16tDataSize );

  public:
    /// Clear (free) the data container
    void freeData();
    /// Copy the entire content from another data object
    void copy( const CData &_roData );
    /// Synchronize the (defined) content from another data object
    /**
     *  @return Whether some (defined) content has actually been synchronized
     */
    bool sync( const CData &_roData );

  };

}

#endif // SGCTP_CDATA_HPP
