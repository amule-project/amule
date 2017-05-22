//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef DATATOTEXT_H
#define DATATOTEXT_H

class wxString;

// Returns the textual representation of a priority value
wxString PriorityToStr( int priority, bool isAuto );

// Returns the textual representation of download states
wxString DownloadStateToStr( int state, bool queueFull );

/**
 * @return Human-readable client software name.
 */
const wxString GetSoftName( unsigned int software_ident );

/**
 * Get "Source From" text, i.e. where we got the source from.
 *
 * @param source_from A ESourceFrom enum value.
 * @return Human-readable text for the ESourceFrom enum values.
 */
wxString OriginToText(unsigned int source_from);

/**
 * @return The textual representation of a partfile conversion state.
 */
wxString GetConversionState(unsigned int state);

#endif /* DATATOTEXT_H */
// File_checked_for_headers
