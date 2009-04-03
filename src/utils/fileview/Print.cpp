//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include <tags/TagTypes.h>
#include "../../Tag.h"

#include <cctype>


wxString MakePrintableString(const wxString& str)
{
	wxString retval = wxT("\"");
	for (unsigned i = 0; i < str.Length(); i++) {
		if (std::isprint(str[i])) {
			retval += str[i];
		} else {
			retval += wxString::Format(wxT("\\x%02x"), str[i]);
		}
	}
	retval += wxT("\"");
	return retval;
}

template<> void Print<time_t>(const time_t& time)
{
	Print((uint32_t)time);
	DoPrint(wxT(" ("));
	wxDateTime dt(time);
	DoPrint(dt.Format(wxDefaultDateTimeFormat, wxDateTime::UTC));
	DoPrint(wxT(" UTC)"));
}

template<> void Print<CKadIP>(const CKadIP& ip)
{
	PrintHex((uint32_t)ip);
	DoPrint(wxT(" (") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS((uint32_t)ip)) + wxT(")"));
}

wxString DecodeTagNameID(uint8_t id)
{
#define TEST_NAMEID(TAGNAME)	if (id == TAGNAME) return wxT(#TAGNAME); else

	TEST_NAMEID(FT_FILENAME)
	TEST_NAMEID(FT_FILESIZE)
	TEST_NAMEID(FT_FILESIZE_HI)
	TEST_NAMEID(FT_FILETYPE)
	TEST_NAMEID(FT_FILEFORMAT)
	TEST_NAMEID(FT_LASTSEENCOMPLETE)
	TEST_NAMEID(FT_TRANSFERRED)
	TEST_NAMEID(FT_GAPSTART)
	TEST_NAMEID(FT_GAPEND)
	TEST_NAMEID(FT_PARTFILENAME)
	TEST_NAMEID(FT_OLDDLPRIORITY)
	TEST_NAMEID(FT_STATUS)
	TEST_NAMEID(FT_SOURCES)
	TEST_NAMEID(FT_PERMISSIONS)
	TEST_NAMEID(FT_OLDULPRIORITY)
	TEST_NAMEID(FT_DLPRIORITY)
	TEST_NAMEID(FT_ULPRIORITY)
	TEST_NAMEID(FT_KADLASTPUBLISHKEY)
	TEST_NAMEID(FT_KADLASTPUBLISHSRC)
	TEST_NAMEID(FT_FLAGS)
	TEST_NAMEID(FT_DL_ACTIVE_TIME)
	TEST_NAMEID(FT_CORRUPTEDPARTS)
	TEST_NAMEID(FT_DL_PREVIEW)
	TEST_NAMEID(FT_KADLASTPUBLISHNOTES)
	TEST_NAMEID(FT_AICH_HASH)
	TEST_NAMEID(FT_COMPLETE_SOURCES)
	TEST_NAMEID(FT_PUBLISHINFO)
	TEST_NAMEID(FT_ATTRANSFERRED)
	TEST_NAMEID(FT_ATREQUESTED)
	TEST_NAMEID(FT_ATACCEPTED)
	TEST_NAMEID(FT_CATEGORY)
	TEST_NAMEID(FT_ATTRANSFERREDHI)
	TEST_NAMEID(FT_MEDIA_ARTIST)
	TEST_NAMEID(FT_MEDIA_ALBUM)
	TEST_NAMEID(FT_MEDIA_TITLE)
	TEST_NAMEID(FT_MEDIA_LENGTH)
	TEST_NAMEID(FT_MEDIA_BITRATE)
	TEST_NAMEID(FT_MEDIA_CODEC)
	TEST_NAMEID(FT_FILERATING)
	return wxString::Format(wxT("0x%02x"), id);

#undef TEST_NAMEID
}

wxString DecodeTagName(const wxString& name)
{
#define TEST_TAG(TAGNAME)	if (name == TAGNAME) return wxT(#TAGNAME); else

	TEST_TAG(TAG_FILENAME)
	TEST_TAG(TAG_FILESIZE)
	TEST_TAG(TAG_FILESIZE_HI)
	TEST_TAG(TAG_FILETYPE)
	TEST_TAG(TAG_FILEFORMAT)
	TEST_TAG(TAG_COLLECTION)
	TEST_TAG(TAG_PART_PATH)
	TEST_TAG(TAG_PART_HASH)
	TEST_TAG(TAG_COPIED)
	TEST_TAG(TAG_GAP_START)
	TEST_TAG(TAG_GAP_END)
	TEST_TAG(TAG_DESCRIPTION)
	TEST_TAG(TAG_PING)
	TEST_TAG(TAG_FAIL)
	TEST_TAG(TAG_PREFERENCE)
	TEST_TAG(TAG_PORT)
	TEST_TAG(TAG_IP_ADDRESS)
	TEST_TAG(TAG_VERSION)
	TEST_TAG(TAG_TEMPFILE)
	TEST_TAG(TAG_PRIORITY)
	TEST_TAG(TAG_STATUS)
	TEST_TAG(TAG_SOURCES)
	TEST_TAG(TAG_AVAILABILITY)
	TEST_TAG(TAG_PERMISSIONS)
	TEST_TAG(TAG_QTIME)
	TEST_TAG(TAG_PARTS)
	TEST_TAG(TAG_PUBLISHINFO)
	TEST_TAG(TAG_MEDIA_ARTIST)
	TEST_TAG(TAG_MEDIA_ALBUM)
	TEST_TAG(TAG_MEDIA_TITLE)
	TEST_TAG(TAG_MEDIA_LENGTH)
	TEST_TAG(TAG_MEDIA_BITRATE)
	TEST_TAG(TAG_MEDIA_CODEC)
	TEST_TAG(TAG_ENCRYPTION)
	TEST_TAG(TAG_FILERATING)
	TEST_TAG(TAG_BUDDYHASH)
	TEST_TAG(TAG_CLIENTLOWID)
	TEST_TAG(TAG_SERVERPORT)
	TEST_TAG(TAG_SERVERIP)
	TEST_TAG(TAG_SOURCEUPORT)
	TEST_TAG(TAG_SOURCEPORT)
	TEST_TAG(TAG_SOURCEIP)
	TEST_TAG(TAG_SOURCETYPE)
	return MakePrintableString(name);

#undef TEST_TAG
}

wxString DecodeTagType(uint8_t type)
{
#define TEST_TYPE(TAGTYPE)	if (type == TAGTYPE) return wxT(#TAGTYPE); else

	TEST_TYPE(TAGTYPE_HASH16)
	TEST_TYPE(TAGTYPE_STRING)
	TEST_TYPE(TAGTYPE_UINT32)
	TEST_TYPE(TAGTYPE_FLOAT32)
	TEST_TYPE(TAGTYPE_BOOL)
	TEST_TYPE(TAGTYPE_BOOLARRAY)
	TEST_TYPE(TAGTYPE_BLOB)
	TEST_TYPE(TAGTYPE_UINT16)
	TEST_TYPE(TAGTYPE_UINT8)
	TEST_TYPE(TAGTYPE_BSOB)
	TEST_TYPE(TAGTYPE_UINT64)
	TEST_TYPE(TAGTYPE_STR1)
	TEST_TYPE(TAGTYPE_STR2)
	TEST_TYPE(TAGTYPE_STR3)
	TEST_TYPE(TAGTYPE_STR4)
	TEST_TYPE(TAGTYPE_STR5)
	TEST_TYPE(TAGTYPE_STR6)
	TEST_TYPE(TAGTYPE_STR7)
	TEST_TYPE(TAGTYPE_STR8)
	TEST_TYPE(TAGTYPE_STR9)
	TEST_TYPE(TAGTYPE_STR10)
	TEST_TYPE(TAGTYPE_STR11)
	TEST_TYPE(TAGTYPE_STR12)
	TEST_TYPE(TAGTYPE_STR13)
	TEST_TYPE(TAGTYPE_STR14)
	TEST_TYPE(TAGTYPE_STR15)
	TEST_TYPE(TAGTYPE_STR16)
	TEST_TYPE(TAGTYPE_STR17)
	TEST_TYPE(TAGTYPE_STR18)
	TEST_TYPE(TAGTYPE_STR19)
	TEST_TYPE(TAGTYPE_STR20)
	TEST_TYPE(TAGTYPE_STR21)
	TEST_TYPE(TAGTYPE_STR22)
	return wxString::Format(wxT("0x%02x"), type);

#undef TEST_TYPE
}


template<> void Print<CTag>(const CTag& tag)
{
	DoPrint(wxT("{ "));
	if (tag.GetName().IsEmpty()) {
		DoPrint(DecodeTagNameID(tag.GetNameID()));
	} else {
		DoPrint(DecodeTagName(tag.GetName()));
	}
	DoPrint(wxT(", "));
	DoPrint(DecodeTagType(tag.GetType()));
	DoPrint(wxT(", "));
	if (tag.IsInt()) {
		if (tag.GetName() == TAG_SOURCEIP || tag.GetName() == TAG_SERVERIP) {
			Print(CKadIP(tag.GetInt()));
		} else if (tag.GetName() == TAG_ENCRYPTION) {
			uint8_t enc = tag.GetInt();
			PrintHex(enc);
			wxString encStr;
			if (enc & 0x01) {
				encStr += wxT("CryptSupported");
			}
			if (enc & 0x02) {
				if (!encStr.IsEmpty()) {
					encStr += wxT(" | ");
				}
				encStr += wxT("CryptRequested");
			}
			if (enc & 0x04) {
				if (!encStr.IsEmpty()) {
					encStr += wxT(" | ");
				}
				encStr += wxT("CryptRequired");
			}
			if (enc & 0x08) {
				if (!encStr.IsEmpty()) {
					encStr += wxT(" | ");
				}
				encStr += wxT("DirectCallback");
			}
			if (!encStr.IsEmpty()) {
				DoPrint(wxT(" (") + encStr + wxT(")"));
			}
		} else {
			Print(tag.GetInt());
		}
	} else if (tag.IsStr()) {
		Print(tag.GetStr());
	} else if (tag.IsFloat()) {
		Print(tag.GetFloat());
	} else if (tag.IsHash()) {
		Print(tag.GetHash());
	} else if (tag.IsBlob()) {
		DoPrint(wxString::Format(wxT("(size = %u)"), tag.GetBlobSize()));
	} else if (tag.IsBsob()) {
		DoPrint(wxString::Format(wxT("(size = %u)"), tag.GetBsobSize()));
	} else {
		DoPrint(wxT("(...)"));
	}
	DoPrint(wxT(" }"));
}
