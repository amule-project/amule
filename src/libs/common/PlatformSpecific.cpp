//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "PlatformSpecific.h"	// Interface declarations // Do_not_auto_remove

#ifdef __WXMAC__
	#include <CoreServices/CoreServices.h> // Do_not_auto_remove
	#include <wx/mac/corefoundation/cfstring.h> // Do_not_auto_remove
	#include <wx/intl.h> // Do_not_auto_remove
#elif defined(__WINDOWS__)
	#include <winerror.h> // Do_not_auto_remove
	#include <shlobj.h> // Do_not_auto_remove
#else
	#include <wx/filename.h> // Do_not_auto_remove
#endif


wxString doGetDirectory(int whichDir)
{
	wxString strDir;

#ifdef __WXMAC__

	FSRef fsRef;
	if (FSFindFolder(kUserDomain, whichDir, kCreateFolder, &fsRef) == noErr)
	{
		CFURLRef	urlRef		= CFURLCreateFromFSRef(NULL, &fsRef);
		CFStringRef	cfString	= CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
		CFRelease(urlRef) ;
		strDir = wxMacCFStringHolder(cfString).AsString(wxLocale::GetSystemEncoding());
	}

#elif defined(__WINDOWS__)
 
	LPITEMIDLIST pidl;

	HRESULT hr = SHGetSpecialFolderLocation(NULL, whichDir, &pidl);

	if (SUCCEEDED(hr)) {
		if (!SHGetPathFromIDList(pidl, wxStringBuffer(strDir, MAX_PATH))) {
			strDir = wxEmptyString;
		}
	}

	if (pidl) {
		LPMALLOC pMalloc;
		SHGetMalloc(&pMalloc);
		if (pMalloc) {
			pMalloc->Free(pidl);
			pMalloc->Release();
		}
	}

#else
	(void)whichDir; // Avoid unused-variable warning.

	strDir = wxFileName::GetHomeDir();
#endif

	return strDir;
}



#ifdef __WXMAC__

wxString GetDocumentsDir()
{
	return doGetDirectory(kDocumentsFolderType);
}



#elif defined(__WINDOWS__)

wxString GetDocumentsDir()
{
	return doGetDirectory(CSIDL_PERSONAL);
}

#else

wxString GetDocumentsDir()
{
	return doGetDirectory(0);
}


#endif
// File_checked_for_headers
