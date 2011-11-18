//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Stu Redman ( sturedman@amule.org )
// Copyright (C) 2002-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include <protocol/ed2k/Constants.h>	// needed for PARTSIZE
#include "updownclient.h"		// Needed for CUpDownClient
#include "Logger.h"
#include "amule.h"				// needed for theApp
#include "ClientList.h"
#include "NetworkFunctions.h"	// needed for Uint32toStringIP
#include "OtherFunctions.h"		// needed for CastItoXBytes
#include <common/Format.h>		// needed for CFormat
#include <common/Macros.h>

#define	 CBB_BANTHRESHOLD	32 //% max corrupted data	

// Record to store information which piece of data was downloaded from which client

CCorruptionBlackBox::CCBBRecord::CCBBRecord(uint32 nStartPos, uint32 nEndPos, uint32 dwIP)
{
	if (nStartPos > nEndPos) {
		wxFAIL;
		return;
	}
	m_nStartPos = nStartPos;
	m_nEndPos = nEndPos;
	m_dwIP = dwIP;
}

// Try to merge new data to an existing record
bool CCorruptionBlackBox::CCBBRecord::Merge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP)
{
	if (m_dwIP == dwIP) {
		if (nStartPos == m_nEndPos + 1) {
			m_nEndPos = nEndPos;
			return true;
		} else if (nEndPos + 1 == m_nStartPos) {
			m_nStartPos = nStartPos;
			return true;
		}
	}
	return false;
}

// Release memory (after download completes)
void CCorruptionBlackBox::Free()
{
	m_Records.clear();
	m_goodClients.clear();
	m_badClients.clear();
}

// Store a piece of received data (don't know if it's good or bad yet).
// Data is stored in a list for the chunk in belongs to.
void CCorruptionBlackBox::TransferredData(uint64 nStartPos, uint64 nEndPos, uint32 senderIP)
{
	if (nStartPos > nEndPos) {
		wxFAIL;
		return;
	}
	
	// convert pos to relative block pos
	uint16 nPart = (uint16)(nStartPos / PARTSIZE);
	uint32 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint32 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE) {
		// data crosses the partborder, split it
		// (for the fun of it, this should never happen)
		nRelEndPos = PARTSIZE-1;
		TransferredData((nPart+1)*PARTSIZE, nEndPos, senderIP);
	}
	//
	// Let's keep things simple.
	// We don't request data we already have.
	// We check if received data exceeds block boundaries.
	// -> There should not be much overlap here.
	// So just stuff everything received into the list and only join adjacent blocks.
	//
	CRecordList & list = m_Records[nPart]; // this creates the entry if it doesn't exist yet
	bool merged = false;
	for (CRecordList::iterator it = list.begin(); it != list.end() && !merged; ++it) {
		merged = it->Merge(nRelStartPos, nRelEndPos, senderIP);
	}
	if (!merged) {
		list.push_back(CCBBRecord(nRelStartPos, nRelEndPos, senderIP));
		AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox(%s): transferred: new record for part %d (%d - %d, %s)")) 
			% m_partNumber % nPart % nRelStartPos % nRelEndPos % Uint32toStringIP(senderIP));
	}
}

// Mark a piece of data as good or bad.
// Piece is removed from the chunk list and added to the client's record.
void CCorruptionBlackBox::VerifiedData(bool ok, uint16 nPart, uint32 nRelStartPos, uint32 nRelEndPos) {
	if (nRelStartPos > nRelEndPos) {
		wxFAIL;
		return;
	}

	CRecordList & list = m_Records[nPart];
#ifdef __DEBUG__
	std::map<uint32, bool> mapDebug;
	uint32 nDbgVerifiedBytes = 0;
	size_t listsize1 = list.size();
#endif
	for (CRecordList::iterator it1 = list.begin(); it1 != list.end();) {
		CRecordList::iterator it = it1++;
		uint32 & curStart = it->m_nStartPos;
		uint32 & curEnd   = it->m_nEndPos;
		uint32 ip = it->m_dwIP;
		uint32 data = 0;
		if (curStart >= nRelStartPos && curStart <= nRelEndPos) {
			// [arg
			//   [cur
			if (curEnd > nRelEndPos) {
				// [arg]
				//   [cur]
				data = nRelEndPos - curStart + 1;
				curStart = nRelEndPos + 1;
			} else {
				// [arg    ]
				//   [cur]
				data = curEnd - curStart + 1;
				list.erase(it);
			}
		} else if (curStart < nRelStartPos && curEnd >= nRelStartPos) {
			//   [arg
			// [cur
			if (curEnd > nRelEndPos) {
				//   [arg]
				// [cur    ]
				data = nRelEndPos - nRelStartPos + 1;
				// split it: insert new block before current block
				list.insert(it, CCBBRecord(curStart, nRelStartPos - 1, ip));
				curStart = nRelEndPos + 1;
			} else {
				//   [arg]
				// [cur]
				data = curEnd - nRelStartPos + 1;
				curEnd = nRelStartPos - 1;
			}
		// else no overlap
		}
		if (data) {
			if (ok) {
				m_goodClients[ip].m_downloaded += data;
			} else {
				// corrupted data records are always counted as at least blocksize or bigger
				m_badClients[ip].m_downloaded += (data > EMBLOCKSIZE) ? data : EMBLOCKSIZE;;
			}
			DEBUG_ONLY(nDbgVerifiedBytes += data);
			DEBUG_ONLY(mapDebug[ip] = 1);
		}
	}
	DEBUG_ONLY(size_t listsize2 = list.size());
	// when everything is added to the stats drop the whole record
	if (list.empty()) {
		m_Records.erase(nPart);
	}
#ifdef __DEBUG__
	AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox(%s): found and marked %d recorded bytes of %d as %s in part %d, %d records found, %d records left, %d different clients"))
		% m_partNumber % nDbgVerifiedBytes % (nRelEndPos-nRelStartPos+1) % (ok ? wxT("verified") : wxT("corrupt"))
		% nPart % listsize1 % listsize2 % mapDebug.size());
#endif
}

// Check all clients that uploaded corrupted data,
// and ban them if they didn't upload enough good data too.
void CCorruptionBlackBox::EvaluateData()
{
	CCBBClientMap::iterator it = m_badClients.begin();
	for (; it != m_badClients.end(); ++it) {
		uint32 ip = it->first;
		uint64 bad = it->second.m_downloaded;
		if (!bad) {
			wxFAIL;		// this should not happen
			continue;
		}
		uint64 good = 0;
		CCBBClientMap::iterator it2 = m_goodClients.find(ip);
		if (it2 != m_goodClients.end()) {
			good = it2->second.m_downloaded;
		}

		int nCorruptPercentage = bad * 100 / (bad + good);

		if (nCorruptPercentage > CBB_BANTHRESHOLD) {
			CUpDownClient* pEvilClient = theApp->clientlist->FindClientByIP(ip);
			wxString clientName;
			if (pEvilClient != NULL) {
				clientName = pEvilClient->GetClientShortInfo();
				AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox(%s): Banning: Found client which sent %d of %d corrupted data, %s"))
					% m_partNumber % bad % (good + bad) % pEvilClient->GetClientFullInfo());
				theApp->clientlist->AddTrackClient(pEvilClient);
				pEvilClient->Ban();  // Identified as sender of corrupt data
				// Stop download right away
				pEvilClient->SetDownloadState(DS_BANNED);
				if (pEvilClient->Disconnected(wxT("Upload of corrupted data"))) {
					pEvilClient->Safe_Delete();
				}
			} else {
				clientName = Uint32toStringIP(ip);
				theApp->clientlist->AddBannedClient(ip);
			}
			AddLogLineN(CFormat(_("Banned client %s for sending %s corrupt data of %s total for the file '%s'")) 
				% clientName % CastItoXBytes(bad) % CastItoXBytes(good + bad) % m_fileName);
		} else {
			CUpDownClient* pSuspectClient = theApp->clientlist->FindClientByIP(ip);
			if (pSuspectClient != NULL) {
				AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox(%s): Reporting: Found client which probably sent %d of %d corrupted data, but it is within the acceptable limit, %s"))
					% m_partNumber % bad % (good + bad) % pSuspectClient->GetClientFullInfo());
				theApp->clientlist->AddTrackClient(pSuspectClient);
			} else {
				AddDebugLogLineN(logPartFile, CFormat(wxT("CorruptionBlackBox(%s): Reporting: Found client which probably sent %d of %d corrupted data, but it is within the acceptable limit, %s"))
					% m_partNumber % bad % (good + bad) % Uint32toStringIP(ip));
			}
		}
	}
}

// Full debug output of all data
void CCorruptionBlackBox::DumpAll()
{
#ifdef __DEBUG__
	AddDebugLogLineN(logPartFile, wxT("CBB Dump Records"));
	std::map<uint16, CRecordList>::iterator it = m_Records.begin();
	for (; it != m_Records.end(); ++it) {
		uint16 block = it->first;
		CRecordList & list = it->second;
		for (CRecordList::iterator it2 = list.begin(); it2 != list.end(); ++it2) {
			AddDebugLogLineN(logPartFile, CFormat(wxT("CBBD %6d %.16s %10d - %10d"))
				% block % Uint32toStringIP(it2->m_dwIP) % it2->m_nStartPos % it2->m_nEndPos);
		}
	}
	if (!m_goodClients.empty()) {
		AddDebugLogLineN(logPartFile, wxT("CBB Dump good Clients"));
		CCBBClientMap::iterator it3 = m_goodClients.begin();
		for (; it3 != m_goodClients.end(); ++it3) {
			AddDebugLogLineN(logPartFile, CFormat(wxT("CBBD %.16s good %10d"))
				% Uint32toStringIP(it3->first) % it3->second.m_downloaded);
		}
	}
	if (!m_badClients.empty()) {
		AddDebugLogLineN(logPartFile, wxT("CBB Dump bad Clients"));
		CCBBClientMap::iterator it3 = m_badClients.begin();
		for (; it3 != m_badClients.end(); ++it3) {
			AddDebugLogLineN(logPartFile, CFormat(wxT("CBBD %.16s bad %10d"))
				% Uint32toStringIP(it3->first) % it3->second.m_downloaded);
		}
	}
#endif
}
