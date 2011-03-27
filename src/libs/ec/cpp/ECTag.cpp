//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifdef __DEBUG__
#define DEBUG_EC_IMPLEMENTATION

#include <common/Format.h>  // Needed for CFormat
#endif

#include "ECTag.h"	// Needed for ECTag
#include "ECSocket.h"	// Needed for CECSocket
#include "ECSpecialTags.h"	// Needed for CValueMap
#include "ECID.h"	// Needed for CECID

/**********************************************************
 *							  *
 *	CECTag class					  *
 *							  *
 **********************************************************/

//! Defines the Null tag which may be returned by GetTagByNameSafe.
const CECTag CECTag::s_theNullTag;

/**
 * Creates a new null-valued CECTag instance
 *
 * @see s_theNullTag
 * @see GetTagByNameSafe
 */
CECTag::CECTag() :
	m_tagName(0),
	m_dataType(EC_TAGTYPE_UNKNOWN),
	m_dataLen(0),
	m_tagData(NULL)	// All access functions check m_dataType, so no need to allocate a dummy buffer.
{
}

/**
 * Creates a new CECTag instance from the given data
 *
 * @param name	 TAG name
 * @param length length of data buffer
 * @param data	 TAG data
 *
 */
CECTag::CECTag(ec_tagname_t name, unsigned int length, const void *data) : m_tagName(name)
{
	if (data) {
		m_dataLen = length;
		NewData();
		memcpy(m_tagData, data, m_dataLen);
	} else {
		wxASSERT(length == 0);
		m_dataLen = 0;
		m_tagData = NULL;
	}
	m_dataType = EC_TAGTYPE_CUSTOM;
}

/**
 * Creates a new CECTag instance for custom data
 *
 * @param name	 	TAG name
 * @param length 	length of data buffer that will be alloc'ed
 * @param dataptr	pointer to a void pointer which will be assigned the internal TAG data buffer
 *
 * \note TAG data buffer has to be filled with valid data after the ctor
 */
CECTag::CECTag(ec_tagname_t name, unsigned int length, void **dataptr)  : m_tagName(name)
{
	m_dataLen = length;
	NewData();
	*dataptr = m_tagData;
	m_dataType = EC_TAGTYPE_CUSTOM;
}

/**
 * Creates a new CECTag instance, which contains an IPv4 address.
 *
 * This function takes care of the endianness of the port number.
 *
 * @param name TAG name
 * @param data The EC_IPv4_t class containing the IPv4 address.
 *
 * @see GetIPv4Data()
 */
CECTag::CECTag(ec_tagname_t name, const EC_IPv4_t& data) : m_tagName(name)
{

	m_dataLen = sizeof(EC_IPv4_t);
	NewData();
	RawPokeUInt32( ((EC_IPv4_t *)m_tagData)->m_ip, RawPeekUInt32( data.m_ip ) );
	((EC_IPv4_t *)m_tagData)->m_port = ENDIAN_HTONS(data.m_port);
	m_dataType = EC_TAGTYPE_IPV4;
}

/**
 * Creates a new CECTag instance, which contains a MD4 hash.
 *
 * This function takes care to store hash in network byte order.
 *
 * @param name TAG name
 * @param data The CMD4Hash class containing the MD4 hash.
 *
 * @see GetMD4Data()
 */
CECTag::CECTag(ec_tagname_t name, const CMD4Hash& data) : m_tagName(name)
{
	m_dataLen = 16;
	NewData();
	RawPokeUInt64( m_tagData,		RawPeekUInt64( data.GetHash() ) );
	RawPokeUInt64( m_tagData + 8,	RawPeekUInt64( data.GetHash() + 8 ) );
	m_dataType = EC_TAGTYPE_HASH16;
}

/**
 * Creates a new CECTag instance, which contains a string
 *
 * @param name TAG name
 * @param data wxString object, it's contents are converted to UTF-8.
 *
 * @see GetStringDataSTL()
 */
CECTag::CECTag(ec_tagname_t name, const std::string& data) : m_tagName(name)
{
	ConstructStringTag(name, data);
}

/**
 * Creates a new CECTag instance, which contains a string
 *
 * @param name TAG name
 * @param data wxString object, it's contents are converted to UTF-8.
 *
 * @see GetStringData()
 */
CECTag::CECTag(ec_tagname_t name, const wxString& data)
{
	ConstructStringTag(name, (const char*)unicode2UTF8(data));
}
CECTag::CECTag(ec_tagname_t name, const wxChar* data)
{
	ConstructStringTag(name, (const char*)unicode2UTF8(data));
}

/**
 * Copy constructor
 */
CECTag::CECTag(const CECTag& tag)
{
	m_tagData = NULL;
	*this = tag;
}

/**
 * Creates a new CECTag instance, which contains an int value.
 *
 * This takes care of endianness problems with numbers.
 *
 * @param name TAG name.
 * @param data number.
 *
 * @see GetInt()
 */
CECTag::CECTag(ec_tagname_t name, bool data) : m_tagName(name)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint8 data) : m_tagName(name)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint16 data) : m_tagName(name)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint32 data) : m_tagName(name)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint64 data) : m_tagName(name)
{
	InitInt(data);
}

void CECTag::InitInt(uint64 data)
{
	if (data <= 0xFF) {
		m_dataType = EC_TAGTYPE_UINT8;
		m_dataLen = 1;
	} else if (data <= 0xFFFF) {
		m_dataType = EC_TAGTYPE_UINT16;
		m_dataLen = 2;
	} else if (data <= 0xFFFFFFFF) {
		m_dataType = EC_TAGTYPE_UINT32;
		m_dataLen = 4;
	} else {
		m_dataType = EC_TAGTYPE_UINT64;
		m_dataLen = 8;
	}	
	
	NewData();
	
	switch (m_dataType) {
		case EC_TAGTYPE_UINT8:
			PokeUInt8( m_tagData, (uint8) data );
			break;
		case EC_TAGTYPE_UINT16:
			PokeUInt16( m_tagData, wxUINT16_SWAP_ALWAYS((uint16) data ));
			break;
		case EC_TAGTYPE_UINT32:
			PokeUInt32( m_tagData, wxUINT32_SWAP_ALWAYS((uint32) data ));
			break;
		case EC_TAGTYPE_UINT64:
			PokeUInt64( m_tagData, wxUINT64_SWAP_ALWAYS(data) );
			break;
	}
}

/**
 * Creates a new CECTag instance, which contains a double precision floating point number
 *
 * @param name TAG name
 * @param data double number
 *
 * @note The actual data is converted to string representation, because we have not found
 * yet an effective and safe way to transmit floating point numbers.
 *
 * @see GetDoubleData()
 */
CECTag::CECTag(ec_tagname_t name, double data) : m_tagName(name)
{
	std::ostringstream double_str;
	double_str << data;
	std::string double_string = double_str.str();
	const char * double_chr = double_string.c_str();
	m_dataLen = (ec_taglen_t)strlen(double_chr) + 1;
	NewData();
	memcpy(m_tagData, double_chr, m_dataLen);
	m_dataType = EC_TAGTYPE_DOUBLE;
}

/**
 * Destructor - frees allocated data and deletes child TAGs.
 */
CECTag::~CECTag(void)
{
	delete [] m_tagData;
}

/**
 * Copy assignment operator.
 *
 * std::vector uses this, but the compiler-supplied version wouldn't properly
 * handle m_dynamic and m_tagData.  This wouldn't be necessary if m_tagData
 * was a smart pointer (Hi, Kry!).
 */
CECTag& CECTag::operator=(const CECTag& tag)
{
	if (&tag != this) {
		m_tagName = tag.m_tagName;
		m_dataLen = tag.m_dataLen;
		m_dataType = tag.m_dataType;
		delete [] m_tagData;
		if (m_dataLen != 0) {
			NewData();
			memcpy(m_tagData, tag.m_tagData, m_dataLen);
		} else {
			m_tagData = NULL;
		}
		m_tagList.clear();
		for (const_iterator it = tag.begin(); it != tag.end(); it++) {
			m_tagList.push_back(*it);
		}
	}
	return *this;
}

/**
 * Compare operator.
 *
 */
bool CECTag::operator==(const CECTag& tag) const
{
	return	m_dataType == tag.m_dataType
			&& m_tagName == tag.m_tagName
			&& m_dataLen == tag.m_dataLen
			&&	(m_dataLen == 0
				|| !memcmp(m_tagData, tag.m_tagData, m_dataLen))
			&& m_tagList == tag.m_tagList;
}

/**
 * Add a child tag to this one. The tag argument is reset to an empty tag.
 *
 * Be very careful that this method swallows the content of \e tag, leaving \e tag empty. 
 * Thus, the following code won't work as expected:
 * \code
 * {
 *	CECPacket *p = new CECPacket(whatever);
 *	CECTag *t1 = new CECTag(whatever);
 *	CECTag *t2 = new CECTag(whatever);
 *	p->AddTag(*t1);
 *	t1->AddTag(*t2);	// t2 won't be part of p !!!
 * }
 * \endcode
 *
 * To get the desired results, the above should be replaced with something like:
 *
 * \code
 * {
 *	CECPacket *p = new CECPacket(whatever);
 *	CECTag *t1 = new CECTag(whatever);
 *	CECTag *t2 = new CECTag(whatever);
 *	t1->AddTag(*t2);
 *	delete t2;	// we can safely delete the now empty t2 here, because t1 holds its content
 *	p->AddTag(*t1);
 *	delete t1;	// now p holds the content of both t1 and t2
 * }
 * \endcode
 *
 * Then why copying? The answer is to enable simplifying the code like this:
 *
 * \code
 * {
 *	CECPacket *p = new CECPacket(whatever);
 *	CECTag t1(whatever);
 *	t1.AddTag(CECTag(whatever));	// t2 is now created on-the-fly
 *	p->AddTag(t1);	// now p holds a copy of both t1 and t2
 * }
 * \endcode
 *
 * @param tag a CECTag class instance to add.
 * @return \b true if tag was really added, 
 * \b false when it was omitted through valuemap.
 */
bool CECTag::AddTag(const CECTag& tag, CValueMap* valuemap)
{
	if (valuemap) {
		return valuemap->AddTag(tag, this);
	}
	// cannot have more than 64k tags
	wxASSERT(m_tagList.size() < 0xffff);

	// First add an empty tag.
	m_tagList.push_back(CECEmptyTag());
	// Then exchange the data. The original tag will be destroyed right after this call anyway.
	// UGLY - GCC allows a reference to an in place constructed object only to be passed as const.
	// So pass it the way it wants it and then cheat and cast it back to non-const. :-/
	CECTag& wtag = const_cast<CECTag&>(tag);
	wtag.swap(m_tagList.back());
	return true;
}

void CECTag::AddTag(ec_tagname_t name, uint64_t data, CValueMap* valuemap)
{
	if (valuemap) {
		valuemap->CreateTag(name, data, this);
	} else {
		AddTag(CECTag(name, data));
	}
}

void CECTag::AddTag(ec_tagname_t name, const wxString& data, CValueMap* valuemap)
{
	if (valuemap) {
		valuemap->CreateTag(name, data, this);
	} else {
		AddTag(CECTag(name, data));
	}
}

void CECTag::AddTag(ec_tagname_t name, const CMD4Hash& data, CValueMap* valuemap)
{
	if (valuemap) {
		valuemap->CreateTag(name, data, this);
	} else {
		AddTag(CECTag(name, data));
	}
}

void CECTag::swap(CECTag& t2)
{
	std::swap(m_tagName, t2.m_tagName);
	std::swap(m_dataType, t2.m_dataType);
	std::swap(m_dataLen, t2.m_dataLen);
	std::swap(m_tagData, t2.m_tagData);
	std::swap(m_tagList, t2.m_tagList);
}

bool CECTag::ReadFromSocket(CECSocket& socket)
{
	ec_tagname_t tmp_tagName;
	if (!socket.ReadNumber(&tmp_tagName, sizeof(ec_tagname_t))) {
		return false;
	}
	m_tagName = tmp_tagName >> 1;
	bool hasChildren = (tmp_tagName & 0x01) != 0;
	
	if (!socket.ReadNumber(&m_dataType, sizeof(ec_tagtype_t))) {
		return false;
	}
	
	if (!socket.ReadNumber(&m_dataLen, sizeof(ec_taglen_t))) {
		return false;
	}

	if (hasChildren && !ReadChildren(socket)) {
		return false;
	}
	
	unsigned int tmp_len = m_dataLen;
	m_dataLen = 0;
	m_dataLen = tmp_len - GetTagLen();
	if (m_dataLen > 0) {
		NewData();
		if (!socket.ReadBuffer(m_tagData, m_dataLen)) {
			return false;
		}
	} else {
		m_tagData = NULL;
	}

	return true;
}


bool CECTag::WriteTag(CECSocket& socket) const
{
	ec_tagname_t tmp_tagName = (m_tagName << 1) | (m_tagList.empty() ? 0 : 1);
	ec_tagtype_t type = m_dataType;
	ec_taglen_t tagLen = GetTagLen();
	wxASSERT(type != EC_TAGTYPE_UNKNOWN);
	
	if (!socket.WriteNumber(&tmp_tagName, sizeof(ec_tagname_t))) return false;
	if (!socket.WriteNumber(&type, sizeof(ec_tagtype_t))) return false;
	if (!socket.WriteNumber(&tagLen, sizeof(ec_taglen_t))) return false;
	
	if (!m_tagList.empty()) {
		if (!WriteChildren(socket)) return false;
	}
	
	if (m_dataLen > 0) {
		if (m_tagData != NULL) {	// This is here only to make sure everything, it should not be NULL at this point
			if (!socket.WriteBuffer(m_tagData, m_dataLen)) return false;
		}
	}
	
	return true;
}

bool CECTag::ReadChildren(CECSocket& socket)
{
	uint16 tmp_tagCount;
	if (!socket.ReadNumber(&tmp_tagCount, sizeof(uint16))) {
		return false;
	}
	m_tagList.clear();
	for (int i = 0; i < tmp_tagCount; i++) {
		m_tagList.push_back(CECTag());
		CECTag& tag = m_tagList.back();
		if (!tag.ReadFromSocket(socket)) {
			return false;
		}
	}
	return true;
}

bool CECTag::WriteChildren(CECSocket& socket) const
{
	wxASSERT(m_tagList.size() < 0xFFFF);
    uint16 tmp = (uint16)m_tagList.size();
	if (!socket.WriteNumber(&tmp, sizeof(tmp))) return false;
	for (const_iterator it = begin(); it != end(); it++) {
		if (!it->WriteTag(socket)) return false;
	}
	return true;
}

/**
 * Finds the (first) child tag with given name.
 *
 * @param name TAG name to look for.
 * @return the tag found, or NULL.
 */
const CECTag* CECTag::GetTagByName(ec_tagname_t name) const
{
	for (const_iterator it = begin(); it != end(); it++) {
		if (it->m_tagName == name) return & *it;
	}
	return NULL;
}

/**
 * Finds the (first) child tag with given name.
 *
 * @param name TAG name to look for.
 * @return the tag found, or NULL.
 */
CECTag* CECTag::GetTagByName(ec_tagname_t name)
{
	for (TagList::iterator it = m_tagList.begin(); it != m_tagList.end(); it++) {
		if (it->m_tagName == name) return & *it;
	}
	return NULL;
}

/**
 * Finds the (first) child tag with given name.
 *
 * @param name TAG name to look for.
 * @return the tag found, or a special null-valued tag otherwise.
 *
 * @see s_theNullTag
 */
const CECTag* CECTag::GetTagByNameSafe(ec_tagname_t name) const
{
	const CECTag* result = GetTagByName(name);
	if (result == NULL)
		result = &s_theNullTag;
	return result;
}

/**
 * Query TAG length that is suitable for the TAGLEN field (i.e.\ 
 * without it's own header size).
 *
 * @return Tag length, containing its childs' length.
 */
uint32 CECTag::GetTagLen(void) const
{
	uint32 length = m_dataLen;
	for (const_iterator it = begin(); it != end(); it++) {
		length += it->GetTagLen();
		length += sizeof(ec_tagname_t) + sizeof(ec_tagtype_t) + sizeof(ec_taglen_t) + (it->HasChildTags() ? 2 : 0);
	}
	return length;
}


uint64_t CECTag::GetInt() const
{
	if (m_tagData == NULL) {
		// Empty tag - This is NOT an error.
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
		return 0;
	}

	switch (m_dataType) {
		case EC_TAGTYPE_UINT8:
			return PeekUInt8(m_tagData);
		case EC_TAGTYPE_UINT16:
			return ENDIAN_NTOHS( RawPeekUInt16( m_tagData ) );
		case EC_TAGTYPE_UINT32:
			return ENDIAN_NTOHL( RawPeekUInt32( m_tagData ) );
		case EC_TAGTYPE_UINT64:
			return ENDIAN_NTOHLL( RawPeekUInt64( m_tagData ) );
		case EC_TAGTYPE_UNKNOWN:
			// Empty tag - This is NOT an error.
			return 0;
		default:
			EC_ASSERT(0);
			return 0;
	}
}


std::string CECTag::GetStringDataSTL() const
{ 
	if (m_dataType != EC_TAGTYPE_STRING) {
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
		return std::string();
	} else if (m_tagData == NULL) {
		EC_ASSERT(false);
		return std::string();
	}

	return std::string(m_tagData);
}


#ifdef USE_WX_EXTENSIONS
wxString CECTag::GetStringData() const
{ 
	return UTF82unicode(GetStringDataSTL().c_str());
}
#endif


CMD4Hash CECTag::GetMD4Data() const
{ 
	if (m_dataType != EC_TAGTYPE_HASH16) {
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
		return CMD4Hash();
	}

	EC_ASSERT(m_tagData != NULL);

	// Doesn't matter if m_tagData is NULL in CMD4Hash(), 
	// that'll just result in an empty hash.
	return CMD4Hash((const unsigned char *)m_tagData); 
}


/**
 * Returns an EC_IPv4_t class.
 *
 * This function takes care of the endianness of the port number.
 *
 * @return EC_IPv4_t class.
 *
 * @see CECTag(ec_tagname_t, const EC_IPv4_t&)
 */
EC_IPv4_t CECTag::GetIPv4Data() const
{
	EC_IPv4_t p(0, 0);
	
	if (m_dataType != EC_TAGTYPE_IPV4) {
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
	} else if (m_tagData == NULL) {
		EC_ASSERT(false);
	} else {
		RawPokeUInt32( p.m_ip, RawPeekUInt32( ((EC_IPv4_t *)m_tagData)->m_ip ) );
		p.m_port = ENDIAN_NTOHS(((EC_IPv4_t *)m_tagData)->m_port);
	}

	return p;
}

/**
 * Returns a double value.
 *
 * @note The returned value is what we get by converting the string form
 * of the number to a double.
 *
 * @return The double value of the tag.
 *
 * @see CECTag(ec_tagname_t, double)
 */
double CECTag::GetDoubleData(void) const
{
	if (m_dataType != EC_TAGTYPE_DOUBLE) {
		EC_ASSERT(m_dataType == EC_TAGTYPE_UNKNOWN);
		return 0;
	} else if (m_tagData == NULL) {
		EC_ASSERT(false);
		return 0;
	}
	
	std::istringstream double_str(m_tagData);
	
	double data;
	double_str >> data;
	return data;
}


void CECTag::ConstructStringTag(ec_tagname_t name, const std::string& data)
{
	m_tagName = name;
	m_dataLen = (ec_taglen_t)strlen(data.c_str()) + 1;
	NewData();
	memcpy(m_tagData, data.c_str(), m_dataLen);
	m_dataType = EC_TAGTYPE_STRING;
}

void CECTag::SetStringData(const wxString& s)
{
	if (IsString()) {
		delete [] m_tagData;
		ConstructStringTag(m_tagName, (const char*)unicode2UTF8(s));
	}
}


bool CECTag::AssignIfExist(ec_tagname_t tagname, bool *target) const
{
	bool ret = false;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetInt() > 0;
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, bool &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetInt() > 0;
		return true;
	}
	return false;
}

uint8_t CECTag::AssignIfExist(ec_tagname_t tagname, uint8_t *target) const
{
	uint8_t ret = 0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT((tag->GetType() == EC_TAGTYPE_UINT8) || (m_dataType == EC_TAGTYPE_UNKNOWN));
		ret = tag->GetInt();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, uint8_t &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT((tag->GetType() == EC_TAGTYPE_UINT8) || (m_dataType == EC_TAGTYPE_UNKNOWN));
		target = tag->GetInt();
		return true;
	}
	return false;
}

uint16_t CECTag::AssignIfExist(ec_tagname_t tagname, uint16_t *target) const
{
	uint16_t ret = 0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT(
			(tag->GetType() == EC_TAGTYPE_UINT16)
			|| (tag->GetType() == EC_TAGTYPE_UINT8)
			|| (m_dataType == EC_TAGTYPE_UNKNOWN)
		);
		ret = tag->GetInt();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, uint16_t &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT(
			(tag->GetType() == EC_TAGTYPE_UINT16)
			|| (tag->GetType() == EC_TAGTYPE_UINT8)
			|| (m_dataType == EC_TAGTYPE_UNKNOWN)
		);
		target = tag->GetInt();
		return true;
	}
	return false;
}

uint32_t CECTag::AssignIfExist(ec_tagname_t tagname, uint32_t *target) const
{
	uint32_t ret = 0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT(
			(tag->GetType() == EC_TAGTYPE_UINT32)
			|| (tag->GetType() == EC_TAGTYPE_UINT16)
			|| (tag->GetType() == EC_TAGTYPE_UINT8)
			|| (m_dataType == EC_TAGTYPE_UNKNOWN)
		);
		ret = tag->GetInt();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, uint32_t &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		EC_ASSERT(
			(tag->GetType() == EC_TAGTYPE_UINT32)
			|| (tag->GetType() == EC_TAGTYPE_UINT16)
			|| (tag->GetType() == EC_TAGTYPE_UINT8)
			|| (m_dataType == EC_TAGTYPE_UNKNOWN)
		);
		target = tag->GetInt();
		return true;
	}
	return false;
}

uint64_t CECTag::AssignIfExist(ec_tagname_t tagname, uint64_t *target) const
{
	uint64_t ret = 0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetInt();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, uint64_t &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetInt();
		return true;
	}
	return false;
}

time_t CECTag::AssignIfExist(ec_tagname_t tagname, time_t *target) const
{
	time_t ret = 0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetInt();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, time_t &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetInt();
		return true;
	}
	return false;
}

double CECTag::AssignIfExist(ec_tagname_t tagname, double *target) const
{
	double ret = 0.0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetDoubleData();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, double &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetDoubleData();
		return true;
	}
	return false;
}

float CECTag::AssignIfExist(ec_tagname_t tagname, float *target) const
{
	float ret = 0.0;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetDoubleData();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, float &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetDoubleData();
		return true;
	}
	return false;
}

CMD4Hash CECTag::AssignIfExist(ec_tagname_t tagname, CMD4Hash *target) const
{
	CMD4Hash ret;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetMD4Data();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, CMD4Hash &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetMD4Data();
		return true;
	}
	return false;
}

std::string CECTag::AssignIfExist(ec_tagname_t tagname, std::string *target) const
{
	std::string ret;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetStringDataSTL();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, std::string &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetStringDataSTL();
		return true;
	}
	return false;
}

#ifdef USE_WX_EXTENSIONS
wxString CECTag::AssignIfExist(ec_tagname_t tagname, wxString *target) const
{
	wxString ret;
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		ret = tag->GetStringData();
		if (target) {
			*target = ret;
		}
	}
	return ret;
}

bool CECTag::AssignIfExist(ec_tagname_t tagname, wxString &target) const
{
	const CECTag *tag = GetTagByName(tagname);
	if (tag) {
		target = tag->GetStringData();
		return true;
	}
	return false;
}		
#endif


#if	__DEBUG__
void CECTag::DebugPrint(int level, bool print_empty) const
{
	if (m_dataLen || print_empty) {
		wxString space;
		for (int i = level; i--;) space += wxT("  ");
		wxString s1 = CFormat(wxT("%s%s %d = ")) % space % GetDebugNameECTagNames(m_tagName) % m_dataLen;
		wxString s2;
		switch (m_tagName) {
			case EC_TAG_DETAIL_LEVEL:
				s2 = GetDebugNameEC_DETAIL_LEVEL(GetInt()); break;
			case EC_TAG_SEARCH_TYPE:
				s2 = GetDebugNameEC_SEARCH_TYPE(GetInt()); break;
			case EC_TAG_STAT_VALUE_TYPE:
				s2 = GetDebugNameEC_STATTREE_NODE_VALUE_TYPE(GetInt()); break;
			default:
				switch (m_dataType) {
					case EC_TAGTYPE_UINT8:
					case EC_TAGTYPE_UINT16:
					case EC_TAGTYPE_UINT32:
					case EC_TAGTYPE_UINT64:
						s2 = CFormat(wxT("%d")) % GetInt(); break;
					case EC_TAGTYPE_STRING:
						s2 = GetStringData(); break;
					case EC_TAGTYPE_DOUBLE:
						s2 = CFormat(wxT("%.1f")) % GetDoubleData(); break;
					case EC_TAGTYPE_HASH16:
						s2 = GetMD4Data().Encode(); break;
					case EC_TAGTYPE_CUSTOM:
						if (m_dataLen == 0) {
							s2 = wxT("empty");
						} else {
							// Make a hex dump (limited to maxOutput)
							const uint32 maxOutput = 50;
							for (uint32 i = 0; i < m_dataLen; i++) {
								if (i == maxOutput) {
									s2 += wxT("...");
									break;
								}
								s2 += CFormat(wxT("%02X ")) % (unsigned char) m_tagData[i];
							}
						}
						break;
					default:
						s2 = GetDebugNameECTagTypes(m_dataType);
				}
		}
		DoECLogLine(s1 + s2);
	}
	for (TagList::const_iterator it = m_tagList.begin(); it != m_tagList.end(); ++it) {
		it->DebugPrint(level + 1, true);
	}
}
#endif

/*!
 * \fn CMD4Hash CECTag::GetMD4Data(void) const
 *
 * \brief Returns a CMD4Hash class.
 *
 * This function takes care of converting from MSB to LSB as necessary.
 *
 * \return CMD4Hash class.
 *
 * \sa CECTag(ec_tagname_t, const CMD4Hash&)
 */

/*!
 * \fn CECTag *CECTag::GetTagByIndex(size_t index) const
 *
 * \brief Finds the indexth child tag.
 *
 * \param index 0-based index, 0 <= \e index < GetTagCount()
 *
 * \return The child tag, or NULL if index out of range.
 */

/*!
 * \fn CECTag *CECTag::GetTagByIndexSafe(size_t index) const
 *
 * \brief Finds the indexth child tag.
 *
 * \param index 0-based index, 0 <= \e index < GetTagCount()
 *
 * \return The child tag, or a special null-valued tag if index out of range.
 */

/*!
 * \fn size_t CECTag::GetTagCount(void) const
 *
 * \brief Returns the number of child tags.
 *
 * \return The number of child tags.
 */

/*!
 * \fn const void *CECTag::GetTagData(void) const
 *
 * \brief Returns a pointer to the TAG DATA.
 *
 * \return A pointer to the TAG DATA. (As specified with the data field of the constructor.)
*/

/*!
 * \fn uint16 CECTag::GetTagDataLen(void) const
 *
 * \brief Returns the length of the data buffer.
 *
 * \return The length of the data buffer.
 */

/*!
 * \fn ec_tagname_t CECTag::GetTagName(void) const
 *
 * \brief Returns TAGNAME.
 *
 * \return The name of the tag.
 */

/*!
 * \fn wxString CECTag::GetStringData(void) const
 *
 * \brief Returns the string data of the tag.
 *
 * Returns a wxString created from TAGDATA. It is automatically
 * converted from UTF-8 to the internal application encoding.
 * Should be used with care (only on tags created with the
 * CECTag(ec_tagname_t, const wxString&) constructor),
 * becuse it does not perform any check to see if the tag really contains a
 * string object.
 *
 * \return The string data of the tag.
 *
 * \sa CECTag(ec_tagname_t, const wxString&)
 */

/*!
 * \fn uint8 CECTag::GetInt(void) const
 *
 * \brief Returns the uint8 data of the tag.
 *
 * This function takes care of the endianness problems with numbers.
 *
 * \return The uint8 data of the tag.
 *
 * \sa CECTag(ec_tagname_t, uint8)
 */

/*!
 * \fn uint16 CECTag::GetInt(void) const
 *
 * \brief Returns the uint16 data of the tag.
 *
 * This function takes care of the endianness problems with numbers.
 *
 * \return The uint16 data of the tag.
 *
 * \sa CECTag(ec_tagname_t, uint16)
 */

/*!
 * \fn uint32 CECTag::GetInt(void) const
 *
 * \brief Returns the uint32 data of the tag.
 *
 * This function takes care of the endianness problems with numbers.
 *
 * \return The uint32 data of the tag.
 *
 * \sa CECTag(ec_tagname_t, uint32)
 */

/*!
 * \fn uint64 CECTag::GetInt(void) const
 *
 * \brief Returns the uint64 data of the tag.
 *
 * This function takes care of the endianness problems with numbers.
 *
 * \return The uint64 data of the tag.
 *
 * \sa CECTag(ec_tagname_t, uint64)
 */

uint32 CECID::s_IDCounter = 0;

// File_checked_for_headers
