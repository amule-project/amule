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

#include "SearchFile.h"			// Interface declarations.

#include <tags/FileTags.h>

#include "amule.h"				// Needed for theApp
#include "CanceledFileList.h"
#include "MemFile.h"			// Needed for CMemFile
#include "Preferences.h"		// Needed for thePrefs
#include "GuiEvents.h"
#include "Logger.h"
#include "PartFile.h"			// Needed for CPartFile::CanAddSource
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "KnownFileList.h"		// Needed for CKnownFileList

CSearchFile::CSearchFile(const CMemFile& data, bool optUTF8, wxUIntPtr searchID, uint32_t serverIP, uint16_t serverPort, const wxString& directory, bool kademlia)
	: m_parent(NULL),
	  m_showChildren(false),
	  m_searchID(searchID),
	  m_sourceCount(0),
	  m_completeSourceCount(0),
	  m_kademlia(kademlia),
	  m_downloadStatus(NEW),
	  m_directory(directory),
	  m_clientServerIP(serverIP),
	  m_clientServerPort(serverPort),
	  m_kadPublishInfo(0)
{
	// Validate we have enough data to read the FileID (16 bytes)
	// This prevents corrupted hashes from truncated packets
	if (data.GetLength() - data.GetPosition() < 16) {
		throw CInvalidPacket(wxT("Search result packet too short to read FileID"));
	}

	m_abyFileHash = data.ReadHash();

	// Validate the FileID is not corrupted
	// Truncated packets can result in hashes with zeros in first/last half
	if (m_abyFileHash.IsCorrupted()) {
		throw CInvalidPacket(CFormat(wxT("Corrupted FileID in search result: %s"))
			% m_abyFileHash.Encode());
	}
	SetDownloadStatus();
	m_clientID = data.ReadUInt32();
	m_clientPort = data.ReadUInt16();

	if (!m_clientID || !m_clientPort || !IsGoodIP(m_clientID, thePrefs::FilterLanIPs())) {
		m_clientID = 0;
		m_clientPort = 0;
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
			case FT_PERMISSIONS:
			case FT_KADLASTPUBLISHKEY:
			case FT_PARTFILENAME:
				// Just ignore
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
	  CECID(),	// create a new ID for now
	  m_parent(other.m_parent),
	  m_showChildren(other.m_showChildren),
	  m_searchID(other.m_searchID),
	  m_sourceCount(other.m_sourceCount),
	  m_completeSourceCount(other.m_completeSourceCount),
	  m_kademlia(other.m_kademlia),
	  m_downloadStatus(other.m_downloadStatus),
	  m_directory(other.m_directory),
	  m_clients(other.m_clients),
	  m_clientID(other.m_clientID),
	  m_clientPort(other.m_clientPort),
	  m_clientServerIP(other.m_clientServerIP),
	  m_clientServerPort(other.m_clientServerPort),
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


void CSearchFile::AddClient(const ClientStruct& client)
{
	for (std::list<ClientStruct>::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
		if (client.m_ip == it->m_ip && client.m_port == it->m_port) return;
	}
	m_clients.push_back(client);
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

	// copy possible available sources from new result
	if (other.GetClientID() && other.GetClientPort()) {
		// pre-filter sources which would be dropped by CPartFile::AddSources
		if (CPartFile::CanAddSource(other.GetClientID(), other.GetClientPort(), other.GetClientServerIP(), other.GetClientServerPort())) {
			CSearchFile::ClientStruct client(other.GetClientID(), other.GetClientPort(), other.GetClientServerIP(), other.GetClientServerPort());
			AddClient(client);
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
			AddDebugLogLineN(logSearch, CFormat(wxT("Merged results for '%s'")) % GetFileName());
			MergeResults(*file);
			delete file;
			return;
		} else {
			// The first child will always be the first result we received.
			AddDebugLogLineN(logSearch, CFormat(wxT("Created initial child for result '%s'")) % GetFileName());
			m_children.push_back(new CSearchFile(*this));
			m_children.back()->m_parent = this;
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


// Helper function to detect if a filename has mojibake (corrupted characters)
static bool HasMojibake(const CPath& filename)
{
	wxString name = filename.GetPrintable();
	
	// Check for common mojibake patterns
	// The 啐 character (U+5550) is a common sign of UTF-8 encoding corruption
	if (name.Find(wxT("啐")) != wxNOT_FOUND) {
		return true;
	}
	
	// Check for other common corrupted characters
	// These are replacement characters that appear when UTF-8 is incorrectly decoded
	if (name.Find(wxT("")) != wxNOT_FOUND) {
		return true;
	}
	
	// Check for sequences of characters that look like incorrectly decoded UTF-8
	// Multi-byte UTF-8 sequences decoded as ISO-8859-1 often produce these patterns
	for (size_t i = 0; i < name.length(); ++i) {
		wxChar c = name[i];
		// Check for characters in the range that commonly appear in mojibake
		// These are continuation bytes or start bytes that weren't properly handled
		if ((c >= 0x80 && c <= 0x9F) || (c >= 0xC0 && c <= 0xFF)) {
			// This might be a corrupted UTF-8 byte
			// Check if it's followed by more suspicious characters
			if (i + 1 < name.length()) {
				wxChar next = name[i + 1];
				if ((next >= 0x80 && next <= 0x9F) || (next >= 0xC0 && next <= 0xFF)) {
					return true;
				}
			}
		}
	}
	
	return false;
}

// Helper function to score a filename for quality (higher is better)
static int ScoreFilename(const CPath& filename, uint32_t sourceCount)
{
	wxString name = filename.GetPrintable();
	int score = 0;
	
	// Penalty for mojibake (severe penalty)
	if (HasMojibake(filename)) {
		score -= 1000;
	}
	
	// Bonus for longer filenames (more descriptive)
	if (name.length() > 20) {
		score += (name.length() - 20) / 5;  // +1 for every 5 chars over 20
	}
	
	// Bonus for source count (more sources = more popular)
	score += sourceCount / 10;  // +1 for every 10 sources
	
	// Bonus for having year (e.g., 2024, 2023, etc.)
	if (name.Contains(wxT("202")) || name.Contains(wxT("201")) || name.Contains(wxT("200"))) {
		score += 5;
	}
	
	// Bonus for quality indicators
	if (name.Contains(wxT("1080p")) || name.Contains(wxT("720p")) || 
	    name.Contains(wxT("BluRay")) || name.Contains(wxT("HDR")) ||
	    name.Contains(wxT("4K")) || name.Contains(wxT("UHD"))) {
		score += 3;
	}
	
	// Bonus for proper spacing (has spaces between words)
	int spaceCount = 0;
	for (size_t i = 0; i < name.length(); ++i) {
		if (name[i] == wxT(' ')) {
			spaceCount++;
		}
	}
	if (spaceCount > 2) {
		score += spaceCount;  // +1 for each space (max reasonable benefit)
	}
	
	// Penalty for excessive punctuation (might indicate poor formatting)
	int punctCount = 0;
	for (size_t i = 0; i < name.length(); ++i) {
		wxChar c = name[i];
		if (c == wxT('.') || c == wxT('_') || c == wxT('-')) {
			punctCount++;
		}
	}
	if (punctCount > 10) {
		score -= (punctCount - 10);  // Penalty for too much punctuation
	}
	
	return score;
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

		// Score each filename based on quality criteria
		// Prefer filenames that are: clean, descriptive, popular, well-formatted
		int bestScore = ScoreFilename((*best)->GetFileName(), (*best)->GetSourceCount());
		int childScore = ScoreFilename(child->GetFileName(), child->GetSourceCount());
		
		if (childScore > bestScore) {
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

		// Available sources
		if (child->GetClientID() && child->GetClientPort()) {
			CSearchFile::ClientStruct client(child->GetClientID(), child->GetClientPort(), child->GetClientServerIP(), child->GetClientServerPort());
			AddClient(client);
		}
		for (std::list<ClientStruct>::const_iterator cit = child->m_clients.begin(); cit != child->m_clients.end(); ++cit) {
			AddClient(*cit);
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

void CSearchFile::SetDownloadStatus()
{
	bool isPart		= theApp->downloadqueue->GetFileByID(m_abyFileHash) != NULL;
	bool isKnown	= theApp->knownfiles->FindKnownFileByID(m_abyFileHash) != NULL;
	bool isCanceled	= theApp->canceledfiles->IsCanceledFile(m_abyFileHash);

	if (isCanceled && isPart) {
		m_downloadStatus = QUEUEDCANCELED;
	} else if (isCanceled) {
		m_downloadStatus = CANCELED;
	} else if (isPart) {
		m_downloadStatus = QUEUED;
	} else if (isKnown) {
		m_downloadStatus = DOWNLOADED;
	} else {
		m_downloadStatus = NEW;
	}
	// Update status of children too
	for (CSearchResultList::iterator it = m_children.begin(); it != m_children.end(); ++it) {
		Notify_Search_Update_Sources(*it);
	}
}

// File_checked_for_headers
