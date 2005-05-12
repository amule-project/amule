//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
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

//#include "stdafx.h"
//#include "../utils/MiscUtils.h"
#include "Indexed.h"
#include "Kademlia.h"
#include "../../OPCodes.h"
#include "Defines.h"
#include "Prefs.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/UInt128.h"
#include "../../OtherFunctions.h"
#include "../kademlia/Tag.h"
#include "../io/FileIO.h"
#include "../io/IOException.h"
#include "amule.h"
#include "Preferences.h"
#include "Logger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

wxString CIndexed::m_kfilename;
wxString CIndexed::m_sfilename;
wxString CIndexed::m_loadfilename;

CIndexed::CIndexed()
{
	m_sfilename = theApp.ConfigDir + wxT("src_index.dat");
	m_kfilename = theApp.ConfigDir + wxT("key_index.dat");
	m_loadfilename = theApp.ConfigDir + wxT("load_index.dat");
	m_lastClean = time(NULL) + (60*30);
	m_totalIndexSource = 0;
	m_totalIndexKeyword = 0;
	readFile();
}

void CIndexed::readFile(void)
{
	try {

		uint32 totalLoad = 0;
		uint32 totalSource = 0;
		uint32 totalKeyword = 0;
		uint32 numKeys = 0;
		uint32 numSource = 0;
		uint32 numName = 0;
		uint32 numLoad = 0;
		uint32 tagList = 0;

		CBufferedFileIO load_file;
		if(load_file.Open(m_loadfilename, CFile::read)) {
			uint32 version = load_file.readUInt32();
			if(version<2) {
				time_t savetime = load_file.readUInt32();
				numLoad = load_file.readUInt32();
				while(numLoad) {
					CUInt128 keyID;
					load_file.readUInt128(&keyID);
					if(AddLoad(keyID, load_file.readUInt32())) {
						totalLoad++;
					}
					numLoad--;
				}
			}
			load_file.Close();
		}
	
		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::read)) {

			uint32 version = k_file.readUInt32();
			if( version < 2 ) {
				time_t savetime = k_file.readUInt32();
				if( savetime > time(NULL) ) {
					CUInt128 id;

					k_file.readUInt128(&id);
					if( !Kademlia::CKademlia::getPrefs()->getKadID().compareTo(id) ) {
						numKeys = k_file.readUInt32();
						while( numKeys ) {
							CUInt128 keyID;
							k_file.readUInt128(&keyID);
							numSource = k_file.readUInt32();
							while( numSource ) {
								CUInt128 sourceID;
								k_file.readUInt128(&sourceID);
								numName = k_file.readUInt32();
								while( numName ) {
									Kademlia::CEntry* toaddN = new Kademlia::CEntry();
									toaddN->source = false;
									uint32 expire = k_file.readUInt32();
									toaddN->lifetime = expire;
									tagList = k_file.readByte();
									while( tagList ) {
										CTag* tag = k_file.readTag();
										if(tag) {
											if (!tag->m_name.Compare(TAG_FILENAME)) {
												toaddN->fileName = tag->GetStr();
												KadTagStrMakeLower(toaddN->fileName); // Make lowercase, the search code expects lower case strings!
												// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
												toaddN->taglist.push_back(tag);
											} else if (!tag->m_name.Compare(TAG_FILESIZE)) {
												toaddN->size = tag->GetInt();
												// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
												toaddN->taglist.push_back(tag);
											} else if (!tag->m_name.Compare(TAG_SOURCEIP)) {
												toaddN->ip = tag->GetInt();
												toaddN->taglist.push_back(tag);
											} else if (!tag->m_name.Compare(TAG_SOURCEPORT)) {
												toaddN->tcpport = tag->GetInt();
												toaddN->taglist.push_back(tag);
											} else if (!tag->m_name.Compare(TAG_SOURCEUPORT)) {
												toaddN->udpport = tag->GetInt();
												toaddN->taglist.push_back(tag);
											} else {
												toaddN->taglist.push_back(tag);
											}
										}
										tagList--;
									}
									toaddN->keyID.setValue(keyID);
									toaddN->sourceID.setValue(sourceID);
									uint8 load = 0;
									if(AddKeyword(keyID, sourceID, toaddN, load)) {
										totalKeyword++;
									} else {
										delete toaddN;
									}
									numName--;
								}
								numSource--;
							}
							numKeys--;
						}
					}
				}
			}
			k_file.Close();
		}

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::read)) {

			uint32 version = s_file.readUInt32();
			if( version < 2 ) {
				time_t savetime = s_file.readUInt32();
				if( savetime > time(NULL) ) {
					numKeys = s_file.readUInt32();
					CUInt128 id;
					while( numKeys ) {
						CUInt128 keyID;
						s_file.readUInt128(&keyID);
						numSource = s_file.readUInt32();
						while( numSource ) {
							CUInt128 sourceID;
							s_file.readUInt128(&sourceID);
							numName = s_file.readUInt32();
							while( numName ) {
								Kademlia::CEntry* toaddN = new Kademlia::CEntry();
								toaddN->source = true;
								uint32 test = s_file.readUInt32();
								toaddN->lifetime = test;
								tagList = s_file.readByte();
								while( tagList ) {
									CTag* tag = s_file.readTag();
									if(tag) {
										if (!tag->m_name.Compare(TAG_SOURCEIP))
										{
											toaddN->ip = tag->GetInt();
											toaddN->taglist.push_back(tag);
										} else if (!tag->m_name.Compare(TAG_SOURCEPORT)) {
											toaddN->tcpport = tag->GetInt();
											toaddN->taglist.push_back(tag);
										} else if (!tag->m_name.Compare(TAG_SOURCEUPORT)) {
											toaddN->udpport = tag->GetInt();
											toaddN->taglist.push_back(tag);
										} else {
											toaddN->taglist.push_back(tag);
										}
									}
									tagList--;
								}
								toaddN->keyID.setValue(keyID);
								toaddN->sourceID.setValue(sourceID);
								uint8 load = 0;
								if(AddSources(keyID, sourceID, toaddN, load)) {
									totalSource++;
								} else {
									delete toaddN;
								}
								numName--;
							}
							numSource--;
						}
						numKeys--;
					}
				}
			}
			s_file.Close();

			m_totalIndexSource = totalSource;
			m_totalIndexKeyword = totalKeyword;
			AddDebugLogLineM( false, logKadIndex, wxString::Format(wxT("Read %u source, %u keyword, and %u load entries"),totalSource,totalKeyword,totalLoad));
		}
	} catch ( CIOException *ioe ) {
		AddDebugLogLineM( false, logKadIndex, wxString::Format(wxT("Exception in CIndexed::readFile (IO error(%i))"),ioe->m_cause));
		ioe->Delete();
	} catch (...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::readFile"));
	}
}

CIndexed::~CIndexed()
{
	try
	{
		uint32 s_total = 0;
		uint32 k_total = 0;
		uint32 l_total = 0;

		CBufferedFileIO load_file;
		if(load_file.Open(m_loadfilename, CFile::write)) {
			uint32 version = 1;
			load_file.writeUInt32(version);
			load_file.writeUInt32(time(NULL));
			CCKey key;
			load_file.writeUInt32(m_Load_map.size());
			LoadMap::iterator it = m_Load_map.begin();
			for ( ; it != m_Load_map.end(); ++it ) {
				Load* load = it->second;
				load_file.writeUInt128(load->keyID);
				load_file.writeUInt32(load->time);
				l_total++;
				delete load;
			}
			load_file.Close();
		}

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::write)) {

			uint32 version = 1;
			s_file.writeUInt32(version);

			s_file.writeUInt32(time(NULL)+KADEMLIAREPUBLISHTIMES);

			CCKey key;
			CCKey key2;

			s_file.writeUInt32(m_Sources_map.size());
			SrcHashMap::iterator it = m_Sources_map.begin();
			for ( ; it != m_Sources_map.end(); ++it ) {
				SrcHash* currSrcHash = it->second;
				s_file.writeUInt128(currSrcHash->keyID);

				CKadSourcePtrList& KeyHashSrcMap = currSrcHash->m_Source_map;
				s_file.writeUInt32(KeyHashSrcMap.GetCount());
				POSITION pos2 = KeyHashSrcMap.GetHeadPosition();
				while( pos2 != NULL )
				{
					Source* currSource = KeyHashSrcMap.GetNext(pos2);
					s_file.writeUInt128(currSource->sourceID);

					CKadEntryPtrList& SrcEntryList = currSource->entryList;
					s_file.writeUInt32(SrcEntryList.GetCount());
					for(POSITION pos5 = SrcEntryList.GetHeadPosition(); pos5 != NULL; )
					{
						Kademlia::CEntry* currName = SrcEntryList.GetNext(pos5);
						s_file.writeUInt32(currName->lifetime);
						s_file.writeTagList(currName->taglist);
						delete currName;
						s_total++;
					}
					delete currSource;
				}
				delete currSrcHash;
			}
			s_file.Close();
		}

		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::write)) {

			CCKey key;
			CCKey key2;

			uint32 version = 1;
			k_file.writeUInt32(version);

			k_file.writeUInt32(time(NULL)+KADEMLIAREPUBLISHTIMEK);

			k_file.writeUInt128(Kademlia::CKademlia::getPrefs()->getKadID());

			k_file.writeUInt32(m_Keyword_map.size());
			KeyHashMap::iterator it = m_Keyword_map.begin();
			for ( ; it != m_Keyword_map.end(); ++it ) {
				KeyHash* currKeyHash = it->second;
				k_file.writeUInt128(currKeyHash->keyID);

				CSourceKeyMap& KeyHashSrcMap = currKeyHash->m_Source_map;
				k_file.writeUInt32(KeyHashSrcMap.size());
				
				CSourceKeyMap::iterator it2 = KeyHashSrcMap.begin();
				for ( ; it2 != KeyHashSrcMap.end(); ++it2 ) {
					Source* currSource = it2->second;
					k_file.writeUInt128(currSource->sourceID);
				
					CKadEntryPtrList& SrcEntryList = currSource->entryList;
					k_file.writeUInt32(SrcEntryList.GetCount());
					for(POSITION pos5 = SrcEntryList.GetHeadPosition(); pos5 != NULL; ) {
						Kademlia::CEntry* currName = SrcEntryList.GetNext(pos5);
						k_file.writeUInt32(currName->lifetime);
						k_file.writeTagList(currName->taglist);
						delete currName;
						k_total++;
					}
					delete currSource;
				}
				delete currKeyHash;
			}
			k_file.Close();
		}
		AddDebugLogLineM( false, logKadIndex, wxString::Format(wxT("Wrote %u source, %u keyword, and %u load entries"), s_total, k_total, l_total));

		CCKey key;
		CCKey key2;
		SrcHashMap::iterator it = m_Notes_map.begin();
		for ( ; it != m_Notes_map.end(); ++it) {
			SrcHash* currNoteHash = it->second;
			CKadSourcePtrList& KeyHashNoteMap = currNoteHash->m_Source_map;
			POSITION pos2 = KeyHashNoteMap.GetHeadPosition();
			while( pos2 != NULL )
			{
				Source* currNote = KeyHashNoteMap.GetNext(pos2);
				CKadEntryPtrList& NoteEntryList = currNote->entryList;
				for(POSITION pos5 = NoteEntryList.GetHeadPosition(); pos5 != NULL; ) {
					Kademlia::CEntry* currName = NoteEntryList.GetNext(pos5);
					delete currName;
				}
				delete currNote;
			}
			delete currNoteHash;
		} 
	} catch ( CIOException *ioe ) {
		AddDebugLogLineM( false, logKadIndex, wxString::Format(wxT("Exception in CIndexed::~CIndexed (IO error(%i))"), ioe->m_cause));
		ioe->Delete();
	} catch (...)  {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::~CIndexed"));
	}
}

void CIndexed::clean(void)
{
	try {
		if( m_lastClean > time(NULL) ) {
			return;
		}

		uint32 k_Removed = 0;
		uint32 s_Removed = 0;
		uint32 s_Total = 0;
		uint32 k_Total = 0;
		time_t tNow = time(NULL);

		KeyHashMap::iterator it = m_Keyword_map.begin();
		for ( ; it != m_Keyword_map.end(); ++it) {
			KeyHash* currKeyHash = it->second;
			CCKey key = it->first;
			
			CSourceKeyMap::iterator it2 = currKeyHash->m_Source_map.begin();
			for ( ; it2 != currKeyHash->m_Source_map.end(); ++it2) {
				Source* currSource = it2->second;
				CCKey key2 = it2->first;
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; ) {
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					k_Total++;
					if( !currName->source && currName->lifetime < tNow) {
						k_Removed++;
						currSource->entryList.RemoveAt(pos6);
						delete currName;
					}
				}
				if( currSource->entryList.IsEmpty()) {
					currKeyHash->m_Source_map.erase(key2);
					delete currSource;
				}
			}
			if( currKeyHash->m_Source_map.empty()) {
				m_Keyword_map.erase(key);
				delete currKeyHash;
			}
		}

		SrcHashMap::iterator it_src = m_Sources_map.begin();
		for ( ; it_src != m_Sources_map.end(); ++it_src) {
			SrcHash* currSrcHash = it_src->second;
			CCKey key = it_src->first;
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				POSITION pos3 = pos2;
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);			
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; ) {
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					s_Total++;
					if( currName->lifetime < tNow) {
						s_Removed++;
						currSource->entryList.RemoveAt(pos6);
						delete currName;
					}
				}
				if( currSource->entryList.IsEmpty()) {
					currSrcHash->m_Source_map.RemoveAt(pos3);
					delete currSource;
				}
			}
			if( currSrcHash->m_Source_map.IsEmpty()) {
				m_Sources_map.erase(key);
				delete currSrcHash;
			}
		}

		m_totalIndexSource = s_Total;
		m_totalIndexKeyword = k_Total;
		AddDebugLogLineM( false, logKadIndex, wxString::Format(wxT("Removed %u keyword out of %u and %u source out of %u"),  k_Removed, k_Total, s_Removed, s_Total));
		m_lastClean = time(NULL) + MIN2S(30);
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::clean"));
		wxASSERT(0);
	}
}

bool CIndexed::AddKeyword(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load){
	try {
		if( !entry ) {
			return false;
		}

		if( m_totalIndexKeyword > KADEMLIAMAXENTRIES ) {
			load = 100;
			return false;
		}

		if( entry->size == 0 || entry->fileName.IsEmpty() || entry->taglist.size() == 0 || entry->lifetime < time(NULL)) {
			return false;
		}

		KeyHashMap::iterator it = m_Keyword_map.find(CCKey(keyID.getData())); 
		KeyHash* currKeyHash = NULL;
		if(it == m_Keyword_map.end()) {
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currKeyHash = new KeyHash;
			currKeyHash->keyID.setValue(keyID);
			currKeyHash->m_Source_map[CCKey(currSource->sourceID.getData())] = currSource;
			m_Keyword_map[CCKey(currKeyHash->keyID.getData())] = currKeyHash;
			load = 1;
			m_totalIndexKeyword++;
			return true;
		} else {
			currKeyHash = it->second; 
			uint32 indexTotal = currKeyHash->m_Source_map.size();
			if ( indexTotal > KADEMLIAMAXINDEX ) {
				load = 100;
				//Too many entries for this Keyword..
				return false;
			}
			Source* currSource = NULL;
			CSourceKeyMap::iterator it2 = currKeyHash->m_Source_map.find(CCKey(sourceID.getData()));
			if(it2 != currKeyHash->m_Source_map.end()) {
				currSource = it2->second;
				if (currSource->entryList.GetCount() > 0) {
					if( indexTotal > KADEMLIAMAXINDEX - 5000 ) {
						load = 100;
						//We are in a hot node.. If we continued to update all the publishes
						//while this index is full, popular files will be the only thing you index.
						return false;
					}
					delete currSource->entryList.GetHead();
					currSource->entryList.RemoveHead();
				} else {
					m_totalIndexKeyword++;
				}
				load = (indexTotal*100)/KADEMLIAMAXINDEX;
				currSource->entryList.AddHead(entry);
				return true;
			} else {
				currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currKeyHash->m_Source_map[CCKey(currSource->sourceID.getData())] = currSource;
				m_totalIndexKeyword++;
				load = (indexTotal*100)/KADEMLIAMAXINDEX;
				return true;
			}
		}
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::AddKeyword"));
		wxASSERT(0);
	}
	return false;

}

bool CIndexed::AddSources(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load)
{
	if( !entry ) {
		return false;
	}
	if( entry->ip == 0 || entry->tcpport == 0 || entry->udpport == 0 || entry->taglist.size() == 0 || entry->lifetime < time(NULL)) {
		return false;
	}
	try {
		SrcHash* currSrcHash = NULL;
		SrcHashMap::iterator it = m_Sources_map.find(CCKey(keyID.getData()));
		if(it == m_Sources_map.end()) {
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currSrcHash = new SrcHash;
			currSrcHash->keyID.setValue(keyID);
			currSrcHash->m_Source_map.AddHead(currSource);
			m_Sources_map[CCKey(currSrcHash->keyID.getData())] =  currSrcHash;
			m_totalIndexSource++;
			load = 1;
			return true;
		} else {
			currSrcHash = it->second;
			uint32 size = currSrcHash->m_Source_map.GetSize();
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);
				if( currSource->entryList.GetSize() ) {
					CEntry* currEntry = currSource->entryList.GetHead();
					wxASSERT(currEntry!=NULL);
					if( currEntry->ip == entry->ip && ( currEntry->tcpport == entry->tcpport || currEntry->udpport == entry->udpport )) {
						CEntry* currName = currSource->entryList.RemoveHead();
						delete currName;
						currSource->entryList.AddHead(entry);
						load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
						return true;
					}
				} else {
					//This should never happen!
					currSource->entryList.AddHead(entry);
					wxASSERT(0);
					load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
					return true;
				}
			}
			if( size > KADEMLIAMAXSOUCEPERFILE ) {
				Source* currSource = currSrcHash->m_Source_map.RemoveTail();
				wxASSERT(currSource!=NULL);
				Kademlia::CEntry* currName = currSource->entryList.RemoveTail();
				wxASSERT(currName!=NULL);
				delete currName;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currSrcHash->m_Source_map.AddHead(currSource);
				load = 100;
				return true;
			} else {
				Source* currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currSrcHash->m_Source_map.AddHead(currSource);
				m_totalIndexSource++;
				load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
				return true;
			}
		}
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::AddSource"));
	}
	return false;
}

bool CIndexed::AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load)
{
	if( !entry ) {
		return false;
	}
	if( entry->ip == 0 || entry->taglist.size() == 0 ) {
		return false;
	}
	try {
		SrcHash* currNoteHash = NULL;
		SrcHashMap::iterator it = m_Notes_map.find(CCKey(keyID.getData()));
		if(it == m_Notes_map.end()) {
			Source* currNote = new Source;
			currNote->sourceID.setValue(sourceID);
			currNote->entryList.AddHead(entry);
			currNoteHash = new SrcHash;
			currNoteHash->keyID.setValue(keyID);
			currNoteHash->m_Source_map.AddHead(currNote);
			m_Notes_map[CCKey(currNoteHash->keyID.getData())] = currNoteHash;
			load = 1;
			return true;
		} else {
			currNoteHash = it->second;
			uint32 size = currNoteHash->m_Source_map.GetSize();
			for(POSITION pos2 = currNoteHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				Source* currNote = currNoteHash->m_Source_map.GetHead();			
				if( currNote->entryList.GetSize() ) {
					CEntry* currEntry = currNote->entryList.GetHead();
					wxASSERT(currEntry!=NULL);
					if(currEntry->ip == entry->ip || !currEntry->sourceID.compareTo(entry->sourceID)) {
						CEntry* currName = currNote->entryList.RemoveHead();
						delete currName;
						currNote->entryList.AddHead(entry);
						load = (size*100)/KADEMLIAMAXNOTESPERFILE;
						return true;
					}
				} else {
					//This should never happen!
					currNote->entryList.AddHead(entry);
					wxASSERT(0);
					load = (size*100)/KADEMLIAMAXNOTESPERFILE;
					return true;
				}
			}
			if( size > KADEMLIAMAXNOTESPERFILE ) {
				Source* currNote = currNoteHash->m_Source_map.RemoveTail();
				wxASSERT(currNote!=NULL);
				CEntry* currName = currNote->entryList.RemoveTail();
				wxASSERT(currName!=NULL);
				delete currName;
				currNote->sourceID.setValue(sourceID);
				currNote->entryList.AddHead(entry);
				currNoteHash->m_Source_map.AddHead(currNote);
				load = 100;
				return true;
			} else {
				Source* currNote = new Source;
				currNote->sourceID.setValue(sourceID);
				currNote->entryList.AddHead(entry);
				currNoteHash->m_Source_map.AddHead(currNote);
				load = (size*100)/KADEMLIAMAXNOTESPERFILE;
				return true;
			}
		}
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::AddNotes"));
	}
	return false;
}

bool CIndexed::AddLoad(const CUInt128& keyID, uint32 timet)
{
	Load* load = NULL;
	LoadMap::iterator it = m_Load_map.find(CCKey(keyID.getData()));
	if(it != m_Load_map.end())
	{
		wxASSERT(0);
		return false;
	}
	wxASSERT((uint32)time(NULL)<timet);
	load = new Load();
	load->keyID.setValue(keyID);
	load->time = timet;
	m_Load_map[CCKey(load->keyID.getData())] =  load;
	return true;
}

bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item/*, CStringArray& astrFileNameTokens*/)
{
	// boolean operators
	if (pSearchTerm->type == SSearchTerm::AND) {
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) && SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);
	}
	if (pSearchTerm->type == SSearchTerm::OR) {
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) || SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);
	}
	if (pSearchTerm->type == SSearchTerm::NAND) {
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) && !SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);
	}
	// word which is to be searched in the file name (and in additional meta data as done by some ed2k servers???)
	if (pSearchTerm->type == SSearchTerm::String) {
		int iStrSearchTerms = pSearchTerm->astr->GetCount();
		if (iStrSearchTerms == 0) {
			return false;
		}
#if 0
		//TODO: Use a pre-tokenized list for better performance.
		// tokenize the filename (very expensive) only once per search expression and only if really needed
		if (astrFileNameTokens.GetCount() == 0)
		{
			int iPosTok = 0;
			CString strTok(item->fileName.Tokenize(_aszInvKadKeywordChars, iPosTok));
			while (!strTok.IsEmpty())
			{
				astrFileNameTokens.Add(strTok);
				strTok = item->fileName.Tokenize(_aszInvKadKeywordChars, iPosTok);
			}
		}
		if (astrFileNameTokens.GetCount() == 0)
			return false;

		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		for (int iSearchTerm = 0; iSearchTerm < iStrSearchTerms; iSearchTerm++)
		{
			bool bFoundSearchTerm = false;
			for (int i = 0; i < astrFileNameTokens.GetCount(); i++)
			{
				// the file name string was already stored in lowercase
				// the string search term was already stored in lowercase
				if (strcmp(astrFileNameTokens.GetAt(i), pSearchTerm->astr->GetAt(iSearchTerm)) == 0)
				{
					bFoundSearchTerm = true;
					break;
				}
			}
			if (!bFoundSearchTerm)
				return false;
		}
#else
		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		for (int iSearchTerm = 0; iSearchTerm < iStrSearchTerms; iSearchTerm++) {
			// this will not give the same results as when tokenizing the filename string, but it is 20 times faster.
			if (item->fileName.Find((*(pSearchTerm->astr))[iSearchTerm]) == -1) {
				return false;
			}
		}
#endif
		return true;

		// search string value in all string meta tags (this includes also the filename)
		// although this would work, I am no longer sure if it's the correct way to process the search requests..
		/*const Kademlia::CTag *tag;
		TagList::const_iterator it;
		for (it = item->taglist.begin(); it != item->taglist.end(); it++)
		{
			tag = *it;
			if (tag->m_type == 2)
			{
				//TODO: Use a pre-tokenized list for better performance.
				int iPos = 0;
				CString strTok(static_cast<const CTagStr *>(tag)->m_value.Tokenize(_aszInvKadKeywordChars, iPos));
				while (!strTok.IsEmpty()){
					if (stricmp(strTok, *(pSearchTerm->str)) == 0)
						return true;
					strTok = static_cast<const CTagStr *>(tag)->m_value.Tokenize(_aszInvKadKeywordChars, iPos);
				}
			}
		}
		return false;*/
	}

	if (pSearchTerm->type == SSearchTerm::MetaTag) {
		if (pSearchTerm->tag->m_type == 2) { // meta tags with string values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsStr() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetStr().CmpNoCase(pSearchTerm->tag->GetStr()) == 0;
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpGreaterEqual) { 
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() >= pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() >= pSearchTerm->tag->GetFloat();
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpLessEqual) {
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() <= pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() <= pSearchTerm->tag->GetFloat();
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpGreater) {
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() > pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() > pSearchTerm->tag->GetFloat();
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpLess) {
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() < pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); ++it) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() < pSearchTerm->tag->GetFloat();
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpEqual) {
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() == pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() == pSearchTerm->tag->GetFloat();
				}
			}
		}
	} else if (pSearchTerm->type == SSearchTerm::OpNotEqual) {
		if (pSearchTerm->tag->IsInt()) { // meta tags with integer values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetInt() != pSearchTerm->tag->GetInt();
				}
			}
		} else if (pSearchTerm->tag->IsFloat()) { // meta tags with float values
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++) {
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0) {
					return tag->GetFloat() != pSearchTerm->tag->GetFloat();
				}
			}
		}
	}

	return false;
}

//bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item)
//{
//	// tokenize the filename (very expensive) only once per search expression and only if really needed
//	CStringArray astrFileNameTokens;
//	return SearchTermsMatch(pSearchTerm, item, astrFileNameTokens);
//}

void CIndexed::SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32 ip, uint16 port)
{
	try {
		KeyHash* currKeyHash = NULL;
		KeyHashMap::iterator it = m_Keyword_map.find(CCKey(keyID.getData()));
		if(it != m_Keyword_map.end()) {
			currKeyHash = it->second;
			byte packet[1024*50];
			CByteIO bio(packet,sizeof(packet));
			bio.writeByte(OP_KADEMLIAHEADER);
			bio.writeByte(KADEMLIA_SEARCH_RES);
			bio.writeUInt128(keyID);
			bio.writeUInt16(50);
			uint16 maxResults = 300;
			uint16 count = 0;
			CSourceKeyMap::iterator it2 = currKeyHash->m_Source_map.begin();
			for ( ; it2 != currKeyHash->m_Source_map.end(); ++it2) {
				Source* currSource =  it2->second;
				CCKey key2 = it2->first;
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; ) {
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					if ( !pSearchTerms || SearchTermsMatch(pSearchTerms, currName) ) {
						if( count < maxResults ) {
							bio.writeUInt128(currName->sourceID);
							bio.writeTagList(currName->taglist);
							count++;
							if( count % 50 == 0 ) {
								uint32 len = sizeof(packet)-bio.getAvailable();
								AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchRes ") + Uint32_16toStringIP_Port(ip, port));
								CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
								bio.reset();
								bio.writeByte(OP_KADEMLIAHEADER);
								bio.writeByte(KADEMLIA_SEARCH_RES);
								bio.writeUInt128(keyID);
								bio.writeUInt16(50);
							}
						}
					}
				}
			}
			uint16 ccount = count % 50;
			if( ccount ) {
				uint32 len = sizeof(packet)-bio.getAvailable();
				ENDIAN_SWAP_I_16(ccount);
				memcpy(packet+18, &ccount, 2);
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchRes ") + Uint32_16toStringIP_Port(ip, port));
				CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
			}
			clean();
		}
	} catch(...)  {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::SendValidKeywordResult"));
	}
}

void CIndexed::SendValidSourceResult(const CUInt128& keyID, uint32 ip, uint16 port)
{
	try {
		SrcHash* currSrcHash = NULL;
		SrcHashMap::iterator it = m_Sources_map.find(CCKey(keyID.getData()));
		if(it != m_Sources_map.end()) {
			currSrcHash = it->second;
			byte packet[1024*50];
			CByteIO bio(packet,sizeof(packet));
			bio.writeByte(OP_KADEMLIAHEADER);
			bio.writeByte(KADEMLIA_SEARCH_RES);
			bio.writeUInt128(keyID);
			bio.writeUInt16(50);
			uint16 maxResults = 300;
			uint16 count = 0;
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; ) {
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);	
				if( currSource->entryList.GetSize() ) {
					Kademlia::CEntry* currName = currSource->entryList.GetHead();
					if( count < maxResults ) {
						bio.writeUInt128(currName->sourceID);
						bio.writeTagList(currName->taglist);
						count++;
						if( count % 50 == 0 ) {
							uint32 len = sizeof(packet)-bio.getAvailable();
							AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchRes ") + Uint32_16toStringIP_Port(ip , port));
							CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
							bio.reset();
							bio.writeByte(OP_KADEMLIAHEADER);
							bio.writeByte(KADEMLIA_SEARCH_RES);
							bio.writeUInt128(keyID);
							bio.writeUInt16(50);
						}
					}
				}
			}
			uint16 ccount = count % 50;
			if( ccount ) {
				ENDIAN_SWAP_I_16(ccount);
				uint32 len = sizeof(packet)-bio.getAvailable();
				memcpy(packet+18, &ccount, 2);
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadSearchRes ") + Uint32_16toStringIP_Port(ip, port));
				CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
			}
			clean();
		}
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::SendValidSourceResult"));
	}
}

void CIndexed::SendValidNoteResult(const CUInt128& keyID, const CUInt128& sourceID, uint32 ip, uint16 port)
{
	try {
		SrcHash* currNoteHash = NULL;
		SrcHashMap::iterator it = m_Notes_map.find(CCKey(keyID.getData()));
		if(it != m_Notes_map.end()) {
			currNoteHash = it->second;		
			byte packet[1024*50];
			CByteIO bio(packet,sizeof(packet));
			bio.writeByte(OP_KADEMLIAHEADER);
			bio.writeByte(KADEMLIA_SRC_NOTES_RES);
			bio.writeUInt128(keyID);
			bio.writeUInt16(50);
			uint16 maxResults = 50;
			uint16 count = 0;
			for(POSITION pos2 = currNoteHash->m_Source_map.GetHeadPosition(); pos2 != NULL; ) {
				Source* currNote = currNoteHash->m_Source_map.GetNext(pos2);
				if( currNote->entryList.GetSize() ) {
					Kademlia::CEntry* currName = currNote->entryList.GetHead();
					if( count < maxResults ) {
						bio.writeUInt128(currName->sourceID);
						bio.writeTagList(currName->taglist);
					}
					if( count % 50 == 0 ) {
						uint32 len = sizeof(packet)-bio.getAvailable();
						AddDebugLogLineM(false, logClientKadUDP, wxT("KadNotesRes ") + Uint32_16toStringIP_Port(ip, port));
						CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
						bio.reset();
						bio.writeByte(OP_KADEMLIAHEADER);
						bio.writeByte(KADEMLIA_SRC_NOTES_RES);
						bio.writeUInt128(keyID);
						bio.writeUInt16(50);
					}
				}
			}
			uint16 ccount = count % 50;
			if( ccount ) {
				ENDIAN_SWAP_I_16(ccount);
				uint32 len = sizeof(packet)-bio.getAvailable();
				memcpy(packet+18, &ccount, 2);
				AddDebugLogLineM(false, logClientKadUDP, wxT("KadNotesRes ") + Uint32_16toStringIP_Port(ip, port));
				CKademlia::getUDPListener()->sendPacket(packet, len, ip, port);
			}
			//clean(); //Not needed at the moment.
		}
	} catch(...) {
		AddDebugLogLineM(false, logKadIndex, wxT("Exception in CIndexed::SendValidSourceResult"));
	}
}

bool CIndexed::SendStoreRequest(const CUInt128& keyID)
{
	Load* load = NULL;
	LoadMap::iterator it = m_Load_map.find(CCKey(keyID.getData()));
	if(it != m_Load_map.end()) {
		load = it->second;
		if(load->time < (uint32)time(NULL)) {
			m_Load_map.erase(it);
			delete load;
			return true;
		}
		return false;
	}
	return true;
}

SSearchTerm::SSearchTerm()
{
	type = AND;
	tag = NULL;
	left = NULL;
	right = NULL;
}

SSearchTerm::~SSearchTerm()
{
	if (type == String) {
		delete astr;
	}
	delete tag;
}
