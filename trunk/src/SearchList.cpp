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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "SearchList.h"		// Interface declarations.
#include "OtherFunctions.h"	// Needed for GetFiletypeByName
#include "NetworkFunctions.h" // Needed for IsGoodIP
#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "amule.h"			// Needed for theApp
#include "ServerSocket.h"
#include "Server.h"
#include "ServerList.h"
#include "SharedFileList.h" // Needed for GetFileByID
#include "DownloadQueue.h"  // Needed for GetFileByID
#include "Statistics.h"		// Needed for theStats
#include "ObservableQueue.h"// Needed for CQueueObserver
#include <common/Format.h>
#include "Logger.h"
#include "Preferences.h"
#include "Packet.h"			// Neeed for CPacket
#include "kademlia/utils/UInt128.h" // Needed for CUInt128

#include <algorithm>

#ifndef AMULE_DAEMON
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for ID_NOTEBOOK
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

// Helper class for packet creation

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


// Search classes

CGlobalSearchThread::CGlobalSearchThread( CPacket* packet )
	: wxThread(wxTHREAD_DETACHED)
{
	m_packet = packet;
	
	CServer* current = theApp.serverconnect->GetCurrentServer();
	current = theApp.serverlist->GetServerByIP( current->GetIP(), current->GetPort() );
	
	m_packet->SetOpCode(OP_GLOBSEARCHREQ);

	m_progress = 0;
}


CGlobalSearchThread::~CGlobalSearchThread()
{
	// Delete the packet whose ownerwhip we got on constructor.
	delete m_packet;
}


void *CGlobalSearchThread::Entry()
{
	CServer* current = theApp.serverconnect->GetCurrentServer();
	// If we are not lucky, GetCurrentServer() may return NULL
	if (current) {
		current = theApp.serverlist->GetServerByIP(
			current->GetIP(), current->GetPort() );
	} else {
		return NULL;
	}

	// Create the a queue of servers
	CQueueObserver<CServer*> queue;

	// Initialize it with the current servers
	theApp.serverlist->AddObserver( &queue );

	while ( !TestDestroy() ) {
		if ( !queue.GetRemaining() ) {
			break;
		} else {
			CServer* server = queue.GetNext();

			if ( server != current ) {
	
				theStats::AddUpOverheadServer( m_packet->GetPacketSize() );
				theApp.serverconnect->SendUDPPacket( m_packet, server, false );
		
				m_progress = 100 - ( queue.GetRemaining() * 100 ) / theApp.serverlist->GetServerCount();
				CoreNotify_Search_Update_Progress( m_progress );
			}	
		}
		
		Sleep(750);
	}
	
	// Global search ended, reset progress and controls
	CoreNotify_Search_Update_Progress(0xffff);
	
	// When shutting down, the searchlist may have been deleted
	// by the time the sleep() call returns, so a check is needed.
	if (theApp.searchlist) {
		theApp.searchlist->ClearThreadData(this);
	}
		
	return NULL;
}

CSearchFile::CSearchFile(const CMemFile& in_data, bool bOptUTF8, long nSearchID, uint32 WXUNUSED(nServerIP), uint16 WXUNUSED(nServerPort), const wxString& pszDirectory, bool nKademlia)
{
	m_nSearchID = nSearchID;
	m_nKademlia = nKademlia;
	
	m_abyFileHash = in_data.ReadHash();
	m_nClientID = in_data.ReadUInt32();
	m_nClientPort = in_data.ReadUInt16();
	
	if (( m_nClientID || m_nClientPort ) && ( !IsGoodIP(m_nClientID, thePrefs::FilterLanIPs()) || !m_nClientPort ) ) {
		m_nClientID = 0;
		m_nClientPort = 0;
	}
	
	uint32 tagcount = in_data.ReadUInt32();

	for (unsigned int i = 0; i != tagcount; ++i){
		CTag* toadd = new CTag(in_data,bOptUTF8 );
		m_taglist.push_back(toadd);
	}

	// here we have two choices
	//	- if the server/client sent us a filetype, we could use it (though it could be wrong)
	//	- we always trust our filetype list and determine the filetype by the extension of the file
	wxString tempName = GetStrTagValue(FT_FILENAME);
	
	if (tempName.IsEmpty()) {
		throw CInvalidPacket(wxT("No filename in search result"));
	}
		
	SetFileName(tempName);
	SetFileSize(GetIntTagValue(FT_FILESIZE));

	m_iUserRating = (GetIntTagValue(FT_FILERATING) & 0xF) / 3;
	m_Directory = pszDirectory;
}


CSearchFile::~CSearchFile()
{	
	for ( unsigned int i = 0; i < m_taglist.size(); ++i )
		delete m_taglist[i];
	
	m_taglist.clear();
}


uint32 CSearchFile::GetIntTagValue(uint8 tagname) const
{
	for (unsigned int i = 0; i != m_taglist.size(); ++i) {
		if ( m_taglist[i]->GetNameID() == tagname && m_taglist[i]->IsInt() )
			return m_taglist[i]->GetInt();
	}
	
	return 0;
}


wxString CSearchFile::GetStrTagValue(uint8 tagname) const
{
	for (unsigned int i = 0; i != m_taglist.size(); ++i) {
		if ( m_taglist[i]->GetNameID() == tagname && m_taglist[i]->IsStr() )
			return m_taglist[i]->GetStr();
	}
	
	return wxEmptyString;
}


void CSearchFile::AddSources(uint32 count, uint32 count_complete)
{
	for ( unsigned int i = 0; i < m_taglist.size(); ++i ) {
		CTag* tag = m_taglist[i];
	
		switch ( tag->GetNameID() ) {
			case FT_SOURCES:
				if (m_nKademlia) {
					if (count > tag->GetInt()) {
						tag->SetInt(count);
					}
				} else {
					tag->SetInt(tag->GetInt() + count);
				}
				break;
				
			case FT_COMPLETE_SOURCES:
				if (m_nKademlia) {
					if (count > tag->GetInt()) {
						tag->SetInt(count_complete);
					}
				} else { 
					tag->SetInt(tag->GetInt() + count_complete);
				}
				break;
		}
	}
}


uint32 CSearchFile::GetSourceCount() const
{
	return GetIntTagValue(FT_SOURCES);
}


uint32 CSearchFile::GetCompleteSourceCount() const
{
	return GetIntTagValue(FT_COMPLETE_SOURCES);
}

uint32 CSearchFile::GetFileSize() const 
{
	return GetIntTagValue(FT_FILESIZE);
}

int CSearchFile::IsComplete() const {
	return IsComplete(GetSourceCount(), GetIntTagValue(FT_COMPLETE_SOURCES));
}

int CSearchFile::IsComplete(uint32 uSources, uint32 uCompleteSources) const {
	if (IsKademlia()) {
		return -1;		// unknown
	} else if (uSources > 0 && uCompleteSources > 0) {
		return 1;		// complete
	} else {
		return 0;		// not complete
	}
}

//
// CSearchList
//

CSearchList::CSearchList()
{
	m_CurrentSearch = 0;
	m_searchpacket = NULL;
	m_searchthread = NULL;
	m_SearchInProgress = false;
}


CSearchList::~CSearchList()
{
	StopGlobalSearch();
	delete m_searchpacket;
	Clear();
}


void CSearchList::Clear()
{
	ResultMap::iterator it = m_Results.begin();
	
	for ( ; it != m_Results.end(); ++it ) {
		SearchList& list = it->second;
	
		for ( unsigned int i = 0; i < list.size(); ++i ) 
			delete list[i];
	}
		
	m_Results.clear();
}


void CSearchList::RemoveResults(long nSearchID)
{
	// A non-existant search id will just be ignored
	Kademlia::CSearchManager::stopSearch(nSearchID,true);
	
	ResultMap::iterator it = m_Results.find( nSearchID );

	if ( it != m_Results.end() ) {
		SearchList& list = it->second;
	
		for ( unsigned int i = 0; i < list.size(); ++i ) 
			delete list[i];
	
		m_Results.erase( it );
	}
}


wxString CSearchList::StartNewSearch(uint32* nSearchID, SearchType search_type, const wxString& searchString, const wxString& typeText, 
											const wxString& extension, uint32 min, uint32 max, uint32 availability)
{
	
	if(!theApp.IsConnected()) {
		// Failed!
		return _("aMule is not connected!");
	}
	
	m_resultType = typeText;
	m_CurrentSearch = *(nSearchID); // This will be set for ed2k results

	CMemFile* search_data = CreateSearchData(searchString, typeText, extension, min, max, availability, (search_type == KadSearch));
	
	if (!search_data) {
		wxASSERT(_astrParserErrors.Count());
		wxString error;
		for (unsigned int i = 0; i < _astrParserErrors.Count(); ++i) {
			error += _astrParserErrors[i] + wxT("\n");
		}
		return error;
	}
	
	if (search_type == KadSearch) {
		if (Kademlia::CKademlia::isRunning()) {
			try {
				if ( *nSearchID == 0xffffffff ) {
					Kademlia::CSearchManager::stopSearch(0xffffffff, false);
				}
				// Kad search takes ownership of data and searchstring will get tokenized there
				Kademlia::CSearch* search = Kademlia::CSearchManager::prepareFindKeywords(searchString,
					search_data, *nSearchID);
				*(nSearchID) = search->getSearchID(); // The tab must be created with the Kad search id
			} catch(wxString& what) {
				AddLogLineM(true,what);
				delete search_data;
				return _("Unexpected error while attempting Kad search: ") + what;				
			}
			return wxEmptyString;
		} else {
			delete search_data;
			return _("Kad search can't be done if Kad is not running");
		}
	}
	
	// This is ed2k search...
	
	if(!theApp.IsConnectedED2K()) {
		// Failed!
		delete search_data;
		return _("ED2K search can't be done if ED2K is not connected");
	}
	
	// Packet takes ownership of data
	CPacket* searchpacket = new CPacket(search_data);
	delete search_data;
	searchpacket->SetOpCode(OP_SEARCHREQUEST);
	

	theStats::AddUpOverheadServer(searchpacket->GetPacketSize());
	// Send packet. If it's not a global search, delete it after sending.
	theApp.serverconnect->SendPacket(searchpacket, (search_type == LocalSearch) ); 
	
	wxASSERT(m_searchthread == NULL);
	
	if (m_searchthread) {
		m_searchthread->Delete();
	}
	
	ClearThreadData();
	
	if ( search_type == GlobalSearch ) {
		m_searchpacket = searchpacket;
	}
	
	m_SearchInProgress = true;
	
	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	wxThreadError result;
	
	if ( m_searchpacket ) {
		m_searchthread = new CGlobalSearchThread(m_searchpacket);
		if ( (result =  m_searchthread->Create()) == wxTHREAD_NO_ERROR) {
			m_searchthread->Run();
		} else {
			printf("THREAD CREATION ERROR FOR GLOBAL SEARCH: ");
			switch (result) {
				case wxTHREAD_NO_RESOURCE:
					printf("NOT ENOUGH RESOURCES!\n");
					break;
				case wxTHREAD_RUNNING:
					printf("ALREADY RUNNING!\n");
					break;
				default:
					break;
			}
			// Free resources.
			delete m_searchpacket;			
		}
		// Thread takes ownership
		m_searchpacket = NULL;
	}
	if (!IsGlobalSearch()) {
		m_SearchInProgress = false;
		// On global search, we must not reset the GUI controls.
		Notify_SearchLocalEnd();
	}
}


void CSearchList::ProcessSearchanswer(const char *in_packet, uint32 size, 
	CUpDownClient *Sender, bool *pbMoreResultsAvailable, const wxString& pszDirectory)
{
	wxASSERT( Sender != NULL );
	
	long nSearchID = (long)Sender;

#ifndef AMULE_DAEMON
	if (!theApp.amuledlg->searchwnd->CheckTabNameExists(Sender->GetUserName())) {
		theApp.amuledlg->searchwnd->CreateNewTab(Sender->GetUserName() + wxT(" (0)"),nSearchID);
	}
#endif

	const CMemFile packet((byte*)in_packet, size);
	uint32 results = packet.ReadUInt32();
	bool unicoded = (Sender && Sender->GetUnicodeSupport());
	for (unsigned int i = 0; i != results; ++i){			
		CSearchFile* toadd = new CSearchFile(packet, unicoded, nSearchID,  0, 0, pszDirectory);
		if (Sender){
			toadd->SetClientID(Sender->GetUserIDHybrid());
			toadd->SetClientPort(Sender->GetUserPort());
		}
		AddToList(toadd, true);
	}

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1) {
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
		}
	}
}


void CSearchList::ProcessSearchanswer(const char* in_packet, uint32 size, bool bOptUTF8, uint32 WXUNUSED(nServerIP), uint16 WXUNUSED(nServerPort))
{
	CMemFile packet((byte*)in_packet,size);

	uint32 results = packet.ReadUInt32();

	for (unsigned int i = 0; i != results; ++i) {
		CSearchFile* toadd = new CSearchFile(packet, bOptUTF8, m_CurrentSearch);
		AddToList(toadd, false);
	}
	
}


void CSearchList::ProcessUDPSearchanswer(const CMemFile& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort)
{
	CSearchFile* toadd = new CSearchFile(packet, bOptUTF8, m_CurrentSearch, nServerIP, nServerPort);
	AddToList(toadd);
}


bool CSearchList::AddToList(CSearchFile* toadd, bool bClientResponse)
{
	uint32 fileSize = toadd->GetIntTagValue(FT_FILESIZE);
	// If filesize is 0, or file is too large for the network, drop it 
	if ((fileSize == 0) or (fileSize > MAX_FILE_SIZE)) {
		delete toadd;
		return false;
	}
	
	// If the result was not the type user wanted, drop it.
	if (	!bClientResponse &&
		!(m_resultType.IsEmpty() ||
		GetFiletypeByName(toadd->GetFileName(), false) == m_resultType)) {
		AddDebugLogLineM( false, logSearch,
			CFormat( wxT("Dropped result type %s != %s, file %s") )
				% GetFiletypeByName(toadd->GetFileName(),false)
				% m_resultType
				% toadd->GetFileName() 
		);
		delete toadd;
		return false;
	}



	ResultMap::iterator it = m_Results.find( toadd->GetSearchID() );

	if ( it != m_Results.end() ) {
		SearchList& list = it->second;
	
		for ( unsigned int i = 0; i < list.size(); ++i ) {
			if ( toadd->GetFileHash() == list[i]->GetFileHash() ) {
				list[i]->AddSources( toadd->GetSourceCount(), toadd->GetCompleteSourceCount() );
				
				Notify_Search_Update_Sources( list[i] );
				
				delete toadd;
				
				return true;
			}
		}
	}

		
	m_Results[ toadd->GetSearchID() ].push_back( toadd );
	
	Notify_Search_Add_Result(toadd);
	
		
	return true;
}



class CmpFiles
{
public:
	CmpFiles( int sortBy = 0, bool ascending = true )
	{
		m_type = sortBy;
		m_asc = ascending;
	}

	bool operator()( CSearchFile* file1, CSearchFile* file2 )
	{
		int mod = ( m_asc ? 1 : -1 );
		int result = 0;
	
		switch ( m_type ) {
			case 0: result = file1->GetFileName().CmpNoCase( file2->GetFileName() ); break;				
			case 1: result = CmpAny( file1->GetFileSize(), file2->GetFileSize() ); break;
			case 2: result = file1->GetFileHash().Encode().Cmp( file2->GetFileHash().Encode() ); break;
			case 3: result = CmpAny( file1->GetSourceCount(), file2->GetSourceCount() ); break;
		}

		return (result * mod) < 0;
	}

private:
	bool	m_asc;
	int		m_type;
};

const std::vector<CSearchFile*> CSearchList::GetSearchResults(long nSearchID)
{
	ResultMap::const_iterator it = m_Results.find(nSearchID);

	if ( it != m_Results.end() ) {
		return it->second;
	}
	return std::vector<CSearchFile*>();
}

void CSearchList::AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat)
{
	ResultMap::iterator it = m_Results.begin();
	for ( ; it != m_Results.end(); ++it ) {
		SearchList& list = it->second;
	
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
 	if (IsGlobalSearch()) {
		m_searchthread->Delete();
		ClearThreadData();
 	} else {
		// Maybe this was a global search that didn't
		// get a reply from local server yet.
		if (m_searchpacket) {
			delete m_searchpacket;
			m_searchpacket = NULL;
		}
	}
}


CMemFile* CSearchList::CreateSearchData(const wxString& searchString, const wxString& typeText,
				const wxString& extension, uint32 min, uint32 max, uint32 availability, bool kad)
{
	// Count the number of used parameters
	unsigned int parametercount = 0;
	if ( !typeText.IsEmpty() )	++parametercount;
	if ( min > 0 )			++parametercount;
	if ( max > 0 ) 			++parametercount;
	if ( availability > 0 )		++parametercount;
	if ( !extension.IsEmpty() )	++parametercount;
	
	// Must write parametercount - 1 parameter headers
	CMemFile* data =  new CMemFile(100);

	if (kad) {
		// We need to make some room for the keyword hash
		data->WriteUInt128(CUInt128());
		// and the search type (0/1 if there is ed2k data or not)		
		// There will obviously be... at least the search string.
		data->WriteUInt8(0);
	}

	_astrParserErrors.Empty();	
	_SearchExpr.m_aExpr.Empty();
	
    LexInit(searchString);
    int iParseResult = yyparse();
    LexFree();
	
	#ifdef __DEBUG__
	printf("Search parsing resultfor \"%s\": %i\n",(const char*)unicode2char(searchString),iParseResult);
	#endif
	if (_astrParserErrors.Count() > 0) {
		for (unsigned int i=0; i < _astrParserErrors.Count(); ++i) {
			printf("Error %u: %s\n",i,(const char*)unicode2char(_astrParserErrors[i]));
		}
		delete data;
		return NULL;
	}

	if (iParseResult != 0) {
		_astrParserErrors.Add(wxString::Format(wxT("Undefined error %i on search expression"),iParseResult));
		delete data;
		return NULL;
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
	CSearchExprTarget target(data, true /*I assume everyone is unicoded */ ? utf8strRaw : utf8strNone);

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
		
		if (min > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, min, !kad);
		}

		if (max > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, max, !kad);
		}
		
		if (availability > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, availability, !kad);
		}

		if (!extension.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, extension);
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
		if (!extension.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (availability > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}
	  
		if (max > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}
        
		if (min > 0) {
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

		if (!typeText.IsEmpty()) {
			// Type is always ASCII string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, typeText);
		}

		if (min > 0) {
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, min, !kad);
		}

		if (max > 0) {
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, max, !kad);
		}

		if (availability > 0) {
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, availability, !kad);
		}

		if (!extension.IsEmpty()) {
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, extension);
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

#if wxUSE_UNICODE
	EUtf8Str eStrEncode = utf8strRaw;
#else
	EUtf8Str eStrEncode = utf8strNone;
#endif
	
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
	
	CSearchFile* tempFile = new CSearchFile(temp, eStrEncode == utf8strRaw, searchID , 0, 0, wxEmptyString, true);
	AddToList(tempFile);
	
}
