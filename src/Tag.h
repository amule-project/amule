//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/string.h>
#include <common/StringFunctions.h>	// Needed for EUtf8Str

#include "Types.h"		// Needed for int8, int32, uint8 and uint32
#include "OPCodes.h"		// Needed for TAGTYPE_*
#include "OtherFunctions.h"

class CMD4Hash;
class CFileDataIO;

///////////////////////////////////////////////////////////////////////////////
// CTag

class CTag
{
public:
	CTag(const wxString& Name, float uVal);
	CTag(const wxString& Name, uint32 uVal);
	CTag(uint8 uName, uint32 uVal);
	CTag(const wxString& Name, const wxString& rstrVal);
	CTag(uint8 uName, const wxString& rstrVal);
	CTag(const wxString& Name, const CMD4Hash& hash);
	CTag(uint8 uName, const CMD4Hash& hash);
	CTag(uint8 uName, uint32 nSize, const unsigned char* pucData);
	CTag(const CTag& rTag);
	CTag(const CFileDataIO& data, bool bOptUTF8);
	virtual ~CTag();

	uint8 GetType() const			{ return m_uType; }
	uint8 GetNameID() const			{ return m_uName; }
	const wxString& GetName() const	{ return m_Name; }
	
	bool IsStr() const				{ return m_uType == TAGTYPE_STRING; }
	bool IsInt() const				{ return 
											(m_uType == TAGTYPE_UINT32) ||
											(m_uType == TAGTYPE_UINT16) ||
											(m_uType == TAGTYPE_UINT8) ||
											(m_uType == TAGTYPE_KADSPECIALINT)
											; }
	bool IsFloat() const			{ return m_uType == TAGTYPE_FLOAT32; }
	bool IsHash() const				{ return m_uType == TAGTYPE_HASH; }
	bool IsBlob() const				{ return m_uType == TAGTYPE_BLOB; }
	bool IsBsob() const				{ return m_uType == TAGTYPE_BSOB; }
	
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

	virtual const byte* GetBsob() const { wxASSERT(0); return NULL; }
	virtual uint8 GetBsobSize() const { wxASSERT(0); return 0; }
	
protected:
	uint8	m_uType;
	union {
	  CMD4Hash*	m_hashVal;
	  wxString*	m_pstrVal;
	  uint32	m_uVal;
	  float		m_fVal;
	  unsigned char*		m_pData;
	};

private:
	//! CTag is not assignable.
	CTag& operator=(const CTag&);
	
	uint8	m_uName;
	wxString	m_Name;
	uint32	m_nBlobSize;
	
};

typedef std::list<CTag*> TagPtrList;

class CTagUInt : public CTag
{
public:
	CTagUInt(const wxString& name, uint32 value)
		: CTag(name, value)
	{ m_uType = TAGTYPE_KADSPECIALINT; }
};


class CTagUInt16 : public CTag
{
public:

	CTagUInt16(const wxString& name, uint16 value)
		: CTag(name, (uint32)value)
	{ m_uType = TAGTYPE_UINT16; }
};


class CTagUInt8 : public CTag
{
public:
	CTagUInt8(const wxString& name, uint8 value)
		: CTag(name, (uint32)value)
	{ m_uType = TAGTYPE_UINT8; }
};


class CTagBsob : public CTag
{
public:
	CTagBsob(const wxString& name, const byte* value, uint8 nSize)
		: CTag(name, (uint32)0)
	{
		m_uType = TAGTYPE_BSOB;
		m_value = new byte[nSize];
		memcpy(m_value, value, nSize);
		m_size = nSize;
	}

	~CTagBsob()
	{
		delete[] m_value;
	}

	virtual const byte* GetBsob() const { return m_value; }
	virtual uint8 GetBsobSize() const { return m_size; }

protected:
	byte* m_value;
	uint8 m_size;
};

void deleteTagPtrListEntries(TagPtrList* taglist);

#endif // TAG_H
