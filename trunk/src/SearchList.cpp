// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "muuli_wdr.h"		// Needed for ID_NOTEBOOK
#include "SearchList.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for GetFiletypeByName
#include "updownclient.h"	// Needed for CUpDownClient
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "SearchListCtrl.h"	// Needed for CSearchListCtrl
#include "packets.h"		// Needed for CTag
#include "CFile.h"		// Needed for CFile
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "amule.h"			// Needed for theApp
#include "ServerSocket.h"
#include "server.h"
#include "ServerList.h"
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "SharedFileList.h" // Needed for GetFileByID
#include "DownloadQueue.h"  // Needed for GetFileByID

CGlobalSearchThread::CGlobalSearchThread(Packet *packet)
{
	CGlobalSearchThread::packet = packet;
	
	CServer* current = theApp.serverconnect->GetCurrentServer();
	current = theApp.serverlist->GetServerByIP( current->GetIP(), current->GetPort() );
	askedlist.insert( current );
	
	packet->SetOpCode(OP_GLOBSEARCHREQ);
}

void CGlobalSearchThread::Start()
{
}

Packet *CreateSearchPacket(wxString &searchString, wxString& typeText,
				wxString &extension, uint32 min, uint32 max, uint32 avaibility)
{
	// Count the number of used parameters
	int parametercount = 0;
	if ( !searchString.IsEmpty() )	parametercount++;
	if ( !typeText.IsEmpty() ) 	parametercount++;
	if ( min > 0 )			parametercount++;
	if ( max > 0 ) 			parametercount++;
	if ( avaibility > 0 ) 		parametercount++;
	if ( !extension.IsEmpty() )	parametercount++;
	
	// Must write parametercount - 1 parameter headers
	CSafeMemFile* data = new CSafeMemFile(100);
	
	const byte stringParameter = 1;
	const byte typeParameter = 2;
	const byte numericParameter = 3;
	const uint16 andParameter = 0x0000;	
	const uint32 typeNemonic = 0x00030001;
	const uint32 minNemonic = 0x02000101;
	const uint32 maxNemonic = 0x02000102;
	const uint32 avaibilityNemonic = 0x15000101;
	const uint32 extensionNemonic = 0x00040001;
	
	for ( int i = 0; i < parametercount - 1; i++ ) {
		data->WriteUInt16(andParameter);
	}

	// Packet body:
	if ( !searchString.IsEmpty() ) {
		data->WriteUInt8( stringParameter ); // Search-String is a string parameter type
		data->WriteString( searchString );   // Write the value of the string
	}
	
	if ( !typeText.IsEmpty() ) {
		data->WriteUInt8( typeParameter );		// Search-Type is a type parameter type
		data->WriteString( typeText ); 			// Write the parameter
#if wxBYTE_ORDER == wxLITTLE_ENDIAN
		data->Write(&typeNemonic, 3); 		// Nemonic for this kind of parameter (only 3 bytes!!)
#else
		uint32 endian_corrected = ENDIAN_SWAP_32(typeNemonic);
		data->Write(&endian_corrected, 3); 	// Nemonic for this kind of parameter (only 3 bytes!!)
#endif
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
#if wxBYTE_ORDER == wxLITTLE_ENDIAN
		data->Write(&extensionNemonic, 3); // Nemonic for this kind of parameter (only 3 bytes!!)
#else
		uint32 endian_corrected = ENDIAN_SWAP_32(extensionNemonic);
		data->Write(&endian_corrected, 3); // Nemonic for this kind of parameter (only 3 bytes!!)
#endif		
	}
	Packet* packet = new Packet(data);
	packet->SetOpCode(OP_SEARCHREQUEST);
	delete data;
	
	return packet;
}

bool IsValidClientIPPort(uint32 nIP, uint16 nPort)
{
	return     nIP != 0
			&& nPort != 0
		        && ((nIP) != nPort)		// this filters most of the false data
			&& ((nIP & 0x000000FF) != 0)
			&& ((nIP & 0x0000FF00) != 0)
			&& ((nIP & 0x00FF0000) != 0)
			&& ((nIP & 0xFF000000) != 0);
}


///////////////////////////////////////////////////////////////////////////////
// CSearchFile
	
CSearchFile::CSearchFile(CSearchFile* copyfrom)
{
	m_abyFileHash = copyfrom->GetFileHash();
	SetFileSize(copyfrom->GetIntTagValue(FT_FILESIZE));
	SetFileName(char2unicode(copyfrom->GetStrTagValue(FT_FILENAME)));
	m_nClientServerIP = copyfrom->GetClientServerIP();
	m_nClientID = copyfrom->GetClientID();
	m_nClientPort = copyfrom->GetClientPort();
	m_nClientServerPort = copyfrom->GetClientServerPort();
	m_pszDirectory = copyfrom->m_pszDirectory ? nstrdup(copyfrom->m_pszDirectory) : NULL;
	m_nSearchID = copyfrom->GetSearchID();
	for (unsigned int i = 0; i < copyfrom->taglist.size(); i++)
		taglist.push_back(new CTag(*copyfrom->taglist.at(i)));

	m_aServers.insert( m_aServers.end(), copyfrom->GetServers().begin(), 
	                                     copyfrom->GetServers().end() );

	m_list_bExpanded = false;
	m_list_parent = copyfrom;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}


CSearchFile::CSearchFile(const CSafeMemFile* in_data, long nSearchID, uint32 nServerIP, uint16 nServerPort, LPCTSTR pszDirectory)
{
	m_nSearchID = nSearchID;
	in_data->ReadHash16(m_abyFileHash);
	m_nClientID = in_data->ReadUInt32();
	m_nClientPort = in_data->ReadUInt16();
	if ((m_nClientID || m_nClientPort) && !IsValidClientIPPort(m_nClientID, m_nClientPort)){
		m_nClientID = 0;
		m_nClientPort = 0;
	}
	uint32 tagcount = in_data->ReadUInt32();

	for (unsigned int i = 0; i != tagcount; ++i){
		CTag* toadd = new CTag(*in_data);
		taglist.push_back(toadd);
	}

	// here we have two choices
	//	- if the server/client sent us a filetype, we could use it (though it could be wrong)
	//	- we always trust our filetype list and determine the filetype by the extension of the file
	char* tempName = GetStrTagValue(FT_FILENAME);
	if ( !tempName )
		throw CInvalidPacket("No filename in search result");
	int iSize = (int)strlen(tempName)+1;
	if ( iSize < 2 ) {
		iSize = 2;		// required by tag format
	}

	SetFileName(char2unicode(tempName));
	
	SetFileSize(GetIntTagValue(FT_FILESIZE));

	m_nClientServerIP = nServerIP;
	m_nClientServerPort = nServerPort;
	if (m_nClientServerIP && m_nClientServerPort){
		SServer server(m_nClientServerIP, m_nClientServerPort);
		server.m_uAvail = GetIntTagValue(FT_SOURCES);
		AddServer(server);
	}

	m_pszDirectory = pszDirectory ? nstrdup(pszDirectory) : NULL;
	
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}

CSearchFile::CSearchFile(long nSearchID, const CMD4Hash& pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int WXUNUSED(iFileType), int iAvailability)
{
	m_nSearchID = nSearchID;
	m_abyFileHash = pucFileHash;

	taglist.push_back(new CTag(FT_FILESIZE, uFileSize));
	taglist.push_back(new CTag(FT_FILENAME, pszFileName));
	taglist.push_back(new CTag(FT_SOURCES, iAvailability));
	printf("Filename2: %s\n",pszFileName);

	SetFileName(char2unicode(pszFileName));
	SetFileSize(uFileSize);

	m_nClientID = 0;
	m_nClientPort = 0;
	m_nClientServerIP = 0;
	m_nClientServerPort = 0;
	m_pszDirectory = NULL;
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
	
}

CSearchFile::~CSearchFile(){
	
	for (unsigned int i = 0; i != taglist.size();i++) {
		delete taglist[i];
	}
	taglist.clear();

	if (m_pszDirectory)
		delete[] m_pszDirectory;
}

uint32 CSearchFile::GetIntTagValue(uint8 tagname){
	for (unsigned int i = 0; i != taglist.size(); i++){
		if (taglist[i]->tag.specialtag == tagname)
			return taglist[i]->tag.intvalue;
	}
	return 0;
}

char* CSearchFile::GetStrTagValue(uint8 tagname){
	for (unsigned int i = 0; i != taglist.size(); i++){
		if (taglist[i]->tag.specialtag == tagname)
			return taglist[i]->tag.stringvalue;
	}
	return 0;
}

uint32 CSearchFile::AddSources(uint32 count, uint32 count_complete){
	for (unsigned int i = 0; i != taglist.size(); i++){
		if (taglist[i]->tag.specialtag == FT_SOURCES){
			taglist[i]->tag.intvalue += count;
			return taglist[i]->tag.intvalue;
		}
		if (taglist[i]->tag.specialtag == FT_COMPLETE_SOURCES){
			taglist[i]->tag.intvalue += count_complete;
			return taglist[i]->tag.intvalue;
		}		
	}
	return 0;
}

uint32 CSearchFile::GetSourceCount(){
	return GetIntTagValue(FT_SOURCES);
}

uint32 CSearchFile::GetCompleteSourceCount(){
	return GetIntTagValue(FT_COMPLETE_SOURCES);
}

CSearchList::CSearchList(){
}

CSearchList::~CSearchList(){
	Clear();
}

void CSearchList::Clear(){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; ) {
		delete list.GetNext(pos);
	}
	list.RemoveAll();
}

void CSearchList::RemoveResults(long nSearchID){
	// this will not delete the item from the window, make sure your code does it if you call this
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CSearchFile* cur_file =	list.GetAt(pos2);
		if( cur_file->GetSearchID() == nSearchID ){
			list.RemoveAt(pos2);
			delete cur_file;
		}
	}
}

// void CSearchList::ShowResults(long nSearchID){
// 	CSearchListCtrl* outputwnd = GetSearchListControl(nSearchID);
// 	if ( outputwnd ) {
// 		//outputwnd->SetRedraw(false);
// 		for (POSITION pos = list.GetHeadPosition(); pos !=0;list.GetNext(pos)){
// 			if( ((CSearchFile*)list.GetAt(pos))->GetSearchID() == nSearchID ){
// 				outputwnd->AddResult(list.GetAt(pos));
// 			}
// 		}
// 		//outputwnd->SetRedraw(true);
// 	
// 		// Update the result count
// 		theApp.amuledlg->searchwnd->UpdateHitCount( outputwnd );
// 	}
// 
// 	UngetSearchListControl(outputwnd);
// }

void CSearchList::NewSearch(const wxString& resTypes, long nSearchID){
	resultType=resTypes;
	m_nCurrentSearch = nSearchID;
	myHashList=wxEmptyString;

	foundFilesCount[nSearchID] = 0;
}

// NEVER called: commenting it out untill someone will explain why it's here
// uint16 CSearchList::ProcessSearchanswer(const char *in_packet, uint32 size, CUpDownClient *Sender) {
// 	const CSafeMemFile *packet = new CSafeMemFile((BYTE*)in_packet,size,0);
// #ifndef AMULE_DAEMON
// 	if(Sender) {
// 	  theApp.amuledlg->searchwnd->CreateNewTab(Sender->GetUserName() + wxT(" (0)"),(long)Sender);
// 	}
// #endif
// 	try
// 	{
// 		uint32 results = packet->ReadUInt32();
// 		long mySearchID=( (Sender != NULL)? (long)Sender : m_nCurrentSearch);
// 		foundFilesCount[mySearchID] = 0;
// 
// 		try
// 		{
// 			for (uint32 i = 0; i != results; i++){
// 				CSearchFile* toadd = new CSearchFile(packet, mySearchID);
// 				AddToList(toadd, (Sender != NULL) );
// 			}
// 		}
// 		catch ( CStrangePacket )
// 		{
// 			printf("Strange search result on packet\n");
// 		}
// 		catch ( CInvalidPacket )
// 		{
// 			printf("Invalid search result on packet\n");
// 		}
// 		
// 	}
// 	catch ( CInvalidPacket e )
// 	{
// 
// 		printf("Invalid search result packet: %s\n", e.what());
// 		DumpMem(in_packet, size);
// 
// 	}
// 
// 	packet->Close();
// 	delete packet;
// 	return GetResultCount();
// }

// Called when some of clients returns list of files
uint16 CSearchList::ProcessSearchanswer(const char *in_packet, uint32 size, 
	CUpDownClient *Sender, bool *pbMoreResultsAvailable, LPCTSTR pszDirectory)
{
	wxASSERT( Sender != NULL );
	// Elandal: Assumes sizeof(void*) == sizeof(uint32)
	long nSearchID = (long)Sender;
#ifndef AMULE_DAEMON
	if (!theApp.amuledlg->searchwnd->CheckTabNameExists(Sender->GetUserName())) {
		theApp.amuledlg->searchwnd->CreateNewTab(Sender->GetUserName() + wxT(" (0)"),nSearchID);
	}
#endif
	const CSafeMemFile packet((BYTE*)in_packet,size);
	uint32 results = packet.ReadUInt32();

	for (unsigned int i = 0; i != results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, nSearchID, 0, 0, pszDirectory);
		if (Sender){
			toadd->SetClientID(Sender->GetUserID());
			toadd->SetClientPort(Sender->GetUserPort());
			toadd->SetClientServerIP(Sender->GetServerIP());
			toadd->SetClientServerPort(Sender->GetServerPort());
			if (Sender->GetServerIP() && Sender->GetServerPort()){
	   			CSearchFile::SServer server(Sender->GetServerIP(), Sender->GetServerPort());
				server.m_uAvail = 1;
				toadd->AddServer(server);
			}			
			// Well, preview is not available yet.
			//toadd->SetPreviewPossible( Sender->SupportsPreview() && ED2KFT_VIDEO == GetED2KFileTypeID(toadd->GetFileName()) );
		}
		AddToList(toadd, true);
	}

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	
	int iAddData = (int)(packet.Length() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
		}
	}

	packet.Close();
	return GetResultCount(nSearchID);
}

//
// Called from ServerSocket when server returns answer about local search
uint16 CSearchList::ProcessSearchanswer(const char *in_packet, uint32 size, uint32 nServerIP, uint16 nServerPort) {

	//CSafeMemFile packet((BYTE*)in_packet,size);
	const CSafeMemFile* packet = new CSafeMemFile((BYTE*)in_packet,size,0);

	uint32 results = packet->ReadUInt32();

	
//	in_addr server;
//	server.s_addr = nServerIP;
//	printf("ip %s siz %i rslt %i\n",inet_ntoa(server), size, results);

	//DumpMem(in_packet,64);

	
	for (unsigned int i = 0; i != results; ++i){
//		printf("Result %i\n",i);
//		DumpMem(packet->GetCurrentBuffer(),64);
		CSearchFile* toadd = new CSearchFile(packet, m_nCurrentSearch);
		toadd->SetClientServerIP(nServerIP);
		toadd->SetClientServerPort(nServerPort);
		if (nServerIP && nServerPort){
   			CSearchFile::SServer server(nServerIP, nServerPort);
			server.m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
			toadd->AddServer(server);
		}
		AddToList(toadd, false);
	}
	/*if (m_MobilMuleSearch)
		theApp.mmserver->SearchFinished(false);
	m_MobilMuleSearch = false;*/
	packet->Close();
	delete packet;
	return GetResultCount();
}

uint16 CSearchList::ProcessUDPSearchanswer(CSafeMemFile* packet, uint32 nServerIP, uint16 nServerPort)
{
	CSearchFile* toadd = new CSearchFile(packet, m_nCurrentSearch, nServerIP, nServerPort);
	AddToList(toadd);
	return GetResultCount();
}
/*
uint16 CSearchList::ProcessUDPSearchanswer(char* in_packet, uint32 size){
	CSafeMemFile* packet = new CSafeMemFile((BYTE*)in_packet,size,0);
	try
	{
		CSearchFile* toadd = new CSearchFile(packet, m_nCurrentSearch);
		AddToList(toadd);
	}
	catch ( CStrangePacket )
	{ }
	packet->Close();
	delete packet;
	return GetResultCount();
}
*/
bool CSearchList::AddToList(CSearchFile* toadd, bool bClientResponse){
	
	// If filesize is 0, drop it (why would we want to download a 0-byte file anyway?)
	if (!toadd->GetIntTagValue(FT_FILESIZE)) {
		delete toadd;
		return false;
	}
	
	// If the result was not the type user wanted, drop it.
	if (!bClientResponse && !(resultType == wxString(_("Any")) || GetFiletypeByName(toadd->GetFileName())==resultType)){
		delete toadd;
		return false;
	}

	for (POSITION pos = list.GetHeadPosition(); pos != NULL; ){
		CSearchFile* cur_file = list.GetNext(pos);
		if ( (toadd->GetFileHash() == cur_file->GetFileHash()) && cur_file->GetSearchID() ==  toadd->GetSearchID()){
			cur_file->AddSources(toadd->GetIntTagValue(FT_SOURCES),toadd->GetIntTagValue(FT_COMPLETE_SOURCES));
			CoreNotify_Search_Update_Sources(toadd, cur_file);
			delete toadd;
			return true;
		}
	}
	if (list.AddTail(toadd)) {	
		foundFilesCount[toadd->GetSearchID()]++;
	}
	CoreNotify_Search_Add_Result(toadd);
	return true;
}

uint16 CSearchList::GetResultCount(long nSearchID) {
	uint16 hits = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; list.GetNext(pos)){
		if( list.GetAt(pos)->GetSearchID() == nSearchID ) {
			hits += list.GetAt(pos)->GetSourceCount();
		}
	}
	return hits;
}


uint16 CSearchList::GetResultCount(){
	return GetResultCount(m_nCurrentSearch);
}

uint16 CSearchList::GetFoundFiles(long searchID) {
	return foundFilesCount[ searchID ];
}

wxString CSearchList::GetWebList(const wxString& linePattern,int sortby,bool asc) const {
	wxString buffer;
	wxString temp;
	std::list<CSearchFile*> sortarray;
	int swap = 0;
	bool inserted = false;

	// insertsort
	CSearchFile* sf1;
	CSearchFile* sf2;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;) {
		inserted=false;
		sf1 = list.GetNext(pos);
		
		if (sf1->GetListParent()!=NULL) continue;
		
		std::list<CSearchFile*>::iterator it = sortarray.begin();
		for ( ; it != sortarray.end(); ++it ) {
			sf2 = (*it);
			
			switch (sortby) {
				case 0: swap=sf1->GetFileName().CmpNoCase(sf2->GetFileName()); break;
				case 1: swap=sf1->GetFileSize()-sf2->GetFileSize();break;
				case 2: swap=EncodeBase16(sf1->GetFileHash(), 16).Cmp( EncodeBase16(sf2->GetFileHash(), 16) ); break;
				case 3: swap=sf1->GetSourceCount()-sf2->GetSourceCount(); break;
			}
			if (!asc) swap=0-swap;
			if (swap<0) {inserted=true; sortarray.insert(it, sf1);break;}
		}
		if (!inserted) sortarray.push_back(sf1);
	}
	
	std::list<CSearchFile*>::iterator it = sortarray.begin();
	for ( ; it != sortarray.end(); ++it ) {
		/*const*/ CSearchFile* sf = (*it);

		// colorize
		wxString coloraddon;
		wxString coloraddonE;
		CKnownFile* sameFile = theApp.sharedfiles->GetFileByID(sf->GetFileHash());
		CPartFile* samePFile = NULL;
		
		if (!sameFile)
			samePFile = theApp.downloadqueue->GetFileByID(sf->GetFileHash());

#if 0 //shakraw
		if (sameFile) {
			if (sameFile->IsPartFile())
				coloraddon = _T("<font color=\"#FF0000\">");
			else
				coloraddon = _T("<font color=\"#00FF00\">");
		}
#endif
		
		if (sameFile) 
			coloraddon = _T("<font color=\"#00FF00\">");
		else
			coloraddon = _T("<font color=\"#FF0000\">");
		
		
		if (coloraddon.Length()>0)
			coloraddonE = _T("</font>");

		wxString strHash(EncodeBase16(sf->GetFileHash(),16));
		temp.Printf(linePattern,
					//coloraddon + StringLimit(sf->GetFileName(),70) + coloraddonE,
					wxString(coloraddon + sf->GetFileName() + coloraddonE).GetData(),
					CastItoXBytes(sf->GetFileSize()).GetData(),
					strHash.GetData(),
					sf->GetSourceCount(),
					strHash.GetData());
		buffer.Append(temp);
	}
	return buffer;
}

void CSearchList::AddFileToDownloadByHash(const CMD4Hash& hash,uint8 cat) {
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ){
		CSearchFile* sf=list.GetNext(pos);
		if (hash == sf->GetFileHash()) {
			CoreNotify_Search_Add_Download(sf, cat);
			break;
		}
	}
}
