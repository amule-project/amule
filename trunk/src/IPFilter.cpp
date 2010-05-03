//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "IPFilter.h"			// Interface declarations.
#include "Preferences.h"		// Needed for thePrefs
#include "amule.h"			// Needed for theApp
#include "Statistics.h"			// Needed for theStats
#include "HTTPDownload.h"		// Needed for CHTTPDownloadThread
#include "Logger.h"			// Needed for AddDebugLogLineM
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
	CIPFilterTask(wxEvtHandler* owner, bool block = false)
		: CThreadTask(wxT("Load IPFilter"), wxEmptyString, ETP_Critical),
		  m_storeDescriptions(false),
		  m_owner(owner)
	{
		if (block) {
			LockBlockDuringUpdate();
		}
	}

	void LockBlockDuringUpdate()	{ m_blockDuringUpdate.Lock(); }
	void UnlockBlockDuringUpdate()	{ m_blockDuringUpdate.Unlock(); }
	
private:
	void Entry()
	{
		// Block thread while there is still an update going on
		LockBlockDuringUpdate();
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
						m_rangeNames.push_back(it->Description);
					}
#endif
					realLength -= curLength;
					if (realLength) {
						AddDebugLogLineN(logIPFilter, CFormat(wxT("Split range %s - %s %04X")) 
							% KadIPToString(startIP) % KadIPToString(it.keyEnd()) % realLength );
					}
					startIP += curLength;
				}
			}
		}
		AddDebugLogLineN(logIPFilter, CFormat(wxT("Ranges in map: %d  blocked ranges %d")) % size % m_rangeIPs.size());

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
		wxString	Description;
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
	// Mutex to block thread during active update
	wxMutex				m_blockDuringUpdate;

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
	bool AddIPRange(uint32 IPStart, uint32 IPEnd, uint16 AccessLevel, const wxString& DEBUG_ONLY(Description))
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
	 * Helper function.
	 * 
	 * @param str A string representation of an IP-range in the format "<ip>-<ip>".
	 * @param ipA The target of the first IP in the range.
	 * @param ipB The target of the second IP in the range.
	 * @return True if the parsing succeded, false otherwise (results will be invalid).
	 *
	 * The IPs returned by this function are in host order, not network order.
	 */
	bool ReadIPRange(const wxString &str, uint32& ipA, uint32& ipB)
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
		if (!ReadIPRange(first, IPStart, IPEnd)) {
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

		if (!ReadIPRange(IPRange ,IPStart, IPEnd)) {
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
			AddLogLineM(true, 
				CFormat(_("Failed to load ipfilter.dat file '%s', unknown format encountered.")) % file);
			return 0;
		}
		
		int filtercount = 0;
		int discardedCount = 0;
		
		CTextFile readFile;
		if (readFile.Open(path, CTextFile::read)) {
			// Function pointer-type of the parse-functions we can use
			typedef bool (CIPFilterTask::*ParseFunc)(const wxString&);

			ParseFunc func = NULL;

			while (!readFile.Eof()) {
				wxString line = readFile.GetNextLine();

				if (TestDestroy()) {
					return 0;
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
			return 0;
		}

		AddLogLineM(false,
			( CFormat(wxPLURAL("Loaded %u IP-range from '%s'.", "Loaded %u IP-ranges from '%s'.", filtercount)) % filtercount % file )
			+ wxT(" ") +
			( CFormat(wxPLURAL("%u malformed line was discarded.", "%u malformed lines were discarded.", discardedCount)) % discardedCount )
		);

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
	m_connectToAnyServerWhenReady(false),
	m_ipFilterTask(NULL)
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

	if (thePrefs::IPFilterAutoLoad() && !thePrefs::IPFilterURL().IsEmpty()) {
		//
		// We want to update the filter on startup.
		// If we create a CThreadTask now it will be the first to be processed.
		// But if we start a download thread first other thread tasks started meanwhile
		// will be processed first, and can take a long time (like AICH hashing).
		// During this time filter won't be active, and thus networks won't be started.
		//
		// So start a new task now and block it until update is finished.
		// Then unblock it.
		//
		m_ipFilterTask = new CIPFilterTask(this, true);
		CThreadScheduler::AddTask(m_ipFilterTask);
		Update(thePrefs::IPFilterURL());
	} else {
		Reload();
	}
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
			% (i < (int)m_rangeNames.size() ? (wxT(" (") + m_rangeNames[i] + wxT(")")) : wxString(wxEmptyString)));
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
		CHTTPDownloadThread *downloader = new CHTTPDownloadThread(m_URL, filename, oldfilename, HTTP_IPFilter);

		downloader->Create();
		downloader->Run();
	}
}


void CIPFilter::DownloadFinished(uint32 result)
{
	if (result == HTTP_Success) {
		// download succeeded. proceed with ipfilter loading
		wxString newDat = theApp->ConfigDir + wxT("ipfilter.download");
		wxString oldDat = theApp->ConfigDir + wxT("ipfilter.dat");

		if (wxFileExists(oldDat) && !wxRemoveFile(oldDat)) {
			AddLogLineC(CFormat(_("Failed to remove %s file, aborting update.")) % wxT("ipfilter.dat"));
			result = HTTP_Error;
		} else if (!wxRenameFile(newDat, oldDat)) {
			AddLogLineC(CFormat(_("Failed to rename new %s file, aborting update.")) % wxT("ipfilter.dat"));
			result = HTTP_Error;
		} else {
			AddLogLineN(CFormat(_("Successfully updated %s")) % wxT("ipfilter.dat"));
		}
	} else if (result == HTTP_Skipped) {
		AddLogLineN(CFormat(_("Skipped download of %s, because requested file is not newer.")) % wxT("ipfilter.dat"));
	} else {
		AddLogLineC(CFormat(_("Failed to download %s from %s")) % wxT("ipfilter.dat") % m_URL);
	}

	if (m_ipFilterTask) {
	// If we updated during startup, there is already a task waiting to load the filter.
	// Trigger it to continue.
		m_ipFilterTask->UnlockBlockDuringUpdate();
		m_ipFilterTask = NULL;
	// reload on success, or if filter is not up yet (shouldn't happen)
	} else if (result == HTTP_Success || !m_ready) {
		// Reload both ipfilter files
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
	if (thePrefs::GetSrcSeedsOn()) {
		theApp->downloadqueue->LoadSourceSeeds();
	}
}

// File_checked_for_headers
