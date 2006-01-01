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

#include <wx/string.h>
#include <wx/regex.h>			// Needed for wxRegEx
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer

#include "ED2KLink.h"			// Interface declarations.
#include "MemFile.h"			// Needed for CMemFile
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP
#include <common/StringFunctions.h>	// Needed for unicode2char
#include "OtherFunctions.h"		// Needed for DecodeBase32
#include "OPCodes.h"			// Needed for MAX_FILE_SIZE



CED2KLink::CED2KLink( LinkType type )
	: m_type( type )
{
}


CED2KLink::~CED2KLink()
{
}


CED2KLink::LinkType CED2KLink::GetKind() const
{
	return m_type;
}


CED2KLink* CED2KLink::CreateLinkFromUrl(const wxString& link)
{
	wxRegEx re_type(wxT("ed2k://\\|(file|server|serverlist)\\|.*/"), wxRE_ICASE | wxRE_DEFAULT);
	wxCHECK(re_type.IsValid(), NULL);

	if (!re_type.Matches(link)) {
		throw wxString(wxT("Not a valid ed2k-URI"));
	}

	wxString type = re_type.GetMatch(link, 1).MakeLower();
	wxCHECK(type.Length(), NULL);
	
	if (type == wxT("file")) {
		return new CED2KFileLink(link);
	} else if (type == wxT("server")) {
		return new CED2KServerLink(link);	
	} else if (type == wxT("serverlist")) {
		return new CED2KServerListLink(link);
	} else {
		wxCHECK(false, NULL);
	}
}
	

/////////////////////////////////////////////
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const wxString& link)
	: CED2KLink( kServerList )
{
	wxRegEx re(wxT("ed2k://\\|serverlist\\|(.*)\\|/"), wxRE_ICASE | wxRE_DEFAULT);
	if (!re.Matches(link)) {
		throw wxString(wxT("Not a valid server-list link."));
	}
	
	m_address = UnescapeHTML(re.GetMatch(link, 1));
}


wxString CED2KServerListLink::GetLink() const
{
	return wxT("ed2k://|serverlist|") + m_address + wxT("|/");
}


const wxString& CED2KServerListLink::GetAddress() const
{
	return m_address;
}


/////////////////////////////////////////////
// CED2KServerLink implementation
/////////////////////////////////////////////
CED2KServerLink::CED2KServerLink(const wxString& link)
	: CED2KLink( kServer )
{
	wxRegEx re(wxT("ed2k://\\|server\\|([^\\|]+)\\|([0-9]+)\\|/"), wxRE_ICASE | wxRE_DEFAULT);
	if (!re.Matches(link)) {
		throw wxString(wxT("Not a valid server link."));
	}
	
	wxString ip = UnescapeHTML(re.GetMatch(link, 1));
	wxString port = re.GetMatch(link, 2);
	
	unsigned long ul = StrToULong(port);
	if (ul > 0xFFFF || ul == 0) {
		throw wxString( wxT("Bad port number") );
	}

	m_port = static_cast<uint16>(ul);
	m_ip = StringIPtoUint32(ip);
}


wxString CED2KServerLink::GetLink() const
{
	return wxString(wxT("ed2k://|server|")) << Uint32toStringIP(m_ip) << wxT("|") << m_port << wxT("|/");
}


uint32 CED2KServerLink::GetIP() const
{
	return m_ip;
}


uint16 CED2KServerLink::GetPort() const
{
	return m_port;
}


/////////////////////////////////////////////
// CED2KFileLink implementation
/////////////////////////////////////////////
CED2KFileLink::CED2KFileLink(const wxString& link)
	: CED2KLink( kFile ),
	  m_hashset(NULL),
	  m_size(0),
	  m_bAICHHashValid(false)
{
	// Start tokenizing after the "ed2k:://|file|" part of the link
	wxStringTokenizer tokens(link.Mid(13), wxT("|/"), wxTOKEN_RET_EMPTY_ALL);

	// Must at least be ed2k://|file|NAME|SIZE|HASH|/
	if (tokens.CountTokens() < 5) {
		throw wxString(wxT("Not a valid file link"));
	}

	m_name = UnescapeHTML(tokens.GetNextToken().Strip(wxString::both));
	
	// Note that StrToULong returns ULONG_MAX if the value is
	// too large to be contained in a unsigned long, which means
	// that this check is valid, as odd as it seems
	wxString size = tokens.GetNextToken().Strip(wxString::both);
	m_size = StrToULong(size);
	if ((m_size == 0) or (m_size > MAX_FILE_SIZE)) {
		throw wxString(wxT("Invalid file size"));
	}
	
	m_hash.Decode(tokens.GetNextToken().Strip(wxString::both));

	// Check extra fields (sources, parthashes, masterhashes)
	while (tokens.HasMoreTokens()) {
		wxString field = tokens.GetNextToken().MakeLower().Strip(wxString::both);

		if (field.StartsWith(wxT("sources,"))) {
			wxStringTokenizer srcTokens(field, wxT(","));
			// Skipping the first token ("sources").
			wxString token = srcTokens.GetNextToken();
			while (srcTokens.HasMoreTokens()) {
				token = srcTokens.GetNextToken().Strip(wxString::both);

				wxString addr = token.BeforeFirst(wxT(':'));
				unsigned port = StrToULong(token.AfterFirst(wxT(':')));

				// Sanity checking
				if ((port == 0) or (port > 0xFFFF) or addr.IsEmpty()) {
					throw wxString( wxT("Invalid Address/Port" ) );
				}

				SED2KLinkSource entry = {addr, port};

				m_sources.push_back(entry);
			}
		} else if (field.StartsWith(wxT("p="))) {
			wxStringTokenizer hashTokens(field.AfterFirst(wxT('=')), wxT(":"), wxTOKEN_RET_EMPTY_ALL);
		
			m_hashset = new CMemFile();
			m_hashset->WriteHash(m_hash);
			m_hashset->WriteUInt16(0);

			while (hashTokens.HasMoreTokens()) {
				wxString hash = hashTokens.GetNextToken().Strip(wxString::both);
				
				if (hash.Length() != 32u) {
					throw wxString(wxT("Invalid hash in part-hashes list"));
				}

				m_hashset->WriteHash(CMD4Hash(hash));
			}
			
			unsigned count = m_hashset->GetLength() / 16u - 1u;

			if (count) {
				m_hashset->Seek( 16, wxFromStart);
				m_hashset->WriteUInt16( count );
				m_hashset->Seek( 0, wxFromStart);
			} else {
				delete m_hashset;
				m_hashset = NULL;
			}
		} else if (field.StartsWith(wxT("h="))) {
			wxString hash = field.AfterFirst(wxT('=')).MakeUpper();
			
			size_t decodedSize = DecodeBase32(hash, CAICHHash::GetHashSize(), m_AICHHash.GetRawHash());
			if ((decodedSize != CAICHHash::GetHashSize()) || m_AICHHash.GetString() != hash) {
				throw wxString(wxT("Invalid master-hash"));
			}

			m_bAICHHashValid = true;
		}
	}
}


CED2KFileLink::~CED2KFileLink()
{
	delete m_hashset;
	m_hashset =  NULL;
}


wxString CED2KFileLink::GetLink() const
{
	return wxT("ed2k://|file|") + m_name + wxString::Format(wxT("|%u|"), m_size) + m_hash.Encode() + wxT("|/");
}


wxString CED2KFileLink::GetName() const
{
	return m_name;
}


uint32 CED2KFileLink::GetSize() const
{
	return m_size;
}


const CMD4Hash& CED2KFileLink::GetHashKey() const
{
	return m_hash;
}


bool CED2KFileLink::HasValidAICHHash() const
{
	return m_bAICHHashValid;
}


const CAICHHash& CED2KFileLink::GetAICHHash() const
{
	return m_AICHHash;
}

