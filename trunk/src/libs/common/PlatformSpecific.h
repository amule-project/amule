//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef PLATFORMSPECIFIC_H
#define PLATFORMSPECIFIC_H

#include <wx/string.h>	// Needed for wxString

/**
 * Returs the location of the "Documents" folder of the current user.
 */
wxString GetDocumentsDir();


#if (defined(__WXMSW__) && !wxCHECK_VERSION_FULL(2,6,0,1))
/**
 * Reimplementation of wxStandardPaths::GetUserDataDir() for wxMSW 2.6.0.0
 */
wxString GetUserDataDir();

#else

#include <wx/stdpaths.h>

inline wxString GetUserDataDir() { return wxStandardPaths::Get().GetUserDataDir(); }
#endif


#endif /* PLATFORMSPECIFIC_H */
