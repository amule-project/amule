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

#include "SearchList.h"		// Interface declarations.

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <tags/ClientTags.h>
#include <tags/FileTags.h>

#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "amule.h"			// Needed for theApp
#include "ServerConnect.h"	// Needed for theApp->serverconnect
#include "Server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for theApp->serverlist
#include "Statistics.h"		// Needed for theStats
#include "ObservableQueue.h"// Needed for CQueueObserver
#include <common/Format.h>
#include "Logger.h"			// Needed for AddLogLineM/...
#include "Packet.h"			// Needed for CPacket
#include "GuiEvents.h"		// Needed for Notify_*


#ifndef AMULE_DAEMON
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "SearchDlg.h"		// Needed for CSearchDlg
#endif

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"

#include "SearchExpr.h"
#include "Scanner.h.in"


extern int yyparse();
extern int yyerror(const char* errstr);
extern int yyerror(wxString errstr);

static wxString s_strCurKadKeyword;

static CSearchExpr _SearchExpr;

wxArrayString _astrParserErrors;


// Helper function for lexer.
void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;

	for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
		const wxString& str = pexpr->m_aExpr[i];
		if (str == SEARCHOPTOK_AND) {
			iOpAnd++;
		} else if (str == SEARCHOPTOK_OR) {
			iOpOr++;
		} else if (str == SEARCHOPTOK_NOT) {
			iOpNot++;
		}
	}

	// this limit (+ the additional operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
	//	+1 Type (Audio, Video)
	//	+1 MinSize
	//	+1 MaxSize
	//	+1 Avail
	//	+1 Extension
	//	+1 Complete sources
	//	+1 Codec
	//	+1 Bitrate
	//	+1 Length
	//	+1 Title
	//	+1 Album
	//	+1 Artist
	// ---------------
	//  12
	if (iOpAnd + iOpOr + iOpNot > 10) {
		yyerror(wxT("Search expression is too complex"));
	}

	_SearchExpr.m_aExpr.Empty();

	// optimize search expression, if no OR nor NOT specified
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0) {
		// figure out if we can use a better keyword than the one the user selected
		// for example most user will search like this "The oxymoronaccelerator 2", which would ask the node which indexes "the"
		// This causes higher traffic for such nodes and makes them a viable target to attackers, while the kad result should be
		// the same or even better if we ask the node which indexes the rare keyword "oxymoronaccelerator", so we try to rearrange
		// keywords and generally assume that the longer keywords are rarer
		if (/*thePrefs::GetRearrangeKadSearchKeywords() &&*/ !s_strCurKadKeyword.IsEmpty()) {
			for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
				if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
					if (pexpr->m_aExpr[i] != s_strCurKadKeyword
						&& pexpr->m_aExpr[i].find_first_of(Kademlia::CSearchManager::GetInvalidKeywordChars()) == wxString::npos
						&& pexpr->m_aExpr[i].Find('"') != 0 // no quoted expressions as keyword
						&& pexpr->m_aExpr[i].length() >= 3
						&& s_strCurKadKeyword.length() < pexpr->m_aExpr[i].length())
					{
						s_strCurKadKeyword = pexpr->m_aExpr[i];
					}
				}
			}
		}
		wxString strAndTerms;
		for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
			if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
				// Minor optimization: Because we added the Kad keyword to the boolean search expression,
				// we remove it here (and only here) again because we know that the entire search expression
				// does only contain (implicit) ANDed strings.
				if (pexpr->m_aExpr[i] != s_strCurKadKeyword) {
					if (!strAndTerms.IsEmpty()) {
						strAndTerms += ' ';
					}
					strAndTerms += pexpr->m_aExpr[i];
				}
			}
		}
		wxASSERT( _SearchExpr.m_aExpr.GetCount() == 0);
		_SearchExpr.m_aExpr.Add(strAndTerms);
	} else {
		if (pexpr->m_aExpr.GetCount() != 1 || pexpr->m_aExpr[0] != s_strCurKadKeyword)
			_SearchExpr.Add(pexpr);
	}
}


//! Helper class for packet creation
class CSearchExprTarget
{
public:
	CSearchExprTarget(CMemFile* pData, EUtf8Str eStrEncode, bool supports64bit, bool& using64bit)
		: m_data(pData),
		  m_eStrEncode(eStrEncode),
		  m_supports64bit(supports64bit),
		  m_using64bit(using64bit)
	{
		m_using64bit = false;
	}

	void WriteBooleanAND()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x00);			// "AND"
	}

	void WriteBooleanOR()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x01);			// "OR"
	}

	void WriteBooleanNOT()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x02);			// "NOT"
	}

	void WriteMetaDataSearchParam(const wxString& rstrValue)
	{
		m_data->WriteUInt8(1);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
	}

	void WriteMetaDataSearchParam(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParamASCII(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, utf8strNone); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

	void WriteMetaDataSearchParam(uint8_t uMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

protected:
	CMemFile* m_data;
	EUtf8Str m_eStrEncode;
	bool m_supports64bit;
	bool& m_using64bit;
};




///////////////////////////////////////////////////////////
// CSearchList

BEGIN_EVENT_TABLE(CSearchList, wxEvtHandler)
	EVT_MULE_TIMER(wxID_ANY, CSearchList::OnGlobalSearchTimer)
END_EVENT_TABLE()


CSearchList::CSearchList()
	: m_searchTimer(this, 0 /* Timer-id doesn't matter. */ ),
	  m_searchType(LocalSearch),
	  m_searchInProgress(false),
	  m_currentSearch(-1),
	  m_searchPacket(NULL),
	  m_64bitSearchPacket(false),
	  m_KadSearchFinished(true)
{}


CSearchList::~CSearchList()
{
	StopSearch();

	while (!m_results.empty()) {
		RemoveResults(m_results.begin()->first);
	}
}


void CSearchList::RemoveResults(long searchID)
{
	// A non-existant search id will just be ignored
	Kademlia::CSearchManager::StopSearch(searchID, true);

	ResultMap::iterator it = m_results.find(searchID);
	if ( it != m_results.end() ) {
		CSearchResultList& list = it->second;

		for (size_t i = 0; i < list.size(); ++i) {
			delete list.at(i);
		}

		m_results.erase( it );
	}
}


wxString CSearchList::StartNewSearch(uint32* searchID, SearchType type, CSearchParams& params)
{
	// Check that we can actually perform the specified desired search.
	if ((type == KadSearch) && !Kademlia::CKademlia::IsRunning()) {
		return _("Kad search can't be done if Kad is not running");
	} else if ((type != KadSearch) && !theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	if (params.typeText != ED2KFTSTR_PROGRAM) {
		if (params.typeText.CmpNoCase(wxT("Any"))) {
			m_resultType = params.typeText;
		} else {
			m_resultType.Clear();
		}
	} else {
		// No check is to be made on returned results if the
		// type is 'Programs', since this returns multiple types.
		m_resultType.Clear();
	}

	if (type == KadSearch) {
		Kademlia::WordList words;
		Kademlia::CSearchManager::GetWords(params.searchString, &words);
		if (!words.empty()) {
			params.strKeyword = words.front();
		} else {
			return _("No keyword for Kad search - aborting");
		}
	}

	bool supports64bit = type == KadSearch ? true : theApp->serverconnect->GetCurrentServer() != NULL && (theApp->serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
	bool packetUsing64bit;

	// This MemFile is automatically free'd
	CMemFilePtr data = CreateSearchData(params, type, supports64bit, packetUsing64bit);

	if (data.get() == NULL) {
		wxASSERT(_astrParserErrors.GetCount());
		wxString error;

		for (unsigned int i = 0; i < _astrParserErrors.GetCount(); ++i) {
			error += _astrParserErrors[i] + wxT("\n");
		}

		return error;
	}

	m_searchType = type;
	if (type == KadSearch) {
		try {
			if (*searchID == 0xffffffff) {
				Kademlia::CSearchManager::StopSearch(0xffffffff, false);
			}

			// searchstring will get tokenized there
			// The tab must be created with the Kad search ID, so searchID is updated.
			Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(params.strKeyword, data->GetLength(), data->GetRawBuffer(), *searchID);

			*searchID = search->GetSearchID();
			m_currentSearch = *searchID;
			m_KadSearchFinished = false;
		} catch (const wxString& what) {
			AddLogLineC(what);
			return _("Unexpected error while attempting Kad search: ") + what;
		}
	} else {
		// This is an ed2k search, local or global
		m_currentSearch = *(searchID);
		m_searchInProgress = true;

		CPacket* searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_SEARCHREQUEST);

		theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
		theApp->serverconnect->SendPacket(searchPacket, (type == LocalSearch));

		if (type == GlobalSearch) {
			delete m_searchPacket;
			m_searchPacket = searchPacket;
			m_64bitSearchPacket = packetUsing64bit;
			m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ); // will be changed later when actually sending the packet!!
		}
	}

	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	if (m_searchType == GlobalSearch) {
		wxCHECK_RET(m_searchPacket, wxT("Global search, but no packet"));

		// Ensure that every global search starts over.
		theApp->serverlist->RemoveObserver(&m_serverQueue);
		m_searchTimer.Start(750);
	} else {
		m_searchInProgress = false;
		Notify_SearchLocalEnd();
	}
}


uint32 CSearchList::GetSearchProgress() const
{
	if (m_searchType == KadSearch) {
		// We cannot measure the progress of Kad searches.
		// But we can tell when they are over.
		return m_KadSearchFinished ? 0xfffe : 0;
	}
	if (m_searchInProgress == false) {	// true only for ED2K search
		// No search, no progress ;)
		return 0;
	}

	switch (m_searchType) {
		case LocalSearch:
			return 0xffff;

		case GlobalSearch:
			return 100 - (m_serverQueue.GetRemaining() * 100)
					/ theApp->serverlist->GetServerCount();

		default:
			wxFAIL;
	}
	return 0;
}


void CSearchList::OnGlobalSearchTimer(CTimerEvent& WXUNUSED(evt))
{
	// Ensure that the server-queue contains the current servers.
	if (m_searchPacket == NULL) {
		// This was a pending event, handled after 'Stop' was pressed.
		return;
	} else if (!m_serverQueue.IsActive()) {
		theApp->serverlist->AddObserver(&m_serverQueue);
	}

	// UDP requests must not be sent to this server.
	const CServer* localServer = theApp->serverconnect->GetCurrentServer();
	if (localServer) {
		uint32 localIP = localServer->GetIP();
		uint16 localPort = localServer->GetPort();
		while (m_serverQueue.GetRemaining()) {
			CServer* server = m_serverQueue.GetNext();

			// Compare against the currently connected server.
			if ((server->GetPort() == localPort) && (server->GetIP() == localIP)) {
				// We've already requested from the local server.
				continue;
			} else {
				if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
					CMemFile data(50);
					uint32_t tagCount = 1;
					data.WriteUInt32(tagCount);
					CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
					flags.WriteNewEd2kTag(&data);
					CPacket *extSearchPacket = new CPacket(OP_GLOBSEARCHREQ3, m_searchPacket->GetPacketSize() + (uint32_t)data.GetLength(), OP_EDONKEYPROT);
					extSearchPacket->CopyToDataBuffer(0, data.GetRawBuffer(), data.GetLength());
					extSearchPacket->CopyToDataBuffer(data.GetLength(), m_searchPacket->GetDataBuffer(), m_searchPacket->GetPacketSize());
					theStats::AddUpOverheadServer(extSearchPacket->GetPacketSize());
					theApp->serverconnect->SendUDPPacket(extSearchPacket, server, true);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ3 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
				} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
					if (!m_64bitSearchPacket || server->SupportsLargeFilesUDP()) {
						m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ2);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ2 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
						theApp->serverconnect->SendUDPPacket(m_searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				} else {
					if (!m_64bitSearchPacket || server->SupportsLargeFilesUDP()) {
						m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
						theApp->serverconnect->SendUDPPacket(m_searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				}
				CoreNotify_Search_Update_Progress(GetSearchProgress());
				return;
			}
		}
	}
	// No more servers left to ask.
	StopSearch(true);
}


void CSearchList::ProcessSharedFileList(const byte* in_packet, uint32 size,
	CUpDownClient* sender, bool *moreResultsAvailable, const wxString& directory)
{
	wxCHECK_RET(sender, wxT("No sender in search-results from client."));

	long searchID = reinterpret_cast<wxUIntPtr>(sender);

#ifndef AMULE_DAEMON
	if (!theApp->amuledlg->m_searchwnd->CheckTabNameExists(sender->GetUserName())) {
		theApp->amuledlg->m_searchwnd->CreateNewTab(sender->GetUserName() + wxT(" (0)"), searchID);
	}
#endif

	const CMemFile packet(in_packet, size);
	uint32 results = packet.ReadUInt32();
	bool unicoded = (sender->GetUnicodeSupport() != utf8strNone);
	for (unsigned int i = 0; i != results; ++i){
		CSearchFile* toadd = new CSearchFile(packet, unicoded, searchID, 0, 0, directory);
		toadd->SetClientID(sender->GetUserIDHybrid());
		toadd->SetClientPort(sender->GetUserPort());
		AddToList(toadd, true);
	}

	if (moreResultsAvailable)
		*moreResultsAvailable = false;

	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1) {
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (moreResultsAvailable) {
				*moreResultsAvailable = (ucMore == 1);
			}
		}
	}
}


void CSearchList::ProcessSearchAnswer(const uint8_t* in_packet, uint32_t size, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	CMemFile packet(in_packet, size);

	uint32_t results = packet.ReadUInt32();
	for (; results > 0; --results) {
		AddToList(new CSearchFile(packet, optUTF8, m_currentSearch, serverIP, serverPort), false);
	}
}


void CSearchList::ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	AddToList(new CSearchFile(packet, optUTF8, m_currentSearch, serverIP, serverPort), false);
}


bool CSearchList::AddToList(CSearchFile* toadd, bool clientResponse)
{
	const uint64 fileSize = toadd->GetFileSize();
	// If filesize is 0, or file is too large for the network, drop it
	if ((fileSize == 0) || (fileSize > MAX_FILE_SIZE)) {
		AddDebugLogLineN(logSearch,
				CFormat(wxT("Dropped result with filesize %u: %s"))
					% fileSize
					% toadd->GetFileName());

		delete toadd;
		return false;
	}

	// If the result was not the type the user wanted, drop it.
	if ((clientResponse == false) && !m_resultType.IsEmpty()) {
		if (GetFileTypeByName(toadd->GetFileName()) != m_resultType) {
			AddDebugLogLineN(logSearch,
				CFormat( wxT("Dropped result type %s != %s, file %s") )
					% GetFileTypeByName(toadd->GetFileName())
					% m_resultType
					% toadd->GetFileName());

			delete toadd;
			return false;
		}
	}


	// Get, or implictly create, the map of results for this search
	CSearchResultList& results = m_results[toadd->GetSearchID()];

	for (size_t i = 0; i < results.size(); ++i) {
		CSearchFile* item = results.at(i);

		if ((toadd->GetFileHash() == item->GetFileHash()) && (toadd->GetFileSize() == item->GetFileSize())) {
			AddDebugLogLineN(logSearch, CFormat(wxT("Received duplicate results for '%s' : %s")) % item->GetFileName() % item->GetFileHash().Encode());
			// Add the child, possibly updating the parents filename.
			item->AddChild(toadd);
			Notify_Search_Update_Sources(item);
			return true;
		}
	}

	AddDebugLogLineN(logSearch,
		CFormat(wxT("Added new result '%s' : %s"))
			% toadd->GetFileName() % toadd->GetFileHash().Encode());

	// New unique result, simply add and display.
	results.push_back(toadd);
	Notify_Search_Add_Result(toadd);

	return true;
}


const CSearchResultList& CSearchList::GetSearchResults(long searchID) const
{
	ResultMap::const_iterator it = m_results.find(searchID);
	if (it != m_results.end()) {
		return it->second;
	}

	// TODO: Should we assert in this case?
	static CSearchResultList list;
	return list;
}


void CSearchList::AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat)
{
	ResultMap::iterator it = m_results.begin();
	for ( ; it != m_results.end(); ++it ) {
		CSearchResultList& list = it->second;

		for ( unsigned int i = 0; i < list.size(); ++i ) {
			if ( list[i]->GetFileHash() == hash ) {
				CoreNotify_Search_Add_Download( list[i], cat );

				return;
			}
		}
	}
}


void CSearchList::StopSearch(bool globalOnly)
{
	if (m_searchType == GlobalSearch) {
		m_currentSearch = -1;
		delete m_searchPacket;
		m_searchPacket = NULL;
		m_searchInProgress = false;

		// Order is crucial here: on wx_MSW an additional event can be generated during the stop.
		// So the packet has to be deleted first, so that OnGlobalSearchTimer() returns immediately
		// without calling StopGlobalSearch() again.
		m_searchTimer.Stop();

		CoreNotify_Search_Update_Progress(0xffff);
	} else if (m_searchType == KadSearch && !globalOnly) {
		Kademlia::CSearchManager::StopSearch(m_currentSearch, false);
		m_currentSearch = -1;
	}
}


CSearchList::CMemFilePtr CSearchList::CreateSearchData(CSearchParams& params, SearchType type, bool supports64bit, bool& packetUsing64bit)
{
	// Count the number of used parameters
	unsigned int parametercount = 0;
	if ( !params.typeText.IsEmpty() )	++parametercount;
	if ( params.minSize > 0 )			++parametercount;
	if ( params.maxSize > 0 )			++parametercount;
	if ( params.availability > 0 )		++parametercount;
	if ( !params.extension.IsEmpty() )	++parametercount;

	wxString typeText = params.typeText;
	if (typeText == ED2KFTSTR_ARCHIVE){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		typeText = ED2KFTSTR_PROGRAM;
	} else if (typeText == ED2KFTSTR_CDIMAGE){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		typeText = ED2KFTSTR_PROGRAM;
	}

	// Must write parametercount - 1 parameter headers
	CMemFilePtr data(new CMemFile(100));

	_astrParserErrors.Empty();
	_SearchExpr.m_aExpr.Empty();

	s_strCurKadKeyword.Clear();
	if (type == KadSearch) {
		wxASSERT( !params.strKeyword.IsEmpty() );
		s_strCurKadKeyword = params.strKeyword;
	}

	LexInit(params.searchString);
	int iParseResult = yyparse();
	LexFree();

	if (_astrParserErrors.GetCount() > 0) {
		for (unsigned int i=0; i < _astrParserErrors.GetCount(); ++i) {
			AddLogLineNS(CFormat(wxT("Error %u: %s\n")) % i % _astrParserErrors[i]);
		}

		return CMemFilePtr(NULL);
	}

	if (iParseResult != 0) {
		_astrParserErrors.Add(CFormat(wxT("Undefined error %i on search expression")) % iParseResult);

		return CMemFilePtr(NULL);
	}

	if (type == KadSearch && s_strCurKadKeyword != params.strKeyword) {
		AddDebugLogLineN(logSearch, CFormat(wxT("Keyword was rearranged, using '%s' instead of '%s'")) % s_strCurKadKeyword % params.strKeyword);
		params.strKeyword = s_strCurKadKeyword;
	}

	parametercount += _SearchExpr.m_aExpr.GetCount();

	/* Leave the unicode comment there, please... */
	CSearchExprTarget target(data.get(), true /*I assume everyone is unicoded */ ? utf8strRaw : utf8strNone, supports64bit, packetUsing64bit);

	unsigned int iParameterCount = 0;
	if (_SearchExpr.m_aExpr.GetCount() <= 1) {
		// lugdunummaster requested that searchs without OR or NOT operators,
		// and hence with no more expressions than the string itself, be sent
		// using a series of ANDed terms, intersecting the ANDs on the terms
		// (but prepending them) instead of putting the boolean tree at the start
		// like other searches. This type of search is supposed to take less load
		// on servers. Go figure.
		//
		// input:      "a" AND min=1 AND max=2
		// instead of: AND AND "a" min=1 max=2
		// we use:     AND "a" AND min=1 max=2

		if (_SearchExpr.m_aExpr.GetCount() > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[0]);
		}

		if (!typeText.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			// Type is always ascii string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, typeText);
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - I keep this here, ready if we ever allow such searches...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, complete);
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}
		#endif // 0

		// If this assert fails... we're seriously fucked up

		wxASSERT( iParameterCount == parametercount );

	} else {
		if (!params.extension.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.availability > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!typeText.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		//#warning TODO - same as above...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minLength > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!album.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!artist.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}
		#endif // 0

		// As above, if this fails, we're seriously fucked up.
		wxASSERT( iParameterCount + _SearchExpr.m_aExpr.GetCount() == parametercount );

		for (unsigned int j = 0; j < _SearchExpr.m_aExpr.GetCount(); ++j) {
			if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_AND) {
				target.WriteBooleanAND();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_OR) {
				target.WriteBooleanOR();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_NOT) {
				target.WriteBooleanNOT();
			} else {
				target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[j]);
			}
		}

		if (!params.typeText.IsEmpty()) {
			// Type is always ASCII string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, params.typeText);
		}

		if (params.minSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0) {
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()) {
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - third and last warning of the same series.
		#if 0
		if (complete > 0) {
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, pParams->uComplete);
		}

		if (minBitrate > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}

		#endif // 0
	}

	// Packet ready to go.
	return data;
}


void CSearchList::KademliaSearchKeyword(uint32_t searchID, const Kademlia::CUInt128 *fileID,
	const wxString& name, uint64_t size, const wxString& type, uint32_t kadPublishInfo, const TagPtrList& taglist)
{
	EUtf8Str eStrEncode = utf8strRaw;

	CMemFile temp(250);
	byte fileid[16];
	fileID->ToByteArray(fileid);
	temp.WriteHash(CMD4Hash(fileid));

	temp.WriteUInt32(0);	// client IP
	temp.WriteUInt16(0);	// client port

	// write tag list
	unsigned int uFilePosTagCount = temp.GetPosition();
	uint32 tagcount = 0;
	temp.WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTagString tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	CTagInt64 tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	if (!type.IsEmpty()) {
		CTagString tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(&temp, eStrEncode);
		tagcount++;
	}

	// Misc tags (bitrate, etc)
	for (TagPtrList::const_iterator it = taglist.begin(); it != taglist.end(); ++it) {
		(*it)->WriteTagToFile(&temp,eStrEncode);
		tagcount++;
	}

	temp.Seek(uFilePosTagCount, wxFromStart);
	temp.WriteUInt32(tagcount);

	temp.Seek(0, wxFromStart);

	CSearchFile *tempFile = new CSearchFile(temp, (eStrEncode == utf8strRaw), searchID, 0, 0, wxEmptyString, true);
	tempFile->SetKadPublishInfo(kadPublishInfo);

	AddToList(tempFile);
}

void CSearchList::UpdateSearchFileByHash(const CMD4Hash& hash)
{
	for (ResultMap::iterator it = m_results.begin(); it != m_results.end(); ++it) {
		CSearchResultList& results = it->second;
		for (size_t i = 0; i < results.size(); ++i) {
			CSearchFile* item = results.at(i);

			if (hash == item->GetFileHash()) {
				// This covers only parent items,
				// child items have to be updated separately.
				Notify_Search_Update_Sources(item);
			}
		}
	}
}

// File_checked_for_headers
