/*
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

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
#include <wx/string.h>

#include "CMD4Hash.h"

/**
 * Extension to wxFileConfig for reading/writing CMD4Hash values.
 *
 * This class converts between the text and binary representation of an MD4 hash,
 * and also maps empty strings to the empty hash and vica versa.
 */
class CECFileConfig : public wxFileConfig {
	public:

		CECFileConfig(const wxString& appName = wxEmptyString,
			const wxString& vendorName = wxEmptyString,
			const wxString& localFilename = wxEmptyString,
			const wxString& globalFilename = wxEmptyString,
			long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
			wxMBConv& conv = wxConvUTF8) 
			: wxFileConfig(appName, vendorName, localFilename, globalFilename, style, conv)
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
