//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef	ECFILECONFIG_H
#define	ECFILECONFIG_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ECFileConfig.h"
// implementation in ExternalConnector.cpp
#endif

#if wxCHECK_VERSION(2,4,2)
	#include <wx/config.h>
#endif
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/string.h>

#include "CMD4Hash.h"		// Needed for CMD4Hash
#include "OtherFunctions.h"	// Needed for GetConfigDir()


/**
 * Prepends ConfigDir to filename, if it has no PathSeparator chars in it
 */
inline wxString FinalizeFilename(const wxString filename)
{
	if (wxStrchr(filename, wxFileName::GetPathSeparator()) == NULL) {
		return GetConfigDir() + filename;
	}
	if ((filename.GetChar(0) == '~') && (filename.GetChar(1) == wxFileName::GetPathSeparator())) {
		return wxGetHomeDir() + filename.Mid(1);
	}
	return filename;
}


/**
 * Extension to wxFileConfig for reading/writing CMD4Hash values.
 *
 * This class converts between the text and binary representation of an MD4 hash,
 * and also maps empty strings to the empty hash and vica versa.
 */
class CECFileConfig : public wxFileConfig {
	public:

		CECFileConfig(const wxString& localFilename = wxEmptyString)
			: wxFileConfig(wxEmptyString, wxEmptyString, FinalizeFilename(localFilename),
				wxEmptyString, wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_RELATIVE_PATH)
			{}

		/**
		 * Reads a hash from the config file
		 *
		 * @param key	the key to be read
		 * @param hash	the CMD4Hash object to write the hash to
		 *
		 * @return true on success, false otherwise.
		 */
		bool	ReadHash(const wxString& key, CMD4Hash *hash)
		{
			wxString sHash;
			bool retval = wxFileConfig::Read(key, &sHash, wxEmptyString);
			if (sHash.IsEmpty()) {
				hash->Clear();
			} else {
				hash->Decode(sHash);
			}
			return retval;
		}

		/**
		 * Writes a CMD4Hash object to the config file
		 *
		 * @param key	the key to write to
		 * @param hash	the hash to be written
		 *
		 * @return true on success, false otherwise.
		 */
		bool	WriteHash(const wxString& key, const CMD4Hash& hash)
		{
			return wxFileConfig::Write(key, hash.IsEmpty() ? wxString(wxEmptyString) : hash.Encode());
		}
};

#endif /* ECFILECONFIG_H */
