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
#include "resource.h"
#include "DataIO.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Tag.h"
#include "../utils/LittleEndian.h"
#include "../utils/UInt128.h"
#include "IOException.h"
#include "StringConversion.h"
#include "SafeFile.h"
#include <atlenc.h>
#include "Log.h"

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
	return retVal;
}

uint32 CDataIO::readUInt32()
{
	uint32 retVal;
	readArray(&retVal, sizeof(uint32));
	return retVal;
}

void CDataIO::readUInt128(CUInt128* value)
{
	readArray(value->getDataPtr(), sizeof(uint32)*4);
}

float CDataIO::readFloat()
{
	float retVal;
	readArray(&retVal, sizeof(float));
	return retVal;
}

void CDataIO::readHash(BYTE* value)
{
	readArray(value, 16);
}

BYTE* CDataIO::readBsob(uint8* puSize)
{
	*puSize = readUInt8();
	if (getAvailable() < *puSize)
		throw new CIOException(ERR_BUFFER_TOO_SMALL);
	BYTE* pucBsob = new BYTE[*puSize];
	try{
		readArray(pucBsob, *puSize);
	}
	catch(CException*){
		delete[] pucBsob;
		throw;
	}
	return pucBsob;
}

CStringW CDataIO::readStringUTF8(bool bOptACP)
{
	UINT uRawSize = readUInt16();
	const UINT uMaxShortRawSize = SHORT_RAW_ED2K_UTF8_STR;
	if (uRawSize <= uMaxShortRawSize)
	{
		char acRaw[uMaxShortRawSize];
		readArray(acRaw, uRawSize);
		WCHAR awc[uMaxShortRawSize];
		int iChars = bOptACP
					   ? utf8towc(acRaw, uRawSize, awc, ARRSIZE(awc))
					   : ByteStreamToWideChar(acRaw, uRawSize, awc, ARRSIZE(awc));
		if (iChars >= 0)
			return CStringW(awc, iChars);
		return CStringW(acRaw, uRawSize); // use local codepage
	}
	else
	{
		Array<char> acRaw(uRawSize);
		readArray(acRaw, uRawSize);
		Array<WCHAR> awc(uRawSize);
		int iChars = bOptACP
					   ? utf8towc(acRaw, uRawSize, awc, uRawSize)
					   : ByteStreamToWideChar(acRaw, uRawSize, awc, uRawSize);
		if (iChars >= 0)
			return CStringW(awc, iChars);
		return CStringW(acRaw, uRawSize); // use local codepage
	}
}

CTag *CDataIO::readTag(bool bOptACP)
{
	CTag *retVal = NULL;
	char *name = NULL;
	byte type = 0;
	uint16 lenName = 0;
	try
	{
		type = readByte();
		lenName = readUInt16();
		name = new char[lenName+1];
		name[lenName] = 0;
		readArray(name, lenName);

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
				BYTE value[16];
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
				BYTE* value = readBsob(&size);
				try{
					retVal = new CTagBsob(name, value, size);
				}
				catch(CException*){
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
				throw new CNotSupportedException;
		}
		delete [] name;
		name = NULL;
	}
	catch (...)
	{
		DebugLogError(_T("Invalid Kad tag; type=0x%02x  lenName=%u  name=0x%02x"), type, lenName, name!=NULL ? (BYTE)name[0] : 0);
		delete[] name;
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
	writeArray(&val, sizeof(uint16));
}

void CDataIO::writeUInt32(uint32 val)
{
	writeArray(&val, sizeof(uint32));
}

void CDataIO::writeUInt128(const CUInt128& val)
{
	writeArray(val.getData(), sizeof(uint32)*4);
}

void CDataIO::writeFloat(float val)
{
	writeArray(&val, sizeof(float));
}

void CDataIO::writeHash(const BYTE* value)
{
	writeArray(value, 16);
}

void CDataIO::writeBsob(const BYTE* value, uint8 size)
{
	writeUInt8(size);
	writeArray(value, size);
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

		const CTagNameString& name = tag->m_name;
		writeUInt16(name.GetLength());
		writeArray((LPCSTR)name, name.GetLength());

		switch (type)
		{
			case TAGTYPE_HASH:
				// Do NOT use this to transfer any tags for at least half a year!!
				writeHash(tag->GetHash());
				ASSERT(0);
				break;
			case TAGTYPE_STRING:
			{
				CUnicodeToUTF8 utf8(tag->GetStr());
				writeUInt16(utf8.GetLength());
				writeArray(utf8, utf8.GetLength());
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
				ASSERT(0);
				break;
			case TAGTYPE_UINT16:
				writeUInt16(tag->GetInt());
				break;
			case TAGTYPE_UINT8:
				writeUInt8(tag->GetInt());
				break;
		}
	} 
	catch (CIOException *ioe)
	{
		AddDebugLogLine( false, _T("Exception in CDataIO:writeTag (IO Error(%i))"), ioe->m_cause);
		throw ioe;
	}
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CDataIO:writeTag"));
		throw;
	}
}

void CDataIO::writeTag(LPCSTR name, uint32 value)
{
	CTagUInt32 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, uint16 value)
{
	CTagUInt16 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, uint8 value)
{
	CTagUInt8 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, float value)
{
	CTagFloat tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTagList(const TagList& tagList)
{
	uint32 count = (uint32)tagList.size();
	ASSERT( count <= 0xFF );
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

static WCHAR _awcLowerMap[0x10000];

bool CKademlia::initUnicode(HMODULE hInst)
{
	bool bResult = false;
	HRSRC hResInfo = FindResource(hInst, MAKEINTRESOURCE(IDR_WIDECHARLOWERMAP), _T("WIDECHARMAP"));
	if (hResInfo)
	{
		HGLOBAL hRes = LoadResource(hInst, hResInfo);
		if (hRes)
		{
			LPBYTE pRes = (LPBYTE)LockResource(hRes);
			if (pRes)
			{
				if (SizeofResource(hInst, hResInfo) == sizeof _awcLowerMap)
				{
					memcpy(_awcLowerMap, pRes, sizeof _awcLowerMap);
					if (_awcLowerMap[L'A'] == L'a' && _awcLowerMap[L'Z'] == L'z')
						bResult = true;
				}
				UnlockResource(hRes);
			}
			FreeResource(hRes);
		}
	}
	return bResult;
}

void KadTagStrMakeLower(CTagValueString& rwstr)
{
	// NOTE: We can *not* use any locale dependant string functions here. All clients in the network have to
	// use the same character mapping whereby it actually does not matter if they 'understand' the strings
	// or not -- they just have to use the same mapping. That's why we hardcode to 'LANG_ENGLISH' here!
	// Note also, using 'LANG_ENGLISH' is not the same as using the "C" locale. The "C" locale would only
	// handle ASCII-7 characters while the 'LANG_ENGLISH' locale also handles chars from 0x80-0xFF and more.
	//rwstr.MakeLower();

#if 0
	//PROBLEM: LCMapStringW does not work on Win9x (the string is not changed and LCMapStringW returns 0!)
	// Possible solution: use a pre-computed static character map..
	int iLen = rwstr.GetLength();
	LPWSTR pwsz = rwstr.GetBuffer(iLen);
	int iSize = LCMapStringW(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT),
							 LCMAP_LOWERCASE, pwsz, -1, pwsz, iLen + 1);
	ASSERT( iSize - 1 == iLen );
	rwstr.ReleaseBuffer(iLen);
#else
	// NOTE: It's very important that the Unicode->LowerCase map already was initialized!
	if (_awcLowerMap[L'A'] != L'a'){
		AfxMessageBox(_T("Kad Unicode lowercase character map not initialized!"));
		exit(1);
	}

	int iLen = rwstr.GetLength();
	LPWSTR pwsz = rwstr.GetBuffer(iLen);
	while ((*pwsz = _awcLowerMap[*pwsz]) != L'\0')
		pwsz++;
	rwstr.ReleaseBuffer(iLen);
#endif
}
