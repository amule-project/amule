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


#ifndef ECPACKET_H
#define ECPACKET_H

#include "types.h"	// Needed for uint* types
#include <wx/string.h>	// Needed for wxString
#include "otherfunctions.h"	// Needed for aMuleConv
#include "ECcodes.h"	// Needed for EC types


// Define this to keep partial packets
// (those that had an error upon reception/creation)
#undef KEEP_PARTIAL_PACKETS


/**
 * taglist_t
 *
 * Type of ECTag::m_tagList
 *
 */
class CECTag;

typedef CECTag* (*_taglist_t)[];


/**
 * High level EC packet TAGs handler class
 *
 */
class ECSocket;
class wxSocketBase;
class CECPacket;

class CECTag {
	public:
				CECTag(const ec_tagname_t name, const ec_taglen_t length, const void *data, bool copy = true);
				CECTag(const ec_tagname_t name, const wxString& data);
				CECTag(const CECTag& tag);
				~CECTag();
		bool		AddTag(const CECTag& tag);
		CECTag *	GetTagByIndex(const unsigned int index) const { return ((index >= m_tagCount) ? NULL : (*m_tagList)[index]); }
		CECTag *	GetTagByName(const ec_tagname_t name) const;
		uint16		GetTagCount(void) const { return m_tagCount; }
		const void *	GetTagData(void) const { return m_tagData; }
		uint16		GetTagDataLen(void) const { return m_dataLen; }
		uint32		GetTagLen(void);
		ec_tagname_t	GetTagName(void) const { return m_tagName; }
		wxString	GetTagString(void) const { return wxString(wxConvUTF8.cMB2WC((const char *)m_tagData), aMuleConv); }
	protected:
				CECTag(wxSocketBase *sock, ECSocket& socket);
		bool		WriteTag(wxSocketBase *sock, ECSocket& socket) const;
		bool		ReadChildren(wxSocketBase *sock, ECSocket& socket);
		bool		WriteChildren(wxSocketBase *sock, ECSocket& socket) const;
		int		m_error;
	private:
		ec_tagname_t	m_tagName;
		uint32		m_tagLen;
		uint16		m_tagCount;
		unsigned int	m_dataLen;
		const void *	m_tagData;
		const bool	m_dynamic;
		uint16		m_listSize;
		_taglist_t	m_tagList;
};


/**
 * High level EC packet handler class
 *
 */

class CECPacket : private CECTag {
	friend class ECSocket;
	public:
				CECPacket(const ec_opcode_t opCode) : CECTag(0, 0, NULL, false), m_opCode(opCode) {};
				~CECPacket() {};
				CECTag::AddTag;
				CECTag::GetTagByIndex;
				CECTag::GetTagByName;
				CECTag::GetTagCount;
		ec_opcode_t	GetOpCode(void) const { return m_opCode; }
		uint32		GetPacketLength(void) { return CECTag::GetTagLen(); }
	private:
				CECPacket(wxSocketBase *sock, ECSocket& socket);
		bool		WritePacket(wxSocketBase *sock, ECSocket& socket) const;
		ec_opcode_t	m_opCode;
};

#endif /* ECPACKET_H */
