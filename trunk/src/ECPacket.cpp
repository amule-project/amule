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


#include "ECPacket.h"	// Needed for ECTag, ECPacket
#include "ECSocket.h"	// Needed for ECSocket
#include <stdlib.h>	// Needed for malloc(), realloc(), free(), NULL
#include <string.h>	// Needed for memcpy(), strlen()

#define	ARRAY_ALLOC_CHUNKS	16

// Define this to keep partially received packets.
// (Those that had an error upon reception/creation.)
#undef KEEP_PARTIAL_PACKETS

//
// CECTag class
//

CECTag::CECTag(const ec_tagname_t name, const ec_taglen_t length, const void *data, bool copy) : m_tagName(name), m_dynamic(copy)
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
    m_tagLen = m_dataLen;
    m_listSize = 0;
};


CECTag::CECTag(const ec_tagname_t name, const wxString& data) : m_tagName(name), m_dynamic(true)
{
    const wxCharBuffer buf = wxConvUTF8.cWC2MB(data.wc_str(aMuleConv));
    const char *utf8 = (const char *)buf;

    m_dataLen = m_tagLen = strlen(utf8) + 1;
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
    m_tagLen = tag.m_dataLen;
    if (tag.m_tagCount != 0) {
	m_tagList = (_taglist_t)malloc(tag.m_tagCount * sizeof(CECTag *));
	if (m_tagList != NULL) {
	    m_listSize = tag.m_tagCount;
	    for (int i=0; i<m_listSize; i++) {
		(*m_tagList)[i] = new CECTag(*(*tag.m_tagList)[i]);
		if ((*m_tagList)[i] != NULL) {
		    if ((*m_tagList)[i]->m_error == 0) {
			m_tagCount++;
			m_tagLen += (*tag.m_tagList)[i]->m_tagLen;
		    } else {
			m_error = (*m_tagList)[i]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
			delete (*m_tagList)[i];
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


CECTag::~CECTag()
{
    if (m_dynamic) free((void *)m_tagData);
    for (int i=0; i<m_tagCount; i++) {
	delete (*m_tagList)[i];
    }
}


bool CECTag::AddTag(const CECTag& tag)
{
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
    (*m_tagList)[m_tagCount] = new CECTag(tag);
    if (((*m_tagList)[m_tagCount]) != NULL) {
	if ((*m_tagList)[m_tagCount]->m_error == 0) {
	    m_tagCount++;
	    m_tagLen += tag.m_tagLen;
	    return true;
	} else {
	    m_error = (*m_tagList)[m_tagCount]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
	    delete (*m_tagList)[m_tagCount];
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
    ec_taglen_t tmp_tagLen;

    m_tagCount = 0;
    m_tagData = NULL;
    if (!socket.ReadNumber(sock, &m_tagName, sizeof(ec_tagname_t))) {
	m_error = 2;
	return;
    }
    if (!socket.ReadNumber(sock, &tmp_tagLen, sizeof(ec_taglen_t))) {
	m_error = 2;
	return;
    }
    m_tagLen = tmp_tagLen;
    if (HasTagCount(m_tagName)) {
	if (!ReadChildren(sock, socket)) {
	    return;
	}
    } else {
	m_dataLen = m_tagLen;
    }
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
    ec_taglen_t tmp_tagLen = m_tagLen;

    if (!socket.WriteNumber(sock, &m_tagName, sizeof(ec_tagname_t))) return false;
    if (!socket.WriteNumber(sock, &tmp_tagLen, sizeof(ec_taglen_t))) return false;
    if (HasTagCount(m_tagName)) {
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
    ec_taglen_t tmp_tagLen = m_tagLen;
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
			tmp_tagLen -= (*m_tagList)[i]->m_tagLen;
		    } else {
			m_error = (*m_tagList)[i]->m_error;
#ifndef KEEP_PARTIAL_PACKETS
			delete (*m_tagList)[i];
#endif
			return false;
		    }
		} else {
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
    m_dataLen = tmp_tagLen;
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


CECTag *CECTag::GetTagByName(const ec_tagname_t name) const
{
    for (int i=0; i<m_tagCount; i++)
	if ((*m_tagList)[i]->m_tagName == name) return (*m_tagList)[i];
    return NULL;
}


CECTag *CECTag::GetTagByIndex(const unsigned int index) const
{
    if (index >= m_tagCount) return NULL;
    else return (*m_tagList)[index];
}


//
// ECPacket class
//

CECPacket::CECPacket(wxSocketBase *sock, ECSocket& socket) : CECTag(0, 0, NULL, true)
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
