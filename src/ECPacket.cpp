/*
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#pragma implementation

#include "ECPacket.h"	// Needed for ECTag, ECPacket
#include "ECSocket.h"	// Needed for ECSocket
#include <stdlib.h>	// Needed for malloc(), realloc(), free(), NULL
#include <string.h>	// Needed for memcpy(), strlen()

#define	ARRAY_ALLOC_CHUNKS	16

/**********************************************************
 *							  *
 *	CECTag class					  *
 *							  *
 **********************************************************/

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
CECTag::CECTag(ec_tagname_t name, unsigned int length, const void *data, bool copy) : m_tagName(name), m_dynamic(copy)
{
	m_error = 0;
	m_dataLen = length;
	m_tagCount = 0;
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
	m_listSize = 0;
};

/**
 * Creates a new CECTag instance for custom data
 *
 * @param name	 	TAG name
 * @param length 	length of data buffer that will be alloc'ed
 * @param dataptr	pointer to internal TAG data buffer
 *
 * 
 */
CECTag::CECTag(ec_tagname_t name, unsigned int length, void **dataptr)  : m_tagName(name), m_dynamic(true)
{
	m_error = 0;
	m_dataLen = length;
	m_tagData = malloc(m_dataLen);
	if ( !m_tagData ) {
		m_dataLen = 0;
		m_error = 1;
	}
	*dataptr = (void *)m_tagData;
	m_listSize = 0;
	m_tagCount = 0;
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
CECTag::CECTag(ec_tagname_t name, const EC_IPv4_t &data) : m_tagName(name), m_dynamic(true)
{

	m_dataLen = sizeof(EC_IPv4_t);
	m_tagData = new EC_IPv4_t;
	if (m_tagData != NULL) {
		*((uint32 *)(((EC_IPv4_t *)m_tagData)->ip)) = *((uint32 *)(data.ip));
		((EC_IPv4_t *)m_tagData)->port = htons(data.port);
		m_error = 0;
	} else {
		m_error = 1;
	}
	m_tagCount = m_listSize = 0;
	m_tagList = NULL;
}

/**
 * Creates a new CECTag instance, which contains a string
 *
 * @param name TAG name
 * @param data wxString object, it's contents are converted to UTF-8.
 *
 * @see GetStringData()
 */
CECTag::CECTag(ec_tagname_t name, const wxString& data) : m_tagName(name), m_dynamic(true)
{
	const wxCharBuffer buf = wxConvUTF8.cWC2MB(data.wc_str(aMuleConv));
	const char *utf8 = (const char *)buf;

	m_dataLen = strlen(utf8) + 1;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		memcpy((void *)m_tagData, utf8, m_dataLen);
		m_error = 0;
	} else {
		m_error = 1;
	}
	m_tagCount = m_listSize = 0;
	m_tagList = NULL;
}

/**
 * Copy constructor
 */
CECTag::CECTag(const CECTag& tag) : m_tagName( tag.m_tagName ), m_dynamic( tag.m_dynamic )
{
	m_error = 0;
	m_dataLen = tag.m_dataLen;
	m_tagCount = 0;
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
	if (tag.m_tagCount != 0) {
		m_tagList = (_taglist_t)malloc(tag.m_tagCount * sizeof(CECTag *));
		if (m_tagList != NULL) {
			m_listSize = tag.m_tagCount;
			for (int i=0; i<m_listSize; i++) {
				(*m_tagList)[i] = new CECTag(*(*tag.m_tagList)[i]);
				if ((*m_tagList)[i] != NULL) {
					if ((*m_tagList)[i]->m_error == 0) {
						m_tagCount++;
					} else {
						m_error = (*m_tagList)[i]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
						delete (*m_tagList)[i];
						(*m_tagList)[i] = NULL;
#endif
						break;
					}
				} else {
					m_error = 1;
					break;
				}
			}
		} else {
			m_error = 1;
		}
	} else {
		m_listSize = 0;
		m_tagCount = 0;
		m_tagList = NULL;
	}
}

/**
 * Creates a new CECTag instance, which contains an uint8 value.
 *
 * This takes care of endianness problems with numbers.
 *
 * @param name TAG name.
 * @param data uint8 number.
 *
 * @see GetInt8Data()
 */
CECTag::CECTag(ec_tagname_t name, uint8 data) : m_tagName(name), m_dynamic(true)
{
	m_dataLen = 1;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		*((uint8 *)m_tagData) = data;
		m_error = 0;
	} else {
		m_error = 1;
	}
	m_tagCount = m_listSize = 0;
	m_tagList = NULL;
}

/**
 * Creates a new CECTag instance, which contains an uint16 value.
 *
 * This takes care of endianness problems with numbers.
 *
 * @param name TAG name.
 * @param data uint16 number.
 *
 * @see GetInt16Data()
 */
CECTag::CECTag(ec_tagname_t name, uint16 data) : m_tagName(name), m_dynamic(true)
{
	m_dataLen = 2;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		*((uint16 *)m_tagData) = htons(data);
		m_error = 0;
	} else {
		m_error = 1;
	}
	m_tagCount = m_listSize = 0;
	m_tagList = NULL;
}

/**
 * Creates a new CECTag instance, which contains an uint32 value.
 *
 * This takes care of endianness problems with numbers.
 *
 * @param name TAG name.
 * @param data uint32 number.
 *
 * @see GetInt32Data()
 */
CECTag::CECTag(ec_tagname_t name, uint32 data) : m_tagName(name), m_dynamic(true)
{
	m_dataLen = 4;
	m_tagData = malloc(m_dataLen);
	if (m_tagData != NULL) {
		*((uint32 *)m_tagData) = htonl(data);
		m_error = 0;
	} else {
		m_error = 1;
	}
	m_tagCount = m_listSize = 0;
	m_tagList = NULL;
}

/**
 * Destructor - frees allocated data and deletes child TAGs.
 */
CECTag::~CECTag(void)
{
	if (m_dynamic) free((void *)m_tagData);
	for (int i=0; i<m_tagCount; i++) {
		delete (*m_tagList)[i];
	}
	free(m_tagList);
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
	CECTag *copy = new CECTag(tag);
	if ( AddTag(copy) ) {
		return true;
	} else {
		delete copy;
		return false;
	}
}

bool CECTag::AddTag(const CECTag *tag)
{
	// cannot have more than 64k tags
	wxASSERT(m_tagCount < 0xffff);

	if (m_listSize == 0) {
		m_tagList = (_taglist_t)malloc(ARRAY_ALLOC_CHUNKS * sizeof(CECTag *));
		if (m_tagList != NULL) {
			m_listSize = ARRAY_ALLOC_CHUNKS;
		} else {
			m_error = 1;
			return false;
		}
	} else if (m_listSize == m_tagCount) {
		void *tmp = realloc(m_tagList, (m_listSize + ARRAY_ALLOC_CHUNKS) * sizeof(CECTag *));
		if (tmp != NULL) {
			m_tagList = (_taglist_t)tmp;
			m_listSize += ARRAY_ALLOC_CHUNKS;
		} else {
			m_error = 1;
			return false;
		}
	}
	// remove const on assign
	(*m_tagList)[m_tagCount] = (CECTag *)tag;
	if (((*m_tagList)[m_tagCount]) != NULL) {
		if ((*m_tagList)[m_tagCount]->m_error == 0) {
			m_tagCount++;
			return true;
		} else {
			m_error = (*m_tagList)[m_tagCount]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
			delete (*m_tagList)[m_tagCount];
			(*m_tagList)[m_tagCount] = NULL;
#endif
			return false;
		}
	} else {
		m_error = 1;
		return false;
	}
}

CECTag::CECTag(wxSocketBase *sock, ECSocket& socket) : m_dynamic(true)
{
	ec_taglen_t tagLen;
	ec_tagname_t tmp_tagName;

	m_listSize = m_tagCount = 0;
	m_tagData = NULL;
	m_dataLen = 0;
	if (!socket.ReadNumber(sock, &tmp_tagName, sizeof(ec_tagname_t))) {
		m_error = 2;
		m_tagName = 0;
		return;
	}
	m_tagName = tmp_tagName & 0x7fff;
	if (!socket.ReadNumber(sock, &tagLen, sizeof(ec_taglen_t))) {
		m_error = 2;
		return;
	}
	if (tmp_tagName > 0x7fff) {
		if (!ReadChildren(sock, socket)) {
			return;
		}
	}
	m_dataLen = tagLen - GetTagLen();
	if (m_dataLen > 0) {
		m_tagData = malloc(m_dataLen);
		if (m_tagData != NULL) {
			if (!socket.ReadBuffer(sock, (void *)m_tagData, m_dataLen)) {
				m_error = 2;
				return;
			}
		} else {
			m_error = 1;
			return;
		}
	} else {
		m_tagData = NULL;
	}
	m_error = 0;
}


bool CECTag::WriteTag(wxSocketBase *sock, ECSocket& socket) const
{
	ec_taglen_t tagLen = GetTagLen();
	ec_tagname_t tmp_tagName = (m_tagCount > 0) ? m_tagName | 0x8000 : m_tagName;
	
	if (!socket.WriteNumber(sock, &tmp_tagName, sizeof(ec_tagname_t))) return false;
	if (!socket.WriteNumber(sock, &tagLen, sizeof(ec_taglen_t))) return false;
	if (tmp_tagName > 0x7fff) {
		if (!WriteChildren(sock, socket)) return false;
	}
	if (m_dataLen > 0) {
		if (m_tagData != NULL) {	// This is here only to make sure everything, it should not be NULL at this point
			if (!socket.WriteBuffer(sock, m_tagData, m_dataLen)) return false;
		}
	}
	return true;
}


bool CECTag::ReadChildren(wxSocketBase *sock, ECSocket& socket)
{
	uint16 tmp_tagCount;

	if (!socket.ReadNumber(sock, &tmp_tagCount, 2)) {
		m_error = 2;
		return false;
	}
	m_listSize = tmp_tagCount;
	if (tmp_tagCount > 0) {
		m_tagList = (_taglist_t)malloc(m_listSize * sizeof(CECTag *));
		if (m_tagList != NULL) {
			for (int i=0; i<m_listSize; i++) {
				(*m_tagList)[i] = new CECTag(sock, socket);
				if ((*m_tagList)[i] != NULL) {
					if ((*m_tagList)[i]->m_error == 0) {
						m_tagCount++;
					} else {
						m_error = (*m_tagList)[i]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
						delete (*m_tagList)[i];
						(*m_tagList)[i] = NULL;
#endif
						return false;
					}
				}  else {
					m_error = 1;
					return false;
				}
			}
		} else {
			m_error = 1;
			return false;
		}
	} else {
		m_tagList = NULL;
	}
	return true;
}


bool CECTag::WriteChildren(wxSocketBase *sock, ECSocket& socket) const
{
	if (!socket.WriteNumber(sock, &m_tagCount, 2)) return false;
	if (m_tagCount > 0) {
		if (m_tagList != NULL) {	// This is here only to make sure everything, it should not be NULL at this point
			for (int i=0; i<m_tagCount; i++) {
				if (!(*m_tagList)[i]->WriteTag(sock, socket)) return false;
			}
		} else return false;
	}
	return true;
}

/**
 * Finds the (first) child tag with given name.
 *
 * @param name TAG name to look for.
 * @return the tag found, or NULL.
 */
CECTag *CECTag::GetTagByName(ec_tagname_t name) const
{
	for (int i=0; i<m_tagCount; i++)
		if ((*m_tagList)[i]->m_tagName == name) return (*m_tagList)[i];
	return NULL;
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
	for (int i=0; i<m_tagCount; i++) {
		length += (*m_tagList)[i]->GetTagLen();
		length += sizeof(ec_tagname_t) + sizeof(ec_taglen_t) + (((*m_tagList)[i]->GetTagCount() > 0) ? 2 : 0);
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
 * @see CECTag(ec_tagname_t, const EC_IPv4_t *)
 */
EC_IPv4_t CECTag::GetIPv4Data(void) const
{
	EC_IPv4_t p;

	*((uint32 *)(p.ip)) = *((uint32 *)(((EC_IPv4_t *)m_tagData)->ip));
	p.port = ntohs(((EC_IPv4_t *)m_tagData)->port);

	return p;
}

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
 * \fn uint8 CECTag::GetInt8Data(void) const
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
 * \fn uint8 CECTag::GetInt16Data(void) const
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
 * \fn uint8 CECTag::GetInt32Data(void) const
 *
 * \brief Returns the uint32 data of the tag.
 *
 * This function takes care of the endianness problems with numbers.
 *
 * \return The uint32 data of the tag.
 *
 * \sa CECTag(ec_tagname_t, uint32)
 */

/**********************************************************
 *							  *
 *	CECPacket class					  *
 *							  *
 **********************************************************/

CECPacket::CECPacket(wxSocketBase *sock, ECSocket& socket) : CECEmptyTag(0)
{
	m_error = 0;
	if (!socket.ReadNumber(sock, &m_opCode, sizeof(ec_opcode_t))) {
		m_error = 2;
		return;
	}
	ReadChildren(sock, socket);
}


bool CECPacket::WritePacket(wxSocketBase *sock, ECSocket& socket) const
{
	if (!socket.WriteNumber(sock, &m_opCode, sizeof(ec_opcode_t))) return false;
	if (!WriteChildren(sock, socket)) return false;
	return true;
}

/*!
 * \fn CECPacket::CECPacket(ec_opcode_t opCode)
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
