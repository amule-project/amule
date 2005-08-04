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
#pragma implementation "SearchList.h"
#endif

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
#include "UploadQueue.h"	// Needed for AddUpDataOverheadServer
#include "Statistics.h"		// Needed for CStatistics
#include "ObservableQueue.h"		// Needed for CQueueObserver
#include "Format.h"
#include "Logger.h"
#include "Preferences.h"
#include "kademlia/utils/UInt128.h" // Needed for CUInt128

#include <algorithm>

#ifndef AMULE_DAEMON
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "muuli_wdr.h"		// Needed for ID_NOTEBOOK
#endif

#ifdef __COMPILE_KAD__
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/Search.h"
#endif

CGlobalSearchThread::CGlobalSearchThread( CPacket* packet )
	: wxThread(wxTHREAD_DETACHED)
{
	m_packet = packet;
	
	CServer* current = theApp.serverconnect->GetCurrentServer();
	current = theApp.serverlist->GetServerByIP( current->GetIP(), current->GetPort() );
	
	m_packet->SetOpCode(OP_GLOBSEARCHREQ);

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
	
				theApp.statistics->AddUpDataOverheadServer( m_packet->GetPacketSize() );
				theApp.serverconnect->SendUDPPacket( m_packet, server, false );
		
				int percentage = 100 - ( queue.GetRemaining() * 100 ) / theApp.serverlist->GetServerCount();
				CoreNotify_Search_Update_Progress( percentage );
			}	
		}
		
		Sleep(750);
		
	}
	
	// Global search ended, reset progress and controls
	CoreNotify_Search_Update_Progress(0xffff);
	
	theApp.searchlist->ClearThreadData(this);
		
	return NULL;
}

CSearchFile::CSearchFile(const CMemFile& in_data, bool bOptUTF8, long nSearchID, uint32 WXUNUSED(nServerIP), uint16 WXUNUSED(nServerPort), const wxString& pszDirectory, bool nKademlia)
{
	m_nSearchID = nSearchID;
	m_nKademlia = nKademlia;
	
	in_data.ReadHash16(m_abyFileHash);
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
				if (tag->IsInt()) {
					if (m_nKademlia) {
						if (count > tag->GetInt()) {
							tag->SetInt(count);
						}
					} else {
						tag->SetInt(tag->GetInt() + count);
					}
				}
				break;
				
			case FT_COMPLETE_SOURCES:
				if (tag->IsInt()) {
					if (m_nKademlia) {
						if (count > tag->GetInt())
							tag->SetInt(count_complete);
						} else { 
							tag->SetInt(tag->GetInt() + count_complete);
						}
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
	#ifdef __COMPILE_KAD__
	// A non-existant search id will just be ignored
	Kademlia::CSearchManager::stopSearch(nSearchID,true);
	#endif
	
	ResultMap::iterator it = m_Results.find( nSearchID );

	if ( it != m_Results.end() ) {
		SearchList& list = it->second;
	
		for ( unsigned int i = 0; i < list.size(); ++i ) 
			delete list[i];
	
		m_Results.erase( it );
	}
}


bool CSearchList::StartNewSearch(uint32* nSearchID, SearchType search_type, const wxString& searchString, const wxString& typeText, 
											const wxString& extension, uint32 min, uint32 max, uint32 availability)
{
	
	if(!theApp.IsConnected()) {
		// Failed!
		return false;
	}
	
	m_resultType = typeText;
	m_CurrentSearch = *(nSearchID); // This will be set for ed2k results

	CMemFile* ed2k_data = CreateED2KSearchData(searchString, typeText, extension, min, max, availability, (search_type == KadSearch));
	
	if (search_type == KadSearch) {
		#ifdef __COMPILE_KAD__
		if (Kademlia::CKademlia::isRunning()) {
			try {
				// Kad search takes ownership of data and searchstring will get tokenized there
				Kademlia::CSearch* search = Kademlia::CSearchManager::prepareFindKeywords(searchString,ed2k_data);
				*(nSearchID) = search->getSearchID(); // The tab must be created with the Kad search id
			} catch(wxString& what) {
				AddLogLineM(true,what);
				delete ed2k_data;
				return false;				
			}
			return true;
		} else {
			delete ed2k_data;
			return false;
		}
		#else
		delete ed2k_data;
		return false;
		#endif
	}
	
	// This is ed2k search...
	
	if(!theApp.IsConnectedED2K()) {
		// Failed!
		delete ed2k_data;
		return false;
	}
	
	// Packet takes ownership of data
	CPacket* searchpacket = new CPacket(ed2k_data);
	searchpacket->SetOpCode(OP_SEARCHREQUEST);
	

	theApp.statistics->AddUpDataOverheadServer(searchpacket->GetPacketSize());
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
	
	return true;
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
	CMemFile packet((byte*)in_packet,size,0);

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
	// If filesize is 0, drop it (why would we want to download a 0-byte file anyway?)
	if (!toadd->GetIntTagValue(FT_FILESIZE)) {
		delete toadd;
		return false;
	}
	
	// If the result was not the type user wanted, drop it.
	if (	!bClientResponse &&
		!(m_resultType == wxString(wxT("Any")) ||
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


CMemFile* CSearchList::CreateED2KSearchData(const wxString& searchString, const wxString& typeText,
				const wxString &extension, uint32 min, uint32 max, uint32 avaibility, bool kad_padding)
{
	// Count the number of used parameters
	int parametercount = 0;
	if ( !searchString.IsEmpty() )	++parametercount;
	if ( !typeText.IsEmpty() )	++parametercount;
	if ( min > 0 )			++parametercount;
	if ( max > 0 ) 			++parametercount;
	if ( avaibility > 0 )		++parametercount;
	if ( !extension.IsEmpty() )	++parametercount;
	
	// Must write parametercount - 1 parameter headers
	CMemFile* data =  new CMemFile(100);
	
	if (kad_padding) {
		// We need to make some room for the keyword hash
		data->WriteUInt128((uint32)0);
		// and the search type (0/1 if there is ed2k data or not)		
		data->WriteUInt8(0);
		// Now the search string
		data->WriteUInt8( 0x01 ); // Search-String is a string parameter type
		data->WriteString( searchString, utf8strRaw );   // Write the value of the string
		// Nothing more is supported on Kad right now
		return data;
	}
	
	const byte stringParameter = 1;
	const byte typeParameter = 2;
	const byte numericParameter = 3;
	const uint16 andParameter = 0x0000;	
	// Kry - sadly, it has to keep like this, It's 3 bytes, god knows why.
	// So we can't use any WriteUInt*
	const char typeNemonic[3] = {0x01,0x00,0x03};
	const char extensionNemonic[3] = {0x01,0x00,0x04};
	const uint32 minNemonic = 0x02000101;
	const uint32 maxNemonic = 0x02000102;
	const uint32 avaibilityNemonic = 0x15000101;
	
	for ( int i = 0; i < parametercount - 1; ++i ) {
		data->WriteUInt16(andParameter);
	}

	// Packet body:
	if ( !searchString.IsEmpty() ) {
		data->WriteUInt8( stringParameter ); // Search-String is a string parameter type
		data->WriteString( searchString, utf8strRaw );   // Write the value of the string
	}
	
	if ( !typeText.IsEmpty() ) {
		data->WriteUInt8( typeParameter );		// Search-Type is a type parameter type
		data->WriteString( typeText ); 			// Write the parameter
		data->Write(typeNemonic, 3); 		// Nemonic for this kind of parameter (only 3 bytes!!)
	}
	
	if ( min > 0 ) {
		data->WriteUInt8( numericParameter );	// Write the parameter type
		data->WriteUInt32( min );		// Write the parameter
		data->WriteUInt32( minNemonic );	// Nemonic for this kind of parameter
	}
	
	if ( max > 0 ) {
		data->WriteUInt8( numericParameter );	// Write the parameter type
		data->WriteUInt32( max );		// Write the parameter
		data->WriteUInt32( maxNemonic );	// Nemonic for this kind of parameter
	}
	
	if ( avaibility > 0 ) {
		data->WriteUInt8( numericParameter );	// Write the parameter type
		data->WriteUInt32( avaibility );	// Write the parameter
		data->WriteUInt32( avaibilityNemonic );	// Nemonic for this kind of parameter
	}
	
	if ( !extension.IsEmpty() ) {
		data->WriteUInt8( stringParameter );	// Write the parameter type
		data->WriteString( extension );			// Write the parameter
		data->Write(extensionNemonic, 3); // Nemonic for this kind of parameter (only 3 bytes!!)
	}

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
	temp.WriteHash16(fileid);
	
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
