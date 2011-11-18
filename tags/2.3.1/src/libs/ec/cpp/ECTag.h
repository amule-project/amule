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

#ifndef ECTAG_H
#define ECTAG_H

#include <iostream>
#include <sstream>

// Must be first! 
#ifdef USE_WX_EXTENSIONS
#include <wx/string.h> // Do_not_auto_remove
#include <common/StringFunctions.h>

#define EC_ASSERT(x)	wxASSERT(x)
#else
#define EC_ASSERT(x)	assert(x)
#endif

/* aMule/libcommon generic includes */
#include "../../../MD4Hash.h"		// Needed for CMD4Hash

/* EC specific includes */
#include "ECCodes.h"		// Needed for EC types
#include "ECTagTypes.h"	// Needed for TagTypes


class CECSocket;
class CValueMap;

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

		std::string StringIPSTL(bool brackets = true)
		{
			std::ostringstream string_ip;
			if (brackets) string_ip << "[";
			string_ip << (int)m_ip[0] << "." << (int)m_ip[1] << "." << (int)m_ip[2] << "." << (int)m_ip[3] << ":" << m_port;
			if (brackets) string_ip << "]";
			return string_ip.str();
		}
		
		#ifdef USE_WX_EXTENSIONS
		wxString StringIP(bool brackets = true) {
			return char2unicode(StringIPSTL(brackets).c_str());
		}
		#endif
		
		uint8 m_ip[4];
		uint16 m_port;
};


/**
 * High level EC packet TAGs handler class
 */

class CECTag {
	public:
		CECTag(ec_tagname_t name, unsigned int length, const void *data);
		// tag for custom data: just init object, alloc buffer and return pointer
		CECTag(ec_tagname_t name, unsigned int length, void **dataptr);
		// Routines for special data types.
		CECTag(ec_tagname_t name, bool data);
		CECTag(ec_tagname_t name, uint8_t data);
		CECTag(ec_tagname_t name, uint16_t data);
		CECTag(ec_tagname_t name, uint32_t data);
		CECTag(ec_tagname_t name, uint64_t data);
		CECTag(ec_tagname_t name, double data);
		CECTag(ec_tagname_t name, const std::string& data);
		CECTag(ec_tagname_t name, const EC_IPv4_t& data);
		CECTag(ec_tagname_t name, const CMD4Hash& data);
		#ifdef USE_WX_EXTENSIONS
		CECTag(ec_tagname_t name, const wxString& data);
		CECTag(ec_tagname_t name, const wxChar* data);
		#endif
		CECTag(ec_tagname_t name, const char* data) { ConstructStringTag(name, data); }
		CECTag();
		CECTag(const CECTag& tag);
		~CECTag(void);

		CECTag&		operator=(const CECTag& rhs);
		bool		operator==(const CECTag& tag) const;
		bool		operator!=(const CECTag& tag) const	{ return !(*this == tag); }
		bool		AddTag(const CECTag& tag, CValueMap* valuemap = NULL);
		void		AddTag(ec_tagname_t name, uint64_t data, CValueMap* valuemap = NULL);
		void		AddTag(ec_tagname_t name, const CMD4Hash& data, CValueMap* valuemap);
		#ifdef USE_WX_EXTENSIONS
		void		AddTag(ec_tagname_t name, const wxString& data, CValueMap* valuemap = NULL);
		#endif

		const CECTag*	GetFirstTagSafe() const { return m_tagList.empty() ? &s_theNullTag : & *m_tagList.begin(); }
		
		const CECTag*	GetTagByName(ec_tagname_t name) const;
		CECTag*			GetTagByName(ec_tagname_t name);
		const CECTag*	GetTagByNameSafe(ec_tagname_t name) const;
		
		size_t			GetTagCount() const { return m_tagList.size(); }
		bool			HasChildTags() const { return !m_tagList.empty(); }
		const void *	GetTagData() const { 
			EC_ASSERT(m_dataType == EC_TAGTYPE_CUSTOM);
			return m_tagData; 
		}
		uint16_t		GetTagDataLen() const { return m_dataLen; }
		uint32_t		GetTagLen() const;
		ec_tagname_t		GetTagName() const { return m_tagName; }
		
		// Retrieving special data types
		uint64_t		GetInt() const;
		bool			IsInt() const { return m_dataType >= EC_TAGTYPE_UINT8 && m_dataType <= EC_TAGTYPE_UINT64; }
		double			GetDoubleData() const;
		std::string		GetStringDataSTL() const;
		bool			IsString() const { return m_dataType == EC_TAGTYPE_STRING; }
		
		#ifdef USE_WX_EXTENSIONS
		wxString GetStringData() const;
		void SetStringData(const wxString& s);
		#endif 
		
		EC_IPv4_t 	GetIPv4Data() const;
		CMD4Hash	GetMD4Data() const;

		void		DebugPrint(int level, bool print_empty) const;
		void		swap(CECTag & t);
		
		// If tag exists, return its value and store it in target (if target != NULL)
		// Else return safe value and don't touch target
		// Allows for one function for old and new style.
		bool		AssignIfExist(ec_tagname_t tagname, bool *target) const;
		uint8_t		AssignIfExist(ec_tagname_t tagname, uint8_t *target) const;
		uint16_t	AssignIfExist(ec_tagname_t tagname, uint16_t *target) const;
		uint32_t	AssignIfExist(ec_tagname_t tagname, uint32_t *target) const;
		uint64_t	AssignIfExist(ec_tagname_t tagname, uint64_t *target) const;
		time_t		AssignIfExist(ec_tagname_t tagname, time_t *target) const;
		double		AssignIfExist(ec_tagname_t tagname, double *target) const;
		float		AssignIfExist(ec_tagname_t tagname, float *target) const;
		CMD4Hash	AssignIfExist(ec_tagname_t tagname, CMD4Hash *target) const;
		std::string	AssignIfExist(ec_tagname_t tagname, std::string *target) const;
		#ifdef USE_WX_EXTENSIONS
		wxString	AssignIfExist(ec_tagname_t tagname, wxString *target) const;
		#endif

		// If tag exists, return true and store it in target
		// Else return false and don't touch target
		bool		AssignIfExist(ec_tagname_t tagname, bool &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, uint8_t &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, uint16_t &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, uint32_t &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, uint64_t &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, time_t &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, double &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, float &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, CMD4Hash &target) const;
		bool		AssignIfExist(ec_tagname_t tagname, std::string &target) const;
		#ifdef USE_WX_EXTENSIONS
		bool		AssignIfExist(ec_tagname_t tagname, wxString &target) const;
		#endif

	protected:

		uint8_t GetType() const { return m_dataType; }

		bool		ReadFromSocket(CECSocket& socket);
		bool		WriteTag(CECSocket& socket) const;
		bool		ReadChildren(CECSocket& socket);
		bool		WriteChildren(CECSocket& socket) const;

	private:
		// To init. the automatic int data
		void InitInt(uint64_t data);

		ec_tagname_t	m_tagName;
		ec_tagtype_t	m_dataType;
		ec_taglen_t		m_dataLen;
		char *			m_tagData;
		void NewData()	{ m_tagData = new char[m_dataLen]; }

		typedef std::list<CECTag> TagList;
		TagList m_tagList;
		
		static const CECTag s_theNullTag;
		
		// To be used by the string constructors.
		void ConstructStringTag(ec_tagname_t name, const std::string& data);

	public:
		// Iteration through child tags
		typedef TagList::const_iterator const_iterator;
		const_iterator begin()	const { return m_tagList.begin(); }
		const_iterator end()	const { return m_tagList.end(); }
};


/**
 * An empty TAG
 *
 * Note, that an "empty" tag is empty because it contains no data, but it still
 * may contain children.
 */
class CECEmptyTag : public CECTag {
	public:
		CECEmptyTag(ec_tagname_t name = 0) : CECTag(name, 0, (const void *) NULL) {}
};

/**
 * An integer TAG
 *
 * This is just to easily overcome ctor ambiguity. It's prettier to write
 *		CECIntTag(name, some_value)
 * instead of
 *		CECTag(name, (uint64)value)
 */
class CECIntTag : public CECTag {
	public:
		CECIntTag(ec_tagname_t name, uint64 data) : CECTag(name, data) {}
};

#endif /* ECTAG_H */
// File_checked_for_headers
