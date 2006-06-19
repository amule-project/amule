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

#ifndef ECTAG_H
#define ECTAG_H

/* aMule/libcommon generic includes */
#include "MD4Hash.h"		// Needed for CMD4Hash

/* EC specific includes */
#include "ECCodes.h"		// Needed for EC types

#warning Kry - Can you say "Design flaw"?
class CECSocket;
#include "OtherFunctions.h"
#include <common/StringFunctions.h>	// Needed for aMuleConv define
/* Design flaw end */

/**
 * Class to hold IPv4 address.
 */
class EC_IPv4_t {
	public:
		EC_IPv4_t() { }
		EC_IPv4_t(uint32 ip, uint16 port)
		{
			m_ip[0] = ip & 0xff;
			m_ip[1] = (ip >> 8) & 0xff;
			m_ip[2] = (ip >> 16) & 0xff;
			m_ip[3] = (ip >> 24) & 0xff;
			m_port = port;
		}
		
		uint32 IP()
		{
			return m_ip[0] | (m_ip[1] << 8) | (m_ip[2] << 16) | (m_ip[3] << 24);
		}

		wxString StringIP(bool brackets = true)
		{
			return wxString::Format(brackets ? wxT("[%d.%d.%d.%d:%d]") : wxT("%d.%d.%d.%d : %d"), m_ip[0], m_ip[1], m_ip[2], m_ip[3], m_port);
		}
		
		uint8 m_ip[4];
		uint16 m_port;
};


enum ECTagTypes {
	EC_TAGTYPE_UNKNOWN,
	EC_TAGTYPE_CUSTOM,
	EC_TAGTYPE_UINT8,
	EC_TAGTYPE_UINT16,
	EC_TAGTYPE_UINT32,
	EC_TAGTYPE_UINT64,
	EC_TAGTYPE_STRING,
	EC_TAGTYPE_DOUBLE,
	EC_TAGTYPE_IPV4,
	EC_TAGTYPE_HASH
};

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
		CECTag(ec_tagname_t name, uint64 data);
		CECTag(ec_tagname_t name, double data);
		CECTag(ec_tagname_t name, const wxString& data);
		CECTag(ec_tagname_t name, const EC_IPv4_t& data);
		CECTag(ec_tagname_t name, const CMD4Hash& data);
		CECTag(const CECTag& tag);
		~CECTag(void);

		CECTag&		operator=(const CECTag& rhs);
		bool		AddTag(const CECTag& tag);
		const CECTag*	GetTagByIndex(unsigned int index) const
			{ return ((index >= m_tagList.size()) ? NULL : &m_tagList[index]); }
		CECTag*		GetTagByIndex(unsigned int index)
			{ return ((index >= m_tagList.size()) ? NULL : &m_tagList[index]); }
		const CECTag*	GetTagByIndexSafe(unsigned int index) const
			{ const CECTag* result = GetTagByIndex(index); return result ? result : &s_theNullTag; }
		
		const CECTag*	GetTagByName(ec_tagname_t name) const;
		CECTag*			GetTagByName(ec_tagname_t name);
		const CECTag*	GetTagByNameSafe(ec_tagname_t name) const;
		
		uint16		GetTagCount(void) const { return m_tagList.size(); }
		const void *	GetTagData(void) const { 
			wxASSERT(m_dataType == EC_TAGTYPE_CUSTOM);
			return m_tagData; 
		}
		uint16		GetTagDataLen(void) const { return m_dataLen; }
		uint32		GetTagLen(void) const;
		ec_tagname_t	GetTagName(void) const { return m_tagName; }
		// Retrieving special data types
		uint64		GetInt(void) const { 
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
					DumpMem(m_tagData, m_dataLen, wxT("Unk Int Tag"));
					wxASSERT(0);
					return 0;
			}
		}
		double		GetDoubleData(void) const;
		wxString	GetStringData(void) const { 
			wxASSERT((m_dataType == EC_TAGTYPE_STRING) || (m_dataType == EC_TAGTYPE_UNKNOWN));
			return wxString(wxConvUTF8.cMB2WC((const char *)m_tagData), aMuleConv); 
		}
		EC_IPv4_t 	GetIPv4Data(void) const;
		CMD4Hash	GetMD4Data(void) const { 
			wxASSERT((m_dataType == EC_TAGTYPE_HASH) || (m_dataType == EC_TAGTYPE_UNKNOWN)); 
			return CMD4Hash((const unsigned char *)m_tagData); 
		}
		
		void AssignIfExist(ec_tagname_t tagname, uint8 &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) {
				wxASSERT((tag->GetType() == EC_TAGTYPE_UINT8) || (m_dataType == EC_TAGTYPE_UNKNOWN));
				target = tag->GetInt();
			}
		}
		void AssignIfExist(ec_tagname_t tagname, uint16 &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) {
				wxASSERT(
					(tag->GetType() == EC_TAGTYPE_UINT16)
					|| (tag->GetType() == EC_TAGTYPE_UINT8)
					|| (m_dataType == EC_TAGTYPE_UNKNOWN)
				);
				target = tag->GetInt();
			}
		}
		void AssignIfExist(ec_tagname_t tagname, uint32 &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) {
				wxASSERT(
					(tag->GetType() == EC_TAGTYPE_UINT32)
					|| (tag->GetType() == EC_TAGTYPE_UINT16)
					|| (tag->GetType() == EC_TAGTYPE_UINT8)
					|| (m_dataType == EC_TAGTYPE_UNKNOWN)
				);
				target = tag->GetInt();	
			}
		}
		void AssignIfExist(ec_tagname_t tagname, uint64 &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) target = tag->GetInt();
		}
		void AssignIfExist(ec_tagname_t tagname, double &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) target = tag->GetDoubleData();
		}
		void AssignIfExist(ec_tagname_t tagname, CMD4Hash &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) target = tag->GetMD4Data();
		}
		void AssignIfExist(ec_tagname_t tagname, wxString &target)
		{
			CECTag *tag = GetTagByName(tagname);
			if ( tag ) target = tag->GetStringData();
		}
		
	protected:

		uint8 GetType() const { return m_dataType; }

		enum BuildState {
			bsName,
			bsType,
			bsLength,
			bsLengthChld,
			bsChildCnt,
			bsChildren,
			bsData1,
			bsData2,
			bsFinished
		};

		CECTag(const CECSocket&)
			: m_error(0), m_tagData(NULL), m_state(bsName), m_dataLen(0), m_dataType(EC_TAGTYPE_UNKNOWN), m_dynamic(true), m_haschildren(false)
			{}

		bool		ReadFromSocket(CECSocket& socket);
		bool		WriteTag(CECSocket& socket) const;
		bool		ReadChildren(CECSocket& socket);
		bool		WriteChildren(CECSocket& socket) const;
		int		m_error;
		const void *	m_tagData;

		BuildState	m_state;

		bool		IsOk() const { return m_state == bsFinished; }

	private:
		// Special type used to invoke the Null tag constructor
		struct NullTagConstructorSelector { };
		
		// To init. the automatic int data
		void InitInt(uint64 data);

		// Special constructor to construct the Null tag.
		explicit CECTag(const NullTagConstructorSelector*);

		ec_tagname_t	m_tagName;
		ec_taglen_t		m_dataLen;
		mutable ec_tagtype_t	m_dataType;
		bool		m_dynamic;

		typedef std::vector<CECTag> TagList;
		TagList m_tagList;
		
		bool m_haschildren;

		static const CECTag s_theNullTag;
		static const uint32 s_theNullTagData[4];
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
	protected:
		CECEmptyTag(const CECSocket& socket) : CECTag(socket) {}
};

#endif /* ECTAG_H */
// File_checked_for_headers
