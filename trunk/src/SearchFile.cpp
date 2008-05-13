//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <tags/FileTags.h>

#include "MemFile.h"			// Needed for CMemFile
#include "Preferences.h"		// Needed for thePrefs
#include "GuiEvents.h"
#include "Logger.h"

CSearchFile::CSearchFile(const CMemFile& data, bool optUTF8, wxUIntPtr searchID, uint32 WXUNUSED(serverIP), uint16 WXUNUSED(serverPort), const wxString& directory, bool kademlia)
	: m_parent(NULL),
	  m_showChildren(false),
	  m_searchID(searchID),
	  m_sourceCount(0),
	  m_completeSourceCount(0),
	  m_kademlia(kademlia),
	  m_directory(directory),
	  m_kadPublishInfo(0)
{
	m_abyFileHash = data.ReadHash();
	m_clientID = data.ReadUInt32();
	m_clientPort = data.ReadUInt16();

	if ((!m_clientID) || (!m_clientPort)) {
		m_clientID = m_clientPort = 0;
	} else if (!IsGoodIP(m_clientID, thePrefs::FilterLanIPs())) {
		m_clientID = m_clientPort = 0;
	}
	
	
	uint32 tagcount = data.ReadUInt32();
	for (unsigned int i = 0; i < tagcount; ++i) {
		CTag tag(data, optUTF8);
		switch (tag.GetNameID()) {
			case FT_FILENAME:
				SetFileName(CPath(tag.GetStr()));
				break;
			case FT_FILESIZE:
				SetFileSize(tag.GetInt());
				break;
			case FT_FILESIZE_HI:
				SetFileSize((((uint64)tag.GetInt()) << 32) + GetFileSize());
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

	if (!GetFileName().IsOk()) {
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
	  m_directory(other.m_directory),
	  m_kadPublishInfo(other.m_kadPublishInfo)
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


void CSearchFile::MergeResults(const CSearchFile& other)
{
	// Sources
	if (m_kademlia) {
		m_sourceCount = std::max(m_sourceCount, other.m_sourceCount);
		m_completeSourceCount = std::max(m_completeSourceCount, other.m_completeSourceCount);
	} else {
		m_sourceCount += other.m_sourceCount;
		m_completeSourceCount += other.m_completeSourceCount;
	}

	// Publish info
	if (m_kadPublishInfo == 0) {
		if (other.m_kadPublishInfo != 0) {
			m_kadPublishInfo = other.m_kadPublishInfo;
		}
	} else {
		if (other.m_kadPublishInfo != 0) {
			m_kadPublishInfo =
				std::max(m_kadPublishInfo & 0xFF000000, other.m_kadPublishInfo & 0xFF000000) |
				std::max(m_kadPublishInfo & 0x00FF0000, other.m_kadPublishInfo & 0x00FF0000) |
				(((m_kadPublishInfo & 0x0000FFFF) + (other.m_kadPublishInfo & 0x0000FFFF)) >> 1);
		}
	}

	// Rating
	if (m_iUserRating != 0) {
		if (other.m_iUserRating != 0) {
			m_iUserRating = (m_iUserRating + other.m_iUserRating) / 2;
		}
	} else {
		if (other.m_iUserRating != 0) {
			m_iUserRating = other.m_iUserRating;
		}
	}
}


void CSearchFile::AddChild(CSearchFile* file)
{
	wxCHECK_RET(file, wxT("Not a valid child!"));
	wxCHECK_RET(!file->GetParent(), wxT("Search-result can only be child of one other result"));
	wxCHECK_RET(!file->HasChildren(), wxT("Result already has children, cannot become child."));
	wxCHECK_RET(!GetParent(), wxT("A child cannot have children of its own"));
	wxCHECK_RET(GetFileHash() == file->GetFileHash(), wxT("Mismatching child/parent hashes"));
	wxCHECK_RET(GetFileSize() == file->GetFileSize(), wxT("Mismatching child/parent sizes"));
	
	// If no children exists, then we add the current item.
	if (GetChildren().empty()) {
		// Merging duplicate names instead of adding a new one
		if (file->GetFileName() == GetFileName()) {
			AddDebugLogLineM( false, logSearch, CFormat(wxT("Merged results for '%s'")) % GetFileName());
			MergeResults(*file);
			delete file;
			return;
		} else {
			// The first child will always be the first result we received.
			AddDebugLogLineM(false, logSearch, CFormat(wxT("Created initial child for result '%s'")) % GetFileName());
			m_children.push_back(new CSearchFile(*this));
		}
	}

	file->m_parent = this;

	for (size_t i = 0; i < m_children.size(); ++i) {
		CSearchFile* other = m_children.at(i);
		// Merge duplicate filenames
		if (other->GetFileName() == file->GetFileName()) {
			other->MergeResults(*file);
			UpdateParent();
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
	wxCHECK_RET(!m_parent, wxT("UpdateParent called on child item"));

	uint32_t sourceCount = 0;		// ed2k: sum of all sources, kad: the max sources found
	uint32_t completeSourceCount = 0;	// ed2k: sum of all sources, kad: the max sources found
	uint32_t differentNames = 0;		// max known different names
	uint32_t publishersKnown = 0;		// max publishers known
	uint32_t trustValue = 0;		// average trust value
	unsigned publishInfoTags = 0;
	unsigned ratingCount = 0;
	unsigned ratingTotal = 0;
	CSearchResultList::const_iterator best = m_children.begin();
	for (CSearchResultList::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		const CSearchFile* child = *it;

		// Locate the most common name
		if (child->GetSourceCount() > (*best)->GetSourceCount()) {
			best = it;
		}

		// Sources
		if (m_kademlia) {
			sourceCount = std::max(sourceCount, child->m_sourceCount);
			completeSourceCount = std::max(completeSourceCount, child->m_completeSourceCount);
		} else {
			sourceCount += child->m_sourceCount;
			completeSourceCount += child->m_completeSourceCount;
		}

		// Publish info
		if (child->GetKadPublishInfo() != 0) {
			differentNames = std::max(differentNames, (child->GetKadPublishInfo() & 0xFF000000) >> 24);
			publishersKnown = std::max(publishersKnown, (child->GetKadPublishInfo() & 0x00FF0000) >> 16);
			trustValue += child->GetKadPublishInfo() & 0x0000FFFF;
			publishInfoTags++;
		}

		// Rating
		if (child->HasRating()) {
			ratingCount++;
			ratingTotal += child->UserRating();
		}
	}

	m_sourceCount = sourceCount;
	m_completeSourceCount = completeSourceCount;

	if (publishInfoTags > 0) {
		m_kadPublishInfo = ((differentNames & 0x000000FF) << 24) | ((publishersKnown & 0x000000FF) << 16) | ((trustValue / publishInfoTags) & 0x0000FFFF);
	} else {
		m_kadPublishInfo = 0;
	}

	if (ratingCount > 0) {
		m_iUserRating = ratingTotal / ratingCount;
	} else {
		m_iUserRating = 0;
	}

	SetFileName((*best)->GetFileName());
}
// File_checked_for_headers