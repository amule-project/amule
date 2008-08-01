//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://sourceforge.net/projects/amule )
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
#include "CamuleAppBase.h"	// Needed for theApp
#include "MuleNotebook.h"	// Needed for CMuleNotebook
#include "SharedFileList.h" // Needed for GetFileByID
#include "DownloadQueue.h"  // Needed for GetFileByID

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

// CARE: This will lock the notebook when it does find a control.
//       Call the function UngetSearchListControl() to unlock the notebook. But
//       only after you are done with the Control returned by this function.
//
static CSearchListCtrl* GetSearchListControl(uint32 nSearchID)
{
	CMuleNotebook* nb=(CMuleNotebook*)wxWindow::FindWindowById(ID_NOTEBOOK);
	if ( !nb ) return NULL;

	nb->m_LockTabs.Lock();

	for (int tabCounter=0; tabCounter < nb->GetPageCount(); tabCounter++) {
		if(nb->GetUserData(tabCounter)==nSearchID) {
			return (CSearchListCtrl*)theApp.amuledlg->searchwnd->FindWindowById(ID_SEARCHLISTCTRL,nb->GetPage(tabCounter));
		}
	}

	nb->m_LockTabs.Unlock();
	return NULL;
}

static void UngetSearchListControl(CSearchListCtrl* ctrl)
{
	if ( !ctrl ) return;			// NB was already unlocked

	CMuleNotebook* nb=(CMuleNotebook*)wxWindow::FindWindowById(ID_NOTEBOOK);
	nb->m_LockTabs.Unlock();
}

///////////////////////////////////////////////////////////////////////////////
// CSearchFile
	
CSearchFile::CSearchFile(CSearchFile* copyfrom)
{
	int i;
	
	md4cpy(m_abyFileHash, copyfrom->GetFileHash());
	SetFileSize(copyfrom->GetIntTagValue(FT_FILESIZE));
	SetFileName(copyfrom->GetStrTagValue(FT_FILENAME));
	m_nClientServerIP = copyfrom->GetClientServerIP();
	m_nClientID = copyfrom->GetClientID();
	m_nClientPort = copyfrom->GetClientPort();
	m_nClientServerPort = copyfrom->GetClientServerPort();
	m_pszDirectory = copyfrom->m_pszDirectory? nstrdup(m_pszDirectory) : NULL;
	m_nSearchID = copyfrom->GetSearchID();
	for (i = 0; i < copyfrom->taglist.GetCount(); i++)
		taglist.Add(new CTag(*copyfrom->taglist.GetAt(i)));
	for (i = 0; i < copyfrom->GetServers().GetSize(); i++){
		SServer server = *copyfrom->GetServer(i);
		AddServer(&server);
	}

	m_list_bExpanded = false;
	m_list_parent = copyfrom;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}


CSearchFile::CSearchFile(CMemFile* in_data, uint32 nSearchID, uint32 nServerIP, uint16 nServerPort, LPCTSTR pszDirectory)
{
	m_nSearchID = nSearchID;
	in_data->Read(m_abyFileHash);
	in_data->Read(m_nClientID);
	in_data->Read(m_nClientPort);
	if ((m_nClientID || m_nClientPort) && !IsValidClientIPPort(m_nClientID, m_nClientPort)){
		m_nClientID = 0;
		m_nClientPort = 0;
	}
	uint32 tagcount;
	in_data->Read(tagcount);

	for (unsigned int i = 0; i != tagcount; ++i){
		CTag* toadd = new CTag(in_data);
		taglist.Add(toadd);
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

	m_strFileName = tempName;
	
	SetFileSize(GetIntTagValue(FT_FILESIZE));

	m_nClientServerIP = nServerIP;
	m_nClientServerPort = nServerPort;
	if (m_nClientServerIP && m_nClientServerPort){
		SServer* server = new SServer(m_nClientServerIP, m_nClientServerPort);
		server->m_uAvail = GetIntTagValue(FT_SOURCES);
		AddServer(server);
	}

	m_pszDirectory = pszDirectory ? nstrdup(pszDirectory) : NULL;
	
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}

CSearchFile::CSearchFile(uint32 nSearchID, const uchar* pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability)
{
	m_nSearchID = nSearchID;
	md4cpy(m_abyFileHash, pucFileHash);
	taglist.Add(new CTag(FT_FILESIZE, uFileSize));
	taglist.Add(new CTag(FT_FILENAME, pszFileName));
	taglist.Add(new CTag(FT_SOURCES, iAvailability));
	SetFileName(pszFileName);
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
	for (int i = 0; i != taglist.GetSize();i++)
		delete taglist[i];
	taglist.RemoveAll();
	taglist.SetSize(0);
}

uint32 CSearchFile::GetIntTagValue(uint8 tagname){
	for (int i = 0; i != taglist.GetSize(); i++){
		if (taglist[i]->tag.specialtag == tagname)
			return taglist[i]->tag.intvalue;
	}
	return 0;
}

char* CSearchFile::GetStrTagValue(uint8 tagname){
	for (int i = 0; i != taglist.GetSize(); i++){
		if (taglist[i]->tag.specialtag == tagname)
			return taglist[i]->tag.stringvalue;
	}
	return 0;
}

uint32 CSearchFile::AddSources(uint32 count){
	for (int i = 0; i != taglist.GetSize(); i++){
		if (taglist[i]->tag.specialtag == FT_SOURCES){
			taglist[i]->tag.intvalue += count;
			return taglist[i]->tag.intvalue;
		}
	}
	return 0;
}

uint32 CSearchFile::GetSourceCount(){
	return GetIntTagValue(FT_SOURCES);
}

CSearchList::CSearchList(){
}

CSearchList::~CSearchList(){
	Clear();
}

void CSearchList::Clear(){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; pos = list.GetHeadPosition()) {
		delete list.GetAt(pos);
		list.RemoveAt(pos);
	}
}

void CSearchList::RemoveResults( uint32 nSearchID){
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

void CSearchList::ShowResults( uint32 nSearchID){
	CSearchListCtrl* outputwnd = GetSearchListControl(nSearchID);
	if ( outputwnd ) {
		//outputwnd->SetRedraw(false);
		for (POSITION pos = list.GetHeadPosition(); pos !=0;list.GetNext(pos)){
			if( ((CSearchFile*)list.GetAt(pos))->GetSearchID() == nSearchID ){
				outputwnd->AddResult(list.GetAt(pos));
			}
		}
		//outputwnd->SetRedraw(true);
	}
	UngetSearchListControl(outputwnd);
}

// Ok, nobody knows why is this here, so disabling it until someone needs it.
#if 0
	void CSearchList::RemoveResults( CSearchFile* todel ){
		for (POSITION pos = list.GetHeadPosition(); pos !=0;list.GetNext(pos)){
			if( (CSearchFile*)list.GetAt(pos) == todel ){
			// this should also be routed to the selected page only
			CMuleNotebook* nb=(CMuleNotebook*)theApp.amuledlg->searchwnd->FindWindowById(ID_NOTEBOOK);
			if(nb->GetSelection()==-1)
			return;
			CSearchListCtrl* ctrl=(CSearchListCtrl*)nb->FindWindowById(ID_SEARCHLISTCTRL,nb->GetPage(nb->GetSelection()));
			//theApp.amuledlg->searchwnd->searchlistctrl->RemoveResult( todel );
				ctrl->RemoveResult(todel);
				list.RemoveAt(pos);
				delete todel;
				return;
			}
		}
	}
#endif

void CSearchList::NewSearch(CString resTypes, uint16 nSearchID){
	resultType=resTypes;
	m_nCurrentSearch = nSearchID;
	myHashList="";

	foundFilesCount.SetAt(nSearchID,0);
}

uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, CUpDownClient* Sender){
	CSafeMemFile* packet = new CSafeMemFile((BYTE*)in_packet,size,0);

	if(Sender) {
	  theApp.amuledlg->searchwnd->CreateNewTab(Sender->GetUserName(),(uint32)Sender);
	}

	try
	{
		uint32 results;
		// Why? Emule don't catch anything. I assume it's safe not to do that.
//		if ( 4 != packet->Read(&results,4) )
//			throw CInvalidPacket("short packet reading search result count");
		uint32 mySearchID=( (Sender != NULL)? (uint32)Sender : m_nCurrentSearch);
		foundFilesCount.SetAt(mySearchID,0);

		try
		{
			for (uint32 i = 0; i != results; i++){
				CSearchFile* toadd = new CSearchFile(packet, mySearchID);
				AddToList(toadd, (Sender != NULL) );
			}
		}
		catch ( CStrangePacket )
		{ }
	}
	catch ( CInvalidPacket e )
	{
#if 0
		printf("Invalid search result packet: %s\n", e.what());
		HexDump(in_packet, size);
#endif
	}

	packet->Close();
	delete packet;
	return GetResultCount();
}

uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory)
{
	wxASSERT( Sender != NULL );
	// Elandal: Assumes sizeof(void*) == sizeof(uint32)
	uint32 nSearchID = (uint32)Sender;
	/*
	SSearchParams* pParams = new SSearchParams;
	pParams->strExpression = Sender->GetUserName();
	pParams->dwSearchID = nSearchID;
	if (theApp.amuledlg->searchwnd->CreateNewTab(pParams)){
		m_foundFilesCount.SetAt(nSearchID,0);
		m_foundSourcesCount.SetAt(nSearchID,0);
	}
	else{
		delete pParams;
		pParams = NULL;
	}
	*/
	if (!theApp.amuledlg->searchwnd->CheckTabNameExists(Sender->GetUserName())) {
		theApp.amuledlg->searchwnd->CreateNewTab(Sender->GetUserName(),nSearchID);
	}
	
	CSafeMemFile packet((BYTE*)in_packet,size);
	uint32 results;
	packet.Read(results);

	for (unsigned int i = 0; i != results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, nSearchID, 0, 0, pszDirectory);
		if (Sender){
			toadd->SetClientID(Sender->GetUserID());
			toadd->SetClientPort(Sender->GetUserPort());
			toadd->SetClientServerIP(Sender->GetServerIP());
			toadd->SetClientServerPort(Sender->GetServerPort());
			if (Sender->GetServerIP() && Sender->GetServerPort()){
	   			CSearchFile::SServer* server = new CSearchFile::SServer(Sender->GetServerIP(), Sender->GetServerPort());
				server->m_uAvail = 1;
				toadd->AddServer(server);
			}			
			// Well, preview is not available yet.
			//toadd->SetPreviewPossible( Sender->SupportsPreview() && ED2KFT_VIDEO == GetED2KFileTypeID(toadd->GetFileName()) );
		}
		AddToList(toadd, true);
	}

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore;
		packet.Read(ucMore);
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
		}
	}

	packet.Close();
	return GetResultCount(nSearchID);
}


uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, uint32 nServerIP, uint16 nServerPort){

	//CSafeMemFile packet((BYTE*)in_packet,size);
	CSafeMemFile* packet = new CSafeMemFile((BYTE*)in_packet,size,0);

	uint32 results;
	packet->Read(results);

	for (unsigned int i = 0; i != results; ++i){
		CSearchFile* toadd = new CSearchFile(packet, m_nCurrentSearch);
		toadd->SetClientServerIP(nServerIP);
		toadd->SetClientServerPort(nServerPort);
		if (nServerIP && nServerPort){
   			CSearchFile::SServer* server = new CSearchFile::SServer(nServerIP, nServerPort);
			server->m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
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
		return false;
	}
	
	// If the result was not the type user wanted, drop it.
	if (!bClientResponse && !(resultType == CString(_("Any")) || GetFiletypeByName(toadd->GetFileName())==resultType)){
		delete toadd;
		return false;
	}

	CSearchListCtrl* outputwnd = GetSearchListControl(toadd->GetSearchID());
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; list.GetNext(pos)){
		CSearchFile* cur_file = list.GetAt(pos);
		if ( (!memcmp(toadd->GetFileHash(),cur_file->GetFileHash(),16)) && cur_file->GetSearchID() ==  toadd->GetSearchID()){
			cur_file->AddSources(toadd->GetIntTagValue(FT_SOURCES));
			if (outputwnd) {
				outputwnd->UpdateSources(cur_file);
			}
			delete toadd;
			UngetSearchListControl(outputwnd);
			return true;
		}
	}
	if (list.AddTail(toadd)) {	
		uint16 tempValue;
		foundFilesCount.Lookup(toadd->GetSearchID(),tempValue);
		foundFilesCount.SetAt(toadd->GetSearchID(),tempValue+1);
	}
	if (outputwnd) {
		outputwnd->AddResult(toadd);
	}
	UngetSearchListControl(outputwnd);
	return true;
}

uint16 CSearchList::GetResultCount(uint32 nSearchID) {
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

uint16 CSearchList::GetFoundFiles(uint32 searchID) {
	uint16 returnVal;
	foundFilesCount.Lookup(searchID,returnVal);
	return returnVal;
}

CString CSearchList::GetWebList(CString linePattern,int sortby,bool asc) const {
	CString buffer;
	CString temp;
	CArray<CSearchFile*, CSearchFile*> sortarray;
	int swap;
	bool inserted;

	// insertsort
	CSearchFile* sf1;
	CSearchFile* sf2;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;) {
		inserted=false;
		sf1 = list.GetNext(pos);
		
		if (sf1->GetListParent()!=NULL) continue;
		
		for (uint16 i1=0;i1<sortarray.GetCount();++i1) {
			sf2 = sortarray.GetAt(i1);
			
			switch (sortby) {
				case 0: swap=CString(sf1->GetFileName()).CmpNoCase(sf2->GetFileName()); break;
				case 1: swap=sf1->GetFileSize()-sf2->GetFileSize();break;
				case 2: swap=CString((char*)sf1->GetFileHash()).CmpNoCase((char*)sf2->GetFileHash()); break;
				case 3: swap=sf1->GetSourceCount()-sf2->GetSourceCount(); break;
			}
			if (!asc) swap=0-swap;
			if (swap<0) {inserted=true; sortarray.InsertAt(i1,sf1);break;}
		}
		if (!inserted) sortarray.Add(sf1);
	}
	
	for (uint16 i=0;i<sortarray.GetCount();++i) {
		/*const*/ CSearchFile* sf = sortarray.GetAt(i);

		// colorize
		CString coloraddon;
		CString coloraddonE;
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
		
		
		if (coloraddon.GetLength()>0)
			coloraddonE = _T("</font>");

		CString strHash(EncodeBase16(sf->GetFileHash(),16));
		temp.Format(linePattern,
					//coloraddon + StringLimit(sf->GetFileName(),70) + coloraddonE,
					CString(coloraddon + sf->GetFileName() + coloraddonE).GetData(),
					CastItoXBytes(sf->GetFileSize()).GetData(),
					strHash.GetData(),
					sf->GetSourceCount(),
					strHash.GetData());
		buffer.Append(temp);
	}
	return buffer;
}

void CSearchList::AddFileToDownloadByHash(const uchar* hash,uint8 cat) {
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ){
		CSearchFile* sf=list.GetNext(pos);//->GetSearchID() == nSearchID ){
		if (!md4cmp(hash,sf->GetFileHash())) {
			//theApp.downloadqueue->AddSearchToDownload(sf,2,cat);
			theApp.downloadqueue->AddSearchToDownload(sf);
			break;
		}
	}
}