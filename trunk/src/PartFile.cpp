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

#include <wx/wx.h>

#include "PartFile.h"		// Interface declarations.

#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for VERSION
#endif

#include <protocol/kad/Constants.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/Protocols.h>
#include <common/DataFileVersion.h>
#include <common/Constants.h>
#include <tags/FileTags.h>

#include <wx/utils.h>
#include <wx/tokenzr.h>		// Needed for wxStringTokenizer

#include "KnownFileList.h"	// Needed for CKnownFileList
#include "CanceledFileList.h"
#include "UploadQueue.h"	// Needed for CFileHash
#include "IPFilter.h"		// Needed for CIPFilter
#include "Server.h"		// Needed for CServer
#include "ServerConnect.h"	// Needed for CServerConnect
#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "Preferences.h"	// Needed for CPreferences
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"		// Needed for theApp
#include "ED2KLink.h"		// Needed for CED2KLink
#include "Packet.h"		// Needed for CTag
#include "SearchList.h"		// Needed for CSearchFile
#include "ClientList.h"		// Needed for clientlist
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include <common/Format.h>	// Needed for CFormat
#include <common/FileFunctions.h>	// Needed for GetLastModificationTime
#include "ThreadTasks.h"	// Needed for CHashingTask/CCompletionTask/CAllocateFileTask
#include "GuiEvents.h"		// Needed for Notify_*
#include "DataToText.h"		// Needed for OriginToText()
#include "PlatformSpecific.h"	// Needed for CreateSparseFile()
#include "FileArea.h"		// Needed for CFileArea
#include "ScopedPtr.h"		// Needed for CScopedArray
#include "CorruptionBlackBox.h"

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"


SFileRating::SFileRating(const wxString &u, const wxString &f, sint16 r, const wxString &c)
:
UserName(u),
FileName(f),
Rating(r),
Comment(c)
{
}


SFileRating::SFileRating(const SFileRating &fr)
:
UserName(fr.UserName),
FileName(fr.FileName),
Rating(fr.Rating),
Comment(fr.Comment)
{
}


SFileRating::SFileRating(const CUpDownClient &client)
:
UserName(client.GetUserName()),
FileName(client.GetClientFilename()),
Rating(client.GetFileRating()),
Comment(client.GetFileComment())
{
}


SFileRating::~SFileRating()
{
}


class PartFileBufferedData
{
public:
	CFileArea area;				// File area to be written
	uint64 start;					// This is the start offset of the data
	uint64 end;						// This is the end offset of the data
	Requested_Block_Struct *block;	// This is the requested block that this data relates to

	PartFileBufferedData(CFileAutoClose& file, byte * data, uint64 _start, uint64 _end, Requested_Block_Struct *_block)
		: start(_start), end(_end), block(_block)
	{
		area.StartWriteAt(file, start, end-start+1);
		memcpy(area.GetBuffer(), data, end-start+1);
	}
};


typedef std::list<Chunk> ChunkList;


#ifndef CLIENT_GUI

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
	
	SetFileName(CPath(fileLink->GetName()));
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
	// if it's not opened, it was completed or deleted
	if (m_hpartfile.IsOpened()) { 
		FlushBuffer();
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();			
	}

	DeleteContents(m_BufferedData_list);
	delete m_CorruptionBlackBox;

	wxASSERT(m_SrcList.empty());
	wxASSERT(m_A4AFsrclist.empty());
}

void CPartFile::CreatePartFile()
{
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	do { 
		++i; 
		m_partmetfilename = CPath(wxString::Format(wxT("%03i.part.met"), i));
		m_fullname = thePrefs::GetTempDir().JoinPaths(m_partmetfilename);
	} while (m_fullname.FileExists());

	m_CorruptionBlackBox->SetPartFileInfo(GetFileName().GetPrintable(), m_partmetfilename.RemoveAllExt().GetPrintable());
	
	wxString strPartName = m_partmetfilename.RemoveExt().GetRaw();
	m_taglist.push_back(CTagString(FT_PARTFILENAME, strPartName ));
	
	m_gaplist.Init(GetFileSize(), true);	// Init empty
	
	m_PartPath = m_fullname.RemoveExt();
	bool fileCreated;
	if (thePrefs::GetAllocFullFile()) {
		fileCreated = m_hpartfile.Create(m_PartPath, true);
		m_hpartfile.Close();
	} else {
		fileCreated = PlatformSpecific::CreateSparseFile(m_PartPath, GetFileSize());
	}
	if (!fileCreated) {
		AddLogLineM(false,_("ERROR: Failed to create partfile)"));
		SetPartFileStatus(PS_ERROR);
	}

	SetFilePath(thePrefs::GetTempDir());
			
	if (thePrefs::GetAllocFullFile()) {
		SetPartFileStatus(PS_ALLOCATING);
		CThreadScheduler::AddTask(new CAllocateFileTask(this, thePrefs::AddNewFilesPaused()));
	} else {
		AllocationFinished();
	}
	
	m_hashsetneeded = (GetED2KPartHashCount() > 0);
	
	SavePartFile(true);
	SetActive(theApp->IsConnected());
}


uint8 CPartFile::LoadPartFile(const CPath& in_directory, const CPath& filename, bool from_backup, bool getsizeonly)
{
	bool isnewstyle = false;
	uint8 version,partmettype=PMT_UNKNOWN;
	
	std::map<uint16, Gap_Struct*> gap_map; // Slugfiller
	transferred = 0;
	
	m_partmetfilename = filename;
	m_CorruptionBlackBox->SetPartFileInfo(GetFileName().GetPrintable(), m_partmetfilename.RemoveAllExt().GetPrintable());
	m_filePath = in_directory;
	m_fullname = m_filePath.JoinPaths(m_partmetfilename);
	m_PartPath = m_fullname.RemoveExt();
	
	// readfile data form part.met file
	CPath curMetFilename = m_fullname;
	if (from_backup) {
		curMetFilename = curMetFilename.AppendExt(PARTMET_BAK_EXT);
		AddLogLineM(false, CFormat( _("Trying to load backup of met-file from %s") )
			% curMetFilename );
	}
	
	try {
		CFile metFile(curMetFilename, CFile::read);
		if (!metFile.IsOpened()) {
			AddLogLineM(false, CFormat( _("ERROR: Failed to open part.met file: %s ==> %s") )
				% curMetFilename
				% GetFileName() );

			return false;
		} else if (metFile.GetLength() == 0) {
			AddLogLineM(false, CFormat( _("ERROR: part.met file is 0 size: %s ==> %s") )
				% m_partmetfilename
				% GetFileName() );
			
			return false;
		}

		version = metFile.ReadUInt8();
		if (version != PARTFILE_VERSION && version != PARTFILE_SPLITTEDVERSION && version != PARTFILE_VERSION_LARGEFILE){
			metFile.Close();
			//if (version == 83) return ImportShareazaTempFile(...)
			AddLogLineM(false, CFormat( _("ERROR: Invalid part.met file version: %s ==> %s") )
				% m_partmetfilename 
				% GetFileName() );
			return false;
		}

		isnewstyle = (version == PARTFILE_SPLITTEDVERSION);
		partmettype = isnewstyle ? PMT_SPLITTED : PMT_DEFAULTOLD;
		
		if (version == PARTFILE_VERSION) {// Do we still need this check ?
			uint8 test[4];									// It will fail for certain files.
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
						if (!GetFileName().IsOk()) {
							// If it's not empty, we already loaded the unicoded one
							SetFileName(CPath(newtag.GetStr()));
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
					case FT_TRANSFERRED: {
						transferred = newtag.GetInt();
						break;
					}
					case FT_FILETYPE:{
						//#warning needs setfiletype string
						//SetFileType(newtag.GetStr());
						break;
					}					
					case FT_CATEGORY: {
						m_category = newtag.GetInt();
						if (m_category > theApp->glob_prefs->GetCatCount() - 1 ) {
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
						m_paused = (newtag.GetInt() == 1);
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
					case FT_DL_ACTIVE_TIME:
						if (newtag.IsInt()) {
							m_nDlActiveTime = newtag.GetInt();
						}
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
					case FT_ATTRANSFERRED:{
						statistic.SetAllTimeTransferred(statistic.GetAllTimeTransferred() + (uint64)newtag.GetInt());
						break;
					}
					case FT_ATTRANSFERREDHI:{
						statistic.SetAllTimeTransferred(statistic.GetAllTimeTransferred() + (((uint64)newtag.GetInt()) << 32));	
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
								AddDebugLogLineN(logPartFile, wxT("Wrong gap map key while reading met file!"));
								wxFAIL;
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
			if (!m_hashlist.empty()) {
				CreateHashFromHashlist(m_hashlist, &checkhash);
			}
			bool flag=false;
			if (m_abyFileHash == checkhash) {
				flag=true;
			} else {
				m_hashlist.clear();
				flag=false;
			}
		}			
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
	} catch (const CEOFException& WXUNUSED(e)) {
		AddLogLineM(true, CFormat( _("ERROR: %s (%s) is corrupt (wrong tagcount), unable to load file.") )
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
			if (!GetFileName().IsOk()) {
				// Not critical, let's put a random filename.
				AddLogLineM(true, _(
					"Recovering no-named file - will try to recover it as RecoveredFile.dat"));
				SetFileName(CPath(wxT("RecoveredFile.dat")));
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
	// Init Gaplist
	m_gaplist.Init(GetFileSize(), false);	// Init full, then add gaps
	// Now to flush the map into the list (Slugfiller)
	std::map<uint16, Gap_Struct*>::iterator it = gap_map.begin();
	for ( ; it != gap_map.end(); ++it ) {
		Gap_Struct* gap = it->second;
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (	(gap->start != (uint64)-1) &&
			(gap->end != (uint64)-1) &&
			gap->start <= gap->end &&
			gap->start < GetFileSize()) {
			if (gap->end >= GetFileSize()) {
				gap->end = GetFileSize()-1; // Clipping
			}
			m_gaplist.AddGap(gap->start, gap->end); // All tags accounted for, use safe adding
		}
		delete gap;
		// SLUGFILLER: SafeHash
	}

	//check if this is a backup
	if ( m_fullname.GetExt().MakeLower() == wxT("backup" )) {
		m_fullname = m_fullname.RemoveExt();
	}

	// open permanent handle
	if ( !m_hpartfile.Open(m_PartPath, CFile::read_write)) {
		AddLogLineM(false, CFormat( _("Failed to open %s (%s)") )
			% m_fullname
			% GetFileName() );
		return false;
	}
	
	SetPartFileStatus(PS_EMPTY);

	try {
		// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
		if (m_hpartfile.GetLength() < GetFileSize())
			AddGap(m_hpartfile.GetLength(), GetFileSize()-1);
		// Goes both ways - Partfile should never be too large
		if (m_hpartfile.GetLength() > GetFileSize()) {
			AddDebugLogLineM( true, logPartFile, CFormat( wxT("Partfile \"%s\" is too large! Truncating %llu bytes.") ) % GetFileName() % (m_hpartfile.GetLength() - GetFileSize()));
			m_hpartfile.SetLength(GetFileSize());
		}
		// SLUGFILLER: SafeHash
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM( true, logPartFile, CFormat( wxT("Error while accessing partfile \"%s\": %s") ) % GetFileName() % e.what());
		SetPartFileStatus(PS_ERROR);
	}

	// now close the file again until needed
	m_hpartfile.Release(true);
	
	// check hashcount, file status etc
	if (GetHashCount() != GetED2KPartHashCount()){	
		m_hashsetneeded = true;
		return true;
	} else {
		m_hashsetneeded = false;
		for (size_t i = 0; i < m_hashlist.size(); ++i) {
			if (IsComplete(i)) {
				SetPartFileStatus(PS_READY);
			}
		}
	}
	
	if (m_gaplist.IsComplete()) { // is this file complete already?
		CompleteFile(false);
		return true;
	}

	if (!isnewstyle) { // not for importing	
		const time_t file_date = CPath::GetModificationTime(m_PartPath);
		if (m_lastDateChanged != file_date) {
			// It's pointless to rehash an empty file, since the case
			// where a user has zero'd a file is handled above ...
			if (m_hpartfile.GetLength()) {
				AddLogLineM(false, CFormat( _("WARNING: %s might be corrupted (%i)") )
					% m_PartPath
					% (m_lastDateChanged - file_date) );
				// rehash
				SetPartFileStatus(PS_WAITINGFORHASH);
			
				CPath partFileName = m_partmetfilename.RemoveExt();
				CThreadScheduler::AddTask(new CHashingTask(m_filePath, partFileName, this));
			}
		}
	}

	UpdateCompletedInfos();
	if (completedsize > transferred) {
		m_iGainDueToCompression = completedsize - transferred;
	} else if (completedsize != transferred) {
		m_iLostDueToCorruption = transferred - completedsize;
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
	
	/* Don't write anything to disk if less than 100 KB of free space is left. */
	sint64 free = CPath::GetFreeSpaceAt(GetFilePath());
	if ((free != wxInvalidOffset) && (free < (100 * 1024))) {
		return false;
	}
	
	CFile file;
	try {
		if (!m_PartPath.FileExists()) {
			throw wxString(wxT(".part file not found"));
		}
		
		uint32 lsc = lastseencomplete;

		if (!Initial) {
			CPath::BackupFile(m_fullname, wxT(".backup"));
			CPath::RemoveFile(m_fullname);
		}
		
		file.Open(m_fullname, CFile::write);
		if (!file.IsOpened()) {
			throw wxString(wxT("Failed to open part.met file"));
		}

		// version
		file.WriteUInt8(IsLargeFile() ? PARTFILE_VERSION_LARGEFILE : PARTFILE_VERSION);
		
		file.WriteUInt32(CPath::GetModificationTime(m_PartPath));
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
		if (!m_corrupted_list.empty()) {			
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
		
		if (GetDlActiveTime()){
			++tagcount;
		}
		
		file.WriteUInt32(tagcount);

		//#warning Kry - Where are lost by coruption and gained by compression?
		
		// 0 (unicoded part file name) 
		// We write it with BOM to keep eMule compatibility. Note that the 'printable' filename is saved,
		// as presently the filename does not represent an actual file.
		CTagString(	FT_FILENAME,	GetFileName().GetPrintable()).WriteTagToFile( &file, utf8strOptBOM );
		CTagString(	FT_FILENAME,	GetFileName().GetPrintable()).WriteTagToFile( &file );                         // 1

		CTagIntSized(	FT_FILESIZE,	GetFileSize(), IsLargeFile() ? 64 : 32).WriteTagToFile( &file );// 2
		CTagIntSized(	FT_TRANSFERRED,	transferred, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );   // 3
		CTagInt32(	FT_STATUS,	(m_paused?1:0)).WriteTagToFile( &file );                        // 4

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
	
		CTagInt32(FT_CATEGORY,       m_category).WriteTagToFile( &file );                       // 10
		CTagInt32(FT_ATTRANSFERRED,   statistic.GetAllTimeTransferred() & 0xFFFFFFFF).WriteTagToFile( &file );// 11
		CTagInt32(FT_ATTRANSFERREDHI, statistic.GetAllTimeTransferred() >>32).WriteTagToFile( &file );// 12
		CTagInt32(FT_ATREQUESTED,    statistic.GetAllTimeRequests()).WriteTagToFile( &file );	// 13
		CTagInt32(FT_ATACCEPTED,     statistic.GetAllTimeAccepts()).WriteTagToFile( &file );	// 14

		// currupt part infos
		if (!m_corrupted_list.empty()) {
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
			CTagInt32(FT_KADLASTPUBLISHSRC, GetLastPublishTimeKadSrc()).WriteTagToFile(&file); // 15? 
		}
		
		if (GetLastPublishTimeKadNotes()){
			CTagInt32(FT_KADLASTPUBLISHNOTES, GetLastPublishTimeKadNotes()).WriteTagToFile(&file); // 16? 
		}		
		
		if (GetDlActiveTime()){
			CTagInt32(FT_DL_ACTIVE_TIME, GetDlActiveTime()).WriteTagToFile(&file); // 17
		}

		for (uint32 j = 0; j < (uint32)m_taglist.size();++j) {
			m_taglist[j].WriteTagToFile(&file);
		}
		
		// gaps
		unsigned i_pos = 0;
		for (CGapList::const_iterator it = m_gaplist.begin(); it != m_gaplist.end(); ++it) {
			wxString tagName = wxString::Format(wxT(" %u"), i_pos);
			
			// gap start = first missing byte but gap ends = first non-missing byte
			// in edonkey but I think its easier to user the real limits
			tagName[0] = FT_GAPSTART;
			CTagIntSized(tagName, it.start()		, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );
			
			tagName[0] = FT_GAPEND;
			CTagIntSized(tagName, it.end() + 1, IsLargeFile() ? 64 : 32).WriteTagToFile( &file );
			
			++i_pos;
		}
	} catch (const wxString& error) {
		AddLogLineNS(CFormat( _("ERROR while saving partfile: %s (%s ==> %s)") )
			% error
			% m_partmetfilename
			% GetFileName() );

		return false;
	} catch (const CIOFailureException& e) {
		AddLogLineCS(_("IO failure while saving partfile: ") + e.what());
		
		return false;
	}
	
	file.Close();

	if (!Initial) {
		CPath::RemoveFile(m_fullname.AppendExt(wxT(".backup")));
	}
	
	sint64 metLength = m_fullname.GetFileSize();
	if (metLength == wxInvalidOffset) {
		theApp->ShowAlert( CFormat( _("Could not retrieve length of '%s' - using %s file.") )
			% m_fullname
			% PARTMET_BAK_EXT,
			_("Message"), wxOK);

		CPath::CloneFile(m_fullname.AppendExt(PARTMET_BAK_EXT), m_fullname, true);
	} else if (metLength == 0) {
		// Don't backup if it's 0 size but raise a warning!!!
		theApp->ShowAlert( CFormat( _("'%s' is 0 size somehow - using %s file.") )
			% m_fullname
			% PARTMET_BAK_EXT,
			_("Message"), wxOK);
				
		CPath::CloneFile(m_fullname.AppendExt(PARTMET_BAK_EXT), m_fullname, true);
	} else {
		// no error, just backup
		CPath::BackupFile(m_fullname, PARTMET_BAK_EXT);
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
		if (!cur_src->HasLowID()) {
			source_seeds.push_back(cur_src);
			++n_sources;
		}
	}

	if (n_sources < MAX_SAVED_SOURCES) {
		// Not enough downloading sources to fill the list, going to sources list	
		if (GetSourceCount() > 0) {
			SourceSet::reverse_iterator rit = m_SrcList.rbegin();
			for ( ; ((rit != m_SrcList.rend()) && (n_sources<MAX_SAVED_SOURCES)); ++rit) {
				CUpDownClient* cur_src = *rit;
				if (!cur_src->HasLowID()) {
					source_seeds.push_back(cur_src);
					++n_sources;
				}
			}
		}
	}
	
	// Write the file
	if (!n_sources) {
		return;
	} 
	
	const CPath seedsPath = m_fullname.AppendExt(wxT(".seeds"));

	CFile file;
	file.Create(seedsPath, true);
	if (!file.IsOpened()) {
		AddLogLineM(false, CFormat( _("Failed to save part.met.seeds file for %s") )
			% m_fullname);
		return;
	}	

	try {
		file.WriteUInt8(0); // v3, to avoid v2 clients choking on it.
		file.WriteUInt8(source_seeds.size());
		
		CClientPtrList::iterator it2 = source_seeds.begin();
		for (; it2 != source_seeds.end(); ++it2) {
			CUpDownClient* cur_src = *it2;		
			file.WriteUInt32(cur_src->GetUserIDHybrid());
			file.WriteUInt16(cur_src->GetUserPort());
			file.WriteHash(cur_src->GetUserHash());
			// CryptSettings - See SourceExchange V4
			const uint8 uSupportsCryptLayer	= cur_src->SupportsCryptLayer() ? 1 : 0;
			const uint8 uRequestsCryptLayer	= cur_src->RequestsCryptLayer() ? 1 : 0;
			const uint8 uRequiresCryptLayer	= cur_src->RequiresCryptLayer() ? 1 : 0;
			const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
			file.WriteUInt8(byCryptOptions);		
		}

		/* v2: Added to keep track of too old seeds */
		file.WriteUInt32(wxDateTime::Now().GetTicks());
		
		AddLogLineM(false, CFormat( wxPLURAL("Saved %i source seed for partfile: %s (%s)", "Saved %i source seeds for partfile: %s (%s)", n_sources) )
			% n_sources
			% m_fullname
			% GetFileName());
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile, CFormat( wxT("Error saving partfile's seeds file (%s - %s): %s") )
				% m_partmetfilename
				% GetFileName()
				% e.what() );
		
		n_sources = 0;
		file.Close();
		CPath::RemoveFile(seedsPath);
	}
}	

void CPartFile::LoadSourceSeeds()
{	
	CMemFile sources_data;
	
	bool valid_sources = false;
	
	const CPath seedsPath = m_fullname.AppendExt(wxT(".seeds"));
	if (!seedsPath.FileExists()) {
		return;
	} 
	
	CFile file(seedsPath, CFile::read);
	if (!file.IsOpened()) {
		AddLogLineM(false, CFormat( _("Partfile %s (%s) has no seeds file") )
			% m_partmetfilename
			% GetFileName() );
		return;
	}	
	
		
	try {
		if (file.GetLength() <= 1) {
			AddLogLineM(false, CFormat( _("Partfile %s (%s) has a void seeds file") )
				% m_partmetfilename
				% GetFileName() );
			return;
		}

		uint8 src_count = file.ReadUInt8();

		bool bUseSX2Format = (src_count == 0);

		if (bUseSX2Format) {
			// v3 sources seeds
			src_count = file.ReadUInt8();
		}
		
		sources_data.WriteUInt16(src_count);
	
		for (int i = 0; i< src_count; ++i) {		
			uint32 dwID = file.ReadUInt32();
			uint16 nPort = file.ReadUInt16();

			sources_data.WriteUInt32(bUseSX2Format ? dwID : wxUINT32_SWAP_ALWAYS(dwID));
			sources_data.WriteUInt16(nPort);
			sources_data.WriteUInt32(0);
			sources_data.WriteUInt16(0);	

			if (bUseSX2Format) {
				sources_data.WriteHash(file.ReadHash());
				sources_data.WriteUInt8(file.ReadUInt8());
			}
			
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
		
		if (valid_sources) {
			sources_data.Seek(0);
			AddClientSources(&sources_data, SF_SOURCE_SEEDS, bUseSX2Format ? 4 : 1, bUseSX2Format);		
		}
	
	} catch (const CSafeIOException& e) {
		AddLogLineM(false, CFormat( _("Error reading partfile's seeds file (%s - %s): %s") )
				% m_partmetfilename
				% GetFileName()
				% e.what() );		
	}

	file.Close();
}		

void CPartFile::PartFileHashFinished(CKnownFile* result)
{
	m_lastDateChanged = result->m_lastDateChanged;
	bool errorfound = false;
	if (GetED2KPartHashCount() == 0){
		if (IsComplete(0, GetFileSize()-1)){
			if (result->GetFileHash() != GetFileHash()){
				AddLogLineM(false,
					CFormat(wxPLURAL(
						"Found corrupted part (%d) in %d part file %s - FileResultHash |%s| FileHash |%s|",
						"Found corrupted part (%d) in %d parts file %s - FileResultHash |%s| FileHash |%s|",
						0)
					)
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
			// Very nice feature, if a file is completed but .part.met don't believe it,
			// update it.
			
			uint64 partStart = i * PARTSIZE;
			uint64 partEnd   = partStart + GetPartSize(i) - 1;
			if (!( i < result->GetHashCount() && (result->GetPartHash(i) == GetPartHash(i)))){
				if (IsComplete(i)) {
					CMD4Hash wronghash;
					if ( i < result->GetHashCount() )
						wronghash = result->GetPartHash(i);
			
					AddLogLineM(false,
						CFormat(wxPLURAL(
							"Found corrupted part (%d) in %d part file %s - FileResultHash |%s| FileHash |%s|",
							"Found corrupted part (%d) in %d parts file %s - FileResultHash |%s| FileHash |%s|",
							GetED2KPartHashCount())
						)
						% ( i + 1 )
						% GetED2KPartHashCount()
						% GetFileName()
						% wronghash.Encode()
						% GetPartHash(i).Encode() );
				
					AddGap(i);
					errorfound = true;
				}
			} else {
				if (!IsComplete(i)){
					AddLogLineM(false, CFormat( _("Found completed part (%i) in %s") )
						% ( i + 1 )
						% GetFileName() );

					FillGap(i);
					RemoveBlockFromList(partStart, partEnd);
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
			CFormat(wxT("Failed to store new AICH Hashset for completed file: %s"))
				% GetFileName());
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
	theApp->sharedfiles->SafeAddKFile(this);		
}

void CPartFile::AddGap(uint64 start, uint64 end)
{
	m_gaplist.AddGap(start, end);
	UpdateDisplayedInfo();
}

void CPartFile::AddGap(uint16 part)
{
	m_gaplist.AddGap(part);
	UpdateDisplayedInfo();
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
	// Find start of this part
	uint64 partStart = (PARTSIZE * partNumber);
	uint64 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint64 partEnd = partStart + GetPartSize(partNumber) - 1;
	// Loop until find a suitable gap and return true, or no more gaps and return false
	CGapList::const_iterator it = m_gaplist.begin();
	while (true) {
		bool noGap = true;
		uint64 gapStart, end;

		// Find the first gap from the start position
		for (; it != m_gaplist.end(); ++it) {
			gapStart = it.start();
			end = it.end();
			
			// Want gaps that overlap start<->partEnd
			if (gapStart <= partEnd && end >= start) {
				noGap = false;
				break;
			} else if (gapStart > partEnd) {
				break;
			}
		}

		// If no gaps after start, exit
		if (noGap) {
			return false;
		}
		// Update start position if gap starts after current pos
		if (start < gapStart) {
			start = gapStart;
		}
		// Find end, keeping within the max block size and the part limit
		uint64 blockLimit = partStart + (BLOCKSIZE * (((start - partStart) / BLOCKSIZE) + 1)) - 1;
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
	m_gaplist.FillGap(start, end);
	UpdateCompletedInfos();
	UpdateDisplayedInfo();
}

void CPartFile::FillGap(uint16 part)
{
	m_gaplist.FillGap(part);
	UpdateCompletedInfos();
	UpdateDisplayedInfo();
}


void CPartFile::UpdateCompletedInfos()
{
	uint64 allgaps = m_gaplist.GetGapSize();

	percentcompleted = (1.0 - (double)allgaps/GetFileSize()) * 100.0;
	completedsize = GetFileSize() - allgaps;
}


void CPartFile::WritePartStatus(CMemFile* file)
{
	uint16 parts = GetED2KPartCount();
	file->WriteUInt16(parts);
	uint16 done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (uint32 i = 0;i != 8;++i) {
			if (IsComplete(done)) {
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
					if (cur_src->HasLowID() && !theApp->CanDoCallback(cur_src)) {
						// If we are almost maxed on sources,
						// slowly remove these client to see 
						// if we can find a better source.
						if (((dwCurTick - lastpurgetime) > 30000) &&
							(GetSourceCount() >= (thePrefs::GetMaxSourcePerFile()*.8))) {
							RemoveSource(cur_src);
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
					if (	theApp->IsConnected() &&
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
					if (	theApp->IsConnected() &&
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

		if (((old_trans==0) && (transferingsrc>0)) || ((old_trans>0) && (transferingsrc==0))) {
			SetPartFileStatus(status);
		}
	
		// Kad source search		
		if( GetMaxSourcePerFileUDP() > GetSourceCount()){
			//Once we can handle lowID users in Kad, we remove the second IsConnected
			if (theApp->downloadqueue->DoKademliaFileRequest() && (Kademlia::CKademlia::GetTotalFile() < KADEMLIATOTALFILE) && (dwCurTick > m_LastSearchTimeKad) &&  Kademlia::CKademlia::IsConnected() && theApp->IsConnected() && !IsStopped()){ 
				//Kademlia
				theApp->downloadqueue->SetLastKademliaFileRequest();
			
				if (GetKadFileSearchID()) {
					/*	This will never happen anyway. We're talking a 
						1h timespan and searches are at max 45secs */
					Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), false);
				}
			
				Kademlia::CUInt128 kadFileID(GetFileHash().GetHash());
				Kademlia::CSearch* pSearch = Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::FILE, true, kadFileID);
				AddDebugLogLineM(false, logKadSearch, CFormat(wxT("Preparing a Kad Search for '%s'")) % GetFileName());
				if (pSearch) {
					AddDebugLogLineM(false, logKadSearch, CFormat(wxT("Kad lookup started for '%s'")) % GetFileName());
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
			theApp->IsConnectedED2K() &&
			thePrefs::GetMaxSourcePerFileSoft() > GetSourceCount() &&
			!m_stopped ) {
			m_localSrcReqQueued = true;
			theApp->downloadqueue->SendLocalSrcRequest(this);
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

	// release file handle if unused for some time
	m_hpartfile.Release();
	
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
	if (theApp->IsConnectedED2K()) {
		if(::IsLowID(theApp->GetED2KID())) {
			if(theApp->GetED2KID() == userid && theApp->serverconnect->GetCurrentServer()->GetIP() == serverip && theApp->serverconnect->GetCurrentServer()->GetPort() == serverport ) {
				return false;
			}
			if(theApp->GetPublicIP() == userid) {
				return false;
			}
		} else {
			if(theApp->GetED2KID() == userid && thePrefs::GetPort() == port) {
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
	if ( IsLowID(hybridID) && theApp->IsFirewalled()) {
		if (pdebug_lowiddropped) {
			(*pdebug_lowiddropped)++;
		}
		return false;
	}
	// MOD Note - end
	return true;
}

void CPartFile::AddSources(CMemFile& sources,uint32 serverip, uint16 serverport, unsigned origin, bool bWithObfuscationAndHash)
{
	uint8 count = sources.ReadUInt8();
	uint8 debug_lowiddropped = 0;
	uint8 debug_possiblesources = 0;
	CMD4Hash achUserHash;
	
	if (m_stopped) {
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		AddDebugLogLineM(false, logPartFile, wxT("Trying to add sources for a stopped file"));
		sources.Seek(count*(4+2), wxFromCurrent);
		return;
	}

	for (int i = 0;i != count;++i) {
		uint32 userid = sources.ReadUInt32();
		uint16 port   = sources.ReadUInt16();
		
		uint8 byCryptOptions = 0;
		if (bWithObfuscationAndHash){
			byCryptOptions = sources.ReadUInt8();
			if ((byCryptOptions & 0x80) > 0) {
				achUserHash = sources.ReadHash();
			}

			if ((thePrefs::IsClientCryptLayerRequested() && (byCryptOptions & 0x01/*supported*/) > 0 && (byCryptOptions & 0x80) == 0)
				|| (thePrefs::IsClientCryptLayerSupported() && (byCryptOptions & 0x02/*requested*/) > 0 && (byCryptOptions & 0x80) == 0)) {
				AddDebugLogLineM(false, logPartFile, wxString::Format(wxT("Server didn't provide UserHash for source %u, even if it was expected to (or local obfuscationsettings changed during serverconnect"), userid));
			} else if (!thePrefs::IsClientCryptLayerRequested() && (byCryptOptions & 0x02/*requested*/) == 0 && (byCryptOptions & 0x80) != 0) {
				AddDebugLogLineM(false, logPartFile, wxString::Format(wxT("Server provided UserHash for source %u, even if it wasn't expected to (or local obfuscationsettings changed during serverconnect"), userid));
			}
		}
			
		
		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (!IsLowID(userid)) {
			// check for 0-IP, localhost and optionally for LAN addresses
			if ( !IsGoodIP(userid, thePrefs::FilterLanIPs()) ) {
				continue;
			}
			if (theApp->ipfilter->IsFiltered(userid)) {
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
			newsource->SetConnectOptions(byCryptOptions, true, false);

			if ((byCryptOptions & 0x80) != 0) {
				newsource->SetUserHash(achUserHash);
			}

			theApp->downloadqueue->CheckAndAddSource(this,newsource);
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

// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]
bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, 
							std::vector<Requested_Block_Struct*>& toadd, uint16& count)
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
	if ( sender->GetPartStatus().empty() ) {
		return false;
	}
	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	ChunkList chunksList;
	
	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != count) {
		// Create a request block stucture if a chunk has been previously selected
		if(sender->GetLastPartAsked() != 0xffff) {
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->GetLastPartAsked(), pBlock) == true) {
				// Keep a track of all pending requested blocks
				m_requestedblocks_list.push_back(pBlock);
				// Update list of blocks to return
				toadd.push_back(pBlock);
				newBlockCount++;
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
					const uint64 uEnd   = uStart + GetPartSize(cur_chunk.part) - 1;
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
					// PARTSIZE instead of GetPartSize() favours the last chunk - but that may be intentional
					uint32 partSize = PARTSIZE - m_gaplist.GetGapSize(cur_chunk.part);
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
			if(!chunksList.empty()) {
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
	count = newBlockCount;
	// Return
	return (newBlockCount > 0);
}
// Maella end


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

	theApp->downloadqueue->RemoveLocalServerRequest(this);

	AddDebugLogLineM( false, logPartFile, wxString( wxT("CPartFile::CompleteFile: Hash ") ) + ( bIsHashingDone ? wxT("done") : wxT("not done") ) );
			  
	if (!bIsHashingDone) {
		SetPartFileStatus(PS_COMPLETING);
		kBpsDown = 0.0;

		CPath partFile = m_partmetfilename.RemoveExt();
		CThreadScheduler::AddTask(new CHashingTask(GetFilePath(), partFile, this));
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


void CPartFile::CompleteFileEnded(bool errorOccured, const CPath& newname)
{	
	if (errorOccured) {
		m_paused = true;
		SetPartFileStatus(PS_ERROR);
		AddLogLineM(true, CFormat( _("Unexpected error while completing %s. File paused") )% GetFileName() );
	} else {
		m_fullname = newname;

		SetFilePath(m_fullname.GetPath());
		SetFileName(m_fullname.GetFullName());
		m_lastDateChanged = CPath::GetModificationTime(m_fullname);
		
		SetPartFileStatus(PS_COMPLETE);
		m_paused = false;
		ClearPriority();
		

		// Remove from list of canceled files in case it was canceled once upon a time
		if (theApp->canceledfiles->Remove(GetFileHash())) {
			theApp->canceledfiles->Save();
		}

		// Mark as known (checks if it's already known),
		// also updates search files
		theApp->knownfiles->SafeAddKFile(this);		

		// remove the file from the suspended uploads list
		theApp->uploadqueue->ResumeUpload(GetFileHash());		
		theApp->downloadqueue->RemoveFile(this);
		theApp->sharedfiles->SafeAddKFile(this);
		UpdateDisplayedInfo(true);

		// republish that file to the ed2k-server to update the 'FT_COMPLETE_SOURCES' counter on the server.
		theApp->sharedfiles->RepublishFile(this);		
		
		// Ensure that completed shows the correct value
		completedsize = GetFileSize();

		// clear the blackbox to free up memory
		m_CorruptionBlackBox->Free();

		AddLogLineM(true, CFormat( _("Finished downloading: %s") ) % GetFileName() );
	}
	
	theApp->downloadqueue->StartNextFile(this);	
}


void CPartFile::PerformFileComplete()
{
	// add this file to the suspended uploads list
	theApp->uploadqueue->SuspendUpload(GetFileHash());
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
	
	uint16 removed = theApp->uploadqueue->SuspendUpload(GetFileHash());
	AddDebugLogLineM(false, logPartFile, CFormat(wxT("\tSuspended upload to %d clients")) % removed);
	theApp->sharedfiles->RemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved from shared"));
	theApp->downloadqueue->RemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved from download queue"));
	Notify_DownloadCtrlRemoveFile(this);
	AddDebugLogLineM(false, logPartFile, wxT("\tRemoved from transferwnd"));
	if (theApp->canceledfiles->Add(GetFileHash())) {
		theApp->canceledfiles->Save();
	}
	AddDebugLogLineM(false, logPartFile, wxT("\tAdded to canceled file list"));
	theApp->searchlist->UpdateSearchFileByHash(GetFileHash()); 	// Update file in the search dialog if it's still open

	if (m_hpartfile.IsOpened()) {
		m_hpartfile.Close();
	}

	AddDebugLogLineM(false, logPartFile, wxT("\tClosed"));
	
	if (!CPath::RemoveFile(m_fullname)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("\tFailed to delete '%s'")) % m_fullname);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .part.met"));
	}

	if (!CPath::RemoveFile(m_PartPath)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("Failed to delete '%s'")) % m_PartPath);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .part"));
	}
	
	CPath BAKName = m_fullname.AppendExt(PARTMET_BAK_EXT);
	if (!CPath::RemoveFile(BAKName)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("Failed to delete '%s'")) % BAKName);
	} else {
		AddDebugLogLineM(false, logPartFile, wxT("\tRemoved .BAK"));
	}
	
	CPath SEEDSName = m_fullname.AppendExt(wxT(".seeds"));
	if (SEEDSName.FileExists()) {
		if (CPath::RemoveFile(SEEDSName)) {
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
			CFormat( _("WARNING: Unable to hash downloaded part - hashset incomplete for '%s'") )
				% GetFileName() );
		m_hashsetneeded = true;
		return true;
	} else if ((GetHashCount() <= partnumber) && GetPartCount() != 1) {
		AddLogLineM(true, CFormat( _("ERROR: Unable to hash downloaded part - hashset incomplete (%s). This should never happen")) % GetFileName() );
		m_hashsetneeded = true;
		return true;		
	} else {
		CMD4Hash hashresult;
		uint64 offset = PARTSIZE * partnumber;
		uint32 length = GetPartSize(partnumber);
		try {
			CreateHashFromFile(m_hpartfile, offset, length, &hashresult, NULL);
		} catch (const CIOFailureException& e) {
			AddLogLineM(true, CFormat( wxT("EOF while hashing downloaded part %u with length %u (max %u) of partfile '%s' with length %u: %s"))
				% partnumber % length % (offset+length) % GetFileName() % GetFileSize() % e.what());
			SetPartFileStatus(PS_ERROR);
			return false;
		} catch (const CEOFException& e) {
			AddLogLineM(true, CFormat( wxT("EOF while hashing downloaded part %u with length %u (max %u) of partfile '%s' with length %u: %s"))
				% partnumber % length % (offset+length) % GetFileName() % GetFileSize() % e.what());
			return false;
		}

		if (GetPartCount() > 1) {
			if (hashresult != GetPartHash(partnumber)) {
				AddDebugLogLineM(false, logPartFile, CFormat( wxT("%s: Expected hash of part %d: %s")) % GetFileName() % partnumber % GetPartHash(partnumber).Encode() );
				AddDebugLogLineM(false, logPartFile, CFormat( wxT("%s: Actual   hash of part %d: %s")) % GetFileName() % partnumber % hashresult.Encode() );
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
	
	if (!bCancel) {
		FlushBuffer();
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
				kBpsDown = 0.0;
		}
	}
	// release file handle if unused for some time
	m_hpartfile.Release();
}


void CPartFile::PauseFile(bool bInsufficient)
{
	SetActive(false);
	
	if ( status == PS_COMPLETE || status == PS_COMPLETING ) {
		return;
	}

	if (GetKadFileSearchID()) {
		Kademlia::CSearchManager::StopSearch(GetKadFileSearchID(), true);
		// If we were in the middle of searching, reset timer so they can resume searching.
		m_LastSearchTimeKad = 0; 
	}

	m_iLastPausePurge = time(NULL);
	
	theApp->downloadqueue->RemoveLocalServerRequest(this);

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
			// Allow immediate reconnect on resume
			cur_src->ResetLastAskedTime();
		}
	}

	
	m_insufficient = bInsufficient;
	m_paused = true;
	
	
	kBpsDown = 0.0;
	transferingsrc = 0;

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
	SetActive(theApp->IsConnected());

	if (m_gaplist.IsComplete() && (GetStatus() == PS_ERROR)) {
		// The file has already been hashed at this point
		CompleteFile(true);
	}
	
	UpdateDisplayedInfo(true);
}


bool CPartFile::CheckFreeDiskSpace( uint64 neededSpace )
{
	uint64 free = CPath::GetFreeSpaceAt(GetFilePath());
	if (free == static_cast<uint64>(wxInvalidOffset)) {
		// If GetFreeSpaceAt() fails, then the path probably does not exist.
		return false;
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

CPacket *CPartFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions)
{

	if ( m_SrcList.empty() ) {
		return NULL;
	}

	if(!IsPartFile())  {
		return CKnownFile::CreateSrcInfoPacket(forClient, byRequestedVersion, nRequestedOptions);
	}
	
	if (((forClient->GetRequestFile() != this)
		&& (forClient->GetUploadFile() != this)) || forClient->GetUploadFileID() != GetFileHash()) {
		wxString file1 = _("Unknown");
		if (forClient->GetRequestFile() && forClient->GetRequestFile()->GetFileName().IsOk()) {
			file1 = forClient->GetRequestFile()->GetFileName().GetPrintable();
		} else if (forClient->GetUploadFile() && forClient->GetUploadFile()->GetFileName().IsOk()) {
			file1 = forClient->GetUploadFile()->GetFileName().GetPrintable();
		}
		wxString file2 = _("Unknown");
		if (GetFileName().IsOk()) {
			file2 = GetFileName().GetPrintable();
		}
		AddDebugLogLineM(false, logPartFile, wxT("File mismatch on source packet (P) Sending: ") + file1 + wxT("  From: ") + file2);
		return NULL;
	}

	if ( !(GetStatus() == PS_READY || GetStatus() == PS_EMPTY)) {
		return NULL;
	}

	const BitVector& reqstatus = forClient->GetPartStatus();
	bool KnowNeededParts = !reqstatus.empty();
	//wxASSERT(rcvstatus.size() == GetPartCount()); // Obviously!
	if (KnowNeededParts && (reqstatus.size() != GetPartCount())) {
		// Yuck. Same file but different part count? Seriously fucked up.
		// This happens rather often with reqstatus.size() == 0. Don't log then.
		if (reqstatus.size()) {
			AddDebugLogLineM(false, logKnownFiles, CFormat(wxT("Impossible situation: different partcounts: %i (client) and %i (file) for %s")) % reqstatus.size() % GetPartCount() % GetFileName());
		}
		return NULL;
	}	
	
	CMemFile data(1024);
	
	uint8 byUsedVersion;
	bool bIsSX2Packet;
	if (forClient->SupportsSourceExchange2() && byRequestedVersion > 0){
		// the client uses SourceExchange2 and requested the highest version he knows
		// and we send the highest version we know, but of course not higher than his request
		byUsedVersion = std::min(byRequestedVersion, (uint8)SOURCEEXCHANGE2_VERSION);
		bIsSX2Packet = true;
		data.WriteUInt8(byUsedVersion);

		// we don't support any special SX2 options yet, reserved for later use
		if (nRequestedOptions != 0) {
			AddDebugLogLineM(false, logKnownFiles, CFormat(wxT("Client requested unknown options for SourceExchange2: %u")) % nRequestedOptions);
		}
	} else {
		byUsedVersion = forClient->GetSourceExchange1Version();
		bIsSX2Packet = false;
		if (forClient->SupportsSourceExchange2()) {
			AddDebugLogLineM(false, logKnownFiles, wxT("Client which announced to support SX2 sent SX1 packet instead"));
		}
	}
	
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
			if(forClient->GetSourceExchange1Version() > 2) {
				dwID = cur_src->GetUserIDHybrid();
			} else {
				dwID = wxUINT32_SWAP_ALWAYS(cur_src->GetUserIDHybrid());
			}
			data.WriteUInt32(dwID);
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			
			if (byUsedVersion >= 2) {
			    data.WriteHash(cur_src->GetUserHash());
			}
			
			if (byUsedVersion >= 4){
				// CryptSettings - SourceExchange V4
				// 5 Reserved (!)
				// 1 CryptLayer Required
				// 1 CryptLayer Requested
				// 1 CryptLayer Supported
				const uint8 uSupportsCryptLayer	= cur_src->SupportsCryptLayer() ? 1 : 0;
				const uint8 uRequestsCryptLayer	= cur_src->RequestsCryptLayer() ? 1 : 0;
				const uint8 uRequiresCryptLayer	= cur_src->RequiresCryptLayer() ? 1 : 0;
				const uint8 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0);
				data.WriteUInt8(byCryptOptions);
			}			
			
			if (nCount > 500) {
				break;
			}
		}
	}
	if (!nCount) {
		return 0;
	}
	data.Seek(bIsSX2Packet ? 17 : 16, wxFromStart);
	data.WriteUInt16(nCount);

	CPacket* result = new CPacket(data, OP_EMULEPROT, bIsSX2Packet ? OP_ANSWERSOURCES2 : OP_ANSWERSOURCES);

	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if (result->GetPacketSize() > 354) {
		result->PackPacket();
	}
	
	return result;
}

void CPartFile::AddClientSources(CMemFile* sources, unsigned nSourceFrom, uint8 uClientSXVersion, bool bSourceExchange2, const CUpDownClient* /*pClient*/)
{
	// Kad reviewed
	
	if (m_stopped) {
		return;
	}

	uint32 nCount = 0;
	uint8 uPacketSXVersion = 0;
	if (!bSourceExchange2) {
		nCount = sources->ReadUInt16();
		
		// Check if the data size matches the 'nCount' for v1 or v2 and eventually correct the source
		// exchange version while reading the packet data. Otherwise we could experience a higher
		// chance in dealing with wrong source data, userhashs and finally duplicate sources.
		uint32 uDataSize = sources->GetLength() - sources->GetPosition();
		
		if ((uint32)(nCount*(4+2+4+2)) == uDataSize) { //Checks if version 1 packet is correct size
			if(uClientSXVersion != 1) {
				return;
			}
			uPacketSXVersion = 1;
		} else if ((uint32)(nCount*(4+2+4+2+16)) == uDataSize) { // Checks if version 2&3 packet is correct size
			if (uClientSXVersion == 2) {
				uPacketSXVersion = 2;
			} else if (uClientSXVersion > 2) {
				uPacketSXVersion = 3;
			} else {
				return;
			}
		} else if (nCount*(4+2+4+2+16+1) == uDataSize) {
			if (uClientSXVersion != 4 ) {
				return;
			}
			uPacketSXVersion = 4;
		} else {
			// If v5 inserts additional data (like v2), the above code will correctly filter those packets.
			// If v5 appends additional data after <count>(<Sources>)[count], we are in trouble with the 
			// above code. Though a client which does not understand v5+ should never receive such a packet.
			AddDebugLogLineM(false, logClient, CFormat(wxT("Received invalid source exchange packet (v%u) of data size %u for %s")) % uClientSXVersion % uDataSize % GetFileName());
			return;
		}
	} else {
		// for SX2:
		// We only check if the version is known by us and do a quick sanitize check on known version
		// other then SX1, the packet will be ignored if any error appears, sicne it can't be a "misunderstanding" anymore
		if (uClientSXVersion > SOURCEEXCHANGE2_VERSION || uClientSXVersion == 0 ){
			AddDebugLogLineM(false, logPartFile, CFormat(wxT("Invalid source exchange type version: %i")) % uClientSXVersion);
			return;
		}
		
		// all known versions use the first 2 bytes as count and unknown version are already filtered above
		nCount = sources->ReadUInt16();
		uint32 uDataSize = (uint32)(sources->GetLength() - sources->GetPosition());	
		bool bError = false;
		switch (uClientSXVersion){
			case 1:
				bError = nCount*(4+2+4+2) != uDataSize;
				break;
			case 2:
			case 3:
				bError = nCount*(4+2+4+2+16) != uDataSize;
				break;
			case 4:
				bError = nCount*(4+2+4+2+16+1) != uDataSize;
				break;
			default:
				wxFAIL;
		}

		if (bError){
			wxFAIL;
			AddDebugLogLineM(false, logPartFile, wxT("Invalid source exchange data size."));
			return;
		}
		uPacketSXVersion = uClientSXVersion;		
	}
	
	for (uint16 i = 0;i != nCount;++i) {
		
		uint32 dwID = sources->ReadUInt32();
		uint16 nPort = sources->ReadUInt16();
		uint32 dwServerIP = sources->ReadUInt32();
		uint16 nServerPort = sources->ReadUInt16();
	
		CMD4Hash userHash;
		if (uPacketSXVersion > 1) {
			userHash = sources->ReadHash();
		}
		
		uint8 byCryptOptions = 0;
		if (uPacketSXVersion >= 4) {
			byCryptOptions = sources->ReadUInt8();
		}
		
		//Clients send ID's the the Hyrbid format so highID clients with *.*.*.0 won't be falsely switched to a lowID..
		uint32 dwIDED2K;
		if (uPacketSXVersion >= 3) {
			dwIDED2K = wxUINT32_SWAP_ALWAYS(dwID);
		} else {
			dwIDED2K = dwID;
		}
		
		// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (!IsLowID(dwID)) {
			if (!IsGoodIP(dwIDED2K, thePrefs::FilterLanIPs())) {
				// check for 0-IP, localhost and optionally for LAN addresses
				AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via %s - bad IP")) % Uint32toStringIP(dwIDED2K) % OriginToText(nSourceFrom));
				continue;
			}
			if (theApp->ipfilter->IsFiltered(dwIDED2K)) {
				AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via %s - IPFilter")) % Uint32toStringIP(dwIDED2K) % OriginToText(nSourceFrom));
				continue;
			}
			if (theApp->clientlist->IsBannedClient(dwIDED2K)){
				continue;
			}
		}

		// additionally check for LowID and own IP
		if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort, NULL, false)) {
			AddDebugLogLineM(false, logIPFilter, CFormat(wxT("Ignored source (IP=%s) received via source exchange")) % Uint32toStringIP(dwIDED2K));
			continue;
		}
		
		if(thePrefs::GetMaxSourcePerFile() > GetSourceCount()) {
			CUpDownClient* newsource = new CUpDownClient(nPort,dwID,dwServerIP,nServerPort,this, (uPacketSXVersion < 3), true);
			if (uPacketSXVersion > 1) {
				newsource->SetUserHash(userHash);
			}
			
			if (uPacketSXVersion >= 4) {
				newsource->SetConnectOptions(byCryptOptions, true, false);
			}

			newsource->SetSourceFrom((ESourceFrom)nSourceFrom);
			theApp->downloadqueue->CheckAndAddSource(this,newsource);
			
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
uint32 CPartFile::WriteToBuffer(uint32 transize, byte* data, uint64 start, uint64 end, Requested_Block_Struct *block, const CUpDownClient* client)
{
	// Increment transferred bytes counter for this file
	transferred += transize;

	// This is needed a few times
	// Kry - should not need a uint64 here - no block is larger than
	// 2GB even after uncompressed.
	uint32 lenData = (uint32) (end - start + 1);

	if(lenData > transize) {
		m_iGainDueToCompression += lenData-transize;
	}

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end)) {
		AddDebugLogLineM(false, logPartFile,	
			CFormat(wxT("File '%s' has already been written from %u to %u"))
				% GetFileName() % start % end);
		return 0;
	}

	// security sanitize check to make sure we do not write anything into an already hashed complete chunk
	const uint64 nStartChunk = start / PARTSIZE;
	const uint64 nEndChunk = end / PARTSIZE;
	if (IsComplete(nStartChunk)) {
		AddDebugLogLineM(false, logPartFile, CFormat(wxT("Received data touches already hashed chunk - ignored (start): %u-%u; File=%s")) % start % end % GetFileName());
		return 0;
	} else if (nStartChunk != nEndChunk) {
		if (IsComplete(nEndChunk)) {
			AddDebugLogLineM(false, logPartFile, CFormat(wxT("Received data touches already hashed chunk - ignored (end): %u-%u; File=%s")) % start % end % GetFileName());
			return 0;
		}
#ifdef __DEBUG__
		else {
			AddDebugLogLineM(false, logPartFile, CFormat(wxT("Received data crosses chunk boundaries: %u-%u; File=%s")) % start % end % GetFileName());
		}
#endif
	}

	// log transferinformation in our "blackbox"
	m_CorruptionBlackBox->TransferredData(start, end, client->GetIP());

	// Create a new buffered queue entry
	PartFileBufferedData *item = new PartFileBufferedData(m_hpartfile, data, start, end, block);

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

	if (m_gaplist.IsComplete()) {
		FlushBuffer();
	}

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(bool fromAICHRecoveryDataAvailable)
{
	m_nLastBufferFlushTime = GetTickCount();
	
	if (m_BufferedData_list.empty()) {
		return;
	}

	
	uint32 partCount = GetPartCount();
	// Remember which parts need to be checked at the end of the flush
	std::vector<bool> changedPart(partCount, false);
	
	// Ensure file is big enough to write data to (the last item will be the furthest from the start)
	if (!CheckFreeDiskSpace(m_nTotalBufferData)) {
		// Not enough free space to write the last item, bail
		AddLogLineM(true, CFormat( _("WARNING: Not enough free disk-space! Pausing file: %s") ) % GetFileName());
	
		PauseFile( true );
		return;
	}
	
	// Loop through queue
	while ( !m_BufferedData_list.empty() ) {
		// Get top item and remove it from the queue
		CScopedPtr<PartFileBufferedData> item(m_BufferedData_list.front());
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
			item->area.FlushAt(m_hpartfile, item->start, lenData);
			// Decrease buffer size
			m_nTotalBufferData -= lenData;
		} catch (const CIOFailureException& e) {
			AddDebugLogLineM(true, logPartFile, wxT("Error while saving part-file: ") + e.what());
			SetPartFileStatus(PS_ERROR);
			// No need to bang your head against it again and again if it has already failed.
			DeleteContents(m_BufferedData_list);
			m_nTotalBufferData = 0;
			return;
		}
	}
	
	
	// Update last-changed date
	m_lastDateChanged = wxDateTime::GetTimeNow();

	try {	
		// Partfile should never be too large
		if (m_hpartfile.GetLength() > GetFileSize()) {
			// it's "last chance" correction. the real bugfix has to be applied 'somewhere' else
			m_hpartfile.SetLength(GetFileSize());
		}
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile,
			CFormat(wxT("Error while truncating part-file (%s): %s"))
				% m_PartPath % e.what());
		SetPartFileStatus(PS_ERROR);
	}


	
	// Check each part of the file
	for (uint16 partNumber = 0; partNumber < partCount; ++partNumber) {
		if (changedPart[partNumber] == false) {
			continue;
		}

		uint32 partRange = GetPartSize(partNumber) - 1;
	
		// Is this 9MB part complete
		if (IsComplete(partNumber)) {
			// Is part corrupt
			if (!HashSinglePart(partNumber)) {
				AddLogLineM(true, CFormat(
					_("Downloaded part %i is corrupt in file: %s") ) % partNumber % GetFileName() );
				AddGap(partNumber);
				// add part to corrupted list, if not already there
				if (!IsCorruptedPart(partNumber)) {
					m_corrupted_list.push_back(partNumber);
				}
				// request AICH recovery data
				// Don't if called from the AICHRecovery. It's already there and would lead to an infinite recursion.
				if (!fromAICHRecoveryDataAvailable) { 
					RequestAICHRecovery(partNumber);					
				}
				// Reduce transferred amount by corrupt amount
				m_iLostDueToCorruption += (partRange + 1);
			} else {
				if (!m_hashsetneeded) {
					AddDebugLogLineM(false, logPartFile, CFormat(
						wxT("Finished part %u of '%s'")) % partNumber % GetFileName());
				}
				
				// tell the blackbox about the verified data
				m_CorruptionBlackBox->VerifiedData(true, partNumber, 0, partRange);

				// if this part was successfully completed (although ICH is active), remove from corrupted list
				EraseFirstValue(m_corrupted_list, partNumber);
				
				if (status == PS_EMPTY) {
					if (theApp->IsRunning()) { // may be called during shutdown!
						if (GetHashCount() == GetED2KPartHashCount() && !m_hashsetneeded) {
							// Successfully completed part, make it available for sharing
							SetStatus(PS_READY);
							theApp->sharedfiles->SafeAddKFile(this);
						}
					}
				}
			}
		} else if ( IsCorruptedPart(partNumber) &&		// corrupted part:
					(thePrefs::IsICHEnabled()			// old ICH:  rehash whenever we have new data hoping it will be good now
					|| fromAICHRecoveryDataAvailable)) {// new AICH: one rehash right before performing it (maybe it's already good)
			// Try to recover with minimal loss
			if (HashSinglePart(partNumber)) {
				++m_iTotalPacketsSavedDueToICH;
				
				uint64 uMissingInPart = m_gaplist.GetGapSize(partNumber);					
				FillGap(partNumber);
				RemoveBlockFromList(PARTSIZE*partNumber,(PARTSIZE*partNumber + partRange));

				// tell the blackbox about the verified data
				m_CorruptionBlackBox->VerifiedData(true, partNumber, 0, partRange);

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
						if (theApp->IsRunning()) // may be called during shutdown!
							theApp->sharedfiles->SafeAddKFile(this);
					}
				}
			}
		}
	}

	// Update met file
	SavePartFile();

	if (theApp->IsRunning()) { // may be called during shutdown!
		// Is this file finished ?
		if (m_gaplist.IsComplete()) {
			CompleteFile(false);
		}
	}
}


// read data for upload, return false on error
bool CPartFile::ReadData(CFileArea & area, uint64 offset, uint32 toread)
{
	// Sanity check
	if (offset + toread > GetFileSize()) {
		AddDebugLogLineM(false, logPartFile, CFormat(wxT("tried to read %d bytes past eof of %s")) 
			% (offset + toread - GetFileSize()) % GetFileName());
		wxFAIL;
		return false;
	}

	area.ReadAt(m_hpartfile, offset, toread);
	// if it fails it throws (which the caller should catch)
	return true;
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
			if (thePrefs::IsCommentFiltered(cur_src->GetFileComment())) {
				continue;
			}
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


void CPartFile::SetCategory(uint8 cat)
{
	wxASSERT( cat < theApp->glob_prefs->GetCatCount() );
	
	m_category = cat; 
	SavePartFile(); 
}

bool CPartFile::RemoveSource(CUpDownClient* toremove, bool updatewindow, bool bDoStatsUpdate)
{
	wxASSERT( toremove );

	bool result = theApp->downloadqueue->RemoveSource( toremove, updatewindow, bDoStatsUpdate );

	// Check if the client should be deleted, but not if the client is already dying
	if ( !toremove->GetSocket() && !toremove->HasBeenDeleted() ) {
		if ( toremove->Disconnected(wxT("RemoveSource - purged")) ) {
			toremove->Safe_Delete();
		}
	}

	return result;
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
	try {
		uint64 length = m_hpartfile.GetLength();

		if (length > GetFileSize()) {
			return 0;	// Shouldn't happen, but just in case
		}
	
		return GetFileSize() - length;
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logPartFile,
			CFormat(wxT("Error while retrieving file-length (%s): %s"))
				% m_PartPath % e.what());
		SetPartFileStatus(PS_ERROR);
		return 0;
	}
}

void CPartFile::SetStatus(uint8 in)
{
	wxASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );
	
	status = in;
	
	if (theApp->IsRunning()) {
		UpdateDisplayedInfo( true );
	
		if ( thePrefs::ShowCatTabInfos() ) {
			Notify_ShowUpdateCatTabTitles();
		}
	}
}


void CPartFile::RequestAICHRecovery(uint16 nPart)
{

	if (	!m_pAICHHashSet->HasValidMasterHash() ||
		(m_pAICHHashSet->GetStatus() != AICH_TRUSTED && m_pAICHHashSet->GetStatus() != AICH_VERIFIED)){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Unable to request AICH Recoverydata because we have no trusted Masterhash") );
		return;
	}
	if (GetPartSize(nPart) <= EMBLOCKSIZE)
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
		wxFAIL;
		return;
	}

	AddDebugLogLineM( false, logAICHRecovery, CFormat( wxT("Requesting AICH Hash (%s) form client %s") ) % ( cAICHClients ? wxT("HighId") : wxT("LowID") ) % pClient->GetClientFullInfo() );
	pClient->SendAICHRequest(this, nPart);
	
}


void CPartFile::AICHRecoveryDataAvailable(uint16 nPart)
{
	if (GetPartCount() < nPart){
		wxFAIL;
		return;
	}

	FlushBuffer(true);
	uint32 length = GetPartSize(nPart);
	// if the part was already ok, it would now be complete
	if (IsComplete(nPart)){
		AddDebugLogLineM( false, logAICHRecovery,
			wxString::Format( wxT("Processing AICH Recovery data: The part (%u) is already complete, canceling"), nPart ) );
		return;
	}
	


	CAICHHashTree* pVerifiedHash = m_pAICHHashSet->m_pHashTree.FindHash(nPart*PARTSIZE, length);
	if (pVerifiedHash == NULL || !pVerifiedHash->GetHashValid()){
		AddDebugLogLineM( true, logAICHRecovery, wxT("Processing AICH Recovery data: Unable to get verified hash from hashset (should never happen)") );
		wxFAIL;
		return;
	}
	CAICHHashTree htOurHash(pVerifiedHash->GetNDataSize(), pVerifiedHash->GetIsLeftBranch(), pVerifiedHash->GetNBaseSize());
	try {
		CreateHashFromFile(m_hpartfile, PARTSIZE * nPart, length, NULL, &htOurHash);
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logAICHRecovery,
			CFormat(wxT("IO failure while hashing part-file '%s': %s"))
				% m_hpartfile.GetFilePath() % e.what());
		SetPartFileStatus(PS_ERROR);
		return;
	}
	
	if (!htOurHash.GetHashValid()){
		AddDebugLogLineM( false, logAICHRecovery, wxT("Processing AICH Recovery data: Failed to retrieve AICH Hashset of corrupt part") );
		wxFAIL;
		return;
	}

	// now compare the hash we just did, to the verified hash and readd all blocks which are ok
	uint32 nRecovered = 0;
	for (uint32 pos = 0; pos < length; pos += EMBLOCKSIZE){
		const uint32 nBlockSize = min<uint32>(EMBLOCKSIZE, length - pos);
		CAICHHashTree* pVerifiedBlock = pVerifiedHash->FindHash(pos, nBlockSize);
		CAICHHashTree* pOurBlock = htOurHash.FindHash(pos, nBlockSize);
		if ( pVerifiedBlock == NULL || pOurBlock == NULL || !pVerifiedBlock->GetHashValid() || !pOurBlock->GetHashValid()){
			wxFAIL;
			continue;
		}
		if (pOurBlock->GetHash() == pVerifiedBlock->GetHash()){
			FillGap(PARTSIZE*nPart+pos, PARTSIZE*nPart + pos + (nBlockSize-1));
			RemoveBlockFromList(PARTSIZE*nPart, PARTSIZE*nPart + (nBlockSize-1));
			nRecovered += nBlockSize;
			// tell the blackbox about the verified data
			m_CorruptionBlackBox->VerifiedData(true, nPart, pos, pos + nBlockSize - 1);
		} else {
			// inform our "blackbox" about the corrupted block which may ban clients who sent it
			m_CorruptionBlackBox->VerifiedData(false, nPart, pos, pos + nBlockSize - 1);
		}
	}
	m_CorruptionBlackBox->EvaluateData();

	// ok now some sanity checks
	if (IsComplete(nPart)){
		// this is a bad, but it could probably happen under some rare circumstances
		// make sure that MD4 agrres to this fact too
		if (!HashSinglePart(nPart)){
			AddDebugLogLineM( false, logAICHRecovery, 
				wxString::Format(wxT("Processing AICH Recovery data: The part (%u) got completed while recovering - but MD4 says it corrupt! Setting hashset to error state, deleting part"), nPart));
			// now we are fu... unhappy
			m_pAICHHashSet->SetStatus(AICH_ERROR);
			AddGap(nPart);
			wxFAIL;
			return;
		}
		else{
			AddDebugLogLineM( false, logAICHRecovery, wxString::Format( 
				wxT("Processing AICH Recovery data: The part (%u) got completed while recovering and MD4 agrees"), nPart) );
			if (status == PS_EMPTY && theApp->IsRunning()){
				if (GetHashCount() == GetED2KPartHashCount() && !m_hashsetneeded){
					// Successfully recovered part, make it available for sharing
					SetStatus(PS_READY);
					theApp->sharedfiles->SafeAddKFile(this);
				}
			}

			if (theApp->IsRunning()){
				// Is this file finished?
				if (m_gaplist.IsComplete()) {
					CompleteFile(false);
				}
			}
		}
	} // end sanity check
	// We did the best we could. If it's still incomplete, then no need to keep
	// bashing it with ICH. So remove it from the list of corrupted parts.
	EraseFirstValue(m_corrupted_list, nPart);
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

void CPartFile::GetRatingAndComments(FileRatingList & list) const
{
	list.clear();
	// This can be pre-processed, but is it worth the CPU?
	CPartFile::SourceSet::const_iterator it = m_SrcList.begin();
	for ( ; it != m_SrcList.end(); ++it ) {
		CUpDownClient *cur_src = *it;
		if (cur_src->GetFileComment().Length()>0 || cur_src->GetFileRating()>0) {
			// AddDebugLogLineM(false, logPartFile, wxString(wxT("found a comment for ")) << GetFileName());
			list.push_back(SFileRating(*cur_src));
		}
	}
}

#else   // CLIENT_GUI

CPartFile::CPartFile(CEC_PartFile_Tag *tag) : CKnownFile(tag->ID())
{
	Init();
	
	SetFileName(CPath(tag->FileName()));
	m_abyFileHash = tag->FileHash();
	SetFileSize(tag->SizeFull());
	m_gaplist.Init(GetFileSize(), true);	// Init empty
	m_partmetfilename = CPath(tag->PartMetName());
	m_fullname = m_partmetfilename;		// We have only the met number, so show it without path in the detail dialog.

	m_SrcpartFrequency.insert(m_SrcpartFrequency.end(), GetPartCount(), 0);

	// these are only in CLIENT_GUI and not covered by Init()
	m_source_count = 0;
	m_kbpsDown = 0;
	m_iDownPriorityEC = 0;
	m_a4af_source_count = 0;
}

/*
 * Remote gui specific code
 */
CPartFile::~CPartFile()
{
}

void CPartFile::GetRatingAndComments(FileRatingList & list) const
{ 
	list = m_FileRatingList; 
}

void CPartFile::SetCategory(uint8 cat)
{
	m_category = cat; 
}

#endif // !CLIENT_GUI


void CPartFile::UpdateDisplayedInfo(bool force)
{
	uint32 curTick = ::GetTickCount();

	 // Wait 1.5s between each redraw
	 if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE ) {
		 Notify_DownloadCtrlUpdateItem(this);
		m_lastRefreshedDLDisplay = curTick;
	}
	
}


void CPartFile::Init()
{
	m_showSources = false;
	m_lastsearchtime = 0;
	lastpurgetime = ::GetTickCount();
	m_paused = false;
	m_stopped = false;
	m_insufficient = false;

	status = PS_EMPTY;
	
	transferred = 0;
	m_iLastPausePurge = time(NULL);
	
	if(thePrefs::GetNewAutoDown()) {
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	} else {
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	
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
	m_nDlActiveTime = 0;
	m_tActivated = 0;
	m_is_A4AF_auto = false;
	m_localSrcReqQueued = false;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	
	m_validSources = 0;
	m_notCurrentSources = 0;

	// Kad
	m_LastSearchTimeKad = 0;
	m_TotalSearchesKad = 0;

	m_gapptrlist.Init(&m_gaplist);

#ifndef CLIENT_GUI
	m_CorruptionBlackBox = new CCorruptionBlackBox();
#endif
}

wxString CPartFile::getPartfileStatus() const
{

	wxString mybuffer; 

	if ((status == PS_HASHING) || (status == PS_WAITINGFORHASH)) {
		mybuffer=_("Hashing");
	} else if (status == PS_ALLOCATING) {
		mybuffer = _("Allocating");
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
				mybuffer = _("Insufficient disk space");
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


wxString CPartFile::GetFeedback() const
{
	wxString retval = CKnownFile::GetFeedback();
	if (GetStatus() != PS_COMPLETE) {
		retval += wxString(_("Downloaded")) + wxT(": ") + CastItoXBytes(GetCompletedSize()) + wxString::Format(wxT(" (%.2f%%)\n"), GetPercentCompleted())
			+ _("Sources") + CFormat(wxT(": %u\n")) % GetSourceCount();
	}
	return retval + _("Status") + wxT(": ") + getPartfileStatus() + wxT("\n");
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
	FileType type = GetFiletype(GetFileName());

	return (((type == ftVideo) || (type == ftAudio)) && IsComplete(0, 256*1024));
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


void CPartFile::SetActive(bool bActive)
{
	time_t tNow = time(NULL);
	if (bActive) {
		if (theApp->IsConnected()) {
			if (m_tActivated == 0) {
				m_tActivated = tNow;
			}
		}
	} else {
		if (m_tActivated != 0) {
			m_nDlActiveTime += tNow - m_tActivated;
			m_tActivated = 0;
		}
	}
}


uint32 CPartFile::GetDlActiveTime() const
{
	uint32 nDlActiveTime = m_nDlActiveTime;
	if (m_tActivated != 0) {
		nDlActiveTime += time(NULL) - m_tActivated;
	}
	return nDlActiveTime;
}


uint16 CPartFile::GetPartMetNumber() const
{
	long nr;
	return m_partmetfilename.RemoveAllExt().GetRaw().ToLong(&nr) ? nr : 0;
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

void CPartFile::SetFileName(const CPath& fileName)
{
	CKnownFile* pFile = theApp->sharedfiles->GetFileByID(GetFileHash());
	
	bool is_shared = (pFile && pFile == this);
	
	if (is_shared) {
		// The file is shared, we must clear the search keywords so we don't
		// publish the old name anymore.
		theApp->sharedfiles->RemoveKeywords(this);
	}
	
	CKnownFile::SetFileName(fileName);
	
	if (is_shared) {
		// And of course, we must advertise the new name if the file is shared.
		theApp->sharedfiles->AddKeywords(this);
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

#define DROP_FACTOR 2

CUpDownClient* CPartFile::GetSlowerDownloadingClient(uint32 speed, CUpDownClient* caller) {
//	printf("Start slower source calculation\n");
	for( SourceSet::iterator it = m_SrcList.begin(); it != m_SrcList.end(); ) {
		CUpDownClient* cur_src = *it++;
		if ((cur_src->GetDownloadState() == DS_DOWNLOADING) && (cur_src != caller)) {
			uint32 factored_bytes_per_second = static_cast<uint32>(
				(cur_src->GetKBpsDown() * 1024) * DROP_FACTOR);
			if ( factored_bytes_per_second< speed) {
//				printf("Selecting source %p to drop: %d < %d\n", cur_src, factored_bytes_per_second, speed);
//				printf("End slower source calculation\n");
				return cur_src;
			} else {
//				printf("Not selecting source %p to drop: %d > %d\n", cur_src, factored_bytes_per_second, speed);
			}
		}
	}	
//	printf("End slower source calculation\n");
	return NULL;
}

void CPartFile::AllocationFinished()
{
	// see if it can be opened
	if (!m_hpartfile.Open(m_PartPath, CFile::read_write)) {
		AddLogLineM(false, CFormat(_("ERROR: Failed to open partfile '%s'")) % GetFullName());
		SetPartFileStatus(PS_ERROR);
	}
	// then close the handle again
	m_hpartfile.Release(true);
}

#endif
// File_checked_for_headers
