/*
 * This file is part of the aMule Project
 *
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

#include "Types.h"	// Needed for uint* types
#include <wx/string.h>	// Needed for wxString
#ifndef __WXMSW__
#include <netinet/in.h>	// Needed for ntoh, hton functions
#else
#include <winsock.h>
#endif
#include "StringFunctions.h"	// Needed for aMuleConv
#include "ECcodes.h"	// Needed for EC types
#include "CMD4Hash.h"	// Needed for CMD4Hash
#include <vector>

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ECPacket.h"
#endif

// Define this to keep partial packets
// (those that had an error upon reception/creation)
#undef KEEP_PARTIAL_PACKETS


class CECTag;
class ECSocket;
class wxSocketBase;


/**
 * High level EC packet TAGs handler class
 */

class CECTag {
	public:
				CECTag(ec_tagname_t name, unsigned int length, const void *data, bool copy = true);
				// tag for custom data: just init object, alloc buffer and return pointer
				CECTag(ec_tagname_t name, unsigned int length, void **dataptr);
			// Routines for special data types.
				CECTag(ec_tagname_t name, uint8 data);
				CECTag(ec_tagname_t name, uint16 data);
				CECTag(ec_tagname_t name, uint32 data);
				CECTag(ec_tagname_t name, const wxString& data);
				CECTag(ec_tagname_t name, const EC_IPv4_t& data);
				CECTag(ec_tagname_t name, const CMD4Hash& data);
				CECTag(const CECTag& tag);
				~CECTag(void);

		CECTag& operator=(const CECTag& rhs);

		bool		AddTag(const CECTag& tag);

		const	CECTag*	GetTagByIndex(unsigned int index) const { return ((index >= m_tagList.size()) ? NULL : &m_tagList[index]); }
				CECTag*	GetTagByIndex(unsigned int index)		{ return ((index >= m_tagList.size()) ? NULL : &m_tagList[index]); }
		const	CECTag*	GetTagByName(ec_tagname_t name) const;
				CECTag*	GetTagByName(ec_tagname_t name);

		uint16		GetTagCount(void) const { return m_tagList.size(); }
		const void *	GetTagData(void) const { return m_tagData; }
		uint16		GetTagDataLen(void) const { return m_dataLen; }
		uint32		GetTagLen(void) const;
		ec_tagname_t	GetTagName(void) const { return m_tagName; }
			// Retrieving special data types
		uint8		GetInt8Data(void) const { return *((uint8 *)m_tagData); }
		uint16		GetInt16Data(void) const { return ntohs(*((uint16 *)m_tagData)); }
		uint32		GetInt32Data(void) const { return ntohl(*((uint32 *)m_tagData)); }
		wxString	GetStringData(void) const { return wxString(wxConvUTF8.cMB2WC((const char *)m_tagData), aMuleConv); }
		EC_IPv4_t 	GetIPv4Data(void) const;
		CMD4Hash	GetMD4Data(void) const { return CMD4Hash((const unsigned char *)m_tagData); }
	protected:
				CECTag(wxSocketBase *sock, ECSocket& socket, void *opaque);
		bool		WriteTag(wxSocketBase *sock, ECSocket& socket, void *opaque) const;
		bool		ReadChildren(wxSocketBase *sock, ECSocket& socket, void *opaque);
		bool		WriteChildren(wxSocketBase *sock, ECSocket& socket, void *opaque) const;
		int		m_error;
		const void *	m_tagData;
	private:
		ec_tagname_t	m_tagName;
		unsigned int	m_dataLen;
		bool			m_dynamic;

		typedef std::vector<CECTag> TagList;
		TagList m_tagList;
};


/**
 * An empty TAG
 *
 * Note, that an "empty" tag is empty because it contains no data, but it still
 * may contain children.
 */

class CECEmptyTag : public CECTag {
	public:
				CECEmptyTag(ec_tagname_t name) : CECTag(name, 0, NULL, false) {}
};


/**
 * High level EC packet handler class
 */

class CECPacket : private CECEmptyTag {
	friend class ECSocket;
	public:
				CECPacket(ec_opcode_t opCode, EC_DETAIL_LEVEL detail_level = EC_DETAIL_GUI) : CECEmptyTag(0), m_opCode(opCode)
				{
					// since EC_DETAIL_GUI is default - no point transmit it
					if ( detail_level != EC_DETAIL_GUI ) {
						AddTag(CECTag(EC_TAG_DETAIL_LEVEL, (uint8)detail_level));
					}
				}
				CECTag::AddTag;
				CECTag::GetTagByIndex;
				CECTag::GetTagByName;
				CECTag::GetTagCount;
		ec_opcode_t	GetOpCode(void) const { return m_opCode; }
		uint32		GetPacketLength(void) const { return CECTag::GetTagLen(); }
		EC_DETAIL_LEVEL GetDetailLevel() const
				{
					const CECTag *tag = GetTagByName(EC_TAG_DETAIL_LEVEL);
					return (tag) ? (EC_DETAIL_LEVEL)tag->GetInt8Data() : EC_DETAIL_GUI;
				}
	private:
				CECPacket(wxSocketBase *sock, ECSocket& socket, void *opaque);
		bool		WritePacket(wxSocketBase *sock, ECSocket& socket, void *opaque) const;
		ec_opcode_t	m_opCode;
};

#endif /* ECPACKET_H */
