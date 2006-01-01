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

#ifndef __KAD_FILEIO_H__
#define __KAD_FILEIO_H__

#include "DataIO.h"
#include "../../Types.h"
#include "../../CFile.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CFileIO : public CFile, public CDataIO
{
public:
	virtual void readArray(void* lpResult, uint32 byteCount);
	virtual void writeArray(const void* lpVal, uint32 byteCount);
	virtual uint32 getAvailable() const;
};

///////////////////////////////////////////////////////////////////////////////
// This is just a workaround as the one on SafeFile.h
class CBufferedFileIO : public CFile, public CDataIO
{
public:
	virtual void readArray(void* lpResult, uint32 byteCount);
	virtual void writeArray(const void* lpVal, uint32 byteCount);
	virtual uint32 getAvailable() const;
};

} // End namespace

#endif // __KAD_FILEIO_H__
