/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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

#include "stdafx.h"
#include "FileIO.h"
#include "IOException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void CFileIO::readArray(LPVOID lpResult, uint32 byteCount)
{
	uint32 test = Read(lpResult, byteCount);
	if( test != byteCount )
		throw new CIOException(ERR_END_OF_FILE);
}

void CFileIO::writeArray(LPCVOID lpVal, uint32 byteCount)
{
	Write(lpVal, byteCount);
}

UINT CFileIO::getAvailable() const
{
	return (UINT)(GetLength() - GetPosition());
}

void CBufferedFileIO::readArray(LPVOID lpResult, uint32 byteCount)
{
	uint32 test = Read(lpResult, byteCount);
	if( test != byteCount )
		throw new CIOException(ERR_END_OF_FILE);
}

void CBufferedFileIO::writeArray(LPCVOID lpVal, uint32 byteCount)
{
	Write(lpVal, byteCount);
}

UINT CBufferedFileIO::getAvailable() const
{
	return (UINT)(GetLength() - GetPosition());
}
