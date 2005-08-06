//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "IPFilter.h"
#pragma implementation "RangeMap.h"
#endif

#include <wx/intl.h>		// Needed for _()
#include <wx/filefn.h>		// Needed for wxFileExists
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/string.h>		// Needed for wxString
#include <wx/zipstrm.h>		// Needed for wxZipInputStream
#include <wx/zstream.h>		// Needed for wxZlibInputStream
#include <wx/wfstream.h>	// wxFileInputStream
#include <wx/fs_zip.h>		// Needed for wxZipFSHandler
#include <wx/file.h>		// Needed for wxTempFile
#include <wx/filename.h>

#include "IPFilter.h"		// Interface declarations.
#include "NetworkFunctions.h"
#include "Preferences.h"	// Needed for thePrefs
#include "amule.h"		// Needed for theApp
#include "Statistics.h"		// Needed for theStats
#include "HTTPDownload.h"	// Needed for CHTTPDownloadThread
#include "Logger.h"		// Needed for AddDebugLogLineM
#include "Format.h"		// Needed for CFormat
#include "StringFunctions.h"	// Needed for CSimpleTokenizer


enum EFileType
{
	EFT_Text,
	EFT_Zip,
	EFT_GZip,
	EFT_Unknown
};


/**
 * Returns true if the file is a zip-archive.
 */
EFileType GuessFiletype(const wxString& file)
{
	wxFile archive(file, wxFile::read);
	char head[10];

	if (archive.Read(head, 2) != 2) {
		// Probably just an empty text-file
		return EFT_Text;
	}

	// Attempt to guess the filetype.
	if ((head[0] == 'P') && (head[1] == 'K')) {
		// Zip-archives have a header of "PK".
		return EFT_Zip;
	} else if (head[0] == (char)0x1F && head[1] == (char)0x8B) {
		// Gzip-archives have a header of 0x1F8B
		return EFT_GZip;
	} else {
		// Check the first ten chars, if all are printable, 
		// then we can probably safely assume that this is 
		// a ascii text-file.
		archive.Seek(0, wxFromStart);
		size_t read = archive.Read(head, 10);

		for (size_t i = 0; i < read; ++i) {
			if (!isprint(head[i]) && !isspace(head[i])) {
				return EFT_Unknown;
			}
		}
		
		return EFT_Text;
	}
}


/**
 * Replaces the zip-archive with "guarding.p2p" or "ipfilter.dat",
 * if either of those files are found in the archive.
 */
bool UnpackZipFile(const wxString& file)
{
	wxZipFSHandler archive; 
	wxString filename = archive.FindFirst(file + wxT("#file:/*"), wxFILE);

	while (!filename.IsEmpty()) {
		// Extract the filename part of the URI
		filename = filename.AfterLast(wxT(':')).Lower();
	
		// We only care about the following files
		if (filename == wxT("guarding.p2p") || filename == wxT("ipfilter.dat")) {
			wxZipInputStream inputStream(file, filename);
			
			wxTempFile target(file);
			char buffer[10240];

			while (!inputStream.Eof()) {
				inputStream.Read(buffer, sizeof(buffer));

				target.Write(buffer, inputStream.LastRead());
			}

			// Save the unpacked file
			target.Commit();
			
			return true;
		}
		
		filename = archive.FindNext();
	}

	return false;
}


/**
 * Unpacks a GZip file and replaces the archive.
 */
void UnpackGZipFile(const wxString& file)
{
	wxFileInputStream source(file);
	wxZlibInputStream inputStream(source);
	wxTempFile target(file);
	char buffer[10240];

	while (!inputStream.Eof()) {
		inputStream.Read(buffer, sizeof(buffer));

		target.Write(buffer, inputStream.LastRead());
	}

	// Save the unpacked file
	target.Commit();
}



/**
 * This function creates a text-file containing the specified text, 
 * but only if the file does not already exist.
 */
void CreateDummyFile(const wxString& filename, const wxString& text)
{
	// Create template files
	if (!wxFileExists(filename)) {
		wxTextFile file;

		if (file.Create(filename)) {
			file.AddLine(text);
			file.Write();
		}
	}
}




CIPFilter::CIPFilter()
{
	// Setup dummy files for the curious user.
	
	const wxString normalDat = theApp.ConfigDir + wxT("ipfilter.dat");
	const wxString normalMsg = wxString()
		<< wxT("# This file is used by aMule to store ipfilter lists downloaded\n")
		<< wxT("# through the auto-update functionality. Do not save ipfilter-\n")
		<< wxT("# ranges here that should not be overwritten by aMule.\n");

	CreateDummyFile(normalDat, normalMsg);
	
	
	const wxString staticDat = theApp.ConfigDir + wxT("ipfilter_static.dat");
	const wxString staticMsg = wxString()
		<< wxT("# This file is used to store ipfilter-ranges that should\n")
		<< wxT("# not be overwritten by aMule. If you wish to keep a custom\n")
		<< wxT("# set of ipfilter-ranges that take precedence over ipfilter-\n")
		<< wxT("# ranges aquired through the auto-update functionality, then\n")
		<< wxT("# place them in this file. aMule will not change this file.");

	CreateDummyFile(staticDat, staticMsg);
}


void CIPFilter::Reload()
{
	RemoveAllIPs();

	LoadFromFile(theApp.ConfigDir + wxT("ipfilter.dat"));
	LoadFromFile(theApp.ConfigDir + wxT("ipfilter_static.dat"));
}


uint32 CIPFilter::BanCount() const
{
	wxMutexLocker lock(m_mutex);

	return m_iplist.size();
}


bool CIPFilter::AddIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const wxString& Description)
{
	if (AccessLevel < 256) {
		if (IPStart <= IPEnd) {
			rangeObject item;
			item.AccessLevel = AccessLevel;
			item.Description = Description;

			m_iplist.insert(IPStart, IPEnd, item);

			return true;
		}
	}

	return false;
}


bool CIPFilter::m_inet_atoh(const wxString &str, uint32& ipA, uint32& ipB)
{
	wxString first = str.BeforeFirst(wxT('-'));
	wxString second = str.Mid(first.Len() + 1);

	bool result = StringIPtoUint32(first, ipA) && StringIPtoUint32(second, ipB);

	// StringIPtoUint32 saves the ip in anti-host order, but in order
	// to be able to make relational comparisons, we need to convert
	// it back to host-order.
	ipA = wxUINT32_SWAP_ALWAYS(ipA);
	ipB = wxUINT32_SWAP_ALWAYS(ipB);
	
	return result;
}


bool CIPFilter::ProcessPeerGuardianLine(const wxString& sLine)
{
	CSimpleTokenizer tkz(sLine, wxT(','));
	
	wxString first	= tkz.next();
	wxString second	= tkz.next();
	wxString third  = tkz.remaining().Strip(wxString::both);

	// If there were less than two tokens, fail
	if (tkz.tokenCount() != 2) {
		return false;
	}

	// Convert string IP's to host order IP numbers
	uint32 IPStart = 0;
	uint32 IPEnd   = 0;

	// This will also fail if the line is commented out
	if (!m_inet_atoh(first, IPStart, IPEnd)) {
		return false;
	}

	// Second token is Access Level, default is 0.
	unsigned long AccessLevel = 0;
	if (!second.Strip(wxString::both).ToULong(&AccessLevel) || AccessLevel >= 255) {
		return false;
	}

	// Add the filter
	return AddIPRange(IPStart, IPEnd, AccessLevel, third);
}


bool CIPFilter::ProcessAntiP2PLine(const wxString& sLine)
{
	// remove spaces from the left and right.
	const wxString line = sLine.Strip(wxString::leading);

	// Extract description (first) and IP-range (second) form the line
	int pos = line.Find(wxT(':'), true);
	if (pos == -1) {
		return false;
	}

	wxString Description = line.Left(pos).Strip(wxString::trailing);
	wxString IPRange     = line.Right(line.Len() - pos - 1);

	// Convert string IP's to host order IP numbers
	uint32 IPStart = 0;
	uint32 IPEnd   = 0;

	if (!m_inet_atoh(IPRange ,IPStart, IPEnd)) {
		return false;
	}

	// Add the filter
	return AddIPRange(IPStart, IPEnd, 0, Description);
}


void CIPFilter::LoadFromFile(const wxString& file)
{
	if (wxFileExists(file)) {	
		// Attempt to discover the filetype
		switch (GuessFiletype(file)) {
			case EFT_Text: {
				AddedAndDiscarded stat = LoadFromDatFile(file);

				AddLogLineM(false,
					CFormat(_("Loaded %u IP-ranges from '%s'. %u malformed lines were discarded."))
						% stat.first
						% file.AfterLast(wxFileName::GetPathSeparator())
						% stat.second
				);

				break;
			}
			
			case EFT_Zip: {
				if (UnpackZipFile(file)) {
					AddLogLineM(true, CFormat(_("Extracted ipfilter list from zip-archive '%s'.")) % file);
					LoadFromFile(file);
				} else {
					AddLogLineM(true, CFormat(_("Failed to extract ipfilter list from zip-archive '%s'. File may be damaged or contain misnamed files.")) % file);
				}

				break;
			}

			case EFT_GZip: {
				UnpackGZipFile(file);
				AddLogLineM(true, CFormat(_("Extracted ipfilter list from gz-archive '%s'.")) % file);
				LoadFromFile(file);

				break;
			}

			case EFT_Unknown: {
				AddLogLineM(true, CFormat(_("IPFilter stored in unknown format, file skipped: '%s'.")) % file);				
			}
		}
	}
}


CIPFilter::AddedAndDiscarded CIPFilter::LoadFromDatFile(const wxString& file)
{
	int filtercount = 0;
	int discardedCount = 0;
	
	wxTextFile readFile(file);
	if(readFile.Exists() && readFile.Open()) {
		wxMutexLocker lock(m_mutex);
	
		// Function pointer-type of the parse-functions we can use
		typedef bool (CIPFilter::*ParseFunc)(const wxString&);

		ParseFunc func = NULL;

		for (size_t i = 0; i < readFile.GetLineCount(); i++) {
			wxString line = readFile.GetLine(i);

			if (func && (*this.*func)(line)) {
				filtercount++;
			} else if (ProcessPeerGuardianLine(line)) {
				func = &CIPFilter::ProcessPeerGuardianLine;
				filtercount++;
			} else if (ProcessAntiP2PLine(line)) {
				func = &CIPFilter::ProcessAntiP2PLine;
				filtercount++;
			} else {
				// Comments and empty lines are ignored
				line = line.Strip(wxString::both);
				
				if (!line.IsEmpty() && !line.StartsWith(wxT("#"))) {
					discardedCount++;
					AddDebugLogLineM(false, logIPFilter, wxT("Invalid line found while reading ipfilter file: ") + line);
				}
			}
		}
	}

	return AddedAndDiscarded(filtercount, discardedCount);
}


void CIPFilter::RemoveAllIPs()
{
	wxMutexLocker lock(m_mutex);

	m_iplist.clear();
}


bool CIPFilter::IsFiltered(uint32 IPTest, bool isServer)
{
	if (thePrefs::GetIPFilterOn()) {
		wxMutexLocker lock(m_mutex);

		// The IP needs to be in host order
		IPMap::iterator it = m_iplist.find_range(wxUINT32_SWAP_ALWAYS(IPTest));

		if (it != m_iplist.end()) {
			if (it->AccessLevel < thePrefs::GetIPFilterLevel()) {
				AddDebugLogLineM(false, logIPFilter, wxT("Filtered IP: ") + Uint32toStringIP(IPTest) + wxT(" (") + it->Description + wxT(")"));
				if (isServer) {
					theStats::AddFilteredServer();
				} else {
					theStats::AddFilteredClient();
				}
				return true;
			}
		}
	}

	return false;
}


void CIPFilter::Update(const wxString& strURL)
{
	if (!strURL.IsEmpty()) {
		wxString filename = theApp.ConfigDir + wxT("ipfilter.download");
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread(strURL, filename, HTTP_IPFilter);

		downloader->Create();

		downloader->Run();
	}
}


void CIPFilter::DownloadFinished(uint32 result)
{
	if (result == 1) {
		// download succeeded. proceed with ipfilter loading
		wxString newDat = theApp.ConfigDir + wxT("ipfilter.download");
		wxString oldDat = theApp.ConfigDir + wxT("ipfilter.dat");

		if (wxFileExists(oldDat)) {
			if (!wxRemoveFile(oldDat)) {
				AddDebugLogLineM(true, logIPFilter, wxT("Failed to remove ipfilter.dat file, aborting update."));
				return;
			}
		}

		if (!wxRenameFile(newDat, oldDat)) {
			AddDebugLogLineM(true, logIPFilter, wxT("Failed to rename new ipfilter.dat file, aborting update."));
			return;
		}

		// Reload both ipfilter files
		Reload();
	} else {
		AddDebugLogLineM(true, logIPFilter, wxT("Failed to download the ipfilter from ") + thePrefs::IPFilterURL());
	}
}
