//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Stu Redman ( sturedman@amule.org / http://www.amule.org )
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

#include "IndexedDB.h"
#include "../../Database.h"
#include "../../Logger.h"
#include <protocol/kad/Constants.h>

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CIndexedDB::CIndexedDB()
{
	m_lastClean = time(NULL) + (60*30);
	m_totalIndexSource = 0;
	m_totalIndexKeyword = 0;
	m_totalIndexNotes = 0;
	m_totalIndexLoad = 0;
}

bool CIndexedDB::AddKeyword(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CKeyEntry* entry, uint8_t& load)
{
	if (!entry) {
		return false;
	}

	wxCHECK(entry->IsKeyEntry(), false);

	if (m_totalIndexKeyword > KADEMLIAMAXENTRIES) {
		load = 100;
		return false;
	}

	wxString filename = entry->GetCommonFileName();
	if (entry->m_uSize == 0 || filename.IsEmpty() || entry->GetTagCount() == 0 || entry->m_tLifeTime < time(NULL)) {
		return false;
	}

	try {
		CDatabaseTransaction transaction(database);

		uint64 sourceIndex;
		uint64 keywordIndex = database->KadGetKeyword(keyID.GetLowerHalf(), keyID.GetUpperHalf());
		if (keywordIndex == 0) {
			keywordIndex = database->KadInsertKeyword(keyID.GetLowerHalf(), keyID.GetUpperHalf());
			AddDebugLogLineN(logSQL, CFormat(wxT("New keyword %d")) % keywordIndex);
			sourceIndex = database->KadInsertSource(keywordIndex, sourceID.GetLowerHalf(), sourceID.GetUpperHalf(),
				entry->m_uSize, entry->m_tLifeTime);
			AddDebugLogLineN(logSQL, CFormat(wxT("New source %d for keyword %d")) % sourceIndex % keywordIndex);
			m_totalIndexKeyword++;
			load = 1;
		} else {
			uint32 indexTotal = database->KadCountSource(keywordIndex);
			if (indexTotal > KADEMLIAMAXINDEX) {
				load = 100;
				//Too many entries for this Keyword..
				return false;
			}
			// If there are different sizes published for a source hash they are treated as different sources.
			sourceIndex = database->KadGetSource(keywordIndex, sourceID.GetLowerHalf(), sourceID.GetUpperHalf(), entry->m_uSize);
			if (sourceIndex) {
				if (indexTotal > KADEMLIAMAXINDEX - 5000) {
					load = 100;
					//We are in a hot node.. If we continued to update all the publishes
					//while this index is full, popular files will be the only thing you index.
					return false;
				}
				database->KadUpdateSource(sourceIndex, entry->m_tLifeTime);
				AddDebugLogLineN(logSQL, CFormat(wxT("Update source %d for keyword %d")) % sourceIndex % keywordIndex);
			} else {
				sourceIndex = database->KadInsertSource(keywordIndex, sourceID.GetLowerHalf(), sourceID.GetUpperHalf(),
					entry->m_uSize, entry->m_tLifeTime);
				m_totalIndexKeyword++;
				AddDebugLogLineN(logSQL, CFormat(wxT("New source %d for keyword %d")) % sourceIndex % keywordIndex);
			}
			load = (indexTotal * 100) / KADEMLIAMAXINDEX;
		}
		if (entry->HasPublishingIPs()) {
			entry->MergeIPsAndFilenames(database, sourceIndex); // IpTracking init
		} else {
			MergeIPsAndFilenames(sourceIndex, filename, entry->m_uIP);
		}
		database->KadReplaceTaglist(sourceIndex, entry->GetTagList(), false);
		return true;
	} catch (wxString s) {
		database->ShowError(s);
 		return false;
	} catch (...) {
		AddDebugLogLineN(logSQL, wxT("WTF ?"));
		return false;
	}
	//
	// TODO: get that CKeyEntry deleted when we switch of old CIndexed
	//
}


void CIndexedDB::MergeIPsAndFilenames(uint64 sourceIndex, const wxString & filename, uint32 uIP)
{
	int refresh = 0;	// 0: new ip, 1: refresh, 2: fast refresh

	if (filename.empty()) {
		AddDebugLogLineN(logSQL, wxT("skipped entry without file name"));	// maybe this should cause the whole packet to be skipped
		return;
	}
	if (! uIP) {
		AddDebugLogLineN(logSQL, wxT("skipped entry with zero IP"));	// maybe this should cause the whole packet to be skipped
		return;
	}
	// put publishing IP in database, remove oldest IP if there are too many, update tracking list
	refresh = database->KadSetKeywordPublishIP(sourceIndex, uIP, 0);
	database->KadSetSourcename(sourceIndex, filename, 1, refresh < 2);

	//AddDebugLogLineN(logKadEntryTracking, CFormat(wxT("Indexed Keyword, Refresh: %s, Current Publisher: %s, Total Publishers: %u, Total different Names: %u, TrustValue: %.2f, file: %s"))
	//	% (refresh ? wxT("Yes") : wxT("No")) % KadIPToString(uIP) % m_publishingIPs->size() % m_filenames.size() % m_trustValue % m_uSourceID.ToHexString());
}


// This is only for importing entries from saved files
void CKeyEntry::MergeIPsAndFilenames(CDatabase* database, uint64 sourceIndex)
{
	// This is used only when importing from saved file
	for (PublishingIPList::iterator it = m_publishingIPs->begin(); it != m_publishingIPs->end(); it++) {
		database->KadSetKeywordPublishIP(sourceIndex, it->m_ip, it->m_lastPublish);
	}
	for (FileNameList::iterator it = m_filenames.begin(); it != m_filenames.end(); it++) {
		// This is a loop only when importing from saved file, usually it's a single entry.
		database->KadSetSourcename(sourceIndex, it->m_filename, it->m_popularityIndex, false);
	}
}


void CIndexedDB::StartupCleanup()
{
	database->KadStartupCleanup();
}


void CIndexedDB::SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32_t ip, uint16_t port, bool oldClient, uint16_t startPosition, const CKadUDPKey& senderKey, CMemFile & mirror)
{
}
