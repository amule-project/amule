//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef TAG_H
#define TAG_H


#include <common/StringFunctions.h>	// Needed for EUtf8Str

#include <tags/TagTypes.h>

#include "OtherFunctions.h"

class CMD4Hash;
class CFileDataIO;

///////////////////////////////////////////////////////////////////////////////
// CTag

class CTag
{
public:
	CTag(const CTag& rTag);
	CTag(const CFileDataIO& data, bool bOptUTF8);
	virtual ~CTag();
	CTag& operator=(const CTag&);

	uint8 GetType() const		{ return m_uType; }
	uint8 GetNameID() const		{ return m_uName; }
	const wxString& GetName() const	{ return m_Name; }

	bool IsStr() const		{ return m_uType == TAGTYPE_STRING; }
	bool IsInt() const		{ return
		(m_uType == TAGTYPE_UINT64) ||
		(m_uType == TAGTYPE_UINT32) ||
		(m_uType == TAGTYPE_UINT16) ||
		(m_uType == TAGTYPE_UINT8); }
	bool IsFloat() const		{ return m_uType == TAGTYPE_FLOAT32; }
	bool IsHash() const		{ return m_uType == TAGTYPE_HASH16; }
	bool IsBlob() const		{ return m_uType == TAGTYPE_BLOB; }
	bool IsBsob() const		{ return m_uType == TAGTYPE_BSOB; }

	uint64 GetInt() const;

	const wxString& GetStr() const;

	float GetFloat() const;

	const CMD4Hash& GetHash() const;

	const byte* GetBlob() const;
	uint32 GetBlobSize() const;

	const byte* GetBsob() const;
	uint32 GetBsobSize() const;

	CTag* CloneTag()		{ return new CTag(*this); }

	bool WriteTagToFile(CFileDataIO* file,
		EUtf8Str eStrEncode = utf8strNone,
		bool restrictive = true) const;			// old eD2K tags
	bool WriteNewEd2kTag(CFileDataIO* file,
		EUtf8Str eStrEncode = utf8strNone) const;	// new eD2K tags

	wxString GetFullInfo() const;

protected:
	CTag(const wxString& Name);
	CTag(uint8 uName);

	uint8	m_uType;
	union {
		CMD4Hash*	m_hashVal;
		wxString*	m_pstrVal;
		uint64		m_uVal;
		float		m_fVal;
		unsigned char*	m_pData;
	};

	uint32		m_nSize;

private:
	uint8		m_uName;
	wxString	m_Name;

};

typedef std::list<CTag*> TagPtrList;

class CTagIntSized : public CTag
{
public:
	CTagIntSized(const wxString& name, uint64 value, uint8 bitsize)
		: CTag(name) {
			Init(value, bitsize);
		}

	CTagIntSized(uint8 name, uint64 value, uint8 bitsize)
		: CTag(name) {
			Init(value, bitsize);
		}

protected:
	CTagIntSized(const wxString& name) : CTag(name) {}
	CTagIntSized(uint8 name) : CTag(name) {}

	void Init(uint64 value, uint8 bitsize) {
			switch (bitsize) {
				case 64:
					wxASSERT(value <= ULONGLONG(0xFFFFFFFFFFFFFFFF));
					m_uVal = value;
					m_uType = TAGTYPE_UINT64;
					break;
				case 32:
					wxASSERT(value <= 0xFFFFFFFF);
					m_uVal = value;
					m_uType = TAGTYPE_UINT32;
					break;
				case 16:
					wxASSERT(value <= 0xFFFF);
					m_uVal = value;
					m_uType = TAGTYPE_UINT16;
					break;
				case 8:
					wxASSERT(value <= 0xFF);
					m_uVal = value;
					m_uType = TAGTYPE_UINT8;
					break;
				default:
					throw wxString(wxT("Invalid bitsize on int tag"));
			}
	}
};

class CTagVarInt : public CTagIntSized
{
public:
	CTagVarInt(const wxString& name, uint64 value, uint8 forced_bits = 0)
		: CTagIntSized(name) {
			SizedInit(value, forced_bits);
		}
	CTagVarInt(uint8 name, uint64 value, uint8 forced_bits = 0)
		: CTagIntSized(name) {
			SizedInit(value, forced_bits);
		}
private:
	void SizedInit(uint64 value, uint8 forced_bits) {
		if (forced_bits) {
			// The bitsize was forced.
			Init(value,forced_bits);
		} else {
			m_uVal = value;
			if (value <= 0xFF) {
				m_uType = TAGTYPE_UINT8;
			} else if (value <= 0xFFFF) {
				m_uType = TAGTYPE_UINT16;
			} else if (value <= 0xFFFFFFFF) {
				m_uType = TAGTYPE_UINT32;
			} else {
				m_uType = TAGTYPE_UINT64;
			}
		}
	}
};

class CTagInt64 : public CTagIntSized
{
public:
	CTagInt64(const wxString& name, uint64 value)
		: CTagIntSized(name, value, 64) {	}

	CTagInt64(uint8 name, uint64 value)
		: CTagIntSized(name, value, 64) { }
};

class CTagInt32 : public CTagIntSized
{
public:
	CTagInt32(const wxString& name, uint64 value)
		: CTagIntSized(name, value, 32) {	}

	CTagInt32(uint8 name, uint64 value)
		: CTagIntSized(name, value, 32) { }
};

class CTagInt16 : public CTagIntSized
{
public:
	CTagInt16(const wxString& name, uint64 value)
		: CTagIntSized(name, value, 16) {	}

	CTagInt16(uint8 name, uint64 value)
		: CTagIntSized(name, value, 16) { }
};

class CTagInt8 : public CTagIntSized
{
public:
	CTagInt8(const wxString& name, uint64 value)
		: CTagIntSized(name, value, 8) {	}

	CTagInt8(uint8 name, uint64 value)
		: CTagIntSized(name, value, 8) { }
};

class CTagFloat : public CTag
{
public:
	CTagFloat(const wxString& name, float value)
		: CTag(name) {
			m_fVal = value;
			m_uType = TAGTYPE_FLOAT32;
		}

	CTagFloat(uint8 name, float value)
		: CTag(name) {
			m_fVal = value;
			m_uType = TAGTYPE_FLOAT32;
		}
};

class CTagString : public CTag
{
public:
	CTagString(const wxString& name, const wxString& value)
		: CTag(name) {
			m_pstrVal = new wxString(value);
			m_uType = TAGTYPE_STRING;
		}

	CTagString(uint8 name, const wxString& value)
		: CTag(name) {
			m_pstrVal = new wxString(value);
			m_uType = TAGTYPE_STRING;
		}
};

class CTagHash : public CTag
{
public:
	// Implementation on .cpp to allow forward declaration of CMD4Hash
	CTagHash(const wxString& name, const CMD4Hash& value);
	CTagHash(uint8 name, const CMD4Hash& value);
};

class CTagBsob : public CTag
{
public:
	CTagBsob(const wxString& name, const byte* value, uint8 nSize)
		: CTag(name)
	{
		m_uType = TAGTYPE_BSOB;
		m_pData = new byte[nSize];
		memcpy(m_pData, value, nSize);
		m_nSize = nSize;
	}
};

class CTagBlob : public CTag
{
public:
	CTagBlob(const wxString& name, const byte* value, uint8 nSize)
		: CTag(name)
	{
		m_uType = TAGTYPE_BLOB;
		m_pData = new byte[nSize];
		memcpy(m_pData, value, nSize);
		m_nSize = nSize;
	}
};

void deleteTagPtrListEntries(TagPtrList* taglist);

#endif // TAG_H
// File_checked_for_headers
