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
#include "StringFunctions.h"	// Needed for aMuleConvToUTF8
#include "ECcodes.h"	// Needed for EC types
#include "endianfix.h"	// Needed for ENDIAN_SWAP_* macros

// Commented out, because with current code it'll most likely scew up gcc optimizations
//#pragma interface

// Define this to keep partial packets
// (those that had an error upon reception/creation)
#undef KEEP_PARTIAL_PACKETS


class CECTag;
class ECSocket;
class wxSocketBase;


/**
 * Type of ECTag::m_tagList
 */
typedef CECTag* (*_taglist_t)[];


/**
 * High level EC packet TAGs handler class
 */

class CECTag {
	public:
				CECTag(ec_tagname_t name, unsigned int length, const void *data, bool copy = true);
			// Routines for special data types.
				CECTag(ec_tagname_t name, uint8 data);
				CECTag(ec_tagname_t name, uint16 data);
				CECTag(ec_tagname_t name, uint32 data);
				CECTag(ec_tagname_t name, const wxString& data);
				CECTag(ec_tagname_t name, const EC_IPv4_t &data);
				CECTag(const CECTag& tag);
				~CECTag(void);
		bool		AddTag(const CECTag& tag);
		CECTag *	GetTagByIndex(unsigned int index) const { return ((index >= m_tagCount) ? NULL : (*m_tagList)[index]); }
		CECTag *	GetTagByName(ec_tagname_t name) const;
		uint16		GetTagCount(void) const { return m_tagCount; }
		const void *	GetTagData(void) const { return m_tagData; }
		uint16		GetTagDataLen(void) const { return m_dataLen; }
		uint32		GetTagLen(void) const;
		ec_tagname_t	GetTagName(void) const { return m_tagName; }
			// Retrieving special data types
		uint8		GetInt8Data(void) const { return *((uint8 *)m_tagData); }
		uint16		GetInt16Data(void) const { return ENDIAN_SWAP_16(*((uint16 *)m_tagData)); }
		uint32		GetInt32Data(void) const { return ENDIAN_SWAP_32(*((uint32 *)m_tagData)); }
		wxString	GetStringData(void) const { return wxString(wxConvUTF8.cMB2WC((const char *)m_tagData), aMuleConv); }
		EC_IPv4_t 	GetIPv4Data(void) const;
	protected:
				CECTag(wxSocketBase *sock, ECSocket& socket);
		bool		WriteTag(wxSocketBase *sock, ECSocket& socket) const;
		bool		ReadChildren(wxSocketBase *sock, ECSocket& socket);
		bool		WriteChildren(wxSocketBase *sock, ECSocket& socket) const;
		int		m_error;
		const void *	m_tagData;
	private:
		ec_tagname_t	m_tagName;
		uint16		m_tagCount;
		unsigned int	m_dataLen;
		const bool	m_dynamic;
		uint16		m_listSize;
		_taglist_t	m_tagList;
};


/**
 * High level EC packet handler class
 */

class CECPacket : private CECTag {
	friend class ECSocket;
	public:
				CECPacket(ec_opcode_t opCode) : CECTag(0, 0, NULL, false), m_opCode(opCode) {};
				~CECPacket(void) {};
				CECTag::AddTag;
				CECTag::GetTagByIndex;
				CECTag::GetTagByName;
				CECTag::GetTagCount;
		ec_opcode_t	GetOpCode(void) const { return m_opCode; }
		uint32		GetPacketLength(void) const { return CECTag::GetTagLen(); }
	private:
				CECPacket(wxSocketBase *sock, ECSocket& socket);
		bool		WritePacket(wxSocketBase *sock, ECSocket& socket) const;
		ec_opcode_t	m_opCode;
};


/*
 * Specific tags for specific requests
 */

class CServer;
class CPartFile;

class CEC_Server_Tag : public CECTag {
 	public:
 		CEC_Server_Tag(CServer *, unsigned int);
};

class CEC_ConnState_Tag : public CECTag {
 	public:
 		CEC_ConnState_Tag(unsigned int);
};

class CEC_PartFile_Tag : public CECTag {
 	public:
 		CEC_PartFile_Tag(CPartFile *file, bool onlystatus, bool includeparts);
 		
 		uint32 FileID() { return GetInt32Data(); }
 		wxString FileName() { return GetTagByName(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32 SizeFull() { return GetTagByName(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
 		uint32 SizeXfer() { return GetTagByName(EC_TAG_PARTFILE_SIZE_XFER)->GetInt32Data(); }
  		uint32 SizeDone() { return GetTagByName(EC_TAG_PARTFILE_SIZE_DONE)->GetInt32Data(); }
 		wxString FileEd2kLink() { return GetTagByName(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }
 		wxString FileStatus() { return GetTagByName(EC_TAG_PARTFILE_STATUS)->GetStringData(); }
  		uint32 SourceCount() { return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT)->GetInt32Data(); }
  		uint32 SourceNotCurrCount() { return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT)->GetInt32Data(); }
  		uint32 SourceXferCount() { return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_XFER)->GetInt32Data(); }
  		uint32 Speed() { return GetTagByName(EC_TAG_PARTFILE_SPEED)->GetInt32Data(); }
  		uint32 Prio() { return GetTagByName(EC_TAG_PARTFILE_PRIO)->GetInt32Data(); }
};

class CEC_PartStatus_Tag : public CECTag {
 	public:
 		CEC_PartStatus_Tag(CPartFile *file, int statussize);
};

#endif /* ECPacket.h */
