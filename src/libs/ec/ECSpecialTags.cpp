//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 Kry ( elkry@sourceforge.net / http://www.amule.org )
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <vector>

#include <ec/ECPacket.h>		// Needed for CECTag
#include <ec/ECCodes.h>		// Needed for TAGnames
#include <ec/ECSpecialTags.h>	// Needed for special EC tag creator classes

#include <common/Format.h>		// Needed for CFormat

#include <wx/intl.h>		// Needed for _()
#include "KnownFile.h"		// Needed for PS_*

wxString CEC_PartFile_Tag::GetFileStatusString()
{
	uint8 nFileStatus = FileStatus();
	
        if ((nFileStatus == PS_HASHING) || (nFileStatus == PS_WAITINGFORHASH)) {
                return _("Hashing");
        } else {
                switch (nFileStatus) {
                        case PS_COMPLETING:
                                return _("Completing");
                        case PS_COMPLETE:
                                return _("Complete");
                        case PS_PAUSED:
                                return _("Paused");
                        case PS_ERROR:
                                return _("Erroneous");
                        default:
                                if (SourceXferCount() > 0) {
                                        return _("Downloading");
                                } else {
                                        return _("Waiting");
                                }
                }
                // if stopped
        }
}

//
// Search request
//
CEC_Search_Tag::CEC_Search_Tag(wxString &name, EC_SEARCH_TYPE search_type, wxString &file_type,
			wxString &extension, uint32 avail, uint32 min_size, uint32 max_size) : CECTag(EC_TAG_SEARCH_TYPE, (uint32)search_type)
{
	AddTag(CECTag(EC_TAG_SEARCH_NAME, name));
	AddTag(CECTag(EC_TAG_SEARCH_FILE_TYPE, file_type));
	if ( !extension.IsEmpty() ) {
		AddTag(CECTag(EC_TAG_SEARCH_EXTENSION, extension));
	}
	if ( avail ) {
		AddTag(CECTag(EC_TAG_SEARCH_AVAILABILITY, avail));
	}
	if ( min_size != 0 ) {
		AddTag(CECTag(EC_TAG_SEARCH_MIN_SIZE, min_size));
	}
	if ( max_size != 0 ) {
		AddTag(CECTag(EC_TAG_SEARCH_MAX_SIZE, max_size));
	}
}

void FormatValue(CFormat& format, const CECTag* tag)
{
	wxASSERT(tag->GetTagName() == EC_TAG_STAT_NODE_VALUE);

	wxString extra;
	const CECTag *tmp_tag = tag->GetTagByName(EC_TAG_STAT_NODE_VALUE);
	if (tmp_tag) {
		wxString tmp_fmt;
		const CECTag* tmp_vt = tmp_tag->GetTagByName(EC_TAG_STAT_VALUE_TYPE);
		EC_STATTREE_NODE_VALUE_TYPE tmp_valueType = tmp_vt != NULL ? (EC_STATTREE_NODE_VALUE_TYPE)tmp_vt->GetInt8Data() : EC_VALUE_INTEGER;
		switch (tmp_valueType) {
			case EC_VALUE_INTEGER:
				tmp_fmt = wxT("%llu");
				break;
			case EC_VALUE_DOUBLE:
				tmp_fmt = wxT("%.2f%%");	// it's used for percentages
				break;
			default:
				tmp_fmt = wxT("%s");
		}
		CFormat tmp_format(wxT(" (") + tmp_fmt + wxT(")"));
		FormatValue(tmp_format, tmp_tag);
		extra = tmp_format.GetString();
	}

	const CECTag* vt = tag->GetTagByName(EC_TAG_STAT_VALUE_TYPE);
	EC_STATTREE_NODE_VALUE_TYPE valueType = vt != NULL ? (EC_STATTREE_NODE_VALUE_TYPE)vt->GetInt8Data() : EC_VALUE_INTEGER;
	switch (valueType) {
		case EC_VALUE_INTEGER:
			format = format % tag->GetInt64Data();
			break;
		case EC_VALUE_ISTRING:
			format = format % (wxString::Format(wxT("%llu"), tag->GetInt64Data()) + extra);
			break;
		case EC_VALUE_BYTES:
			format = format % (CastItoXBytes(tag->GetInt64Data()) + extra);
			break;
		case EC_VALUE_ISHORT:
			format = format % (CastItoIShort(tag->GetInt64Data()) + extra);
			break;
		case EC_VALUE_TIME:
			format = format % (CastSecondsToHM(tag->GetInt32Data()) + extra);
			break;
		case EC_VALUE_SPEED:
			format = format % (CastItoSpeed(tag->GetInt32Data()) + extra);
			break;
		case EC_VALUE_STRING:
			format = format % (wxGetTranslation(tag->GetStringData()) + extra);
			break;
		case EC_VALUE_DOUBLE:
			format = format % tag->GetDoubleData();
			break;
		default:
			wxASSERT(0);
	}
}

wxString CEC_StatTree_Node_Tag::GetDisplayString() const
{
	wxString en_label = GetStringData();
	wxString my_label = wxGetTranslation(en_label);
	// This is needed for client names, for example
	if (my_label == en_label) {
		if (en_label.Right(4) == wxT(": %s")) {
			my_label = wxGetTranslation(en_label.Mid(0, en_label.Length() - 4)) + wxString(wxT(": %s"));
		}
	}
	CFormat label(my_label);
	for (int i = 0; i < GetTagCount(); ++i) {
		const CECTag *tmp = GetTagByIndex(i);
		if (tmp->GetTagName() == EC_TAG_STAT_NODE_VALUE) {
			FormatValue(label, tmp);
		}
	}
	return label.GetString();
}
