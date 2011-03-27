//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <wx/stdpaths.h>		// Needed for GetDataDir
#include <wx/ffile.h>

#include "IPFilter.h"			// Interface declarations.
#include "IPFilterScanner.h"	// Interface for flexer
#include "Preferences.h"		// Needed for thePrefs
#include "amule.h"			// Needed for theApp
#include "Statistics.h"			// Needed for theStats
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "Logger.h"			// Needed for AddDebugLogLine{C,N}
#include <common/Format.h>		// Needed for CFormat
#include <common/StringFunctions.h>	// Needed for CSimpleTokenizer
#include <common/FileFunctions.h>	// Needed for UnpackArchive
#include <common/TextFile.h>		// Needed for CTextFile
#include "ThreadScheduler.h"		// Needed for CThreadScheduler and CThreadTask
#include "ClientList.h"			// Needed for CClientList
#include "ServerList.h"			// Needed for CServerList
#include <common/Macros.h>		// Needed for DEBUG_ONLY()
#include "RangeMap.h"			// Needed for CRangeMap
#include "ServerConnect.h"		// Needed for ConnectToAnyServer()
#include "DownloadQueue.h"		// Needed for theApp->downloadqueue


////////////////////////////////////////////////////////////
// CIPFilterEvent

BEGIN_DECLARE_EVENT_TYPES()
	DECLARE_EVENT_TYPE(MULE_EVT_IPFILTER_LOADED, -1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(MULE_EVT_IPFILTER_LOADED)

	
class CIPFilterEvent : public wxEvent
{
public:
	CIPFilterEvent(CIPFilter::RangeIPs rangeIPs, CIPFilter::RangeLengths rangeLengths, CIPFilter::RangeNames rangeNames) 
		: wxEvent(-1, MULE_EVT_IPFILTER_LOADED)
	{
		// Physically copy the vectors, this will hopefully resize them back to their needed capacity.
		m_rangeIPs = rangeIPs;
		m_rangeLengths = rangeLengths;
		// This one is usually empty, and should always be swapped, not copied.
		std::swap(m_rangeNames, rangeNames);
	}
	
	/** @see wxEvent::Clone */
	virtual wxEvent* Clone() const {
		return new CIPFilterEvent(*this);
	}
	
	CIPFilter::RangeIPs m_rangeIPs;
	CIPFilter::RangeLengths m_rangeLengths;
	CIPFilter::RangeNames m_rangeNames;
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
		  m_storeDescriptions(false),
		  m_owner(owner)
	{
	}

private:
	void Entry()
	{
		AddLogLineN(_("Loading IP filters 'ipfilter.dat' and 'ipfilter_static.dat'."));
		if ( !LoadFromFile(theApp->ConfigDir + wxT("ipfilter.dat")) &&
		     thePrefs::UseIPFilterSystem() ) {
			// Load from system wide IP filter file
			wxStandardPathsBase &spb(wxStandardPaths::Get());
#ifdef __WXMSW__
			wxString dataDir(spb.GetPluginsDir());
#elif defined(__WXMAC__)
			wxString dataDir(spb.GetDataDir());
#else
			wxString dataDir(spb.GetDataDir().BeforeLast(wxT('/')) + wxT("/amule"));
#endif
			wxString systemwideFile(JoinPaths(dataDir,wxT("ipfilter.dat")));
			LoadFromFile(systemwideFile);
		}


		LoadFromFile(theApp->ConfigDir + wxT("ipfilter_static.dat"));

		uint8 accessLevel = thePrefs::GetIPFilterLevel();
		uint32 size = m_result.size();
		// Reserve a little more so we don't have to resize the vector later.
		// (Map ranges can exist that have to be stored in several parts.)
		// Extra memory will be freed in the end.
		m_rangeIPs.reserve(size + 1000);
		m_rangeLengths.reserve(size + 1000);
		if (m_storeDescriptions) {
			m_rangeNames.reserve(size + 1000);
		}
		for (IPMap::iterator it = m_result.begin(); it != m_result.end(); ++it) {
			if (it->AccessLevel < accessLevel) {
				// Calculate range "length"
				// (which is included-end - start and thus length - 1)
				// Encoding:
				// 0      - 0x7fff	same
				// 0x8000 - 0xffff	0xfff	- 0x07ffffff
				// that means: remove msb, shift left by 12 bit, add 0xfff
				// so it can cover 8 consecutive class A nets
				// larger ranges (or theoretical ranges with uneven ends) have to be split
				uint32 startIP = it.keyStart();
				uint32 realLength = it.keyEnd() - it.keyStart() + 1;
				std::string * descp = 0;
				while (realLength) {
					m_rangeIPs.push_back(startIP);
					uint32 curLength = realLength;
					uint16 pushLength;
					if (realLength <= 0x8000) {
						pushLength = realLength - 1;
					} else {
						if (curLength >= 0x08000000) {
							// range to big, limit
							curLength = 0x08000000;
						} else {
							// cut off LSBs
							curLength &= 0x07FFF000;
						}
						pushLength = ((curLength - 1) >> 12) | 0x8000;
					}
					m_rangeLengths.push_back(pushLength);
#ifdef __DEBUG__
					if (m_storeDescriptions) {
						// std::string has no ref counting, so swap it
						// (it's used so we need half the space than wxString with wide chars)
						if (descp) {
							// we split the range so we have to duplicate it
							m_rangeNames.push_back(*descp);
						} else {
							// push back empty string and swap
							m_rangeNames.push_back(std::string());
							descp = & * m_rangeNames.rbegin();
							std::swap(*descp, it->Description);
						}
					}
#endif
					realLength -= curLength;
					startIP += curLength;
				}
			}
		}
		// Numbers are probably different:
		// - ranges from map that are not blocked because of their level are not added to the table
		// - some ranges from the map have to be split for the table
		AddDebugLogLineN(logIPFilter, CFormat(wxT("Ranges in map: %d  blocked ranges in table: %d")) % size % m_rangeIPs.size());

		CIPFilterEvent evt(m_rangeIPs, m_rangeLengths, m_rangeNames);
		wxPostEvent(m_owner, evt);
	}

	/**
	 * This structure is used to contain the range-data in the rangemap.
	 */
	struct rangeObject
	{
		bool operator==( const rangeObject& other ) const {
			return AccessLevel == other.AccessLevel;
		}

// Since descriptions are only used for debugging messages, there 
// is no need to keep them in memory when running a non-debug build.
#ifdef __DEBUG__
		//! Contains the user-description of the range.
		std::string	Description;
#endif
		
		//! The AccessLevel for this filter.
		uint8		AccessLevel;
	};

	//! The is the type of map used to store the IPs.
	typedef CRangeMap<rangeObject, uint32> IPMap;
	
	bool m_storeDescriptions;

	// the generated filter
	CIPFilter::RangeIPs m_rangeIPs;
	CIPFilter::RangeLengths m_rangeLengths;
	CIPFilter::RangeNames m_rangeNames;

	wxEvtHandler*		m_owner;
	// temporary map for filter generation
	IPMap				m_result;

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
	bool AddIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const char* DEBUG_ONLY(Description))
	{
		if (AccessLevel < 256) {
			if (IPStart <= IPEnd) {
				rangeObject item;
				item.AccessLevel = AccessLevel;
#ifdef __DEBUG__
				if (m_storeDescriptions) {
					item.Description = Description;
				}
#endif

				m_result.insert(IPStart, IPEnd, item);

				return true;
			}
		}

		return false;
	}


	/**
	 * Loads a IP-list from the specified file, can be text or zip.
	 *
	 * @return True if the file was loaded, false otherwise.
	 **/
	int LoadFromFile(const wxString& file)
	{
		const CPath path = CPath(file);

		if (!path.FileExists() || TestDestroy()) {
			return 0;
		}

#ifdef __DEBUG__
		m_storeDescriptions = theLogger.IsEnabled(logIPFilter);
#endif

		const wxChar* ipfilter_files[] = {
			wxT("ipfilter.dat"),
			wxT("guardian.p2p"),
			wxT("guarding.p2p"),
			NULL
		};
		
		// Try to unpack the file, might be an archive

		if (UnpackArchive(path, ipfilter_files).second != EFT_Text) {
			AddLogLineC(CFormat(_("Failed to load ipfilter.dat file '%s', unknown format encountered.")) % file);
			return 0;
		}
		
		int filtercount = 0;
		yyip_Bad = 0;
		wxFFile readFile;
		if (readFile.Open(path.GetRaw())) {
			yyip_Line = 1;
			yyiprestart(readFile.fp());
			uint32 IPStart = 0;
			uint32 IPEnd   = 0;
			uint32 IPAccessLevel = 0;
			char * IPDescription;
			uint32 time1 = GetTickCountFullRes();
			while (yyiplex(IPStart, IPEnd, IPAccessLevel, IPDescription)) {
				AddIPRange(IPStart, IPEnd, IPAccessLevel, IPDescription);
				filtercount++;
			}
			uint32 time2 = GetTickCountFullRes();
			AddDebugLogLineN(logIPFilter, CFormat(wxT("time for lexer: %.3f")) % ((time2-time1) / 1000.0));
		} else {
			AddLogLineC(CFormat(_("Failed to load ipfilter.dat file '%s', could not open file.")) % file);
			return 0;
		}

		wxString msg = CFormat(wxPLURAL("Loaded %u IP-range from '%s'.", "Loaded %u IP-ranges from '%s'.", filtercount)) % filtercount % file;
		if (yyip_Bad) {
			msg << wxT(" ") << ( CFormat(wxPLURAL("%u malformed line was discarded.", "%u malformed lines were discarded.", yyip_Bad)) % yyip_Bad );
		}
		AddLogLineN(msg);

		return filtercount;
	}
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
static bool CreateDummyFile(const wxString& filename, const wxString& text)
{
	// Create template files
	if (!wxFileExists(filename)) {
		CTextFile file;

		if (file.Open(filename, CTextFile::write)) {
			file.WriteLine(text);
			return true;
		}
	}
	return false;
}


CIPFilter::CIPFilter() :
	m_ready(false),
	m_startKADWhenReady(false),
	m_connectToAnyServerWhenReady(false)
{
	// Setup dummy files for the curious user.
	const wxString normalDat = theApp->ConfigDir + wxT("ipfilter.dat");
	const wxString normalMsg = wxString()
		<< wxT("# This file is used by aMule to store ipfilter lists downloaded\n")
		<< wxT("# through the auto-update functionality. Do not save ipfilter-\n")
		<< wxT("# ranges here that should not be overwritten by aMule.\n");

	if (CreateDummyFile(normalDat, normalMsg)) {
		// redownload if user deleted file
		thePrefs::SetLastHTTPDownloadURL(HTTP_IPFilter, wxEmptyString);
	}
	
	const wxString staticDat = theApp->ConfigDir + wxT("ipfilter_static.dat");
	const wxString staticMsg = wxString()
		<< wxT("# This file is used to store ipfilter-ranges that should\n")
		<< wxT("# not be overwritten by aMule. If you wish to keep a custom\n")
		<< wxT("# set of ipfilter-ranges that take precedence over ipfilter-\n")
		<< wxT("# ranges aquired through the auto-update functionality, then\n")
		<< wxT("# place them in this file. aMule will not change this file.");

	CreateDummyFile(staticDat, staticMsg);

	// First load currently available filter, so network connect is possible right after
	// (in case filter download takes some time).
	Reload();
	// Check if update should be done only after that.
	m_updateAfterLoading = thePrefs::IPFilterAutoLoad() && !thePrefs::IPFilterURL().IsEmpty();
}


void CIPFilter::Reload()
{
	// We keep the current filter till the new one has been loaded.
	CThreadScheduler::AddTask(new CIPFilterTask(this));
}


uint32 CIPFilter::BanCount() const
{
	wxMutexLocker lock(m_mutex);

	return m_rangeIPs.size();
}


bool CIPFilter::IsFiltered(uint32 IPTest, bool isServer)
{
	if ((!thePrefs::IsFilteringClients() && !isServer) || (!thePrefs::IsFilteringServers() && isServer)) {
		return false;
	}
	if (!m_ready) {
		// Somebody connected before we even started the networks.
		// Filter is not up yet, so block him.
		AddDebugLogLineN(logIPFilter, CFormat(wxT("Filtered IP %s because filter isn't ready yet.")) % Uint32toStringIP(IPTest));
		if (isServer) {
			theStats::AddFilteredServer();
		} else {
			theStats::AddFilteredClient();
		}
		return true;
	}
	wxMutexLocker lock(m_mutex);
	// The IP needs to be in host order
	uint32 ip = wxUINT32_SWAP_ALWAYS(IPTest);
	int imin = 0;
	int imax = m_rangeIPs.size() - 1;
	int i;
	bool found = false;
	while (imin <= imax) {
		i = (imin + imax) / 2;
		uint32 curIP = m_rangeIPs[i];
		if (curIP <= ip) {
			uint32 curLength = m_rangeLengths[i];
			if (curLength >= 0x8000) {
				curLength = ((curLength & 0x7fff) << 12) + 0xfff;
			}
			if (curIP + curLength >= ip) {
				found = true;
				break;
			}
		}
		if (curIP > ip) {
			imax = i - 1;
		} else {
			imin = i + 1;
		}
	}
	if (found) {
		AddDebugLogLineN(logIPFilter, CFormat(wxT("Filtered IP %s%s")) % Uint32toStringIP(IPTest)
			% (i < (int)m_rangeNames.size() ? (wxT(" (") + wxString(char2unicode(m_rangeNames[i].c_str())) + wxT(")")) 
											: wxString(wxEmptyString)));
		if (isServer) {
			theStats::AddFilteredServer();
		} else {
			theStats::AddFilteredClient();
		}
		return true;
	}
	return false;
}


void CIPFilter::Update(const wxString& strURL)
{
	if (!strURL.IsEmpty()) {
		m_URL = strURL;

		wxString filename = theApp->ConfigDir + wxT("ipfilter.download");
		wxString oldfilename = theApp->ConfigDir + wxT("ipfilter.dat");
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread(m_URL, filename, oldfilename, HTTP_IPFilter, true, true);

		downloader->Create();
		downloader->Run();
	}
}


void CIPFilter::DownloadFinished(uint32 result)
{
	wxString datName = wxT("ipfilter.dat");
	if (result == HTTP_Success) {
		// download succeeded. proceed with ipfilter loading
		wxString newDat = theApp->ConfigDir + wxT("ipfilter.download");
		wxString oldDat = theApp->ConfigDir + datName;

		if (wxFileExists(oldDat) && !wxRemoveFile(oldDat)) {
			AddLogLineC(CFormat(_("Failed to remove %s file, aborting update.")) % datName);
			result = HTTP_Error;
		} else if (!wxRenameFile(newDat, oldDat)) {
			AddLogLineC(CFormat(_("Failed to rename new %s file, aborting update.")) % datName);
			result = HTTP_Error;
		} else {
			AddLogLineN(CFormat(_("Successfully updated %s")) % datName);
		}
	} else if (result == HTTP_Skipped) {
		AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % datName);
	} else {
		AddLogLineC(CFormat(_("Failed to download %s from %s")) % datName % m_URL);
	}

	if (result == HTTP_Success) {
		// Reload both ipfilter files on success
		Reload();
	}
}


void CIPFilter::OnIPFilterEvent(CIPFilterEvent& evt)
{
	{
		wxMutexLocker lock(m_mutex);
		std::swap(m_rangeIPs, evt.m_rangeIPs);
		std::swap(m_rangeLengths, evt.m_rangeLengths);
		std::swap(m_rangeNames, evt.m_rangeNames);
		m_ready = true;
	}
	if (theApp->IsOnShutDown()) {
		return;
	}
	AddLogLineN(_("IP filter is ready"));
	
	if (thePrefs::IsFilteringClients()) {
		theApp->clientlist->FilterQueues();
	}
	if (thePrefs::IsFilteringServers()) {
		theApp->serverlist->FilterServers();
	}
	// Now start networks we didn't start earlier
	if (m_connectToAnyServerWhenReady || m_startKADWhenReady) {
		AddLogLineC(_("Connecting"));
	}
	if (m_connectToAnyServerWhenReady) {
		m_connectToAnyServerWhenReady = false;
		theApp->serverconnect->ConnectToAnyServer();
	}
	if (m_startKADWhenReady) {
		m_startKADWhenReady = false;
		theApp->StartKad();
	}
	theApp->ShowConnectionState(true);			// update connect button
	if (thePrefs::GetSrcSeedsOn()) {
		theApp->downloadqueue->LoadSourceSeeds();
	}
	// Trigger filter update if configured
	if (m_updateAfterLoading) {
		m_updateAfterLoading = false;
		Update(thePrefs::IPFilterURL());
	}
}

// File_checked_for_headers
