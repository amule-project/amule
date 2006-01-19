//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __KAD_BYTEIO_H__
#define __KAD_BYTEIO_H__

#include "../../SafeFile.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

#warning Marked for deletion
	
class CIOException;

class CByteIO : public CFileDataIO
{
public:
	CByteIO(byte* buffer, uint32 available)
	{
		m_bReadOnly = false; 
		m_buffer = buffer; 
		m_available = available; 
		m_used = 0;
	}
	CByteIO(const byte* buffer, uint32 available)
	{
		m_bReadOnly = true; 
		m_buffer = const_cast<byte*>(buffer);
		m_available = available; 
		m_used = 0;
	}

	/** @see CFileDataIO::doRead */
	virtual sint64 doRead(void* buffer, size_t count) const;

	/** @see CFileDataIO::doWrite */
	virtual sint64 doWrite(const void* buffer, size_t count);	

	/** @see CFileDataIO::doSeek */
	virtual sint64 doSeek(sint64 offset) const;	
	
	uint64 GetPosition() const;
	
	uint64 GetLength() const;
	
	uint32 getAvailable() const { return m_available; }
	void reset(void);

	virtual ~CByteIO() {};

private:
	bool	m_bReadOnly;
	mutable byte*	m_buffer;
	mutable uint32	m_available;
	mutable uint32	m_used;
};

} // End namespace

#endif // __KAD_BYTEIO_H__
