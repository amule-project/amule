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

#include "SearchList.h"		// Interface declarations.
#include "OtherFunctions.h"	// Needed for GetFiletypeByName
#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "amule.h"			// Needed for theApp
#include "ServerSocket.h"	// Needed for theApp.serverconnect
#include "Server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for theApp.serverlist
#include "Statistics.h"		// Needed for theStats
#include "ObservableQueue.h"// Needed for CQueueObserver
#include <common/Format.h>	
#include "Logger.h"			// Needed for AddLogLineM/...
#include "Preferences.h"	// Needed for thePrefs
#include "Packet.h"			// Needed for CPacket
#include "kademlia/utils/UInt128.h" // Needed for CUInt128
#include "SearchFile.h"		// Needed for CSearchFile

#include <algorithm>
#include <memory>

#ifndef AMULE_DAEMON
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "SearchDlg.h"		// Needed for CSearchDlg
#endif

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/Search.h"

#include "SearchExpr.h"
#include "Scanner.h.in"
#include "Scanner.h"


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

	for (unsigned int i = 0; i < pexpr->m_aExpr.Count(); i++) {
		wxString str(pexpr->m_aExpr[i]);
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
		wxString strAndTerms;
		for (unsigned int i = 0; i < pexpr->m_aExpr.Count(); i++) {
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
		wxASSERT( _SearchExpr.m_aExpr.Count() == 0);
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
	CSearchExprTarget(CMemFile* pData, EUtf8Str eStrEncode)
	{
		m_data = pData;
		m_eStrEncode = eStrEncode;
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

	void WriteMetaDataSearchParam(uint8 uMetaTagID, uint8 uOperator, uint32 uValue, bool WXUNUSED(bEd2k))
	{
		m_data->WriteUInt8(3);				// numeric parameter type
		m_data->WriteUInt32(uValue);		// numeric value
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, uint8 uOperator, uint32 uValue, bool WXUNUSED(bEd2k))
	{
		m_data->WriteUInt8(3);				// numeric parameter type
		m_data->WriteUInt32(uValue);		// numeric value
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

	void WriteOldMinMetaDataSearchParam(uint8 uMetaTagID, uint32 uValue, bool bEd2k)
	{
		uint8 uOperator;
		if (bEd2k){
			uOperator = ED2K_SEARCH_OP_GREATER;
			uValue -= 1;
		} else {
			uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
		}
		WriteMetaDataSearchParam(uMetaTagID, uOperator, uValue, bEd2k);
	}

	void WriteOldMinMetaDataSearchParam(const wxString& pszMetaTagID, uint32 uValue, bool bEd2k)
	{
		uint8 uOperator;
		if (bEd2k){
			uOperator = ED2K_SEARCH_OP_GREATER;
			uValue -= 1;
		} else {
			uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
		}
		WriteMetaDataSearchParam(pszMetaTagID, uOperator, uValue, bEd2k);
	}

	void WriteOldMaxMetaDataSearchParam(const wxString& pszMetaTagID, uint32 uValue, bool bEd2k)
	{
		uint8 uOperator;
		if (bEd2k){
			uOperator = ED2K_SEARCH_OP_LESS;
			uValue += 1;
		} else {
			uOperator = KAD_SEARCH_OP_LESS_EQUAL;
		}
		WriteMetaDataSearchParam(pszMetaTagID, uOperator, uValue, bEd2k);
	}

	void WriteOldMaxMetaDataSearchParam(uint8 uMetaTagID, uint32 uValue, bool bEd2k)
	{
		uint8 uOperator;
		if (bEd2k){
			uOperator = ED2K_SEARCH_OP_LESS;
			uValue += 1;
		} else {
			uOperator = KAD_SEARCH_OP_LESS_EQUAL;
		}
		WriteMetaDataSearchParam(uMetaTagID, uOperator, uValue, bEd2k);
	}

protected:
	CMemFile* m_data;
	EUtf8Str m_eStrEncode;
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
	  m_searchPacket(NULL)
{
}


CSearchList::~CSearchList()
{
	StopGlobalSearch();

	while (not m_results.empty()) {
		RemoveResults(m_results.begin()->first);
	}
}


void CSearchList::RemoveResults(long searchID)
{
	// A non-existant search id will just be ignored
	Kademlia::CSearchManager::stopSearch(searchID, true);
	
	ResultMap::iterator it = m_results.find(searchID);
	if ( it != m_results.end() ) {
		CSearchResultList& list = it->second;
	
		for (size_t i = 0; i < list.size(); ++i) {
			delete list.at(i);
		}
	
		m_results.erase( it );
	}
}


wxString CSearchList::StartNewSearch(uint32* searchID, SearchType type, const CSearchParams& params)
{
	// Check that we can actually perform the specified desired search.
	if ((type == KadSearch) and not Kademlia::CKademlia::isRunning()) {
		return _("Kad search can't be done if Kad is not running");
	} else if ((type != KadSearch) and not theApp.IsConnectedED2K()) {
		return _("ED2K search can't be done if ED2K is not connected");
	}
	
	if (params.typeText != ED2KFTSTR_PROGRAM) {
		// No check is to be made on returned results if the 
		// type is 'Programs', since this returns multiple types.
		m_resultType = params.typeText;
	}

	// This MemFile is automatically free'd, except for kad searches.	
	CMemFilePtr data = CreateSearchData(params, type);
	
	if (data.get() == NULL) {
		wxASSERT(_astrParserErrors.Count());
		wxString error;
		
		for (unsigned int i = 0; i < _astrParserErrors.Count(); ++i) {
			error += _astrParserErrors[i] + wxT("\n");
		}
		
		return error;
	}
	
	m_searchType = type;
	if (type == KadSearch) {
		try {
			if (*searchID == 0xffffffff) {
				Kademlia::CSearchManager::stopSearch(0xffffffff, false);
			}
		
			// Kad search takes ownership of data and searchstring will get tokenized there
			// The tab must be created with the Kad search ID, so seardhID is updated.
			Kademlia::CSearch* search = Kademlia::CSearchManager::prepareFindKeywords(
										 params.searchString, data.release(), *searchID);

			*searchID = search->getSearchID();
		} catch (const wxString& what) {
			AddLogLineM(true, what);
			return _("Unexpected error while attempting Kad search: ") + what;				
		}
	} else {
		// This is an ed2k search, local or global
		m_currentSearch = *(searchID);
		m_searchInProgress = true;
	
		CPacket* searchPacket = new CPacket(data.get());
		searchPacket->SetOpCode(OP_SEARCHREQUEST);
		
		theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
		theApp.serverconnect->SendPacket(searchPacket, (type == LocalSearch));

		if (type == GlobalSearch) {
			m_searchPacket = searchPacket;
			
			// The OPCode must be changed since global searches are UDP requests
			m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
		}
	}

	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	if (m_searchType == GlobalSearch) {
		wxCHECK_RET(m_searchPacket, wxT("Global search, but no packet"));
		
		// Ensure that every global search starts over.	
		theApp.serverlist->RemoveObserver(&m_serverQueue);
		m_searchTimer.Start(750);
 	} else {
		m_searchInProgress = false;
		Notify_SearchLocalEnd();
	}
}


uint32 CSearchList::GetSearchProgress() const
{
	if (m_searchInProgress == false) {
		// No search, no progress ;)
		return 0;
	}
	
	switch (m_searchType) {
		case LocalSearch:
			return 0xffff;

		case GlobalSearch:
			return 100 - (m_serverQueue.GetRemaining() * 100) 
					/ theApp.serverlist->GetServerCount();

		case KadSearch:
			// We cannot meassure the progress of Kad searches.
			return 0;
		
		default:
			wxCHECK(false, 0);
	}
}


void CSearchList::OnGlobalSearchTimer(CTimerEvent& WXUNUSED(evt))
{
	// Ensure that the server-queue contains the current servers.
	if (not m_serverQueue.IsActive()) {
		theApp.serverlist->AddObserver(&m_serverQueue);		
	}

	// UDP requests must not be sent to this server.
	const CServer* localServer = theApp.serverconnect->GetCurrentServer();
	uint32 localIP = localServer->GetIP();
	uint16 localPort = localServer->GetPort();


	while (m_serverQueue.GetRemaining()) {
		CServer* server = m_serverQueue.GetNext();

		// Compare against the currently connected server.
		if ((server->GetPort() == localPort) and (server->GetIP() == localIP)) {
			// We've already requested from the local server.
			continue;
		} else {
			theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
			theApp.serverconnect->SendUDPPacket(m_searchPacket, server, false);
			
			CoreNotify_Search_Update_Progress(GetSearchProgress());					
			
			return;
		}
	}
	
	// No more servers left to ask.
	StopGlobalSearch();
}


void CSearchList::ProcessSharedFileList(const char *in_packet, uint32 size, 
	CUpDownClient* sender, bool *moreResultsAvailable, const wxString& directory)
{
	wxCHECK_RET(sender, wxT("No sender in search-results from client."));
	
	long searchID = (long)sender;

#ifndef AMULE_DAEMON
	if (!theApp.amuledlg->searchwnd->CheckTabNameExists(sender->GetUserName())) {
		theApp.amuledlg->searchwnd->CreateNewTab(sender->GetUserName() + wxT(" (0)"), searchID);
	}
#endif

	const CMemFile packet((byte*)in_packet, size);
	uint32 results = packet.ReadUInt32();
	bool unicoded = sender->GetUnicodeSupport();
	for (unsigned int i = 0; i != results; ++i){			
		CSearchFile* toadd = new CSearchFile(packet, unicoded, searchID, 0, 0, directory);
		if (sender){
			toadd->SetClientID(sender->GetUserIDHybrid());
			toadd->SetClientPort(sender->GetUserPort());
		}

		AddToList(toadd, true);	
	}

	if (moreResultsAvailable)
		*moreResultsAvailable = false;
	
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1) {
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (moreResultsAvailable) {
				*moreResultsAvailable = (bool)ucMore;
			}
		}
	}
}


void CSearchList::ProcessSearchAnswer(const char* in_packet, uint32 size, bool optUTF8, uint32 WXUNUSED(serverIP), uint16 WXUNUSED(serverPort))
{
	CMemFile packet((byte*)in_packet,size);

	uint32 results = packet.ReadUInt32();
	for (unsigned int i = 0; i != results; ++i) {
		AddToList(new CSearchFile(packet, optUTF8, m_currentSearch));
	}
}


void CSearchList::ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32 serverIP, uint16 serverPort)
{
	AddToList(new CSearchFile(packet, optUTF8, m_currentSearch, serverIP, serverPort));
}


bool CSearchList::AddToList(CSearchFile* toadd, bool clientResponse)
{
	const uint32 fileSize = toadd->GetFileSize();
	// If filesize is 0, or file is too large for the network, drop it 
	if ((fileSize == 0) or (fileSize > MAX_FILE_SIZE)) {
		AddDebugLogLineM(false, logSearch,
				CFormat(wxT("Dropped result with filesize %u: %s"))
					% fileSize
					% toadd->GetFileName());
		
		delete toadd;
		return false;
	}
	
	// If the result was not the type the user wanted, drop it.
	if ((clientResponse == false) and not m_resultType.IsEmpty()) {
		if (GetFileTypeByName(toadd->GetFileName()) != m_resultType) {
			AddDebugLogLineM( false, logSearch,
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
		
		if ((toadd->GetFileHash() == item->GetFileHash())
			and (toadd->GetFileSize() == item->GetFileSize())) {
			
			AddDebugLogLineM(false, logSearch, 
				CFormat(wxT("Received duplicate results for '%s' : %s"))
					% item->GetFileName() % item->GetFileHash().Encode());
				
			// If no children exists, then we add the current item. The
			// "parent" item will show the most common filename and the 
			// sum of sources for all variants.
			if (item->GetChildren().empty()) {
				if (toadd->GetFileName() == item->GetFileName()) {
					AddDebugLogLineM( false, logSearch, 
						CFormat(wxT("Merged results for '%s'")) 
							% item->GetFileName());
					
					// Merge duplicate items rather than creating a child item
					item->AddSources(toadd->GetSourceCount(), toadd->GetCompleteSourceCount());
					Notify_Search_Update_Sources(item);
					delete toadd;
					return true;
				} else {
					AddDebugLogLineM(false, logSearch, 
						CFormat(wxT("Created initial child for result '%s'")) 
							% item->GetFileName());
				
					// The first child will always be the first result we received.
					item->AddChild(new CSearchFile(*item));
				}
			}

			AddDebugLogLineM( false, logSearch,
				CFormat(wxT("Adding child '%s' to result '%s'"))
					% toadd->GetFileName() % item->GetFileName());
			
			// Parent item includes sum of all sources for this file
			item->AddSources(toadd->GetSourceCount(), toadd->GetCompleteSourceCount());
			// Add the child, possibly updating the parents filename.
			item->AddChild(toadd);			
			
			Notify_Search_Update_Sources(item);
			
			return true;
		}
	}

	AddDebugLogLineM(false, logSearch,
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


void CSearchList::StopGlobalSearch()
{
	m_searchTimer.Stop();

	m_currentSearch = -1;
	delete m_searchPacket;
	m_searchPacket = NULL;
	m_searchInProgress = false;
	
	CoreNotify_Search_Update_Progress(0xffff);
}


CSearchList::CMemFilePtr CSearchList::CreateSearchData(const CSearchParams& params, SearchType type)
{
	const bool kad = (type == KadSearch);
	
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

	if (type == KadSearch) {
		// We need to make some room for the keyword hash
		data->WriteUInt128(CUInt128());
		// and the search type (0/1 if there is ed2k data or not)		
		// There will obviously be... at least the search string.
		data->WriteUInt8(0);
	}

	_astrParserErrors.Empty();	
	_SearchExpr.m_aExpr.Empty();
	
    LexInit(params.searchString);
    int iParseResult = yyparse();
    LexFree();
	
	#ifdef __DEBUG__
	printf("Search parsing resultfor \"%s\": %i\n",(const char*)unicode2UTF8(params.searchString),iParseResult);
	#endif
	if (_astrParserErrors.Count() > 0) {
		for (unsigned int i=0; i < _astrParserErrors.Count(); ++i) {
			printf("Error %u: %s\n",i,(const char*)unicode2UTF8(_astrParserErrors[i]));
		}
		
		return CMemFilePtr(NULL);
	}

	if (iParseResult != 0) {
		_astrParserErrors.Add(wxString::Format(wxT("Undefined error %i on search expression"),iParseResult));
	
		return CMemFilePtr(NULL);
	}
	
	#ifdef __DEBUG__
	printf("Search expression: ");
	for (unsigned int i = 0; i < _SearchExpr.m_aExpr.Count(); i++){
		printf("%s ",(const char*)unicode2char(_SearchExpr.m_aExpr[i]));
	}
	printf("\nExpression count: %i\n",(int)_SearchExpr.m_aExpr.GetCount());
	#endif

	parametercount += _SearchExpr.m_aExpr.GetCount();
	
	#ifdef __DEBUG__
	printf("Parameters: %i\n",parametercount);
	#endif
	
	/* Leave the unicode comment there, please... */
	CSearchExprTarget target(data.get(), true /*I assume everyone is unicoded */ ? utf8strRaw : utf8strNone);

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
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, params.minSize, !kad);
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, params.maxSize, !kad);
		}
		
		if (params.availability > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, params.availability, !kad);
		}

		if (!params.extension.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		#warning TODO - I keep this here, ready if we ever allow such searches...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(FT_COMPLETE_SOURCES, complete, !kad);
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(kad ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, minBitrate, !kad);
		}

		if (minLength > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(kad ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, minLength, !kad);
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
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
        
		#warning TODO - same as above...
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
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, params.minSize, !kad);
		}

		if (params.maxSize > 0) {
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, params.maxSize, !kad);
		}

		if (params.availability > 0) {
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, params.availability, !kad);
		}

		if (!params.extension.IsEmpty()) {
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		#warning TODO - third and last warning of the same series.
		#if 0
		if (complete > 0) {
			target.WriteOldMinMetaDataSearchParam(FT_COMPLETE_SOURCES, pParams->uComplete, !kad);
		}

		if (minBitrate > 0) {
			target.WriteOldMinMetaDataSearchParam(kad ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, minBitrate, !kad);
		}
		
		if (minLength > 0) {
			target.WriteOldMinMetaDataSearchParam(kad ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, minLength, bEd2k);
		}
		
		if (!codec.IsEmpty()) {
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}
		
		if (!title.IsEmpty()) {
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}
		
		if (!album.IsEmpty()) {
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}
		
		if (!artist.IsEmpty()) {
			target.WriteMetaDataSearchParam(kad ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}
		
		#endif // 0
	}
	
	// Packet ready to go.
	return data;
}


void CSearchList::KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* fileID, 
										const wxString&  name, uint32 size, const wxString& type, const TagPtrList& taglist)
{
	EUtf8Str eStrEncode = (wxUSE_UNICODE ? utf8strRaw : utf8strNone);

	CMemFile temp(250);
	byte fileid[16];
	fileID->toByteArray(fileid);
	temp.WriteHash(CMD4Hash(fileid));
	
	temp.WriteUInt32(0);	// client IP
	temp.WriteUInt16(0);	// client port
	
	// write tag list
	unsigned int uFilePosTagCount = temp.GetPosition();
	uint32 tagcount = 0;
	temp.WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTag tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	CTag tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	if (!type.IsEmpty()) {
		CTag tagType(FT_FILETYPE, type);
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
	
	AddToList(new CSearchFile(temp, (eStrEncode == utf8strRaw), searchID, 0, 0, wxEmptyString, true));
}

