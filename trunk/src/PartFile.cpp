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

#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wx/defs.h>		// Needed before any other wx/*.h

#include <wx/intl.h>		// Needed for _
#include <wx/filename.h>	// Needed for wxFileName
#include <wx/utils.h>
#include <wx/tokenzr.h>		// Needed for wxStringTokenizer

#ifndef AMULE_DAEMON
	#include <wx/gdicmn.h>
	#include "Color.h"              // Needed for RGB, DarkenColour
#endif

#include "MuleTrayIcon.h"	// Needed for TBN_DLOAD
#include "PartFile.h"		// Interface declarations.
#include "OtherFunctions.h"	// Needed for nstrdup
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "UploadQueue.h"	// Needed for CFileHash
#include "IPFilter.h"		// Needed for CIPFilter
#include "Server.h"		// Needed for CServer
#include "ServerConnect.h"	// Needed for CServerConnect
#include "ListenSocket.h"	// Needed for CClientTCPSocket
#include "updownclient.h"	// Needed for CUpDownClient
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "MemFile.h"		// Needed for CMemFile
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "ED2KLink.h"		// Needed for CED2KLink
#include "Packet.h"		// Needed for CTag
#include "SearchList.h"		// Needed for CSearchFile
#include "GetTickCount.h"	// Needed for GetTickCount
#include "ClientList.h"		// Needed for clientlist
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP
#include <common/StringFunctions.h>	// Needed for CleanupFilename
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>		// Needed for CFormat
#include "FileFunctions.h"	// Needed for GetLastModificationTime
#include "ThreadTasks.h"			// Needed for CHashingTask/CCompletionTask
#include "GuiEvents.h"		// Needed for Notify_*

#include <map>
#include <algorithm>

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"


typedef std::list<Chunk> ChunkList;


#ifndef CLIENT_GUI
#include "InternalEvents.h"	// Needed for CMuleInternalEvent

CPartFile::CPartFile()
{
	Init();
}

CPartFile::CPartFile(CSearchFile* searchresult)
{
	Init();
	
	m_abyFileHash = searchresult->GetFileHash();
	SetFileName(searchresult->GetFileName());
	SetFileSize(searchresult->GetFileSize());
	
	for (unsigned int i = 0; i < searchresult->m_taglist.size(); ++i){
		const CTag& pTag = searchresult->m_taglist[i];
		
		bool bTagAdded = false;
		if (pTag.GetNameID() == 0 && !pTag.GetName().IsEmpty() && (pTag.IsStr() || pTag.IsInt())) {
			static const struct {
				wxString	pszName;
				uint8	nType;
			} _aMetaTags[] = 
				{
					{ wxT(FT_ED2K_MEDIA_ARTIST),  2 },
					{ wxT(FT_ED2K_MEDIA_ALBUM),   2 },
					{ wxT(FT_ED2K_MEDIA_TITLE),   2 },
					{ wxT(FT_ED2K_MEDIA_LENGTH),  2 },
					{ wxT(FT_ED2K_MEDIA_BITRATE), 3 },
					{ wxT(FT_ED2K_MEDIA_CODEC),   2 }
				};
			
			for (unsigned int t = 0; t < itemsof(_aMetaTags); ++t) {
				if (	pTag.GetType() == _aMetaTags[t].nType &&
					(pTag.GetName() == _aMetaTags[t].pszName)) {
					// skip string tags with empty string values
					if (pTag.IsStr() && pTag.GetStr().IsEmpty()) {
						break;
					}

					// skip "length" tags with "0: 0" values
					if (pTag.GetName() == wxT(FT_ED2K_MEDIA_LENGTH)) {
						if (pTag.GetStr().IsSameAs(wxT("0: 0")) ||
							pTag.GetStr().IsSameAs(wxT("0:0"))) {
							break;
						}
					}

					// skip "bitrate" tags with '0' values
					if ((pTag.GetName() == wxT(FT_ED2K_MEDIA_BITRATE)) && !pTag.GetInt()) {
						break;
					}

					AddDebugLogLineM( false, logPartFile,
						wxT("CPartFile::CPartFile(CSearchFile*): added tag ") +
						pTag.GetFullInfo() );
					m_taglist.push_back(pTag);
					bTagAdded = true;
					break;
				}
			}
		} else if (pTag.GetNameID() != 0 && pTag.GetName().IsEmpty() && (pTag.IsStr() || pTag.IsInt())) {
			static const struct {
				uint8	nID;
				uint8	nType;
			} _aMetaTags[] = 
				{
					{ FT_FILETYPE,		2 },
					{ FT_FILEFORMAT,	2 }
				};
			for (unsigned int t = 0; t < itemsof(_aMetaTags); ++t) {
				if (pTag.GetType() == _aMetaTags[t].nType && pTag.GetNameID() == _aMetaTags[t].nID) {
					// skip string tags with empty string values
					if (pTag.IsStr() && pTag.GetStr().IsEmpty()) {
						break;
					}

					AddDebugLogLineM( false, logPartFile,
						wxT("CPartFile::CPartFile(CSearchFile*): added tag ") +
						pTag.GetFullInfo() );
					m_taglist.push_back(pTag);
					bTagAdded = true;
					break;
				}
			}
		}

		if (!bTagAdded) {
			AddDebugLogLineM( false, logPartFile,
				wxT("CPartFile::CPartFile(CSearchFile*): ignored tag ") +
				pTag.GetFullInfo() );
		}
	}

	CreatePartFile();
}


CPartFile::CPartFile(const CED2KFileLink* fileLink)
{
	Init();
	
	SetFileName(fileLink->GetName());
	SetFileSize(fileLink->GetSize());
	m_abyFileHash = fileLink->GetHashKey();

	CreatePartFile();

	if (fileLink->m_hashset) {
		if (!LoadHashsetFromFile(fileLink->m_hashset, true)) {
			AddDebugLogLineM(true, logPartFile, wxT("eD2K link contained invalid hashset: ") + fileLink->GetLink());
		}
	}
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
		FlushBuffer(true);
	}
	
	if (m_hpartfile.IsOpened() && (m_hpartfile.fd() > 2)) {
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();			
	}

	DeleteContents(m_gaplist);

	std::list<PartFileBufferedData*>::iterator it = m_BufferedData_list.begin();
	for (; it != m_BufferedData_list.end(); ++it) {
		PartFileBufferedData* item = *it;

		delete[] item->data;
		delete item;
	}

	wxASSERT(m_SrcList.empty());
	wxASSERT(m_A4AFsrclist.empty());
}

void CPartFile::CreatePartFile()
{
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	do { 
		++i; 
		m_partmetfilename = wxString::Format(wxT("%03i.part.met"), i);
		m_fullname = JoinPaths(thePrefs::GetTempDir(), m_partmetfilename);
	} while (wxFileName::FileExists(m_fullname));
	
	wxString strPartName = m_partmetfilename.Left( m_partmetfilename.Length() - 4);
	m_taglist.push_back(CTagString(FT_PARTFILENAME, strPartName ));
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = GetFileSize() - 1;
	
	m_gaplist.push_back(gap);
	
	wxString strPartPath = m_fullname.Left( m_fullname.Length() - 4);
	if (m_hpartfile.Create(strPartPath, true)) {
		m_hpartfile.Close();

		if(!m_hpartfile.Open(strPartPath, CFile::read_write)) {
			AddLogLineM(false,_("ERROR: Failed to open partfile)"));
			SetPartFileStatus(PS_ERROR);
		}
	} else {
		AddLogLineM(false,_("ERROR: Failed to create partfile)"));
		SetPartFileStatus(PS_ERROR);
	}

	SetFilePath( thePrefs::GetTempDir() );
			
	if (thePrefs::GetAllocFullPart()) {
		#warning Code for full file alloc - should be done on thread.
	}
	
	
	m_hashsetneeded = GetED2KPartHashCount();
	
	SavePartFile(true);
}


uint8 CPartFile::LoadPartFile(const wxString& in_directory, const wxString& filename, bool from_backup, bool getsizeonly)
{
	bool isnewstyle = false;
	uint8 version,partmettype=PMT_UNKNOWN;
	
	std::map<uint16, Gap_Struct*> gap_map; // Slugfiller
	transfered = 0;
	
	m_partmetfilename = filename;
	m_strFilePath = in_directory;
	m_fullname = JoinPaths(m_strFilePath, m_partmetfilename);
	
	CFile metFile;

	// readfile data form part.met file
	wxString file_to_open;
	if (from_backup) {
		file_to_open = m_fullname + PARTMET_BAK_EXT;
	} else {
		file_to_open = m_fullname;
	}
	if (!metFile.Open(file_to_open,CFile::read)) {
		if (from_backup) {
			AddLogLineM(false, 
				_("Error: Failed to load backup file. "
				"Search http://forum.amule.org for .part.met recovery solutions"));
		} else {
			AddLogLineM(false, CFormat( _("Error: Failed to open part.met file: %s ==> %s") )
				% m_partmetfilename
				% GetFileName() );
		}
		return false;
	} else {
		if (!(metFile.GetLength()>0)) {
			if (from_backup) {
				AddLogLineM(false, _(
					"Error: Backup part.met file is 0 size! "
					"Search http://forum.amule.org for .part.met recovery solutions"));	
			} else {
				AddLogLineM(false, CFormat( _("Error: part.met file is 0 size: %s ==> %s") )
					% m_partmetfilename
					% GetFileName() );
			}
			metFile.Close();
			return false;
		}
	}

	if (from_backup) {
		AddLogLineM(false, CFormat( _("Trying to load backup of met-file from %s") )
			% ( m_partmetfilename + PARTMET_BAK_EXT) );
		wxString BackupFile;
		BackupFile = m_fullname + PARTMET_BAK_EXT;
		if (!metFile.Open(BackupFile)) {
			AddLogLineM(false, _(
				"Error: Failed to load backup file. "
				"Search http://forum.amule.org for .part.met recovery solutions"));				
			return false;
		} else {
			if (!(metFile.GetLength()>0)) {
				AddLogLineM(false, CFormat( _("Error: part.met backup file is 0 size: %s ==> %s") )
					% m_partmetfilename
					% GetFileName() );
				metFile.Close();
				return false;
			}
		}
	}
	
	try {
		version = metFile.ReadUInt8();
		if (version != PARTFILE_VERSION && version != PARTFILE_SPLITTEDVERSION && version != PARTFILE_VERSION_LARGEFILE){
			metFile.Close();
			//if (version == 83) return ImportShareazaTempFile(...)
			AddLogLineM(false, CFormat( _("Error: Invalid part.met fileversion: %s ==> %s") )
				% m_partmetfilename 
				% GetFileName() );
			return false;
		}

		isnewstyle = (version == PARTFILE_SPLITTEDVERSION);
		partmettype = isnewstyle ? PMT_SPLITTED : PMT_DEFAULTOLD;
		
		if (!isnewstyle) {
			uint8 test[4];
			metFile.Seek(24, wxFromStart);
			metFile.Read(test,4);
		
			metFile.Seek(1, wxFromStart);
			if (test[0]==0 && test[1]==0 && test[2]==2 && test[3]==1) {
				isnewstyle=true;	// edonkeys so called "old part style"
				partmettype=PMT_NEWOLD;
			}
		}
		
		if (isnewstyle) {
			uint32 temp = metFile.ReadUInt32();
	
			if (temp==0) {	// 0.48 partmets - different again
				LoadHashsetFromFile(&metFile, false);
			} else {
				metFile.Seek(2, wxFromStart);
				LoadDateFromFile(&metFile);
				m_abyFileHash = metFile.ReadHash();
			}

		} else {
			LoadDateFromFile(&metFile);
			LoadHashsetFromFile(&metFile, false);
		}	

		uint32 tagcount = metFile.ReadUInt32();

		for (uint32 j = 0; j < tagcount; ++j) {
			CTag newtag(metFile,true);
			if (	!getsizeonly ||
				(getsizeonly && 
				 	(newtag.GetNameID() == FT_FILESIZE ||
					 newtag.GetNameID() == FT_FILENAME))) {
				switch(newtag.GetNameID()) {
					case FT_FILENAME: {
						if (GetFileName().IsEmpty()) {
							// If it's not empty, we already loaded the unicoded one
							SetFileName(newtag.GetStr());
						}
						break;
					}
					case FT_LASTSEENCOMPLETE: {
						lastseencomplete = newtag.GetInt();		
						break;
					}
					case FT_FILESIZE: {
						SetFileSize(newtag.GetInt());
						break;
					}
					case FT_TRANSFERED: {
						transfered = newtag.GetInt();
						break;
					}
					case FT_FILETYPE:{
						#warning needs setfiletype string
						//SetFileType(newtag.GetStr());
						break;
					}					
					case FT_CATEGORY: {
						m_category = newtag.GetInt();
						if (m_category > theApp.glob_prefs->GetCatCount() - 1 ) {
							m_category = 0;
						}
						break;
					}
					case FT_OLDDLPRIORITY:
					case FT_DLPRIORITY: {
						if (!isnewstyle){
							m_iDownPriority = newtag.GetInt();
							if( m_iDownPriority == PR_AUTO ){
								m_iDownPriority = PR_HIGH;
								SetAutoDownPriority(true);
							}
							else{
								if (	m_iDownPriority != PR_LOW &&
									m_iDownPriority != PR_NORMAL &&
									m_iDownPriority != PR_HIGH)
									m_iDownPriority = PR_NORMAL;
								SetAutoDownPriority(false);
							}
						}
						break;
					}
					case FT_STATUS: {
						m_paused = newtag.GetInt();
						m_stopped = m_paused;
						break;
					}
					case FT_OLDULPRIORITY:
					case FT_ULPRIORITY: {			
						if (!isnewstyle){
							SetUpPriority(newtag.GetInt(), false);
							if( GetUpPriority() == PR_AUTO ){
								SetUpPriority(PR_HIGH, false);
								SetAutoUpPriority(true);
							} else {
								SetAutoUpPriority(false);
							}
						}					
						break;
					}				
					case FT_KADLASTPUBLISHSRC:{
						SetLastPublishTimeKadSrc(newtag.GetInt(), 0);
						if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES) {
							//There may be a posibility of an older client that saved a random number here.. This will check for that..
							SetLastPublishTimeKadSrc(0,0);
						}
						break;
					}
					case FT_KADLASTPUBLISHNOTES:{
						SetLastPublishTimeKadNotes(newtag.GetInt());
						break;
					}					
					// old tags: as long as they are not needed, take the chance to purge them
					case FT_PERMISSIONS:
					case FT_KADLASTPUBLISHKEY:
						break;
					case FT_CORRUPTEDPARTS: {
						wxASSERT(m_corrupted_list.empty());
						wxString strCorruptedParts(newtag.GetStr());
						wxStringTokenizer tokenizer(strCorruptedParts, wxT(","));
						while ( tokenizer.HasMoreTokens() ) {
							wxString token = tokenizer.GetNextToken();
							unsigned long uPart;
							if (token.ToULong(&uPart)) {
								if (uPart < GetPartCount() && !IsCorruptedPart(uPart)) {
									m_corrupted_list.push_back(uPart);
								}
							}
						}
						break;
					}
					case FT_AICH_HASH:{
						CAICHHash hash;
						bool hashSizeOk =
							hash.DecodeBase32(newtag.GetStr()) == CAICHHash::GetHashSize();
						wxASSERT(hashSizeOk);
						if (hashSizeOk) {
							m_pAICHHashSet->SetMasterHash(hash, AICH_VERIFIED);
						}
						break;
					}
					case FT_ATTRANSFERED:{
						statistic.SetAllTimeTransfered(statistic.GetAllTimeTransfered() + (uint64)newtag.GetInt());
						break;
					}
					case FT_ATTRANSFEREDHI:{
						statistic.SetAllTimeTransfered(statistic.GetAllTimeTransfered() + (((uint64)newtag.GetInt()) << 32));	
						break;
					}
					case FT_ATREQUESTED:{
						statistic.SetAllTimeRequests(newtag.GetInt());
						break;
					}
					case FT_ATACCEPTED:{
						statistic.SetAllTimeAccepts(newtag.GetInt());
						break;
					}
					default: {
						// Start Changes by Slugfiller for better exception handling
						
						wxCharBuffer tag_ansi_name = newtag.GetName().ToAscii();
						char gap_mark = tag_ansi_name ? tag_ansi_name[0u] : 0;
						if ( newtag.IsInt() && (newtag.GetName().Length() > 1) &&
							((gap_mark == FT_GAPSTART) ||
							 (gap_mark == FT_GAPEND))) {
							Gap_Struct *gap = NULL;
							unsigned long int gapkey;
							if (newtag.GetName().Mid(1).ToULong(&gapkey)) {
								if ( gap_map.find( gapkey ) == gap_map.end() ) {
									gap = new Gap_Struct;
									gap_map[gapkey] = gap;
									gap->start = (uint64)-1;
									gap->end = (uint64)-1;
								} else {
									gap = gap_map[ gapkey ];
								}
								if (gap_mark == FT_GAPSTART) {
									gap->start = newtag.GetInt();
								}
								if (gap_mark == FT_GAPEND) {
									gap->end = newtag.GetInt()-1;
								}
							} else {
								printf("Wrong gap map key while reading met file!\n");
								wxASSERT(0);
							}
							// End Changes by Slugfiller for better exception handling
						} else {
							m_taglist.push_back(newtag);
						}
					}
				}
			} else {
				// Nothing. Else, nothing.
			}
		}
		
		// load the hashsets from the hybridstylepartmet
		if (isnewstyle && !getsizeonly && (metFile.GetPosition()<metFile.GetLength()) ) {
			metFile.Seek(1, wxFromCurrent);
			
			uint16 parts=GetPartCount();	// assuming we will get all hashsets
			
			for (uint16 i = 0; i < parts && (metFile.GetPosition()+16<metFile.GetLength()); ++i){
				CMD4Hash cur_hash = metFile.ReadHash();
				m_hashlist.push_back(cur_hash);
			}

			CMD4Hash checkhash;
			if (!m_hashlist.empty()){
				byte buffer[m_hashlist.size() * 16];
				for (size_t i = 0; i < m_hashlist.size(); ++i)
					md4cpy(buffer+(i*16), m_hashlist[i].GetHash());
				CreateHashFromString(buffer, m_hashlist.size()*16, checkhash.GetHash());
			}
			bool flag=false;
			if (m_abyFileHash == checkhash) {
				flag=true;
			} else {
				m_hashlist.clear();
				flag=false;
			}
		}			
			
		metFile.Close();
	} catch (const CInvalidPacket& e) {
		AddLogLineM(true, CFormat(wxT("Error: %s (%s) is corrupt (bad tags: %s), unable to load file.")) 
			% m_partmetfilename
			% GetFileName()
			% e.what());
		return false;		
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile, CFormat( wxT("IO failure while loading '%s': %s") )
			% m_partmetfilename
			% e.what() );
		return false;
	} catch (const CEOFException& e) {
		AddLogLineM(true, CFormat( _("Error: %s (%s) is corrupt (wrong tagcount), unable to load file.") )
			% m_partmetfilename
			% GetFileName() );
		AddLogLineM(true, _("Trying to recover file info..."));
		
		// Safe file is that who have 
		// - FileSize
		if (GetFileSize()) {
			// We have filesize, try other needed info

			// Do we need to check gaps? I think not,
			// because they are checked below. Worst 
			// scenario will only mark file as 0 bytes downloaded.
				
			// -Filename
			if (GetFileName().IsEmpty()) {
				// Not critical, let's put a random filename.
				AddLogLineM(true, _(
					"Recovering no-named file - will try to recover it as RecoveredFile.dat"));
				SetFileName(wxT("RecoveredFile.dat"));
			}
		
			AddLogLineM(true,
				_("Recovered all available file info :D - Trying to use it..."));
		} else {
			AddLogLineM(true, _("Unable to recover file info :("));
			return false;			
		}		
	}

	if (getsizeonly) {
		return partmettype;
	}
	// Now to flush the map into the list (Slugfiller)
	std::map<uint16, Gap_Struct*>::iterator it = gap_map.begin();
	for ( ; it != gap_map.end(); ++it ) {
		Gap_Struct* gap = it->second;
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (	((int)gap->start) != -1 &&
			((int)gap->end) != -1 &&
			gap->start <= gap->end &&
			gap->start < GetFileSize()) {
			if (gap->end >= GetFileSize()) {
				gap->end = GetFileSize()-1; // Clipping
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
		AddLogLineM(false, CFormat( _("Failed to open %s (%s)") )
			% m_fullname
			% GetFileName() );
		return false;
	}
	
	// Update last-changed date if anything has been written
	if ( transfered ) {
		m_lastDateChanged = wxFileName( strSearchPath ).GetModificationTime();
	}

	// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
	if (m_hpartfile.GetLength() < GetFileSize())
		AddGap(m_hpartfile.GetLength(), GetFileSize()-1);
	// Goes both ways - Partfile should never be too large
	if (m_hpartfile.GetLength() > GetFileSize()){
		AddDebugLogLineM( true, logPartFile, CFormat( wxT("Partfile \"%s\" is too large! Truncating %llu bytes.") ) % GetFileName() % (m_hpartfile.GetLength() - GetFileSize()));
		m_hpartfile.SetLength(GetFileSize());
	}
	// SLUGFILLER: SafeHash

	SetPartFileStatus(PS_EMPTY);
	
	// check hashcount, file status etc
	if (GetHashCount() != GetED2KPartHashCount()){	
		m_hashsetneeded = true;
		return true;
	} else {
		m_hashsetneeded = false;
		for (size_t i = 0; i < m_hashlist.size(); ++i) {
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
				SetPartFileStatus(PS_READY);
			}
		}
	}
	
	if (m_gaplist.empty()) { // is this file complete already?
		CompleteFile(false);
		return true;
	}

	if (!isnewstyle) { // not for importing	
		time_t file_date = GetLastModificationTime(m_fullname);
		if (	(((time_t)date) < (time_t)(file_date - 10)) ||
			(((time_t)date) > (time_t)(file_date + 10))) {
			AddLogLineM(false, CFormat( _("Warning: %s might be corrupted (%i)") )
				% m_fullname 
				% (date - file_date) );
			// rehash
			SetPartFileStatus(PS_WAITINGFORHASH);
			
			wxString strPartFileName = m_partmetfilename.Left( m_partmetfilename.Length() - 4 );
			CThreadScheduler::AddTask(new CHashingTask(m_strFilePath, strPartFileName, this));
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
	wxLongLong free = 0;
	if (wxGetDiskSpace( GetFilePath(), NULL, &free) && free < 5000 ) {
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
		file.WriteUInt8(IsLargeFile() ? PARTFILE_VERSION_LARGEFILE : PARTFILE_VERSION);
		
		file.WriteUInt32(GetLastModificationTime(m_fullname));
		// hash
		file.WriteHash(m_abyFileHash);
		uint16 parts = m_hashlist.size();
		file.WriteUInt16(parts);
		for (int x = 0; x < parts; ++x) {
			file.WriteHash(m_hashlist[x]);
		}
		// tags		
		#define FIXED_TAGS 15
		uint32 tagcount = m_taglist.size() + FIXED_TAGS + (m_gaplist.size()*2);
		if (not m_corrupted_list.empty()) {			
			++tagcount;
		}
		
		if (m_pAICHHashSet->HasValidMasterHash() && (m_pAICHHashSet->GetStatus() == AICH_VERIFIED)){			
			++tagcount;
		}
		
		if (GetLastPublishTimeKadSrc()){
			++tagcount;
		}

		if (GetLastPublishTimeKadNotes()){
			++tagcount;
		}

		file.WriteUInt32(tagcount);

		#warning Kry - Where are lost by coruption and gained by compression?
		
		// 0 (unicoded part file name) 
		// We write it with BOM to kep eMule compatibility
		CTagString( FT_FILENAME, GetFileName() ).WriteTagToFile( &file, utf8strOptBOM );

		CTagString( FT_FILENAME,		GetFileName()	).WriteTagToFile( &file );	// 1

		CTagIntSized( FT_FILESIZE,		GetFileSize()		, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );	// 2

		CTagIntSized( FT_TRANSFERED,	transfered		, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );	// 3
		
		CTagInt32( FT_STATUS,		(m_paused?1:0)	).WriteTagToFile( &file );	// 4

		if ( IsAutoDownPriority() ) {
			CTagInt32( FT_DLPRIORITY,	(uint8)PR_AUTO	).WriteTagToFile( &file );	// 5
			CTagInt32( FT_OLDDLPRIORITY,	(uint8)PR_AUTO	).WriteTagToFile( &file );	// 6
		} else {
			CTagInt32( FT_DLPRIORITY,	m_iDownPriority	).WriteTagToFile( &file );	// 5
			CTagInt32( FT_OLDDLPRIORITY,	m_iDownPriority	).WriteTagToFile( &file );	// 6
		}

		CTagInt32( FT_LASTSEENCOMPLETE,	lsc			).WriteTagToFile( &file );	// 7

		if ( IsAutoUpPriority() ) {
			CTagInt32( FT_ULPRIORITY,	(uint8)PR_AUTO	).WriteTagToFile( &file );	// 8
			CTagInt32( FT_OLDULPRIORITY,	(uint8)PR_AUTO	).WriteTagToFile( &file );	// 9
		} else {
			CTagInt32( FT_ULPRIORITY,	GetUpPriority() ).WriteTagToFile( &file );	// 8
			CTagInt32( FT_OLDULPRIORITY,	GetUpPriority() ).WriteTagToFile( &file );	// 9
		}
	
		CTagInt32( FT_CATEGORY, 		m_category		).WriteTagToFile( &file ); 	// 10
		
		CTagInt32( FT_ATTRANSFERED,	statistic.GetAllTimeTransfered() & 0xFFFFFFFF	).WriteTagToFile( &file );	// 11
		
		CTagInt32( FT_ATTRANSFEREDHI,	statistic.GetAllTimeTransfered() >>32		).WriteTagToFile( &file );	// 12
		
		CTagInt32( FT_ATREQUESTED,	statistic.GetAllTimeRequests()		).WriteTagToFile( &file );	// 13
		
		CTagInt32( FT_ATACCEPTED,	statistic.GetAllTimeAccepts()		).WriteTagToFile( &file );	// 14

		// currupt part infos
		if (not m_corrupted_list.empty()) {
			wxString strCorruptedParts;
			std::list<uint16>::iterator it = m_corrupted_list.begin();
			for (; it != m_corrupted_list.end(); ++it) {
				uint16 uCorruptedPart = *it;
				if (!strCorruptedParts.IsEmpty()) {
					strCorruptedParts += wxT(",");
				}
				strCorruptedParts += wxString::Format(wxT("%u"), (unsigned)uCorruptedPart);
			}
			wxASSERT( !strCorruptedParts.IsEmpty() );
			
			CTagString( FT_CORRUPTEDPARTS, strCorruptedParts ).WriteTagToFile( &file); // 11?
		}

		//AICH Filehash
		if (m_pAICHHashSet->HasValidMasterHash() && (m_pAICHHashSet->GetStatus() == AICH_VERIFIED)){
			CTagString aichtag(FT_AICH_HASH, m_pAICHHashSet->GetMasterHash().GetString() );
			aichtag.WriteTagToFile(&file); // 12?
		}
		
		if (GetLastPublishTimeKadSrc()){
			CTagInt32( FT_KADLASTPUBLISHSRC,	GetLastPublishTimeKadSrc()).WriteTagToFile(&file); // 15? 
		}
		
		if (GetLastPublishTimeKadNotes()){
			CTagInt32( FT_KADLASTPUBLISHNOTES,	GetLastPublishTimeKadNotes()).WriteTagToFile(&file); // 16? 
		}		
		
		for (uint32 j = 0; j < (uint32)m_taglist.size();++j) {
			m_taglist[j].WriteTagToFile(&file);
		}
		
		// gaps
		unsigned i_pos = 0;
		std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
		for (; it != m_gaplist.end(); ++it) {
			wxString tagName = wxString::Format(wxT(" %u"), i_pos);
			
			// gap start = first missing byte but gap ends = first non-missing byte
			// in edonkey but I think its easier to user the real limits
			tagName[0] = FT_GAPSTART;
			CTagIntSized(tagName, (*it)->start		, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );
			
			tagName[0] = FT_GAPEND;
			CTagIntSized(tagName, ((*it)->end + 1), IsLargeFile() ? 64 : 32).WriteTagToFile( &file );
			
			++i_pos;
		}
	} catch (const wxString& error) {
		AddLogLineM(false, CFormat( _("ERROR while saving partfile: %s (%s ==> %s)") )
			% error
			% m_partmetfilename
			% GetFileName() );

		wxString err = CFormat( _("ERROR while saving partfile: %s (%s ==> %s)") )
			% error
			% m_partmetfilename
			% GetFileName();
		
		printf("%s\n", (const char*)unicode2char(err));
		
		return false;
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile, wxT("IO failure while saving partfile: ") + e.what());
		printf("IO failure while saving partfile: %s\n", (const char*)unicode2char(e.what()));
		
		return false;
	}
	
	file.Close();

	if (!Initial) {
		wxRemoveFile(m_fullname + wxT(".backup"));
	}
	
	// Kry -don't backup if it's 0 size but raise a warning!!!
	CFile newpartmet;
	if (newpartmet.Open(m_fullname)!=TRUE) {
		theApp.ShowAlert( CFormat( _("Unable to open %s file - using %s file.") )
			% m_fullname
			% PARTMET_BAK_EXT,
			_("Message"), wxOK);

		UTF8_CopyFile(m_fullname + PARTMET_BAK_EXT, m_fullname);
	} else {
		if (newpartmet.GetLength()>0) {			
			// not error, just backup
			newpartmet.Close();
			BackupFile(m_fullname, PARTMET_BAK_EXT);
		} else {
			newpartmet.Close();
			theApp.ShowAlert( CFormat( _("'%s' is 0 size somehow - using %s file.") )
				% m_fullname
				% PARTMET_BAK_EXT,
				_("Message"), wxOK);
					
			UTF8_CopyFile(m_fullname + PARTMET_BAK_EXT,m_fullname);
		}
	}
	
	return true;
}


void CPartFile::SaveSourceSeeds()
{
	#define MAX_SAVED_SOURCES 10
	
	// Kry - Sources seeds
	// Based on a Feature request, this saves the last MAX_SAVED_SOURCES 
	// sources of the file, giving a 'seed' for the next run.
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
	
	CClientPtrList source_seeds;
	int n_sources = 0;
	
	CClientPtrList::iterator it = m_downloadingSourcesList.begin();
	for( ; it != m_downloadingSourcesList.end() && n_sources < MAX_SAVED_SOURCES; ++it) {
		CUpDownClient *cur_src = *it;
		if (cur_src->HasLowID()) {
			continue;
		} else {
			source_seeds.push_back(cur_src);
		}
		++n_sources;
	}

	if (n_sources < MAX_SAVED_SOURCES) {
		// Not enought downloading sources to fill the list, going to sources list	
		if (GetSourceCount() > 0) {
			SourceSet::reverse_iterator rit = m_SrcList.rbegin();
			for ( ; ((rit != m_SrcList.rend()) && (n_sources<MAX_SAVED_SOURCES)); ++rit) {
				CUpDownClient* cur_src = *rit;
				if (cur_src->HasLowID()) {
					continue;
				} else {
					source_seeds.push_back(cur_src);
				}
				++n_sources;
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
		AddLogLineM(false, CFormat( _("Failed to save part.met.seeds file for %s") )
			% m_fullname);
		return;
	}	

	try {
		file.WriteUInt8(source_seeds.size());
		
		CClientPtrList::iterator it2 = source_seeds.begin();
		for (; it2 != source_seeds.end(); ++it2) {
			CUpDownClient* cur_src = *it2;		
			file.WriteUInt32(cur_src->GetUserIDHybrid());
			file.WriteUInt16(cur_src->GetUserPort());
		}

		/* v2: Added to keep track of too old seeds */
		file.WriteUInt32(wxDateTime::Now().GetTicks());
		
		AddLogLineM(false, CFormat( _("Saved %i sources seeds for partfile: %s (%s)") )
			% n_sources
			% m_fullname
			% GetFileName());
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile, CFormat( wxT("Error saving partfile's seeds file (%s - %s): %s") )
				% m_partmetfilename
				% e.what() );
		
		n_sources = 0;
		file.Close();
		wxRemoveFile(m_fullname + wxT(".seeds"));
	}
}	


void CPartFile::LoadSourceSeeds()
{	
	CFile file;
	CMemFile sources_data;
	
	bool valid_sources = false;
	
	if (!wxFileName::FileExists(m_fullname + wxT(".seeds"))) {
		return;
	} 
	
	file.Open(m_fullname + wxT(".seeds"),CFile::read);

	if (!file.IsOpened()) {
		AddLogLineM(false, CFormat( _("Partfile %s (%s) has no seeds file") )
			% m_partmetfilename
			% GetFileName() );
		return;
	}	
	
	if (file.GetLength() <= 1) {
		AddLogLineM(false, CFormat( _("Partfile %s (%s) has a void seeds file") )
			% m_partmetfilename
			% GetFileName() );
		file.Close();
		return;
	}	
		
	try {
		uint8 src_count = file.ReadUInt8();
		
		sources_data.WriteUInt16(src_count);
	
		for (int i = 0; i< src_count; ++i) {		
			uint32 dwID = file.ReadUInt32();
			uint16 nPort = file.ReadUInt16();
			
			sources_data.WriteUInt32(dwID);
			sources_data.WriteUInt16(nPort);
			sources_data.WriteUInt32(0);
			sources_data.WriteUInt16(0);	
		}	
		
		if (!file.Eof()) {
	
			// v2: Added to keep track of too old seeds 
			time_t time = (time_t)file.ReadUInt32();
	
			// Time frame is 2 hours. More than enough to compile
			// your new aMule version!.
			if ((time + MIN2S(120)) >= wxDateTime::Now().GetTicks()) {
				valid_sources = true;
			}
			
		} else {
			// v1 has no time data. We can safely use
			// the sources, next time will be saved.
			valid_sources = true;
		}
		
	} catch (const CSafeIOException& e) {
		AddLogLineM(false, CFormat( _("Error reading partfile's seeds file (%s - %s): %s") )
				% m_partmetfilename
				% GetFileName()
				% e.what() );
		return;
	}
	
	if (valid_sources) {
		sources_data.Seek(0);
		AddClientSources(&sources_data, 1, SF_SOURCE_SEEDS);		
	}
	
	file.Close();
}		

void CPartFile::PartFileHashFinished(CKnownFile* result)
{
	date = result->date;
	newdate = true;
	bool errorfound = false;
	if (GetED2KPartHashCount() == 0){
		if (IsComplete(0, GetFileSize()-1)){
			if (result->GetFileHash() != GetFileHash()){
				AddLogLineM(false, CFormat( _("Found corrupted part (%d) in %d parts file %s - FileResultHash |%s| FileHash |%s|") )
					% 1
					% 0
					% GetFileName()
					% result->GetFileHash().Encode()
					% GetFileHash().Encode() );
				AddGap(0, GetFileSize()-1);
				errorfound = true;
			}
		}
	}
	else{
		for (size_t i = 0; i < m_hashlist.size(); ++i){
			// Kry - trel_ar's completed parts check on rehashing.
			// Very nice feature, if a file is completed but .part.met don't belive it,
			// update it.
			
			if (!( i < result->GetHashCount() && (result->GetPartHash(i) == GetPartHash(i)))){
				if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)) {
					CMD4Hash wronghash;
					if ( i < result->GetHashCount() )
						wronghash = result->GetPartHash(i);
			
					AddLogLineM(false, CFormat( _("Found corrupted part (%d) in %d parts file %s - FileResultHash |%s| FileHash |%s|") )
						% ( i + 1 )
						% GetED2KPartHashCount()
						% GetFileName()
						% wronghash.Encode()
						% GetPartHash(i).Encode() );
				
					AddGap(i*PARTSIZE,
						((((i+1)*PARTSIZE)-1) >= GetFileSize()) ?
							GetFileSize()-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			} else {
				if (!IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
					AddLogLineM(false, CFormat( _("Found completed part (%i) in %s") )
						% ( i + 1 )
						% GetFileName() );

					FillGap(i*PARTSIZE,
						((((i+1)*PARTSIZE)-1) >= GetFileSize()) ?
							GetFileSize()-1 : ((i+1)*PARTSIZE)-1);
					RemoveBlockFromList(i*PARTSIZE,
						((((i+1)*PARTSIZE)-1) >= GetFileSize()) ?
							GetFileSize()-1 : ((i+1)*PARTSIZE)-1);
				}
			}						
		}
	}

	if (	!errorfound &&
		result->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE &&
		status == PS_COMPLETING) {
		delete m_pAICHHashSet;
		m_pAICHHashSet = result->GetAICHHashset();
		result->SetAICHHashset(NULL);
		m_pAICHHashSet->SetOwner(this); 
	}
	else if (status == PS_COMPLETING) {
		AddDebugLogLineM(false, logPartFile,
			wxT("Failed to store new AICH Hashset for completed file: ") +
			GetFileName());
	}

	
	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			CompleteFile(true);
			return;
		}
		else {
			AddLogLineM(false, CFormat( _("Finished rehashing %s") ) % GetFileName());
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

void CPartFile::AddGap(uint64 start, uint64 end)
{
	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	while (it != m_gaplist.end()) {
		std::list<Gap_Struct*>::iterator it2 = it++;
		Gap_Struct* cur_gap = *it2;
	
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// this gap is inside the new gap - delete
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the new gap - extend limit and delete
			end = cur_gap->end;
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the new gap - extend limit and delete
			start = cur_gap->start;
			m_gaplist.erase(it2);
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
	m_gaplist.push_back(new_gap);
	UpdateDisplayedInfo();
	newdate = true;
}

bool CPartFile::IsAlreadyRequested(uint64 start, uint64 end)
{
	std::list<Requested_Block_Struct*>::iterator it = m_requestedblocks_list.begin();
	for (; it != m_requestedblocks_list.end(); ++it) {
		Requested_Block_Struct* cur_block =  *it;

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
	uint64 end;
	uint64 blockLimit;

	// Find start of this part
	uint64 partStart = (PARTSIZE * partNumber);
	uint64 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint64 partEnd = (PARTSIZE * (partNumber + 1)) - 1;
	if (partEnd >= GetFileSize()) {
		partEnd = GetFileSize() - 1;
	}
	// Loop until find a suitable gap and return true, or no more gaps and return false
	while (true) {
		firstGap = NULL;

		// Find the first gap from the start position
		std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
		for (; it != m_gaplist.end(); ++it) {
			currentGap = *it;
			
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
				md4cpy(result->FileID, GetFileHash().GetHash());
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


void CPartFile::FillGap(uint64 start, uint64 end)
{
	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	while (it != m_gaplist.end()) {
		std::list<Gap_Struct*>::iterator it2 = it++;
		Gap_Struct* cur_gap = *it2;
	
		if (cur_gap->start >= start && cur_gap->end <= end) {
			// our part fills this gap completly
			m_gaplist.erase(it2);
			delete cur_gap;
			continue;
		} else if (cur_gap->start >= start && cur_gap->start <= end) {
			// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		} else if (cur_gap->end <= end && cur_gap->end >= start) {
			// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		} else if (start >= cur_gap->start && end <= cur_gap->end) {
			uint64 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			m_gaplist.insert(++it2, cur_gap);
			break;
		}
	}
	UpdateCompletedInfos();
	UpdateDisplayedInfo();
	newdate = true;
}


void CPartFile::UpdateCompletedInfos()
{
   	uint64 allgaps = 0;

	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); ) {
		std::list<Gap_Struct*>::iterator it2 = it++;
		Gap_Struct* cur_gap = *it2;

		if ((cur_gap->end > GetFileSize()) || (cur_gap->start >= GetFileSize())) {
			m_gaplist.erase(it2);
		} else {
			allgaps += cur_gap->end - cur_gap->start + 1;
		}
	}

	if ((not m_gaplist.empty()) || (not m_requestedblocks_list.empty())) {
		percentcompleted = (1.0f-(double)allgaps/GetFileSize()) * 100;
		completedsize = GetFileSize() - allgaps;
	} else {
		percentcompleted = 100;
		completedsize = GetFileSize();
	}
}


void CPartFile::WritePartStatus(CMemFile* file)
{
	uint16 parts = GetED2KPartCount();
	file->WriteUInt16(parts);
	uint16 done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;++i) {
			if (IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)) {
				towrite |= (1<<i);
			}
			++done;
			if (done == parts) {
				break;
			}
		}
		file->WriteUInt8(towrite);
	}
}

void CPartFile::WriteCompleteSourcesCount(CMemFile* file)
{
	file->WriteUInt16(m_nCompleteSourcesCount);
}

uint32 CPartFile::Process(uint32 reducedownload/*in percent*/,uint8 m_icounter)
{
	uint16 old_trans;
	uint32 dwCurTick = ::GetTickCount();

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if (	(m_nTotalBufferData > thePrefs::GetFileBufferSize()) ||
		(dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT))) {
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
		// Update only downloading sources.
		CClientPtrList::iterator it = m_downloadingSourcesList.begin();
		for( ; it != m_downloadingSourcesList.end(); ) {
			CUpDownClient *cur_src = *it++;
			if(cur_src->GetDownloadState() == DS_DOWNLOADING) {
				++transferingsrc;
				kBpsDown += cur_src->SetDownloadLimit(reducedownload);
			}
		}
	} else {
		// Update all sources (including downloading sources)
		for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
			CUpDownClient* cur_src = *it++;
			switch (cur_src->GetDownloadState()) {
				case DS_DOWNLOADING: {
					++transferingsrc;
					kBpsDown += cur_src->SetDownloadLimit(reducedownload);			
					break;
				}
				case DS_BANNED: {
					break;
				}
				case DS_ERROR: {
					break;
				}
				case DS_LOWTOLOWIP: {
					if ( cur_src->HasLowID() && !theApp.DoCallback( cur_src ) ) {
						// If we are almost maxed on sources,
						// slowly remove these client to see 
						// if we can find a better source.
						if( 	((dwCurTick - lastpurgetime) > 30000) &&
							(GetSourceCount() >= (thePrefs::GetMaxSourcePerFile()*.8))) {
							RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break;
						}
					} else {
						cur_src->SetDownloadState(DS_ONQUEUE);
					}
					
					break;
				}
				case DS_NONEEDEDPARTS: {
					// we try to purge noneeded source, even without reaching the limit
					if((dwCurTick - lastpurgetime) > 40000) {
						if(!cur_src->SwapToAnotherFile(false , false, false , NULL)) {
							//however we only delete them if reaching the limit
							if (GetSourceCount() >= (thePrefs::GetMaxSourcePerFile()*.8 )) {
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
					if (	!((!cur_src->GetLastAskedTime()) ||
						 (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME*2)) {
						break;
					}
					// Recheck this client to see if still NNP..
					// Set to DS_NONE so that we force a TCP reask next time..
					cur_src->SetDownloadState(DS_NONE);
					
					break;
				}
				case DS_ONQUEUE: {
					if( cur_src->IsRemoteQueueFull()) {
						if(	((dwCurTick - lastpurgetime) > 60000) &&
							(GetSourceCount() >= (thePrefs::GetMaxSourcePerFile()*.8 )) ) {
							RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break; //Johnny-B - nothing more to do here (good eye!)
						}
					} 
					
					// Give up to 1 min for UDP to respond..
					// If we are within on min on TCP, do not try..
					if (	theApp.IsConnected() &&
						(	(!cur_src->GetLastAskedTime()) ||
							(dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME-20000)) {
						cur_src->UDPReaskForDownload();
					}
					
					// No break here, since the next case takes care of asking for downloads.
				}
				case DS_CONNECTING: 
				case DS_TOOMANYCONNS: 
				case DS_NONE: 
				case DS_WAITCALLBACK: 
				case DS_WAITCALLBACKKAD:	{							
					if (	theApp.IsConnected() &&
						(	(!cur_src->GetLastAskedTime()) ||
							(dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME)) {
						if (!cur_src->AskForDownload()) {
							// I left this break here just as a reminder
							// just in case re rearange things..
							break;
						}
					}
					break;
				}
			}
		}

		/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
		if (IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000))) {
			m_LastNoNeededCheck = dwCurTick;
			for ( SourceSet::iterator it = m_A4AFsrclist.begin(); it != m_A4AFsrclist.end(); ) {
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
		if (dwCurTick > m_LastSourceDropTime + thePrefs::GetAutoDropTimer() * 1000) {
			m_LastSourceDropTime = dwCurTick;
			/* If all three are enabled, use CleanUpSources() function, will save us some CPU. */
			
			bool noNeeded  = thePrefs::DropNoNeededSources();
			bool fullQueue = thePrefs::DropFullQueueSources();
			bool highQueue = thePrefs::DropHighQueueRankingSources();
			
			if ( noNeeded || fullQueue || highQueue )
				CleanUpSources( noNeeded, fullQueue, highQueue );
		}
	
		if (((old_trans==0) && (transferingsrc>0)) || ((old_trans>0) && (transferingsrc==0))) {
			SetPartFileStatus(status);
		}
	
		// Kad source search		
		if( GetMaxSourcePerFileUDP() > GetSourceCount()){
			//Once we can handle lowID users in Kad, we remove the second IsConnected
			if (theApp.downloadqueue->DoKademliaFileRequest() && (Kademlia::CKademlia::GetTotalFile() < KADEMLIATOTALFILE) && (dwCurTick > m_LastSearchTimeKad) &&  Kademlia::CKademlia::IsConnected() && theApp.IsConnected() && !IsStopped()){ 
				//Kademlia
				theApp.downloadqueue->SetLastKademliaFileRequest();
			
				if (GetKadFileSearchID()) {
					/*	This will never happen anyway. We're talking a 
						1h timespan and searches are at max 45secs */
					Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), false);
				}
			
				Kademlia::CUInt128 kadFileID(GetFileHash().GetHash());
				Kademlia::CSearch* pSearch = Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::FILE, true, kadFileID);
				AddDebugLogLineM(false, logKadSearch, wxT("Preparing a Kad Search for ") + GetFileName());
				if (pSearch) {
					AddDebugLogLineM(false, logKadSearch, wxT("Kad lookup started for ") + GetFileName());
					if(m_TotalSearchesKad < 7) {
						m_TotalSearchesKad++;
					}
					m_LastSearchTimeKad = dwCurTick + (KADEMLIAREASKTIME*m_TotalSearchesKad);
					SetKadFileSearchID(pSearch->GetSearchID());
				}
			}
		} else {
			if(GetKadFileSearchID()) {
				Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), true);
			}
		}
		
		// check if we want new sources from server
		if (	!m_localSrcReqQueued &&
			(	(!m_lastsearchtime) ||
				(dwCurTick - m_lastsearchtime) > SERVERREASKTIME) &&
			theApp.IsConnectedED2K() &&
			thePrefs::GetMaxSourcePerFileSoft() > GetSourceCount() &&
			!m_stopped ) {
			m_localSrcReqQueued = true;
			theApp.downloadqueue->SendLocalSrcRequest(this);
		}
	
		// calculate datarate, set limit etc.
	}

	++m_count;
	
	// Kry - does the 3 / 30 difference produce too much flickering or CPU?
	if (m_count >= 30) {
		m_count = 0;
		UpdateAutoDownPriority();
		UpdateDisplayedInfo();
		if(m_bPercentUpdated == false) {
			UpdateCompletedInfos();
		}
		m_bPercentUpdated = false;
		if (thePrefs::ShowCatTabInfos()) {
			Notify_ShowUpdateCatTabTitles();
		}				
	}
	
	return (uint32)(kBpsDown*1024.0);
}

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped, bool ed2kID)
{

	//The incoming ID could have the userid in the Hybrid format.. 
	uint32 hybridID = 0;
	if( ed2kID ) {
		if (IsLowID(userid)) {
			hybridID = userid;
		} else {
			hybridID = wxUINT32_SWAP_ALWAYS(userid);
		}
	} else {
		hybridID = userid;
		if (!IsLowID(userid)) {
			userid = wxUINT32_SWAP_ALWAYS(userid);
		}
	}
	
	// MOD Note: Do not change this part - Merkur
	if (theApp.IsConnectedED2K()) {
		if(::IsLowID(theApp.GetED2KID())) {
			if(theApp.GetED2KID() == userid && theApp.serverconnect->GetCurrentServer()->GetIP() == serverip && theApp.serverconnect->GetCurrentServer()->GetPort() == serverport ) {
				return false;
			}
			if(theApp.GetPublicIP() == userid) {
				return false;
			}
		} else {
			if(theApp.GetED2KID() == userid && thePrefs::GetPort() == port) {
				return false;
			}
		}
	}
	
	if (Kademlia::CKademlia::IsConnected()) {
		if(!Kademlia::CKademlia::IsFirewalled()) {
			if(Kademlia::CKademlia::GetIPAddress() == hybridID && thePrefs::GetPort() == port) {
				return false;
			}
		}
	}

	//This allows *.*.*.0 clients to not be removed if Ed2kID == false
	if ( IsLowID(hybridID) && theApp.IsFirewalled()) {
		if (pdebug_lowiddropped) {
			(*pdebug_lowiddropped)++;
		}
		return false;
	}
	// MOD Note - end
	return true;
}

void CPartFile::AddSources(CMemFile& sources,uint32 serverip, uint16 serverport, unsigned origin)
{
	uint8 count = sources.ReadUInt8();
	uint8 debug_lowiddropped = 0;
	uint8 debug_possiblesources = 0;

	if (m_stopped) {
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		AddDebugLogLineM(false, logPartFile, wxT("Trying to add sources for a stopped file"));
		sources.Seek(count*(4+2), wxFromCurrent);
		return;
	}

	for (int i = 0;i != count;++i) {
		uint32 userid = sources.ReadUInt32();
		uint16 port   = sources.ReadUInt16();
		
		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (!IsLowID(userid)) {
			// check for 0-IP, localhost and optionally for LAN addresses
			if ( !IsGoodIP(userid, thePrefs::FilterLanIPs()) ) {
				continue;
			}
			if (theApp.ipfilter->IsFiltered(userid)) {
				continue;
			}
		}

		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped)) {
			continue;
		}
		if(thePrefs::GetMaxSourcePerFile() > GetSourceCount()) {
			++debug_possiblesources;
			CUpDownClient* newsource = new CUpDownClient(port,userid,serverip,serverport,this, true, true);
			newsource->SetSourceFrom((ESourceFrom)origin);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} else {
			AddDebugLogLineM(false, logPartFile, wxT("Consuming a packet because of max sources reached"));
			// Since we may receive multiple search source UDP results we have to "consume" all data of that packet
			// This '+1' is added because 'i' counts from 0.
			sources.Seek((count-(i+1))*(4+2), wxFromCurrent);
			if (GetKadFileSearchID()) {
				Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), false);
			}
			break;
		}
	}
}

void CPartFile::UpdatePartsInfo()
{
	if( !IsPartFile() ) {
		CKnownFile::UpdatePartsInfo();
		return;
	}
	
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	// Ensure the frequency-list is ready
	if ( m_SrcpartFrequency.size() != GetPartCount() ) {
		m_SrcpartFrequency.clear();
		m_SrcpartFrequency.insert(m_SrcpartFrequency.begin(), GetPartCount(), 0);
	}

	// Find number of available parts
	uint16 availablecounter = 0;
	for ( uint16 i = 0; i < partcount; ++i ) {		
		if ( m_SrcpartFrequency[i] )
			++availablecounter;
	}
	
	if ( ( availablecounter == partcount ) && ( m_availablePartsCount < partcount ) ) {
		lastseencomplete = time(NULL);
	}
		
	m_availablePartsCount = availablecounter;

	if ( flag ) {
		ArrayOfUInts16 count;	
	
		count.reserve(GetSourceCount());	
	
		for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ) {
			if ( !(*it)->GetUpPartStatus().empty() && (*it)->GetUpPartCount() == partcount ) {
				count.push_back((*it)->GetUpCompleteSourcesCount());
			}
		}
	
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; ++i)	{
			if( !i )	{
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i]) {
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
		}
		count.push_back(m_nCompleteSourcesCount);
	
		int32 n = count.size();
		if (n > 0) {
			std::sort(count.begin(), count.end(), std::less<uint16>());
			
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
					m_nCompleteSourcesCountLo =
						(uint16)((float)(count[i]*.8) +
							 (float)(m_nCompleteSourcesCount*.2));
				}
				m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi =
					(uint16)((float)(count[j]*.8) +
						 (float)(m_nCompleteSourcesCount*.2));
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
	if ( sender->GetPartStatus().empty() ) {
		return false;
	}
	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	ChunkList chunksList;
	
	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != *count) {
		// Create a request block stucture if a chunk has been previously selected
		if(sender->GetLastPartAsked() != 0xffff) {
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->GetLastPartAsked(), pBlock) == true) {
				// Keep a track of all pending requested blocks
				m_requestedblocks_list.push_back(pBlock);
				// Update list of blocks to return
				newblocks[newBlockCount++] = pBlock;
				// Skip end of loop (=> CPU load)
				continue;
			} else {
				// All blocks for this chunk have been already requested
				delete pBlock;
				// => Try to select another chunk
				sender->SetLastPartAsked(0xffff);
			}
		}

		// Check if a new chunk must be selected (e.g. download starting, previous chunk complete)
		if(sender->GetLastPartAsked() == 0xffff) {
			// Quantify all chunks (create list of chunks to download) 
			// This is done only one time and only if it is necessary (=> CPU load)
			if(chunksList.empty()) {
				// Indentify the locally missing part(s) that this source has
				for(uint16 i=0; i < partCount; ++i) {
					if(sender->IsPartAvailable(i) == true && GetNextEmptyBlockInPart(i, NULL) == true) {
						// Create a new entry for this chunk and add it to the list
						Chunk newEntry;
						newEntry.part = i;
						newEntry.frequency = m_SrcpartFrequency[i];
						chunksList.push_back(newEntry);
					}
				}

				// Check if any bloks(s) could be downloaded
				if(chunksList.empty()) {
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
				const bool isPreviewEnable =
					thePrefs::GetPreviewPrio() &&
					(type == ftArchive || type == ftVideo);
					
				// Collect and calculate criteria for all chunks
				for (ChunkList::iterator it = chunksList.begin(); it != chunksList.end(); ++it) {
					Chunk& cur_chunk = *it;
					
					// Offsets of chunk
					const uint64 uStart = cur_chunk.part * PARTSIZE;
					const uint64 uEnd  =
						((GetFileSize() - 1) < (uStart + PARTSIZE - 1)) ?
							(GetFileSize() - 1) : (uStart + PARTSIZE - 1);
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
					const bool critRequested =
						cur_chunk.frequency > veryRareBound &&
						IsAlreadyRequested(uStart, uEnd);

					// Criterion 4. Completion
					uint64 partSize = PARTSIZE;

					std::list<Gap_Struct*>::iterator it2 = m_gaplist.begin();
					for (; it2 != m_gaplist.end(); ++it2) {
						const Gap_Struct* cur_gap = *it2;
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
			if(not chunksList.empty()) {
				// Find and count the chunck(s) with the highest priority
				uint16 chunkCount = 0; // Number of found chunks with same priority
				uint16 rank = 0xffff; // Highest priority found

				// Collect and calculate criteria for all chunks
				for (ChunkList::iterator it = chunksList.begin(); it != chunksList.end(); ++it) {
					const Chunk& cur_chunk = *it;
					if(cur_chunk.rank < rank) {
						chunkCount = 1;
						rank = cur_chunk.rank;
					} else if(cur_chunk.rank == rank) {
						++chunkCount;
					}
				}

				// Use a random access to avoid that everybody tries to download the 
				// same chunks at the same time (=> spread the selected chunk among clients)
				uint16 randomness = 1 + (int) (((float)(chunkCount-1))*rand()/(RAND_MAX+1.0));

				for (ChunkList::iterator it = chunksList.begin(); it != chunksList.end(); ++it) {
					const Chunk& cur_chunk = *it;
					if(cur_chunk.rank == rank) {
						randomness--;
						if(randomness == 0) {
							// Selection process is over
							sender->SetLastPartAsked(cur_chunk.part);
							// Remark: this list might be reused up to *count times
							chunksList.erase(it);
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


void  CPartFile::RemoveBlockFromList(uint64 start,uint64 end)
{
	std::list<Requested_Block_Struct*>::iterator it = m_requestedblocks_list.begin();
	while (it != m_requestedblocks_list.end()) {
		std::list<Requested_Block_Struct*>::iterator it2 = it++;

		if ((*it2)->StartOffset <= start && (*it2)->EndOffset >= end) {
			m_requestedblocks_list.erase(it2);
		}
	}
}


void CPartFile::RemoveAllRequestedBlocks(void)
{
	m_requestedblocks_list.clear();
}


void CPartFile::CompleteFile(bool bIsHashingDone)
{
	if (GetKadFileSearchID()) {
		Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), false);
	}

	theApp.downloadqueue->RemoveLocalServerRequest(this);

	AddDebugLogLineM( false, logPartFile, wxString( wxT("CPartFile::CompleteFile: Hash ") ) + ( bIsHashingDone ? wxT("done") : wxT("not done") ) );
			  
	if (!bIsHashingDone) {
		SetPartFileStatus(PS_COMPLETING);
		kBpsDown = 0.0;

		wxString strPartFile = m_partmetfilename.Left( m_partmetfilename.Length() - 4 );
		CThreadScheduler::AddTask(new CHashingTask(GetFilePath(), strPartFile, this));
		return;
	} else {
		StopFile();
		m_is_A4AF_auto=false;
		SetPartFileStatus(PS_COMPLETING);
		// guess I was wrong about not need to spaw a thread ...
		// It is if the temp and incoming dirs are on different
		// partitions/drives and the file is large...[oz]
		//

		PerformFileComplete();


	}
	if (thePrefs::ShowCatTabInfos()) {
		Notify_ShowUpdateCatTabTitles();
	}			
	UpdateDisplayedInfo(true);
}


void CPartFile::CompleteFileEnded(bool errorOccured, const wxString& newname)
{	
	if (errorOccured) {
		m_paused = true;
		SetPartFileStatus(PS_ERROR);
		AddLogLineM(true, CFormat( _("Unexpected file error while completing %s. File paused") )% GetFileName() );
	} else {
		m_fullname = newname;

		SetFilePath(wxFileName(m_fullname).GetPath());
		SetFileName(wxFileName(m_fullname).GetFullName());
		
		SetPartFileStatus(PS_COMPLETE);
		m_paused = false;
		ClearPriority();
		
		// TODO: What the f*** if it is already known?
		theApp.knownfiles->SafeAddKFile(this);
		
		// remove the file from the suspended uploads list
		theApp.uploadqueue->ResumeUpload(GetFileHash());		
		theApp.downloadqueue->RemoveFile(this);
		theApp.sharedfiles->SafeAddKFile(this);
		UpdateDisplayedInfo(true);

		// republish that file to the ed2k-server to update the 'FT_COMPLETE_SOURCES' counter on the server.
		theApp.sharedfiles->RepublishFile(this);		
		
		// Ensure that completed shows the correct value
		completedsize = GetFileSize();

		AddLogLineM(true, CFormat( _("Finished downloading: %s") ) % GetFileName() );
	}
	
	theApp.downloadqueue->StartNextFile(this);	
}


void CPartFile::PerformFileComplete()
{
	// add this file to the suspended uploads list
	theApp.uploadqueue->SuspendUpload(GetFileHash());
	FlushBuffer();

	// close permanent handle
	if (m_hpartfile.IsOpened()) {
		m_hpartfile.Close();
	}

	// Schedule task for completion of the file
	CThreadScheduler::AddTask(new CCompletionTask(this));
}


void  CPartFile::RemoveAllSources(bool bTryToSwap)
{
	for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end();) {
		CUpDownClient* cur_src = *it++;
		if (bTryToSwap) {
			if (!cur_src->SwapToAnotherFile(true, true, true, NULL)) {
				RemoveSource(cur_src,true,false);
				// If it was not swapped, it's not on any file anymore, and should die 
			}
		} else {
			RemoveSource(cur_src,true,false);
		}
	}

	UpdatePartsInfo(); 
	
	/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
	// remove all links A4AF in sources to this file
	if(!m_A4AFsrclist.empty()) {
		for( SourceSet::iterator it = m_A4AFsrclist.begin(); it != m_A4AFsrclist.end(); ) {
			CUpDownClient* cur_src = *it++;
			if ( cur_src->DeleteFileRequest( this ) ) {
				Notify_DownloadCtrlRemoveSource(cur_src, this);
			}
		}
		m_A4AFsrclist.clear();
	}
	/* eMule 0.30c implementation, i give it a try (Creteil) END ... */
	UpdateFileRatingCommentAvail();
}


void CPartFile::Delete()
{
	AddLogLineM(false, CFormat(_("Deleting file: %s")) % GetFileName());
	// Barry - Need to tell any connected clients to stop sending the file
	StopFile(true);
	AddDebugLogLineM(false, logPartFile, wxT("\tStopped"));
	
	theApp.sharedfiles->RemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved from shared"));
	theApp.downloadqueue->RemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved from download queue"));
	Notify_DownloadCtrlRemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved transferwnd"));

	// Kry - WTF? 
	// eMule had same problem with lseek error ... and override with a simple 
	// check for INVALID_HANDLE_VALUE (that, btw, does not exist on linux)
	// So we just guess is < 0 on error and > 2 if ok (0 stdin, 1 stdout, 2 stderr)
	if (m_hpartfile.fd() > 2) {  // 0 stdin, 1 stdout, 2 stderr
		m_hpartfile.Close();
	}

	AddDebugLogLineM(false, logPartFile, wxT("\tClosed"));
	
	if (!wxRemoveFile(m_fullname)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("\tFailed to delete '%s'")) % m_fullname);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .part.met"));
	}

	wxString strPartFile = m_fullname.Left(m_fullname.Length() - 4);
	
	if (!wxRemoveFile(strPartFile)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("Failed to delete '%s'")) % strPartFile);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .part"));
	}
	
	wxString BAKName = m_fullname + PARTMET_BAK_EXT;

	if (!wxRemoveFile(BAKName)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("Failed to delete '%s'")) % BAKName);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .BAK"));
	}
	
	wxString SEEDSName = m_fullname + wxT(".seeds");
	
	if (wxFileName::FileExists(SEEDSName)) {
		if (wxRemoveFile(SEEDSName)) {
			AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .seeds"));
		} else {
			AddDebugLogLineM(true, logPartFile, CFormat(wxT("Failed to delete '%s'")) % SEEDSName);
		}
	}

	AddDebugLogLineM(false, logPartFile, wxT("Done"));
	
	delete this;
}


bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)) {
		AddLogLineM(true,
			CFormat( _("Warning: Unable to hash downloaded part - hashset incomplete for '%s'") )
				% GetFileName() );
		m_hashsetneeded = true;
		return true;
	} else if ((GetHashCount() <= partnumber) && GetPartCount() != 1) {
		AddLogLineM(true, CFormat( _("Error: Unable to hash downloaded part - hashset incomplete (%s). This should never happen")) % GetFileName() );
		m_hashsetneeded = true;
		return true;		
	} else {
		CMD4Hash hashresult;
		m_hpartfile.Seek(PARTSIZE * partnumber, wxFromStart);
		uint64 length = PARTSIZE;
		if ((PARTSIZE * (partnumber + 1)) > m_hpartfile.GetLength()){
			length = (m_hpartfile.GetLength() - (PARTSIZE * partnumber));
			wxASSERT( length <= PARTSIZE );
		}
		
		try {
			CreateHashFromFile(&m_hpartfile, length, hashresult.GetHash());
		} catch (const CIOFailureException& e) {
			AddLogLineM(true, CFormat( wxT("IO failure while hashing downloaded part of partfile '%s': %s"))
				% GetFileName() % e.what());
			return false;
		} catch (const CEOFException& e) {
			AddLogLineM(true, CFormat( wxT("Critical IO failure (EOF) while hashing downloaded part %u with length %u (max %u) of partfile '%s' with length %u: %s"))
				% partnumber % length % ((partnumber*PARTSIZE)+length) % GetFileName() % GetFileSize() % e.what());
			return false;
		}

		if (GetPartCount() > 1) {
			if (hashresult != GetPartHash(partnumber)) {
				AddDebugLogLineM(false, logPartFile, CFormat( wxT("%s: Expected part-hash: %s")) % GetFileName() % GetPartHash(partnumber).Encode() );
				AddDebugLogLineM(false, logPartFile, CFormat( wxT("%s: Actual part-hash: %s")) % GetFileName() % hashresult.Encode() );
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
	return std::find(m_corrupted_list.begin(), m_corrupted_list.end(), partnumber) 
		!= m_corrupted_list.end();
}


void CPartFile::SetDownPriority(uint8 np, bool bSave, bool bRefresh )
{
	if ( m_iDownPriority != np ) {
		m_iDownPriority = np;
		if ( bRefresh )
			UpdateDisplayedInfo(true);
		if ( bSave )
			SavePartFile();
	}
}


void CPartFile::StopFile(bool bCancel)
{
	// Kry - Need to set it here to get into SetPartFileStatus(status) correctly
	m_stopped = true; 
	
	// Barry - Need to tell any connected clients to stop sending the file
	PauseFile();
	
	m_LastSearchTimeKad = 0;
	m_TotalSearchesKad = 0;
	
	RemoveAllSources(true);
	kBpsDown = 0.0;
	transferingsrc = 0;
	memset(m_anStates,0,sizeof(m_anStates));
	
	if (!bCancel) {
		FlushBuffer(true);
	}
	
	UpdateDisplayedInfo(true);
}


void CPartFile::StopPausedFile()
{
	if (!IsStopped()) {
		// Once an hour, remove any sources for files which are no longer active downloads
		switch (GetStatus()) {
			case PS_PAUSED:
			case PS_INSUFFICIENT:
			case PS_ERROR:
				if (time(NULL) - m_iLastPausePurge > (60*60)) {
					m_iLastPausePurge = time(NULL);
					StopFile();
				}
		}
	}
}


void CPartFile::PauseFile(bool bInsufficient)
{
	if ( status == PS_COMPLETE || status == PS_COMPLETING ) {
		return;
	}

	if (GetKadFileSearchID()) {
		Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), true);
		// If we were in the middle of searching, reset timer so they can resume searching.
		m_LastSearchTimeKad = 0; 
	}
	
	m_iLastPausePurge = time(NULL);
	
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	CPacket packet( OP_CANCELTRANSFER, 0, OP_EDONKEYPROT );
	for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* cur_src = *it++;
		if (cur_src->GetDownloadState() == DS_DOWNLOADING) {
			if (!cur_src->GetSentCancelTransfer()) {				
				theStats::AddUpOverheadOther( packet.GetPacketSize() );
				AddDebugLogLineM( false, logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + cur_src->GetFullIP() );
				cur_src->SendPacket( &packet, false, true );
				cur_src->SetSentCancelTransfer( true );
			}
			cur_src->SetDownloadState(DS_ONQUEUE);
		}
	}

	
	m_insufficient = bInsufficient;
	m_paused = true;
	
	
	kBpsDown = 0.0;
	transferingsrc = 0;
	m_anStates[DS_DOWNLOADING] = 0;

	SetStatus(status);
}


void CPartFile::ResumeFile()
{
	if ( status == PS_COMPLETE || status == PS_COMPLETING ) {
		return;
	}

	if ( m_insufficient && !CheckFreeDiskSpace() ) {
		// Still not enough free discspace
		return;
	}
	
	m_paused = false;
	m_stopped = false;
	m_insufficient = false;
		
	m_lastsearchtime = 0;
	SetStatus(status);

	if (m_gaplist.empty() and (GetStatus() == PS_ERROR)) {
		// The file has already been hashed at this point
		CompleteFile(true);
	}
	
	UpdateDisplayedInfo(true);
}


bool CPartFile::CheckFreeDiskSpace( uint32 neededSpace )
{
	wxLongLong free = 0;
	if ( !wxGetDiskSpace( GetFilePath(), NULL, &free ) ) {
		return true;
	}

	// The very least acceptable diskspace is a single PART
	if ( free < PARTSIZE ) {
		// Always fail in this case, since we risk losing data if we try to
		// write on a full partition.
		return false;
	}
	
	// All other checks are only made if the user has enabled them
	if ( thePrefs::IsCheckDiskspaceEnabled() ) {
		neededSpace += thePrefs::GetMinFreeDiskSpace();
		
		// Due to the the existance of sparse files, we cannot assume that
		// writes within the file doesn't cause new blocks to be allocated.
		// Therefore, we have to simply stop writing the moment the limit has
		// been exceeded.
		return free >= neededSpace;
	}
	
	return true;
}


void CPartFile::SetLastAnsweredTime()
{
	m_ClientSrcAnswered = ::GetTickCount();
}

void CPartFile::SetLastAnsweredTimeTimeout()
{ 
	m_ClientSrcAnswered = 2 * CONNECTION_LATENCY + ::GetTickCount() - SOURCECLIENTREASKS;
}

CPacket *CPartFile::CreateSrcInfoPacket(const CUpDownClient* forClient)
{
	
	// Kad reviewed
	
	if ((forClient->GetRequestFile() != this)
		&& (forClient->GetUploadFile() != this)) {
		wxString file1 = _("Unknown");
		if (forClient->GetRequestFile() && !forClient->GetRequestFile()->GetFileName().IsEmpty()) {
			file1 = forClient->GetRequestFile()->GetFileName();
		} else if (forClient->GetUploadFile() &&  !forClient->GetUploadFile()->GetFileName().IsEmpty()) {
			file1 = forClient->GetUploadFile()->GetFileName();
		}
		wxString file2 = _("Unknown");
		if (!GetFileName().IsEmpty()) {
			file2 = GetFileName();
		}
		AddDebugLogLineM(false, logPartFile, wxT("File missmatch on source packet (P) Sending: ") + file1 + wxT("  From: ") + file2);
		return NULL;
	}

	if(!IsPartFile())  {
		return CKnownFile::CreateSrcInfoPacket(forClient);
	}

	if ( !(GetStatus() == PS_READY || GetStatus() == PS_EMPTY)) {
		return NULL;
	}

	if ( m_SrcList.empty() ) {
		return NULL;
	}

	const BitVector& reqstatus = forClient->GetPartStatus();
	bool KnowNeededParts = !reqstatus.empty();
	//wxASSERT(rcvstatus.size() == GetPartCount()); // Obviously!
	if (reqstatus.size() != GetPartCount()) {
		// Yuck. Same file but different part count? Seriously fucked up.
		AddDebugLogLineM(false, logPartFile, wxString::Format(wxT("Impossible situation: different partcounts for the same part file: %i (client) and %i (file)"),reqstatus.size(),GetPartCount()));
		return NULL;
	}	
	
	CMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash(m_abyFileHash);
	data.WriteUInt16(nCount);
	bool bNeeded;
	for (SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it ) {
		bNeeded = false;
		CUpDownClient* cur_src = *it;
		
		int state = cur_src->GetDownloadState();
		int valid = ( state == DS_DOWNLOADING ) || ( state == DS_ONQUEUE && !cur_src->IsRemoteQueueFull() );
		
		if ( cur_src->HasLowID() || !valid ) {
			continue;
		}
		
		// only send source which have needed parts for this client if possible
		const BitVector& srcstatus = cur_src->GetPartStatus();
		if ( !srcstatus.empty() ) {
			//wxASSERT(srcstatus.size() == GetPartCount()); // Obviously!
			if (srcstatus.size() != GetPartCount()) {
				continue;
			}
			if ( KnowNeededParts ) {
				// only send sources which have needed parts for this client
				for (int x = 0; x < GetPartCount(); ++x) {
					if (srcstatus[x] && !reqstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			} else {
				// if we don't know the need parts for this client, 
				// return any source currently a client sends it's 
				// file status only after it has at least one complete part
				if (srcstatus.size() != GetPartCount()) {
					continue;
				}
				for (int x = 0; x < GetPartCount(); ++x){
					if (srcstatus[x]) {
						bNeeded = true;
						break;
					}
				}
			}
		}
		if(bNeeded) {
			++nCount;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2) {
				dwID = cur_src->GetUserIDHybrid();
			} else {
				dwID = wxUINT32_SWAP_ALWAYS(cur_src->GetUserIDHybrid());
			}
			data.WriteUInt32(dwID);
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion()>1) {
				data.WriteHash(cur_src->GetUserHash());
			}
			if (nCount > 500) {
				break;
			}
		}
	}
	if (!nCount) {
		return 0;
	}
	data.Seek(16, wxFromStart);
	data.WriteUInt16(nCount);

	CPacket* result = new CPacket(&data, OP_EMULEPROT, OP_ANSWERSOURCES);

	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if (result->GetPacketSize() > 354) {
		result->PackPacket();
	}
	
	return result;
}

void CPartFile::AddClientSources(CMemFile* sources, uint8 sourceexchangeversion, unsigned nSourceFrom)
{
	// Kad reviewed
	
	if (m_stopped) {
		return;
	}

	uint16 nCount = sources->ReadUInt16();
	
	// Check if the data size matches the 'nCount' for v1 or v2 and eventually correct the source
	// exchange version while reading the packet data. Otherwise we could experience a higher
	// chance in dealing with wrong source data, userhashs and finally duplicate sources.
	uint32 uDataSize = sources->GetLength() - sources->GetPosition();
	
	if ((uint32)(nCount*(4+2+4+2)) == uDataSize) { //Checks if version 1 packet is correct size
		if( sourceexchangeversion != 1 ) {
			return;
		}
	} else if ((uint32)(nCount*(4+2+4+2+16)) == uDataSize) { // Checks if version 2&3 packet is correct size
		if( sourceexchangeversion == 1 ) {
			return;
		}
	} else {
		// If v4 inserts additional data (like v2), the above code will correctly filter those packets.
		// If v4 appends additional data after <count>(<Sources>)[count], we are in trouble with the 
		// above code. Though a client which does not understand v4+ should never receive such a packet.
		AddDebugLogLineM(false, logClient, CFormat(wxT("Received invalid source exchange packet (v%u) of data size %u for %s")) % sourceexchangeversion % uDataSize % GetFileName());
		return;
	}
	
	for (int i = 0;i != nCount;++i) {
		
		uint32 dwID = sources->ReadUInt32();
		uint16 nPort = sources->ReadUInt16();
		uint32 dwServerIP = sources->ReadUInt32();
		uint16 nServerPort = sources->ReadUInt16();
	
		CMD4Hash userHash;
		if (sourceexchangeversion > 1) {
			userHash = sources->ReadHash();
		}
		
		//Clients send ID's the the Hyrbid format so highID clients with *.*.*.0 won't be falsely switched to a lowID..
		if (sourceexchangeversion == 3) {
			uint32 dwIDED2K = wxUINT32_SWAP_ALWAYS(dwID);

			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID)) {
				if (!IsGoodIP(dwIDED2K, thePrefs::FilterLanIPs())) {
					// check for 0-IP, localhost and optionally for LAN addresses
					AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange - bad IP")) % Uint32toStringIP(dwIDED2K));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwIDED2K)) {
					AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange - IPFilter")) % Uint32toStringIP(dwIDED2K));
					continue;
				}
				if (theApp.clientlist->IsBannedClient(dwIDED2K)){
					continue;
				}
			}

			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort, NULL, false)) {
				AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange")) % Uint32toStringIP(dwIDED2K));
				continue;
			}
		} else {
			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID)) {
				if (!IsGoodIP(dwID, thePrefs::FilterLanIPs())) { 
					// check for 0-IP, localhost and optionally for LAN addresses
					AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange - bad IP")) % Uint32toStringIP(dwID));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwID)) {
					AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange - IPfilter")) % Uint32toStringIP(dwID));
					continue;
				}
				if (theApp.clientlist->IsBannedClient(dwID)){
					continue;
				}
			}

			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort)) {
				AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange")) % Uint32toStringIP(dwID));
				continue;
			}
		}
		
		if(thePrefs::GetMaxSourcePerFile() > GetSourceCount()) {
			CUpDownClient* newsource = new CUpDownClient(nPort,dwID,dwServerIP,nServerPort,this, (sourceexchangeversion != 3), true);
			if (sourceexchangeversion > 1) {
				newsource->SetUserHash(userHash);
			}
			newsource->SetSourceFrom((ESourceFrom)nSourceFrom);
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

// Kry - transize is 32bits, no packet can be more than that (this is
// compressed size). Even 32bits is too much imho.As for the return size,
// look at the lenData below.
uint32 CPartFile::WriteToBuffer(uint32 transize, byte* data, uint64 start, uint64 end, Requested_Block_Struct *block)
{
	// Increment transfered bytes counter for this file
	transfered += transize;

	// This is needed a few times
	// Kry - should not need a uint64 here - no block is larger than
	// 2GB even after uncompressed.
	uint32 lenData = (uint32) (end - start + 1);

	if(lenData > transize) {
		m_iGainDueToCompression += lenData-transize;
	}

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end)) {
		AddDebugLogLineM(false, logPartFile, wxT("File '") + GetFileName() +
			wxString::Format(wxT("' has already been written from %lu to %lu\n"),
				(long)start, (long)end));
		return 0;
	}

	// Create copy of data as new buffer
	byte *buffer = new byte[lenData];
	memcpy(buffer, data, lenData);

	// Create a new buffered queue entry
	PartFileBufferedData *item = new PartFileBufferedData;
	item->data = buffer;
	item->start = start;
	item->end = end;
	item->block = block;

	// Add to the queue in the correct position (most likely the end)
	bool added = false;
	
	std::list<PartFileBufferedData*>::iterator it = m_BufferedData_list.begin();
	for (; it != m_BufferedData_list.end(); ++it) {
		PartFileBufferedData* queueItem = *it;

		if (item->end <= queueItem->end) {
			if (it != m_BufferedData_list.begin()) {
				added = true;

				m_BufferedData_list.insert(--it, item);
			}
			
			break;
		}
	}
	
	if (!added) {
		m_BufferedData_list.push_front(item);
	}

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(item->start, item->end);

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	
	std::list<Requested_Block_Struct*>::iterator it2 = m_requestedblocks_list.begin();
	for (; it2 != m_requestedblocks_list.end(); ++it2) {
		if (*it2 == item->block) {
			item->block->transferred += lenData;
		}
	}

	if (m_gaplist.empty()) {
		FlushBuffer(true);
	}

	// Return the length of data written to the buffer
	return lenData;
}

#warning Kry - serious review. I did some, and seems to be ok, but...
void CPartFile::FlushBuffer(bool /*forcewait*/, bool bForceICH, bool bNoAICH)
{
	m_nLastBufferFlushTime = GetTickCount();
	
	if (m_BufferedData_list.empty()) {
		return;
	}

	
	uint32 partCount = GetPartCount();
	bool changedPart[partCount];
	
	// Remember which parts need to be checked at the end of the flush
	for ( uint32 i = 0; i < partCount; ++i ) {
		changedPart[ i ] = false;
	}

	
	// Ensure file is big enough to write data to (the last item will be the furthest from the start)
	uint32 newData = 0;

	std::list<PartFileBufferedData*>::iterator it = m_BufferedData_list.begin();
	for (; it != m_BufferedData_list.end(); ++it) {
		PartFileBufferedData* item = *it;
		wxASSERT((item->end - item->start) < 0xFFFFFFFF);
		newData += (uint32) (item->end - item->start + 1);
	}
	
	if ( !CheckFreeDiskSpace( newData ) ) {
		// Not enough free space to write the last item, bail
		AddLogLineM(true, CFormat( _("WARNING: Not enough free disk-space! Pausing file: %s") ) % GetFileName());
	
		PauseFile( true );
		return;
	}
	
	// Loop through queue
	while ( !m_BufferedData_list.empty() ) {
		// Get top item and remove it from the queue
		PartFileBufferedData* item = m_BufferedData_list.front();
		m_BufferedData_list.pop_front();

		// This is needed a few times
		wxASSERT((item->end - item->start) < 0xFFFFFFFF);
		uint32 lenData = (uint32)(item->end - item->start + 1);

		// SLUGFILLER: SafeHash - could be more than one part
		for (uint32 curpart = (item->start/PARTSIZE); curpart <= (item->end/PARTSIZE); ++curpart) {
			wxASSERT(curpart < partCount);
			changedPart[curpart] = true;
		}
		// SLUGFILLER: SafeHash
		
		// Go to the correct position in file and write block of data			
		try {
			m_hpartfile.Seek(item->start);
			m_hpartfile.Write(item->data, lenData);
		} catch (const CIOFailureException& e) {
			AddDebugLogLineM(true, logPartFile, wxT("Error while saving part-file: ") + e.what());
		}

		// Decrease buffer size
		m_nTotalBufferData -= lenData;

		// Release memory used by this item
		delete [] item->data;
		delete item;
	}
	
	
	// Update last-changed date
	m_lastDateChanged = wxDateTime().Now();

	
	// Partfile should never be too large
	if (m_hpartfile.GetLength() > GetFileSize()){
		// it's "last chance" correction. the real bugfix has to be applied 'somewhere' else
		m_hpartfile.SetLength(GetFileSize());
	}		
	
	// Check each part of the file
	uint32 partRange = (uint32)((m_hpartfile.GetLength() % PARTSIZE > 0) ? ((m_hpartfile.GetLength() % PARTSIZE) - 1) : (PARTSIZE - 1));
	wxASSERT(((int)partRange) > 0);
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
				AddLogLineM(true, CFormat(
					_("Downloaded part %i is corrupt in file: %s") ) % partNumber % GetFileName() );
				AddGap(PARTSIZE*partNumber, (PARTSIZE*partNumber + partRange));
				// add part to corrupted list, if not already there
				if (!IsCorruptedPart(partNumber)) {
					m_corrupted_list.push_back(partNumber);
				}
				// request AICH recovery data
				if (!bNoAICH) {
					RequestAICHRecovery((uint16)partNumber);					
				}
				// Reduce transfered amount by corrupt amount
				m_iLostDueToCorruption += (partRange + 1);
			} else {
				if (!m_hashsetneeded) {
					AddDebugLogLineM(false, logPartFile, wxString::Format(
						wxT("Finished part %u of "), partNumber) + GetFileName());
				}
				
				// if this part was successfully completed (although ICH is active), remove from corrupted list
				EraseFirstValue(m_corrupted_list, partNumber);
				
				if (status == PS_EMPTY) {
					if (theApp.IsRunning()) { // may be called during shutdown!
						if (GetHashCount() == GetED2KPartHashCount() && !m_hashsetneeded) {
							// Successfully completed part, make it available for sharing
							SetStatus(PS_READY);
							theApp.sharedfiles->SafeAddKFile(this);
						}
					}
				}
			}
		} else if ( IsCorruptedPart(partNumber) && (thePrefs::IsICHEnabled() || bForceICH)) {
			// Try to recover with minimal loss
			if (HashSinglePart(partNumber)) {
				++m_iTotalPacketsSavedDueToICH;
				
				uint64 uMissingInPart = GetTotalGapSizeInPart(partNumber);					
				FillGap(PARTSIZE*partNumber,(PARTSIZE*partNumber+partRange));
				RemoveBlockFromList(PARTSIZE*partNumber,(PARTSIZE*partNumber + partRange));

				// remove from corrupted list
				EraseFirstValue(m_corrupted_list, partNumber);
				
				AddLogLineM(true, CFormat( _("ICH: Recovered corrupted part %i for %s -> Saved bytes: %s") )
					% partNumber
					% GetFileName()
					% CastItoXBytes(uMissingInPart));
				
				if (GetHashCount() == GetED2KPartHashCount() && !m_hashsetneeded) {
					if (status == PS_EMPTY) {
						// Successfully recovered part, make it available for sharing							
						SetStatus(PS_READY);
						if (theApp.IsRunning()) // may be called during shutdown!
							theApp.sharedfiles->SafeAddKFile(this);
					}
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
		if (m_gaplist.empty()) {
			CompleteFile(false);
		}
	}
}


void CPartFile::UpdateFileRatingCommentAvail()
{
	bool prevComment = m_hasComment;
	int prevRating = m_iUserRating;

	m_hasComment = false;
	m_iUserRating = 0;
	int ratingCount = 0;

	SourceSet::iterator it = m_SrcList.begin();
	for (; it != m_SrcList.end(); ++it) {
		CUpDownClient* cur_src = *it;
		
		if (!cur_src->GetFileComment().IsEmpty()) {
			m_hasComment = true;
		}

		uint8 rating = cur_src->GetFileRating();
		if (rating) {
			wxASSERT(rating <= 5);
			
			ratingCount++;
			m_iUserRating += rating;
		}
	}
	
	if (ratingCount) {
		m_iUserRating /= ratingCount;
		wxASSERT(m_iUserRating > 0 && m_iUserRating <= 5);
	}
	
	if ((prevComment != m_hasComment) || (prevRating != m_iUserRating)) {
		UpdateDisplayedInfo();
	}
}


void CPartFile::UpdateDisplayedInfo(bool force)
{
	uint32 curTick = ::GetTickCount();

	 // Wait 1.5s between each redraw
	 if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE ) {
		 Notify_DownloadCtrlUpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
	
}


void CPartFile::SetCategory(uint8 cat)
{
	wxASSERT( cat < theApp.glob_prefs->GetCatCount() );
	
	m_category = cat; 
	SavePartFile(); 
}

bool CPartFile::RemoveSource(CUpDownClient* toremove, bool updatewindow, bool bDoStatsUpdate)
{
	wxASSERT( toremove );

	bool result = theApp.downloadqueue->RemoveSource( toremove, updatewindow, bDoStatsUpdate );

	// Check if the client should be deleted, but not if the client is already dying
	if ( !toremove->GetSocket() && !toremove->HasBeenDeleted() ) {
		if ( toremove->Disconnected(wxT("RemoveSource - purged")) ) {
			toremove->Safe_Delete();
		}
	}

	return result;
}

void CPartFile::CleanUpSources( bool noNeeded, bool fullQueue, bool highQueue )
{
	SourceSet::iterator it = m_SrcList.begin();
	for ( ; it != m_SrcList.end(); ) {
		CUpDownClient* client = *it++;

		bool remove = false;
	
		// Using val = val || <blah>, to avoid unnescesarry evaluations
		if ( noNeeded && ( client->GetDownloadState() == DS_NONEEDEDPARTS ) ) {
			if ( client->SwapToAnotherFile(true, true, true) ) {
				continue;
			} else {
				remove = true;
			}
		} 
		
		if ( client->GetDownloadState() == DS_ONQUEUE ) {
			remove = remove || ( fullQueue && ( client->IsRemoteQueueFull() ) );
			remove = remove || ( highQueue && ( client->GetRemoteQueueRank() > thePrefs::HighQueueRanking() ) );
		}

		if ( remove )
			RemoveSource( client );
	}
}


void CPartFile::AddDownloadingSource(CUpDownClient* client)
{
	CClientPtrList::iterator it = 
		std::find(m_downloadingSourcesList.begin(), m_downloadingSourcesList.end(), client);
	if (it == m_downloadingSourcesList.end()) {
		m_downloadingSourcesList.push_back(client);
	}
}


void CPartFile::RemoveDownloadingSource(CUpDownClient* client)
{
	CClientPtrList::iterator it = 
		std::find(m_downloadingSourcesList.begin(), m_downloadingSourcesList.end(), client);
	if (it != m_downloadingSourcesList.end()) {
		m_downloadingSourcesList.erase(it);
	}
}


void CPartFile::SetPartFileStatus(uint8 newstatus)
{
	status=newstatus;
	
	if (thePrefs::GetAllcatType()) {
		Notify_DownloadCtrlUpdateItem(this);
	}

	Notify_DownloadCtrlSort();
}


uint64 CPartFile::GetNeededSpace()
{
	if (m_hpartfile.GetLength() > GetFileSize()) {
		return 0;	// Shouldn't happen, but just in case
	}
	return GetFileSize()-m_hpartfile.GetLength();
}


void CPartFile::SetStatus(uint8 in)
{
	wxASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );
	
	status = in;
	
	if (theApp.IsRunning()) {
		UpdateDisplayedInfo( true );
	
		if ( thePrefs::ShowCatTabInfos() ) {
			Notify_ShowUpdateCatTabTitles();
		}
	}
}


uint64 CPartFile::GetTotalGapSizeInRange(uint64 uRangeStart, uint64 uRangeEnd) const
{
	uint64 uTotalGapSize = 0;

	if (uRangeEnd >= GetFileSize()) {
		uRangeEnd = GetFileSize() - 1;
	}

	std::list<Gap_Struct*>::const_iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); ++it) {
		const Gap_Struct* pGap = *it;

		if (pGap->start < uRangeStart && pGap->end > uRangeEnd) {
			uTotalGapSize += uRangeEnd - uRangeStart + 1;
			break;
		}

		if (pGap->start >= uRangeStart && pGap->start <= uRangeEnd) {
			uint64 uEnd = (pGap->end > uRangeEnd) ? uRangeEnd : pGap->end;
			uTotalGapSize += uEnd - pGap->start + 1;
		} else if (pGap->end >= uRangeStart && pGap->end <= uRangeEnd) {
			uTotalGapSize += pGap->end - uRangeStart + 1;
		}
	}

	wxASSERT( uTotalGapSize <= uRangeEnd - uRangeStart + 1 );

	return uTotalGapSize;
}

uint64 CPartFile::GetTotalGapSizeInPart(uint32 uPart) const
{
	uint64 uRangeStart = uPart * PARTSIZE;
	uint64 uRangeEnd = uRangeStart + PARTSIZE - 1;
	if (uRangeEnd >= GetFileSize()) {
		uRangeEnd = GetFileSize();
	}
	return GetTotalGapSizeInRange(uRangeStart, uRangeEnd);
}


void CPartFile::RequestAICHRecovery(uint16 nPart)
{

	if (	!m_pAICHHashSet->HasValidMasterHash() ||
		(m_pAICHHashSet->GetStatus() != AICH_TRUSTED && m_pAICHHashSet->GetStatus() != AICH_VERIFIED)){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Unable to request AICH Recoverydata because we have no trusted Masterhash") );
		return;
	}
	if (GetFileSize() <= EMBLOCKSIZE || GetFileSize() - PARTSIZE*nPart <= EMBLOCKSIZE)
		return;
	if (CAICHHashSet::IsClientRequestPending(this, nPart)){
		AddDebugLogLineM( false, logAICHRecovery, wxT("RequestAICHRecovery: Already a request for this part pending"));
		return;
	}

	// first check if we have already the recoverydata, no need to rerequest it then
	if (m_pAICHHashSet->IsPartDataAvailable(nPart*PARTSIZE)){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Found PartRecoveryData in memory"));
		AICHRecoveryDataAvailable(nPart);
		return;
	}

	wxASSERT( nPart < GetPartCount() );
	// find some random client which support AICH to ask for the blocks
	// first lets see how many we have at all, we prefer high id very much
	uint32 cAICHClients = 0;
	uint32 cAICHLowIDClients = 0;
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it) {	
		CUpDownClient* pCurClient = *(it);
		if (	pCurClient->IsSupportingAICH() &&
			pCurClient->GetReqFileAICHHash() != NULL &&
			!pCurClient->IsAICHReqPending()
			&& (*pCurClient->GetReqFileAICHHash()) == m_pAICHHashSet->GetMasterHash())
		{
			if (pCurClient->HasLowID()) {
				++cAICHLowIDClients;
			} else {
				++cAICHClients;
			}
		}
	}
	if ((cAICHClients | cAICHLowIDClients) == 0){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Unable to request AICH Recoverydata because found no client who supports it and has the same hash as the trusted one"));
		return;
	}
	uint32 nSeclectedClient;
	if (cAICHClients > 0) {
		nSeclectedClient = (rand() % cAICHClients) + 1;
	} else {
		nSeclectedClient = (rand() % cAICHLowIDClients) + 1;
	}
	CUpDownClient* pClient = NULL;
	for ( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ++it) {	
		CUpDownClient* pCurClient = *(it);
		if (pCurClient->IsSupportingAICH() && pCurClient->GetReqFileAICHHash() != NULL && !pCurClient->IsAICHReqPending()
			&& (*pCurClient->GetReqFileAICHHash()) == m_pAICHHashSet->GetMasterHash())
		{
			if (cAICHClients > 0){
				if (!pCurClient->HasLowID())
					nSeclectedClient--;
			}
			else{
				wxASSERT( pCurClient->HasLowID());
				nSeclectedClient--;
			}
			if (nSeclectedClient == 0){
				pClient = pCurClient;
				break;
			}
		}
	}
	if (pClient == NULL){
		wxASSERT( false );
		return;
	}

	AddDebugLogLineM( false, logAICHRecovery, CFormat( wxT("Requesting AICH Hash (%s) form client %s") ) % ( cAICHClients ? wxT("HighId") : wxT("LowID") ) % pClient->GetClientFullInfo() );
	pClient->SendAICHRequest(this, nPart);
	
}


void CPartFile::AICHRecoveryDataAvailable(uint16 nPart)
{
	if (GetPartCount() < nPart){
		wxASSERT( false );
		return;
	}
	FlushBuffer(true,true,true);
	uint64 length = PARTSIZE;
	if ((unsigned)(PARTSIZE * (nPart + 1)) > m_hpartfile.GetLength()){
		length = (m_hpartfile.GetLength() - (PARTSIZE * nPart));
		wxASSERT( length <= PARTSIZE );
	}	
	// if the part was already ok, it would now be complete
	if (IsComplete(nPart*PARTSIZE, ((nPart*PARTSIZE)+length)-1)){
		AddDebugLogLineM( false, logAICHRecovery,
			wxString::Format( wxT("Processing AICH Recovery data: The part (%u) is already complete, canceling"), nPart ) );
		return;
	}
	


	CAICHHashTree* pVerifiedHash = m_pAICHHashSet->m_pHashTree.FindHash(nPart*PARTSIZE, length);
	if (pVerifiedHash == NULL || !pVerifiedHash->GetHashValid()){
		AddDebugLogLineM( true, logAICHRecovery, wxT("Processing AICH Recovery data: Unable to get verified hash from hashset (should never happen)") );
		wxASSERT( false );
		return;
	}
	CAICHHashTree htOurHash(pVerifiedHash->GetNDataSize(), pVerifiedHash->GetIsLeftBranch(), pVerifiedHash->GetNBaseSize());
	try {
		m_hpartfile.Seek(PARTSIZE * nPart,wxFromStart);
		CreateHashFromFile(&m_hpartfile,length, NULL, &htOurHash);
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logAICHRecovery, wxT("IO failure while hashing part-file '") + m_hpartfile.GetFilePath()
			+ wxT("': ") );
		wxASSERT( false );
		return;
	}
	
	if (!htOurHash.GetHashValid()){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Processing AICH Recovery data: Failed to retrieve AICH Hashset of corrupt part") );
		wxASSERT( false );
		return;
	}

	// now compare the hash we just did, to the verified hash and readd all blocks which are ok
	uint32 nRecovered = 0;
	for (uint32 pos = 0; pos < length; pos += EMBLOCKSIZE){
		const uint32 nBlockSize = min<uint32>(EMBLOCKSIZE, length - pos);
		CAICHHashTree* pVerifiedBlock = pVerifiedHash->FindHash(pos, nBlockSize);
		CAICHHashTree* pOurBlock = htOurHash.FindHash(pos, nBlockSize);
		if ( pVerifiedBlock == NULL || pOurBlock == NULL || !pVerifiedBlock->GetHashValid() || !pOurBlock->GetHashValid()){
			wxASSERT( false );
			continue;
		}
		if (pOurBlock->GetHash() == pVerifiedBlock->GetHash()){
			FillGap(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
			RemoveBlockFromList(PARTSIZE*nPart, PARTSIZE*nPart + (nBlockSize-1));
			nRecovered += nBlockSize;
		}
	}

	// ok now some sanity checks
	if (IsComplete(nPart*PARTSIZE, ((nPart*PARTSIZE)+length)-1)){
		// this is a bad, but it could probably happen under some rare circumstances
		// make sure that MD4 agrres to this fact too
		if (!HashSinglePart(nPart)){
			AddDebugLogLineM( false, logAICHRecovery, 
				wxString::Format(wxT("Processing AICH Recovery data: The part (%u) got completed while recovering "
				"- but MD4 says it corrupt! Setting hashset to error state, deleting part"), nPart));
			// now we are fu... unhappy
			m_pAICHHashSet->SetStatus(AICH_ERROR);
			AddGap(PARTSIZE*nPart, ((nPart*PARTSIZE)+length)-1);
			wxASSERT( false );
			return;
		}
		else{
			AddDebugLogLineM( false, logAICHRecovery, wxString::Format( 
				wxT("Processing AICH Recovery data: The part (%u) "
				"got completed while recovering and MD4 agrees"), nPart) );
			// alrighty not so bad
			EraseFirstValue(m_corrupted_list, nPart);
			if (status == PS_EMPTY && theApp.IsRunning()){
				if (GetHashCount() == GetED2KPartHashCount() && !m_hashsetneeded){
					// Successfully recovered part, make it available for sharing
					SetStatus(PS_READY);
					theApp.sharedfiles->SafeAddKFile(this);
				}
			}

			if (theApp.IsRunning()){
				// Is this file finished?
				if (m_gaplist.empty()) {
					CompleteFile(false);
				}
			}
		}
	} // end sanity check
	// Update met file
	SavePartFile();
	
	// make sure the user appreciates our great recovering work :P
	AddDebugLogLineM( true, logAICHRecovery, CFormat( 
		wxT("AICH successfully recovered %s of %s from part %u for %s") )
		% CastItoXBytes(nRecovered) 
		% CastItoXBytes(length)
		% nPart
		% GetFileName() );
}


void CPartFile::ClientStateChanged( int oldState, int newState )
{
	if ( oldState == newState )
		return;

	// If the state is -1, then it's an entirely new item
	if ( oldState != -1 ) {
		// Was the old state a valid state?
		if ( oldState == DS_ONQUEUE || oldState == DS_DOWNLOADING ) {
			m_validSources--;
		} else {
			if ( oldState == DS_CONNECTED /* || oldState == DS_REMOTEQUEUEFULL  */ ) {
				m_validSources--;
			}

			m_notCurrentSources--;
		}
	}

	// If the state is -1, then the source is being removed
	if ( newState != -1 ) {
		// Was the old state a valid state?
		if ( newState == DS_ONQUEUE || newState == DS_DOWNLOADING ) {
			++m_validSources;
		} else {
			if ( newState == DS_CONNECTED /* || newState == DS_REMOTEQUEUEFULL  */ ) {
				++m_validSources;
			}

			++m_notCurrentSources;
		}
	}
}


bool CPartFile::AddSource( CUpDownClient* client )
{
	if (m_SrcList.insert( client ).second) {
		theStats::AddFoundSource();
		theStats::AddSourceOrigin(client->GetSourceFrom());
		return true;
	} else {
		return false;
	}
}

	
bool CPartFile::DelSource( CUpDownClient* client )
{
	if (m_SrcList.erase( client )) {
		theStats::RemoveSourceOrigin(client->GetSourceFrom());
		theStats::RemoveFoundSource();
		return true;
	} else {
		return false;
	}
}


void CPartFile::UpdatePartsFrequency( CUpDownClient* client, bool increment )
{
	const BitVector& freq = client->GetPartStatus();
	
	if ( m_SrcpartFrequency.size() != GetPartCount() ) {
		m_SrcpartFrequency.clear();
		m_SrcpartFrequency.insert(m_SrcpartFrequency.begin(), GetPartCount(), 0);

		if ( !increment ) {
			return;
		}
	}
	
	unsigned int size = freq.size();
	if ( size != m_SrcpartFrequency.size() ) {
		return;
	}
	
	if ( increment ) {
		for ( unsigned int i = 0; i < size; i++ ) {
			if ( freq[i] ) {
				m_SrcpartFrequency[i]++;
			}
		}
	} else {
		for ( unsigned int i = 0; i < size; i++ ) {
			if ( freq[i] ) {
				m_SrcpartFrequency[i]--;
			}
		}
	}
}

void CPartFile::GetRatingAndComments(FileRatingList& list)
{
	// This can be pre-processed, but is it worth the CPU?
	CPartFile::SourceSet::iterator it = m_SrcList.begin();
	for ( ; it != m_SrcList.end(); ++it ) {
		CUpDownClient *cur_src = *it;

		if (cur_src->GetFileComment().Length()>0 || cur_src->GetFileRating()>0) {
			SFileRating* entry =  new SFileRating;
			entry->UserName = cur_src->GetUserName();
			entry->FileName = cur_src->GetClientFilename();
			entry->Rating   = cur_src->GetFileRating();
			entry->Comment  = cur_src->GetFileComment();
			
			list.push_back(entry);			
		}
	}
}

#else   // CLIENT_GUI

CPartFile::CPartFile(CEC_PartFile_Tag *tag)
{
	Init();
	
	SetFileName(tag->FileName());
	m_abyFileHash = tag->ID();
	SetFileSize(tag->SizeFull());
	m_partmetfilename = tag->PartMetName();
	transfered = tag->SizeXfer();
	percentcompleted = (100.0*completedsize) / GetFileSize();
	completedsize = tag->SizeDone();

	m_category = tag->FileCat();

	m_iPartCount = ((uint64)GetFileSize() + (PARTSIZE - 1)) / PARTSIZE;
	m_SrcpartFrequency.insert(m_SrcpartFrequency.end(), m_iPartCount, 0);
	m_iDownPriority = tag->Prio();
	if ( m_iDownPriority >= 10 ) {
		m_iDownPriority-= 10;
		m_bAutoDownPriority = true;
	} else {
		m_bAutoDownPriority = false;
	}
	// FIXME: !
	m_category = 0;
	
	m_source_count = 0;
	m_a4af_source_count = 0;
}

/*
 * Remote gui specific code
 */
CPartFile::~CPartFile()
{
}

void CPartFile::GetRatingAndComments(FileRatingList& list)
{
	SFileRating* entry =  new SFileRating;
	entry->UserName = _("Comments");
	entry->FileName = _("doesn't work");
	entry->Rating   = -1;
	entry->Comment  = _("remote gui");
	
	list.push_back(entry);			
	
}
#endif // !CLIENT_GUI

void CPartFile::Init()
{
	m_showSources = false;

	m_nLastBufferFlushTime = 0;

	newdate = true;
	m_lastsearchtime = 0;
	lastpurgetime = ::GetTickCount();
	m_paused = false;
	m_stopped = false;
	m_insufficient = false;

	status = PS_EMPTY;
	
	transfered = 0;
	m_iLastPausePurge = time(NULL);
	
	if(thePrefs::GetNewAutoDown()) {
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	} else {
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	
	memset(m_anStates,0,sizeof(m_anStates));
	
	transferingsrc = 0; // new
	
	kBpsDown = 0.0;
	
	m_hashsetneeded = true;
	m_count = 0;
	percentcompleted = 0;
	completedsize=0;
	m_bPreviewing = false;
	lastseencomplete = 0;
	m_availablePartsCount=0;
	m_ClientSrcAnswered = 0;
	m_LastNoNeededCheck = 0;
	m_iRating = 0;
	m_nTotalBufferData = 0;
	m_nLastBufferFlushTime = 0;
	m_bPercentUpdated = false;
	m_bRecoveringArchive = false;
	m_iGainDueToCompression = 0;
	m_iLostDueToCorruption = 0;
	m_iTotalPacketsSavedDueToICH = 0;
	m_category = 0;
	m_lastRefreshedDLDisplay = 0;
	m_is_A4AF_auto = false;
	m_localSrcReqQueued = false;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	
	// Sources dropping
	m_LastSourceDropTime = 0;

	m_validSources = 0;
	m_notCurrentSources = 0;

	// Kad
	m_LastSearchTimeKad = 0;
	m_TotalSearchesKad = 0;	
	
}

wxString CPartFile::getPartfileStatus() const
{

	wxString mybuffer; 

	if ((status == PS_HASHING) || (status == PS_WAITINGFORHASH)) {
		mybuffer=_("Hashing");		
	} else {	
		switch (GetStatus()) {
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
			case PS_INSUFFICIENT:
				mybuffer = _("Insufficient Diskspace");
				break;
			default:
				if (GetTransferingSrcCount()>0) {
					mybuffer=_("Downloading");
				}	else {
					mybuffer=_("Waiting");
				}
				break;				
		} 
		if (m_stopped && (GetStatus()!=PS_COMPLETE)) {
			mybuffer=_("Stopped");
		}		
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

bool CPartFile::PreviewAvailable()
{
#ifndef CLIENT_GUI
	FileType type = GetFiletype(GetFileName());

	return (((type == ftVideo) or (type == ftAudio)) and IsComplete(0, 256*1024));
#else
	return false;
#endif
}

bool CPartFile::CheckShowItemInGivenCat(int inCategory)
{
	// easy normal cases
	bool IsInCat;
	bool IsNotFiltered = true;

	IsInCat = ((inCategory==0) || (inCategory>0 && inCategory==GetCategory()));

	switch (thePrefs::GetAllcatType()) {
		case 1:
			IsNotFiltered = GetCategory() == 0 || inCategory > 0;
			break;
		case 2:
			IsNotFiltered = IsPartFile();
			break;
		case 3:
			IsNotFiltered = !IsPartFile();
			break;
		case 4:
			IsNotFiltered = 
				(GetStatus() == PS_READY || GetStatus() == PS_EMPTY) &&
				GetTransferingSrcCount() == 0;
			break;
		case 5:
			IsNotFiltered =
				(GetStatus() == PS_READY || GetStatus()==PS_EMPTY) &&
				GetTransferingSrcCount() > 0;
			break;
		case 6:
			IsNotFiltered = GetStatus() == PS_ERROR;
			break;
		case 7:
			IsNotFiltered = GetStatus() == PS_PAUSED && !IsStopped();
			break;
		case 8:
			IsNotFiltered = IsStopped();
			break;
		case 9:
			IsNotFiltered = GetFiletype(GetFileName()) == ftVideo;
			break;
		case 10:
			IsNotFiltered = GetFiletype(GetFileName()) == ftAudio;
			break;
		case 11:
			IsNotFiltered = GetFiletype(GetFileName()) == ftArchive;
			break;
		case 12:
			IsNotFiltered = GetFiletype(GetFileName()) == ftCDImage;
			break;
		case 13:
			IsNotFiltered = GetFiletype(GetFileName()) == ftPicture;
			break;
		case 14:
			IsNotFiltered = GetFiletype(GetFileName()) == ftText;
			break;
		case 15:
			IsNotFiltered = !IsStopped() && GetStatus() != PS_PAUSED;
			break;
	}
	
	return IsNotFiltered && IsInCat;
}

bool CPartFile::IsComplete(uint64 start, uint64 end)
{
	if (end >= GetFileSize()) {
		end = GetFileSize()-1;
	}

	std::list<Gap_Struct*>::iterator it = m_gaplist.begin();
	for (; it != m_gaplist.end(); ++it) {
		Gap_Struct* cur_gap = *it;
		if ((cur_gap->start >= start && cur_gap->end <= end)||(cur_gap->start >= start 
		&& cur_gap->start <= end)||(cur_gap->end <= end && cur_gap->end >= start)
		||(start >= cur_gap->start && end <= cur_gap->end)) {
			return false;	
		}
	}
	return true;
}

#ifndef CLIENT_GUI

uint8 CPartFile::GetStatus(bool ignorepause) const
{
	if (	(!m_paused && !m_insufficient) ||
		status == PS_ERROR ||
		status == PS_COMPLETING ||
		status == PS_COMPLETE ||
		ignorepause) {
		return status;
	} else if ( m_insufficient ) {
		return PS_INSUFFICIENT;
	} else {
		return PS_PAUSED;
	}
}

void CPartFile::AddDeadSource(const CUpDownClient* client)
{
	m_deadSources.AddDeadSource( client );
}


bool CPartFile::IsDeadSource(const CUpDownClient* client)
{
	return m_deadSources.IsDeadSource( client );
}

void CPartFile::SetFileName(const wxString& pszFileName)
{
	CKnownFile* pFile = theApp.sharedfiles->GetFileByID(GetFileHash());
	
	bool is_shared = (pFile && pFile == this);
	
	if (is_shared) {
		// The file is shared, we must clear the search keywords so we don't
		// publish the old name anymore.
		theApp.sharedfiles->RemoveKeywords(this);
	}
	
	CKnownFile::SetFileName(pszFileName);
	
	if (is_shared) {
		// And of course, we must advertise the new name if the file is shared.
		theApp.sharedfiles->AddKeywords(this);
	}

	UpdateDisplayedInfo(true);
}


uint16 CPartFile::GetMaxSources() const
{
	// This is just like this, while we don't import the private max sources per file
	return thePrefs::GetMaxSourcePerFile();
}

uint16 CPartFile::GetMaxSourcePerFileSoft() const
{
	unsigned int temp = ((unsigned int)GetMaxSources() * 9L) / 10;
	if (temp > MAX_SOURCES_FILE_SOFT) {
		return MAX_SOURCES_FILE_SOFT;
	}
	return temp;
}

uint16 CPartFile::GetMaxSourcePerFileUDP() const
{	
	unsigned int temp = ((unsigned int)GetMaxSources() * 3L) / 4;
	if (temp > MAX_SOURCES_FILE_UDP) {
		return MAX_SOURCES_FILE_UDP;
	}
	return temp;
}

#endif
