//
// This file is part of the aMule Project.
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

#include "SearchFile.h"			// Interface declarations.
#include "NetworkFunctions.h" 	// Needed for IsGoodIP
#include "MemFile.h"			// Needed for CMemFile
#include "Preferences.h"		// Needed for thePrefs
#include "Tag.h"				// Needed for CTag
#include "GuiEvents.h"
#include "amule.h"				// Needed for theApp

#include <algorithm>			// Needed for std::max
#include <memory>				// Needed for std::auto_ptr




CSearchFile::CSearchFile(const CMemFile& data, bool optUTF8, long searchID, uint32 WXUNUSED(serverIP), uint16 WXUNUSED(serverPort), const wxString& directory, bool kademlia)
	: m_parent(NULL),
	  m_showChildren(false),
	  m_searchID(searchID),
	  m_sourceCount(0),
	  m_completeSourceCount(0),
	  m_kademlia(kademlia),
	  m_directory(directory)	
{
	m_abyFileHash = data.ReadHash();
	m_clientID = data.ReadUInt32();
	m_clientPort = data.ReadUInt16();

	if ((not m_clientID) or (not m_clientPort)) {
		m_clientID = m_clientPort = 0;
	} else if (not IsGoodIP(m_clientID, thePrefs::FilterLanIPs())) {
		m_clientID = m_clientPort = 0;
	}
	
	
	uint32 tagcount = data.ReadUInt32();
	for (unsigned int i = 0; i < tagcount; ++i) {
		CTag tag(data, optUTF8);
		switch (tag.GetNameID()) {
			case FT_FILENAME:
				SetFileName(tag.GetStr());
				break;
			case FT_FILESIZE:
				SetFileSize(tag.GetInt());
				break;
			case FT_FILESIZE_HI:
				SetFileSize( (((uint64)tag.GetInt()) << 32) + GetFileSize());
				break;				
			case FT_FILERATING:
				m_iUserRating = (tag.GetInt() & 0xF) / 3;
				break;
			case FT_SOURCES:
				m_sourceCount = tag.GetInt();
				break;
			case FT_COMPLETE_SOURCES:
				m_completeSourceCount = tag.GetInt();
				break;
			default:
				AddTagUnique(tag);
		}
	}

	if (GetFileName().IsEmpty()) {
		throw CInvalidPacket(wxT("No filename in search result"));
	}
}


CSearchFile::CSearchFile(const CSearchFile& other)
	: CAbstractFile(other),
	  m_parent(other.m_parent),
	  m_showChildren(other.m_showChildren),
	  m_searchID(other.m_searchID),
	  m_sourceCount(other.m_sourceCount),
	  m_completeSourceCount(other.m_completeSourceCount),
	  m_kademlia(other.m_kademlia),
	  m_clientID(other.m_clientID),
	  m_clientPort(other.m_clientPort),
	  m_directory(other.m_directory)
{
	for (size_t i = 0; i < other.m_children.size(); ++i) {
		m_children.push_back(new CSearchFile(*other.m_children.at(i)));
	}
}


CSearchFile::~CSearchFile()
{	
	for (size_t i = 0; i < m_children.size(); ++i) {
		delete m_children.at(i);
	}
}


void CSearchFile::AddSources(uint32 count, uint32 count_complete)
{
	if (m_kademlia) {
		m_sourceCount = std::max(m_sourceCount, count);
		m_completeSourceCount = std::max(m_completeSourceCount, count_complete);
	} else {
		m_sourceCount += count;
		m_completeSourceCount += count_complete;
	}
}


void CSearchFile::AddChild(CSearchFile* file)
{
	wxCHECK_RET(file, wxT("Not a valid child!"));
	wxCHECK_RET(not file->GetParent(), wxT("Search-result can only be child of one other result"));
	wxCHECK_RET(not file->HasChildren(), wxT("Result already has children, cannot become child."));
	wxCHECK_RET(not GetParent(), wxT("A child cannot have children of its own"));
	wxCHECK_RET(GetFileHash() == file->GetFileHash(), wxT("Mismatching child/parent hashes"));
	wxCHECK_RET(GetFileSize() == file->GetFileSize(), wxT("Mismatching child/parent sizes"));
	
	file->m_parent = this;

	// TODO: Doesn't handle results with same name but diff. rating.
	for (size_t i = 0; i < m_children.size(); ++i) {
		CSearchFile* other = m_children.at(i);
		
		if (other->GetFileName() == file->GetFileName()) {
			other->AddSources(file->GetSourceCount(), file->GetCompleteSourceCount());
			UpdateParent();
			Notify_Search_Update_Sources(other);
			delete file;
			return;
		}
	}

	// New unique child.	
	m_children.push_back(file);
	UpdateParent();
	
	if (ShowChildren()) {
		Notify_Search_Add_Result(file);
	}
}


void CSearchFile::UpdateParent()
{
	wxCHECK_RET(not m_parent, wxT("UpdateParent called on child item"));
	
	size_t ratingCount = 0, ratingTotal = 0;
	size_t max = 0, index = 0;
	for (size_t i = 0; i < m_children.size(); ++i) {
		const CSearchFile* child = m_children.at(index);
		
		// Locate the most common name
		if (child->GetSourceCount() > max) {
			max = child->GetSourceCount();
			index = i;
		}

		// Create sum of ratings so that the parent contains the avg.
		if (child->HasRating()) {
			ratingCount += 1;
			ratingTotal += child->UserRating();
		}
	}

	if (ratingCount) {
		m_iUserRating = (ratingTotal / ratingCount);
	}

	SetFileName(m_children.at(index)->GetFileName());
}
