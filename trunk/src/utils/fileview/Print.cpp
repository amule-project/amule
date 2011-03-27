//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "Print.h"
#include <wx/datetime.h>
#include <tags/FileTags.h>
#include <tags/ServerTags.h>
#include <tags/TagTypes.h>
#include <protocol/ed2k/Client2Server/UDP.h>
#include "../../OtherFunctions.h"
#include "../../Friend.h"			// Needed for FF_NAME
#include "../../KnownFile.h"		// Needed for PR_*
#include "../../NetworkFunctions.h"	// Needed for Uint32toStringIP

#include <cctype>
#include <map>
#include <iostream>

SDMODE g_stringDecodeMode;

wxString MakePrintableString(const wxString& s)
{
	wxString str = s;
	unsigned c = 0;
	for (unsigned i = 0; i < str.length(); i++) {
		c |= (wxChar) str[i];
	}

	if (c <= 0xff && GetStringsMode() != SD_NONE) {
		wxCharBuffer buf = str.To8BitData();
		wxString test = UTF82unicode(buf.data());
		if (!test.empty()) {
			str = test;
			c = 0;
			for (unsigned i = 0; i < str.length(); i++) {
				c |= (wxChar) str[i];
			}
		}
	}

	wxString retval = wxT("\"");

	if (GetStringsMode() == SD_DISPLAY) {
		retval += str;
	} else if (GetStringsMode() == SD_UTF8) {
		str = wxString::From8BitData((const char *)str.utf8_str());
		c = 0xff;
	}

	if (GetStringsMode() != SD_DISPLAY) {
		for (unsigned i = 0; i < str.length(); i++) {
			if (GetStringsMode() == SD_NONE ? ((unsigned)str[i] >= ' ' && (unsigned)str[i] <= 0x7f) : std::isprint(str[i])) {
				retval += str[i];
			} else if (c <= 0xff) {
				retval += wxString::Format(wxT("\\x%02x"), str[i]);
			} else {
				retval += wxString::Format(wxT("\\u%04x"), str[i]);
			}
		}
	}

	retval += wxT("\"");
	return retval;
}

std::ostream& operator<<(std::ostream& x, const CTimeT& y)
{
	if ((time_t)y != 0) {
		wxDateTime dt((time_t)y);
		return x << (time_t)y << " (" << dt.Format(wxDefaultDateTimeFormat, wxDateTime::UTC) << " UTC)";
	} else {
		return x << "0 (Never)";
	}
}

std::ostream& operator<<(std::ostream& x, const CKadIP& ip)
{
	return x << hex(ip) << " (" << Uint32toStringIP(wxUINT32_SWAP_ALWAYS(ip)) << ')';
}

std::ostream& operator<<(std::ostream& x, const CeD2kIP& ip)
{
	return x << hex(ip) << " (" << Uint32toStringIP(ip) << ')';
}

static inline wxString TagNameString(const wxString& name)
{
	if (name.length() == 1) {
		return wxString::Format(wxT("\"\\x%02x\""), name[0]);
	} else if (name.length() > 1) {
		if (name[0] == FT_GAPSTART || name[0] == FT_GAPEND) {
			return wxString::Format(wxT("\"\\x%02x"), name[0]) + name.substr(1) + wxT("\"");
		}
	}
	return wxT("\"") + name + wxT("\"");
}

#define TEST_VALUE(VALUE)	if (value == VALUE) return #VALUE; else

const char* DecodeTagNameID(uint8_t value)
{
	TEST_VALUE(FT_FILENAME)
	TEST_VALUE(FT_FILESIZE)
	TEST_VALUE(FT_FILESIZE_HI)
	TEST_VALUE(FT_FILETYPE)
	TEST_VALUE(FT_FILEFORMAT)
	TEST_VALUE(FT_LASTSEENCOMPLETE)
	TEST_VALUE(FT_TRANSFERRED)
	TEST_VALUE(FT_GAPSTART)
	TEST_VALUE(FT_GAPEND)
	TEST_VALUE(FT_PARTFILENAME)
	TEST_VALUE(FT_OLDDLPRIORITY)
	TEST_VALUE(FT_STATUS)
	TEST_VALUE(FT_SOURCES)
	TEST_VALUE(FT_PERMISSIONS)
	TEST_VALUE(FT_OLDULPRIORITY)
	TEST_VALUE(FT_DLPRIORITY)
	TEST_VALUE(FT_ULPRIORITY)
	TEST_VALUE(FT_KADLASTPUBLISHKEY)
	TEST_VALUE(FT_KADLASTPUBLISHSRC)
	TEST_VALUE(FT_FLAGS)
	TEST_VALUE(FT_DL_ACTIVE_TIME)
	TEST_VALUE(FT_CORRUPTEDPARTS)
	TEST_VALUE(FT_DL_PREVIEW)
	TEST_VALUE(FT_KADLASTPUBLISHNOTES)
	TEST_VALUE(FT_AICH_HASH)
	TEST_VALUE(FT_COMPLETE_SOURCES)
	TEST_VALUE(FT_PUBLISHINFO)
	TEST_VALUE(FT_ATTRANSFERRED)
	TEST_VALUE(FT_ATREQUESTED)
	TEST_VALUE(FT_ATACCEPTED)
	TEST_VALUE(FT_CATEGORY)
	TEST_VALUE(FT_ATTRANSFERREDHI)
	TEST_VALUE(FT_MEDIA_ARTIST)
	TEST_VALUE(FT_MEDIA_ALBUM)
	TEST_VALUE(FT_MEDIA_TITLE)
	TEST_VALUE(FT_MEDIA_LENGTH)
	TEST_VALUE(FT_MEDIA_BITRATE)
	TEST_VALUE(FT_MEDIA_CODEC)
	TEST_VALUE(FT_FILERATING)
	return "??";
}

const char* DecodeTagName(const wxString& value)
{
	TEST_VALUE(TAG_FILENAME)
	TEST_VALUE(TAG_FILESIZE)
	TEST_VALUE(TAG_FILESIZE_HI)
	TEST_VALUE(TAG_FILETYPE)
	TEST_VALUE(TAG_FILEFORMAT)
	TEST_VALUE(TAG_COLLECTION)
	TEST_VALUE(TAG_PART_PATH)
	TEST_VALUE(TAG_PART_HASH)
	TEST_VALUE(TAG_COPIED)
	TEST_VALUE(TAG_GAP_START)
	TEST_VALUE(TAG_GAP_END)
	TEST_VALUE(TAG_DESCRIPTION)
	TEST_VALUE(TAG_PING)
	TEST_VALUE(TAG_FAIL)
	TEST_VALUE(TAG_PREFERENCE)
	TEST_VALUE(TAG_PORT)
	TEST_VALUE(TAG_IP_ADDRESS)
	TEST_VALUE(TAG_VERSION)
	TEST_VALUE(TAG_TEMPFILE)
	TEST_VALUE(TAG_PRIORITY)
	TEST_VALUE(TAG_STATUS)
	TEST_VALUE(TAG_SOURCES)
	TEST_VALUE(TAG_AVAILABILITY)
	TEST_VALUE(TAG_PERMISSIONS)
	TEST_VALUE(TAG_QTIME)
	TEST_VALUE(TAG_PARTS)
	TEST_VALUE(TAG_PUBLISHINFO)
	TEST_VALUE(TAG_MEDIA_ARTIST)
	TEST_VALUE(TAG_MEDIA_ALBUM)
	TEST_VALUE(TAG_MEDIA_TITLE)
	TEST_VALUE(TAG_MEDIA_LENGTH)
	TEST_VALUE(TAG_MEDIA_BITRATE)
	TEST_VALUE(TAG_MEDIA_CODEC)
	TEST_VALUE(TAG_ENCRYPTION)
	TEST_VALUE(TAG_FILERATING)
	TEST_VALUE(TAG_BUDDYHASH)
	TEST_VALUE(TAG_CLIENTLOWID)
	TEST_VALUE(TAG_SERVERPORT)
	TEST_VALUE(TAG_SERVERIP)
	TEST_VALUE(TAG_SOURCEUPORT)
	TEST_VALUE(TAG_SOURCEPORT)
	TEST_VALUE(TAG_SOURCEIP)
	TEST_VALUE(TAG_SOURCETYPE)
	return "??";
}

const char* DecodeTagType(uint8_t value)
{
	TEST_VALUE(TAGTYPE_HASH16)
	TEST_VALUE(TAGTYPE_STRING)
	TEST_VALUE(TAGTYPE_UINT32)
	TEST_VALUE(TAGTYPE_FLOAT32)
	TEST_VALUE(TAGTYPE_BOOL)
	TEST_VALUE(TAGTYPE_BOOLARRAY)
	TEST_VALUE(TAGTYPE_BLOB)
	TEST_VALUE(TAGTYPE_UINT16)
	TEST_VALUE(TAGTYPE_UINT8)
	TEST_VALUE(TAGTYPE_BSOB)
	TEST_VALUE(TAGTYPE_UINT64)
	TEST_VALUE(TAGTYPE_STR1)
	TEST_VALUE(TAGTYPE_STR2)
	TEST_VALUE(TAGTYPE_STR3)
	TEST_VALUE(TAGTYPE_STR4)
	TEST_VALUE(TAGTYPE_STR5)
	TEST_VALUE(TAGTYPE_STR6)
	TEST_VALUE(TAGTYPE_STR7)
	TEST_VALUE(TAGTYPE_STR8)
	TEST_VALUE(TAGTYPE_STR9)
	TEST_VALUE(TAGTYPE_STR10)
	TEST_VALUE(TAGTYPE_STR11)
	TEST_VALUE(TAGTYPE_STR12)
	TEST_VALUE(TAGTYPE_STR13)
	TEST_VALUE(TAGTYPE_STR14)
	TEST_VALUE(TAGTYPE_STR15)
	TEST_VALUE(TAGTYPE_STR16)
	TEST_VALUE(TAGTYPE_STR17)
	TEST_VALUE(TAGTYPE_STR18)
	TEST_VALUE(TAGTYPE_STR19)
	TEST_VALUE(TAGTYPE_STR20)
	TEST_VALUE(TAGTYPE_STR21)
	TEST_VALUE(TAGTYPE_STR22)
	return "??";
}

const char* DecodeServerTagNameID(uint8_t value)
{
	TEST_VALUE(ST_SERVERNAME)
	TEST_VALUE(ST_DESCRIPTION)
	TEST_VALUE(ST_PING)
	TEST_VALUE(ST_FAIL)
	TEST_VALUE(ST_PREFERENCE)
	TEST_VALUE(ST_DYNIP)
	TEST_VALUE(ST_LASTPING_DEPRECATED)
	TEST_VALUE(ST_MAXUSERS)
	TEST_VALUE(ST_SOFTFILES)
	TEST_VALUE(ST_HARDFILES)
	TEST_VALUE(ST_LASTPING)
	TEST_VALUE(ST_VERSION)
	TEST_VALUE(ST_UDPFLAGS)
	TEST_VALUE(ST_AUXPORTSLIST)
	TEST_VALUE(ST_LOWIDUSERS)
	TEST_VALUE(ST_UDPKEY)
	TEST_VALUE(ST_UDPKEYIP)
	TEST_VALUE(ST_TCPPORTOBFUSCATION)
	TEST_VALUE(ST_UDPPORTOBFUSCATION)
	return "??";
}

const char* DecodeFriendTagNameID(uint8_t value)
{
	TEST_VALUE(FF_NAME)
	return "??";
}

typedef std::map<uint32_t, const char*> FlagMap;

class CFlagDecoder
{
      public:
	void AddFlag(uint32_t bit, const char* name)
	{
		m_flags[bit] = name;
	}

	wxString DecodeFlags(uint32_t flags)
	{
		uint32_t cur_flag = 1;
		wxString result;
		while (flags) {
			if (flags & 1) {
				if (!result.empty()) {
					result += wxT(" | ");
				}
				FlagMap::const_iterator it = m_flags.find(cur_flag);
				if (it != m_flags.end()) {
					result += wxString::FromAscii(it->second);
				} else {
					result += wxString::Format(wxT("%#x"), cur_flag);
				}
			}
			cur_flag <<= 1;
			flags >>= 1;
		}
		return result;
	}

      private:
	FlagMap	m_flags;
};

#define ADD_DECODER_FLAG(decoder, FLAG)	decoder.AddFlag(FLAG, #FLAG)

std::ostream& operator<<(std::ostream& out, const CTag& tag)
{
	out << "{ ";
	if (tag.GetName().empty()) {
		out << hex(tag.GetNameID()) << ' ' << DecodeTagNameID(tag.GetNameID());
	} else {
		out << TagNameString(tag.GetName()) << ' ';
		bool name_decoded = false;
		if (tag.IsInt() && tag.GetName().length() > 1) {
			wxCharBuffer ascii_name = tag.GetName().ToAscii();
			char gap_mark = ascii_name ? ascii_name[(size_t)0] : 0;
			if (gap_mark == FT_GAPSTART || gap_mark == FT_GAPEND) {
				unsigned long gapkey;
				if (tag.GetName().Mid(1).ToULong(&gapkey)) {
					out << DecodeTagNameID(gap_mark) << wxString::Format(wxT("[%lu]"), gapkey);
					name_decoded = true;
				}
			}
		}
		if (!name_decoded) {
			out << DecodeTagName(tag.GetName());
		}
	}
	out << ", " << (unsigned)tag.GetType() << ' ' << DecodeTagType(tag.GetType()) << ", ";
	if (tag.IsInt()) {
		if (tag.GetName() == TAG_SOURCEIP) {
			out << CKadIP(tag.GetInt());
		} else if (tag.GetName() == TAG_SERVERIP) {
			out << CeD2kIP(tag.GetInt());
		} else if (tag.GetName() == TAG_ENCRYPTION) {
			uint8_t enc = tag.GetInt();
			out << hex(enc);
			if (enc) {
				CFlagDecoder decoder;
				decoder.AddFlag(0x01, "CryptSupported");
				decoder.AddFlag(0x02, "CryptRequested");
				decoder.AddFlag(0x04, "CryptRequired");
				decoder.AddFlag(0x08, "SupportsDirectCallback");
				out << " (" << decoder.DecodeFlags(enc) << ')';
			}
		} else if (tag.GetNameID() == FT_LASTSEENCOMPLETE
			   || tag.GetNameID() == FT_KADLASTPUBLISHSRC
			   || tag.GetNameID() == FT_KADLASTPUBLISHNOTES
			   || tag.GetNameID() == FT_KADLASTPUBLISHKEY
			   ) {
			out << CTimeT(tag.GetInt());
		} else if (tag.GetName() == TAG_FILESIZE
			   || tag.GetNameID() == FT_FILESIZE
			   || tag.GetNameID() == FT_TRANSFERRED
			   || tag.GetNameID() == FT_ATTRANSFERRED
			   ) {
			out << tag.GetInt() << " (" << CastItoXBytes(tag.GetInt()) << ')';
		} else if (tag.GetNameID() == FT_ULPRIORITY
			   || tag.GetNameID() == FT_DLPRIORITY
			   || tag.GetNameID() == FT_OLDULPRIORITY
			   || tag.GetNameID() == FT_OLDDLPRIORITY
			   ) {
			out << tag.GetInt() << ' ';
			switch (tag.GetInt()) {
				case PR_VERYLOW:	out << "PR_VERYLOW"; break;
				case PR_LOW:		out << "PR_LOW"; break;
				case PR_NORMAL:		out << "PR_NORMAL"; break;
				case PR_HIGH:		out << "PR_HIGH"; break;
				case PR_VERYHIGH:	out << "PR_VERYHIGH"; break;
				case PR_AUTO:		out << "PR_AUTO"; break;
				case PR_POWERSHARE:	out << "PR_POWERSHARE"; break;
			}
		} else {
			out << tag.GetInt();
		}
	} else if (tag.IsStr()) {
		out << MakePrintableString(tag.GetStr());
	} else if (tag.IsFloat()) {
		out << tag.GetFloat();
	} else if (tag.IsHash()) {
		out << tag.GetHash();
	} else if (tag.IsBlob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBlobSize());
	} else if (tag.IsBsob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBsobSize());
	} else {
		out << "(...)";
	}
	out << " }";
	return out;
}

std::ostream& operator<<(std::ostream& out, const CServerTag& tag)
{
	out << "{ ";
	if (tag.GetName().empty()) {
		out << hex(tag.GetNameID()) << ' ' << DecodeServerTagNameID(tag.GetNameID());
	} else {
		out << TagNameString(tag.GetName());
	}
	out << ", " << (unsigned)tag.GetType() << ' ' << DecodeTagType(tag.GetType()) << ", ";
	if (tag.IsInt()) {
		if (tag.GetNameID() == ST_DYNIP || tag.GetNameID() == ST_UDPKEYIP) {
			out << CeD2kIP(tag.GetInt());
		} else if (tag.GetNameID() == ST_UDPKEY) {
			out << hex((uint32_t)tag.GetInt());
		} else if (tag.GetNameID() == ST_UDPFLAGS) {
			uint32_t flags = tag.GetInt();
			out << hex(flags);
			if (flags) {
				CFlagDecoder decoder;
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_EXT_GETSOURCES);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_EXT_GETFILES);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_NEWTAGS);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_UNICODE);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_EXT_GETSOURCES2);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_LARGEFILES);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_UDPOBFUSCATION);
				ADD_DECODER_FLAG(decoder, SRV_UDPFLG_TCPOBFUSCATION);
				out << " (" << decoder.DecodeFlags(flags) << ')';
			}
		} else if (tag.GetNameID() == ST_PREFERENCE) {
			out << tag.GetInt() << ' ';
			switch (tag.GetInt()) {
				case 0: out << "SRV_PR_NORMAL"; break;
				case 1: out << "SRV_PR_HIGH"; break;
				case 2: out << "SRV_PR_LOW"; break;
				default: out << "??";
			}
		} else {
			out << tag.GetInt();
		}
	} else if (tag.IsStr()) {
		out << MakePrintableString(tag.GetStr());
	} else if (tag.IsFloat()) {
		out << tag.GetFloat();
	} else if (tag.IsHash()) {
		out << tag.GetHash();
	} else if (tag.IsBlob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBlobSize());
	} else if (tag.IsBsob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBsobSize());
	} else {
		out << "(...)";
	}
	out << " }";
	return out;
}

std::ostream& operator<<(std::ostream& out, const CFriendTag& tag)
{
	out << "{ ";
	if (tag.GetName().empty()) {
		out << hex(tag.GetNameID()) << ' ' << DecodeFriendTagNameID(tag.GetNameID());
	} else {
		out << TagNameString(tag.GetName());
	}
	out << ", " << (unsigned)tag.GetType() << ' ' << DecodeTagType(tag.GetType()) << ", ";
	if (tag.IsInt()) {
		out << tag.GetInt();
	} else if (tag.IsStr()) {
		out << MakePrintableString(tag.GetStr());
	} else if (tag.IsFloat()) {
		out << tag.GetFloat();
	} else if (tag.IsHash()) {
		out << tag.GetHash();
	} else if (tag.IsBlob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBlobSize());
	} else if (tag.IsBsob()) {
		out << wxString::Format(wxT("(size = %u)"), tag.GetBsobSize());
	} else {
		out << "(...)";
	}
	out << " }";
	return out;
}
