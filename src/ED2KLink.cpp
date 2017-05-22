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

#include <wx/wx.h>

#include "ED2KLink.h"			// Interface declarations.

#include <wx/string.h>
#include <wx/regex.h>			// Needed for wxRegEx
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer

#include <protocol/ed2k/Constants.h>

#include "MemFile.h"			// Needed for CMemFile
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP
#include <common/Format.h>		// Needed for CFormat


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
	{ wxCHECK(re_type.IsValid(), NULL); }

	if (!re_type.Matches(link)) {
		throw wxString(wxT("Not a valid ed2k-URI"));
	}

	wxString type = re_type.GetMatch(link, 1).MakeLower();
	{ wxCHECK(type.Length(), NULL); }

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
	wxStringTokenizer tokens(link, wxT("|"), wxTOKEN_RET_EMPTY_ALL);

	// Must at least be ed2k://|file|NAME|SIZE|HASH|/
	if (tokens.CountTokens() < 5
		|| tokens.GetNextToken() != wxT("ed2k://")
		|| tokens.GetNextToken() != wxT("file")) {
		throw wxString(wxT("Not a valid file link"));
	}

	m_name = UnescapeHTML(tokens.GetNextToken().Strip(wxString::both));
	// We don't want a path in the name.
	m_name.Replace(wxT("/"), wxT("_"));

	// Note that StrToULong returns ULONG_MAX if the value is
	// too large to be contained in a unsigned long, which means
	// that this check is valid, as odd as it seems
	wxString size = tokens.GetNextToken().Strip(wxString::both);
	m_size = StrToULongLong(size);
	if ((m_size == 0) || (m_size > MAX_FILE_SIZE)) {
		throw wxString(CFormat(wxT("Invalid file size %i")) % m_size);
	}

	if (!m_hash.Decode(tokens.GetNextToken().Strip(wxString::both))) {
		throw wxString(wxT("Invalid hash"));
	}

	// Check extra fields (sources, parthashes, masterhashes)
	while (tokens.HasMoreTokens()) {
		wxString field = tokens.GetNextToken().MakeLower().Strip(wxString::both);

		if (field.StartsWith(wxT("sources,"))) {
			wxStringTokenizer srcTokens(field, wxT(","));
			// Skipping the first token ("sources").
			wxString token = srcTokens.GetNextToken();
			while (srcTokens.HasMoreTokens()) {
				token = srcTokens.GetNextToken().Strip(wxString::both);

				wxStringTokenizer sourceTokens(token, wxT(":"));
				wxString addr = sourceTokens.GetNextToken();
				if (addr.IsEmpty()) {
					throw wxString( wxT("Empty address" ) );
				}

				wxString strport = sourceTokens.GetNextToken();
				if (strport.IsEmpty()) {
					throw wxString( wxT("Empty port" ) );
				}

				unsigned port = StrToULong(strport);

				// Sanity checking
				if ((port == 0) || (port > 0xFFFF)) {
					throw wxString( wxT("Invalid Port" ) );
				}

				wxString sourcehash;
				uint8 cryptoptions =0;
				wxString strcryptoptions = sourceTokens.GetNextToken();
				if (!strcryptoptions.IsEmpty()) {
					cryptoptions = (uint8) StrToULong(strcryptoptions);
					if ((cryptoptions & 0x80) > 0) {
						// Source ready for encryption, hash included.
						sourcehash = sourceTokens.GetNextToken();
						if (sourcehash.IsEmpty()) {
							throw wxString( wxT("Empty sourcehash conflicts with cryptoptions flag 0x80" ) );
						}
					}
				}

				SED2KLinkSource entry = { addr, (uint16) port, sourcehash, cryptoptions };

				m_sources.push_back(entry);
			}
		} else if (field.StartsWith(wxT("p="))) {
			wxStringTokenizer hashTokens(field.AfterFirst(wxT('=')), wxT(":"), wxTOKEN_RET_EMPTY_ALL);

			m_hashset = new CMemFile();
			m_hashset->WriteHash(m_hash);
			m_hashset->WriteUInt16(0);

			while (hashTokens.HasMoreTokens()) {
				CMD4Hash hash;
				if (!hash.Decode(hashTokens.GetNextToken().Strip(wxString::both))) {
					throw wxString(wxT("Invalid hash in part-hashes list"));
				}

				m_hashset->WriteHash(hash);
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
	return CFormat(wxT("ed2k://|file|%s|%u|%s|/")) % m_name % m_size % m_hash.Encode();
}


wxString CED2KFileLink::GetName() const
{
	return m_name;
}


uint64 CED2KFileLink::GetSize() const
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
// File_checked_for_headers
