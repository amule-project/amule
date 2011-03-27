//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "Entry.h"
#include <common/Macros.h>
#include <tags/FileTags.h>
#include <protocol/kad/Constants.h>
#include "Indexed.h"
#include "../../SafeFile.h"
#include "../../GetTickCount.h"
#include "../../Logger.h"
#include "../../NetworkFunctions.h"

using namespace Kademlia;

CKeyEntry::GlobalPublishIPMap	CKeyEntry::s_globalPublishIPs;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// CEntry
CEntry::~CEntry()
{
	deleteTagPtrListEntries(&m_taglist);
}

CEntry* CEntry::Copy() const
{
	CEntry* entry = new CEntry();
	for (FileNameList::const_iterator it = m_filenames.begin(); it != m_filenames.end(); ++it) {
		entry->m_filenames.push_back(*it);
	}
	entry->m_uIP = m_uIP;
	entry->m_uKeyID.SetValue(m_uKeyID);
	entry->m_tLifeTime = m_tLifeTime;
	entry->m_uSize = m_uSize;
	entry->m_bSource = m_bSource;
	entry->m_uSourceID.SetValue(m_uSourceID);
	entry->m_uTCPport = m_uTCPport;
	entry->m_uUDPport = m_uUDPport;
	for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
		entry->m_taglist.push_back((*it)->CloneTag());
	}
	return entry;
}

bool CEntry::GetIntTagValue(const wxString& tagname, uint64_t& value, bool includeVirtualTags) const
{
	for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
		if ((*it)->IsInt() && ((*it)->GetName() == tagname)) {
			value = (*it)->GetInt();
			return true;
		}
	}

	if (includeVirtualTags) {
		// SizeTag is not stored anymore, but queried in some places
		if (tagname == TAG_FILESIZE) {
			value = m_uSize;
			return true;
		}
	}
	value = 0;
	return false;
}

wxString CEntry::GetStrTagValue(const wxString& tagname) const
{
	for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
		if (((*it)->GetName() == tagname) && (*it)->IsStr()) {
			return (*it)->GetStr();
		}
	}
	return wxEmptyString;
}

void CEntry::SetFileName(const wxString& name)
{
	if (!m_filenames.empty()) {
		wxFAIL;
		m_filenames.clear();
	}
	sFileNameEntry sFN = { name, 1 };
	m_filenames.push_front(sFN);
}

wxString CEntry::GetCommonFileName() const
{
	// return the filename on which most publishers seem to agree on
	// due to the counting, this doesn't has to be excact, we just want to make sure to not use a filename which just
	// a few bad publishers used and base or search matching and answering on this, instead of the most popular name
	// Note: The Index values are not the acutal numbers of publishers, but just a relativ number to compare to other entries
	FileNameList::const_iterator result = m_filenames.end();
	uint32_t highestPopularityIndex = 0;
	for (FileNameList::const_iterator it = m_filenames.begin(); it != m_filenames.end(); ++it) {
		if (it->m_popularityIndex > highestPopularityIndex) {
			highestPopularityIndex = it->m_popularityIndex;
			result = it;
		}
	}
	wxString strResult(result != m_filenames.end() ? result->m_filename : wxString(wxEmptyString));
	wxASSERT(!strResult.IsEmpty() || m_filenames.empty());
	return strResult;
}

void CEntry::WriteTagListInc(CFileDataIO* data, uint32_t increaseTagNumber)
{
	// write taglist and add name + size tag
	wxCHECK_RET(data != NULL, wxT("data must not be NULL"));

	uint32_t count = GetTagCount() + increaseTagNumber;	// will include name and size tag in the count if needed
	wxASSERT(count <= 0xFF);
	data->WriteUInt8((uint8_t)count);

	if (!GetCommonFileName().IsEmpty()){
		wxASSERT(count > m_taglist.size());
		data->WriteTag(CTagString(TAG_FILENAME, GetCommonFileName()));
	}
	if (m_uSize != 0){
		wxASSERT(count > m_taglist.size());
		data->WriteTag(CTagVarInt(TAG_FILESIZE, m_uSize));
	}

	for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
		data->WriteTag(**it);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////// CKeyEntry
CKeyEntry::CKeyEntry()
{
	m_publishingIPs = NULL;
	m_trustValue = 0;
	m_lastTrustValueCalc = 0;
}

CKeyEntry::~CKeyEntry()
{
	if (m_publishingIPs != NULL) {
		for (PublishingIPList::const_iterator it = m_publishingIPs->begin(); it != m_publishingIPs->end(); ++it) {
			AdjustGlobalPublishTracking(it->m_ip, false, wxT("instance delete"));
		}
		delete m_publishingIPs;
		m_publishingIPs = NULL;
	}
}

bool CKeyEntry::SearchTermsMatch(const SSearchTerm* searchTerm) const
{
	// boolean operators
	if (searchTerm->type == SSearchTerm::AND) {
		return SearchTermsMatch(searchTerm->left) && SearchTermsMatch(searchTerm->right);
	}

	if (searchTerm->type == SSearchTerm::OR) {
		return SearchTermsMatch(searchTerm->left) || SearchTermsMatch(searchTerm->right);
	}

	if (searchTerm->type == SSearchTerm::NOT) {
		return SearchTermsMatch(searchTerm->left) && !SearchTermsMatch(searchTerm->right);
	}

	// word which is to be searched in the file name (and in additional meta data as done by some ed2k servers???)
	if (searchTerm->type == SSearchTerm::String) {
		int strSearchTerms = searchTerm->astr->GetCount();
		if (strSearchTerms == 0) {
			return false;
		}
		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		wxString commonFileNameLower(GetCommonFileNameLowerCase());
		for (int i = 0; i < strSearchTerms; i++) {
			// this will not give the same results as when tokenizing the filename string, but it is 20 times faster.
			if (commonFileNameLower.Find((*(searchTerm->astr))[i]) == -1) {
				return false;
			}
		}
		return true;
	}

	if (searchTerm->type == SSearchTerm::MetaTag) {
		if (searchTerm->tag->GetType() == 2) {	// meta tags with string values
			if (searchTerm->tag->GetName() == TAG_FILEFORMAT) {
				// 21-Sep-2006 []: Special handling for TAG_FILEFORMAT which is already part
				// of the filename and thus does not need to get published nor stored explicitly,
				wxString commonFileName(GetCommonFileName());
				int ext = commonFileName.Find(wxT('.'), true);
				if (ext != wxNOT_FOUND) {
					return commonFileName.Mid(ext + 1).CmpNoCase(searchTerm->tag->GetStr()) == 0;
				}
			} else {
				for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
					if ((*it)->IsStr() && searchTerm->tag->GetName() == (*it)->GetName()) {
						return (*it)->GetStr().CmpNoCase(searchTerm->tag->GetStr()) == 0;
					}
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpGreaterEqual) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value >= searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() >= searchTerm->tag->GetFloat();
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpLessEqual) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value <= searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() <= searchTerm->tag->GetFloat();
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpGreater) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value > searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() > searchTerm->tag->GetFloat();
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpLess) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value < searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() < searchTerm->tag->GetFloat();
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpEqual) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value == searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() == searchTerm->tag->GetFloat();
				}
			}
		}
	} else if (searchTerm->type == SSearchTerm::OpNotEqual) {
		if (searchTerm->tag->IsInt()) {	// meta tags with integer values
			uint64_t value;
			if (GetIntTagValue(searchTerm->tag->GetName(), value, true)) {
				return value != searchTerm->tag->GetInt();
			}
		} else if (searchTerm->tag->IsFloat()) {	// meta tags with float values
			for (TagPtrList::const_iterator it = m_taglist.begin(); it != m_taglist.end(); ++it) {
				if ((*it)->IsFloat() && searchTerm->tag->GetName() == (*it)->GetName()) {
					return (*it)->GetFloat() != searchTerm->tag->GetFloat();
				}
			}
		}
	}

	return false;
}

void CKeyEntry::AdjustGlobalPublishTracking(uint32_t ip, bool increase, const wxString& DEBUG_ONLY(dbgReason))
{
	uint32_t count = 0;
	bool found = false;
	GlobalPublishIPMap::const_iterator it = s_globalPublishIPs.find(ip & 0xFFFFFF00 /* /24 netmask, take care of endian if needed */ );
	if (it != s_globalPublishIPs.end()) {
		count = it->second;
		found = true;
	}

	if (increase) {
		count++;
	} else {
		count--;
	}

	if (count > 0) {
		s_globalPublishIPs[ip & 0xFFFFFF00] = count;
	} else if (found) {
		s_globalPublishIPs.erase(ip & 0xFFFFFF00);
	} else {
		wxFAIL;
	}
#ifdef __DEBUG__
	if (!dbgReason.IsEmpty()) {
		AddDebugLogLineN(logKadEntryTracking, CFormat(wxT("%s %s (%s) - (%s), new count %u"))
			% (increase ? wxT("Adding") : wxT("Removing")) % KadIPToString(ip & 0xFFFFFF00) % KadIPToString(ip) % dbgReason % count);
	}
#endif
}

void CKeyEntry::MergeIPsAndFilenames(CKeyEntry* fromEntry)
{
	// this is called when replacing a stored entry with a refreshed one. 
	// we want to take over the tracked IPs and the different filenames from the old entry, the rest is still
	// "overwritten" with the refreshed values. This might be not perfect for the taglist in some cases, but we can't afford
	// to store hundreds of taglists to figure out the best one like we do for the filenames now
	if (m_publishingIPs != NULL) { // This instance needs to be a new entry, otherwise we don't want/need to merge
		wxASSERT(fromEntry == NULL);
		wxASSERT(!m_publishingIPs->empty());
		wxASSERT(!m_filenames.empty());
		return;
	}

	bool refresh = false;
	if (fromEntry == NULL || fromEntry->m_publishingIPs == NULL) {
		wxASSERT(fromEntry == NULL);
		// if called with NULL, this is a complete new entry and we need to initalize our lists
		if (m_publishingIPs == NULL) {
			m_publishingIPs = new PublishingIPList();
		}
		// update the global track map below
	} else {
		// merge the tracked IPs, add this one if not already on the list
		m_publishingIPs = fromEntry->m_publishingIPs;
		fromEntry->m_publishingIPs = NULL;	
		bool fastRefresh = false;
		for (PublishingIPList::iterator it = m_publishingIPs->begin(); it != m_publishingIPs->end(); ++it) {
			if (it->m_ip == m_uIP) {
				refresh = true;
				if ((time(NULL) - it->m_lastPublish) < (KADEMLIAREPUBLISHTIMES - HR2S(1))) {
					AddDebugLogLineN(logKadEntryTracking, wxT("FastRefresh publish, ip: ") + KadIPToString(m_uIP));
					fastRefresh = true; // refreshed faster than expected, will not count into filenamepopularity index
				}
				it->m_lastPublish = time(NULL);
				m_publishingIPs->push_back(*it);
				m_publishingIPs->erase(it);
				break;
			}
		}

		// copy over trust value, in case we don't want to recalculate
		m_trustValue = fromEntry->m_trustValue;
		m_lastTrustValueCalc = fromEntry->m_lastTrustValueCalc;

		// copy over the different names, if they are different the one we have right now
		wxASSERT(m_filenames.size() == 1); // we should have only one name here, since it's the entry from one single source
		sFileNameEntry currentName = { wxEmptyString, 0 };
		if (m_filenames.size() != 0) {
			currentName = m_filenames.front();
			m_filenames.pop_front();
		}

		bool duplicate = false;
		for (FileNameList::iterator it = fromEntry->m_filenames.begin(); it != fromEntry->m_filenames.end(); ++it) {
			sFileNameEntry nameToCopy = *it;
			if (currentName.m_filename.CmpNoCase(nameToCopy.m_filename) == 0) {
				// the filename of our new entry matches with our old, increase the popularity index for the old one
				duplicate = true;
				if (!fastRefresh) {
					nameToCopy.m_popularityIndex++;
				}
			}
			m_filenames.push_back(nameToCopy);
		}
		if (!duplicate) {
			m_filenames.push_back(currentName);
		}
	}

	// if this was a refresh done, otherwise update the global track map
	if (!refresh) {
		wxASSERT(m_uIP != 0);
		sPublishingIP add = { m_uIP, time(NULL) };
		m_publishingIPs->push_back(add);

		// add the publisher to the tacking list
		AdjustGlobalPublishTracking(m_uIP, true, wxT("new publisher"));

		// we keep track of max 100 IPs, in order to avoid too much time for calculation/storing/loading.
		if (m_publishingIPs->size() > 100) {
			sPublishingIP curEntry = m_publishingIPs->front();
			m_publishingIPs->pop_front();
			AdjustGlobalPublishTracking(curEntry.m_ip, false, wxT("more than 100 publishers purge"));
		}

		// since we added a new publisher, we want to (re)calculate the trust value for this entry		
		ReCalculateTrustValue();
	}
	AddDebugLogLineN(logKadEntryTracking, CFormat(wxT("Indexed Keyword, Refresh: %s, Current Publisher: %s, Total Publishers: %u, Total different Names: %u, TrustValue: %.2f, file: %s"))
		% (refresh ? wxT("Yes") : wxT("No")) % KadIPToString(m_uIP) % m_publishingIPs->size() % m_filenames.size() % m_trustValue % m_uSourceID.ToHexString());
}

void CKeyEntry::ReCalculateTrustValue()
{
#define PUBLISHPOINTSSPERSUBNET	10.0
	// The trustvalue is supposed to be an indicator how trustworthy/important (or spammy) this entry is and lies between 0 and ~10000,
	// but mostly we say everything below 1 is bad, everything above 1 is good. It is calculated by looking at how many different
	// IPs/24 have published this entry and how many entries each of those IPs have.
	// Each IP/24 has x (say 3) points. This means if one IP publishes 3 different entries without any other IP publishing those entries,
	// each of those entries will have 3 / 3 = 1 Trustvalue. Thats fine. If it publishes 6 alone, each entry has 3 / 6 = 0.5 trustvalue - not so good
	// However if there is another publisher for entry 5, which only publishes this entry then we have 3/6 + 3/1 = 3.5 trustvalue for this entry
	//
	// What's the point? With this rating we try to avoid getting spammed with entries for a given keyword by a small IP range, which blends out
	// all other entries for this keyword do to its amount as well as giving an indicator for the searcher. So if we are the node to index "Knoppix", and someone
	// from 1 IP publishes 500 times "knoppix casino 500% bonus.txt", all those entries will have a trustvalue of 0.006 and we make sure that
	// on search requests for knoppix, those entries are only returned after all entries with a trustvalue > 1 were sent (if there is still space).
	//
	// Its important to note that entry with < 1 do NOT get ignored or singled out, this only comes into play if we have 300 more results for
	// a search request rating > 1
	wxCHECK_RET(m_publishingIPs != NULL, wxT("No publishing IPs?"));

	m_lastTrustValueCalc = ::GetTickCount();
	m_trustValue = 0;
	wxASSERT(!m_publishingIPs->empty());
	for (PublishingIPList::iterator it = m_publishingIPs->begin(); it != m_publishingIPs->end(); ++it) {
		sPublishingIP curEntry = *it;
		uint32_t count = 0;
		GlobalPublishIPMap::const_iterator itMap = s_globalPublishIPs.find(curEntry.m_ip & 0xFFFFFF00 /* /24 netmask, take care of endian if needed*/);
		if (itMap != s_globalPublishIPs.end()) {
			count = itMap->second;
		}
		if (count > 0) {
			m_trustValue += PUBLISHPOINTSSPERSUBNET / count;
		} else {
			AddDebugLogLineN(logKadEntryTracking, wxT("Inconsistency in RecalcualteTrustValue()"));
			wxFAIL;
		}
	}
}

double CKeyEntry::GetTrustValue()
{
	// update if last calcualtion is too old, will assert if this entry is not supposed to have a trustvalue
	if (::GetTickCount() - m_lastTrustValueCalc > MIN2MS(10)) {
		ReCalculateTrustValue();
	}
	return m_trustValue;
}

void CKeyEntry::CleanUpTrackedPublishers()
{
	if (m_publishingIPs == NULL) {
		return;
	}

	time_t now = time(NULL);
	while (!m_publishingIPs->empty()) {
		// entries are ordered, older ones first
		sPublishingIP curEntry = m_publishingIPs->front();
		if (now - curEntry.m_lastPublish > KADEMLIAREPUBLISHTIMEK) {
			AdjustGlobalPublishTracking(curEntry.m_ip, false, wxT("cleanup"));
			m_publishingIPs->pop_front();
		} else {
			break;
		}
	}
}

void CKeyEntry::WritePublishTrackingDataToFile(CFileDataIO* data)
{
	// format: <Names_Count 4><{<Name string><PopularityIndex 4>} Names_Count><PublisherCount 4><{<IP 4><Time 4>} PublisherCount>
	data->WriteUInt32((uint32_t)m_filenames.size());
	for (FileNameList::const_iterator it = m_filenames.begin(); it != m_filenames.end(); ++it) {
		data->WriteString(it->m_filename, utf8strRaw, 2);
		data->WriteUInt32(it->m_popularityIndex);
	}

	if (m_publishingIPs != NULL) {
		data->WriteUInt32((uint32_t)m_publishingIPs->size());
		for (PublishingIPList::const_iterator it = m_publishingIPs->begin(); it != m_publishingIPs->end(); ++it) {
			wxASSERT(it->m_ip != 0);
			data->WriteUInt32(it->m_ip);
			data->WriteUInt32((uint32_t)it->m_lastPublish);
		}
	} else {
		wxFAIL;
		data->WriteUInt32(0);
	}
}

void CKeyEntry::ReadPublishTrackingDataFromFile(CFileDataIO* data)
{
	// format: <Names_Count 4><{<Name string><PopularityIndex 4>} Names_Count><PublisherCount 4><{<IP 4><Time 4>} PublisherCount>
	wxASSERT(m_filenames.empty());
	uint32_t nameCount = data->ReadUInt32();
	for (uint32_t i = 0; i < nameCount; i++) {
		sFileNameEntry toAdd;
		toAdd.m_filename = data->ReadString(true, 2);
		toAdd.m_popularityIndex = data->ReadUInt32();
		m_filenames.push_back(toAdd);
	}

	wxASSERT(m_publishingIPs == NULL);
	m_publishingIPs = new PublishingIPList();
	uint32_t ipCount = data->ReadUInt32();
#ifdef __WXDEBUG__
	uint32_t dbgLastTime = 0;
#endif
	for (uint32_t i = 0; i < ipCount; i++) {
		sPublishingIP toAdd;
		toAdd.m_ip = data->ReadUInt32();
		wxASSERT(toAdd.m_ip != 0);
		toAdd.m_lastPublish = data->ReadUInt32();
#ifdef __WXDEBUG__
		wxASSERT(dbgLastTime <= (uint32_t)toAdd.m_lastPublish); // should always be sorted oldest first
		dbgLastTime = toAdd.m_lastPublish;
#endif

		AdjustGlobalPublishTracking(toAdd.m_ip, true, wxEmptyString);

		m_publishingIPs->push_back(toAdd);
	}
	ReCalculateTrustValue();
// #ifdef __DEBUG__
// 	if (GetTrustValue() < 1.0) {
// 		AddDebugLogLineN(logKadEntryTracking,CFormat(wxT("Loaded %u different names, %u different publishIPs (trustvalue = %.2f) for file %s"))
// 			% nameCount % ipCount % GetTrustValue() % m_uSourceID.ToHexString());
// 	}
// #endif
}

void CKeyEntry::DirtyDeletePublishData()
{
	// instead of deleting our publishers properly in the destructor with decreasing the count in the global map 
	// we just remove them, and trust that the caller in the end also resets the global map, so the
	// kad shutdown is speed up a bit
	delete m_publishingIPs;
	m_publishingIPs = NULL;
}

void CKeyEntry::WriteTagListWithPublishInfo(CFileDataIO* data)
{
	if (m_publishingIPs == NULL || m_publishingIPs->size() == 0) {
		wxFAIL;
		WriteTagList(data);
		return;
	}

	// here we add a tag including how many publishers this entry has, the trustvalue and how many different names are known
	// this is supposed to get used in later versions as an indicator for the user how valid this result is (of course this tag
	// alone cannot be trusted 100%, because we could be a bad node, but it's a part of the puzzle)

	WriteTagListInc(data, 1); // write the standard taglist but increase the tagcount by one

	uint32_t trust = (uint16_t)(GetTrustValue() * 100);
	uint32_t publishers = m_publishingIPs->size() & 0xFF /*% 256*/;
	uint32_t names = m_filenames.size() & 0xFF /*% 256*/;
	// 32 bit tag: <namecount uint8><publishers uint8><trustvalue*100 uint16>
	uint32_t tagValue = (names << 24) | (publishers << 16) | trust;
	data->WriteTag(CTagVarInt(TAG_PUBLISHINFO, tagValue));
}
