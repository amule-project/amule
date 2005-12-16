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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef TAG_H
#define TAG_H

#include <list>

#include "Types.h"		// Needed for int8, int32, uint8 and uint32
#include "OPCodes.h"		// Needed for TAGTYPE_*
#include <common/StringFunctions.h>	// Needed for EUtf8Str

class CMD4Hash;
class CFileDataIO;
class wxString;

///////////////////////////////////////////////////////////////////////////////
// CTag

class CTag
{
public:
	CTag(char* pszName, uint32 uVal);
	CTag(uint8 uName, uint32 uVal);
	CTag(char* pszName, const wxString& rstrVal);
	CTag(uint8 uName, const wxString& rstrVal);
	CTag(uint8 uName, const CMD4Hash& hash);
	CTag(uint8 uName, uint32 nSize, const unsigned char* pucData);
	CTag(const CTag& rTag);
	CTag(const CFileDataIO& data, bool bOptUTF8);
	~CTag();

	uint8 GetType() const			{ return m_uType; }
	uint8 GetNameID() const			{ return m_uName; }
	char* GetName() const			{ return m_pszName; }
	
	bool IsStr() const				{ return m_uType == TAGTYPE_STRING; }
	bool IsInt() const				{ return m_uType == TAGTYPE_UINT32; }
	bool IsFloat() const			{ return m_uType == TAGTYPE_FLOAT32; }
	bool IsHash() const				{ return m_uType == TAGTYPE_HASH; }
	bool IsBlob() const				{ return m_uType == TAGTYPE_BLOB; }
	
	uint32 GetInt() const;
	const wxString& GetStr() const;
	float GetFloat() const;
	const CMD4Hash& GetHash() const;
	uint32 GetBlobSize() const;
	const byte* GetBlob() const;

	void SetInt(uint32 uVal);
	
	CTag* CloneTag()				{ return new CTag(*this); }
	
	bool WriteTagToFile(CFileDataIO* file, EUtf8Str eStrEncode = utf8strNone) const;	// old eD2K tags
	bool WriteNewEd2kTag(CFileDataIO* file, EUtf8Str eStrEncode = utf8strNone) const;	// new eD2K tags
	
	wxString GetFullInfo() const;

private:
	//! CTag is not assignable.
	CTag& operator=(const CTag&);
	
	uint8	m_uType;
	uint8	m_uName;
	char*	m_pszName;
	uint32	m_nBlobSize;
	union {
	  CMD4Hash*	m_hashVal;
	  wxString*	m_pstrVal;
	  uint32	m_uVal;
	  float		m_fVal;
	  unsigned char*		m_pData;
	};
};

typedef std::list<CTag*> TagPtrList;

///////////////////////////////////////////////////////////////////////////////
// CTag and tag string helpers

inline int CmpED2KTagName(const char* pszTagName1, const char* pszTagName2){
	// string compare is independant from any codepage and/or LC_CTYPE setting.
	return strcasecmp(pszTagName1, pszTagName2);
}
void ConvertED2KTag(CTag*& pTag);

bool WriteOptED2KUTF8Tag(CFileDataIO* data, const wchar_t* pwsz, uint8 uTagName);

#endif // TAG_H
