//
// This file is part of the aMule Project.
//
// Parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
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


#include "KnownFile.h"		// Do_not_auto_remove


#include <include/protocol/kad/Constants.h>
#include <include/protocol/ed2k/Client2Client/TCP.h>
#include <include/protocol/Protocols.h>
#include <include/tags/FileTags.h>


#include <wx/config.h>


#include "MemFile.h"		// Needed for CMemFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "Packet.h"		// Needed for CPacket
#include "Preferences.h"	// Needed for CPreferences
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for SavePartFile
#include "ClientList.h" 	// Needed for clientlist (buddy support)
#include "Logger.h"
#include "ScopedPtr.h"		// Needed for CScopedArray and CScopedPtr
#include "GuiEvents.h"		// Needed for Notify_*

#include "CryptoPP_Inc.h"       // Needed for MD4

#include <common/Format.h>

CFileStatistic::CFileStatistic() : 
	requested(0), 
	transfered(0),
	accepted(0),
	alltimerequested(0),
	alltimetransferred(0),
	alltimeaccepted(0) 
{
}

#ifndef CLIENT_GUI

void CFileStatistic::AddRequest(){
	requested++;
	alltimerequested++;
	theApp->knownfiles->requested++;
	theApp->sharedfiles->UpdateItem(fileParent);
}
	
void CFileStatistic::AddAccepted(){
	accepted++;
	alltimeaccepted++;
	theApp->knownfiles->accepted++;
	theApp->sharedfiles->UpdateItem(fileParent);
}
	
void CFileStatistic::AddTransferred(uint64 bytes){
	transfered += bytes;
	alltimetransferred += bytes;
	theApp->knownfiles->transfered += bytes;
	theApp->sharedfiles->UpdateItem(fileParent);
}

#endif // CLIENT_GUI


/* Abstract File (base class)*/

CAbstractFile::CAbstractFile()
:
m_iRating(0),
m_hasComment(false),
m_iUserRating(0),
m_nFileSize(0)
{
}


CAbstractFile::CAbstractFile(const CAbstractFile& other)
:
m_abyFileHash(other.m_abyFileHash),
m_strComment(other.m_strComment),
m_iRating(other.m_iRating),
m_hasComment(other.m_hasComment),
m_iUserRating(other.m_iUserRating),
m_taglist(other.m_taglist),
m_nFileSize(other.m_nFileSize),
m_strFileName(other.m_strFileName)
{
/* // TODO: Currently it's not safe to duplicate the entries, but isn't needed either.
	CKadEntryPtrList::const_iterator it = other.m_kadNotes.begin();
	for (; it != other.m_kadNotes.end(); ++it) {
		m_kadNotes.push_back(new Kademlia::CEntry(**it));
	}
*/
}


void CAbstractFile::SetFileName(const wxString& strFileName)
{ 
	m_strFileName = strFileName;
} 

uint32 CAbstractFile::GetIntTagValue(uint8 tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if (((*it).GetNameID() == tagname) && (*it).IsInt()) {
			return (*it).GetInt();
		}
	}
	return 0;
}

bool CAbstractFile::GetIntTagValue(uint8 tagname, uint32& ruValue) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if (((*it).GetNameID() == tagname) && (*it).IsInt()){
			ruValue = (*it).GetInt();
			return true;
		}
	}
	return false;
}

uint32 CAbstractFile::GetIntTagValue(const wxString& tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).IsInt() && ((*it).GetName() == tagname)) {
			return (*it).GetInt();
		}
	}
	return 0;
}

const wxString& CAbstractFile::GetStrTagValue(uint8 tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).GetNameID() == tagname && (*it).IsStr()) {
			return (*it).GetStr();
		}
	}
	return EmptyString;
}

const wxString& CAbstractFile::GetStrTagValue(const wxString& tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).IsStr() && ((*it).GetName() == tagname)) {
			return (*it).GetStr();
		}
	}
	return EmptyString;
}

const CTag *CAbstractFile::GetTag(uint8 tagname, uint8 tagtype) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).GetNameID() == tagname && (*it).GetType() == tagtype) {
			return &(*it);
		}
	}
	return NULL;
}

const CTag *CAbstractFile::GetTag(const wxString& tagname, uint8 tagtype) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).GetType() == tagtype && (*it).GetName() == tagname) {
			return &(*it);
		}
	}
	return NULL;
}

const CTag *CAbstractFile::GetTag(uint8 tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).GetNameID() == tagname) {
			return &(*it);
		}
	}
	return NULL;
}

const CTag *CAbstractFile::GetTag(const wxString& tagname) const
{
	ArrayOfCTag::const_iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it){
		if ((*it).GetName() == tagname) {
			return &(*it);
		}
	}
	return NULL;
}

void CAbstractFile::AddTagUnique(const CTag &rTag)
{
	ArrayOfCTag::iterator it = m_taglist.begin();
	for (; it != m_taglist.end(); ++it) {
		if ( ( ((*it).GetNameID() != 0 &&
			(*it).GetNameID() == rTag.GetNameID()) ||
		       (!(*it).GetName().IsEmpty() &&
			!rTag.GetName().IsEmpty() &&
			(*it).GetName() == rTag.GetName()) ) &&
		     (*it).GetType() == rTag.GetType()){
			m_taglist.erase(it);
			m_taglist.insert(it, rTag);
			return;
		}
	}
	m_taglist.push_back(rTag);
}

#ifndef CLIENT_GUI
void CAbstractFile::AddNote(Kademlia::CEntry *pEntry)
{
	CKadEntryPtrList::iterator it = m_kadNotes.begin();
	for (; it != m_kadNotes.end(); ++it) {
		Kademlia::CEntry* entry = *it;
		if(entry->m_iIP == pEntry->m_iIP ||
		   !entry->m_iSourceID.CompareTo(pEntry->m_iSourceID)) {
			delete pEntry;
			return;
		}
	}
	m_kadNotes.push_front(pEntry);
}
#else
void CAbstractFile::AddNote(Kademlia::CEntry *)
{
}
#endif


/* Known File */

CKnownFile::CKnownFile()
{
	Init();
	
	m_bAutoUpPriority = thePrefs::GetNewAutoUp();
	m_iUpPriority = ( m_bAutoUpPriority ) ? PR_HIGH : PR_NORMAL;
}


void CKnownFile::Init() 
{
	date = 0;
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	m_bCommentLoaded = false;
	m_iPartCount = 0;
	m_iED2KPartCount = 0;
	m_iED2KPartHashCount = 0;
	m_PublishedED2K = false;
	kadFileSearchID = 0;
	m_lastPublishTimeKadSrc = 0;
	m_lastPublishTimeKadNotes = 0;
	m_lastBuddyIP = 0;

	statistic.fileParent = this;
	m_pAICHHashSet = new CAICHHashSet(this);
}


#ifdef CLIENT_GUI

CKnownFile::CKnownFile(CEC_SharedFile_Tag *tag)
{
	Init();
	
	SetFileName(tag->FileName());
	m_abyFileHash = tag->ID();
	SetFileSize(tag->SizeFull());
	m_iPartCount = (GetFileSize() + (PARTSIZE - 1)) / PARTSIZE;
	m_AvailPartFrequency.insert(m_AvailPartFrequency.end(), m_iPartCount, 0);
	m_iUpPriority = tag->Prio();
	if ( m_iUpPriority >= 10 ) {
		m_iUpPriority-= 10;
		m_bAutoUpPriority = true;
	} else {
		m_bAutoUpPriority = false;
	}
}

CKnownFile::~CKnownFile()
{
	delete m_pAICHHashSet;
}

#else // ! CLIENT_GUI

CKnownFile::~CKnownFile()
{
	SourceSet::iterator it = m_ClientUploadList.begin();
	for ( ; it != m_ClientUploadList.end(); ++it ) {
		(*it)->ClearUploadFileID();
	}
	
	delete m_pAICHHashSet;
}

void CKnownFile::AddUploadingClient(CUpDownClient* client)
{
	m_ClientUploadList.insert(client);
	
	UpdateAutoUpPriority();
}


void CKnownFile::RemoveUploadingClient(CUpDownClient* client)
{
	if (m_ClientUploadList.erase(client)) {
		UpdateAutoUpPriority();
	}
}


void CKnownFile::SetFilePath(const wxString& strFilePath)
{
	m_strFilePath = strFilePath;
}


void CKnownFile::SetFileSize(uint64 nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);
	m_pAICHHashSet->SetFileSize(nFileSize);
	
	// Examples of parthashs, hashsets and filehashs for different filesizes
	// according the ed2k protocol
	//----------------------------------------------------------------------
	//
	//File size: 3 bytes
	//File hash: 2D55E87D0E21F49B9AD25F98531F3724
	//Nr. hashs: 0
	//
	//
	//File size: 1*PARTSIZE
	//File hash: A72CA8DF7F07154E217C236C89C17619
	//Nr. hashs: 2
	//Hash[  0]: 4891ED2E5C9C49F442145A3A5F608299
	//Hash[  1]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 1*PARTSIZE + 1 byte
	//File hash: 2F620AE9D462CBB6A59FE8401D2B3D23
	//Nr. hashs: 2
	//Hash[  0]: 121795F0BEDE02DDC7C5426D0995F53F
	//Hash[  1]: C329E527945B8FE75B3C5E8826755747
	//
	//
	//File size: 2*PARTSIZE
	//File hash: A54C5E562D5E03CA7D77961EB9A745A4
	//Nr. hashs: 3
	//Hash[  0]: B3F5CE2A06BF403BFB9BFFF68BDDC4D9
	//Hash[  1]: 509AA30C9EA8FC136B1159DF2F35B8A9
	//Hash[  2]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE
	//File hash: 5E249B96F9A46A18FC2489B005BF2667
	//Nr. hashs: 4
	//Hash[  0]: 5319896A2ECAD43BF17E2E3575278E72
	//Hash[  1]: D86EF157D5E49C5ED502EDC15BB5F82B
	//Hash[  2]: 10F2D5B1FCB95C0840519C58D708480F
	//Hash[  3]: 31D6CFE0D16AE931B73C59D7E0C089C0	*special part hash*
	//
	//
	//File size: 3*PARTSIZE + 1 byte
	//File hash: 797ED552F34380CAFF8C958207E40355
	//Nr. hashs: 4
	//Hash[  0]: FC7FD02CCD6987DCF1421F4C0AF94FB8
	//Hash[  1]: 2FE466AF8A7C06DA3365317B75A5ACFE
	//Hash[  2]: 873D3BF52629F7C1527C6E8E473C1C30
	//Hash[  3]: BCE50BEE7877BB07BB6FDA56BFE142FB
	//

	// File size       Data parts      ED2K parts      ED2K part hashs
	// ---------------------------------------------------------------
	// 1..PARTSIZE-1   1               1               0(!)
	// PARTSIZE        1               2(!)            2(!)
	// PARTSIZE+1      2               2               2
	// PARTSIZE*2      2               3(!)            3(!)
	// PARTSIZE*2+1    3               3               3

	if (nFileSize == 0){
		//wxASSERT(0); // Kry - Why commented out by lemonfan? it can never be 0
		m_iPartCount = 0;
		m_iED2KPartCount = 0;
		m_iED2KPartHashCount = 0;
		return;
	}

	// nr. of data parts
	m_iPartCount = (nFileSize + (PARTSIZE - 1)) / PARTSIZE;

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = nFileSize / PARTSIZE + 1;

	// nr. of parts to be used with OP_HASHSETANSWER
	m_iED2KPartHashCount = nFileSize / PARTSIZE;
	if (m_iED2KPartHashCount != 0) {
		m_iED2KPartHashCount += 1;
	}
}


// needed for memfiles. its probably better to switch everything to CFile...
bool CKnownFile::LoadHashsetFromFile(const CFileDataIO* file, bool checkhash)
{
	CMD4Hash checkid = file->ReadHash();
	
	uint16 parts = file->ReadUInt16();
	for (uint16 i = 0; i < parts; ++i){
		CMD4Hash cur_hash = file->ReadHash();
		m_hashlist.push_back(cur_hash);
	}
	
	// SLUGFILLER: SafeHash - always check for valid m_hashlist
	if (!checkhash){
		m_abyFileHash = checkid;
		if (parts <= 1) {	// nothing to check
			return true;
		}
	} else {
		if ( m_abyFileHash != checkid ) {
			return false;	// wrong file?
		} else {
			if (parts != GetED2KPartHashCount()) {
				return false;
			}
		}
	}
	// SLUGFILLER: SafeHash
	
	// trust noone ;-)
	// lol, useless comment but made me lmao
	// wtf you guys are weird.

	if (!m_hashlist.empty()){
		byte buffer[m_hashlist.size() * 16];
		for (size_t i = 0;i != m_hashlist.size();++i) {
			md4cpy(buffer+(i*16),m_hashlist[i].GetHash());
		}
		CreateHashFromString(buffer,m_hashlist.size()*16,checkid.GetHash());
	}
	if ( m_abyFileHash == checkid ) {
		return true;
	} else {
		m_hashlist.clear();
		return false;
	}
}


bool CKnownFile::LoadTagsFromFile(const CFileDataIO* file)
{
	uint32 tagcount = file->ReadUInt32();
	for (uint32 j = 0; j != tagcount; ++j) {
		CTag newtag(*file, true);
		switch(newtag.GetNameID()){
			case FT_FILENAME:
				if (GetFileName().IsEmpty()) {
					SetFileName(newtag.GetStr());
				}
				break;
			
			case FT_FILESIZE:
				SetFileSize(newtag.GetInt());
				m_AvailPartFrequency.clear();
				m_AvailPartFrequency.insert(
					m_AvailPartFrequency.begin(),
					GetPartCount(), 0);
				break;
			
			case FT_ATTRANSFERED:
				statistic.alltimetransferred += newtag.GetInt();
				break;
			
			case FT_ATTRANSFEREDHI:
				statistic.alltimetransferred =
					(((uint64)newtag.GetInt()) << 32) +
					((uint64)statistic.alltimetransferred);
				break;
			
			case FT_ATREQUESTED:
				statistic.alltimerequested = newtag.GetInt();
				break;
			
			case FT_ATACCEPTED:
				statistic.alltimeaccepted = newtag.GetInt();
				break;
			
			case FT_ULPRIORITY:
				m_iUpPriority = newtag.GetInt();
				if( m_iUpPriority == PR_AUTO ){
					m_iUpPriority = PR_HIGH;
					m_bAutoUpPriority = true;
				} else {
					if (	m_iUpPriority != PR_VERYLOW &&
						m_iUpPriority != PR_LOW &&
						m_iUpPriority != PR_NORMAL &&
						m_iUpPriority != PR_HIGH &&
						m_iUpPriority != PR_VERYHIGH &&
						m_iUpPriority != PR_POWERSHARE) {
						m_iUpPriority = PR_NORMAL;
					}
					
					m_bAutoUpPriority = false;
				}
				break;
			
			case FT_PERMISSIONS:
				// Ignore it, it's not used anymore.
				break;
			
			case FT_AICH_HASH: {
				CAICHHash hash;
				bool hashSizeOk =
					hash.DecodeBase32(newtag.GetStr()) == CAICHHash::GetHashSize();
				wxASSERT(hashSizeOk);
				if (hashSizeOk) {
					m_pAICHHashSet->SetMasterHash(hash, AICH_HASHSETCOMPLETE);
				}
				break;
			}
			
			case FT_KADLASTPUBLISHSRC:
				SetLastPublishTimeKadSrc( newtag.GetInt(), 0 );
				
				if(GetLastPublishTimeKadSrc() > (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES) {
					//There may be a posibility of an older client that saved a random number here.. This will check for that..
					SetLastPublishTimeKadSrc(0, 0);
				}
				break;
			
			case FT_KADLASTPUBLISHNOTES:
				SetLastPublishTimeKadNotes( newtag.GetInt() );
				break;
			
			case FT_KADLASTPUBLISHKEY:
				// Just purge it
				wxASSERT( newtag.IsInt() );
				break;
				
			default:
				// Store them here and write them back on saving.
				m_taglist.push_back(newtag);
		}	
	}
	
	return true;
}


bool CKnownFile::LoadDateFromFile(const CFileDataIO* file)
{
	date = file->ReadUInt32();
	return true;
}


bool CKnownFile::LoadFromFile(const CFileDataIO* file)
{
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = LoadHashsetFromFile(file,false);
	bool ret3 = LoadTagsFromFile(file);
	UpdatePartsInfo();
	// Final hash-count verification, needs to be done after the tags are loaded.
	return ret1 && ret2 && ret3 && GetED2KPartHashCount()==GetHashCount();
	// SLUGFILLER: SafeHash
}


bool CKnownFile::WriteToFile(CFileDataIO* file)
{
	wxCHECK(!IsPartFile(), false);
	
	// date
	file->WriteUInt32(date); 
	// hashset
	file->WriteHash(m_abyFileHash);
	
	uint16 parts = m_hashlist.size();
	file->WriteUInt16(parts);

	for (int i = 0; i < parts; ++i)
		file->WriteHash(m_hashlist[i]);
	
	//tags
	const int iFixedTags = 8;
	uint32 tagcount = iFixedTags;
	if (	m_pAICHHashSet->HasValidMasterHash() &&
		(	m_pAICHHashSet->GetStatus() == AICH_HASHSETCOMPLETE ||
			m_pAICHHashSet->GetStatus() == AICH_VERIFIED)) {	
		tagcount++;
	}
	// Float meta tags are currently not written. All older eMule versions < 0.28a have 
	// a bug in the meta tag reading+writing code. To achive maximum backward 
	// compatibility for met files with older eMule versions we just don't write float 
	// tags. This is OK, because we (eMule) do not use float tags. The only float tags 
	// we may have to handle is the '# Sent' tag from the Hybrid, which is pretty 
	// useless but may be received from us via the servers.
	// 
	// The code for writing the float tags SHOULD BE ENABLED in SOME MONTHS (after most 
	// people are using the newer eMule versions which do not write broken float tags).	
	for (size_t j = 0; j < m_taglist.size(); ++j){
		if (m_taglist[j].IsInt() || m_taglist[j].IsStr()) {
			++tagcount;
		}
	}

	if (m_lastPublishTimeKadSrc) {
		++tagcount;
	}

	if (m_lastPublishTimeKadNotes){
		++tagcount;
	}
	
	// standard tags

	file->WriteUInt32(tagcount);
	
	CTagString nametag_unicode(FT_FILENAME, GetFileName());
	// We write it with BOM to kep eMule compatibility
	nametag_unicode.WriteTagToFile(file,utf8strOptBOM);	
	
	CTagString nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file);
	
	CTagIntSized sizetag(FT_FILESIZE, GetFileSize(), IsLargeFile() ? 64 : 32);
	sizetag.WriteTagToFile(file);
	
	// statistic
	uint32 tran;
	tran=statistic.alltimetransferred & 0xFFFFFFFF;
	CTagInt32 attag1(FT_ATTRANSFERED, tran);
	attag1.WriteTagToFile(file);
	
	tran=statistic.alltimetransferred>>32;
	CTagInt32 attag4(FT_ATTRANSFEREDHI, tran);
	attag4.WriteTagToFile(file);

	CTagInt32 attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
	attag2.WriteTagToFile(file);
	
	CTagInt32 attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
	attag3.WriteTagToFile(file);

	// priority N permission
	CTagInt32 priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
	priotag.WriteTagToFile(file);

	//AICH Filehash
	if (	m_pAICHHashSet->HasValidMasterHash() && 
		(	m_pAICHHashSet->GetStatus() == AICH_HASHSETCOMPLETE ||
			m_pAICHHashSet->GetStatus() == AICH_VERIFIED)) {
		CTagString aichtag(FT_AICH_HASH, m_pAICHHashSet->GetMasterHash().GetString());
		aichtag.WriteTagToFile(file);
	}

	// Kad sources
	if (m_lastPublishTimeKadSrc){
		CTagInt32 kadLastPubSrc(FT_KADLASTPUBLISHSRC, m_lastPublishTimeKadSrc);
		kadLastPubSrc.WriteTagToFile(file);
	}

	// Kad notes
	if (m_lastPublishTimeKadNotes){
		CTagInt32 kadLastPubNotes(FT_KADLASTPUBLISHNOTES, m_lastPublishTimeKadNotes);
		kadLastPubNotes.WriteTagToFile(file);
	}
	
	//other tags
	for (size_t j = 0; j < m_taglist.size(); ++j){
		if (m_taglist[j].IsInt() || m_taglist[j].IsStr()) {
			m_taglist[j].WriteTagToFile(file);
		}
	}
	return true;
}

#warning Kry - const
void CKnownFile::CreateHashFromInput(CFileDataIO* file, uint32 Length, byte* Output, byte* in_string, CAICHHashTree* pShaHashOut) const
{
	wxASSERT_MSG(Output or pShaHashOut, wxT("Nothing to do in CreateHashFromInput"));
	wxCHECK_RET(file or in_string, wxT("No input to hash from in CreateHashFromInput"));
	wxASSERT(Length <= PARTSIZE); // We never hash more than one PARTSIZE
	
	// When reading from files, this scoped array will take ownership of the temporary array.
	CScopedArray<unsigned char> tmpArray(NULL);
	
	if (file) {
		wxASSERT(!in_string);
		tmpArray.reset(new unsigned char[Length]);
		file->Read(tmpArray.get(), Length);
		in_string = tmpArray.get();
	}
	
	CMemFile data(in_string, Length);
		
	uint32 Required = Length;
	byte   X[64*128];
	
	uint32	posCurrentEMBlock = 0;
	uint32	nIACHPos = 0;
	CScopedPtr<CAICHHashAlgo> pHashAlg(m_pAICHHashSet->GetNewHashAlgo());

	// This is all AICH.
	while (Required >= 64) {
		uint32 len = Required / 64; 
		if (len > sizeof(X)/(64 * sizeof(X[0]))) {
			len = sizeof(X)/(64 * sizeof(X[0])); 
		}
		
		data.Read(&X, len * 64);

		// SHA hash needs 180KB blocks
		if (pShaHashOut) {
			if (nIACHPos + len*64 >= EMBLOCKSIZE) {
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg.get());
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete,(len*64) - nToComplete);
				nIACHPos = (len*64) - nToComplete;
			}
			else{
				pHashAlg->Add(X, len*64);
				nIACHPos += len*64;
			}
		}

		Required -= len*64;
	}
	// bytes to read
	Required = Length % 64;
	if (Required != 0){
		data.Read(&X,Required);

		if (pShaHashOut != NULL){
			if (nIACHPos + Required >= EMBLOCKSIZE){
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg.get());
				posCurrentEMBlock += EMBLOCKSIZE;
				pHashAlg->Reset();
				pHashAlg->Add(X+nToComplete, Required - nToComplete);
				nIACHPos = Required - nToComplete;
			}
			else{
				pHashAlg->Add(X, Required);
				nIACHPos += Required;
			}
		}
	}
	if (pShaHashOut != NULL){
		if(nIACHPos > 0){
			pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg.get());
			posCurrentEMBlock += nIACHPos;
		}
		wxASSERT( posCurrentEMBlock == Length );
		wxCHECK2( pShaHashOut->ReCalculateHash(pHashAlg.get(), false), );
	}

	if (Output != NULL){
		 CryptoPP::MD4 md4_hasher;
		 md4_hasher.CalculateDigest(Output, in_string, Length);
	}
}


const CMD4Hash& CKnownFile::GetPartHash(uint16 part) const {
	wxASSERT( part < m_hashlist.size() );
		
	return m_hashlist[part];
}

CPacket* CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions)
{
	// Kad reviewed
	
	if (m_ClientUploadList.empty()) {
		return NULL;	
	}
	
	if ((((CKnownFile*)forClient->GetRequestFile() != this)
		&& ((CKnownFile*)forClient->GetUploadFile() != this)) || forClient->GetUploadFileID() != GetFileHash()) {
		wxString file1 = _("Unknown");
		if (forClient->GetRequestFile() &&  !forClient->GetRequestFile()->GetFileName().IsEmpty()) {
			file1 = forClient->GetRequestFile()->GetFileName();
		} else if (forClient->GetUploadFile() &&  !forClient->GetUploadFile()->GetFileName().IsEmpty()) {
			file1 = forClient->GetUploadFile()->GetFileName();
		}
		wxString file2 = _("Unknown");
		if (!GetFileName().IsEmpty()) {
			file2 = GetFileName();
		}
		AddDebugLogLineM(false, logKnownFiles, wxT("File missmatch on source packet (K) Sending: ") + file1 + wxT("  From: ") + file2);
		return NULL;
	}

	const BitVector& rcvstatus = forClient->GetUpPartStatus();
	bool SupportsUploadChunksState = !rcvstatus.empty();
	//wxASSERT(rcvstatus.size() == GetPartCount()); // Obviously!
	if (rcvstatus.size() != GetPartCount()) {
		// Yuck. Same file but different part count? Seriously fucked up.
		AddDebugLogLineM(false, logKnownFiles, wxString::Format(wxT("Impossible situation: different partcounts for the same known file: %i (client) and %i (file)"),rcvstatus.size(),GetPartCount()));
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

	data.WriteHash(forClient->GetUploadFileID());
	data.WriteUInt16(nCount);
	uint32 cDbgNoSrc = 0;

	SourceSet::iterator it = m_ClientUploadList.begin();
	for ( ; it != m_ClientUploadList.end(); it++ ) {
		const CUpDownClient *cur_src = *it;
		
		if (	cur_src->HasLowID() ||
			cur_src == forClient ||
			!(	cur_src->GetUploadState() == US_UPLOADING ||
				cur_src->GetUploadState() == US_ONUPLOADQUEUE)) {
			continue;
		}
		
		bool bNeeded = false;
		
		if ( SupportsUploadChunksState ) {
			const BitVector& srcstatus = cur_src->GetUpPartStatus();
			if ( !srcstatus.empty() ) {
				//wxASSERT(srcstatus.size() == GetPartCount()); // Obviously!
				if (srcstatus.size() != GetPartCount()) {
					continue;
				}
				if ( cur_src->GetUpPartCount() == forClient->GetUpPartCount() ) {
					for (int x = 0; x < GetPartCount(); x++ ) {
						if ( srcstatus.at(x) && !rcvstatus.at(x) ) {
							// We know the receiving client needs
							// a chunk from this client.
							bNeeded = true;
							break;
						}
					}
				}
			} else {
				cDbgNoSrc++;
				// This client doesn't support upload chunk status.
				// So just send it and hope for the best.
				bNeeded = true;
			}
		} else {
			// remote client does not support upload chunk status,
			// search sources which have at least one complete part
			// we could even sort the list of sources by available
			// chunks to return as much sources as possible which
			// have the most available chunks. but this could be
			// a noticeable performance problem.
			const BitVector& srcstatus = cur_src->GetUpPartStatus();
			if ( !srcstatus.empty() ) {
				//wxASSERT(srcstatus.size() == GetPartCount());
				if (srcstatus.size() != GetPartCount()) {
					continue;
				}
				for (int x = 0; x < GetPartCount(); x++ ) {
					if ( srcstatus.at(x) ) {
						// this client has at least one chunk
						bNeeded = true;
						break;
					}
				}
			} else {
				// This client doesn't support upload chunk status.
				// So just send it and hope for the best.
				bNeeded = true;
			}
		}

		if ( bNeeded ) {
			nCount++;
			uint32 dwID;
			if(byUsedVersion >= 3) {
				dwID = cur_src->GetUserIDHybrid();
			} else {
				dwID = cur_src->GetIP();
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
	
	if ( result->GetPacketSize() > 354 ) {
		result->PackPacket();
	}
	
	return result;
}


// Updates priority of file if autopriority is activated
void CKnownFile::UpdateAutoUpPriority()
{
	if (IsAutoUpPriority()) {
		uint32 queued = GetQueuedCount();
		uint8 priority = PR_NORMAL;

		if (queued > 20) {
			priority = PR_LOW;
		} else if (queued > 1) {
			priority = PR_NORMAL;
		} else {
			priority = PR_HIGH;
		}

		if (GetUpPriority() != priority) {
			SetUpPriority(priority, false);
			Notify_SharedFilesUpdateItem(this);
		}
	}
}

void CKnownFile::SetFileComment(const wxString& strNewComment)
{ 
	if (m_strComment != strNewComment) {
		SetLastPublishTimeKadNotes(0);
		wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

		wxConfigBase* cfg = wxConfigBase::Get();
		cfg->Write( strCfgPath + wxT("Comment"), strNewComment);
     
		m_strComment = strNewComment;
  
		SourceSet::iterator it = m_ClientUploadList.begin();
		for ( ; it != m_ClientUploadList.end(); it++ ) {
			(*it)->SetCommentDirty();
		}
	}
}


// For File rate 
void CKnownFile::SetFileRating(int8 iNewRating)
{ 
	if (m_iRating != iNewRating) {
		SetLastPublishTimeKadNotes(0);	
		wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");
		wxConfigBase* cfg = wxConfigBase::Get();
		cfg->Write( strCfgPath + wxT("Rate"), iNewRating);
		m_iRating = iNewRating; 

		SourceSet::iterator it = m_ClientUploadList.begin();
		for ( ; it != m_ClientUploadList.end(); it++ ) {
			(*it)->SetCommentDirty();
		}
	}
} 


void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool m_bsave){
	m_iUpPriority = iNewUpPriority;
	if( IsPartFile() && m_bsave ) {
		((CPartFile*)this)->SavePartFile();
	}
}

void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	Notify_SharedFilesUpdateItem(this);
}

bool CKnownFile::PublishNotes()
{
	if(m_lastPublishTimeKadNotes > (uint32)time(NULL)) {
		return false;
	}
	
	if(!GetFileComment().IsEmpty()) {
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}
	
	if(GetFileRating() != 0) {
		m_lastPublishTimeKadNotes = (uint32)time(NULL)+KADEMLIAREPUBLISHTIMEN;
		return true;
	}

	return false;
}

bool CKnownFile::PublishSrc()
{
	uint32 lastBuddyIP = 0;

	if( theApp->IsFirewalled() ) {
		CUpDownClient* buddy = theApp->clientlist->GetBuddy();
		if( buddy ) {
			lastBuddyIP = theApp->clientlist->GetBuddy()->GetIP();
			if( lastBuddyIP != m_lastBuddyIP ) {
				SetLastPublishTimeKadSrc( (uint32)time(NULL)+KADEMLIAREPUBLISHTIMES, lastBuddyIP );
				return true;
			}
		} else {
			return false;
		}
	}

	if(m_lastPublishTimeKadSrc > (uint32)time(NULL)) {
		return false;
	}

	SetLastPublishTimeKadSrc((uint32)time(NULL)+KADEMLIAREPUBLISHTIMES,lastBuddyIP);
	return true;
	
}

void CKnownFile::UpdatePartsInfo()
{
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	// Ensure the frequency-list is ready
	if ( m_AvailPartFrequency.size() != GetPartCount() ) {
		m_AvailPartFrequency.clear();
		m_AvailPartFrequency.insert(m_AvailPartFrequency.begin(), GetPartCount(), 0);
	}

	if (flag) {
		ArrayOfUInts16 count;	
		count.reserve(m_ClientUploadList.size());	
		
		SourceSet::iterator it = m_ClientUploadList.begin();
		for ( ; it != m_ClientUploadList.end(); it++ ) {
			if ( !(*it)->GetUpPartStatus().empty() && (*it)->GetUpPartCount() == partcount ) {
				count.push_back((*it)->GetUpCompleteSourcesCount());
			}
		}
	
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if( partcount > 0) {
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		}
		for (uint16 i = 1; i < partcount; ++i) {
			if( m_nCompleteSourcesCount > m_AvailPartFrequency[i]) {
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
			}
		}
		count.push_back(m_nCompleteSourcesCount);

		int32 n = count.size();	
		if (n > 0) {
			std::sort(count.begin(), count.end(), std::less<uint16>());
			
			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8
			
			// For complete files, trust the people your uploading to more...
			
			// For low guess and normal guess count
			//	- If we see more sources then the guessed low and
			//	normal, use what we see.
			//	- If we see less sources then the guessed low,
			//	adjust network accounts for 100%, we account for
			//	0% with what we see and make sure we are still
			//	above the normal.
			// For high guess
			//	Adjust 100% network and 0% what we see.
			if (n < 20) {
				if ( count[i] < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				} else {
					m_nCompleteSourcesCountLo = count[i];
				}
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi = count[j];
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
				}
			} else {
			// Many sources..
			// For low guess
			//	Use what we see.
			// For normal guess
			//	Adjust network accounts for 100%, we account for
			//	0% with what we see and make sure we are still above the low.
			// For high guess
			//	Adjust network accounts for 100%, we account for 0%
			//	with what we see and make sure we are still above the normal.

				m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				m_nCompleteSourcesCount = count[j];
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo ) {
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				}
				m_nCompleteSourcesCountHi= count[k];
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount ) {
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
				}
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	
	Notify_SharedFilesUpdateItem(this);
}


void CKnownFile::UpdateUpPartsFrequency( CUpDownClient* client, bool increment )
{
	if ( m_AvailPartFrequency.size() != GetPartCount() ) {
		m_AvailPartFrequency.clear();
		m_AvailPartFrequency.insert(m_AvailPartFrequency.begin(), GetPartCount(), 0);
		if ( !increment ) {
			return;
		}
	}
	
	const BitVector& freq = client->GetUpPartStatus();
	unsigned int size = freq.size();
	if ( size != m_AvailPartFrequency.size() ) {
		return;
	}
	
	if ( increment ) {
		for ( unsigned int i = 0; i < size; ++i ) {
			if ( freq[i] ) {
				m_AvailPartFrequency[i]++;
			}
		}
	} else {
		for ( unsigned int i = 0; i < size; ++i ) {
			if ( freq[i] ) {
				m_AvailPartFrequency[i]--;
			}
		}
	}
}

void CKnownFile::ClearPriority() {
	if ( !m_bAutoUpPriority ) return;
	m_iUpPriority = ( m_bAutoUpPriority ) ? PR_HIGH : PR_NORMAL;
	UpdateAutoUpPriority();
}

void CKnownFile::SetFileName(const wxString& strmakeFilename)
{ 
	CAbstractFile::SetFileName(strmakeFilename);
	#ifndef CLIENT_GUI
		wordlist.clear();
		Kademlia::CSearchManager::GetWords(GetFileName(), &wordlist);
	#endif
}

#endif // CLIENT_GUI

//For File Comment // 
void CKnownFile::LoadComment()
{
	#ifndef CLIENT_GUI
	wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

	wxConfigBase* cfg = wxConfigBase::Get();
	
	m_strComment = cfg->Read( strCfgPath + wxT("Comment"), wxEmptyString);
	m_iRating = cfg->Read( strCfgPath + wxT("Rate"), 0l);
	m_bCommentLoaded = true;	
	
	#else
	m_strComment = wxT("Comments are not allowed on remote gui yet");
	m_bCommentLoaded = true;
	m_iRating =0;
	#endif
	
}
// File_checked_for_headers
