//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "ClientCredits.h"	// Interface declarations

#include <cmath>

#include "GetTickCount.h"	// Needed for GetTickCount
#include "Logger.h"			// Needed for Add(Debug)LogLine

CreditStruct::CreditStruct()
	: uploaded(0),
	  downloaded(0),
	  nLastSeen(0),
	  nReserved3(0),
	  nKeySize(0)
{
	memset(abySecureIdent, 0, MAXPUBKEYSIZE);
}


CClientCredits::CClientCredits(CreditStruct* in_credits)
{
	m_pCredits = in_credits;
	InitalizeIdent();
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
	m_dwWaitTimeIP = 0;
}


CClientCredits::CClientCredits(const CMD4Hash& key)
{
	m_pCredits = new CreditStruct();
	m_pCredits->key = key;

	InitalizeIdent();
	m_dwUnSecureWaitTime = ::GetTickCount();
	m_dwSecureWaitTime = ::GetTickCount();
	m_dwWaitTimeIP = 0;
}


CClientCredits::~CClientCredits()
{
	delete m_pCredits;
}


void CClientCredits::AddDownloaded(uint32 bytes, uint32 dwForIP, bool cryptoavail)
{
	switch (GetCurrentIdentState(dwForIP)) {
		case IS_IDFAILED:
		case IS_IDBADGUY:
		case IS_IDNEEDED:
			if (cryptoavail) {
				return;
			}
			break;
		case IS_NOTAVAILABLE:
		case IS_IDENTIFIED:
			break;
	}

	m_pCredits->downloaded += bytes;
}


void CClientCredits::AddUploaded(uint32 bytes, uint32 dwForIP, bool cryptoavail)
{
	switch (GetCurrentIdentState(dwForIP)) {
		case IS_IDFAILED:
		case IS_IDBADGUY:
		case IS_IDNEEDED:
			if (cryptoavail) {
				return;
			}
			break;
		case IS_NOTAVAILABLE:
		case IS_IDENTIFIED:
			break;
	}

	m_pCredits->uploaded += bytes;
}


uint64 CClientCredits::GetUploadedTotal() const
{
	return m_pCredits->uploaded;
}


uint64	CClientCredits::GetDownloadedTotal() const
{
	return m_pCredits->downloaded;
}


float CClientCredits::GetScoreRatio(uint32 dwForIP, bool cryptoavail)
{
	// check the client ident status
	switch (GetCurrentIdentState(dwForIP)) {
		case IS_IDFAILED:
		case IS_IDBADGUY:
		case IS_IDNEEDED:
			if (cryptoavail) {
				// bad guy - no credits for you
				return 1.0f;
			}
			break;
		case IS_NOTAVAILABLE:
		case IS_IDENTIFIED:
			break;
	}

	if (GetDownloadedTotal() < 1000000) {
		return 1.0f;
	}

	float result = 0.0f;
	if (!GetUploadedTotal()) {
		result = 10.0f;
	} else {
		result = (GetDownloadedTotal() * 2.0f) / GetUploadedTotal();
	}

	float result2 = sqrt((GetDownloadedTotal() / 1048576.0) + 2.0f);
	if (result > result2) {
		result = result2;
	}

	if (result < 1.0f) {
		return 1.0f;
	} else if (result > 10.0f) {
		return 10.0f;
	}

	return result;
}


void CClientCredits::SetLastSeen()
{
	m_pCredits->nLastSeen = time(NULL);
}


void CClientCredits::InitalizeIdent()
{
	if (m_pCredits->nKeySize == 0 ){
		memset(m_abyPublicKey,0,80); // for debugging
		m_nPublicKeyLen = 0;
		m_identState = IS_NOTAVAILABLE;
	}
	else{
		m_nPublicKeyLen = m_pCredits->nKeySize;
		memcpy(m_abyPublicKey, m_pCredits->abySecureIdent, m_nPublicKeyLen);
		m_identState = IS_IDNEEDED;
	}
	m_dwCryptRndChallengeFor = 0;
	m_dwCryptRndChallengeFrom = 0;
	m_dwIdentIP = 0;
}


void CClientCredits::Verified(uint32 dwForIP)
{
	m_dwIdentIP = dwForIP;
	// client was verified, copy the keyto store him if not done already
	if (m_pCredits->nKeySize == 0){
		m_pCredits->nKeySize = m_nPublicKeyLen;
		memcpy(m_pCredits->abySecureIdent, m_abyPublicKey, m_nPublicKeyLen);
		if (GetDownloadedTotal() > 0){
			// for security reason, we have to delete all prior credits here
			// in order to save this client, set 1 byte
			m_pCredits->downloaded = 1;
			m_pCredits->uploaded = 1;
			AddDebugLogLineN( logCredits, wxT("Credits deleted due to new SecureIdent") );
		}
	}
	m_identState = IS_IDENTIFIED;
}


bool CClientCredits::SetSecureIdent(const byte* pachIdent, uint8 nIdentLen)
{ // verified Public key cannot change, use only if there is not public key yet
	if (MAXPUBKEYSIZE < nIdentLen || m_pCredits->nKeySize != 0 ) {
		return false;
	}
	memcpy(m_abyPublicKey,pachIdent, nIdentLen);
	m_nPublicKeyLen = nIdentLen;
	m_identState = IS_IDNEEDED;
	return true;
}


EIdentState	CClientCredits::GetCurrentIdentState(uint32 dwForIP) const
{
	if (m_identState != IS_IDENTIFIED)
		return m_identState;
	else{
		if (dwForIP == m_dwIdentIP)
			return IS_IDENTIFIED;
		else
			return IS_IDBADGUY;
			// mod note: clients which just reconnected after an IP change and have to ident yet will also have this state for 1-2 seconds
			//		 so don't try to spam such clients with "bad guy" messages (besides: spam messages are always bad)
	}
}


uint32 CClientCredits::GetSecureWaitStartTime(uint32 dwForIP)
{
	if (m_dwUnSecureWaitTime == 0 || m_dwSecureWaitTime == 0)
		SetSecWaitStartTime(dwForIP);

	if (m_pCredits->nKeySize != 0){	// this client is a SecureHash Client
		if (GetCurrentIdentState(dwForIP) == IS_IDENTIFIED){ // good boy
			return m_dwSecureWaitTime;
		}
		else{	// not so good boy
			if (dwForIP == m_dwWaitTimeIP){
				return m_dwUnSecureWaitTime;
			}
			else{	// bad boy
				// this can also happen if the client has not identified himself yet, but will do later - so maybe he is not a bad boy :) .

				m_dwUnSecureWaitTime = ::GetTickCount();
				m_dwWaitTimeIP = dwForIP;
				return m_dwUnSecureWaitTime;
			}
		}
	}
	else{	// not a SecureHash Client - handle it like before for now (no security checks)
		return m_dwUnSecureWaitTime;
	}
}


void CClientCredits::SetSecWaitStartTime(uint32 dwForIP)
{
	m_dwUnSecureWaitTime = ::GetTickCount()-1;
	m_dwSecureWaitTime = ::GetTickCount()-1;
	m_dwWaitTimeIP = dwForIP;
}


void CClientCredits::ClearWaitStartTime()
{
	m_dwUnSecureWaitTime = 0;
	m_dwSecureWaitTime = 0;
}

// File_checked_for_headers
