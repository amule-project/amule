//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "StringFunctions.h"
#include "MD5Sum.h"		// Interface declarations.

#include <openssl/md5.h>

MD5Sum::MD5Sum()
{
}

MD5Sum::MD5Sum(const wxString& sSource)
{
	Calculate(sSource);
}

MD5Sum::MD5Sum(const uint8* buffer, size_t len)
{
	Calculate(buffer, len);
}

void MD5Sum::Calculate(const wxString& sSource)
{
	// Nothing we can do against this unicode2char
	const Unicode2CharBuf buf = unicode2char(sSource);
	Calculate(reinterpret_cast<const uint8*>(buf.data()), strlen(buf.data()));
}

void MD5Sum::Calculate(const uint8* buffer, size_t len)
{
	MD5(buffer, len, m_rawhash);
	m_sHash.Clear();
}

wxString MD5Sum::GetHash()
{
	if (m_sHash.empty()) {
		m_sHash.Printf(wxT("%02x%02x%02x%02x" "%02x%02x%02x%02x" "%02x%02x%02x%02x" "%02x%02x%02x%02x"),
			m_rawhash[0],  m_rawhash[1],  m_rawhash[2],  m_rawhash[3],
			m_rawhash[4],  m_rawhash[5],  m_rawhash[6],  m_rawhash[7],
			m_rawhash[8],  m_rawhash[9],  m_rawhash[10], m_rawhash[11],
			m_rawhash[12], m_rawhash[13], m_rawhash[14], m_rawhash[15]);
	}
	return m_sHash;
}

// File_checked_for_headers
