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


#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include <utime.h>		// Needed for utime()
#include <wx/intl.h>		// Needed for _
#include <wx/setup.h>
#include <wx/gdicmn.h>
#include <wx/filename.h>	// Needed for wxFileName
#include <wx/msgdlg.h>		// Needed for wxMessageBox

#include "PartFile.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for nstrdup
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "SysTray.h"		// Needed for TBN_DLOAD
#include "UploadQueue.h"	// Needed for CFileHash
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "IPFilter.h"		// Needed for CIPFilter
#include "server.h"		// Needed for CServer
#include "sockets.h"		// Needed for CServerConnect
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "updownclient.h"	// Needed for CUpDownClient
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "AddFileThread.h"	// Needed for CAddFileThread
#include "SafeFile.h"		// Needed for CSafeFile
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "ED2KLink.h"		// Needed for CED2KLink
#include "packets.h"		// Needed for CTag
#include "SearchList.h"		// Needed for CSearchFile
#include "BarShader.h"		// Needed for CBarShader
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ClientList.h"		// Needed for clientlist
#include "color.h"

#include <map>
#include <algorithm>

#define PROGRESS_HEIGHT 3


wxMutex CPartFile::m_FileCompleteMutex; 

CPartFile::CPartFile()
{
	Init();
}

CPartFile::CPartFile(CSearchFile* searchresult)
{
	Init();
	m_abyFileHash = searchresult->GetFileHash();
	for (unsigned int i = 0; i < searchresult->taglist.size();i++){
		const CTag* pTag = searchresult->taglist[i];
		switch (pTag->tag.specialtag){
			case FT_FILENAME:{
				if (pTag->tag.type == 2)
					SetFileName(char2unicode(pTag->tag.stringvalue));
				break;
			}
			case FT_FILESIZE:{
				if (pTag->tag.type == 3) {
					SetFileSize(pTag->tag.intvalue);
				}
				break;
			}
			default:{
				bool bTagAdded = false;
				if (pTag->tag.specialtag == 0 && pTag->tag.tagname != NULL && (pTag->tag.type == 2 || pTag->tag.type == 3))
				{
					static const struct
					{
						LPCSTR	pszName;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_MEDIA_ARTIST,  2 },
						{ FT_MEDIA_ALBUM,   2 },
						{ FT_MEDIA_TITLE,   2 },
						{ FT_MEDIA_LENGTH,  2 },
						{ FT_MEDIA_BITRATE, 3 },
						{ FT_MEDIA_CODEC,   2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->tag.type == _aMetaTags[t].nType && !strcasecmp(pTag->tag.tagname, _aMetaTags[t].pszName))
						{
							// skip string tags with empty string values
							if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							// skip "length" tags with "0: 0" values
							if (!strcasecmp(pTag->tag.tagname, FT_MEDIA_LENGTH) && (!strcmp(pTag->tag.stringvalue, "0: 0") || !strcmp(pTag->tag.stringvalue, "0:0")))
								break;

							// skip "bitrate" tags with '0' values
							if (!strcasecmp(pTag->tag.tagname, FT_MEDIA_BITRATE) && pTag->tag.intvalue == 0)
								break;

							printf("CPartFile::CPartFile(CSearchFile*): added tag %s\n", unicode2char(pTag->GetFullInfo()));
							CTag* newtag = new CTag(pTag->tag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}
				else if (pTag->tag.specialtag != 0 && pTag->tag.tagname == NULL && (pTag->tag.type == 2 || pTag->tag.type == 3))
				{
					static const struct
					{
						uint8	nID;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_FILETYPE,		2 },
						{ FT_FILEFORMAT,	2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->tag.type == _aMetaTags[t].nType && pTag->tag.specialtag == _aMetaTags[t].nID)
						{
							// skip string tags with empty string values
							if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							printf("CPartFile::CPartFile(CSearchFile*): added tag %s\n", unicode2char(pTag->GetFullInfo()));
							CTag* newtag = new CTag(pTag->tag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}

				if (!bTagAdded)
					printf("CPartFile::CPartFile(CSearchFile*): ignored tag %s\n", unicode2char(pTag->GetFullInfo()));
			}
		}
	}
	CreatePartFile();
}

CPartFile::CPartFile(const wxString& edonkeylink)
{
	CED2KLink* pLink = 0;
	try {
		pLink = CED2KLink::CreateLinkFromUrl(unicode2char(edonkeylink));
		wxASSERT(pLink != 0);
		CED2KFileLink* pFileLink = pLink->GetFileLink();
		if (pFileLink==0) {
			throw wxString(wxT("Not a file link"));
		}
		InitializeFromLink(pFileLink);
	} catch (wxString error) {
		wxString strBuffer =  _("This ed2k link is invalid (") +  error + wxT(")");
		AddLogLineM(true, _("Invalid link: ") + strBuffer);
		SetPartFileStatus(PS_ERROR);
	}
	delete pLink;
}

void
CPartFile::InitializeFromLink(CED2KFileLink* fileLink)
{
	Init();
	try {
		m_strFileName = fileLink->GetName();
		SetFileSize(fileLink->GetSize());
		m_abyFileHash = fileLink->GetHashKey();
		if (!theApp.downloadqueue->IsFileExisting(m_abyFileHash)) {
			CreatePartFile();
		} else {
			SetPartFileStatus(PS_ERROR);
		}
	} catch(wxString error) {
		wxString strBuffer =  _("This ed2k link is invalid (") + error + wxT(")");
		AddLogLineM(true, _("Invalid link: ") + strBuffer);
		SetPartFileStatus(PS_ERROR);
	}
}

CPartFile::CPartFile(CED2KFileLink* fileLink)
{
	InitializeFromLink(fileLink);
}

void CPartFile::Init()
{
	m_nLastBufferFlushTime = 0;

	newdate = true;
	lastsearchtime = 0;
	lastpurgetime = ::GetTickCount();
	paused = false;
	stopped = false;
	SetPartFileStatus(PS_EMPTY);
	insufficient = false;
	
	//m_bCompletionError = false; // added
	
	transfered = 0;
	m_iLastPausePurge = time(NULL);
	
	if(theApp.glob_prefs->GetNewAutoDown()) {
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	} else {
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	srcarevisible = false;
	
	memset(m_anStates,0,sizeof(m_anStates));
	
	transferingsrc = 0; // new
	
	kBpsDown = 0.0;
	
	hashsetneeded = true;
	count = 0;
	percentcompleted = 0;
	completedsize=0;
	m_bPreviewing = false;
	lastseencomplete = 0;
	availablePartsCount=0;
	m_ClientSrcAnswered = 0;
	m_LastNoNeededCheck = 0;
	m_iRate = 0;
	m_nTotalBufferData = 0;
	m_nLastBufferFlushTime = 0;
	m_bPercentUpdated = false;
	m_bRecoveringArchive = false;
	m_iGainDueToCompression = 0;
	m_iLostDueToCorruption = 0;
	m_iTotalPacketsSavedDueToICH = 0;
	m_nSavedReduceDownload = 0; // new
	hasRating = false;
	hasComment = false; 
	hasBadRating = false;
	m_lastdatetimecheck = 0;
	m_category = 0;
	m_lastRefreshedDLDisplay = 0;
	m_is_A4AF_auto = false;
	m_bShowOnlyDownloading = false;
	m_bLocalSrcReqQueued = false;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	
	// Kry - added to cache the source count
	CleanCount = 0;
	IsCountDirty = true;
	
	// Sources dropping
	m_LastSourceDropTime = 0;
	
}

CPartFile::~CPartFile()
{
	
	// Barry - Ensure all buffered data is written

	// Kry - WTF? 
	// eMule had same problem with lseek error ... and override with a simple 
	// check for INVALID_HANDLE_VALUE (that, btw, does not exist on linux)
	// So we just guess is < 0 on error and > 2 if ok (0 stdin, 1 stdout, 2 stderr)
	// But, where does this wrong handle comes from?
	
	if (m_hpartfile.IsOpened() && (m_hpartfile.fd() > 2)) { 
			FlushBuffer();
	}
	
	if (m_hpartfile.IsOpened() && (m_hpartfile.fd() > 2)) {
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();			
	}

	m_SrcpartFrequency.Clear();
	
	POSITION pos;
	for (pos = gaplist.GetHeadPosition();pos != 0;) {
		delete gaplist.GetNext(pos);
	}
	pos = m_BufferedData_list.GetHeadPosition();
	while (pos){
		PartFileBufferedData *item = m_BufferedData_list.GetNext(pos);
		delete[] item->data;
		delete item;
	}	
}

void CPartFile::CreatePartFile()
{
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	do { 
		i++; 
		m_partmetfilename = wxString::Format(wxT("%03i.part.met"), i);
		m_fullname = theApp.glob_prefs->GetTempDir() + wxT("/") + m_partmetfilename;
	} while (wxFileName::FileExists(m_fullname));
	
	wxString strPartName = m_partmetfilename.Left( m_partmetfilename.Length() - 4);
	taglist.Add( new CTag(FT_PARTFILENAME, strPartName ) );
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = m_nFileSize - 1;
	
	gaplist.AddTail(gap);
	
	wxString strPartPath = m_fullname.Left( m_fullname.Length() - 4);
	if ( !m_hpartfile.Create(strPartPath, true) ) {
		AddLogLineM(false,_("ERROR: Failed to create partfile)"));
		SetPartFileStatus(PS_ERROR);
	}
	// jesh.. luotu. nyt se vaan pitää avata uudestaan read-writeen..
	m_hpartfile.Close();
	if(!m_hpartfile.Open(strPartPath,CFile::read_write)) {
		AddLogLineM(false,_("ERROR: Failed to open partfile)"));
		SetPartFileStatus(PS_ERROR);
	}
	
	if (theApp.glob_prefs->GetAllocFullPart()) {
		#warning Code for full file alloc - should be done on thread.
	}
	
	
	hashsetneeded = GetED2KPartHashCount();
	
	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(GetPartCount());
	
	m_SrcpartFrequency.Insert(/*Item*/0, /*pos*/0, GetPartCount());
	
	paused = false;
	SavePartFile(true);
}

uint8 CPartFile::LoadPartFile(wxString in_directory, wxString filename, bool getsizeonly)
{
	#warning getsizeonly is ignored because we do not import yet
	
	bool isnewstyle;
	uint8 version,partmettype=PMT_UNKNOWN;
	
	std::map<uint16, Gap_Struct*> gap_map; // Slugfiller
	transfered = 0;
	
	m_partmetfilename = filename;
	m_strFilePath = in_directory;
	m_fullname = m_strFilePath + wxT("/") + m_partmetfilename;
	
	CSafeFile metFile;
	bool load_from_backup = false;
	// readfile data form part.met file
	if (!metFile.Open(m_fullname,CFile::read)) {
		AddLogLineM(false, _("Error: Failed to open part.met file! ") + m_partmetfilename + wxT("==>") + m_strFileName);
		load_from_backup = true;
	} else {
		if (!(metFile.Length()>0)) {
			AddLogLineM(false, _("Error: part.met file is 0 size! ") + m_partmetfilename + wxT("==>") + m_strFileName);
			metFile.Close();
			load_from_backup = true;
		}
	}

	if (load_from_backup) {
		AddLogLineM(false, _("Trying backup of met file on ") + m_partmetfilename + PARTMET_BAK_EXT);
		wxString BackupFile;
		BackupFile = m_fullname + PARTMET_BAK_EXT;
		if (!metFile.Open(BackupFile)) {
			AddLogLineM(false, _("Error: Failed to load backup file. Search http://forum.amule.org for .part.met recovery solutions"));				
			return false;
		} else {
			if (!(metFile.Length()>0)) {
				AddLogLineM(false, _("Error: part.met file is 0 size! ") + m_partmetfilename + wxT("==>") + m_strFileName);
				metFile.Close();
				return false;
			}
		}
	}
	
	try {
		metFile.Read(&version,1);
		if (version != PARTFILE_VERSION  && version!= PARTFILE_SPLITTEDVERSION ){
			metFile.Close();
			AddLogLineM(false, wxString::Format(_("Error: Invalid part.met fileversion! (%s => %s)"), m_partmetfilename.c_str(), m_strFileName.c_str()));
			return false;
		}

		isnewstyle=(version== PARTFILE_SPLITTEDVERSION);
		partmettype= isnewstyle?PMT_SPLITTED:PMT_DEFAULTOLD;
		if (!isnewstyle) {
			uint8 test[4];
			metFile.Seek(24, wxFromStart);
			metFile.Read(&test[0],1);
			metFile.Read(&test[1],1);
			metFile.Read(&test[2],1);
			metFile.Read(&test[3],1);
		
			metFile.Seek(1, wxFromStart);
			if (test[0]==0 && test[1]==0 && test[2]==2 && test[3]==1) {
				isnewstyle=true;	// edonkeys so called "old part style"
				partmettype=PMT_NEWOLD;
			}
		}
		if (isnewstyle) {
			uint32 temp;
			metFile.Read(&temp,4);
			ENDIAN_SWAP_I_32(temp);
	
			if (temp==0) {	// 0.48 partmets - different again
				LoadHashsetFromFile(&metFile, false);
			} else {
				uchar gethash[16];
				metFile.Seek(2, wxFromStart);
				LoadDateFromFile(&metFile);
					metFile.Read(gethash, 16);
				m_abyFileHash = gethash;
			}

		} else {
			LoadDateFromFile(&metFile);
			LoadHashsetFromFile(&metFile, false);
		}	

		uint32 tagcount = 0;	
		metFile.Read(&tagcount,4);
		ENDIAN_SWAP_I_32(tagcount);

		for (uint32 j = 0; j < tagcount;j++) {
			CTag* newtag = new CTag(metFile);
			if (!getsizeonly || (getsizeonly && (newtag->tag.specialtag==FT_FILESIZE || newtag->tag.specialtag==FT_FILENAME))) {
				switch(newtag->tag.specialtag) {
					case FT_FILENAME: {
						if(newtag->tag.stringvalue == NULL) {
							AddLogLineM(true, wxString::Format(_("Error: %s (%s) is corrupt"), m_partmetfilename.c_str(), m_strFileName.c_str()));
							delete newtag;
							return false;
						}
						printf(" - filename %s - ",newtag->tag.stringvalue);
						SetFileName(char2unicode(newtag->tag.stringvalue));
						delete newtag;
						break;
					}
					case FT_LASTSEENCOMPLETE: {
						if (newtag->tag.type == 3) {
						lastseencomplete = newtag->tag.intvalue;					
						}
						delete newtag;
						break;
					}
					case FT_FILESIZE: {
						SetFileSize(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					case FT_TRANSFERED: {
						transfered = newtag->tag.intvalue;
					delete newtag;
						break;
					}
					case FT_FILETYPE:{
						#warning needs setfiletype string
						//SetFileType(newtag->tag.stringvalue);
						delete newtag;
						break;
					}					
					case FT_CATEGORY: {
						m_category = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_OLDDLPRIORITY:
					case FT_DLPRIORITY: {
						if (!isnewstyle){
							m_iDownPriority = newtag->tag.intvalue;
							if( m_iDownPriority == PR_AUTO ){
								m_iDownPriority = PR_HIGH;
								SetAutoDownPriority(true);
							}
							else{
								if (m_iDownPriority != PR_LOW && m_iDownPriority != PR_NORMAL && m_iDownPriority != PR_HIGH)
									m_iDownPriority = PR_NORMAL;
								SetAutoDownPriority(false);
							}
						}
						delete newtag;
						break;
					}
					case FT_STATUS: {
						paused = newtag->tag.intvalue;
						stopped = paused;
						delete newtag;
						break;
						}
					case FT_OLDULPRIORITY:
					case FT_ULPRIORITY: {			
						if (!isnewstyle){
							SetUpPriority(newtag->tag.intvalue, false);
							if( GetUpPriority() == PR_AUTO ){
								SetUpPriority(PR_HIGH, false);
								SetAutoUpPriority(true);
							}
							else
								SetAutoUpPriority(false);
						}					
						delete newtag;
						break;
					}				
					case FT_KADLASTPUBLISHSRC:{
						//SetLastPublishTimeKadSrc(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					// old tags: as long as they are not needed, take the chance to purge them
					case FT_PERMISSIONS:
						delete newtag;
						break;
					case FT_KADLASTPUBLISHKEY:
						delete newtag;
						break;					
					default: {
						// Start Changes by Slugfiller for better exception handling
						if ((!newtag->tag.specialtag) &&
						(newtag->tag.tagname[0] == FT_GAPSTART ||
						newtag->tag.tagname[0] == FT_GAPEND)) {
							Gap_Struct* gap = NULL;
							uint16 gapkey = atoi(&newtag->tag.tagname[1]);

							if ( gap_map.find( gapkey ) == gap_map.end() ) {
								gap = new Gap_Struct;
								gap_map[gapkey] = gap;
								gap->start = (uint32)-1;
								gap->end = (uint32)-1;
							} else {
								gap = gap_map[ gapkey ];
							}
							if (newtag->tag.tagname[0] == FT_GAPSTART) {
								gap->start = newtag->tag.intvalue;
							}
							if (newtag->tag.tagname[0] == FT_GAPEND) {
								gap->end = newtag->tag.intvalue-1;
							}
							delete newtag;
							// End Changes by Slugfiller for better exception handling
						} else {
							taglist.Add(newtag);
						}
					}
				}
			} else {
				delete newtag;
			}
		}
		
		// load the hashsets from the hybridstylepartmet
		if (isnewstyle && !getsizeonly && (metFile.GetPosition()<metFile.Length()) ) {
			int8 temp;
			metFile.Read(&temp,1);
			
			uint16 parts=GetPartCount();	// assuming we will get all hashsets
			
			for (uint16 i = 0; i < parts && (metFile.GetPosition()+16<metFile.Length()); i++){
				CMD4Hash cur_hash;
				metFile.Read(cur_hash, 16);
				hashlist.Add(cur_hash);
			}

			CMD4Hash checkhash;
			if (!hashlist.IsEmpty()){
				uchar* buffer = new uchar[hashlist.GetCount()*16];
				for (size_t i = 0; i < hashlist.GetCount(); i++)
					md4cpy(buffer+(i*16), hashlist[i]);
				CreateHashFromString(buffer, hashlist.GetCount()*16, checkhash);
				delete[] buffer;
			}
			bool flag=false;
			if (m_abyFileHash == checkhash)
				flag=true;
			else{
				/*
				for (size_t i = 0; i < hashlist.GetCount(); i++)
					delete[] hashlist[i];
				*/
				hashlist.Clear();
				flag=false;
			}
		}			
			
		metFile.Close();
			
	} catch (CInvalidPacket e) {

		if (metFile.Eof()) {
			AddLogLineM(true, wxString::Format(_("Error: %s (%s) is corrupt, unable to load file"), m_partmetfilename.c_str(), GetFileName().c_str()));
		} else {
			// This error message makes no sense...
// 			AddLogLineM(true, wxString::Format(_("Unexpected file error while reading server.met: %s, unable to load serverlist"), m_partmetfilename.c_str(), GetFileName().c_str(), m_fullname.c_str()));
		}
		printf(" - Caught an error - ");
		if (metFile.IsOpened()) {
			metFile.Close();		
		}
		return false;
	}

	if (getsizeonly) {
		return partmettype;
	}
	// Now to flush the map into the list (Slugfiller)
	std::map<uint16, Gap_Struct*>::iterator it = gap_map.begin();
	for ( ; it != gap_map.end(); ++it ) {
		Gap_Struct* gap = it->second;
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (((int)gap->start) != -1 && ((int)gap->end) != -1 && gap->start <= gap->end && gap->start < m_nFileSize){
			if (gap->end >= m_nFileSize) {
				gap->end = m_nFileSize-1; // Clipping
			}
			AddGap(gap->start, gap->end); // All tags accounted for, use safe adding
		}
		delete gap;
		// SLUGFILLER: SafeHash
	}

	//check if this is a backup
	if ( m_fullname.Right(7).MakeLower() == wxT(".backup" )) {
		m_fullname = m_fullname.Left( m_fullname.Length() - 7 );
	}

	// open permanent handle
	wxString strSearchPath = m_fullname.Left( m_fullname.Length() - 4 );
	if ( !m_hpartfile.Open(strSearchPath, CFile::read_write)) {
		AddLogLineM(false, wxString::Format(_("Failed to open %s (%s)"), m_fullname.c_str(), m_strFileName.c_str()));
		return false;
	}

	// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
	if ((uint64)m_hpartfile.GetLength() < m_nFileSize)
		AddGap(m_hpartfile.GetLength(), m_nFileSize-1);
	// Goes both ways - Partfile should never be too large
	if ((uint64)m_hpartfile.GetLength() > m_nFileSize){
		printf("Partfile \"%s\" is too large! Truncating %llu bytes.\n", unicode2char(GetFileName()), ((ULONGLONG)m_hpartfile.GetLength() - m_nFileSize));
		m_hpartfile.SetLength(m_nFileSize);
	}
	// SLUGFILLER: SafeHash

	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(GetPartCount());
	
	m_SrcpartFrequency.Insert(/*Item*/0, /*pos*/0, GetPartCount());
	
	SetPartFileStatus(PS_EMPTY);
	
	// check hashcount, file status etc
	if (GetHashCount() != GetED2KPartHashCount()){	
		hashsetneeded = true;
		return true;
	} else {
		hashsetneeded = false;
		for (size_t i = 0; i < hashlist.GetCount(); i++) {
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
				SetPartFileStatus(PS_READY);
			}
		}
	}
	
	if (gaplist.IsEmpty()) { // is this file complete already?
		CompleteFile(false);
		return true;
	}

	if (!isnewstyle) { // not for importing	
		// check date of .part file - if its wrong, rehash file
		//CFileStatus filestatus;
		//m_hpartfile.GetStatus(filestatus);
		//struct stat statbuf;
		//fstat(m_hpartfile.fd(),&statbuf);
		//if ((time_t)date != (time_t)statbuf.st_mtime) {

		time_t file_date = wxFileModificationTime(m_fullname);
		if ( (((time_t)date) < (time_t)(file_date - 10)) || (((time_t)date) > (time_t)(file_date + 10))) {
			AddLogLineM(false, wxString::Format(_("Warning: %s might be corrupted"), m_fullname.c_str(), m_strFileName.c_str()));
			// rehash
			SetPartFileStatus(PS_WAITINGFORHASH);
			//CAddFileThread::AddFile(directory, searchpath, this);
			
			wxString strPartFileName = m_partmetfilename.Left( m_partmetfilename.Length() - 4 );
			CAddFileThread::AddFile(m_strFilePath, strPartFileName, this);
		}
	}

	UpdateCompletedInfos();
	if (completedsize > transfered) {
		m_iGainDueToCompression = completedsize - transfered;
	} else if (completedsize != transfered) {
		m_iLostDueToCorruption = transfered - completedsize;
	}
	
	return true;
}

bool CPartFile::SavePartFile(bool Initial)
{
	switch (status) {
		case PS_WAITINGFORHASH:
		case PS_HASHING:
		case PS_COMPLETE:
			return false;
	}
	
	/* Don't write anything to disk if less than 5000 bytes of free space is left. */
	wxLongLong total = 0, free = 0;
	if (wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free) && free < 5000) {
		return false;
	}
	
	CFile file;
	try {
		if ( !wxFileExists( m_fullname.Left(m_fullname.Length() - 4) ) ) {
			throw wxString(wxT(".part file not found"));
		}
		
		uint32 lsc = lastseencomplete;

		if (!Initial) {
			BackupFile(m_fullname, wxT(".backup"));
			wxRemoveFile(m_fullname);
		}
		
		file.Open(m_fullname,CFile::write);
		if (!file.IsOpened()) {
			throw wxString(wxT("Failed to open part.met file"));
		}

		// version
		uint8 version = PARTFILE_VERSION;
		file.Write(&version,1);
		
		date = ENDIAN_SWAP_32(wxFileModificationTime(m_fullname));
		file.Write(&date,4);
		// hash
		file.Write(m_abyFileHash,16);
		uint16 parts = ENDIAN_SWAP_16(hashlist.GetCount());
		file.Write(&parts,2);
		parts = hashlist.GetCount();
		for (int x = 0; x != parts; x++) {
			file.Write(hashlist[x],16);
		}
		// tags
		uint32 tagcount = ENDIAN_SWAP_32(taglist.GetCount()+10+(gaplist.GetCount()*2));
		file.Write(&tagcount,4);
		tagcount = taglist.GetCount()+10+(gaplist.GetCount()*2);

		CTag(FT_FILENAME,m_strFileName).WriteTagToFile(&file);	// 1
		CTag(FT_FILESIZE,m_nFileSize).WriteTagToFile(&file);	// 2
		CTag(FT_TRANSFERED,transfered).WriteTagToFile(&file);	// 3
		CTag(FT_STATUS,(paused)? 1:0).WriteTagToFile(&file);	// 4

		CTag* prioritytag;
		uint8 autoprio = PR_AUTO;
		if(IsAutoDownPriority()) {
			prioritytag = new CTag(FT_DLPRIORITY,autoprio);
		} else {
			prioritytag = new CTag(FT_DLPRIORITY,m_iDownPriority);
		}
		prioritytag->WriteTagToFile(&file);			// 5
		delete prioritytag;
		if(IsAutoDownPriority()) {
			prioritytag = new CTag(FT_OLDDLPRIORITY,autoprio);
		} else {
			prioritytag = new CTag(FT_OLDDLPRIORITY,m_iDownPriority);
		}
		prioritytag->WriteTagToFile(&file);                      // 6
		delete prioritytag;
		
		CTag* lsctag = new CTag(FT_LASTSEENCOMPLETE,lsc);
		lsctag->WriteTagToFile(&file);				// 7
		delete lsctag;

		CTag* ulprioritytag;
		if(IsAutoUpPriority()) {
			ulprioritytag = new CTag(FT_ULPRIORITY,autoprio);
		} else {
			ulprioritytag = new CTag(FT_ULPRIORITY,GetUpPriority());
		}
		ulprioritytag->WriteTagToFile(&file);			// 8
		delete ulprioritytag;
		
		if(IsAutoUpPriority()) {
			ulprioritytag = new CTag(FT_OLDULPRIORITY,autoprio);
		} else {
			ulprioritytag = new CTag(FT_OLDULPRIORITY,GetUpPriority());
		}
		ulprioritytag->WriteTagToFile(&file);			// 9
		delete ulprioritytag;
		
		// Madcat - Category setting.
		CTag* categorytab = new CTag(FT_CATEGORY,m_category);
		categorytab->WriteTagToFile(&file);			// 10
		delete categorytab;

		for (uint32 j = 0; j != (uint32)taglist.GetCount();j++) {
			taglist[j]->WriteTagToFile(&file);
		}
		// gaps
		char* namebuffer = new char[10];
		char* number = &namebuffer[1];
		uint16 i_pos = 0;
		for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)) {
			// itoa(i_pos,number,10);
			sprintf(number,"%d",i_pos);
			namebuffer[0] = FT_GAPSTART;
			CTag* gapstarttag = new CTag(namebuffer,gaplist.GetAt(pos)->start);
			gapstarttag->WriteTagToFile(&file);
			// gap start = first missing byte but gap ends = first non-missing byte
			// in edonkey but I think its easier to user the real limits
			namebuffer[0] = FT_GAPEND;
			CTag* gapendtag = new CTag(namebuffer,(gaplist.GetAt(pos)->end)+1);
			gapendtag->WriteTagToFile(&file);
			delete gapstarttag;
			delete gapendtag;
			i_pos++;
		}
		delete[] namebuffer;
		
		if ( file.Error() ) {
			throw wxString(wxT("Unexpected write error"));
		}

	} catch(wxString error) {
		if (file.IsOpened()) {
			file.Close();
		}
		
		AddLogLineM(false, wxString::Format(_("ERROR while saving partfile: %s (%s => %s)"), error.c_str(), m_partmetfilename.c_str(), m_strFileName.c_str()));
		return false;
	} catch (...) {
		if (file.IsOpened()) {
			file.Close();
		}	
		
		printf("Uncatched exception on CPartFile::SavePartFile!!!\n"); 
	}
	
	file.Close();

	//file.Flush();
	
	if (!Initial) {
		wxRemoveFile(m_fullname + wxT(".backup"));
	}
	
	// Kry -don't backup if it's 0 size but raise a warning!!!
	CFile newpartmet;
	if (newpartmet.Open(m_fullname)!=TRUE) {
		wxMessageBox(wxString(_("Unable to open ")) + m_fullname + _("file - using ") + PARTMET_BAK_EXT + _(" file.\n"));
		FS_wxCopyFile(m_fullname + PARTMET_BAK_EXT, m_fullname);
	} else {
		if (newpartmet.Length()>0) {			
			// not error, just backup
			newpartmet.Close();
			BackupFile(m_fullname, PARTMET_BAK_EXT);
		} else {
			newpartmet.Close();
			wxMessageBox(m_fullname + _("file is 0 size somehow - using ") + PARTMET_BAK_EXT + _(" file.\n"));
			FS_wxCopyFile(m_fullname + PARTMET_BAK_EXT,m_fullname);
		}
	}
	
	return true;
}


void CPartFile::SaveSourceSeeds() {
	// Kry - Sources seeds
	// Copyright (c) Angel Vidal (Kry) 2004
	// Based on a Feature request, this saves the last 5 sources of the file,
	// giving a 'seed' for the next run.
	// We save the last sources because:
	// 1 - They could be the hardest to get
	// 2 - They will more probably be available
	// However, if we have downloading sources, they have preference because
	// we probably have more credits on them.
	// Anyway, source exchange will get us the rest of the sources
	// This feature is currently used only on rare files (< 20 sources)
	// 
	
	if (GetSourceCount()>20) {
		return;	
	}	
	
	CTypedPtrList<CPtrList, CUpDownClient*>	source_seeds;
	int n_sources = 0;
	
	std::list<CUpDownClient *>::iterator it = m_downloadingSourcesList.begin();
	for( ; it != m_downloadingSourcesList.end() && n_sources < 5; it++) {
		CUpDownClient *cur_src = *it;
		if (cur_src->HasLowID()) {
			continue;
		} else {
			source_seeds.AddTail(cur_src);
		}
		n_sources++;
	}

	if (n_sources < 5) {
		// Not enought downloading sources to fill the list, going to sources list	
		if (GetSourceCount() > 0) {
			SourceSet::reverse_iterator it = m_SrcList.rbegin();
			for ( ; ((it != m_SrcList.rend()) && (n_sources<5)); ++it) {
				CUpDownClient* cur_src = *it;
				if (cur_src->HasLowID()) {
					continue;
				} else {
					source_seeds.AddTail(cur_src);
				}
				n_sources++;
			}
		}
	}	
	
	// Write the file
	if (!n_sources) {
		return;
	} 
	

	CFile file;
	
	file.Create(m_fullname + wxT(".seeds"), true);
	
	if (!file.IsOpened()) {
		AddLogLineM(false, wxString::Format(_("Failed to save part.met.seeds file for %s"), m_fullname.c_str()));
	}	

	uint8 src_count = source_seeds.GetCount();
	file.Write(&src_count,1);
	
	for (POSITION pos = source_seeds.GetHeadPosition(); pos  != NULL;) {
		CUpDownClient* cur_src = source_seeds.GetNext(pos);		
		uint32 dwID = cur_src->GetUserID();
		uint16 nPort = cur_src->GetUserPort();
		//uint32 dwServerIP = cur_src->GetServerIP();
		//uint16 nServerPort =cur_src->GetServerPort();
		file.Write(&dwID,4);
		file.Write(&nPort,2);
		//file.Write(&dwServerIP,4);
		//file.Write(&nServerPort,2);
	}	
	file.Flush();
	file.Close();

	AddLogLineM(false, wxString::Format(_("Saved %i sources seeds for partfile: %s (%s)"), n_sources, m_fullname.c_str(), m_strFileName.c_str()));
	
}	


void CPartFile::LoadSourceSeeds() {
	
	CFile file;
	CSafeMemFile sources_data;
	
	if (!wxFileName::FileExists(m_fullname + wxT(".seeds"))) {
		return;
	} 
	
	file.Open(m_fullname + wxT(".seeds"),CFile::read);

	if (!file.IsOpened()) {
		AddLogLineM(false, wxString::Format(_("Partfile %s (%s) has no seeds file"), m_partmetfilename.c_str(), m_strFileName.c_str()));
		return;
	}	
	
	if (!file.Length()>1) {
		AddLogLineM(false, wxString::Format(_("Partfile %s (%s) has void seeds file"), m_partmetfilename.c_str(), m_strFileName.c_str()));
		return;
	}	
	
	uint8 src_count;
	file.Read(&src_count,1);	
	
	sources_data.WriteUInt16(src_count);

	for (int i=0;i<src_count;i++) {
	
		uint32 dwID;
		uint16 nPort;
		file.Read(&dwID,4);
		file.Read(&nPort,2);
		
		sources_data.WriteUInt32(dwID);
		sources_data.WriteUInt16(nPort);
		sources_data.WriteUInt32(0);
		sources_data.WriteUInt16(0);	
	}
	
	sources_data.Seek(0);
	
	AddClientSources(&sources_data, 1 );
	
	file.Close();	
}		

void CPartFile::PartFileHashFinished(CKnownFile* result)
{

	newdate = true;
	bool errorfound = false;
	if (GetED2KPartHashCount() == 0){
		if (IsComplete(0, m_nFileSize-1)){
			if (result->GetFileHash() != GetFileHash()){
				AddLogLineM(false, wxString::Format(_("Found corrupted part (%i) in 0 parts file %s - FileResultHash |%s| FileHash |%s|"), 1, m_strFileName.c_str(), EncodeBase16(result->GetFileHash(), 16).c_str(), EncodeBase16(GetFileHash(), 16).c_str()));
				AddGap(0, m_nFileSize-1);
				errorfound = true;
			}
		}
	}
	else{
		for (size_t i = 0; i < hashlist.GetCount(); i++){
			// Kry - trel_ar's completed parts check on rehashing.
			// Very nice feature, if a file is completed but .part.met don't belive it,
			// update it.
			
			/*
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
				if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i),GetPartHash(i)))){
					AddLogLineM(false, wxString::Format(_("Found corrupted part (%i) in %i parts file %s - FileResultHash |%s| FileHash |%s|"), i+1, GetED2KPartHashCount(), m_strFileName.c_str(),result->GetPartHash(i),GetPartHash(i)));
//					AddLogLineM(false, wxString::Format(_("Found corrupted part (%i) in %s"), i+1, m_strFileName.c_str()));
					AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			}
			*/
			if (!( i < result->GetHashCount() && (result->GetPartHash(i) == GetPartHash(i)))){
				if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
					CMD4Hash wronghash;
					if ( i < result->GetHashCount() )
						wronghash = result->GetPartHash(i);
				
					AddLogLineM(false, wxString::Format(_("Found corrupted part (%i) in %i parts file %s - FileResultHash |%s| FileHash |%s|"), i + 1, GetED2KPartHashCount(), m_strFileName.c_str(), wronghash.Encode().c_str(), GetPartHash(i).Encode().c_str()));
				
					AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			} else {
				if (!IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
					AddLogLineM(false, wxString::Format(_("Found completed part (%i) in %s"), i+1, m_strFileName.c_str()));
					FillGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					RemoveBlockFromList(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
				}
			}						
		}
	}
	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			CompleteFile(true);
			return;
		}
		else {
			AddLogLineM(false, wxString::Format(_("Finished rehashing %s"), m_strFileName.c_str()));
		}
	}
	else{
		SetStatus(PS_READY);
		SavePartFile();
		return;
	}
	SetStatus(PS_READY);
	SavePartFile();
	theApp.sharedfiles->SafeAddKFile(this);		
}

void CPartFile::AddGap(uint32 start, uint32 end)
{
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// this gap is inside the new gap - delete
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the new gap - extend limit and delete
			end = cur_gap->end;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the new gap - extend limit and delete
			start = cur_gap->start;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (start >= cur_gap->start && end <= cur_gap->end){
			// new gap is already inside this gap - return
			return;
		}
	}
	Gap_Struct* new_gap = new Gap_Struct;
	new_gap->start = start;
	new_gap->end = end;
	gaplist.AddTail(new_gap);
	UpdateDisplayedInfo();
	newdate = true;
}

bool CPartFile::IsComplete(uint32 start, uint32 end)
{
	if (end >= m_nFileSize) {
		end = m_nFileSize-1;
	}
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0; ) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if ((cur_gap->start >= start && cur_gap->end <= end)||(cur_gap->start >= start 
		&& cur_gap->start <= end)||(cur_gap->end <= end && cur_gap->end >= start)
		||(start >= cur_gap->start && end <= cur_gap->end)) {
			return false;	
		}
	}
	return true;
}

bool CPartFile::IsPureGap(uint32 start, uint32 end)
{
	if (end >= m_nFileSize) {
		end = m_nFileSize-1;
	}
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0; ) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (start >= cur_gap->start  && end <= cur_gap->end) {
			return true;
		}
	}
	return false;
}

bool CPartFile::IsAlreadyRequested(uint32 start, uint32 end)
{
	for (POSITION pos =  requestedblocks_list.GetHeadPosition();pos != 0; ) {
		Requested_Block_Struct* cur_block =  requestedblocks_list.GetNext(pos);
		// if (cur_block->StartOffset == start && cur_block->EndOffset == end)
		/* eMule 0.30c manage the problem like that, i give it a try ... (Creteil) */
		if ((start <= cur_block->EndOffset) && (end >= cur_block->StartOffset)) {
			return true;
		}
	}
	return false;
}

bool CPartFile::GetNextEmptyBlockInPart(uint16 partNumber, Requested_Block_Struct *result)
{
	Gap_Struct *firstGap;
	Gap_Struct *currentGap;
	uint32 end;
	uint32 blockLimit;

	// Find start of this part
	uint32 partStart = (PARTSIZE * partNumber);
	uint32 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint32 partEnd = (PARTSIZE * (partNumber + 1)) - 1;
	if (partEnd >= GetFileSize()) {
		partEnd = GetFileSize() - 1;
	}
	// Loop until find a suitable gap and return true, or no more gaps and return false
	while (true) {
		firstGap = NULL;

		// Find the first gap from the start position
		for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; ) {
			currentGap = gaplist.GetNext(pos);
			// Want gaps that overlap start<->partEnd
			if ((currentGap->start <= partEnd) && (currentGap->end >= start)) {
				// Is this the first gap?
				if ((firstGap == NULL) || (currentGap->start < firstGap->start)) {
					firstGap = currentGap;
				}
			}
		}

		// If no gaps after start, exit
		if (firstGap == NULL) {
			return false;
		}
		// Update start position if gap starts after current pos
		if (start < firstGap->start) {
			start = firstGap->start;
		}
		// If this is not within part, exit
		if (start > partEnd) {
			return false;
		}
		// Find end, keeping within the max block size and the part limit
		end = firstGap->end;
		blockLimit = partStart + (BLOCKSIZE * (((start - partStart) / BLOCKSIZE) + 1)) - 1;
		if (end > blockLimit) {
			end = blockLimit;
		}
		if (end > partEnd) {
			end = partEnd;
		}
		// If this gap has not already been requested, we have found a valid entry
		if (!IsAlreadyRequested(start, end)) {
			// Was this block to be returned
			if (result != NULL) {
				result->StartOffset = start;
				result->EndOffset = end;
				md4cpy(result->FileID, GetFileHash());
				result->transferred = 0;
			}
			return true;
		} else {
			// Reposition to end of that gap
			start = end + 1;
		}
		// If tried all gaps then break out of the loop
		if (end == partEnd) {
			break;
		}
	}
	// No suitable gap found
	return false;
}

void CPartFile::FillGap(uint32 start, uint32 end)
{
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// our part fills this gap completly
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		} else if (start >= cur_gap->start && end <= cur_gap->end) {
			uint32 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			gaplist.InsertAfter(pos1,cur_gap);
			break;
		}
	}
	UpdateCompletedInfos();
	UpdateDisplayedInfo();
	newdate = true;
}

void CPartFile::UpdateCompletedInfos()
{
   	uint32 allgaps = 0; 
	for (POSITION pos = gaplist.GetHeadPosition(); pos != 0;) {
		POSITION prev = pos;
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if ((cur_gap->end > m_nFileSize) || (cur_gap->start >= m_nFileSize)) {
			gaplist.RemoveAt(prev);
		} else {
			allgaps += cur_gap->end - cur_gap->start + 1;
		}
	}
	if (gaplist.GetCount() || requestedblocks_list.GetCount()) {
		percentcompleted = (1.0f-(float)allgaps/m_nFileSize) * 100;
		completedsize = m_nFileSize - allgaps;
	} else {
		percentcompleted = 100;
		completedsize = m_nFileSize;
	}
}

#if 0
#warning work in progress
void CPartFile::DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing = RGB(255, 0, 0);

	if(bFlat) { 
		crProgress = RGB(0, 150, 0);
		crHave = RGB(0, 0, 0);
		crPending = RGB(255,208,0);
	} else { 
		crProgress = RGB(0, 224, 0);
		crHave = RGB(104, 104, 104);
		crPending = RGB(255, 208, 0);
	} 

	s_ChunkBar.SetFileSize(GetFileSize()); 
	s_ChunkBar.SetHeight(rect->bottom - rect->top); 
	s_ChunkBar.SetWidth(rect->right - rect->left); 
	s_ChunkBar.Fill(crMissing); 
	COLORREF color;
	if (!onlygreyrect && !m_SrcpartFrequency.IsEmpty()) { 
		for (int i = 0;i < GetPartCount();i++)
			if(m_SrcpartFrequency[i] > 0 ){
				color = RGB(0, (210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)), 255);
				s_ChunkBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
			}
	}
   	s_ChunkBar.Draw(dc, rect->left, rect->top, bFlat); 
} 
#endif

void CPartFile::DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool bFlat)
{
	static CBarShader s_ChunkBar(16);
	static CBarShader s_LoadBar(PROGRESS_HEIGHT);
	
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing = RGB(255, 0, 0);

	if(bFlat) {
		crProgress = RGB(0, 150, 0);
		crHave = RGB(0, 0, 0);
		crPending = RGB(255,255,100);
	} else {
		crProgress = RGB(0, 224, 0);
		crHave = RGB(104, 104, 104);
		crPending = RGB(255, 208, 0);
	}

	s_ChunkBar.SetHeight(rect.height - rect.y);
	s_ChunkBar.SetWidth(rect.width - rect.x); 
	s_ChunkBar.SetFileSize(m_nFileSize);
	s_ChunkBar.Fill(crHave);
	s_ChunkBar.Set3dDepth( theApp.glob_prefs->Get3DDepth() );

	uint32 allgaps = 0;

	if(status == PS_COMPLETE || status == PS_COMPLETING) {
		s_ChunkBar.FillRange(0, m_nFileSize, crProgress);
		s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 
		percentcompleted = 100; completedsize=m_nFileSize;
		return;
	}

	// red gaps
	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0; ) {
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		allgaps += cur_gap->end - cur_gap->start + 1;
		bool gapdone = false;
		uint32 gapstart = cur_gap->start;
		uint32 gapend = cur_gap->end;
		for (uint32 i = 0; i != GetPartCount(); i++) {
			if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE) {
				// is in this part?
				if (gapend <= (i+1)*PARTSIZE) {
					gapdone = true;
				} else {
					gapend = (i+1)*PARTSIZE; // and next part
				}
				// paint
				COLORREF color;
				if (m_SrcpartFrequency.GetCount() >= (size_t)i && m_SrcpartFrequency[i]) {
					// frequency?
					color = RGB(0,(210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)),255);
				} else {
					color = crMissing;
				}
				s_ChunkBar.FillRange(gapstart, gapend + 1,  color);

				if (gapdone) {
					// finished?
					break;
				} else {
					gapstart = gapend;
					gapend = cur_gap->end;
				}
			}
		}
	}
	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0; ) {
		Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		s_ChunkBar.FillRange(block->StartOffset, block->EndOffset,  crPending);
	}
	s_ChunkBar.Draw(dc, rect.x, rect.y, bFlat); 

	// green progress
	float blockpixel = (float)(rect.width)/((float)m_nFileSize);
	RECT gaprect;
	gaprect.top = rect.y; 
	gaprect.bottom = gaprect.top + 4;
	gaprect.left = rect.x; //->left;

	if(!bFlat) {
		s_LoadBar.Set3dDepth( theApp.glob_prefs->Get3DDepth() );
		s_LoadBar.SetWidth((int) ((m_nFileSize - allgaps) * blockpixel));
		s_LoadBar.Fill(crProgress);
		s_LoadBar.Draw(dc, gaprect.left, gaprect.top, false);
	} else {
		gaprect.right = rect.x + (int)((m_nFileSize - allgaps) * blockpixel);
		dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(crProgress),wxSOLID))); //wxBrush(crProgress));
		dc->DrawRectangle(gaprect.left,gaprect.top,gaprect.right,gaprect.bottom);
		//dc->FillRect(&gaprect, &CBrush(crProgress));
		//draw gray progress only if flat
		gaprect.left = gaprect.right;
		gaprect.right = rect.width;
		dc->SetBrush(*(wxTheBrushList->FindOrCreateBrush(wxColour(224,224,224),wxSOLID))); //wxBrush(crPending));
		dc->DrawRectangle(gaprect.left,gaprect.top,gaprect.right,gaprect.bottom);
		//dc->FillRect(&gaprect, &CBrush(RGB(224,224,224)));
	}
	if ((gaplist.GetCount() || requestedblocks_list.GetCount())) {
		percentcompleted = ((1.0f-(float)allgaps/m_nFileSize)) * 100;
		completedsize = m_nFileSize - allgaps;
	} else {
		percentcompleted = 100;
		completedsize=m_nFileSize;
	}
}

void CPartFile::WritePartStatus(CSafeMemFile* file)
{
	uint16 parts = GetED2KPartCount();
	file->WriteUInt16(parts);
	uint16 done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;i++) {
			if (IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)) {
				towrite |= (1<<i);
			}
			done++;
			if (done == parts) {
				break;
			}
		}
		file->WriteUInt8(towrite);
	}
}

void CPartFile::WriteCompleteSourcesCount(CSafeMemFile* file)
{
	file->WriteUInt16(m_nCompleteSourcesCount);
}

int CPartFile::GetValidSourcesCount()
{
	int counter = 0;
	
	for (SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ){
		CUpDownClient* cur_src = *it;
		uint8 state = cur_src->GetDownloadState();
		
		if ( state != DS_ONQUEUE && state != DS_DOWNLOADING && state != DS_NONEEDEDPARTS ) {
			counter++;
		}
	}
	return counter;
}

uint16 CPartFile::GetNotCurrentSourcesCount()
{
	uint16 counter = 0;

	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ){
		CUpDownClient* cur_src = *it;
		uint8 state = cur_src->GetDownloadState();
	
		if ( state != DS_ONQUEUE && state != DS_DOWNLOADING ) {
			counter++;
		}
	}
	return counter;
}

uint8 CPartFile::GetStatus(bool ignorepause) const
{
	if ((!paused && !insufficient) || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE || ignorepause) {
		return status;
	} else {
		return PS_PAUSED;
	}
}

uint32 CPartFile::Process(uint32 reducedownload/*in percent*/,uint8 m_icounter)
{
	uint16 old_trans;
	DWORD dwCurTick = ::GetTickCount();

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if ((m_nTotalBufferData > theApp.glob_prefs->GetFileBufferSize())  || (dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT))) {
		// Avoid flushing while copying preview file
		if (!m_bPreviewing) {
			FlushBuffer();
		}
	}


	// check if we want new sources from server --> MOVED for 16.40 version
	old_trans=transferingsrc;
	transferingsrc = 0;
	kBpsDown = 0.0;

	if (m_icounter < 10) {
		std::list<CUpDownClient *>::iterator it = m_downloadingSourcesList.begin();
		for( ; it != m_downloadingSourcesList.end(); ) {
			CUpDownClient *cur_src = *it++;
			if(cur_src->GetDownloadState() == DS_DOWNLOADING) {
				wxASSERT( cur_src->GetSocket() );
				if (cur_src->GetSocket()) {
					transferingsrc++;
					float kBpsClient = cur_src->CalculateKBpsDown();
					kBpsDown += kBpsClient;
					if (reducedownload) {
						uint32 limit = (uint32)((float)reducedownload*kBpsClient);
						if(limit<1000 && reducedownload == 200) {
							limit +=1000;
						} else if(limit<1) {
							limit = 1;
						}
						cur_src->SetDownloadLimit(limit);
					} else { // Kry - Is this needed?
						cur_src->DisableDownloadLimit();
					}
				}
			}
		}
	} else {
		CUpDownClient* cur_src;
		for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
			cur_src = *it++;
			uint8 download_state=cur_src->GetDownloadState();
			switch (download_state) {
				case DS_DOWNLOADING: {
					transferingsrc++;
						
					float kBpsClient = cur_src->CalculateKBpsDown();
					kBpsDown += kBpsClient;
					if (reducedownload && download_state == DS_DOWNLOADING) {
						uint32 limit = (uint32)((float)reducedownload*kBpsClient);
						
						if (limit < 1000 && reducedownload == 200) {
							limit += 1000;
						} else if (limit < 1) {
							limit = 1;
						}
						
						cur_src->SetDownloadLimit(limit);
					} else {
						cur_src->DisableDownloadLimit();
					}
					cur_src->SetValidSource(true);
					break;
				}
				case DS_BANNED: {
					break;
				}
				case DS_ERROR: {
					break;
				}
				case DS_LOWTOLOWIP: {
					if( cur_src->HasLowID() && (theApp.serverconnect->GetClientID() < 16777216) ) {
						//If we are almost maxed on sources, slowly remove these client to see if we can find a better source.
						if( ((dwCurTick - lastpurgetime) > 30000) && (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8))) {
							RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break;
						}
						if (theApp.serverconnect->IsLowID()) {
							break;
						}
					} else {
						cur_src->SetDownloadState(DS_ONQUEUE);
					}
				}
				case DS_NONEEDEDPARTS: {
					// we try to purge noneeded source, even without reaching the limit
					if((dwCurTick - lastpurgetime) > 40000) {
						if(!cur_src->SwapToAnotherFile(false , false, false , NULL)) {
							//however we only delete them if reaching the limit
							if (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) {
								RemoveSource(cur_src);
								lastpurgetime = dwCurTick;
								break; //Johnny-B - nothing more to do here (good eye!)
							}
						} else {
							lastpurgetime = dwCurTick;
							break;
						}
					}
					// doubled reasktime for no needed parts - save connections and traffic
					if (!((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME*2)) {
						break;
					}
					// Recheck this client to see if still NNP.. Set to DS_NONE so that we force a TCP reask next time..
					cur_src->SetDownloadState(DS_NONE);						
				}
				case DS_ONQUEUE: {
					cur_src->SetValidSource(true);
					if( cur_src->IsRemoteQueueFull()) {
						cur_src->SetValidSource(false);
						if( ((dwCurTick - lastpurgetime) > 60000) && (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) ){
							RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break; //Johnny-B - nothing more to do here (good eye!)
						}
					}
					//Give up to 1 min for UDP to respond.. If we are within on min on TCP, do not try..
					if (theApp.serverconnect->IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME-20000)) {
						cur_src->UDPReaskForDownload();
					}
				}
				case DS_CONNECTING: 
				case DS_TOOMANYCONNS: 
				case DS_NONE: 
				case DS_WAITCALLBACK: {							
					if (theApp.serverconnect->IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME)) {
						if (!cur_src->AskForDownload()) {
							break; //I left this break here just as a reminder just in case re rearange things..
						}
					}
					break;
				}
				// Kry - this extra case is not processed on 0.42e
				/*
				case DS_CONNECTED: {
					if (download_state == DS_CONNECTED) {
						if( !cur_src->IsConnected()) ){
							cur_src->SetDownloadState(DS_NONE);
							break;
						}
						if (dwCurTick - cur_src->GetEnteredConnectedState() > CONNECTION_TIMEOUT + 20000){
							theApp.downloadqueue->RemoveSource( cur_src );
							break;
						}
					}
				}
				*/					
			}
		}

		/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
		if (IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000))) {
			m_LastNoNeededCheck = dwCurTick;
			for ( SourceSet::iterator it = A4AFsrclist.begin(); it != A4AFsrclist.end(); ) {
				CUpDownClient *cur_source = *it++;
				uint8 download_state=cur_source->GetDownloadState();
				if( download_state != DS_DOWNLOADING
				&& cur_source->GetRequestFile() 
				&& ((!cur_source->GetRequestFile()->IsA4AFAuto()) || download_state == DS_NONEEDEDPARTS))
				{
					cur_source->SwapToAnotherFile(false, false, false, this);
				}
			}
		}
		/* eMule 0.30c implementation, i give it a try (Creteil) END ... */
		// swap No needed partfiles if possible
		/* Sources droping engine. Auto drop allowed type of sources at interval. */
		if (dwCurTick > m_LastSourceDropTime + theApp.glob_prefs->GetAutoDropTimer() * 1000) {
			m_LastSourceDropTime = dwCurTick;
			/* If all three are enabled, use CleanUpSources() function, will save us some CPU. */
			if (theApp.glob_prefs->DropNoNeededSources() && theApp.glob_prefs->DropFullQueueSources() && theApp.glob_prefs->DropHighQueueRankingSources()) {
				//printf("Cleaning up sources.\n");
				CleanUpSources();
			} else {
				/* Then check separately for each of them, and act accordingly. */
				if (theApp.glob_prefs->DropNoNeededSources()) {
					//printf("Dropping No Needed Sources.\n");
					RemoveNoNeededSources();
				}
				if (theApp.glob_prefs->DropFullQueueSources()) {
					//printf("Dropping Full Queue Sources.\n");
					RemoveFullQueueSources();
				}
				if (theApp.glob_prefs->DropHighQueueRankingSources()) {
					//printf("Dropping High Queue Rating Sources.\n");
					RemoveHighQueueRatingSources();
				}
			}
		}
	
		if (((old_trans==0) && (transferingsrc>0)) || ((old_trans>0) && (transferingsrc==0))) {
			SetPartFileStatus(status);
		}
	
		// check if we want new sources from server
		if ( !m_bLocalSrcReqQueued && ((!lastsearchtime) || (dwCurTick - lastsearchtime) > SERVERREASKTIME) && theApp.serverconnect->IsConnected()
		&& theApp.glob_prefs->GetMaxSourcePerFileSoft() > GetSourceCount() && !stopped ) {
			m_bLocalSrcReqQueued = true;
			theApp.downloadqueue->SendLocalSrcRequest(this);
		}
	
		// calculate datarate, set limit etc.
		
	}			

	count++;
	
	// Kry - does the 3 / 30 difference produce too much flickering or CPU?
	if (count >= 30) {
		count = 0;
		UpdateAutoDownPriority();
		UpdateDisplayedInfo();
		if(m_bPercentUpdated == false) {
			UpdateCompletedInfos();
		}
		m_bPercentUpdated = false;
		if (theApp.glob_prefs->ShowCatTabInfos()) {
			Notify_ShowUpdateCatTabTitles();
		}				
	}
	
	return (uint32)(kBpsDown*1024.0);
}

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped)
{
	// MOD Note: Do not change this part - Merkur
	// check first if we are this source
	if (theApp.serverconnect->GetClientID() < 16777216 && theApp.serverconnect->IsConnected()){
		if ((theApp.serverconnect->GetClientID() == userid) && inet_addr(unicode2char(theApp.serverconnect->GetCurrentServer()->GetFullIP()))  == serverip) {
			return false;
		}
	} else if (theApp.serverconnect->GetClientID() == userid) {
#ifdef __DEBUG__
		// It seems this can be used to test two amule's on one PC, using different ports --Aleric.
		if (!theApp.serverconnect->IsLowID() && theApp.glob_prefs->GetPort() != port)
		  return true;
#endif
		return false;
	} else if (userid < 16777216 && !theApp.serverconnect->IsLocalServer(serverip,serverport)) {
		if (pdebug_lowiddropped) {
			(*pdebug_lowiddropped)++;
		}
		return false;
	}
	// MOD Note - end
	return true;
}

void CPartFile::AddSources(CSafeMemFile* sources,uint32 serverip, uint16 serverport)
{
	uint8 count = sources->ReadUInt8();
	uint8 debug_lowiddropped = 0;
	uint8 debug_possiblesources = 0;

	if (stopped) {
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		sources->Seek(count*(4+2), wxFromStart);
		return;
	}

	for (int i = 0;i != count;i++) {
		uint32 userid = sources->ReadUInt32();
		uint16 port   = sources->ReadUInt16();
		
		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (userid >= 16777216) {
			if (theApp.glob_prefs->FilterBadIPs()) {
				if (!IsGoodIP(userid)) { // check for 0-IP, localhost and optionally for LAN addresses
					//AddDebugLogLineM(false, _T("Ignored source (IP=%s) received from server"), inet_ntoa(*(in_addr*)&userid));
					continue;
				}
			}
			if (theApp.ipfilter->IsFiltered(userid)) {
				//AddDebugLogLineM(false, _T("IPfiltered source IP=%s (%s) received from server"), inet_ntoa(*(in_addr*)&userid), theApp.ipfilter->GetLastHit());
				continue;
			}
		}

		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped)) {
			continue;
		}
		if(theApp.glob_prefs->GetMaxSourcePerFile() > GetSourceCount()) {
			debug_possiblesources++;
			CUpDownClient* newsource = new CUpDownClient(port,userid,serverip,serverport,this);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} else {
			// since we may received multiple search source UDP results we have to "consume" all data of that packet
			sources->Seek((count-i)*(4+2), wxFromStart);
			break;
		}
	}
}

void CPartFile::UpdatePartsInfo() {

	if( !IsPartFile() ) {
		CKnownFile::UpdatePartsInfo();
		return;
	}
	
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 
	
	// Reset Part Count and allocate it with 0es
	
	m_SrcpartFrequency.Clear();
	m_SrcpartFrequency.Alloc(partcount);
	
	m_SrcpartFrequency.Insert(/*Item*/(uint16)0, /*pos*/0, partcount);

	ArrayOfUInts16 count;	
	
	if (flag) {
		count.Alloc(GetSourceCount());	
	}
	
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ) {
		CUpDownClient *cur_src = *it;
		for (uint16 i = 0; i < partcount; i++)	{
			if (cur_src->IsPartAvailable(i)) {
				m_SrcpartFrequency[i] +=1;
			}
		}
		if (flag) {
			count.Add(cur_src->GetUpCompleteSourcesCount());
		}
	}

	if( flag ) {
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; i++)	{
			if( !i )	{
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i]) {
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
		}
	
		count.Add(m_nCompleteSourcesCount);
	
		count.Shrink();
	
		int32 n = count.GetCount();
		if (n > 0) {

			// Kry - Native wx functions instead
			count.Sort(Uint16CompareValues);
			
			// calculate range
			int32 i= n >> 1;		// (n / 2)
			int32 j= (n * 3) >> 2;	// (n * 3) / 4
			int32 k= (n * 7) >> 3;	// (n * 7) / 8
			
			//When still a part file, adjust your guesses by 20% to what you see..

			
			if (n < 5) {
				//Not many sources, so just use what you see..
				// welcome to 'plain stupid code'
				// m_nCompleteSourcesCount; 
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			} else if (n < 20) {
				// For low guess and normal guess count
				//	 If we see more sources then the guessed low and normal, use what we see.
				//	 If we see less sources then the guessed low, adjust network accounts for 80%, 
				//  we account for 20% with what we see and make sure we are still above the normal.
				// For high guess
				//  Adjust 80% network and 20% what we see.
				if ( count[i] < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				} else {
					m_nCompleteSourcesCountLo = (uint16)((float)(count[i]*.8)+(float)(m_nCompleteSourcesCount*.2));
				}
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count[j]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;	
				}					
			} else {
				// Many sources
				// ------------
				// For low guess
				//	 Use what we see.
				// For normal guess
				//	 Adjust network accounts for 80%, we account for 20% with what 
				//  we see and make sure we are still above the low.
				// For high guess
				//  Adjust network accounts for 80%, we account for 20% with what 
				//  we see and make sure we are still above the normal.

				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= (uint16)((float)(count[j]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo ) {
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				}
				m_nCompleteSourcesCountHi= (uint16)((float)(count[k]*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
				}
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	UpdateDisplayedInfo();
}	

// Kry - Updated to 0.42e + bugfix
// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]
bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, Requested_Block_Struct** newblocks, uint16* count)
{

	// The purpose of this function is to return a list of blocks (~180KB) to
	// download. To avoid a prematurely stop of the downloading, all blocks that 
	// are requested from the same source must be located within the same 
	// chunk (=> part ~9MB).
	//  
	// The selection of the chunk to download is one of the CRITICAL parts of the 
	// edonkey network. The selection algorithm must insure the best spreading
	// of files.
	//  
	// The selection is based on 4 criteria:
	//  1.  Frequency of the chunk (availability), very rare chunks must be downloaded 
	//      as quickly as possible to become a new available source.
	//  2.  Parts used for preview (first + last chunk), preview or check a 
	//      file (e.g. movie, mp3)
	//  3.  Request state (downloading in process), try to ask each source for another 
	//      chunk. Spread the requests between all sources.
	//  4.  Completion (shortest-to-complete), partially retrieved chunks should be 
	//      completed before starting to download other one.
	//  
	// The frequency criterion defines three zones: very rare (<10%), rare (<50%)
	// and common (>30%). Inside each zone, the criteria have a specific weight, used 
	// to calculate the priority of chunks. The chunk(s) with the highest 
	// priority (highest=0, lowest=0xffff) is/are selected first.
	//  
	//          very rare   (preview)       rare                      common
	//    0% <---- +0 pt ----> 10% <----- +10000 pt -----> 50% <---- +20000 pt ----> 100%
	// 1.  <------- frequency: +25*frequency pt ----------->
	// 2.  <- preview: +1 pt --><-------------- preview: set to 10000 pt ------------->
	// 3.                       <------ request: download in progress +20000 pt ------>
	// 4a. <- completion: 0% +100, 25% +75 .. 100% +0 pt --><-- !req => completion --->
	// 4b.                                                  <--- req => !completion -->
	//  
	// Unrolled, the priority scale is:
	//  
	// 0..xxxx       unrequested and requested very rare chunks
	// 10000..1xxxx  unrequested rare chunks + unrequested preview chunks
	// 20000..2xxxx  unrequested common chunks (priority to the most complete)
	// 30000..3xxxx  requested rare chunks + requested preview chunks
	// 40000..4xxxx  requested common chunks (priority to the least complete)
	//
	// This algorithm usually selects first the rarest chunk(s). However, partially
	// complete chunk(s) that is/are close to completion may overtake the priority 
	// (priority inversion).
	// For the common chuncks, the algorithm tries to spread the dowload between
	// the sources
	//

	// Check input parameters
	if(count == NULL) {
		return false;
	}
	if(sender->GetPartStatus() == NULL) {
		return false;
	}
	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	CList<Chunk> chunksList;
	
	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != *count) {
		// Create a request block stucture if a chunk has been previously selected
		if(sender->m_lastPartAsked != 0xffff) {
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->m_lastPartAsked, pBlock) == true) {
				// Keep a track of all pending requested blocks
				requestedblocks_list.AddTail(pBlock);
				// Update list of blocks to return
				newblocks[newBlockCount++] = pBlock;
				// Skip end of loop (=> CPU load)
				continue;
			} else {
				// All blocks for this chunk have been already requested
				delete pBlock;
				// => Try to select another chunk
				sender->m_lastPartAsked = 0xffff;
			}
		}

		// Check if a new chunk must be selected (e.g. download starting, previous chunk complete)
		if(sender->m_lastPartAsked == 0xffff) {
			// Quantify all chunks (create list of chunks to download) 
			// This is done only one time and only if it is necessary (=> CPU load)
			if(chunksList.IsEmpty() == TRUE) {
				// Indentify the locally missing part(s) that this source has
				for(uint16 i=0; i < partCount; i++) {
					if(sender->IsPartAvailable(i) == true && GetNextEmptyBlockInPart(i, NULL) == true) {
						// Create a new entry for this chunk and add it to the list
						Chunk newEntry;
						newEntry.part = i;
						newEntry.frequency = m_SrcpartFrequency[i];
						chunksList.AddTail(newEntry);
					}
				}

				// Check if any bloks(s) could be downloaded
				if(chunksList.IsEmpty() == TRUE) {
					break; // Exit main loop while()
				}

				// Define the bounds of the three zones (very rare, rare)
				// more depending on available sources
				uint8 modif=10;
				if (GetSourceCount()>800) {
					modif=2;
				} else if (GetSourceCount()>200) {
					modif=5;
				}
				uint16 limit= modif*GetSourceCount()/ 100;
				if (limit==0) {
					limit=1;
				}
				const uint16 veryRareBound = limit;
				const uint16 rareBound = 2*limit;

				// Cache Preview state (Criterion 2)
				FileType type = GetFiletype(GetFileName());
				const bool isPreviewEnable = theApp.glob_prefs->GetPreviewPrio() && (type == ftArchive || type == ftVideo);
					
				// Collect and calculate criteria for all chunks
				for (POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ) {
					Chunk& cur_chunk = chunksList.GetNext(pos);
					
					// Offsets of chunk
					const uint32 uStart = cur_chunk.part * PARTSIZE;
					const uint32 uEnd  = ((GetFileSize() - 1) < (uStart + PARTSIZE - 1)) ? (GetFileSize() - 1) : (uStart + PARTSIZE - 1);
					// Criterion 2. Parts used for preview
					// Remark: - We need to download the first part and the last part(s).
					//        - When the last part is very small, it's necessary to 
					//          download the two last parts.
					bool critPreview = false;
					if(isPreviewEnable == true) {
						if(cur_chunk.part == 0) {
							critPreview = true; // First chunk
						} else if(cur_chunk.part == partCount-1) {
							critPreview = true; // Last chunk
						} else if(cur_chunk.part == partCount-2) {
							// Last chunk - 1 (only if last chunk is too small)
							const uint32 sizeOfLastChunk = GetFileSize() - uEnd;
							if(sizeOfLastChunk < PARTSIZE/3) {
								critPreview = true; // Last chunk - 1
							}
						}
					}

					// Criterion 3. Request state (downloading in process from other source(s))
					// => CPU load
					const bool critRequested = cur_chunk.frequency > veryRareBound && IsAlreadyRequested(uStart, uEnd);

					// Criterion 4. Completion
					uint32 partSize = PARTSIZE;
					for(POSITION pos = gaplist.GetHeadPosition(); pos != NULL;) {
						const Gap_Struct* cur_gap = gaplist.GetNext(pos);
						// Check if Gap is into the limit
						if(cur_gap->start < uStart) {
							if(cur_gap->end > uStart && cur_gap->end < uEnd) {
								partSize -= cur_gap->end - uStart + 1;
							} else if(cur_gap->end >= uEnd) {
								partSize = 0;
								break; // exit loop for()
							}
						} else if(cur_gap->start <= uEnd) {
							if(cur_gap->end < uEnd) {
								partSize -= cur_gap->end - cur_gap->start + 1;
							} else {
								partSize -= uEnd - cur_gap->start + 1;
							}
						}
					}
					const uint16 critCompletion = (uint16)(partSize/(PARTSIZE/100)); // in [%]

					// Calculate priority with all criteria
					if(cur_chunk.frequency <= veryRareBound) {
						// 0..xxxx unrequested + requested very rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) + // Criterion 1
						((critPreview == true) ? 0 : 1) + // Criterion 2
						(100 - critCompletion); // Criterion 4
					} else if(critPreview == true) {
						// 10000..10100  unrequested preview chunks
						// 30000..30100  requested preview chunks
						cur_chunk.rank = ((critRequested == false) ? 10000 : 30000) + // Criterion 3
						(100 - critCompletion); // Criterion 4
					} else if(cur_chunk.frequency <= rareBound) {
						// 10101..1xxxx  unrequested rare chunks
						// 30101..3xxxx  requested rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) +                 // Criterion 1 
						((critRequested == false) ? 10101 : 30101) + // Criterion 3
						(100 - critCompletion); // Criterion 4
					} else {
						// common chunk
						if(critRequested == false) { // Criterion 3
							// 20000..2xxxx  unrequested common chunks
							cur_chunk.rank = 20000 + // Criterion 3
							(100 - critCompletion); // Criterion 4
						} else {
							// 40000..4xxxx  requested common chunks
							// Remark: The weight of the completion criterion is inversed
							//         to spead the requests over the completing chunks.
							//         Without this, the chunk closest to completion will
							//         received every new sources.
							cur_chunk.rank = 40000 + // Criterion 3
							(critCompletion); // Criterion 4
						}
					}
				}
			}

			// Select the next chunk to download
			if(chunksList.IsEmpty() == FALSE) {
				// Find and count the chunck(s) with the highest priority
				uint16 count = 0; // Number of found chunks with same priority
				uint16 rank = 0xffff; // Highest priority found

				// Collect and calculate criteria for all chunks
				for (POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ) {
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank < rank) {
						count = 1;
						rank = cur_chunk.rank;
					} else if(cur_chunk.rank == rank) {
						count++;
					}
				}

				// Use a random access to avoid that everybody tries to download the 
				// same chunks at the same time (=> spread the selected chunk among clients)
				uint16 randomness = 1 + (int) (((float)(count-1))*rand()/(RAND_MAX+1.0));
				for (POSITION pos = chunksList.GetHeadPosition(); ; ) {
					POSITION cur_pos = pos;	
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank == rank) {
						randomness--;
						if(randomness == 0) {
							// Selection process is over
							sender->m_lastPartAsked = cur_chunk.part;
							// Remark: this list might be reused up to *count times
							chunksList.RemoveAt(cur_pos);
							break; // exit loop for()
						}  
					}
				}
			} else {
				// There is no remaining chunk to download
				break; // Exit main loop while()
			}
		}
	}
	// Return the number of the blocks 
	*count = newBlockCount;
	// Return
	return (newBlockCount > 0);
}
// Maella end
// Kry EOI

void  CPartFile::RemoveBlockFromList(uint32 start,uint32 end)
{
	POSITION pos1,pos2;
	for (pos1 = requestedblocks_list.GetHeadPosition();(pos2 = pos1) != NULL;) {
		requestedblocks_list.GetNext(pos1);
		if (requestedblocks_list.GetAt(pos2)->StartOffset <= start && requestedblocks_list.GetAt(pos2)->EndOffset >= end) {
			requestedblocks_list.RemoveAt(pos2);
		}
	}
}

void CPartFile::RemoveAllRequestedBlocks(void)
{
	requestedblocks_list.RemoveAll();
}

//#include <pthread.h>
//pthread_attr_t pattr;

void CPartFile::CompleteFile(bool bIsHashingDone)
{
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if(srcarevisible) {
		Notify_DownloadCtrlHideSource(this);
	}
	if (!bIsHashingDone) {
		printf("HashNotDone\n");
		SetPartFileStatus(PS_COMPLETING);
		kBpsDown = 0.0;

		wxString strPartFile = m_partmetfilename.Left( m_partmetfilename.Length() - 4 );
		CAddFileThread::AddFile(theApp.glob_prefs->GetTempDir(), strPartFile, this );
		return;
	} else {
		printf("HashDone\n");		
		StopFile();
		m_is_A4AF_auto=false;
		SetPartFileStatus(PS_COMPLETING);
		// guess I was wrong about not need to spaw a thread ...
		// It is if the temp and incoming dirs are on different
		// partitions/drives and the file is large...[oz]
		//

		PerformFileComplete();


	}
	Notify_DownloadCtrlShowFilesCount();
	if (theApp.glob_prefs->ShowCatTabInfos()) {
		Notify_ShowUpdateCatTabTitles();
	}			
	UpdateDisplayedInfo(true);
}

#define UNEXP_FILE_ERROR			1
#define DELETE_FAIL_MET 			2
#define DELETE_FAIL_MET_BAK		4
#define SAME_NAME_RENAMED 	8
#define DELETE_FAIL_PART		 	16
#define DELETE_FAIL_SEEDS		32

// Kry - Anything to declare? ;)
// Free for new errors / messages

//#define UNEXP_FILE_ERROR 64
//#define UNEXP_FILE_ERROR 128


void CPartFile::CompleteFileEnded(int completing_result, wxString* newname) {
	
	
	if (!(completing_result & UNEXP_FILE_ERROR)) {
		m_fullname = (*newname);
	
		delete newname;
		
		if(wxFileName::DirExists(theApp.glob_prefs->GetCategory(GetCategory())->incomingpath)) {
			m_strFilePath = theApp.glob_prefs->GetCategory(m_category)->incomingpath;
		} else {
			m_strFilePath = theApp.glob_prefs->GetIncomingDir();
		}	
	
		SetPartFileStatus(PS_COMPLETE);
		paused = false;
		// TODO: What the f*** if it is already known?
		theApp.knownfiles->SafeAddKFile(this);
		// remove the file from the suspended uploads list
		theApp.uploadqueue->ResumeUpload(GetFileHash());
		SetAutoUpPriority(false);
		theApp.downloadqueue->RemoveFile(this);
		UpdateDisplayedInfo();
		Notify_DownloadCtrlShowFilesCount();

		//SHAddToRecentDocs(SHARD_PATH, fullname); // This is a real nasty call that takes ~110 ms on my 1.4 GHz Athlon and isn't really needed afai see...[ozon]
		// Barry - Just in case
		transfered = m_nFileSize;

		theApp.downloadqueue->StartNextFile();
	
	} else {
		paused = true;
		SetPartFileStatus(PS_ERROR);
		theApp.downloadqueue->StartNextFile();	
		AddLogLineM(true, wxString::Format(_("Unexpected file error while completing %s. File paused"), GetFileName().c_str()));
		delete newname;
		return;
	}	
	
	if (completing_result & DELETE_FAIL_MET) {
		AddLogLineM(true, wxString::Format(_("WARNING: Failed to delete %s"), m_fullname.c_str()));
	}	
	
	if (completing_result & DELETE_FAIL_MET_BAK) {
		AddLogLineM(true, wxString::Format(_("WARNING: Failed to delete %s%s"), m_fullname.c_str(), PARTMET_BAK_EXT));
	}	
	
	if (completing_result & SAME_NAME_RENAMED) {
		AddLogLineM(true, _("WARNING: A file with that name already exists, the file has been renamed"));
	}		

	if (completing_result & DELETE_FAIL_MET) {
		AddLogLineM(true, wxString::Format(_("WARNING: could not remove original '%s' after creating backup\n"), m_partmetfilename.Left(m_partmetfilename.Length()-4).c_str()));
	}	
	
	if (completing_result & DELETE_FAIL_SEEDS) {
		AddLogLineM(true, wxString::Format(_("WARNING: Failed to delete %s.seeds\n"), m_partmetfilename.c_str()));
	}	

	
	AddLogLineM(true, wxString::Format(_("Finished downloading %s :-)"), GetFileName().c_str()));
	Notify_ShowNotifier(wxString(_("Downloaded:"))+wxT("\n")+GetFileName(), TBN_DLOAD, 0);
}

completingThread::completingThread(wxString FileName, wxString fullname, uint32 Category, CPartFile* caller):wxThread(wxTHREAD_DETACHED)
{
	wxASSERT(!FileName.IsEmpty());
	wxASSERT(caller);
	wxASSERT(fullname);
	completing = caller;
	Completing_FileName = FileName;
	Completing_Fullname = fullname;
	Completing_Category = Category;
}

completingThread::~completingThread()
{
	//maybe a thread deletion needed
}

wxThread::ExitCode completingThread::Entry()
{

	// Threaded Completion code.
	
	completing_result = 0;

	// Strip the .met
	wxString partfilename =  Completing_Fullname.Left(Completing_Fullname.Length()-4);
	
	Completing_FileName = theApp.StripInvalidFilenameChars(Completing_FileName);

	newname = new wxString();
	if(wxFileName::DirExists(theApp.glob_prefs->GetCategory(Completing_Category)->incomingpath)) {
		(*newname) =  theApp.glob_prefs->GetCategory(Completing_Category)->incomingpath;
	} else {
		(*newname) =  theApp.glob_prefs->GetIncomingDir();
	}	
	(*newname) += wxT("/");
	(*newname) += Completing_FileName;
	
	if(wxFileName::FileExists(*newname)) {
		completing_result |= SAME_NAME_RENAMED;

		int namecount = 0;

		// the file extension & name
		wxString ext = Completing_FileName.AfterLast('.');
		wxString filename = Completing_FileName.BeforeLast('.');

		wxString strTestName;
		do {
			namecount++;
			if (ext.IsEmpty()) {
				strTestName = theApp.glob_prefs->GetIncomingDir(); 
				strTestName += wxT("/");
				strTestName += filename + wxString::Format(wxT("(%d)"), namecount);
			} else {
				strTestName = theApp.glob_prefs->GetIncomingDir(); 
				strTestName += wxT("/");
				strTestName += filename + wxString::Format(wxT("(%d)."), namecount);
				strTestName += ext;
			}
		} while(wxFileName::FileExists(strTestName));
		
		*newname = strTestName;
	}

	if (!FS_wxRenameFile(partfilename, *newname)) {

		if (!FS_wxCopyFile(partfilename, *newname)) {
			completing_result |= UNEXP_FILE_ERROR;
			return NULL;
		}
		
		if ( !wxRemoveFile(partfilename) ) {
			completing_result |= DELETE_FAIL_PART;
		}
	}
	
	if (!wxRemoveFile(Completing_Fullname)) {
		completing_result |= DELETE_FAIL_MET;
	}
	
	wxString BAKName(Completing_Fullname);
	BAKName += PARTMET_BAK_EXT;
	if (!wxRemoveFile(BAKName)) {
		completing_result |= DELETE_FAIL_MET_BAK;
	}

	wxString SEEDSName(Completing_Fullname);
	SEEDSName += wxT(".seeds");
	if (wxFileName::FileExists(SEEDSName)) {
		if (!wxRemoveFile(SEEDSName)) {
			completing_result |= DELETE_FAIL_SEEDS;
		}
	}
	
	return NULL;
}

void completingThread::OnExit(){
	
	// Kry - Notice the app that the completion has finished for this file.		
	
	wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED,TM_FILECOMPLETIONFINISHED);
	evt.SetClientData(completing);
	evt.SetInt((int)completing_result);
	evt.SetExtraLong((long)newname);
	wxPostEvent(&theApp,evt);
	
}


// Lord KiRon - using threads for file completion
uint8 CPartFile::PerformFileComplete()
{
	uint8 completed_errno = 0;
	
	//CSingleLock(&m_FileCompleteMutex,TRUE); // will be unlocked on exit
	wxMutexLocker sLock(m_FileCompleteMutex);

	// add this file to the suspended uploads list
	theApp.uploadqueue->SuspendUpload(GetFileHash());
	FlushBuffer();

	// close permanent handle
	m_hpartfile.Close();
	
	// Call thread for completion
	cthread=new completingThread(GetFileName(), m_fullname, GetCategory(), this);
	cthread->Create();
	cthread->Run();
	
	return completed_errno;
}

void  CPartFile::RemoveAllSources(bool bTryToSwap)
{
	for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end();) {
		CUpDownClient* cur_src = *it++;
		if (bTryToSwap) {
			if (!cur_src->SwapToAnotherFile(true, true, true, NULL)) {
				RemoveSource(cur_src,true,false);
				// If it was not swapped, it's not on any file anymore, and should die 
				//theApp.clientlist->RemoveClient(cur_src);
			}
		} else {
			RemoveSource(cur_src,true,false);
			//theApp.clientlist->RemoveClient(cur_src);
		}
	}

	UpdatePartsInfo(); 
	UpdateAvailablePartsCount();
	
	/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
	// remove all links A4AF in sources to this file
	if(!A4AFsrclist.empty()) {
		for( SourceSet::iterator it = A4AFsrclist.begin(); it != A4AFsrclist.end(); ) {
			CUpDownClient* cur_src = *it++;
			if ( cur_src->DeleteFileRequest( this ) ) {
				Notify_DownloadCtrlRemoveSource(cur_src, this);
			}
		}
		A4AFsrclist.clear();
	}
	/* eMule 0.30c implementation, i give it a try (Creteil) END ... */
	UpdateFileRatingCommentAvail();
}

void CPartFile::Delete()
{
	printf("Canceling\n");
	// Barry - Need to tell any connected clients to stop sending the file
	StopFile(true);
	printf("\tStopped\n");
	
	theApp.sharedfiles->RemoveFile(this);
	printf("\tRemoved from shared\n");
	theApp.downloadqueue->RemoveFile(this);
	printf("\tRemoved from download queue\n");
	Notify_DownloadCtrlRemoveFile(this);
	printf("\tRemoved transferwnd\n");

	// Kry - WTF? 
	// eMule had same problem with lseek error ... and override with a simple 
	// check for INVALID_HANDLE_VALUE (that, btw, does not exist on linux)
	// So we just guess is < 0 on error and > 2 if ok (0 stdin, 1 stdout, 2 stderr)
	if (m_hpartfile.fd() > 2) {  // 0 stdin, 1 stdout, 2 stderr
		m_hpartfile.Close();
	}

	printf("\tClosed\n");
	
	if (!wxRemoveFile(m_fullname)) {
		AddLogLineM(true, _("Failed to delete ") + m_fullname);
		printf("\tFailed to remove .part.met\n");
	} else {
		printf("\tRemoved .part.met\n");
	}

	wxString strPartFile = m_fullname.Left( m_fullname.Length() - 4 );
	
	if (!wxRemoveFile(strPartFile)) {
		AddLogLineM(true,_("Failed to delete ") + strPartFile);
		printf("\tFailed to removed .part\n");	
	} else {
		printf("\tRemoved .part\n");
	}
	
	wxString BAKName = m_fullname + PARTMET_BAK_EXT;

	if (!wxRemoveFile(BAKName)) {
		AddLogLineM(true,_("Failed to delete ") + BAKName);
		printf("\tFailed to remove .BAK\n");
	} else {
		printf("\tRemoved .BAK\n");
	}
	
	wxString SEEDSName = m_fullname + wxT(".seeds");
	
	if (wxFileName::FileExists(SEEDSName)) {
		if (!wxRemoveFile(SEEDSName)) {
			AddLogLineM(true,_("Failed to delete ") + SEEDSName);
		}
		printf("\tRemoved .seeds\n");
	}

	printf("Done\n");
	delete this;
}

bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)) {
		AddLogLineM(true, wxString::Format(_("Warning: Unable to hash downloaded part - hashset incomplete (%s)"), GetFileName().c_str()));
		hashsetneeded = true;
		return true;
	} else if ((GetHashCount() <= partnumber) && GetPartCount() != 1) {
		AddLogLineM(true, wxString::Format(_("Error: Unable to hash downloaded part - hashset incomplete (%s). This should never happen"),GetFileName().c_str()));
		hashsetneeded = true;
		return true;		
	} else {
		CMD4Hash hashresult;
		m_hpartfile.Seek((off_t)PARTSIZE*partnumber,wxFromStart);
		uint32 length = PARTSIZE;
		if (((ULONGLONG)PARTSIZE*(partnumber+1)) > (ULONGLONG)m_hpartfile.GetLength()){
			length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
			wxASSERT( length <= PARTSIZE );
		}
		CreateHashFromFile(&m_hpartfile,length,hashresult);

		if (GetPartCount() > 1) {
			if (hashresult != GetPartHash(partnumber)) {
				printf("HashResult: %s\n", unicode2char(hashresult.Encode()));
				printf("GetPartHash(%i): %s\n",partnumber, unicode2char(GetPartHash(partnumber).Encode()));
				/* To output to stdout - we should output to file
				m_hpartfile.Seek((off_t)PARTSIZE*partnumber,wxFromStart);
				uint32 length = PARTSIZE;
				if (((ULONGLONG)PARTSIZE*(partnumber+1)) > (ULONGLONG)m_hpartfile.GetLength()){
					length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
					wxASSERT( length <= PARTSIZE );
				}				
				uchar mychar[length+1];
				m_hpartfile.Read(mychar,length);
				printf("Corrupt Data:\n");
				DumpMem(mychar,length);										
				*/
				return false;
			} else {
				return true;
			}
		} else {
			if (hashresult != m_abyFileHash) {
				return false;
			} else {
				return true;
			}
		}
	}

}

bool CPartFile::IsCorruptedPart(uint16 partnumber)
{
	return corrupted_list.Find(partnumber);
}

void CPartFile::SetDownPriority(uint8 np, bool bSave, bool bRefresh )
{
	m_iDownPriority = np;
	theApp.downloadqueue->SortByPriority();
 // theApp.downloadqueue->CheckDiskspace();
	if ( bRefresh )
	UpdateDisplayedInfo(true);
	if ( bSave )
		SavePartFile();
}

void CPartFile::StopFile(bool bCancel)
{
	// Barry - Need to tell any connected clients to stop sending the file
	stopped = true; // Kry - Need to set it here to get into SetPartFileStatus(status) correctly
	PauseFile();

	RemoveAllSources(true);
	paused = true;
	stopped=true;
	kBpsDown = 0.0;
	transferingsrc = 0;
	insufficient = false;
	memset(m_anStates,0,sizeof(m_anStates));
	if (!bCancel) {
		FlushBuffer();
	}
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();
	UpdateDisplayedInfo(true);
}

void CPartFile::StopPausedFile()
{
	//Once an hour, remove any sources for files which are no longer active downloads
	UINT uState = GetStatus();
	if( (uState==PS_PAUSED || uState==PS_INSUFFICIENT || uState==PS_ERROR) && !stopped && time(NULL) - m_iLastPausePurge > (60*60)) {
		StopFile();
	}
}

void CPartFile::PauseFile(bool bInsufficient)
{
	m_iLastPausePurge = time(NULL);
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}

	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* cur_src = *it++;
		if (cur_src->GetDownloadState() == DS_DOWNLOADING) {
			if (!cur_src->GetSentCancelTransfer()) {				
				theApp.uploadqueue->AddUpDataOverheadOther(packet->GetPacketSize());
				cur_src->SendPacket(packet,false,true);
				cur_src->SetDownloadState(DS_ONQUEUE);
				cur_src->SetSentCancelTransfer(1);
			}
			cur_src->SetDownloadState(DS_ONQUEUE);
		}
	}
	delete packet;

	if (bInsufficient) {
		insufficient = true;
	} else {
		paused = true;
		insufficient = false;
	}
	
	kBpsDown = 0.0;
	transferingsrc = 0;
	m_anStates[DS_DOWNLOADING] = 0;
	
	SetStatus(status);
	
	if (!bInsufficient) {
		theApp.downloadqueue->SortByPriority();
		theApp.downloadqueue->CheckDiskspace();
		SavePartFile();
	}

}

void CPartFile::ResumeFile()
{
	
	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}
	
	paused = false;
	stopped = false;
	lastsearchtime = 0;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();
	SavePartFile();
	//SetPartFileStatus(status);
	UpdateDisplayedInfo(true);
	
}

wxString CPartFile::getPartfileStatus() const
{

	wxString mybuffer; 

	if (GetTransferingSrcCount()>0) {
		mybuffer=_("Downloading");
	}	else {
		mybuffer=_("Waiting");
	}
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			mybuffer=_("Hashing");
			break; 
		case PS_COMPLETING:
			mybuffer=_("Completing");
			break; 
		case PS_COMPLETE:
			mybuffer=_("Complete");
			break; 
		case PS_PAUSED:
			mybuffer=_("Paused");
			break; 
		case PS_ERROR:
			mybuffer=_("Erroneous");
			break;
	} 
	if (stopped && (GetStatus()!=PS_COMPLETE)) {
		mybuffer=_("Stopped");
	}
	return mybuffer; 
} 

int CPartFile::getPartfileStatusRang() const
{
	
	int tempstatus=0;
	if (GetTransferingSrcCount()==0) tempstatus=1;
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			tempstatus=3;
			break; 
		case PS_COMPLETING:
			tempstatus=4;
			break; 
		case PS_COMPLETE:
			tempstatus=5;
			break; 
		case PS_PAUSED:
			tempstatus=2;
			break; 
		case PS_ERROR:
			tempstatus=6;
			break;
	} 
	return tempstatus;
} 

sint32 CPartFile::getTimeRemaining() const
{
	if (GetKBpsDown() < 0.001)
		return -1;
	else 
		return((GetFileSize()-GetCompletedSize()) / ((int)(GetKBpsDown()*1024.0)));
} 

void CPartFile::PreviewFile()
{
	wxString command;

	// If no player set in preferences, use mplayer.
	if (theApp.glob_prefs->GetVideoPlayer().IsEmpty()) {
		command.Append(wxT("mplayer"));
	} else {
		command.Append(theApp.glob_prefs->GetVideoPlayer());
	}
	// Need to use quotes in case filename contains spaces.
	command.Append(wxT(" \""));
	command.Append(GetFullName());
	
	if (!(GetStatus() == PS_COMPLETE)) {
		// Remove the .met from filename.
		for (int i=0;i<4;i++) {
			command.RemoveLast();
		}
	}
	
	command.Append(wxT("\""));
     wxExecute(command);
}

bool CPartFile::PreviewAvailable()
{
	return (( GetFiletype(GetFileName()) == ftVideo ) && IsComplete(0, 256*1024));
}

#if 0
bool CPartFile::PreviewAvailable()
{
	wxLongLong free;
	wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), NULL, &free);
	printf("\nFree Space (wxLongLong): %s\n", unicode2char(free.ToString()));
	typedef unsigned long long uint64;
	uint64 space = free.GetValue();
	printf("\nFree Space (uint64): %lli\n", space);

	// Barry - Allow preview of archives of any length > 1k
	if (IsArchive(true)) {
		if (GetStatus() != PS_COMPLETE && GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && (!m_bRecoveringArchive) && ((space + 100000000) > (2*GetFileSize()))) {
			return true;
		} else {
			return false;
		}
	}
	if (theApp.glob_prefs->IsMoviePreviewBackup()) {
		return !( (GetStatus() != PS_READY && GetStatus() != PS_PAUSED)
		|| m_bPreviewing || GetPartCount() < 5 || !IsMovie() || (space + 100000000) < GetFileSize() 
		|| ( !IsComplete(0,PARTSIZE-1) || !IsComplete(PARTSIZE*(GetPartCount()-1),GetFileSize()-1)));
	} else {
		TCHAR szVideoPlayerFileName[_MAX_FNAME];
		_tsplitpath(theApp.glob_prefs->GetVideoPlayer(), NULL, NULL, szVideoPlayerFileName, NULL);

		// enable the preview command if the according option is specified 'PreviewSmallBlocks' 
		// or if VideoLAN client is specified
		if (theApp.glob_prefs->GetPreviewSmallBlocks() || !_tcsicmp(szVideoPlayerFileName, _T("vlc"))) {
			if (m_bPreviewing) {
				return false;
			}
			uint8 uState = GetStatus();
			if (!(uState == PS_READY || uState == PS_EMPTY || uState == PS_PAUSED)) {
				return false;
			}
			// default: check the ED2K file format to be of type audio, video or CD image. 
			// but because this could disable the preview command for some file types which eMule does not know,
			// this test can be avoided by specifying 'PreviewSmallBlocks=2'
			if (theApp.glob_prefs->GetPreviewSmallBlocks() <= 1) {
				// check the file extension
				EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
				if (!(eFileType == ED2KFT_VIDEO || eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_CDIMAGE)) {
					// check the ED2K file type
					LPCSTR pszED2KFileType = GetStrTagValue(FT_FILETYPE);
					if (pszED2KFileType == NULL || !(!strcasecmp(pszED2KFileType, "Audio") || !strcasecmp(pszED2KFileType, "Video"))) {
						return false;
					}
				}
			}

			// If it's an MPEG file, VLC is even capable of showing parts of the file if the beginning of the file is missing!
			bool bMPEG = false;
			LPCTSTR pszExt = _tcsrchr(GetFileName(), _T('.'));
			if (pszExt != NULL) {
				wxString strExt(pszExt);
				strExt.MakeLower();
				bMPEG = (strExt==_T(".mpg") || strExt==_T(".mpeg") || strExt==_T(".mpe") || strExt==_T(".mp3") || strExt==_T(".mp2") || strExt==_T(".mpa"));
			}
			if (bMPEG) {
				// TODO: search a block which is at least 16K (Audio) or 256K (Video)
				if (GetCompletedSize() < 16*1024) {
					return false;
				}
			} else {
				// For AVI files it depends on the used codec..
				if (!IsComplete(0, 256*1024))
					return false;
				}
			}
			return true;
		} else {
			return !((GetStatus() != PS_READY && GetStatus() != PS_PAUSED)
			|| m_bPreviewing || GetPartCount() < 2 || !IsMovie() || !IsComplete(0,PARTSIZE-1));
		}
	}
}
#endif

void CPartFile::UpdateAvailablePartsCount()
{
	uint8 availablecounter = 0;
	uint16 iPartCount = GetPartCount();
	for (uint32 ixPart = 0; ixPart < iPartCount; ixPart++) {		
		for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it) {
			CUpDownClient *cur_src = *it;
			if (cur_src->IsPartAvailable(ixPart)) {
				availablecounter++;
				break;
			}
		}
	}
	if (iPartCount == availablecounter && availablePartsCount < iPartCount) {
		lastseencomplete = time(NULL);//CTime::GetCurrentTime();
	}
	availablePartsCount = availablecounter;
}

void CPartFile::SetLastAnsweredTime()
{
	m_ClientSrcAnswered = ::GetTickCount();
}

void CPartFile::SetLastAnsweredTimeTimeout()
{ 
	m_ClientSrcAnswered = 2 * CONNECTION_LATENCY + ::GetTickCount() - SOURCECLIENTREASKS;
}

Packet *CPartFile::CreateSrcInfoPacket(const CUpDownClient* forClient)
{
	if(!IsPartFile())
		return CKnownFile::CreateSrcInfoPacket(forClient);

	if (forClient->GetRequestFile() != this)
		return NULL;

	if ( !(GetStatus() == PS_READY || GetStatus() == PS_EMPTY))
		return NULL;

	if ( m_SrcList.empty() )
		return NULL;

	CSafeMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash16(m_abyFileHash);
	data.WriteUInt16(nCount);
	bool bNeeded;
	for (SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ) {
		bNeeded = false;
		CUpDownClient* cur_src = *it;
		if (cur_src->HasLowID() || !cur_src->IsValidSource()) {
			continue;
		}
		// only send source which have needed parts for this client if possible
		const uint8* srcstatus = cur_src->GetPartStatus();
		if (srcstatus) {
			const uint8* reqstatus = forClient->GetPartStatus();
			int n = GetPartCount();
			if (reqstatus) {
				// only send sources which have needed parts for this client
				for (int x = 0; x < n; x++) {
					if (srcstatus[x] && !reqstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			} else {
				// if we don't know the need parts for this client, 
				// return any source currently a client sends it's 
				// file status only after it has at least one complete part
				for (int x = 0; x < GetPartCount(); x++){
					if (srcstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			}
		}
		if(bNeeded) {
			nCount++;
			uint32 dwID;
			#warning We should use the IDHybrid here... but is not implemented yet
			if(forClient->GetSourceExchangeVersion() > 2) {
				dwID = cur_src->GetUserID();
			} else {
				dwID = ntohl(cur_src->GetUserID());
			}
			data.WriteUInt32(dwID);
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion()>1) {
				data.WriteHash16(cur_src->GetUserHash());
			}
			if (nCount > 500) {
				break;
			}
		}
	}
	if (!nCount) {
		return 0;
	}
	data.Seek(16,wxFromStart);
	data.WriteUInt16(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->SetOpCode(OP_ANSWERSOURCES);
	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if (result->GetPacketSize() > 354) {
		result->PackPacket();
	}
	//if (thePrefs.GetDebugSourceExchange()) {
		AddDebugLogLineM( false, wxString::Format(_("Send:Source User(%s) File(%s) Count(%i)"), forClient->GetUserName().c_str(), GetFileName().c_str(), nCount ));
	//}
	return result;
}

void CPartFile::AddClientSources(CSafeMemFile* sources,uint8 sourceexchangeversion)
{
	if(stopped) {
		return;
	}
	uint16 nCount = sources->ReadUInt16();
	for (int i = 0;i != nCount;i++) {
		uint32 dwID = sources->ReadUInt32();
		uint16 nPort = sources->ReadUInt16();
		uint32 dwServerIP = sources->ReadUInt32();
		uint16 nServerPort = sources->ReadUInt16();
		
		uchar achUserHash[16];
		if (sourceexchangeversion > 1) {
			sources->ReadHash16(achUserHash);
		}
		// check first if we are this source
		if (theApp.serverconnect->GetClientID() < 16777216 && theApp.serverconnect->IsConnected()) {
			if ((theApp.serverconnect->GetClientID() == dwID) && theApp.serverconnect->GetCurrentServer()->GetIP() == dwServerIP) {
				continue;
			}
		} else if (theApp.serverconnect->GetClientID() == dwID) {
			continue;
		} else if (dwID < 16777216) {
			continue;
		}
		if(theApp.glob_prefs->GetMaxSourcePerFile() > GetSourceCount()) {
			CUpDownClient* newsource = new CUpDownClient(nPort,dwID,dwServerIP,nServerPort,this);
			if (sourceexchangeversion > 1) {
				newsource->SetUserHash(achUserHash);
			}
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} else {
			break;
		}
	}
}

void CPartFile::UpdateAutoDownPriority()
{
	if (!IsAutoDownPriority()) {
		return;
	}
	if (GetSourceCount() <= RARE_FILE) {
		if ( GetDownPriority() != PR_HIGH )
			SetDownPriority(PR_HIGH, false, false);
	} else if (GetSourceCount() < 100) {
		if ( GetDownPriority() != PR_NORMAL )
			SetDownPriority(PR_NORMAL, false, false);
	} else {
		if ( GetDownPriority() != PR_LOW )
			SetDownPriority(PR_LOW, false, false);
	}
}

// making this function return a higher when more sources have the extended
// protocol will force you to ask a larger variety of people for sources

int CPartFile::GetCommonFilePenalty()
{
	//TODO: implement, but never return less than MINCOMMONPENALTY!
	return MINCOMMONPENALTY;
}

/* Barry - Replaces BlockReceived() 

	Originally this only wrote to disk when a full 180k block
	had been received from a client, and only asked for data in
	180k blocks.

	This meant that on average 90k was lost for every connection
	to a client data source. That is a lot of wasted data.

	To reduce the lost data, packets are now written to a buffer
	and flushed to disk regularly regardless of size downloaded.
	This includes compressed packets.

	Data is also requested only where gaps are, not in 180k blocks.
	The requests will still not exceed 180k, but may be smaller to
	fill a gap.
*/

uint32 CPartFile::WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block)
{
	// Increment transfered bytes counter for this file
	transfered += transize;

	// This is needed a few times
	uint32 lenData = end - start + 1;

	if(lenData > transize) {
		m_iGainDueToCompression += lenData-transize;
	}

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end)) {
		AddDebugLogLineM(false, wxString::Format(_("File '%s' has already been written from %lu to %lu\n"), GetFileName().c_str(), (long)start, (long)end));
		return 0;
	}

	// Create copy of data as new buffer
	BYTE *buffer = new BYTE[lenData];
	memcpy(buffer, data, lenData);

	// Create a new buffered queue entry
	PartFileBufferedData *item = new PartFileBufferedData;
	item->data = buffer;
	item->start = start;
	item->end = end;
	item->block = block;

	// Add to the queue in the correct position (most likely the end)
	PartFileBufferedData *queueItem;
	bool added = false;
	POSITION pos = m_BufferedData_list.GetTailPosition();
	while (pos != NULL) {
		queueItem = m_BufferedData_list.GetPrev(pos);
		if (item->end > queueItem->end) {
			added = true;
			m_BufferedData_list.InsertAfter(pos, item);
			break;
		}
	}
	if (!added) {
		m_BufferedData_list.AddHead(item);
	}

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(item->start, item->end);

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	pos = requestedblocks_list.GetHeadPosition();
	while (pos != NULL) {	
		if (requestedblocks_list.GetNext(pos) == item->block) {
			item->block->transferred += lenData;
		}
	}

	if (gaplist.IsEmpty()) {
		FlushBuffer();
	}

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(void)
{
	m_nLastBufferFlushTime = GetTickCount();
	
	if (m_BufferedData_list.IsEmpty()) {
		return;
	}

	/*
	Madcat - Check if there is at least PARTSIZE amount of free disk space
	in temp dir before flushing. If not enough space, pause the file,
	add log line and abort flushing.
	*/
	wxLongLong total = 0, free = 0;
	if (wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free) && free < PARTSIZE) {
		AddLogLineM(true, _("ERROR: Cannot write to disk"));
		PauseFile();
		return;
	}
	
	uint32 partCount = GetPartCount();
	bool *changedPart = new bool[partCount];
	
	try {
		// Remember which parts need to be checked at the end of the flush
		for (int partNumber=0; (uint32)partNumber<partCount; partNumber++) {
			changedPart[partNumber] = false;
		}

		bool bCheckDiskspace = theApp.glob_prefs->IsCheckDiskspaceEnabled() && theApp.glob_prefs->GetMinFreeDiskSpace() > 0;

		wxLongLong total = 0, free = 0;
		wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free);
		typedef unsigned long long uint64;
		// WHY IS THIS NOT USED? ... //uint64 GetFreeDiskSpaceX = free.GetValue();
		// WHY IS THIS NOT USED? ... //ULONGLONG uFreeDiskSpace = bCheckDiskspace ? GetFreeDiskSpaceX : 0;

		// Ensure file is big enough to write data to (the last item will be the furthest from the start)
		PartFileBufferedData *item = m_BufferedData_list.GetTail();
		if ((unsigned)m_hpartfile.Length() <= item->end) {
			//m_hpartfile.SetLength(item->end + 1);
			#ifdef __WXMSW__
			chsize(m_hpartfile.fd(),item->end+1);
			#else
			ftruncate(m_hpartfile.fd(),item->end+1);
			#endif
		}
		
		// Loop through queue
		for (int i = m_BufferedData_list.GetCount(); i>0; i--) {
			if (i<1) { // Can this happen ? 
				return; // Well, if it did... abort then.
			}
			// Get top item
			item = m_BufferedData_list.GetHead();

			// This is needed a few times
			uint32 lenData = item->end - item->start + 1;

			// SLUGFILLER: SafeHash - could be more than one part
			for (uint32 curpart = item->start/PARTSIZE; curpart <= item->end/PARTSIZE; curpart++)
				changedPart[curpart] = true;
			// SLUGFILLER: SafeHash		
			
			// Go to the correct position in file and write block of data			
			m_hpartfile.Seek(item->start);
			m_hpartfile.Write(item->data, lenData);
			
			// Remove item from queue
			m_BufferedData_list.RemoveHead();

			// Decrease buffer size
			m_nTotalBufferData -= lenData;

			// Release memory used by this item
			delete [] item->data;
			delete item;
		}

		// Flush to disk
		m_hpartfile.Flush();

		// Check each part of the file
		uint32 partRange = (m_hpartfile.GetLength() % PARTSIZE) - 1;
		for (int partNumber = partCount-1; partNumber >= 0; partNumber--) {
			if (changedPart[partNumber] == false) {
				// Any parts other than last must be full size
				partRange = PARTSIZE - 1;
				continue;
			}

			// Is this 9MB part complete
			if (IsComplete(PARTSIZE * partNumber, (PARTSIZE * (partNumber + 1)) - 1)) {
				// Is part corrupt
				if (!HashSinglePart(partNumber)) {
					AddLogLineM(true, wxString::Format(_("Downloaded part %i is corrupt :(  (%s)"), partNumber, GetFileName().c_str()));
					AddGap(PARTSIZE*partNumber, (PARTSIZE*partNumber + partRange));
					corrupted_list.AddTail(partNumber);
					// Reduce transfered amount by corrupt amount
					m_iLostDueToCorruption += (partRange + 1);
				} else {
					if (!hashsetneeded) {
						AddDebugLogLineM(false, wxString::Format(_("Finished part %u of \"%s\""), partNumber, GetFileName().c_str()));
					}
					
					// if this part was successfully completed (although ICH is active), remove from corrupted list
					POSITION posCorrupted = corrupted_list.Find(partNumber);
					if (posCorrupted)
						corrupted_list.RemoveAt(posCorrupted);
					
					// Successfully completed part, make it available for sharing				
					if (status == PS_EMPTY) {
						SetStatus(PS_READY);
						theApp.sharedfiles->SafeAddKFile(this);
					}
				}
			} else if ( IsCorruptedPart(partNumber) && theApp.glob_prefs->IsICHEnabled()) {
				// Try to recover with minimal loss
				if (HashSinglePart(partNumber)) {
					m_iTotalPacketsSavedDueToICH++;
					FillGap(PARTSIZE*partNumber,(PARTSIZE*partNumber+partRange));
					RemoveBlockFromList(PARTSIZE*partNumber,(PARTSIZE*partNumber + partRange));

					// remove from corrupted list
					POSITION posCorrupted = corrupted_list.Find(partNumber);
					if (posCorrupted)
						corrupted_list.RemoveAt(posCorrupted);
					
					AddLogLineM(true, wxString::Format(_("ICH: Recovered corrupted part %i  (%s)"), partNumber,GetFileName().c_str()));
					
					// Successfully recovered part, make it available for sharing
					if (status == PS_EMPTY) {
						SetStatus(PS_READY);
						if (theApp.IsRunning()) // may be called during shutdown!
							theApp.sharedfiles->SafeAddKFile(this);
					}
				}
			}
			// Any parts other than last must be full size
			partRange = PARTSIZE - 1;
		}

		// Update met file
		SavePartFile();

		if (theApp.IsRunning()) { // may be called during shutdown!
			// Is this file finished ?
			if (gaplist.IsEmpty()) {
				CompleteFile(false);
			}

			// Check free diskspace
			//
			// Checking the free disk space again after the file was written could most likely be avoided, but because
			// we do not use real physical disk allocation units for the free disk computations, it should be more safe
			// and accurate to check the free disk space again, after file was written and buffers were flushed to disk.
			//
			// If useing a normal file, we could avoid the check disk space if the file was not increased.
			// If useing a compressed or sparse file, we always have to check the space 
			// regardless whether the file was increased in size or not.
			if (bCheckDiskspace) {
				switch(GetStatus()) {
					case PS_PAUSED:
					case PS_ERROR:
					case PS_COMPLETING:
					case PS_COMPLETE:
						break;
					default: {
						wxLongLong total = 0, free = 0;
						wxGetDiskSpace(theApp.glob_prefs->GetTempDir(), &total, &free);
						typedef unsigned long long uint64;
						uint64 GetFreeDiskSpaceX = free.GetValue();
						if (GetFreeDiskSpaceX < (unsigned)theApp.glob_prefs->GetMinFreeDiskSpace()) {
							// Normal files: pause the file only if it would still grow
							uint32 nSpaceToGrow = GetNeededSpace();
							if (nSpaceToGrow) {
								PauseFile(true/*bInsufficient*/);
							}
						}
					}
				}
			}
		}
	}
	catch(...) {
		AddLogLineM(true, wxString::Format(_("Unexpected file error while writing %s : %s"), GetFileName().c_str(), _("Unknown")));
		SetPartFileStatus(PS_ERROR);
		paused = true;
		m_iLastPausePurge = time(NULL);
		theApp.downloadqueue->RemoveLocalServerRequest(this);
		kBpsDown = 0.0;
		transferingsrc = 0;
		if (theApp.IsRunning()) { // may be called during shutdown!
			UpdateDisplayedInfo();
		}
	}
	delete[] changedPart;

}

// Barry - This will invert the gap list, up to caller to delete gaps when done
// 'Gaps' returned are really the filled areas, and guaranteed to be in order

void CPartFile::GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled)
{
	Gap_Struct *gap = NULL;
	Gap_Struct *best = NULL;
	POSITION pos;
	uint32 start = 0;
	uint32 bestEnd = 0;

	// Loop until done
	bool finished = false;
	while (!finished) {
		finished = true;
		// Find first gap after current start pos
		bestEnd = m_nFileSize;
		pos = gaplist.GetHeadPosition();
		while (pos != NULL) {
			gap = gaplist.GetNext(pos);
			if ((gap->start > start) && (gap->end < bestEnd)) {
				best = gap;
				bestEnd = best->end;
				finished = false;
			}
		}

		if (!finished) {
			// Invert this gap
			gap = new Gap_Struct;
			gap->start = start;
			gap->end = best->start - 1;
			start = best->end + 1;
			filled->AddTail(gap);
		} else if (best->end < m_nFileSize) {
			gap = new Gap_Struct;
			gap->start = best->end + 1;
			gap->end = m_nFileSize;
			filled->AddTail(gap);
		}
	}
}

void CPartFile::UpdateFileRatingCommentAvail()
{
	if (!this) {
		return;
	}

	bool prev=(hasComment || hasRating);
	bool prevbad=hasBadRating;

	hasComment=false;
	int badratings=0;
	int ratings=0;

	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ) {
		CUpDownClient* cur_src = *it;
		
		if (cur_src->GetFileComment().Length()>0) {
			hasComment=true;
		}

		if (cur_src->GetFileRate()>0) {
			ratings++;
		}
		if (cur_src->GetFileRate()==1) {
			badratings++;
		}
	}
	hasBadRating=(badratings> (ratings/3));
	hasRating=(ratings>0);
	if ((prev!=(hasComment || hasRating)) || (prevbad!=hasBadRating)) {
		UpdateDisplayedInfo();
	}
}

void CPartFile::UpdateDisplayedInfo(bool force)
{
	DWORD curTick = ::GetTickCount();

	 // Wait 1.5s between each redraw
	 if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE + 500 ) {
		 Notify_DownloadCtrlUpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
	
}

time_t CPartFile::GetLastChangeDatetime(bool forcecheck)
{
	if ((::GetTickCount()-m_lastdatetimecheck)<60000 && !forcecheck) {
		return m_lastdatecheckvalue;
	}

	m_lastdatetimecheck=::GetTickCount();
	if (!::wxFileExists(m_hpartfile.GetFilePath())) {
		m_lastdatecheckvalue=-1;
	} else {
		//CFileStatus filestatus;
		struct stat filestatus;
		fstat(m_hpartfile.fd(),&filestatus);
		//m_hpartfile.GetStatus(filestatus); // this; "...returns m_attribute without high-order flags" indicates a known MFC bug, wonder how many unknown there are... :)
		m_lastdatecheckvalue=filestatus.st_mtime;
	}
	return m_lastdatecheckvalue;
}

uint8 CPartFile::GetCategory()
{
	if(m_category>theApp.glob_prefs->GetCatCount()-1) {
		m_category=0;
	}
	return m_category;
}


wxString CPartFile::GetProgressString(uint16 size)
{
	char crProgress = '0';	//green
	char crHave = '1';	// black
	char crPending='2';	// yellow
	//char crWaiting='3';	// blue
	//char crMissing='4';	// red
	//added lemonfan's progressbar patch
	char crMissing='3';	// red
	
	char crWaiting[6];
	crWaiting[0]='4'; // blue few source
	crWaiting[1]='5';
	crWaiting[2]='6';
	crWaiting[3]='7';
	crWaiting[4]='8';
	crWaiting[5]='9'; // full sources

	wxString my_ChunkBar=wxT("");

	for (uint16 i=0;i<=size+1;i++) {
		my_ChunkBar.Append(crHave,1); //.AppendChar(crHave);
	}
	// one more for safety

	float unit= (float)size/(float)m_nFileSize;
	uint32 allgaps = 0;

	if(GetStatus() == PS_COMPLETE || GetStatus() == PS_COMPLETING) {  
		CharFillRange(&my_ChunkBar,0,(uint32)(m_nFileSize*unit), crProgress);
	} else {	
		// red gaps
		for (POSITION pos = gaplist.GetHeadPosition();pos !=  0; ) {
			Gap_Struct* cur_gap = gaplist.GetNext(pos);
			allgaps += cur_gap->end - cur_gap->start + 1;
			bool gapdone = false;
			uint32 gapstart = cur_gap->start;
			uint32 gapend = cur_gap->end;
			for (uint32 i = 0; i < GetPartCount(); i++){
				if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE) { // is in this part?
					if (gapend <= (i+1)*PARTSIZE) {
						gapdone = true;
					} else {
						gapend = (i+1)*PARTSIZE; // and next part
					}
					// paint
					uint8 color;
					if (m_SrcpartFrequency.GetCount() >= (size_t)i && m_SrcpartFrequency[i]) {  // frequency?
						//color = crWaiting;
						//added lemonfan's progressbar patch
						color = m_SrcpartFrequency[i] < 10 ? crWaiting[m_SrcpartFrequency[i]/2]:crWaiting[5];
					} else {
						color = crMissing;
					}
					CharFillRange(&my_ChunkBar,(uint32)(gapstart*unit), (uint32)(gapend*unit + 1),  color);
					if (gapdone) { // finished?
						break;
					} else {
						gapstart = gapend;
						gapend = cur_gap->end;
					}
				}
			}
		}
	}

	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0; ) {
		Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		CharFillRange(&my_ChunkBar, (uint32)((block->StartOffset + block->transferred)*unit),(uint32)(block->EndOffset*unit),crPending);
	}
	return my_ChunkBar;
}
                                                                                
void CPartFile::CharFillRange(wxString* buffer,uint32 start, uint32 end, char color)
{
	for (uint32 i = start;i <= end;i++) {
		buffer->SetChar(i,color);
	}
}


bool CPartFile::RemoveSource(CUpDownClient* toremove, bool updatewindow, bool bDoStatsUpdate)
{
	wxASSERT( toremove );

	theApp.downloadqueue->RemoveSource( toremove, updatewindow, bDoStatsUpdate );

	// Check if the client should be deleted, but not if the client is already dying
	if ( !toremove->GetSocket() && !toremove->HasBeenDeleted() ) {
		if ( toremove->Disconnected(wxT("RemoveSource - purged")) ) {
			toremove->Safe_Delete();
		}
	}

}

/* Razor 1a - Modif by MikaelB
   RemoveNoNeededSources function */

void CPartFile::RemoveNoNeededSources()
{
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* client = *it++;
		if (client->GetDownloadState() == DS_NONEEDEDPARTS) {
			/* If allowed, try to swap to other file. If swapping fails, remove from this one. */
			if (theApp.glob_prefs->SwapNoNeededSources()) {
				if (!client->SwapToAnotherFile(true, true, true, NULL)) {
					RemoveSource( client );
				}
			/* If not allowed to swap, simply remove from this one. */
			} else {
				RemoveSource( client );
			}
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   RemoveFullQueueSources function */

void CPartFile::RemoveFullQueueSources()
{
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* client = *it++;
		if ((client->GetDownloadState() == DS_ONQUEUE) && (client->IsRemoteQueueFull())) {
			RemoveSource( client );
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   RemoveHighQueueRatingSources function */

void CPartFile::RemoveHighQueueRatingSources()
{
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* client = *it++;
		if ((client->GetDownloadState() == DS_ONQUEUE) && (client->GetRemoteQueueRank() > theApp.glob_prefs->HighQueueRanking())) {
			RemoveSource( client );
		}
	}
}
/* End modif */

/* Razor 1a - Modif by MikaelB
   CleanUpSources function */

void CPartFile::CleanUpSources()
{
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* client = *it++;
		if (client->GetDownloadState() == DS_NONEEDEDPARTS) {
			if ((theApp.glob_prefs->DropNoNeededSources()) && (!client->SwapToAnotherFile(true, true, true, NULL))) {
				RemoveSource( client );
			}
		}
		if ((client->GetDownloadState() == DS_ONQUEUE) && (client->IsRemoteQueueFull())) {
			RemoveSource( client );
		} else if ((client->GetDownloadState() == DS_ONQUEUE) && (client->GetRemoteQueueRank() > theApp.glob_prefs->HighQueueRanking())) {
			RemoveSource( client );
		}
	}
}
/* End modif */

void CPartFile::AddDownloadingSource(CUpDownClient* client)
{
	std::list<CUpDownClient *>::iterator it = 
		std::find(m_downloadingSourcesList.begin(), m_downloadingSourcesList.end(), client);
	if (it == m_downloadingSourcesList.end()) {
		m_downloadingSourcesList.push_back(client);
	}
}

void CPartFile::RemoveDownloadingSource(CUpDownClient* client)
{
	std::list<CUpDownClient *>::iterator it = 
		std::find(m_downloadingSourcesList.begin(), m_downloadingSourcesList.end(), client);
	if (it != m_downloadingSourcesList.end()) {
		m_downloadingSourcesList.erase(it);
	}
}

void CPartFile::SetPartFileStatus(uint8 newstatus)
{
	status=newstatus;
	
	if (theApp.glob_prefs->GetAllcatType()) {
		// lfroen - just notify gui that show-hide status is changing
		Notify_DownloadCtrlShowHideFileStatus(this);
		Notify_DownloadCtrlShowFilesCount();
	}
	Notify_DownloadCtrlInitSort();
}

void CPartFile::ResumeFileInsufficient()
{
	if (status==PS_COMPLETE || status==PS_COMPLETING) {
		return;
	}
	if (!insufficient) {
		return;
	}
	insufficient = false;
	lastsearchtime = 0;
	UpdateDisplayedInfo(true);
}

uint32 CPartFile::GetNeededSpace()
{
	if ((unsigned)m_hpartfile.Length() > GetFileSize()) {
		return 0;	// Shouldn't happen, but just in case
	}
	return GetFileSize()-m_hpartfile.Length();
}

void CPartFile::SetStatus(uint8 in)
{
	wxASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );
	
	status=in;
	if (theApp.IsRunning()) {
		// lfroen - if needed, must be on gui side, not here anyway
		//if (Notify_DownloadCtrlGetCurrCat()==0) {
			//Notify_DownloadCtrlChangeCat(0);
		//}
		UpdateDisplayedInfo(true);
		if (theApp.glob_prefs->ShowCatTabInfos()) {
			Notify_ShowUpdateCatTabTitles();
		}
	}
}

