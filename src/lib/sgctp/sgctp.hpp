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

/** @mainpage Simple Geolocalization and Course Transmission Protocol (SGCTP)
 *  @author Cedric Dufour <http://cedric.dufour.name>
 *
 *  @tableofcontents
 *
 *  @section synopsis Synopsis
 *
 *  The objective of the Simple Geolocalization and Course Transmission
 *  Protocol (SGCTP) is to transmit 2- or 3-dimensional position and motion data
 *  in such a way as to use as little bandwidth as possible, thus making it fit
 *  to aggregate data from roaming clients with limited network connectivity.
 *
 *  It can be used to:
 *   - transmit data via UDP, with lowest possible bandwidth but lesser reliability
 *     and security
 *   - transmit data via TCP, with guaranteed reliability and security
 *   - store data to file
 *   - all of it along optional encryption (and inherent password authentication)
 *
 *
 *  @section resources Resources
 *
 *  The Simple Geolocalization and Course Transmission Protocol (SGCTP) source
 *  tree contains the following resources:
 *
 *  * A technical description of the protocol (see the PROTOCOL document)
 *
 *  * A reference C/C++ implementation of the protocol:
 *     - libsgctp.so: dynamic library
 *     - libsgctp.a:  static library
 *
 *  * Several utilities...
 *    ... for devices interfacing:
 *     - gps2sgctp: convert GPSD data to SGCTP data
 *     - ais2sgctp: convert GPSD (AIS) data to SGCTP data
 *     - sbs2sgctp: convert SBS-1 data to SGCTP data
 *     - flarm2sgctp: convert FLARM data to SGCTP data
 *    ... for network transmission:
 *     - sgctp2tdp: send SGCTP data via TCP
 *     - tcp2sgctp: receive SGCTP data via TCP (single client)
 *     - sgctp2udp: send SGCTP data via UDP
 *     - udp2sgctp: receive SGCTP data via UDP
 *    ... for data display or storage
 *     - sgctp2dsv: decode and display SGCTP data as DSV
 *     - sgctp2xml: decode and display SGCTP data as XML
 *     - sgctp2gpx: decode and display SGCTP data as GPX
 *    ... for data aggregation and polling
 *     - sgctphub: aggregates and redistributes the data sent by SGCTP agents
 *     - hub2sgctp: connects to an SGCTP hub and receives SGCTP data
 *    ... for data filtering
 *     - sgctpfilter: filter SGCTP data according to specified rules
 *    ... for developers/administrators
 *     - sgctperror: explains error codes return by SGCTP utilities/library
 *
 *
 *  @section copyright Copyright
 *
 *  The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 *  free software:
 *  you can redistribute it and/or modify it under the terms of the GNU General
 *  Public License as published by the Free Software Foundation, Version 3.
 *
 *  The Simple Geolocalization and Course Transmission Protocol (SGCTP) is
 *  distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *  PARTICULAR PURPOSE.
 *
 *  See the GNU General Public License for more details.
 *
 *  The Simple Geolocalization and Course Transmission Protocol (SGCTP) includes
 *  all resources which contain the following mention in their preamble:
 *  "Simple Geolocalization and Course Transmission Protocol (SGCTP)"
 *
 *  Other resources are (and must be) used according to their original license,
 *  which is (should be made) available from their respective author.
 *
 *
 *  @section install Build and Install
 *
 *  The first step is recovering the source code using the GIT versioning system:
 *
 *  @verbatim
mkdir /path/to/source && cd /path/to/source
git clone https://github.com/cedric-dufour/sgctp
@endverbatim
 *
 *  Build environment configuration is then achieved using CMake:
 *
 *  @verbatim
mkdir /path/to/build && cd /path/to/build
cmake /path/to/source
@endverbatim
 *
 *  Compilation and installation are then achieved using regular Make:
 *
 *  @verbatim
cd /path/to/build
make && ls -al ./lib ./bin
make doc && ls -al ./doc
make install
@endverbatim
 *
 *  Alternatively (to Make), distribution-specific Debian packages can be built
 *  and installed using the ad-hoc commands:
 *
 *  @verbatim
cd /path/to/source
debuild -us -uc -b && ls -al ../{lib,}sgctp*.deb
dpkg -i ../{lib,}sgctp*.deb
@endverbatim
 *
 *
 *  @section development Contribute to the Development
 *
 *  @subsection development_coding Coding Conventions
 *
 *  If you are to submit patches for bug-fixing or features-enhancement purposes,
 *  please make sure to stick to the coding conventions detailed in the CODING
 *  file located in the root of the source tree. Patches that do not respect those
 *  conventions *will not* be considered.
 */

#ifndef SGCTP_HPP
#define SGCTP_HPP

// SGCTP
#include "sgctp/version.hpp"
#include "sgctp/data.hpp"
#include "sgctp/payload.hpp"
#include "sgctp/principal.hpp"
#include "sgctp/transmit_udp.hpp"
#include "sgctp/transmit_tcp.hpp"
#include "sgctp/transmit_file.hpp"

#endif // SGCTP_HPP
