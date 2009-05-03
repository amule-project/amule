//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2009 Stu Redman ( sturedman@amule.org )
// Copyright (C) 2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#include "CorruptionBlackBox.h"
#include "KnownFile.h"
#include "updownclient.h"
#include "Logger.h"
#include "amule.h"
#include "ClientList.h"
#include <common/Format.h>	// Needed for CFormat

#define	 CBB_BANTHRESHOLD	32 //% max corrupted data	

#ifdef __DEBUG__
#define DEBUG_ONLY(statement) (statement)
#else
#define DEBUG_ONLY(statement)
#endif

CCBBRecord::CCBBRecord(uint64 nStartPos, uint64 nEndPos, uint32 dwIP, EBBRStatus BBRStatus) {
	if (nStartPos > nEndPos) {
		wxASSERT( false );
		return;
	}
	m_nStartPos = nStartPos;
	m_nEndPos = nEndPos;
	m_dwIP = dwIP;
	m_BBRStatus = BBRStatus;
}

CCBBRecord& CCBBRecord::operator=(const CCBBRecord& cv)
{
	m_nStartPos = cv.m_nStartPos;
	m_nEndPos = cv.m_nEndPos;
	m_dwIP = cv.m_dwIP;
	m_BBRStatus = cv.m_BBRStatus;
	return *this; 
}

bool CCBBRecord::Merge(uint64 nStartPos, uint64 nEndPos, uint32 dwIP, EBBRStatus BBRStatus) {

	if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos)) {
		if (nStartPos == m_nEndPos + 1) {
			m_nEndPos = nEndPos;
		} else if (nEndPos + 1 == m_nStartPos) {
			m_nStartPos = nStartPos;
		} else {
			wxASSERT( false );
		}

		return true;
	} else {
		return false;
	}
}

bool CCBBRecord::CanMerge(uint64 nStartPos, uint64 nEndPos, uint32 dwIP, EBBRStatus BBRStatus) {

	if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos)) {
		return true;
	} else {
		return false;
	}
}

void CCorruptionBlackBox::Init(uint64 nFileSize) {
	m_aaRecords.resize((nFileSize + PARTSIZE - 1) / PARTSIZE);
}

void CCorruptionBlackBox::Free() {
	m_aaRecords.clear();
}

void CCorruptionBlackBox::TransferredData(uint64 nStartPos, uint64 nEndPos, const CUpDownClient* pSender) {
	if (nEndPos - nStartPos >= PARTSIZE) {
		wxASSERT( false );
		return;
	}
	if (nStartPos > nEndPos) {
		wxASSERT( false );
		return;
	}
	uint32 dwSenderIP = pSender->GetIP();
	// we store records seperated for each part, so we don't have to search all entries everytime
	
	// convert pos to relative block pos
	uint16 nPart = (uint16)(nStartPos / PARTSIZE);
	uint64 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint64 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE) {
		// data crosses the partborder, split it
		nRelEndPos = PARTSIZE-1;
		uint64 nTmpStartPos = nPart*PARTSIZE + nRelEndPos + 1;
		TransferredData(nTmpStartPos, nEndPos, pSender);
	}
	if (nPart >= m_aaRecords.size()) {
		m_aaRecords.resize(nPart+1);
	}
	int posMerge = -1;
	uint64 ndbgRewritten = 0;
	for (size_t i = 0; i < m_aaRecords[nPart].size(); i++) {
		if (m_aaRecords[nPart][i].CanMerge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE)) {
			posMerge = i;
		// check if there is already an pending entry and overwrite it
		} else if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE) {
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				// old one is included in new one -> delete
				ndbgRewritten += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart].erase(m_aaRecords[nPart].begin() + i);
				i--;
			} else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos) {
			    // old one includes new one
				// check if the old one and new one have the same ip
				if (dwSenderIP != m_aaRecords[nPart][i].m_dwIP) {
					// different IP, means we have to split it 2 times
					uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
					uint64 nTmpStartPos1 = nRelEndPos + 1;
					uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
					uint64 nTmpEndPos2 = nRelStartPos - 1;
					m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
					m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
					uint32 dwOldIP = m_aaRecords[nPart][i].m_dwIP;
					m_aaRecords[nPart][i].m_dwIP = dwSenderIP;
					m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos1,nTmpEndPos1, dwOldIP));
					m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos2,nTmpEndPos2, dwOldIP));
					// and are done then
				}
				AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (1)")) % (nRelEndPos - nRelStartPos + 1));
				return;
			} else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos) {
				// old one laps over new one on the right site
				wxASSERT( nRelEndPos - m_aaRecords[nPart][i].m_nStartPos > 0 );
				ndbgRewritten += nRelEndPos - m_aaRecords[nPart][i].m_nStartPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelEndPos + 1;
			} else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				// old one laps over new one on the left site
				wxASSERT( m_aaRecords[nPart][i].m_nEndPos - nRelStartPos > 0 );
				ndbgRewritten += m_aaRecords[nPart][i].m_nEndPos - nRelStartPos;
				m_aaRecords[nPart][i].m_nEndPos = nRelStartPos - 1;
			}
		}
	}
	if (posMerge != -1) {
		wxASSERT( m_aaRecords[nPart][posMerge].Merge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE) );
	} else {
		m_aaRecords[nPart].push_back(CCBBRecord(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE));
	}
	
	if (ndbgRewritten > 0) {
		AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (2)")) % ndbgRewritten);
	}
}

void CCorruptionBlackBox::VerifiedData(uint64 nStartPos, uint64 nEndPos) {
	if (nEndPos - nStartPos >= PARTSIZE) {
		wxASSERT( false );
		return;
	}
	// convert pos to relative block pos
	uint16 nPart = (uint16)(nStartPos / PARTSIZE);
	uint64 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint64 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE) {
		wxASSERT( false );
		return;
	}
	if (nPart >= (uint16)m_aaRecords.size()) {
		m_aaRecords.resize(nPart+1);
	}
	uint64 nDbgVerifiedBytes = 0;
#ifdef __DEBUG__
	std::map<int, int> mapDebug;
#endif
	for (size_t i= 0; i < m_aaRecords[nPart].size(); i++) {
		if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE || m_aaRecords[nPart][i].m_BBRStatus == BBR_VERIFIED) {
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug[m_aaRecords[nPart][i].m_dwIP] = 1);
			} else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos) {
			    // need to split it 2*
				uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
				uint64 nTmpStartPos1 = nRelEndPos + 1;
				uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
				uint64 nTmpEndPos2 = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug[m_aaRecords[nPart][i].m_dwIP] = 1);
			} else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos) {
				// need to split it
				uint64 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
				uint64 nTmpStartPos = nRelEndPos + 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug[m_aaRecords[nPart][i].m_dwIP] = 1);
			} else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				// need to split it
				uint64 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
				uint64 nTmpEndPos = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug[m_aaRecords[nPart][i].m_dwIP] = 1);
			}
		}
	}
#ifdef __DEBUG__
	AddDebugLogLineN(logPartFile, CFormat(wxT("Found and marked %u recorded bytes of %u as verified in the CorruptionBlackBox records, %u records found, %u different clients"))
		% nDbgVerifiedBytes % (nEndPos-nStartPos+1) % m_aaRecords[nPart].size() % mapDebug.size());
#endif
}



void CCorruptionBlackBox::CorruptedData(uint64 nStartPos, uint64 nEndPos) {
	if (nEndPos - nStartPos >= EMBLOCKSIZE) {
		wxASSERT( false );
		return;
	}
	// convert pos to relative block pos
	uint16 nPart = (uint16)(nStartPos / PARTSIZE);
	uint64 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint64 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE) {
		wxASSERT( false );
		return;
	}
	if (nPart >= m_aaRecords.size()) {
		m_aaRecords.resize(nPart+1);
	}
	uint64 nDbgVerifiedBytes = 0;
	for (size_t i = 0; i < m_aaRecords[nPart].size(); i++) {
		if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE) {
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
			} else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos) {
			    // need to split it 2*
				uint64 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
				uint64 nTmpStartPos1 = nRelEndPos + 1;
				uint64 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
				uint64 nTmpEndPos2 = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
			} else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos) {
				// need to split it
				uint64 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
				uint64 nTmpStartPos = nRelEndPos + 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
			} else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos) {
				// need to split it
				uint64 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
				uint64 nTmpEndPos = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].push_back(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
			}
		}
	}
	AddDebugLogLineN(logPartFile, CFormat(wxT("Found and marked %d recorded bytes of %d as corrupted in the CorruptionBlackBox records"))
		% nDbgVerifiedBytes % (nEndPos-nStartPos+1));
}

void CCorruptionBlackBox::EvaluateData(uint16 nPart)
{
	std::vector<uint32> aGuiltyClients;
	for (size_t i = 0; i < m_aaRecords[nPart].size(); i++)
		if (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED)
			aGuiltyClients.push_back(m_aaRecords[nPart][i].m_dwIP);

	// check if any IPs are already banned, so we can skip the test for those
	for(size_t k = 0; k < aGuiltyClients.size();) {
		// remove doubles
		for(size_t y = k+1; y < aGuiltyClients.size();) {
			if (aGuiltyClients[k] == aGuiltyClients[y]) {
				aGuiltyClients.erase(aGuiltyClients.begin() + y);
			} else {
				y++;
			}
		}
		if (theApp->clientlist->IsBannedClient(aGuiltyClients[k])) {
			AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Suspicous IP (%s) is already banned, skipping recheck")) 
				% Uint32toStringIP(aGuiltyClients[k]));
			aGuiltyClients.erase(aGuiltyClients.begin() + k);
		} else {
			k++;
		}
	}
	if (aGuiltyClients.size() > 0) {
		// parse all recorded data for this file to produce a statistic for the involved clients
		
		// first init arrays for the statistic
		std::vector<uint64> aDataCorrupt;
		std::vector<uint64> aDataVerified;
		aDataCorrupt.resize(aGuiltyClients.size());
		aDataVerified.resize(aGuiltyClients.size());
		for (size_t j = 0; j < aGuiltyClients.size(); j++) {
			aDataCorrupt[j] = aDataVerified[j] = 0;
		}

		// now the parsing
		for (size_t part = 0; part < m_aaRecords.size(); part++) {
			for (size_t i = 0; i < m_aaRecords[part].size(); i++) {
				for (size_t k = 0; k < aGuiltyClients.size(); k++) {
					if (m_aaRecords[part][i].m_dwIP == aGuiltyClients[k]) {
						if (m_aaRecords[part][i].m_BBRStatus == BBR_CORRUPTED) {
							// corrupted data records are always counted as at least blocksize or bigger
							uint32 corr = m_aaRecords[part][i].m_nEndPos - m_aaRecords[part][i].m_nStartPos + 1;
							aDataCorrupt[k] += (corr > EMBLOCKSIZE) ? corr : EMBLOCKSIZE;
						} else if (m_aaRecords[part][i].m_BBRStatus == BBR_VERIFIED) {
							aDataVerified[k] += (m_aaRecords[part][i].m_nEndPos-m_aaRecords[part][i].m_nStartPos)+1;
						}
					}
				}
			}
		}
		for (size_t k = 0; k < aGuiltyClients.size(); k++) {
			// calculate the percentage of corrupted data for each client and ban
			// him if the limit is reached
			int nCorruptPercentage;
			if ((aDataVerified[k] + aDataCorrupt[k]) > 0) {
				nCorruptPercentage = (int)(((uint64)aDataCorrupt[k]*100)/(aDataVerified[k] + aDataCorrupt[k]));
			} else {
				AddDebugLogLineN(logPartFile, wxT("CorruptionBlackBox: Programm Error: No records for guilty client found!"));
				wxASSERT( false );
				nCorruptPercentage = 0;
			}
			if ( nCorruptPercentage > CBB_BANTHRESHOLD) {

				CUpDownClient* pEvilClient = theApp->clientlist->FindClientByIP(aGuiltyClients[k]);
				if (pEvilClient != NULL) {
					AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"))
						% CastItoXBytes(aDataCorrupt[k]) % CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])) % pEvilClient->GetClientFullInfo());
					theApp->clientlist->AddTrackClient(pEvilClient);
					pEvilClient->Ban();  // Identified as sender of corrupt data
				} else {
					AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"))
						% CastItoXBytes(aDataCorrupt[k]) % CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])) % Uint32toStringIP(aGuiltyClients[k]));
					theApp->clientlist->AddBannedClient(aGuiltyClients[k]);
				}
			} else {
				CUpDownClient* pSuspectClient = theApp->clientlist->FindClientByIP(aGuiltyClients[k]);
				if (pSuspectClient != NULL) {
					AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Reporting: Found client which probably send %s of %s corrupted data, but it is within the acceptable limit, %s"))
						% CastItoXBytes(aDataCorrupt[k]) % CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])) % pSuspectClient->GetClientFullInfo());
					theApp->clientlist->AddTrackClient(pSuspectClient);
				} else {
					AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox: Reporting: Found client which probably send %s of %s corrupted data, but it is within the acceptable limit, %s"))
						% CastItoXBytes(aDataCorrupt[k]) % CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])) % Uint32toStringIP(aGuiltyClients[k]));
				}
			}
		}
	}
}
