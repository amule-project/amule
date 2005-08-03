//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "ED2KLink.h"
#endif

#include <wx/defs.h>			// Needed before any other wx/*.h

#include "ED2KLink.h"			// Interface declarations.
#include "SafeFile.h"			// Needed for CSafeMemFile
#include "NetworkFunctions.h"	// Needed for Uint32toStringIP
#include "StringFunctions.h"	// Needed for unicode2char

#include <vector>


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


CED2KLink* CED2KLink::CreateLinkFromUrl( const wxString& uri )
{
	if ( !uri.Lower().StartsWith( wxT("ed2k://|") ) ) {
		throw wxString( wxT("Not a ed2k-URI" ) );
	}

	if ( uri.Last() != wxT('/') ) {
		throw wxString( wxT("Not a valid ed2k-URI" ) );
	}
	
	// Remove the "ed2k://|" prefix
	wxString URL = uri.Right( uri.Length() - 8 );

	// Split the URL into fields
	std::vector<wxString> fields;

	while ( !URL.IsEmpty() ) {
		fields.push_back( URL.BeforeFirst( wxT('|') ) );
		URL = URL.AfterFirst( wxT('|') );
	}

	wxString type = fields[0].Lower();	
	if ( type == wxT("file") && ( fields.size() >= 4 ) ) {
		wxString hashSet;
		wxString hashMaster;
		wxString sources;


		// Skip the first 4 fields
		for ( unsigned int i = 4; i < fields.size(); i++ ) {
			if ( fields[ i - 1 ] == wxT("/") ) {
				// Past the end of the regular fields, probably a source
				sources += fields[i] + wxT("|");
			} else if ( fields[i].StartsWith( wxT("p=" ) ) ) {
				hashSet = fields[i];
			} else if ( fields[i].StartsWith( wxT("h=" ) ) ) {
				hashMaster = fields[i];
			}
		}

		// Unescape the file-name
		fields[1] = UnescapeHTML( fields[1] );
	
		return new CED2KFileLink( fields[1], fields[2], fields[3], hashSet, hashMaster, sources );
	} else if ( type == wxT("server") && fields.size() >= 3 ) {
		// Unescape the server-name
		fields[1] = UnescapeHTML( fields[1] );
		
		return new CED2KServerLink( fields[1], fields[2] );
	} else if ( type == wxT("serverlist") && fields.size() >= 2 ) {
		return new CED2KServerListLink( fields[1] );
	} else {
		throw wxString( wxT("Not a valid URI-type") );
	}

	return 0;
}


///////////////////////////////////////////// 
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const wxString& address)
	: CED2KLink( kServerList )
{
	m_address = address;
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
CED2KServerLink::CED2KServerLink( const wxString& ip, const wxString& port )
	: CED2KLink( kServer )
{
	unsigned long ul = StrToULong( port );
	
	if ( ul > 0xFFFF || ul == 0) {
		throw wxString( wxT("Bad port number") );
	}
	
	m_port = static_cast<uint16>(ul);
	m_ip = StringIPtoUint32( ip );
}


wxString CED2KServerLink::GetLink() const
{
	return wxT("ed2k://|server|") + Uint32toStringIP(m_ip) + wxString::Format(wxT("|%d|/"), (int)m_port );
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
CED2KFileLink::CED2KFileLink( const wxString& name, const wxString& size, const wxString& hash, const wxString& hashset, const wxString& masterhash, const wxString& sources)
	: CED2KLink( kFile ),
	  m_name( name ),
	  m_size( size )
{
	m_sources = NULL;
	m_hashset = NULL;
	m_bAICHHashValid = false;	
	
	if ( hash.Length() != 32 ) {
		throw wxString( wxT("I'll-formed hash") );
	}

	if ( !size.IsNumber() ) {
		throw wxString( wxT("Illegal size value" ) );
	}

	m_hash.Decode( hash );

	// Parse sources
	if ( !sources.IsEmpty() ) {
		m_sources = new CSafeMemFile();
		m_sources->WriteUInt16( 0 );
		
		wxString srcs = sources;
		while ( !srcs.IsEmpty() ) {
			// Remove the next item
			wxString current = srcs.BeforeFirst( wxT('|') );
			srcs.Remove( 0, current.Length() + 1 );

			// Check for vality
			if ( current.Lower().StartsWith( wxT("sources,") ) ) {
				current.Remove( 0, 8 );
	
				uint32 IP = StringIPtoUint32( current.BeforeFirst( wxT(':') ) );
				uint32 Port = StrToULong( current.AfterFirst( wxT(':') ) );

				// Sainity checking
				if ( Port == 0 || ( Port > (uint16)-1 ) ) {
					throw wxString( wxT("Invalid IP/Port" ) );
				}
				
				if ( IP == 0 ) {
					SUnresolvedHostname* item = new SUnresolvedHostname();
					item->strHostname = current.BeforeFirst( wxT(':') );
					item->nPort = Port;
					
					m_hostSources.AddTail( item );
				} else {
					m_sources->WriteUInt32( IP );
					m_sources->WriteUInt16( Port );
				
					// No server IP and port is known
					m_sources->WriteUInt32( 0 );
					m_sources->WriteUInt16( 0 );
				}	
			}
		}

		// Rewrite the source-count (if any)
		int count = m_sources->GetLength() / 12; // 2xuint16 + 2uint32
		if ( count ) {
			m_sources->Seek(0);
			m_sources->WriteUInt16( count );
			m_sources->Seek(0);
		} else {
			delete m_sources;
			m_sources = NULL;
		}
	}
	
	// Parse AICH-hashset
	if ( !hashset.IsEmpty() ) {
		wxString tmp_hash = hashset + wxT(":");

		if ( tmp_hash.StartsWith( wxT("p=") ) ) {
			tmp_hash.Remove( 0, 2 );
		
			m_hashset = new CSafeMemFile();
			m_hashset->WriteHash16(m_hash);
			m_hashset->WriteUInt16(0);

			while ( !tmp_hash.IsEmpty() ) {
				wxString cur_hash = tmp_hash.BeforeFirst( wxT(':') );
				wxString tmp_hash = tmp_hash.AfterFirst( wxT(':') );
			
				if ( cur_hash.Length() == 32 ) {
					m_hashset->WriteHash16( CMD4Hash( cur_hash ) );
				}
			}
		
			// 
			int count = m_hashset->GetLength() / 16 - 1; 
		
			if ( count ) {
				m_hashset->Seek( 16, wxFromStart);
				m_hashset->WriteUInt16( count );
				m_hashset->Seek( 0, wxFromStart);
			} else {
				delete m_hashset;
				m_hashset = NULL;
			}
		}
	}

	if (!masterhash.IsEmpty()) {
		// Remove the prefix of "h="
		wxString strHash = masterhash.AfterFirst(wxT('='));
		wxASSERT(!strHash.IsEmpty());
		if (!strHash.IsEmpty()) {
			m_bAICHHashValid = otherfunctions::DecodeBase32(
					strHash, CAICHHash::GetHashSize(), m_AICHHash.GetRawHash()) == 
				CAICHHash::GetHashSize();
			
			if (!m_bAICHHashValid || m_AICHHash.GetString() != strHash) {
				throw wxString(wxT("Invalid hash"));
			}
		}
	}
}


CED2KFileLink::~CED2KFileLink()
{
	if (m_sources) {
		delete m_sources;
		m_sources = NULL;
	}
	
	if (m_hashset) {
		delete m_hashset;
		m_hashset =  NULL;
	}
	
	while (!m_hostSources.IsEmpty()) {
		delete m_hostSources.RemoveHead();
	}
}


wxString CED2KFileLink::GetLink() const
{
	return wxT("ed2k://|file|") + m_name + wxT("|") + m_size + wxT("|") + m_hash.Encode() + wxT("|/");
}


wxString CED2KFileLink::GetName() const
{
	return m_name;
}


uint32 CED2KFileLink::GetSize() const
{
	return StrToULong( m_size );
}


const CMD4Hash& CED2KFileLink::GetHashKey() const
{
	return m_hash;
}


bool CED2KFileLink::HasValidSources() const
{
	return m_sources;
}
	

bool CED2KFileLink::HasHostnameSources() const 
{
	return !m_hostSources.IsEmpty();
}


bool CED2KFileLink::HasValidAICHHash() const
{
	return m_bAICHHashValid;
}


const CAICHHash& CED2KFileLink::GetAICHHash() const
{
	return m_AICHHash;
}
