//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ECPacket.h"	// Needed for ECTag, ECPacket
#include "ECSocket.h"	// Needed for CECSocket
#include <cstdlib>	// Needed for malloc(), realloc(), free(), NULL
#include <cstring>	// Needed for memcpy(), strlen()
#include <locale.h>	// Needed for localeconv()
#include "MD4Hash.h"	// Needed for CMD4Hash

/**********************************************************
 *							  *
 *	CECTag class					  *
 *							  *
 **********************************************************/

//! Defines the Null tag which may be returned by GetTagByNameSafe.
const CECTag CECTag::s_theNullTag(static_cast<NullTagConstructorSelector*>(0));

//! Defines the data for the Null tag.  Large enough (16 bytes) for GetMD4Data.
const uint32 CECTag::s_theNullTagData[4] = { 0, 0, 0, 0 };

/**
 * Creates a new null-valued CECTag instance
 *
 * @see s_theNullTag
 * @see s_theNullTagData
 * @see GetTagByNameSafe
 */
CECTag::CECTag(const NullTagConstructorSelector*) :
	m_error(0),
	m_tagData(s_theNullTagData),
	m_tagName(0),
	m_dataLen(0),
	m_dataType(EC_TAGTYPE_UNKNOWN),
	m_dynamic(false),
	m_tagList(),
	m_haschildren(false)
{
}

/**
 * Creates a new CECTag instance from the given data
 *
 * @param name	 TAG name
 * @param length length of data buffer
 * @param data	 TAG data
 * @param copy	 whether to create a copy of the TAG data at \e *data, or should use the provided pointer.
 *
 * \note When you set \e copy to \b false, the provided data buffer must exist
 * in the whole lifetime of the packet.
 */
CECTag::CECTag(ec_tagname_t name, unsigned int length, const void *data, bool copy) : m_tagName(name), m_dynamic(copy), m_haschildren( false )
{
	m_error = 0;
	m_dataLen = length;
	if (copy && (data != NULL)) {
		m_tagData = malloc(m_dataLen);
		if (m_tagData != NULL) {
			memcpy((void *)m_tagData, data, m_dataLen);
		} else {
			m_dataLen = 0;
			m_error = 1;
		}
	} else {
		m_tagData = data;
	}
	m_dataType = EC_TAGTYPE_CUSTOM;
}

/**
 * Creates a new CECTag instance for custom data
 *
 * @param name	 	TAG name
 * @param length 	length of data buffer that will be alloc'ed
 * @param dataptr	pointer to internal TAG data buffer
 *
 * 
 */
CECTag::CECTag(ec_tagname_t name, unsigned int length, void **dataptr)  : m_tagName(name), m_dynamic(true), m_haschildren( false )
{
	m_error = 0;
	m_dataLen = length;
	m_tagData = malloc(m_dataLen);
	if ( !m_tagData ) {
		m_dataLen = 0;
		m_error = 1;
	}
	*dataptr = (void *)m_tagData;
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
CECTag::CECTag(ec_tagname_t name, const EC_IPv4_t& data) : m_tagName(name), m_dynamic(true), m_haschildren( false )
{

	m_dataLen = sizeof(EC_IPv4_t);
	m_tagData = malloc(sizeof(EC_IPv4_t));
	if (m_tagData != NULL) {
		RawPokeUInt32( ((EC_IPv4_t *)m_tagData)->m_ip, RawPeekUInt32( data.m_ip ) );
		((EC_IPv4_t *)m_tagData)->m_port = ENDIAN_HTONS(data.m_port);
		m_error = 0;
		m_dataType = EC_TAGTYPE_IPV4;
	} else {
		m_error = 1;
	}
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
CECTag::CECTag(ec_tagname_t name, const CMD4Hash& data) : m_tagName(name), m_dynamic(true), m_haschildren( false )
{

	m_dataLen = 16;
	m_tagData = malloc(16);
	if (m_tagData != NULL) {
		RawPokeUInt64( (char*)m_tagData,		RawPeekUInt64( data.GetHash() ) );
		RawPokeUInt64( (char*)m_tagData + 8,	RawPeekUInt64( data.GetHash() + 8 ) );
		m_error = 0;
		m_dataType = EC_TAGTYPE_HASH;
	} else {
		m_error = 1;
	}
}

/**
 * Creates a new CECTag instance, which contains a string
 *
 * @param name TAG name
 * @param data wxString object, it's contents are converted to UTF-8.
 *
 * @see GetStringData()
 */
CECTag::CECTag(ec_tagname_t name, const wxString& data) : m_tagName(name), m_dynamic(true), m_haschildren( false )
{
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(data.wc_str(aMuleConv));
	const char *utf8 = (const char *)buf;

	m_dataLen = strlen(utf8) + 1;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		memcpy((void *)m_tagData, utf8, m_dataLen);
		m_error = 0;
		m_dataType = EC_TAGTYPE_STRING;		
	} else {
		m_error = 1;
	}
}

/**
 * Copy constructor
 */
CECTag::CECTag(const CECTag& tag) : m_state( tag.m_state ), m_tagName( tag.m_tagName ), m_dynamic( tag.m_dynamic ), m_haschildren( tag.m_haschildren )
{
	m_error = 0;
	m_dataLen = tag.m_dataLen;
	m_dataType = tag.m_dataType;
	if (m_dataLen != 0) {
		if (m_dynamic) {
			m_tagData = malloc(m_dataLen);
			if (m_tagData != NULL) {
				memcpy((void *)m_tagData, tag.m_tagData, m_dataLen);
			} else {
				m_dataLen = 0;
				m_error = 1;
				return;
			}
		} else {
			m_tagData = tag.m_tagData;
		}
	} else m_tagData = NULL;
	if (!tag.m_tagList.empty()) {
		m_tagList.reserve(tag.m_tagList.size());
		for (TagList::size_type i=0; i<tag.m_tagList.size(); i++) {
			m_tagList.push_back(tag.m_tagList[i]);
			if (m_tagList.back().m_error != 0) {
				m_error = m_tagList.back().m_error;
#ifndef KEEP_PARTIAL_PACKETS
				m_tagList.pop_back();
#endif
				break;
			}
		}
	}
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
CECTag::CECTag(ec_tagname_t name, uint8 data) : m_tagName(name), m_dynamic(true)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint16 data) : m_tagName(name), m_dynamic(true)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint32 data) : m_tagName(name), m_dynamic(true)
{
	InitInt(data);
}
CECTag::CECTag(ec_tagname_t name, uint64 data) : m_tagName(name), m_dynamic(true)
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
	
	m_tagData = malloc(m_dataLen);
	
	if (m_tagData != NULL) {
		switch (m_dataType) {
			case EC_TAGTYPE_UINT8:
				PokeUInt8( (void*)m_tagData, data );
				break;
			case EC_TAGTYPE_UINT16:
				PokeUInt16( (void*)m_tagData, ENDIAN_HTONS( data ) );
				break;
			case EC_TAGTYPE_UINT32:
				PokeUInt32( (void*)m_tagData, ENDIAN_HTONL( data ) );
				break;
			case EC_TAGTYPE_UINT64:
				PokeUInt64( (void*)m_tagData, ENDIAN_HTONLL( data ) );
				break;
			default:
				/* WTF?*/
				wxASSERT(0);
				free((void*)m_tagData);
				m_error = 1;
				return;
		}
		m_error = 0;
	} else {
		m_error = 1;
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
CECTag::CECTag(ec_tagname_t name, double data) : m_tagName(name), m_dynamic(true)
{
	wxString str = wxString::Format(wxT("%g"), data);
	struct lconv *lc = localeconv();
	str.Replace(char2unicode(lc->decimal_point), wxT("."));
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(str.wc_str(aMuleConv));
	const char *utf8 = (const char *)buf;

	m_dataLen = strlen(utf8) + 1;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		memcpy((void *)m_tagData, utf8, m_dataLen);
		m_dataType = EC_TAGTYPE_DOUBLE;
		m_error = 0;
	} else {
		m_error = 1;
	}
}

/**
 * Destructor - frees allocated data and deletes child TAGs.
 */
CECTag::~CECTag(void)
{
	if (m_dynamic) free((void *)m_tagData);
}

/**
 * Copy assignment operator.
 *
 * std::vector uses this, but the compiler-supplied version wouldn't properly
 * handle m_dynamic and m_tagData.  This wouldn't be necessary if m_tagData
 * was a smart pointer (Hi, Kry!).
 */
CECTag& CECTag::operator=(const CECTag& rhs)
{
	if (&rhs != this)
	{
		// This is a trick to reuse the implementation of the copy constructor
		// so we don't have to duplicate it here.  temp is constructed as a
		// copy of rhs, which properly handles m_dynamic and m_tagData.  Then
		// temp's members are swapped for this object's members.  So,
		// effectively, this object has been made a copy of rhs, which is the
		// point.  Then temp is destroyed as it goes out of scope, so its
		// destructor cleans up whatever data used to belong to this object.
		CECTag temp(rhs);
		std::swap(m_error,	temp.m_error);
		std::swap(m_tagData,	temp.m_tagData);
		std::swap(m_tagName,	temp.m_tagName);
		std::swap(m_dataLen,	temp.m_dataLen);
		std::swap(m_dynamic,	temp.m_dynamic);
		std::swap(m_tagList,	temp.m_tagList);
		std::swap(m_state,	temp.m_state);
	}

	return *this;
}

/**
 * Add a child tag to this one.
 *
 * Be very careful that this creates a copy of \e tag. Thus, the following code won't work as expected:
 * \code
 * {
 *	CECPacket *p = new CECPacket(whatever);
 *	CECTag *t1 = new CECTag(whatever);
 *	CECTag *t2 = new CECTag(whatever);
 *	p.AddTag(*t1);
 *	t1.AddTag(*t2);	// t2 won't be part of p !!!
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
 *	t1.AddTag(*t2);
 *	delete t2;	// we can safely delete t2 here, because t1 holds a copy
 *	p.AddTag(*t1);
 *	delete t1;	// now p holds a copy of both t1 and t2
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
 *	p.AddTag(t1);	// now p holds a copy of both t1 and t2
 * }
 * \endcode
 *
 * @param tag a CECTag class instance to add.
 * @return \b true on succcess, \b false when an error occured
 */
bool CECTag::AddTag(const CECTag& tag)
{
	// cannot have more than 64k tags
	wxASSERT(m_tagList.size() < 0xffff);

	m_tagList.push_back(tag);
	if (m_tagList.back().m_error == 0) {
		return true;
	} else {
		m_error = m_tagList.back().m_error;
#ifndef KEEP_PARTIAL_PACKETS
		m_tagList.pop_back();
#endif
		return false;
	}
}

bool CECTag::ReadFromSocket(CECSocket& socket)
{
	if (m_state == bsName) {
		ec_tagname_t tmp_tagName;
		if (!socket.ReadNumber(&tmp_tagName, sizeof(ec_tagname_t))) {
			m_tagName = 0;
			return false;
		} else {
			m_tagName = tmp_tagName >> 1;
			m_haschildren = (tmp_tagName & 0x01) ? true : false;
			m_state = bsType;
		}
	}
	
	if (m_state == bsType) {
		ec_tagtype_t type;
		if (!socket.ReadNumber(&type, sizeof(ec_tagtype_t))) {
			m_dataType = EC_TAGTYPE_UNKNOWN;
			return false;
		} else {
			m_dataType = type;
			if (m_haschildren) {
				m_state = bsLengthChld;
			} else {
				m_state = bsLength;
			}
		}
	}
	
	if (m_state == bsLength || m_state == bsLengthChld) {
		ec_taglen_t tagLen;
		if (!socket.ReadNumber(&tagLen, sizeof(ec_taglen_t))) {
			return false;
		} else {
			m_dataLen = tagLen;
			if (m_state == bsLength) {
				m_state = bsData1;
			} else {
				m_state = bsChildCnt;
			}
		}
	}
	
	if (m_state == bsChildCnt || m_state == bsChildren) {
		if (!ReadChildren(socket)) {
			return false;
		}
	}
	
	if (m_state == bsData1) {
		unsigned int tmp_len = m_dataLen;
		m_dataLen = 0;
		m_dataLen = tmp_len - GetTagLen();
		if (m_dataLen > 0) {
			m_tagData = malloc(m_dataLen);
			m_state = bsData2;
		} else {
			m_tagData = NULL;
			m_state = bsFinished;
		}
	}
	
	if (m_state == bsData2) {
		if (m_tagData != NULL) {
			if (!socket.ReadBuffer((void *)m_tagData, m_dataLen)) {
				return false;
			} else {
				m_state = bsFinished;
			}
		} else {
			m_error = 1;
			return false;
		}
	}
	
	return true;
}


bool CECTag::WriteTag(CECSocket& socket) const
{
	ec_taglen_t tagLen = GetTagLen();
	ec_tagname_t tmp_tagName = (m_tagName << 1) | (m_tagList.empty() ? 0 : 1);
	ec_tagtype_t type = m_dataType;
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
	if (m_state == bsChildCnt) {
		uint16 tmp_tagCount;
		if (!socket.ReadNumber(&tmp_tagCount, sizeof(uint16))) {
			return false;
		} else {
			m_tagList.clear();
			if (tmp_tagCount > 0) {
				m_tagList.reserve(tmp_tagCount);
				for (int i=0; i<tmp_tagCount; i++) {
					m_tagList.push_back(CECTag(socket));
				}
				m_haschildren = true;
				m_state = bsChildren;
			} else {
				m_state = bsData1;
			}
		}
	}
	
	if (m_state == bsChildren) {
		for (unsigned int i=0; i<m_tagList.size(); i++) {
			CECTag& tag = m_tagList[i];
			if (!tag.IsOk()) {
				if (!tag.ReadFromSocket(socket)) {
					if (tag.m_error != 0) {
						m_error = tag.m_error;
					}
					return false;
				}
			}
		}
		m_state = bsData1;
	}
	
	return true;
}

bool CECTag::WriteChildren(CECSocket& socket) const
{
    uint16 tmp = m_tagList.size();
	if (!socket.WriteNumber(&tmp, sizeof(tmp))) return false;
	if (!m_tagList.empty()) {
		for (TagList::size_type i=0; i<m_tagList.size(); i++) {
			if (!m_tagList[i].WriteTag(socket)) return false;
		}
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
	for (TagList::size_type i=0; i<m_tagList.size(); i++)
		if (m_tagList[i].m_tagName == name) return &m_tagList[i];
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
	for (TagList::size_type i=0; i<m_tagList.size(); i++)
		if (m_tagList[i].m_tagName == name) return &m_tagList[i];
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
	for (TagList::size_type i=0; i<m_tagList.size(); i++) {
		length += m_tagList[i].GetTagLen();
		length += sizeof(ec_tagname_t) + sizeof(ec_tagtype_t) + sizeof(ec_taglen_t) + ((m_tagList[i].GetTagCount() > 0) ? 2 : 0);
	}
	return length;
}

/**
 * Returns an EC_IPv4_t class.
 *
 * This function takes care of the enadianness of the port number.
 *
 * @return EC_IPv4_t class.
 *
 * @see CECTag(ec_tagname_t, const EC_IPv4_t&)
 */
EC_IPv4_t CECTag::GetIPv4Data(void) const
{
	EC_IPv4_t p;
	
	wxASSERT(m_dataType = EC_TAGTYPE_IPV4);

	RawPokeUInt32( p.m_ip, RawPeekUInt32( ((EC_IPv4_t *)m_tagData)->m_ip ) );
	p.m_port = ENDIAN_NTOHS(((EC_IPv4_t *)m_tagData)->m_port);

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
	wxASSERT(m_dataType = EC_TAGTYPE_DOUBLE);
	
	wxString str = GetStringData();
	struct lconv *lc = localeconv();
	str.Replace(wxT("."), char2unicode(lc->decimal_point));

	double tmp;
	if (!str.ToDouble(&tmp)) {
		double x = 0.0;		// let's fool g++, it'd error out on 0.0 / 0.0
		tmp = 0.0 / x;		// intentionally generate nan
	}
	return tmp;
}

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
 * \fn CECTag *CECTag::GetTagByIndex(unsigned int index) const
 *
 * \brief Finds the indexth child tag.
 *
 * \param index 0-based index, 0 <= \e index < GetTagCount()
 *
 * \return The child tag, or NULL if index out of range.
 */

/*!
 * \fn CECTag *CECTag::GetTagByIndexSafe(unsigned int index) const
 *
 * \brief Finds the indexth child tag.
 *
 * \param index 0-based index, 0 <= \e index < GetTagCount()
 *
 * \return The child tag, or a special null-valued tag if index out of range.
 */

/*!
 * \fn uint16 CECTag::GetTagCount(void) const
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

/**********************************************************
 *							  *
 *	CECPacket class					  *
 *							  *
 **********************************************************/

bool CECPacket::ReadFromSocket(CECSocket& socket)
{
	if (m_state == bsName) {
		if (!socket.ReadNumber(&m_opCode, sizeof(ec_opcode_t))) {
			return false;
		} else {
			m_state = bsChildCnt;
		}
	}
	if (m_state == bsChildCnt || m_state == bsChildren) {
		if (!ReadChildren(socket)) {
			return false;
		}
	}
	m_state = bsFinished;
	return true;
}


bool CECPacket::WritePacket(CECSocket& socket) const
{
	if (!socket.WriteNumber(&m_opCode, sizeof(ec_opcode_t))) return false;
	if (!WriteChildren(socket)) return false;
	return true;
}

/*!
 * \fn CECPacket::CECPacket(ec_opcode_t opCode, EC_DETAIL_LEVEL detail_level)
 *
 * \brief Creates a new packet with given OPCODE.
 */

/*!
 * \fn ec_opcode_t CECPacket::GetOpCode(void) const
 *
 * \brief Returns OPCODE.
 *
 * \return The OpCode of the packet.
 */

/*!
 * \fn uint32 CECPacket::GetPacketLength(void) const
 *
 * \brief Returns the length of the packet.
 *
 * \return The length of the packet.
 */
