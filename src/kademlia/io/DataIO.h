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

#pragma once

#include <list>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;
class CTag;
typedef std::list<CTag*> TagList;

class CDataIO
{
public:
	byte		readByte();
	uint8		readUInt8();
	uint16		readUInt16();
	uint32		readUInt32();
	void		readUInt128(CUInt128* value);
	void		readHash(BYTE* value);
	BYTE*		readBsob(uint8* size);
	float		readFloat();
	CStringW	readStringUTF8(bool bOptACP = false);
	CTag*		readTag(bool bOptACP = false);
	void		readTagList(TagList* taglist, bool bOptACP = false);

	void		writeByte(byte val);
	void		writeUInt8(uint8 val);
	void		writeUInt16(uint16 val);
	void		writeUInt32(uint32 val);
	void		writeUInt128(const CUInt128& val);
	void		writeHash( const BYTE* val);
	void		writeBsob( const BYTE* val, uint8 size);
	void		writeFloat(float val);
	void		writeTag(const CTag* tag);
	void		writeTag(LPCSTR name, uint8 value);
	void		writeTag(LPCSTR name, uint16 value);
	void		writeTag(LPCSTR name, uint32 value);
	void		writeTag(LPCSTR name, float value);
	void		writeTagList(const TagList& tagList);

	virtual void readArray(LPVOID lpResult, uint32 byteCount) = 0;
	virtual void writeArray(LPCVOID lpVal, uint32 byteCount) = 0;
	virtual UINT getAvailable() const = 0;
};

} // End namespace
