//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

//
// This file is for functions common to all three apps (amule, amuled, amulegui),
// but preprocessor-dependent (using theApp, thePrefs), so it is compiled seperately for each app.
//


#include <wx/wx.h>
#include <wx/textfile.h>		// Needed for wxTextFile

#include "amule.h"				// Interface declarations.
#include <common/Format.h>		// Needed for CFormat
#include "CFile.h"				// Needed for CFile
#include "FileLock.h"			// Needed for CFileLock
#include "GuiEvents.h"			// Needed for Notify_*
#include "KnownFile.h"
#include "Logger.h"
#include "MagnetURI.h"			// Needed for CMagnetURI
#include "Preferences.h"

#ifndef CLIENT_GUI
#include "DownloadQueue.h"
#endif

void CamuleAppCommon::AddLinksFromFile()
{
	const wxString fullPath = theApp->ConfigDir + wxT("ED2KLinks");
	if (!wxFile::Exists(fullPath)) {
		return;
	}
	
	// Attempt to lock the ED2KLinks file.
	CFileLock lock((const char*)unicode2char(fullPath));

	wxTextFile file(fullPath);
	if ( file.Open() ) {
		for ( unsigned int i = 0; i < file.GetLineCount(); i++ ) {
			wxString line = file.GetLine( i ).Strip( wxString::both );
			
			if ( !line.IsEmpty() ) {
				// Special case! used by a secondary running mule to raise this one.
				if (line == wxT("RAISE_DIALOG")) {
					Notify_ShowGUI();
					continue;
				}
				unsigned long category = 0;
				if (line.AfterLast(wxT(':')).ToULong(&category) == true) {
					line = line.BeforeLast(wxT(':'));
				} else { // If ToULong returns false the category still can have been changed!
						 // This is fixed in wx 2.9
					category = 0;
				}
				theApp->downloadqueue->AddLink(line, category);
			}
		}

		file.Close();
	} else {
		AddLogLineNS(_("Failed to open ED2KLinks file."));
	}
	
	// Delete the file.
	wxRemoveFile(theApp->ConfigDir + wxT("ED2KLinks"));
}


// Returns a magnet ed2k URI
wxString CamuleAppCommon::CreateMagnetLink(const CAbstractFile *f)
{
	CMagnetURI uri;

	uri.AddField(wxT("dn"), f->GetFileName().Cleanup(false).GetPrintable());
	uri.AddField(wxT("xt"), wxString(wxT("urn:ed2k:")) + f->GetFileHash().Encode().Lower());
	uri.AddField(wxT("xt"), wxString(wxT("urn:ed2khash:")) + f->GetFileHash().Encode().Lower());
	uri.AddField(wxT("xl"), CFormat(wxT("%d")) % f->GetFileSize());

	return uri.GetLink();
}

// Returns a ed2k file URL
wxString CamuleAppCommon::CreateED2kLink(const CAbstractFile *f, bool add_source, bool use_hostname, bool addcryptoptions)
{
	wxASSERT(!(!add_source && (use_hostname || addcryptoptions)));
	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	wxString strURL = CFormat(wxT("ed2k://|file|%s|%i|%s|/"))
		% f->GetFileName().Cleanup(false)
		% f->GetFileSize() % f->GetFileHash().Encode();
	
	if (add_source && theApp->IsConnected() && !theApp->IsFirewalled()) {
		// Create the first part of the URL
		strURL << wxT("|sources,");
		if (use_hostname) {
			strURL << thePrefs::GetYourHostname();
		} else {
			uint32 clientID = theApp->GetID();
			strURL << (uint8) clientID << wxT(".") <<
			(uint8)(clientID >> 8) << wxT(".") <<
			(uint8)(clientID >> 16) << wxT(".") <<
			(uint8)(clientID >> 24);
		}
		
 		strURL << wxT(":") <<
			thePrefs::GetPort();
		
		if (addcryptoptions) {
			const uint8 uSupportsCryptLayer	= thePrefs::IsClientCryptLayerSupported() ? 1 : 0;
			const uint8 uRequestsCryptLayer	= thePrefs::IsClientCryptLayerRequested() ? 1 : 0;
			const uint8 uRequiresCryptLayer	= thePrefs::IsClientCryptLayerRequired() ? 1 : 0;
			const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0) | (uSupportsCryptLayer ? 0x80 : 0x00);
			
			strURL << wxT(":") << byCryptOptions;
			
			if (byCryptOptions & 0x80) {
				strURL << wxT(":") << thePrefs::GetUserHash().Encode();
			}
		}
		strURL << wxT("|/");
	} else if (add_source) {
		AddLogLineM(true, _("WARNING: You can't add yourself as a source for an eD2k link while having a lowid."));
	}

	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,[(<ip>|<hostname>):<port>[:cryptoptions[:hash]]]|/"
	return strURL;
}

// Returns a ed2k link with AICH info if available
wxString CamuleAppCommon::CreateED2kAICHLink(const CKnownFile* f)
{
	// Create the first part of the URL
	wxString strURL = CreateED2kLink(f);
	// Append the AICH info
	if (f->HasProperAICHHashSet()) {
		strURL.RemoveLast();		// remove trailing '/'
		strURL << wxT("h=") << f->GetAICHMasterHash() << wxT("|/");
	}	

	// Result is "ed2k://|file|<filename>|<size>|<hash>|h=<AICH master hash>|/"
	return strURL;
}

