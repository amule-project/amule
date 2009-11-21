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
#include "Logger.h"
#include "GuiEvents.h"			// Needed for Notify_*

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

