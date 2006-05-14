//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/intl.h>		// Needed for _()
#include <wx/filefn.h>		// Needed for wxFileExists
#include <wx/textfile.h>	// Needed for wxTextFile
#include <wx/string.h>		// Needed for wxString
#include <wx/wfstream.h>	// wxFileInputStream
#include <wx/file.h>		// Needed for wxTempFile
#include <wx/filename.h>

#include "IPFilter.h"			// Interface declarations.
#include "NetworkFunctions.h"	// Needed for StringIPtoUint32
#include "Preferences.h"		// Needed for thePrefs
#include "amule.h"				// Needed for theApp
#include "Statistics.h"			// Needed for theStats
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "Logger.h"				// Needed for AddDebugLogLineM
#include <common/Format.h>				// Needed for CFormat
#include <common/StringFunctions.h>	// Needed for CSimpleTokenizer
#include "FileFunctions.h"		// Needed for UnpackArchive
#include "ThreadScheduler.h"	// Needed for CThreadScheduler and CThreadTask
#include "ClientList.h"			// Needed for CClientList
#include "ServerList.h"			// Needed for CServerList


////////////////////////////////////////////////////////////
// CIPFilterEvent

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(MULE_EVT_IPFILTER_LOADED, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(MULE_EVT_IPFILTER_LOADED)

	
class CIPFilterEvent : public wxEvent
{
public:
	CIPFilterEvent(CIPFilter::IPMap& result) 
		: wxEvent(-1, MULE_EVT_IPFILTER_LOADED)
	{
		// Avoid needles copying
		std::swap(result, m_result);
	}
	
	/** @see wxEvent::Clone */
	virtual wxEvent* Clone() const {
		return new CIPFilterEvent(*this);
	}
	
	CIPFilter::IPMap m_result;
};


typedef void (wxEvtHandler::*MuleIPFilterEventFunction)(CIPFilterEvent&);

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_IPFILTER_LOADED(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_IPFILTER_LOADED, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleIPFilterEventFunction, &func), (wxObject*) NULL),


////////////////////////////////////////////////////////////
// Thread task for loading the ipfilter.dat files.

/**
 * This task loads the two ipfilter.dat files, a task that
 * can take quite a while on a slow system with a large dat-
 * file.
 */
class CIPFilterTask : public CThreadTask
{
public:
	CIPFilterTask(wxEvtHandler* owner)
		: CThreadTask(wxT("Load IPFilter"), wxEmptyString, ETP_Critical),
		  m_owner(owner)
	{
	}
	
private:
	void Entry() {
		AddLogLineM(false, _("Loading IP-filters 'ipfilter.dat' and 'ipfilter_static.dat'."));
		LoadFromFile(theApp.ConfigDir + wxT("ipfilter.dat"));		
		LoadFromFile(theApp.ConfigDir + wxT("ipfilter_static.dat"));

		CIPFilterEvent evt(m_result);
		wxPostEvent(m_owner, evt);
	}

	
	/**
	 * Helper function.
	 *
	 * @param IPstart The start of the IP-range.
	 * @param IPend The end of the IP-range, must be less than or equal to IPstart.
	 * @param AccessLevel The AccessLevel of this range.
	 * @param Description The assosiated description of this range.
	 * @return true if the range was added, false if it was discarded.
	 * 
	 * This function inserts the specified range into the IPMap. Invalid
	 * ranges where the AccessLevel is not within the range 0..255, or
	 * where IPEnd < IPstart not inserted.
	 */	
	bool AddIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const wxString& Description)
	{
		if (AccessLevel < 256) {
			if (IPStart <= IPEnd) {
				CIPFilter::rangeObject item;
				item.AccessLevel = AccessLevel;
#if __DEBUG__
				item.Description = Description;
#endif

				m_result.insert(IPStart, IPEnd, item);

				return true;
			}
		}

		return false;
	}


	/**
	 * Helper function.
	 * 
	 * @param str A string representation of an IP-range in the format "<ip>-<ip>".
	 * @param ipA The target of the first IP in the range.
	 * @param ipB The target of the second IP in the range.
	 * @return True if the parsing succeded, false otherwise (results will be invalid).
	 *
	 * The IPs returned by this function are in host order, not network order.
	 */
	bool m_inet_atoh(const wxString &str, uint32& ipA, uint32& ipB)
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


	/**
	 * Helper-function for processing the PeerGuardian format.
	 *
	 * @return True if the line was valid, false otherwise.
	 * 
	 * This function will correctly parse files that follow the folllowing
	 * format for specifying IP-ranges (whitespace is optional):
	 *  <IPStart> - <IPEnd> , <AccessLevel> , <Description>
	 */
	bool ProcessPeerGuardianLine(const wxString& sLine)
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


	/**
	 * Helper-function for processing the AntiP2P format.
	 *
	 * @return True if the line was valid, false otherwise.
	 * 
	 * This function will correctly parse files that follow the folllowing
	 * format for specifying IP-ranges (whitespace is optional):
	 *  <Description> : <IPStart> - <IPEnd>
	 */
	bool ProcessAntiP2PLine(const wxString& sLine)
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

	
	/**
	 * Loads a IP-list from the specified file, can be text or zip.
	 *
	 * @return True if the file was loaded, false otherwise.
	 **/
	void LoadFromFile(const wxString& file)
	{
		if (!wxFileExists(file) or TestDestroy()) {	
			return;
		}

		const wxChar* ipfilter_files[] = {
			wxT("ipfilter.dat"),
			wxT("guarding.p2p"),
			NULL
		};
		
		// Try to unpack the file, might be an archive
		if (UnpackArchive(file, ipfilter_files).second != EFT_Text) {
			AddLogLineM(true, CFormat(_("Failed to load ipfilter.dat file '%s', unknown format encountered.")) % file);
			return;
		}
		
		int filtercount = 0;
		int discardedCount = 0;
		
		wxTextFile readFile(file);
		if (readFile.Open()) {
			// Function pointer-type of the parse-functions we can use
			typedef bool (CIPFilterTask::*ParseFunc)(const wxString&);

			ParseFunc func = NULL;

			for (size_t i = 0; i < readFile.GetLineCount(); i++) {
				wxString line = readFile.GetLine(i);

				if (TestDestroy()) {
					return;
				} else if (func && (*this.*func)(line)) {
					filtercount++;
				} else if (ProcessPeerGuardianLine(line)) {
					func = &CIPFilterTask::ProcessPeerGuardianLine;
					filtercount++;
				} else if (ProcessAntiP2PLine(line)) {
					func = &CIPFilterTask::ProcessAntiP2PLine;
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
		} else {
			AddLogLineM(true, CFormat(_("Failed to load ipfilter.dat file '%s', could not open file.")) % file);
			return;
		}

		AddLogLineM(false,
			CFormat(_("Loaded %u IP-ranges from '%s'. %u malformed lines were discarded."))
			% filtercount
			% wxFileName(file).GetFullName()
			% discardedCount
		);

		return;
	}
	
private:
	wxEvtHandler*		m_owner;	
	CIPFilter::IPMap	m_result;
};


////////////////////////////////////////////////////////////
// CIPFilter


BEGIN_EVENT_TABLE(CIPFilter, wxEvtHandler)
	EVT_MULE_IPFILTER_LOADED(CIPFilter::OnIPFilterEvent)
END_EVENT_TABLE()



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

	Reload();
}


void CIPFilter::Reload()
{
	// We keep the current filter till the new one has been loaded.
	CThreadScheduler::AddTask(new CIPFilterTask(this));
}


uint32 CIPFilter::BanCount() const
{
	wxMutexLocker lock(m_mutex);

	return m_iplist.size();
}


bool CIPFilter::IsFiltered(uint32 IPTest, bool isServer)
{
	if (thePrefs::GetIPFilterOn()) {
		wxMutexLocker lock(m_mutex);

		// The IP needs to be in host order
		IPMap::iterator it = m_iplist.find_range(wxUINT32_SWAP_ALWAYS(IPTest));

		if (it != m_iplist.end()) {
			if (it->AccessLevel < thePrefs::GetIPFilterLevel()) {
#if __DEBUG__
				AddDebugLogLineM(false, logIPFilter, wxString(wxT("Filtered IP (AccLvl: ")) << (long)it->AccessLevel << wxT("): ")
						<< Uint32toStringIP(IPTest) << wxT(" (") << it->Description + wxT(")"));
#endif
				
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


void CIPFilter::OnIPFilterEvent(CIPFilterEvent& evt)
{
	{
		wxMutexLocker lock(m_mutex);
		std::swap(m_iplist, evt.m_result);
	}
	
	if (thePrefs::GetIPFilterOn()) {
		theApp.clientlist->FilterQueues();
		theApp.serverlist->FilterServers();
	}
}

