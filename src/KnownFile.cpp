//
// This file is part of the aMule Project
//
// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include <wx/filefn.h>

#include <wx/file.h>
#include <wx/ffile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/confbase.h>
#include <wx/config.h>

#include "KnownFile.h"		// Interface declarations.
#include "otherfunctions.h"	// Needed for nstrdup
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SafeFile.h"		// Needed for CSafeMemFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "packets.h"		// Needed for CTag
#include "Preferences.h"	// Needed for CPreferences
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "amule.h"			// Needed for theApp
#include "PartFile.h"		// Needed for SavePartFile
#include "endianfix.h"

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(ArrayOfCMD4Hash);
WX_DEFINE_OBJARRAY(ArrayOfCTag);

CFileStatistic::CFileStatistic() : 
	requested(0), 
	transfered(0),
	accepted(0),
	alltimerequested(0),
	alltimetransferred(0),
	alltimeaccepted(0) 
{
}


void CFileStatistic::AddRequest(){
	requested++;
	alltimerequested++;
	theApp.knownfiles->requested++;
	theApp.sharedfiles->UpdateItem(fileParent);
}
	
void CFileStatistic::AddAccepted(){
	accepted++;
	alltimeaccepted++;
	theApp.knownfiles->accepted++;
	theApp.sharedfiles->UpdateItem(fileParent);
}
	
void CFileStatistic::AddTransferred(uint64 bytes){
	transfered += bytes;
	alltimetransferred += bytes;
	theApp.knownfiles->transfered += bytes;
	theApp.sharedfiles->UpdateItem(fileParent);
}


CAbstractFile::CAbstractFile() :
	m_nFileSize(0),
	m_iRate(0)
{
}


void CAbstractFile::SetFileName(const wxString& strFileName)
{ 
	m_strFileName = strFileName;
} 

CKnownFile::CKnownFile() :
	date(0),
	m_nCompleteSourcesTime(time(NULL)),
	m_nCompleteSourcesCount(0),
	m_nCompleteSourcesCountLo(0),
	m_nCompleteSourcesCountHi(0),
	m_bCommentLoaded(false),
	m_iPartCount(0),
	m_iED2KPartCount(0),
	m_iED2KPartHashCount(0),
	m_iPermissions(PERM_ALL),
	m_iQueuedCount(0),
	m_PublishedED2K(false)
{
	statistic.fileParent = this;
	
	m_bAutoUpPriority = theApp.glob_prefs->GetNewAutoUp();
	m_iUpPriority = ( m_bAutoUpPriority ) ? PR_HIGH : PR_NORMAL;
	m_pAICHHashSet = new CAICHHashSet(this);
}

CKnownFile::~CKnownFile(){
	
	hashlist.Clear();
			
	for (size_t i = 0; i != taglist.GetCount(); i++) {
		delete taglist[i];
	}		
	
	m_AvailPartFrequency.Clear();
	
	delete m_pAICHHashSet;
}

void CKnownFile::AddUploadingClient(CUpDownClient* client){
	if ( m_ClientUploadList.find(client) != m_ClientUploadList.end() ) {
		m_ClientUploadList.insert(client);
	}
}




void CKnownFile::RemoveUploadingClient(CUpDownClient* client){
	if ( m_ClientUploadList.find(client) != m_ClientUploadList.end() ) {
		m_ClientUploadList.erase(client);
	}
}

void CKnownFile::SetFilePath(const wxString& strFilePath)
{
	m_strFilePath = strFilePath;
}


void CKnownFile::GetMetaDataTags()
{

#if 0
// Kry - TODO - GetMetaTags
// Problem: types on id3lib mess with our types
	
	if (theApp.glob_prefs->GetExtractMetaData() == 0)
		return;

	// open file
	wxString namebuffer;
	namebuffer.Printf("%s/%s", directory, m_strFilename);
	
	if ((namebufer.Right(4).ToLower() == wxT(".mp3")) || (namebufer.Right(4).ToLower() == wxT(".mp2")) || (namebufer.Right(4).ToLower() == wxT(".mp1")) || (namebufer.Right(4).ToLower() == wxT(".mpa")))
	{

		try{
			ID3_Tag myTag;
			myTag.Link(unicode2char(namebuffer), ID3TT_ALL);

			const Mp3_Headerinfo* mp3info;
			mp3info = myTag.GetMp3HeaderInfo();
			if (mp3info)
			{
				// length
				if (mp3info->time){
					// the "length" ed2k meta tag was introduced by eDonkeyHybrid with the data type 'string' :/
					wxString strLength;
					SecToTimeLength(mp3info->time, strLength);
					CTag* pTag = new CTag(FT_MEDIA_LENGTH, strLength);
					AddTagUnique(pTag);
				}

				// here we could also create a "codec" ed2k meta tag.. though it would probable not be worth the
				// extra bytes which would have to be sent to the servers..

				// bitrate
				UINT uBitrate = mp3info->bitrate/1000;
				if (uBitrate){
					CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
					AddTagUnique(pTag);
				}
			}

			ID3_Tag::Iterator* iter = myTag.CreateIterator();
			const ID3_Frame* frame;
			while ((frame = iter->GetNext()) != NULL)
			{
				ID3_FrameID eFrameID = frame->GetID();
				switch (eFrameID)
				{
					case ID3FID_LEADARTIST:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CTag* pTag = new CTag(FT_MEDIA_ARTIST, pszText);
						AddTagUnique(pTag);
						delete[] pszText;
						break;
					}
					case ID3FID_ALBUM:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CTag* pTag = new CTag(FT_MEDIA_ALBUM, pszText);
						AddTagUnique(pTag);
						delete[] pszText;
						break;
					}
					case ID3FID_TITLE:{
						char* pszText = ID3_GetString(frame, ID3FN_TEXT);
						CTag* pTag = new CTag(FT_MEDIA_TITLE, pszText);
						AddTagUnique(pTag);
						delete[] pszText;
						break;
					}
				}
			}
			delete iter;
		}
		catch(...){
			wxASSERT(0);
			theApp->AddDebugLogLine(false, _T("Unhandled exception while extracting file meta (MP3) data from \"%s\""), szFullPath);
		}
	}
	else if (theApp.glob_prefs->GetExtractMetaData() > 1)
	{
		// starting the MediaDet object takes a noticeable amount of time.. avoid starting that object
		// for files which are not expected to contain any Audio/Video data.
		// note also: MediaDet does not work well for too short files (e.g. 16K)
		EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
		if ((eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_VIDEO) && GetFileSize() >= 32768)
		{
			TCHAR szFullPath[MAX_PATH];
			_tmakepath(szFullPath, NULL, GetPath(), GetFileName(), NULL);
			try{
				CComPtr<IMediaDet> pMediaDet;
				HRESULT hr = pMediaDet.CoCreateInstance(__uuidof(MediaDet));
				if (SUCCEEDED(hr))
				{
					USES_CONVERSION;
					if (SUCCEEDED(hr = pMediaDet->put_Filename(CComBSTR(T2W(szFullPath)))))
					{
						// Get the first audio/video streams
						long lAudioStream = -1;
						long lVideoStream = -1;
						double fVideoStreamLengthSec = 0.0;
						DWORD dwVideoBitRate = 0;
						DWORD dwVideoCodec = 0;
						double fAudioStreamLengthSec = 0.0;
						DWORD dwAudioBitRate = 0;
						DWORD dwAudioCodec = 0;
						long lStreams;
						if (SUCCEEDED(hr = pMediaDet->get_OutputStreams(&lStreams)))
						{
							for (long i = 0; i < lStreams; i++)
							{
								if (SUCCEEDED(hr = pMediaDet->put_CurrentStream(i)))
								{
									GUID major_type;
									if (SUCCEEDED(hr = pMediaDet->get_StreamType(&major_type)))
									{
										if (major_type == MEDIATYPE_Video)
										{
											if (lVideoStream == -1){
												lVideoStream = i;
												pMediaDet->get_StreamLength(&fVideoStreamLengthSec);

												AM_MEDIA_TYPE mt = {0};
												if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt))){
													if (mt.formattype == FORMAT_VideoInfo){
														VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)mt.pbFormat;
														dwVideoBitRate = pVIH->dwBitRate / 1000;
														// for AVI files this gives that used codec
														// for MPEG(1) files this just gives "Y41P"
														dwVideoCodec = pVIH->bmiHeader.biCompression;
													}
												}

												if (mt.pUnk != NULL)
													mt.pUnk->Release();
												if (mt.pbFormat != NULL)
													CoTaskMemFree(mt.pbFormat);
											}
										}
										else if (major_type == MEDIATYPE_Audio)
										{
											if (lAudioStream == -1){
												lAudioStream = i;
												pMediaDet->get_StreamLength(&fAudioStreamLengthSec);

												AM_MEDIA_TYPE mt = {0};
												if (SUCCEEDED(hr = pMediaDet->get_StreamMediaType(&mt))){
													if (mt.formattype == FORMAT_WaveFormatEx){
														WAVEFORMATEX* wfx = (WAVEFORMATEX*)mt.pbFormat;
														dwAudioBitRate = ((wfx->nAvgBytesPerSec * 8.0) + 500.0) / 1000.0;
													}
												}

												if (mt.pUnk != NULL)
													mt.pUnk->Release();
												if (mt.pbFormat != NULL)
													CoTaskMemFree(mt.pbFormat);
											}
										}
										else{
											TRACE("%s - Unknown stream type\n", GetFileName());
										}

										if (lVideoStream != -1 && lAudioStream != -1)
											break;
									}
								}
							}
						}

						uint32 uLengthSec = 0.0;
						wxString strCodec;
						uint32 uBitrate = 0;
						if (fVideoStreamLengthSec > 0.0){
							uLengthSec = fVideoStreamLengthSec;
							if (dwVideoCodec == BI_RGB)
								strCodec = "rgb";
							else if (dwVideoCodec == BI_RLE8)
								strCodec = "rle8";
							else if (dwVideoCodec == BI_RLE4)
								strCodec = "rle4";
							else if (dwVideoCodec == BI_BITFIELDS)
								strCodec = "bitfields";
							else{
								memcpy(strCodec.GetBuffer(4), &dwVideoCodec, 4);
								strCodec.ReleaseBuffer(4);
								strCodec.MakeLower();
							}
							uBitrate = dwVideoBitRate;
						}
						else if (fAudioStreamLengthSec > 0.0){
							uLengthSec = fAudioStreamLengthSec;
							uBitrate = dwAudioBitRate;
						}

						if (uLengthSec){
							// the "length" ed2k meta tag was introduced by eDonkeyHybrid with the data type 'string' :/
							wxString strLength;
							SecToTimeLength(uLengthSec, strLength);
							CTag* pTag = new CTag(FT_MEDIA_LENGTH, strLength);
							AddTagUnique(pTag);
						}

						if (!strCodec.IsEmpty()){
							CTag* pTag = new CTag(FT_MEDIA_CODEC, strCodec);
							AddTagUnique(pTag);
						}

						if (uBitrate){
							CTag* pTag = new CTag(FT_MEDIA_BITRATE, (uint32)uBitrate);
							AddTagUnique(pTag);
						}
					}
				}
			}
			catch(...){
				AddDebugLogLine(false, _T("Unhandled exception while extracting meta data (MediaDet) from \"%s\""), szFullPath);
			}
		}
	}
	
#endif
}

bool CKnownFile::CreateAICHHashSetOnly()
{
	wxASSERT( !IsPartFile() );
	m_pAICHHashSet->FreeHashSet();
	
	CFile file(GetFilePath(),CFile::read);
	if (!file.IsOpened()){
// TODO
		//theApp.QueueLogLine(false, GetResString(IDS_ERR_FILEOPEN) + _T(" - %hs"), GetFilePath(), _T(""), strerror(errno));
		return false;
	}

	// create aichhashset
	uint32 togo = m_nFileSize;
	uint16 hashcount;
	for (hashcount = 0; togo >= PARTSIZE; ) {
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, PARTSIZE);
		wxASSERT( pBlockAICHHashTree != NULL );
		CreateHashFromFile(&file, PARTSIZE, NULL, pBlockAICHHashTree);
		// SLUGFILLER: SafeHash - quick fallback
		if (theApp.amuledlg==NULL || !theApp.IsRunning()){ // in case of shutdown while still hashing
			file.Close();
			return false;
		}

		togo -= PARTSIZE;
		hashcount++;
	}

	if (togo != 0){
		CAICHHashTree* pBlockAICHHashTree = m_pAICHHashSet->m_pHashTree.FindHash(hashcount*PARTSIZE, togo);
		wxASSERT( pBlockAICHHashTree != NULL );
		CreateHashFromFile(&file, togo, NULL, pBlockAICHHashTree);
	}
	
	m_pAICHHashSet->ReCalculateHash(false);
	if ( m_pAICHHashSet->VerifyHashTree(true) ){
		m_pAICHHashSet->SetStatus(AICH_HASHSETCOMPLETE);
		if (!m_pAICHHashSet->SaveHashSet()){
// TODO
			//theApp.QueueLogLine(true, GetResString(IDS_SAVEACFAILED));
		}
	}
	else{
		// now something went pretty wrong
// TODO
		//theApp.QueueDebugLogLine(true,_T("Failed to calculate AICH Hashset from file %s"), GetFileName());
	}
	
	file.Close();	
	return true;	
}

void CKnownFile::SetFileSize(uint32 nFileSize)
{
	CAbstractFile::SetFileSize(nFileSize);

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
	m_iPartCount = ((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE;

	// nr. of parts to be used with OP_FILESTATUS
	m_iED2KPartCount = nFileSize / PARTSIZE + 1;
	wxASSERT(m_iED2KPartCount <= 441);
	// nr. of parts to be used with OP_HASHSETANSWER
	m_iED2KPartHashCount = nFileSize / PARTSIZE;
	if (m_iED2KPartHashCount != 0)
		m_iED2KPartHashCount += 1;
}


// needed for memfiles. its probably better to switch everything to CFile...
bool CKnownFile::LoadHashsetFromFile(const CFile* file, bool checkhash){
	CMD4Hash checkid;
	file->Read(&checkid,16);
	
	uint16	parts;
	file->Read(&parts,2);
	ENDIAN_SWAP_I_16(parts);
	
	for (uint16 i = 0; i < parts; i++){
		CMD4Hash cur_hash;
		file->Read(cur_hash,16);
		hashlist.Add(cur_hash);
	}
	
	// SLUGFILLER: SafeHash - always check for valid hashlist
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

	if (!hashlist.IsEmpty()){
		uchar* buffer = new uchar[hashlist.GetCount()*16];
		for (size_t i = 0;i != hashlist.GetCount();i++) {
			md4cpy(buffer+(i*16),hashlist[i]);
		}
		CreateHashFromString(buffer,hashlist.GetCount()*16,checkid);
		delete[] buffer;
	}
	if ( m_abyFileHash == checkid ) {
		return true;
	} else {
		hashlist.Clear();
		/*
		for (int i = 0; i < hashlist.GetCount(); i++) {
			delete[] hashlist[i];
		}
		hashlist.RemoveAll();
		*/
		return false;
	}
}

bool CKnownFile::LoadTagsFromFile(const CFile* file)
{
	try {
		uint32 tagcount;
		file->Read(&tagcount,4);
		ENDIAN_SWAP_I_32(tagcount);
		for (uint32 j = 0; j != tagcount;j++){
			CTag* newtag = new CTag(*file);
			switch(newtag->tag.specialtag){
				case FT_FILENAME:{
					SetFileName(char2unicode(newtag->tag.stringvalue));
					delete newtag;
					break;
				}
				case FT_FILESIZE:{
					SetFileSize(newtag->tag.intvalue);
					m_AvailPartFrequency.Alloc(GetPartCount());
					for (uint32 i = 0; i < GetPartCount();i++) {
						m_AvailPartFrequency.Add(0);
					}
					delete newtag;
					break;
				}
				case FT_ATTRANSFERED:{
					statistic.alltimetransferred += newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_ATTRANSFEREDHI:{
					statistic.alltimetransferred = (((uint64)newtag->tag.intvalue) << 32) + ((uint64)statistic.alltimetransferred);
					delete newtag;
					break;
				}
				case FT_ATREQUESTED:{
					statistic.alltimerequested = newtag->tag.intvalue;
					delete newtag;
					break;
				}
 				case FT_ATACCEPTED:{
					statistic.alltimeaccepted = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_ULPRIORITY:{
					m_iUpPriority = newtag->tag.intvalue;
					if( m_iUpPriority == PR_AUTO ){
						m_iUpPriority = PR_HIGH;
						m_bAutoUpPriority = true;
					}
					else {
						if (m_iUpPriority != PR_VERYLOW && m_iUpPriority != PR_LOW && m_iUpPriority != PR_NORMAL && m_iUpPriority != PR_HIGH && m_iUpPriority != PR_VERYHIGH && m_iUpPriority != PR_POWERSHARE) {
							m_iUpPriority = PR_NORMAL;
						}					
						m_bAutoUpPriority = false;
					}
					delete newtag;
					break;
				}
				case FT_PERMISSIONS:{
					m_iPermissions = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				default:
					taglist.Add(newtag);
			}	
		}
		return true;
	} catch (...) {
		printf("Caught exception in CKnownFile::LoadTagsFromFile!\n");
		return false;
	}
}

bool CKnownFile::LoadDateFromFile(const CFile* file){
	uint32 i;
	int j = file->Read(&i,4);
	date = ENDIAN_SWAP_32(i);
	return (4 == j);
}

bool CKnownFile::LoadFromFile(const CFile* file){
	// SLUGFILLER: SafeHash - load first, verify later
	bool ret1 = LoadDateFromFile(file);
	bool ret2 = LoadHashsetFromFile(file,false);
	bool ret3 = LoadTagsFromFile(file);
	UpdatePartsInfo();
	return ret1 && ret2 && ret3 && GetED2KPartHashCount()==GetHashCount();// Final hash-count verification, needs to be done after the tags are loaded.
	// SLUGFILLER: SafeHash
}


bool CKnownFile::WriteToFile(CFile* file){
	// date
	uint32 endiandate = ENDIAN_SWAP_32(date);
	file->Write(&endiandate,4); 
	// hashset
	file->Write(m_abyFileHash,16);
	uint16 parts = hashlist.GetCount();

	ENDIAN_SWAP_I_16(parts);

	file->Write(&parts,2);
	parts = hashlist.GetCount();
	for (int i = 0; i < parts; i++)
		file->Write(hashlist[i],16);
	//tags
	const int iFixedTags = 8;
	uint32 tagcount = iFixedTags;
	// Float meta tags are currently not written. All older eMule versions < 0.28a have 
	// a bug in the meta tag reading+writing code. To achive maximum backward 
	// compatibility for met files with older eMule versions we just don't write float 
	// tags. This is OK, because we (eMule) do not use float tags. The only float tags 
	// we may have to handle is the '# Sent' tag from the Hybrid, which is pretty 
	// useless but may be received from us via the servers.
	// 
	// The code for writing the float tags SHOULD BE ENABLED in SOME MONTHS (after most 
	// people are using the newer eMule versions which do not write broken float tags).	
	for (size_t j = 0; j < taglist.GetCount(); j++){
		if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
			tagcount++;
	}
	// standard tags

	ENDIAN_SWAP_I_32(tagcount);

	file->Write(&tagcount, 4);
	
	CTag nametag(FT_FILENAME, GetFileName());
	nametag.WriteTagToFile(file);
	
	CTag sizetag(FT_FILESIZE, m_nFileSize);
	sizetag.WriteTagToFile(file);
	
	// statistic
	uint32 tran;
	tran=statistic.alltimetransferred;
	CTag attag1(FT_ATTRANSFERED, tran);
	attag1.WriteTagToFile(file);
	
	tran=statistic.alltimetransferred>>32;
	CTag attag4(FT_ATTRANSFEREDHI, tran);
	attag4.WriteTagToFile(file);

	CTag attag2(FT_ATREQUESTED, statistic.GetAllTimeRequests());
	attag2.WriteTagToFile(file);
	
	CTag attag3(FT_ATACCEPTED, statistic.GetAllTimeAccepts());
	attag3.WriteTagToFile(file);

	// priority N permission
	CTag priotag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : m_iUpPriority);
	priotag.WriteTagToFile(file);

	CTag permtag(FT_PERMISSIONS, m_iPermissions);
	permtag.WriteTagToFile(file);

	//other tags
	for (size_t j = 0; j < taglist.GetCount(); j++){
		if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
			taglist[j]->WriteTagToFile(file);
	}
	return true;
}

#ifndef _AFX
#ifndef VERIFY
#define VERIFY(x) (void(x))
#endif //VERIFY
#endif //_AFX

// This code is taken from aLinkCreator
// Copyright holders are those on md4.cpp file
// on aLinkCreator sources.

/// BIG ENDIAN byte reversing
#if wxBYTE_ORDER == wxBIG_ENDIAN
// Note: this code is harmless on little-endian machines.
void byteReverse(unsigned char *buf, unsigned longs) {
  uint32 t;
  do
    {
      t = (uint32) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
          ((unsigned) buf[1] << 8 | buf[0]);
      *(uint32 *) buf = t;
      buf += 4;
    }
  while (--longs);
}
#else
#define byteReverse(buf, len)	do { } while (0)
#endif

void CKnownFile::CreateHashFromInput(FILE* file,CFile* file2, uint32 Length, uchar* Output, uchar* in_string, CAICHHashTree* pShaHashOut) const
{
	wxASSERT( Output != NULL || pShaHashOut != NULL);
	// time critial
	bool PaddingStarted = false;
	uint32 Hash[4];
	Hash[0] = 0x67452301;
	Hash[1] = 0xEFCDAB89;
	Hash[2] = 0x98BADCFE;
	Hash[3] = 0x10325476;
	CFile* data = NULL;
	if (in_string)
		data = new CMemFile(in_string,Length);
	uint32 Required = Length;
	uchar   X[64*128];

	uint32	posCurrentEMBlock = 0;
	uint32	nIACHPos = 0;
	CAICHHashAlgo* pHashAlg = m_pAICHHashSet->GetNewHashAlgo();

	while (Required >= 64){
        uint32 len = Required / 64; 
        if (len > sizeof(X)/(64 * sizeof(X[0]))) 
             len = sizeof(X)/(64 * sizeof(X[0])); 
		if (in_string)
			data->Read(&X,len*64);
		else if (file)
            fread(&X,len*64,1,file); 
		else if (file2)
			file2->Read(&X,len*64);

		// SHA hash needs 180KB blocks
		if (pShaHashOut != NULL){
			if (nIACHPos + len*64 >= EMBLOCKSIZE){
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
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

		if (Output != NULL){
			for (uint32 i = 0; i < len; i++) 
			{ 
				byteReverse((uchar*)(X + i*64), 16);
				MD4Transform(Hash, (uint32*)(X + i*64)); 
			}
		}
		Required -= len*64;
	}
	// bytes to read
	Required = Length % 64;
	if (Required != 0){
		if (in_string)
			data->Read(&X,Required);
		else if (file)
			fread(&X,Required,1,file);
		else if (file2)
			file2->Read(&X,Required);

		if (pShaHashOut != NULL){
			if (nIACHPos + Required >= EMBLOCKSIZE){
				uint32 nToComplete = EMBLOCKSIZE - nIACHPos;
				pHashAlg->Add(X, nToComplete);
				wxASSERT( nIACHPos + nToComplete == EMBLOCKSIZE );
				pShaHashOut->SetBlockHash(EMBLOCKSIZE, posCurrentEMBlock, pHashAlg);
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
			pShaHashOut->SetBlockHash(nIACHPos, posCurrentEMBlock, pHashAlg);
			posCurrentEMBlock += nIACHPos;
		}
		wxASSERT( posCurrentEMBlock == Length );
		VERIFY( pShaHashOut->ReCalculateHash(pHashAlg, false) );
	}

	if (Output != NULL){
		// in byte scale 512 = 64, 448 = 56
		if (Required >= 56){
			X[Required] = 0x80;
			PaddingStarted = TRUE;
			memset(&X[Required + 1], 0, 63 - Required);
			byteReverse((uchar*)X, 16);
			MD4Transform(Hash, (uint32*)X);
			Required = 0;
		}
		if (!PaddingStarted)
			X[Required++] = 0x80;
		memset(&X[Required], 0, 64 - Required);
		
		// add size (convert to bits)
		// Kry - Afaik << and >> are endian safe ;)
		((uint32*)X)[14] = Length << 3;
		((uint32*)X)[15] = Length >> 29;			
		
		byteReverse((uchar*)X, 16); 
	
		MD4Transform(Hash, (uint32*)X);
		md4cpy(Output, Hash);
	}

	delete pHashAlg;
	if (data)
		delete data;
}

const CMD4Hash& CKnownFile::GetPartHash(uint16 part) const {
	wxASSERT( part < hashlist.GetCount() );
		
	return hashlist[part];
}

void CKnownFile::MD4Transform(uint32 Hash[4], uint32 x[16]) const
{
  uint32 a = Hash[0];
  uint32 b = Hash[1];
  uint32 c = Hash[2];
  uint32 d = Hash[3];

  /* Round 1 */
  MD4_FF(a, b, c, d, x[ 0], S11); // 01
  MD4_FF(d, a, b, c, x[ 1], S12); // 02
  MD4_FF(c, d, a, b, x[ 2], S13); // 03
  MD4_FF(b, c, d, a, x[ 3], S14); // 04
  MD4_FF(a, b, c, d, x[ 4], S11); // 05
  MD4_FF(d, a, b, c, x[ 5], S12); // 06
  MD4_FF(c, d, a, b, x[ 6], S13); // 07
  MD4_FF(b, c, d, a, x[ 7], S14); // 08
  MD4_FF(a, b, c, d, x[ 8], S11); // 09
  MD4_FF(d, a, b, c, x[ 9], S12); // 10
  MD4_FF(c, d, a, b, x[10], S13); // 11
  MD4_FF(b, c, d, a, x[11], S14); // 12
  MD4_FF(a, b, c, d, x[12], S11); // 13
  MD4_FF(d, a, b, c, x[13], S12); // 14
  MD4_FF(c, d, a, b, x[14], S13); // 15
  MD4_FF(b, c, d, a, x[15], S14); // 16

  /* Round 2 */
  MD4_GG(a, b, c, d, x[ 0], S21); // 17
  MD4_GG(d, a, b, c, x[ 4], S22); // 18
  MD4_GG(c, d, a, b, x[ 8], S23); // 19
  MD4_GG(b, c, d, a, x[12], S24); // 20
  MD4_GG(a, b, c, d, x[ 1], S21); // 21
  MD4_GG(d, a, b, c, x[ 5], S22); // 22
  MD4_GG(c, d, a, b, x[ 9], S23); // 23
  MD4_GG(b, c, d, a, x[13], S24); // 24
  MD4_GG(a, b, c, d, x[ 2], S21); // 25
  MD4_GG(d, a, b, c, x[ 6], S22); // 26
  MD4_GG(c, d, a, b, x[10], S23); // 27
  MD4_GG(b, c, d, a, x[14], S24); // 28
  MD4_GG(a, b, c, d, x[ 3], S21); // 29
  MD4_GG(d, a, b, c, x[ 7], S22); // 30
  MD4_GG(c, d, a, b, x[11], S23); // 31
  MD4_GG(b, c, d, a, x[15], S24); // 32

  /* Round 3 */
  MD4_HH(a, b, c, d, x[ 0], S31); // 33
  MD4_HH(d, a, b, c, x[ 8], S32); // 34
  MD4_HH(c, d, a, b, x[ 4], S33); // 35
  MD4_HH(b, c, d, a, x[12], S34); // 36
  MD4_HH(a, b, c, d, x[ 2], S31); // 37
  MD4_HH(d, a, b, c, x[10], S32); // 38
  MD4_HH(c, d, a, b, x[ 6], S33); // 39
  MD4_HH(b, c, d, a, x[14], S34); // 40
  MD4_HH(a, b, c, d, x[ 1], S31); // 41
  MD4_HH(d, a, b, c, x[ 9], S32); // 42
  MD4_HH(c, d, a, b, x[ 5], S33); // 43
  MD4_HH(b, c, d, a, x[13], S34); // 44
  MD4_HH(a, b, c, d, x[ 3], S31); // 45
  MD4_HH(d, a, b, c, x[11], S32); // 46
  MD4_HH(c, d, a, b, x[ 7], S33); // 47
  MD4_HH(b, c, d, a, x[15], S34); // 48

  Hash[0] += a;
  Hash[1] += b;
  Hash[2] += c;
  Hash[3] += d;
}

Packet*	CKnownFile::CreateSrcInfoPacket(const CUpDownClient* forClient){
	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	//theApp.uploadqueue->FindSourcesForFileById(&srclist, forClient->reqfileid); //should we use "filehash"?
	theApp.uploadqueue->FindSourcesForFileById(&srclist, forClient->GetUploadFileID()); //should we use "filehash"?

	if(srclist.IsEmpty())
		return 0;

	CSafeMemFile data;
	uint16 nCount = 0;

	//data.Write(forClient->reqfileid, 16);
	data.WriteHash16(forClient->GetUploadFileID());
	data.WriteUInt16(nCount);

	//uint32 lastRequest = forClient->GetLastSrcReqTime();
	//we are only taking 30 random sources since we can't be sure if they have parts we need
	//this is hard coded because its a temp solution until next(?) version
	srand(time(NULL));
	for(int i = 0; i < 30; i++) {
		int victim = ((rand() >> 7) % srclist.GetSize());
		POSITION pos = srclist.FindIndex(victim);
		CUpDownClient *cur_src = srclist.GetAt(pos);
		if(!cur_src->HasLowID() && cur_src != forClient) {
			nCount++;
			data.WriteUInt32(cur_src->GetUserID());
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion() > 1)
				data.WriteHash16(cur_src->GetUserHash());
		}

		srclist.RemoveAt(pos);
		if(srclist.GetSize() == 0)
			break;
	}
	if (!nCount)
		return 0;
	data.Seek(16);
	data.WriteUInt16(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->SetOpCode(OP_ANSWERSOURCES);
	if (nCount > 28)
		result->PackPacket();
	return result;
}

// Updates priority of file if autopriority is activated
//void CKnownFile::UpdateUploadAutoPriority(void)
void CKnownFile::UpdateAutoUpPriority(void)
{
		if (!this->IsAutoUpPriority())
			return;

		if ( GetQueuedCount() > 20 ) {
			if ( GetUpPriority() != PR_LOW ) {
				SetUpPriority(PR_LOW, false);
				Notify_SharedFilesUpdateItem(this);
			}
			return;
		}

		if ( GetQueuedCount() > 1 ) {
			if ( GetUpPriority() != PR_NORMAL ) {
				SetUpPriority(PR_NORMAL, false);
				Notify_SharedFilesUpdateItem(this);
		}
		return;
	}
	if ( GetUpPriority() != PR_HIGH ) {
		SetUpPriority(PR_HIGH, false);
		Notify_SharedFilesUpdateItem(this);
	}
}

//For File Comment // 
void CKnownFile::LoadComment()
{
	wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

	wxConfigBase* cfg = wxConfig::Get();
	
	m_strComment = cfg->Read( strCfgPath + wxT("Comment"), wxEmptyString);
	m_iRate = cfg->Read( strCfgPath + wxT("Rate"), 0l);
	m_bCommentLoaded = true;
}    

void CKnownFile::SetFileComment(const wxString& strNewComment)
{ 
	wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");

	wxConfigBase* cfg = wxConfig::Get();
	cfg->Write( strCfgPath + wxT("Comment"), strNewComment);
     
	m_strComment = strNewComment;
   
	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	theApp.uploadqueue->FindSourcesForFileById(&srclist, this->GetFileHash());

	for (POSITION pos = srclist.GetHeadPosition();pos != 0; ){
		srclist.GetNext(pos)->SetCommentDirty();
	}   
}


// For File rate 
void CKnownFile::SetFileRate(int8 iNewRate)
{ 
	wxString strCfgPath = wxT("/") + m_abyFileHash.Encode() + wxT("/");
	wxConfigBase* cfg = wxConfig::Get();
	cfg->Write( strCfgPath + wxT("Rate"), iNewRate);
	m_iRate = iNewRate; 

	CTypedPtrList<CPtrList, CUpDownClient*> srclist;
	theApp.uploadqueue->FindSourcesForFileById(&srclist, this->GetFileHash());
	for (POSITION pos = srclist.GetHeadPosition(); pos != 0; ) {
		srclist.GetNext(pos)->SetCommentDirty();
	}
} 


void CKnownFile::SetUpPriority(uint8 iNewUpPriority, bool m_bsave){
	m_iUpPriority = iNewUpPriority;
	if( this->IsPartFile() && m_bsave )
		((CPartFile*)this)->SavePartFile();
}

void CKnownFile::SetPublishedED2K(bool val){
	m_PublishedED2K = val;
	Notify_SharedFilesUpdateItem(this);
}


void CKnownFile::UpdatePartsInfo()
{
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	// Reset Part Count and allocate it with 0es
	
	m_AvailPartFrequency.Clear();
	m_AvailPartFrequency.Alloc(partcount);
	
	m_AvailPartFrequency.Insert(/*Item*/(uint16) 1, /*pos*/0, partcount);

	ArrayOfUInts16 count;	
	
	if (flag) {
		count.Alloc(m_ClientUploadList.size());	
	}

	uint k_test = 0;
	
	for (SourceSet::iterator it = m_ClientUploadList.begin(); it != m_ClientUploadList.end(); ) {
		k_test++;
		CUpDownClient* cur_src = *it++;
		//This could be a partfile that just completed.. Many of these clients will not have this information.
		if(cur_src->m_abyUpPartStatus && cur_src->GetUpPartCount() == partcount ) {
			for (uint16 i = 0; i < partcount; i++) {
				if (cur_src->IsUpPartAvailable(i)) {
					m_AvailPartFrequency[i] += 1;
				}
			}
			if ( flag ) {
				count.Add(cur_src->GetUpCompleteSourcesCount());
			}
		}
	}

	if (flag) {
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;

		if( partcount > 0) {
			m_nCompleteSourcesCount = m_AvailPartFrequency[0];
		}
		for (uint16 i = 1; i < partcount; i++) {
			if( m_nCompleteSourcesCount > m_AvailPartFrequency[i]) {
				m_nCompleteSourcesCount = m_AvailPartFrequency[i];
			}
		}
	
		count.Add(m_nCompleteSourcesCount);
		
		count.Shrink();
	
		int32 n = count.GetCount();	

		if (n > 0) {
			
			// Kry - Native wx functions instead
			count.Sort(Uint16CompareValues);
			
			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8

			//For complete files, trust the people your uploading to more...

			//For low guess and normal guess count
			//	If we see more sources then the guessed low and normal, use what we see.
			//	If we see less sources then the guessed low, adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.
			//For high guess
			//  Adjust 100% network and 0% what we see.
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
			//Many sources..
			//For low guess
			//	Use what we see.
			//For normal guess
			//	Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the low.
			//For high guess
			//  Adjust network accounts for 100%, we account for 0% with what we see and make sure we are still above the normal.

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
