//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 Angel Vidal (Kry) ( kry@amule.org )
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

#include "SHAHashSet.h"
#include "OPCodes.h"
#include "amule.h"
#include "MemFile.h"
#include "KnownFile.h"
#include "Preferences.h"
#include "SHA.h"
#include "updownclient.h"
#include "DownloadQueue.h"
#include "PartFile.h"
#include "Logger.h"
#include <common/Format.h>

#include <algorithm>

using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// for this version the limits are set very high, they might be lowered later
// to make a hash trustworthy, at least 10 unique Ips (255.255.128.0) must have send it
// and if we have received more than one hash  for the file, one hash has to be send by more than 95% of all unique IPs
#define MINUNIQUEIPS_TOTRUST		10	// how many unique IPs most have send us a hash to make it trustworthy
#define	MINPERCENTAGE_TOTRUST		92  // how many percentage of clients most have sent the same hash to make it trustworthy

CAICHRequestedDataList CAICHHashSet::m_liRequestedData;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHash
wxString CAICHHash::GetString() const{
	return EncodeBase32(m_abyBuffer, HASHSIZE);
}


void CAICHHash::Read(CFileDataIO* file)	{ 
	file->Read(m_abyBuffer,HASHSIZE);
}


void CAICHHash::Write(CFileDataIO* file) const{ 
	file->Write(m_abyBuffer,HASHSIZE);
}

unsigned int CAICHHash::DecodeBase32(const wxString &base32)
{
	return ::DecodeBase32(base32, HASHSIZE, m_abyBuffer);	
}	


/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashTree

CAICHHashTree::CAICHHashTree(uint32 nDataSize, bool bLeftBranch, uint32 nBaseSize){
	m_nDataSize = nDataSize;
	m_nBaseSize = nBaseSize;
	m_bIsLeftBranch = bLeftBranch;
	m_pLeftTree = NULL;
	m_pRightTree = NULL;
	m_bHashValid = false;
}

CAICHHashTree::~CAICHHashTree(){
	if (m_pLeftTree)
		delete m_pLeftTree;
	if (m_pRightTree)
		delete m_pRightTree;
}

// recursive
CAICHHashTree* CAICHHashTree::FindHash(uint32 nStartPos, uint32 nSize, uint8* nLevel){
	(*nLevel)++;
	if (*nLevel > 16){ // sanity
		wxASSERT( false );
		return false;
	}
	if (nStartPos + nSize > m_nDataSize){ // sanity
		wxASSERT ( false );
		return NULL;
	}
	if (nSize > m_nDataSize){ // sanity
		wxASSERT ( false );
		return NULL;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched hash
		return this;
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		wxASSERT( false );
		return NULL;
	}
	else{
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft){ // sanity
				wxASSERT ( false );
				return NULL;
			}
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				wxASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->FindHash(nStartPos, nSize, nLevel);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight){ // sanity
				wxASSERT ( false );
				return NULL;
			}
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				wxASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->FindHash(nStartPos, nSize, nLevel);
		}
	}
}

// recursive
// calculates missing hash fromt he existing ones
// overwrites existing hashs
// fails if no hash is found for any branch
bool CAICHHashTree::ReCalculateHash(CAICHHashAlgo* hashalg, bool bDontReplace){
	wxASSERT ( !( (m_pLeftTree != NULL) ^ (m_pRightTree != NULL)) ); 
	if (m_pLeftTree && m_pRightTree){
		if ( !m_pLeftTree->ReCalculateHash(hashalg, bDontReplace) || !m_pRightTree->ReCalculateHash(hashalg, bDontReplace) )
			return false;
		if (bDontReplace && m_bHashValid)
			return true;
		if (m_pRightTree->m_bHashValid && m_pLeftTree->m_bHashValid){
			hashalg->Reset();
			hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
			hashalg->Finish(m_Hash);
			m_bHashValid = true;
			return true;
		}
		else
			return m_bHashValid;
	}
	else
		return true;
}

bool CAICHHashTree::VerifyHashTree(CAICHHashAlgo* hashalg, bool bDeleteBadTrees){
	if (!m_bHashValid){
		wxASSERT ( false );
		if (bDeleteBadTrees){
			if (m_pLeftTree){
				delete m_pLeftTree;
				m_pLeftTree = NULL;
			}
			if (m_pRightTree){
				delete m_pRightTree;
				m_pRightTree = NULL;
			}
		}
		AddDebugLogLineM( false, logSHAHashSet, wxT("VerifyHashTree - No masterhash available"));
		return false;
	}
	
		// calculated missing hashs without overwriting anything
		if (m_pLeftTree && !m_pLeftTree->m_bHashValid)
			m_pLeftTree->ReCalculateHash(hashalg, true);
		if (m_pRightTree && !m_pRightTree->m_bHashValid)
			m_pRightTree->ReCalculateHash(hashalg, true);
		
		if ((m_pRightTree && m_pRightTree->m_bHashValid) ^ (m_pLeftTree && m_pLeftTree->m_bHashValid)){
			// one branch can never be verified
			if (bDeleteBadTrees){
				if (m_pLeftTree){
					delete m_pLeftTree;
					m_pLeftTree = NULL;
				}
				if (m_pRightTree){
					delete m_pRightTree;
					m_pRightTree = NULL;
				}
			}
			AddDebugLogLineM( false, logSHAHashSet, wxT("VerifyHashSet failed - Hashtree incomplete"));
			return false;
		}
	if ((m_pRightTree && m_pRightTree->m_bHashValid) && (m_pLeftTree && m_pLeftTree->m_bHashValid)){			
	    // check verify the hashs of both child nodes against my hash 
	
		CAICHHash CmpHash;
		hashalg->Reset();
		hashalg->Add(m_pLeftTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Add(m_pRightTree->m_Hash.GetRawHash(), HASHSIZE);
		hashalg->Finish(CmpHash);
		
		if (m_Hash != CmpHash){
			if (bDeleteBadTrees){
				if (m_pLeftTree){
					delete m_pLeftTree;
					m_pLeftTree = NULL;
				}
				if (m_pRightTree){
					delete m_pRightTree;
					m_pRightTree = NULL;
				}
			}
			return false;
		}
		return m_pLeftTree->VerifyHashTree(hashalg, bDeleteBadTrees) && m_pRightTree->VerifyHashTree(hashalg, bDeleteBadTrees);
	}
	else
		// last hash in branch - nothing below to verify
		return true;

}

void CAICHHashTree::SetBlockHash(uint32 nSize, uint32 nStartPos, CAICHHashAlgo* pHashAlg){
	wxASSERT ( nSize <= EMBLOCKSIZE );
	CAICHHashTree* pToInsert = FindHash(nStartPos, nSize);
	if (pToInsert == NULL){ // sanity
		wxASSERT ( false );
		AddDebugLogLineM( false, logSHAHashSet, wxT("Critical Error: Failed to Insert SHA-HashBlock, FindHash() failed!"));
		return;
	}
	
	//sanity
	if (pToInsert->m_nBaseSize != EMBLOCKSIZE || pToInsert->m_nDataSize != nSize){
		wxASSERT ( false );
		AddDebugLogLineM( false, logSHAHashSet, wxT("Critical Error: Logical error on values in SetBlockHashFromData"));
		return;
	}

	pHashAlg->Finish(pToInsert->m_Hash);
	pToInsert->m_bHashValid = true;
}

bool CAICHHashTree::CreatePartRecoveryData(uint64 nStartPos, uint32 nSize, CFileDataIO* fileDataOut, uint16 wHashIdent){
	if (nStartPos + nSize > m_nDataSize){ // sanity
		wxASSERT ( false );
		return false;
	}
	if (nSize > m_nDataSize){ // sanity
		wxASSERT ( false );
		return false;
	}

	if (nStartPos == 0 && nSize == m_nDataSize){
		// this is the searched part, now write all blocks of this part
		// hashident for this level will be adjsuted by WriteLowestLevelHash
		return WriteLowestLevelHashs(fileDataOut, wHashIdent);
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		wxASSERT( false );
		return false;
	}
	else{
		wHashIdent <<= 1;
		wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
		
		uint64 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint64 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint64 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL || m_pRightTree == NULL){
			wxASSERT( false );
			return false;
		}
		if (nStartPos < nLeft){
			if (nStartPos + nSize > nLeft || !m_pRightTree->m_bHashValid){ // sanity
				wxASSERT ( false );
				return false;
			}
			m_pRightTree->WriteHash(fileDataOut, wHashIdent);
			return m_pLeftTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent);
		}
		else{
			nStartPos -= nLeft;
			if (nStartPos + nSize > nRight || !m_pLeftTree->m_bHashValid){ // sanity
				wxASSERT ( false );
				return false;
			}
			m_pLeftTree->WriteHash(fileDataOut, wHashIdent);
			return m_pRightTree->CreatePartRecoveryData(nStartPos, nSize, fileDataOut, wHashIdent);

		}
	}
}

void CAICHHashTree::WriteHash(CFileDataIO* fileDataOut, uint16 wHashIdent) const{
	wxASSERT( m_bHashValid );
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	fileDataOut->WriteUInt16(wHashIdent);
	m_Hash.Write(fileDataOut);
}

// write lowest level hashs into file, ordered from left to right optional without identifier
bool CAICHHashTree::WriteLowestLevelHashs(CFileDataIO* fileDataOut, uint16 wHashIdent, bool bNoIdent) const{
	wHashIdent <<= 1;
	wHashIdent |= (m_bIsLeftBranch) ? 1: 0;
	if (m_pLeftTree == NULL && m_pRightTree == NULL){
		if (m_nDataSize <= m_nBaseSize && m_bHashValid ){
			if (!bNoIdent)
				fileDataOut->WriteUInt16(wHashIdent);
			m_Hash.Write(fileDataOut);
			return true;
		}
		else{
			wxASSERT( false );
			return false;
		}
	}
	else if (m_pLeftTree == NULL || m_pRightTree == NULL){
		wxASSERT( false );
		return false;
	}
	else{
		return m_pLeftTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent)
				&& m_pRightTree->WriteLowestLevelHashs(fileDataOut, wHashIdent, bNoIdent);
	}
}

// recover all low level hashs from given data. hashs are assumed to be ordered in left to right - no identifier used
bool CAICHHashTree::LoadLowestLevelHashs(CFileDataIO* fileInput){
	if (m_nDataSize <= m_nBaseSize){ // sanity
		// lowest level, read hash
		m_Hash.Read(fileInput);
		m_bHashValid = true; 
		return true;
	}
	else{
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if (m_pLeftTree == NULL)
			m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			wxASSERT( m_pLeftTree->m_nDataSize == nLeft );
		}
		if (m_pRightTree == NULL)
			m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
		else{
			wxASSERT( m_pRightTree->m_nDataSize == nRight ); 
		}
		return m_pLeftTree->LoadLowestLevelHashs(fileInput)
				&& m_pRightTree->LoadLowestLevelHashs(fileInput);
	}
}


// write the hash, specified by wHashIdent, with Data from fileInput.
bool CAICHHashTree::SetHash(CFileDataIO* fileInput, uint16 wHashIdent, sint8 nLevel, bool bAllowOverwrite){
	if (nLevel == (-1)){
		// first call, check how many level we need to go
		uint8 i;
		for (i = 0; i != 16 && (wHashIdent & 0x8000) == 0; ++i){
			wHashIdent <<= 1;
		}
		if (i > 15){
			AddDebugLogLineM( false, logSHAHashSet, wxT("CAICHHashTree::SetHash - found invalid HashIdent (0)"));
			return false;
		}
		else{
			nLevel = 15 - i;
		}
	}
	if (nLevel == 0){
		// this is the searched hash
		if (m_bHashValid && !bAllowOverwrite){
			// not allowed to overwrite this hash, however move the filepointer by reading a hash
			CAICHHash(file);
			return true;
		}
		m_Hash.Read(fileInput);
		m_bHashValid = true; 
		return true;
	}
	else if (m_nDataSize <= m_nBaseSize){ // sanity
		// this is already the last level, cant go deeper
		wxASSERT( false );
		return false;
	}
	else{
		// adjust ident to point the path to the next node
		wHashIdent <<= 1;
		nLevel--;
		uint32 nBlocks = m_nDataSize / m_nBaseSize + ((m_nDataSize % m_nBaseSize != 0 )? 1:0); 
		uint32 nLeft = ( ((m_bIsLeftBranch) ? nBlocks+1:nBlocks) / 2)* m_nBaseSize;
		uint32 nRight = m_nDataSize - nLeft;
		if ((wHashIdent & 0x8000) > 0){
			if (m_pLeftTree == NULL)
				m_pLeftTree = new CAICHHashTree(nLeft, true, (nLeft <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				wxASSERT( m_pLeftTree->m_nDataSize == nLeft );
			}
			return m_pLeftTree->SetHash(fileInput, wHashIdent, nLevel);
		}
		else{
			if (m_pRightTree == NULL)
				m_pRightTree = new CAICHHashTree(nRight, false, (nRight <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE);
			else{
				wxASSERT( m_pRightTree->m_nDataSize == nRight ); 
			}
			return m_pRightTree->SetHash(fileInput, wHashIdent, nLevel);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
///CAICHUntrustedHash
bool CAICHUntrustedHash::AddSigningIP(uint32 dwIP){
	dwIP &= 0x00F0FFFF; // we use only the 20 most significant bytes for unique IPs
	return m_adwIpsSigning.insert(dwIP).second;
}



/////////////////////////////////////////////////////////////////////////////////////////
///CAICHHashSet
CAICHHashSet::CAICHHashSet(CKnownFile* pOwner)
	: m_pHashTree(0, true, PARTSIZE)
{
	m_eStatus = AICH_EMPTY;
	m_pOwner = pOwner;
}

CAICHHashSet::~CAICHHashSet(void)
{
	FreeHashSet();
}

bool CAICHHashSet::CreatePartRecoveryData(uint64 nPartStartPos, CFileDataIO* fileDataOut, bool bDbgDontLoad){
	wxASSERT( m_pOwner );
	if (m_pOwner->IsPartFile() || m_eStatus != AICH_HASHSETCOMPLETE){
		wxASSERT( false );
		return false;
	}
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE){
		wxASSERT( false );
		return false;
	}
	if (!bDbgDontLoad){
		if (!LoadHashSet()){
			AddDebugLogLineM( false, logSHAHashSet, wxT("Created RecoveryData error: failed to load hashset. File:") + m_pOwner->GetFileName() );
			SetStatus(AICH_ERROR);
			return false;
		}
	}
	bool bResult;
	uint8 nLevel = 0;
	uint32 nPartSize = min<uint32>(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToWrite = (nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0);
	fileDataOut->WriteUInt16(nHashsToWrite);
	uint32 nCheckFilePos = fileDataOut->GetPosition();
	if (m_pHashTree.CreatePartRecoveryData(nPartStartPos, nPartSize, fileDataOut, 0)){
		if (nHashsToWrite*(HASHSIZE+2u) != fileDataOut->GetPosition() - nCheckFilePos){
			wxASSERT( false );
			AddDebugLogLineM( false, logSHAHashSet, wxT("Created RecoveryData has wrong length. File: ") + m_pOwner->GetFileName() );
			bResult = false;
			SetStatus(AICH_ERROR);
		}
		else
			bResult = true;
	} else {
		AddDebugLogLineM( false, logSHAHashSet, wxT("Failed to create RecoveryData for ") + m_pOwner->GetFileName() );
		bResult = false;
		SetStatus(AICH_ERROR);
	}
	if (!bDbgDontLoad){
		FreeHashSet();
	}
	return bResult;
}

bool CAICHHashSet::ReadRecoveryData(uint32 nPartStartPos, CMemFile* fileDataIn)
{
	if (/*eMule TODO !m_pOwner->IsPartFile() ||*/ !(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED) ){
		wxASSERT( false );
		return false;
	}
	// at this time we check the recoverydata for the correct ammounts of hashs only
	// all hash are then taken into the tree, depending on there hashidentifier (except the masterhash)

	uint8 nLevel = 0;
	uint32 nPartSize = min<uint32>(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	m_pHashTree.FindHash(nPartStartPos, nPartSize,&nLevel);
	uint16 nHashsToRead = (nLevel-1) + nPartSize/EMBLOCKSIZE + ((nPartSize % EMBLOCKSIZE != 0 )? 1:0);
	uint16 nHashsAvailable = fileDataIn->ReadUInt16();
	if (fileDataIn->GetLength()-fileDataIn->GetPosition() < nHashsToRead*(HASHSIZE+2u) || nHashsToRead != nHashsAvailable){
		// this check is redunant, CSafememfile would catch such an error too
		AddDebugLogLineM( false, logSHAHashSet, wxT("Failed to read RecoveryData for ") + m_pOwner->GetFileName() + wxT("%s - Received datasize/amounts of hashs was invalid"));
		return false;
	}
	for (uint32 i = 0; i != nHashsToRead; ++i){
		uint16 wHashIdent = fileDataIn->ReadUInt16();
		if (wHashIdent == 1 /*never allow masterhash to be overwritten*/
			|| !m_pHashTree.SetHash(fileDataIn, wHashIdent,(-1), false))
		{
			AddDebugLogLineM( false, logSHAHashSet, wxT("Failed to read RecoveryData for ") + m_pOwner->GetFileName() + wxT(" - Error when trying to read hash into tree"));
			VerifyHashTree(true); // remove invalid hashs which we have already written
			return false;
		}
	}
	if (VerifyHashTree(true)){
		// some final check if all hashs we wanted are there
		for (uint32 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
			CAICHHashTree* phtToCheck = m_pHashTree.FindHash(nPartStartPos+nPartPos, min<uint32>(EMBLOCKSIZE, nPartSize-nPartPos));
			if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
				AddDebugLogLineM( false, logSHAHashSet, wxT("Failed to read RecoveryData for ") + m_pOwner->GetFileName() + wxT(" - Error while verifying presence of all lowest level hashs"));
				return false;
			}
		}
		// all done
		return true;
	}
	else{
		AddDebugLogLineM( false, logSHAHashSet, wxT("Failed to read RecoveryData for ") + m_pOwner->GetFileName() + wxT(" - Verifying received hashtree failed"));
		return false;
	}
}

// this function is only allowed to be called right after successfully calculating the hashset (!)
// will delete the hashset, after saving to free the memory
bool CAICHHashSet::SaveHashSet(){
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		wxASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize()){
		wxASSERT( false );
		return false;
	}
	wxString fullpath = theApp.ConfigDir + KNOWN2_MET_FILENAME;
	CFile file;
	if (wxFileExists(fullpath)) {	
		if (!file.Open(fullpath, CFile::read_write)) {
			// Add logline about unable to open file
			return false;
		}
	} else {
		if (!file.Create(fullpath)) {		
			// Add logline about unable to create file			
			return false;			
		}		
	}
	
	try {
		// first we check if the hashset we want to write is already stored
		CAICHHash CurrentHash;
		uint32 nExistingSize = file.GetLength();
		while (file.GetPosition() < nExistingSize){
			CurrentHash.Read(&file);
			if (m_pHashTree.m_Hash == CurrentHash){
				// this hashset if already available, no need to save it again
				return true;
			}
			uint16 nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				throw fullpath;
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, wxFromCurrent);
		}
		// write hashset
		m_pHashTree.m_Hash.Write(&file);
		uint16 nHashCount = (PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE);
		if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
			nHashCount += (m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0);
		file.WriteUInt16(nHashCount);
		if (!m_pHashTree.WriteLowestLevelHashs(&file, 0, true)){
			// thats bad... really
			file.SetLength(nExistingSize);
			AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to save HashSet: WriteLowestLevelHashs() failed!"));
			return false;
		}
		if (file.GetLength() != nExistingSize + (nHashCount+1)*HASHSIZE + 2){
			// thats even worse
			file.SetLength(nExistingSize);
			AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to save HashSet: Calculated and real size of hashset differ!"));
			return false;
		}
		AddDebugLogLineM( false, logSHAHashSet, wxString::Format(wxT("Sucessfully saved eMuleAC Hashset, %u Hashs + 1 Masterhash written"), nHashCount));
	} catch (const wxString& error){
		AddDebugLogLineM(true, logSHAHashSet, wxT("Error: ") + error);
		return false;
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(true, logSHAHashSet, wxT("IO error while saving AICH HashSet: ") + e.what());
		return false;
	}
			
	FreeHashSet();
	return true;
}


bool CAICHHashSet::LoadHashSet()
{
	if (m_eStatus != AICH_HASHSETCOMPLETE){
		wxASSERT( false );
		return false;
	}
	if ( !m_pHashTree.m_bHashValid || m_pHashTree.m_nDataSize != m_pOwner->GetFileSize() || m_pHashTree.m_nDataSize == 0){
		wxASSERT( false );
		return false;
	}
	wxString fullpath = theApp.ConfigDir + KNOWN2_MET_FILENAME;
	CFile file(fullpath, CFile::read_write);
	if (!file.IsOpened()){
		if (wxFileExists(fullpath)) {
			wxString strError(wxT("Failed to load ") KNOWN2_MET_FILENAME wxT(" file"));
			AddDebugLogLineM( true, logSHAHashSet, strError);
		}
		return false;
	}

	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		CAICHHash CurrentHash;
		uint32 nExistingSize = file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			CurrentHash.Read(&file);
			if (m_pHashTree.m_Hash == CurrentHash){
				// found Hashset
				uint32 nExpectedCount =	(PARTSIZE/EMBLOCKSIZE + ((PARTSIZE % EMBLOCKSIZE != 0)? 1 : 0)) * (m_pHashTree.m_nDataSize/PARTSIZE);
				if (m_pHashTree.m_nDataSize % PARTSIZE != 0)
					nExpectedCount += (m_pHashTree.m_nDataSize % PARTSIZE)/EMBLOCKSIZE + (((m_pHashTree.m_nDataSize % PARTSIZE) % EMBLOCKSIZE != 0)? 1 : 0);
				nHashCount = file.ReadUInt16();
				if (nHashCount != nExpectedCount){
					AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to load HashSet: Available Hashs and expected hashcount differ!"));
					return false;
				}
				if (!m_pHashTree.LoadLowestLevelHashs(&file)){
					AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to load HashSet: LoadLowestLevelHashs failed!"));
					return false;
				}
				if (!ReCalculateHash(false)){
					AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to load HashSet: Calculating loaded hashs failed!"));
					return false;
				}
				if (CurrentHash != m_pHashTree.m_Hash){
					AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to load HashSet: Calculated Masterhash differs from given Masterhash - hashset corrupt!"));
					return false;
				}
				return true;
			}
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				throw fullpath;
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, wxFromCurrent);
		}
		AddDebugLogLineM( true, logSHAHashSet, wxT("Failed to load HashSet: HashSet not found!"));
	} catch (const wxString& error) {
		AddDebugLogLineM(true, logSHAHashSet, wxT("Error: ") + error);
		return false;
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(true, logSHAHashSet, wxT("IO error while loading AICH HashSet: ") + e.what());
		return false;
	}


	return false;
}

// delete the hashset except the masterhash (we dont keep aich hashsets in memory to save ressources)
void CAICHHashSet::FreeHashSet(){
	if (m_pHashTree.m_pLeftTree){
		delete m_pHashTree.m_pLeftTree;
		m_pHashTree.m_pLeftTree = NULL;
	}
	if (m_pHashTree.m_pRightTree){
		delete m_pHashTree.m_pRightTree;
		m_pHashTree.m_pRightTree = NULL;
	}
}

void CAICHHashSet::SetMasterHash(const CAICHHash& Hash, EAICHStatus eNewStatus){
	m_pHashTree.m_Hash = Hash;
	m_pHashTree.m_bHashValid = true;
	SetStatus(eNewStatus);
}

CAICHHashAlgo*	CAICHHashSet::GetNewHashAlgo(){
	return new CSHA();
}

bool CAICHHashSet::ReCalculateHash(bool bDontReplace){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.ReCalculateHash(hashalg, bDontReplace);
	delete hashalg;
	return bResult;
}

bool CAICHHashSet::VerifyHashTree(bool bDeleteBadTrees){
	CAICHHashAlgo* hashalg = GetNewHashAlgo();
	bool bResult = m_pHashTree.VerifyHashTree(hashalg, bDeleteBadTrees);
	delete hashalg;
	return bResult;
}

void CAICHHashSet::SetFileSize(uint64 nSize){
	m_pHashTree.m_nDataSize = nSize;
	m_pHashTree.m_nBaseSize = (nSize <= PARTSIZE) ? EMBLOCKSIZE : PARTSIZE;	
}

void CAICHHashSet::UntrustedHashReceived(const CAICHHash& Hash, uint32 dwFromIP){
	switch(GetStatus()){
		case AICH_EMPTY:
		case AICH_UNTRUSTED:
		case AICH_TRUSTED:
			break;
		default:
			return;
	}
	bool bFound = false;
	bool bAdded = false;
	for (uint32 i = 0; i < m_aUntrustedHashs.size(); ++i){
		if (m_aUntrustedHashs[i].m_Hash == Hash){
			bAdded = m_aUntrustedHashs[i].AddSigningIP(dwFromIP);
			bFound = true;
			break;
		}
	}
	if (!bFound){
		bAdded = true;
		CAICHUntrustedHash uhToAdd;
		uhToAdd.m_Hash = Hash;
		uhToAdd.AddSigningIP(dwFromIP);
		m_aUntrustedHashs.push_back(uhToAdd);
	}

	uint32 nSigningIPsTotal = 0;	// unique clients who send us a hash
	sint32 nMostTrustedPos = (-1);  // the hash which most clients send us
	uint32 nMostTrustedIPs = 0;
	for (uint32 i = 0; i < (uint32)m_aUntrustedHashs.size(); ++i){
		nSigningIPsTotal += m_aUntrustedHashs[i].m_adwIpsSigning.size();
		if ((uint32)m_aUntrustedHashs[i].m_adwIpsSigning.size() > nMostTrustedIPs){
			nMostTrustedIPs = m_aUntrustedHashs[i].m_adwIpsSigning.size();
			nMostTrustedPos = i;
		}
	}
	if (nMostTrustedPos == (-1) || nSigningIPsTotal == 0){
		wxASSERT( false );
		return;
	}
	// the check if we trust any hash
	if ( thePrefs::IsTrustingEveryHash() ||
		(nMostTrustedIPs >= MINUNIQUEIPS_TOTRUST && (100 * nMostTrustedIPs)/nSigningIPsTotal >= MINPERCENTAGE_TOTRUST)){
		//trusted
		AddDebugLogLineM(false, logSHAHashSet, 
			CFormat(wxT("IACH Hash recieved (%sadded), We have now %u hash(es) from %u unique IP(s). ")
			   		wxT("We trust the Hash %s from %u client(s) (%u%%). File: %s"))
				% (bAdded ? wxT("") : wxT("not "))
				% m_aUntrustedHashs.size()
				% nSigningIPsTotal
				% m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
				% nMostTrustedIPs
				% ((100 * nMostTrustedIPs) / nSigningIPsTotal)
				% m_pOwner->GetFileName());
			
		SetStatus(AICH_TRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_TRUSTED);
			FreeHashSet();
		}
	} else {
		// untrusted
		AddDebugLogLineM(false, logSHAHashSet,
			CFormat(wxT("IACH Hash recieved (%sadded), We have now %u hash(es) from %u unique IP(s). ")
					wxT("Best Hash %s from %u clients (%u%%) - but we dont trust it yet. File: %s"))
				% (bAdded ? wxT(""): wxT("not "))
				% m_aUntrustedHashs.size()
				% nSigningIPsTotal
				% m_aUntrustedHashs[nMostTrustedPos].m_Hash.GetString()
				% nMostTrustedIPs
				% ((100 * nMostTrustedIPs) / nSigningIPsTotal)
				% m_pOwner->GetFileName());
					
		SetStatus(AICH_UNTRUSTED);
		if (!HasValidMasterHash() || GetMasterHash() != m_aUntrustedHashs[nMostTrustedPos].m_Hash){
			SetMasterHash(m_aUntrustedHashs[nMostTrustedPos].m_Hash, AICH_UNTRUSTED);
			FreeHashSet();
		}
	}
}

#ifndef CLIENT_GUI

void CAICHHashSet::ClientAICHRequestFailed(CUpDownClient* pClient){
	pClient->SetReqFileAICHHash(NULL);
	CAICHRequestedData data = GetAICHReqDetails(pClient);
	RemoveClientAICHRequest(pClient);
	if (data.m_pClient != pClient)
		return;
	if( theApp.downloadqueue->IsPartFile(data.m_pPartFile)){
		AddDebugLogLineM( false, logSHAHashSet, wxT("IACH Request failed, Trying to ask another client (file ") + data.m_pPartFile->GetFileName() + wxString::Format(wxT(", Part: %u,  Client"),data.m_nPart) + pClient->GetClientFullInfo());
		data.m_pPartFile->RequestAICHRecovery(data.m_nPart);
	}
}

#endif

void CAICHHashSet::RemoveClientAICHRequest(const CUpDownClient* pClient) {
	
	for (CAICHRequestedDataList::iterator it = m_liRequestedData.begin();it != m_liRequestedData.end(); ++it) {
		if ((*(it)).m_pClient == pClient){
			m_liRequestedData.erase(it);
			return;
		}
	}
	wxASSERT( false );
}

bool CAICHHashSet::IsClientRequestPending(const CPartFile* pForFile, uint16 nPart){
	for (CAICHRequestedDataList::iterator it = m_liRequestedData.begin();it != m_liRequestedData.end(); ++it) {
		if ((*(it)).m_pPartFile == pForFile && (*(it)).m_nPart == nPart){
			return true;
		}
	}
	return false;
}

CAICHRequestedData CAICHHashSet::GetAICHReqDetails(const  CUpDownClient* pClient){
	for (CAICHRequestedDataList::iterator it = m_liRequestedData.begin();it != m_liRequestedData.end(); ++it) {
		if ((*(it)).m_pClient == pClient){
			return *(it);
		}
	}	
	wxASSERT( false );
	CAICHRequestedData empty;
	return empty;
}

bool CAICHHashSet::IsPartDataAvailable(uint64 nPartStartPos){
	if (!(m_eStatus == AICH_VERIFIED || m_eStatus == AICH_TRUSTED || m_eStatus == AICH_HASHSETCOMPLETE) ){
		wxASSERT( false );
		return false;
	}
	uint64 nPartSize = min<uint64>(PARTSIZE, m_pOwner->GetFileSize()-nPartStartPos);
	for (uint64 nPartPos = 0; nPartPos < nPartSize; nPartPos += EMBLOCKSIZE){
		CAICHHashTree* phtToCheck = m_pHashTree.FindHash(nPartStartPos+nPartPos, min<uint64>(EMBLOCKSIZE, nPartSize-nPartPos));
		if (phtToCheck == NULL || !phtToCheck->m_bHashValid){
			return false;
		}
	}
	return true;
}

// VC++ defines Assert as ASSERT. VC++ also defines VERIFY MACRO, which is the equivalent of ASSERT but also works in Released builds.

#define VERIFY(x) wxASSERT(x)

void CAICHHashSet::DbgTest(){

	//define TESTSIZE 4294567295
	uint8 maxLevel = 0;
	uint32 cHash = 1;
	uint8 curLevel = 0;
	maxLevel = 0;
/*	
	uint32 cParts = 0;	
	CAICHHashTree* pTest = new CAICHHashTree(TESTSIZE, true, 9728000);
	for (uint64 i = 0; i+9728000 < TESTSIZE; i += 9728000){
		CAICHHashTree* pTest2 = new CAICHHashTree(9728000, true, EMBLOCKSIZE);
		pTest->ReplaceHashTree(i, 9728000, &pTest2);
		cParts++;
	}
	CAICHHashTree* pTest2 = new CAICHHashTree(TESTSIZE-i, true, EMBLOCKSIZE);
	pTest->ReplaceHashTree(i, (TESTSIZE-i), &pTest2);
	cParts++;
*/
#define TESTSIZE m_pHashTree.m_nDataSize
	if (m_pHashTree.m_nDataSize <= EMBLOCKSIZE)
		return;
	CAICHHashSet TestHashSet(m_pOwner);
	TestHashSet.SetFileSize(m_pOwner->GetFileSize());
	TestHashSet.SetMasterHash(GetMasterHash(), AICH_VERIFIED);
	CMemFile file;
	uint64 i = 0;
	for (i = 0; i+9728000 < TESTSIZE; i += 9728000){
		VERIFY( CreatePartRecoveryData(i, &file) );
		
		/*uint32 nRandomCorruption = (rand() * rand()) % (file.GetLength()-4);
		file.Seek(nRandomCorruption, CFile::begin);
		file.Write(&nRandomCorruption, 4);*/

		file.Seek(0,wxFromStart);
		VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
		file.Seek(0,wxFromStart);
		TestHashSet.FreeHashSet();
		uint32 j = 0;
		for (j = 0; j+EMBLOCKSIZE < 9728000; j += EMBLOCKSIZE){
			VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
			//TRACE(wxT("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
			maxLevel = max(curLevel, maxLevel);
			curLevel = 0;
			cHash++;
		}
		VERIFY( m_pHashTree.FindHash(i+j, 9728000-j, &curLevel) );
		//TRACE(wxT("%u - %s\r\n"), cHash, m_pHashTree.FindHash(i+j, 9728000-j, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;

	}
	VERIFY( CreatePartRecoveryData(i, &file) );
	file.Seek(0,wxFromStart);
	VERIFY( TestHashSet.ReadRecoveryData(i, &file) );
	file.Seek(0,wxFromStart);
	TestHashSet.FreeHashSet();
	uint64 j = 0;
	for (j = 0; j+EMBLOCKSIZE < TESTSIZE-i; j += EMBLOCKSIZE){
		VERIFY( m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel) );
		//TRACE(wxT("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, EMBLOCKSIZE, &curLevel)->m_Hash.GetString());
		maxLevel = max(curLevel, maxLevel);
		curLevel = 0;
		cHash++;
	}
	//VERIFY( m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel) );
//	TRACE(wxT("%u - %s\r\n"), cHash,m_pHashTree.FindHash(i+j, (TESTSIZE-i)-j, &curLevel)->m_Hash.GetString());
	maxLevel = max(curLevel, maxLevel);
	
}
