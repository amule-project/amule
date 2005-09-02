//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "PlatformSpecific.h"	// Interface declarations


#ifdef __WXMAC__
//-------------------- Mac specific code --------------------

#include <CoreServices/CoreServices.h>
#include <wx/mac/corefoundation/cfstring.h>
#include <wx/intl.h>


wxString GetDocumentsDir()
{
	wxString strDir;
	
	FSRef fsRef;
	if (FSFindFolder(kUserDomain, kDocumentsFolderType, kCreateFolder, &fsRef) == noErr)
	{
		CFURLRef	urlRef		= CFURLCreateFromFSRef(NULL, &fsRef);
		CFStringRef	cfString	= CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
		CFRelease(urlRef) ;
		strDir = wxMacCFStringHolder(cfString).AsString(wxLocale::GetSystemEncoding());
	}

	return strDir;
}


#elif defined(__WINDOWS__)
//-------------------- Win32 specific code --------------------

#include <winerror.h>
#include <shlobj.h>

wxString GetDocumentsDir()
{
	wxString strDir;
	LPITEMIDLIST pidl;

	HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);

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

	return strDir;
}


#else
//-------------------- Generic code --------------------

#include <wx/filename.h>

wxString GetDocumentsDir()
{
	return wxFileName::GetHomeDir();
}

#endif
