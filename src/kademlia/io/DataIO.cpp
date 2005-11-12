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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
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

#include "DataIO.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Tag.h"
#include "../utils/LittleEndian.h"
#include "../utils/UInt128.h"
#include "IOException.h"
#include "SafeFile.h"
#include "ArchSpecific.h"
#include "StringFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

// This may look confusing that the normal methods use le() and the LE methods don't.
// The reason is the variables are stored in memory in little endian format already.

byte CDataIO::readByte()
{
	byte retVal;
	readArray(&retVal, 1);
	return retVal;
}

uint8 CDataIO::readUInt8()
{
	uint8 retVal;
	readArray(&retVal, sizeof(uint8));
	return retVal;
}

uint16 CDataIO::readUInt16()
{
	uint16 retVal;
	readArray(&retVal, sizeof(uint16));
	return ENDIAN_SWAP_16(retVal);
}

uint32 CDataIO::readUInt32()
{
	uint32 retVal;
	readArray(&retVal, sizeof(uint32));
	return ENDIAN_SWAP_32(retVal);
}

void CDataIO::readUInt128(CUInt128* value)
{
	uint32* data = (uint32*) value->getDataPtr();
	for (int i = 0; i < 4; i++)
		data[i] = readUInt32();
}

float CDataIO::readFloat()
{
	float retVal;
	readArray(&retVal, sizeof(float));
	return retVal;
}

void CDataIO::readHash(unsigned char* value)
{
	readArray(value, 16);
}

unsigned char* CDataIO::readBsob(uint8* puSize)
{
	*puSize = readUInt8();
	if (getAvailable() < *puSize)
		throw CIOException(ERR_BUFFER_TOO_SMALL);
	unsigned char* pucBsob = new unsigned char[*puSize];
	try {
		readArray(pucBsob, *puSize);
	} catch(...) {
		delete[] pucBsob;
		throw;
	}
	return pucBsob;
}

wxString CDataIO::readStringUTF8(bool bOptACP)
{
	uint32 length = readUInt16();
	
	char val[length + 1];
	// We only need to set the the NULL terminator, since we know that
	// reads will either succeed or throw an exception, in which case
	// we wont be returning anything
	val[length] = 0;
	
	readArray(val, length);
	wxString str;
	
	if (bOptACP) {
		str = UTF82unicode(val);
		if (str.IsEmpty()) {
			// Fallback to system locale
			str = char2unicode(val);
		}					
	} else {
		str = char2unicode(val);
	}

	return str;
}

CTag *CDataIO::readTag(bool bOptACP)
{
	CTag *retVal = NULL;
	wxString name;
	byte type = 0;
	try {
		type = readByte();
		name = readStringUTF8(false);

		switch (type)
		{
			// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade 
			// the net in some months.
			//
			// And still.. it doesnt't work this way without breaking backward compatibility. To properly
			// do this without messing up the network the following would have to be done:
			//	 -	those tag types have to be ignored by any client, otherwise those tags would also be sent (and 
			//		that's really the problem)
			//
			//	 -	ignoring means, each client has to read and right throw away those tags, so those tags get
			//		get never stored in any tag list which might be sent by that client to some other client.
			//
			//	 -	all calling functions have to be changed to deal with the 'nr. of tags' attribute (which was 
			//		already parsed) correctly.. just ignoring those tags here is not enough, any taglists have to 
			//		be built with the knowledge that the 'nr. of tags' attribute may get decreased during the tag 
			//		reading..
			// 
			// If those new tags would just be stored and sent to remote clients, any malicious or just bugged
			// client could let send a lot of nodes "corrupted" packets...
			//
			case TAGTYPE_HASH:
			{
				byte value[16];
				readHash(value);
				retVal = new CTagHash(name, value);
				break;
			}

			case TAGTYPE_STRING:
				retVal = new CTagStr(name, readStringUTF8(bOptACP));
				break;

			case TAGTYPE_UINT32:
				retVal = new CTagUInt32(name, readUInt32());
				break;

			case TAGTYPE_FLOAT32:
				retVal = new CTagFloat(name, readFloat());
				break;

			// NOTE: This tag data type is accepted and stored only to give us the possibility to upgrade 
			// the net in some months.
			//
			// And still.. it doesnt't work this way without breaking backward compatibility
			case TAGTYPE_BSOB:
			{
				uint8 size;
				unsigned char* value = readBsob(&size);
				try {
					retVal = new CTagBsob(name, value, size);
				} catch(...) {
					delete[] value;
					throw;
				}
				delete[] value;
				break;
			}

			case TAGTYPE_UINT16:
				retVal = new CTagUInt16(name, readUInt16());
				break;

			case TAGTYPE_UINT8:
				retVal = new CTagUInt8(name, readUInt8());
				break;

			default:
				throw wxString(wxT("Invalid Kad tag type on packet"));
		}
	} catch (...) {
		printf("Invalid Kad tag; type=0x%02x name=0x%02x\n",
			type, ((const char *)unicode2char(name))[0]);
		delete retVal;
		throw;
	}
	
	return retVal;
}

void CDataIO::readTagList(TagList* taglist, bool bOptACP)
{
	uint32 count = readByte();
	for (uint32 i=0; i<count; i++)
	{
		CTag* tag = readTag(bOptACP);
		taglist->push_back(tag);
	}
}

void CDataIO::writeByte(byte val)
{
	writeArray(&val, 1);
}

void CDataIO::writeUInt8(uint8 val)
{
	writeArray(&val, sizeof(uint8));
}

void CDataIO::writeUInt16(uint16 val)
{
	ENDIAN_SWAP_I_16(val);
	writeArray(&val, sizeof(uint16));
}

void CDataIO::writeUInt32(uint32 val)
{
	ENDIAN_SWAP_I_32(val);
	writeArray(&val, sizeof(uint32));
}

void CDataIO::writeUInt128(const CUInt128& val)
{
	for (int i = 0; i < 4; i++)
		writeUInt32(val.get32BitChunk(i));
}

void CDataIO::writeFloat(float val)
{
	writeArray(&val, sizeof(float));
}

void CDataIO::writeHash(const byte* value)
{
	writeArray(value, 16);
}

void CDataIO::writeBsob(const byte* value, uint8 size)
{
	writeUInt8(size);
	writeArray(value, size);
}


void CDataIO::WriteStringCore(const char *s) {
	unsigned int sLength = s ? strlen(s) : 0;
	wxASSERT(sLength < (uint16)0xFFFF); // Can't be higher than a uint16
	writeUInt16(sLength);
	if (sLength) {
		// We dont include the NULL terminator.
		// It is because we write the size, so the NULL is not necessary.
		writeArray(s, sLength);
	}	
}

void CDataIO::writeString(const wxString& rstr,  bool UTF8)
{
	switch (UTF8) {
		case true: {
			Unicode2CharBuf s(unicode2UTF8(rstr));
			if (s) {
				WriteStringCore(s);
				break;
			}
		}
		default: {
			Unicode2CharBuf s(unicode2char(rstr));
			WriteStringCore(s);
		}
	}	
}

void CDataIO::writeTag(const CTag* tag)
{
	try
	{
		uint8 type;
		if (tag->m_type == 0xFE)
		{
			if (tag->GetInt() <= 0xFF)
				type = TAGTYPE_UINT8;
			else if (tag->GetInt() <= 0xFFFF)
				type = TAGTYPE_UINT16;
			else
				type = TAGTYPE_UINT32;
		}
		else
			type = tag->m_type;

		writeByte(type);
		
		writeString(tag->m_name.GetData(),false); // No utf8
		
		switch (type)
		{
			case TAGTYPE_HASH:
				// Do NOT use this to transfer any tags for at least half a year!!
				writeHash(tag->GetHash());
				wxASSERT(0);
				break;
			case TAGTYPE_STRING:
			{
				writeString(tag->GetStr(), true); // Always UTF8
				break;
			}
			case TAGTYPE_UINT32:
				writeUInt32(tag->GetInt());
				break;
			case TAGTYPE_FLOAT32:
				writeFloat(tag->GetFloat());
				break;
			case TAGTYPE_BSOB:
				// Do NOT use this to transfer any tags for at least half a year!!
				writeBsob(tag->GetBsob(), tag->GetBsobSize());
				wxASSERT(0);
				break;
			case TAGTYPE_UINT16:
				writeUInt16(tag->GetInt());
				break;
			case TAGTYPE_UINT8:
				writeUInt8(tag->GetInt());
				break;
		}
	} 
	catch (const CIOException& ioe)
	{
		//AddDebugLogLine( false, wxT("Exception in CDataIO:writeTag (IO Error(%i))"), ioe->m_cause);
		printf("Exception in CDataIO:writeTag (IO Error(%i))", ioe.m_cause);
		throw;
	}
	catch (...) 
	{
		//AddDebugLogLine(false, wxT("Exception in CDataIO:writeTag"));
		printf("Exception in CDataIO:writeTag");
		throw;
	}
}

void CDataIO::writeTag(const wxString& name, uint32 value)
{
	CTagUInt32 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(const wxString& name, uint16 value)
{
	CTagUInt16 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(const wxString& name, uint8 value)
{
	CTagUInt8 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(const wxString& name, float value)
{
	CTagFloat tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTagList(const TagList& tagList)
{
	uint32 count = (uint32)tagList.size();
	wxASSERT( count <= 0xFF );
	writeByte(count);
	TagList::const_iterator it;
	for (it = tagList.begin(); it != tagList.end(); it++)
		writeTag(*it);
}

namespace Kademlia {
void deleteTagListEntries(TagList* taglist)
{
	TagList::const_iterator it;
	for (it = taglist->begin(); it != taglist->end(); it++)
		delete *it;
	taglist->clear();
}
}

void KadTagStrMakeLower(CTagValueString& rwstr)
{
	rwstr.MakeLower();
}
