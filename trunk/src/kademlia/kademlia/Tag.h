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

#ifndef __KAD_TAG_H__
#define __KAD_TAG_H__

#include <list>
#include "../../OPCodes.h"
#include "../../OtherFunctions.h"
#include <wx/string.h>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;

class CTagNameString : public wxString
{
public:
	CTagNameString()
	{
	}

	CTagNameString(const wxString& psz)
		: wxString(psz)
	{
	}

	CTagNameString(wxChar* psz, int len)
		: wxString(psz, len)
	{
	}

	// A tag name may include character values >= 0xD0 and therefor also >= 0xF0. to prevent those
	// characters be interpreted as multi byte character sequences we have to ensure that a binary
	// string compare is performed.
	int Compare(const wxString& psz) const throw()
	{
		// Do a binary string compare. (independant from any codepage and/or LC_CTYPE setting.)
		return Cmp(psz);
	}	
	
	int CompareNoCase(const wxString& psz) const throw()
	{
		return CmpNoCase(psz);
	}

	CTagNameString& operator=(const wxString& pszSrc)
	{
		wxString::operator=(pszSrc);
		return *this;
	}
	
	const wxString	GetString() const {
		return (wxString) GetData();
	}

#if !wxUSE_STL	
	wxChar* GetBuffer(int nMinBufferLength)
	{
		return wxString::GetWriteBuf(nMinBufferLength);
	}

	void ReleaseBuffer() {
		wxString::UngetWriteBuf();
	}
#else
	#warning Need a replacement for wxString::GetWriteBuf()/UngetWriteBuf() !
#endif

	int GetLength() const throw()
	{
		return wxString::Len();
	}
};


#define CTagValueString wxString

class CTag
{
public:
	byte	m_type;
	CTagNameString m_name;

	CTag(byte type, const wxString& name)
		: m_name(name)
	{
		m_type = type;
	}
	
	#if wxUSE_UNICODE
	CTag(byte type, const char* name)
		: m_name(wxString::FromAscii(name))
	{
		m_type = type;
	}	
	#endif
	
	virtual ~CTag() {}

	bool IsStr()  const { return m_type == TAGTYPE_STRING; }
	bool IsNum()  const { return m_type == TAGTYPE_UINT32 || m_type == TAGTYPE_UINT16 || m_type == TAGTYPE_UINT8 || m_type == TAGTYPE_BOOL || m_type == TAGTYPE_FLOAT32 || m_type == 0xFE; }
	bool IsInt()  const { return m_type == TAGTYPE_UINT32 || m_type == TAGTYPE_UINT16 || m_type == TAGTYPE_UINT8 || m_type == 0xFE; }
	bool IsFloat()const { return m_type == TAGTYPE_FLOAT32; }
	bool IsBsob() const { return m_type == TAGTYPE_BSOB; }
	bool IsHash() const { return m_type == TAGTYPE_HASH; }

	virtual CTagValueString GetStr() const { wxASSERT(0); return wxT(""); }
	virtual uint32 GetInt() const { wxASSERT(0); return 0; }
	virtual float GetFloat() const { wxASSERT(0); return 0.0F; }
	virtual const byte* GetBsob() const { wxASSERT(0); return NULL; }
	virtual uint8 GetBsobSize() const { wxASSERT(0); return 0; }
	virtual bool GetBool() const { wxASSERT(0); return false; }
	virtual const byte* GetHash() const { wxASSERT(0); return NULL; }

protected:
	CTag() {}
};


class CTagUnk : public CTag
{
public:
	CTagUnk(byte type, const wxString& name)
		: CTag(type, name)
	{ }
	#if wxUSE_UNICODE
	CTagUnk(byte type, const char* name)
		: CTag(type, wxString::FromAscii(name))
	{ }	
	#endif
};


class CTagStr : public CTag
{
public:
	CTagStr(const wxString& name, const wxString& value)
		: CTag(TAGTYPE_STRING, name)
		, m_value(value)
	{ }
	
	#if wxUSE_UNICODE
	CTagStr(const char* name, const wxString& value)
		: CTag(TAGTYPE_STRING, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif

	virtual CTagValueString GetStr() const { return m_value; }

protected:
	CTagValueString m_value;
};


class CTagUInt : public CTag
{
public:
	CTagUInt(const wxString& name, uint32 value)
		: CTag(0xFE, name)
		, m_value(value)
	{ }
	
	#if wxUSE_UNICODE
	CTagUInt(const char* name, uint32 value)
		: CTag(0xFE, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif


	virtual uint32 GetInt() const { return m_value; }

protected:
	uint32 m_value;
};


class CTagUInt32 : public CTag
{
public:
	CTagUInt32(const wxString& name, uint32 value)
		: CTag(TAGTYPE_UINT32, name)
		, m_value(value)
	{ }
	
	#if wxUSE_UNICODE
	CTagUInt32(const char* name, uint32 value)
		: CTag(TAGTYPE_UINT32, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint32 m_value;
};


class CTagFloat : public CTag
{
public:
	CTagFloat(const wxString& name, float value)
		: CTag(TAGTYPE_FLOAT32, name)
		, m_value(value)
	{ }

	#if wxUSE_UNICODE
	CTagFloat(const char* name, float value)
		: CTag(TAGTYPE_FLOAT32, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif
	
	virtual float GetFloat() const { return m_value; }

protected:
	float m_value;
};


class CTagBool : public CTag
{
public:
	CTagBool(const wxString& name, bool value)
		: CTag(TAGTYPE_BOOL, name)
		, m_value(value)
	{ }
	
	#ifdef wxUSE_UNICODE
	CTagBool(const char* name, bool value)
		: CTag(TAGTYPE_BOOL, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif	

	virtual bool GetBool() const { return m_value; }

protected:
	bool m_value;
};


class CTagUInt16 : public CTag
{
public:

	CTagUInt16(const wxString& name, uint16 value)
		: CTag(TAGTYPE_UINT16, name)
		, m_value(value)
	{ }
	
	#if wxUSE_UNICODE
	CTagUInt16(const char* name, uint16 value)
		: CTag(TAGTYPE_UINT16, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint16 m_value;
};


class CTagUInt8 : public CTag
{
public:
	CTagUInt8(const wxString& name, uint8 value)
		: CTag(TAGTYPE_UINT8, name)
		, m_value(value)
	{ }

	#ifdef wxUSE_UNICODE
	CTagUInt8(const char* name, uint8 value)
		: CTag(TAGTYPE_UINT8, wxString::FromAscii(name))
		, m_value(value)
	{ }
	#endif

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint8 m_value;
};


class CTagBsob : public CTag
{
public:
	CTagBsob(const wxString& name, const byte* value, uint8 nSize)
		: CTag(TAGTYPE_BSOB, name)
	{
		m_value = new byte[nSize];
		memcpy(m_value, value, nSize);
		m_size = nSize;
	}
	
	#if wxUSE_UNICODE
	CTagBsob(const char* name, const byte* value, uint8 nSize)
		: CTag(TAGTYPE_BSOB, wxString::FromAscii(name))
	{
		m_value = new byte[nSize];
		memcpy(m_value, value, nSize);
		m_size = nSize;
	}	
	#endif

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


class CTagHash : public CTag
{
public:
	CTagHash(const wxString& name, const byte* value) 
		: CTag(TAGTYPE_HASH, name)
	{ 
		m_value = new byte[16];
		md4cpy(m_value, value);
	}
	
	#if wxUSE_UNICODE
	CTagHash(const char* name, const byte* value) 
		: CTag(TAGTYPE_HASH, wxString::FromAscii(name))
	{ 
		m_value = new byte[16];
		md4cpy(m_value, value);
	}
	#endif	

	~CTagHash()
	{
		delete[] m_value;
	}

	virtual const byte* GetHash() const { return m_value; }

protected:
	byte* m_value;
};

typedef std::list<CTag*> TagList;


} // End namespace


void KadTagStrMakeLower(CTagValueString& rstr);

#endif //__KAD_TAG_H__
