//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef KADEMLIA_UTILS_KADUDPKEY_H
#define KADEMLIA_UTILS_KADUDPKEY_H

#include "../../SafeFile.h"

namespace Kademlia
{

class CKadUDPKey
{
      public:
	CKadUDPKey(uint32_t WXUNUSED_UNLESS_DEBUG(zero) = 0)			{ wxASSERT(zero == 0); m_key = m_ip = 0; }
	CKadUDPKey(uint32_t key, uint32_t ip) throw()				{ m_key = key; m_ip = ip; }
	CKadUDPKey(CFileDataIO& file)						{ ReadFromFile(file); }
	CKadUDPKey& operator=(const CKadUDPKey& k1) throw()			{ m_key = k1.m_key; m_ip = k1.m_ip; return *this; }
	CKadUDPKey& operator=(const uint32_t WXUNUSED_UNLESS_DEBUG(zero))	{ wxASSERT(zero == 0); m_key = m_ip = 0; return *this; }
	friend bool operator==(const CKadUDPKey& k1, const CKadUDPKey& k2) throw() { return k1.GetKeyValue(k1.m_ip) == k2.GetKeyValue(k2.m_ip);}

	uint32_t	GetKeyValue(uint32_t myIP) const throw()		{ return (myIP == m_ip) ? m_key : 0; }
	bool		IsEmpty() const throw()					{ return (m_key == 0) || (m_ip == 0); }
	void		StoreToFile(CFileDataIO& file) const			{ file.WriteUInt32(m_key); file.WriteUInt32(m_ip); }
	void		ReadFromFile(CFileDataIO& file)				{ m_key = file.ReadUInt32(); m_ip = file.ReadUInt32(); }

      private:
	uint32_t	m_key;
	uint32_t	m_ip;
};

} // namespace Kademlia

#endif /* KADEMLIA_UTILS_KADUDPKEY_H */
