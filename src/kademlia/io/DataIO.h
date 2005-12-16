//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef __KAD_DATAIO_H__
#define __KAD_DATAIO_H__

#include <list>
#include "../../Types.h"
#include <wx/string.h>

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
	void		readHash(unsigned char* value);
	unsigned char*		readBsob(uint8* size);
	float		readFloat();
	wxString	readStringUTF8(bool bOptACP = false);
	CTag*		readTag(bool bOptACP = false);
	void		readTagList(TagList* taglist, bool bOptACP = false);

	void		writeByte(byte val);
	void		writeUInt8(uint8 val);
	void		writeUInt16(uint16 val);
	void		writeUInt32(uint32 val);
	void		writeUInt128(const CUInt128& val);
	void		writeHash( const unsigned char* val);
	void		writeBsob( const unsigned char* val, uint8 size);
	void		writeString(const wxString& rstr,  bool UTF8);
	void		writeFloat(float val);
	void		writeTag(const CTag* tag);
	void		writeTag(const wxString& name, uint8 value);
	void		writeTag(const wxString& name, uint16 value);
	void		writeTag(const wxString& name, uint32 value);
	void		writeTag(const wxString& name, float value);
	void		writeTagList(const TagList& tagList);

	virtual void readArray(void* lpResult, uint32 byteCount) = 0;
	virtual void writeArray(const void* lpVal, uint32 byteCount) = 0;
	virtual uint32 getAvailable() const = 0;
	
protected:
	virtual ~CDataIO() {};
private:
	void	WriteStringCore(const char *s);	
	
};

} // End namespace

#endif // __KAD_DATAIO_H__
