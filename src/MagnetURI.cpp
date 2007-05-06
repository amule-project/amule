//                                                       -*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2007 Dévai Tamás ( gonosztopi@amule.org )
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

#include "MagnetURI.h"


#if USE_STD_STRING
#	ifdef _T
#		undef	_T
#	endif
#	ifdef _C
#		undef _C
#	endif
#	define	_T(str)	str
#	define	_C(ch)	ch
#else
#	// wx/chartype.h defines _T
#	define	_C(ch)	wxChar(ch)
#endif


CMagnetURI::CMagnetURI(const STRING& uri)
{
	if (uri.compare(0, 7, _T("magnet:")) == 0) {
		size_t start = uri.find(_C('?'));
		if (start == STRING::npos) start = uri.length();
		while (start < uri.length() - 1) {
			size_t end = uri.find(_C('&'), start + 1);
			if (end == STRING::npos) end = uri.length();
			size_t pos = uri.find(_C('='), start + 1);
			if (pos == STRING::npos) pos = uri.length();
			if (pos < end) {
				m_fields.push_back(Field_Type(uri.substr(start + 1, pos - start - 1), uri.substr(pos + 1, end - pos - 1)));
			}
			start = end;
		}
	}
}

STRING CMagnetURI::GetLink() const
{
	STRING retval(_T("magnet:"));

	for (List_Type::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
		if (it == m_fields.begin()) {
			retval.append(1, _C('?'));
		} else {
			retval.append(1, _C('&'));
		}
		retval.append(it->first);
		retval.append(1, _C('='));
		retval.append(it->second);
	}
	return retval;
}

CMagnetURI::Value_List CMagnetURI::GetField(const STRING& name) const
{
	Value_List retval;

	for (List_Type::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
		if (it->first.compare(name) == 0) {
			retval.push_back(it->second);
		}
	}
	return retval;
}


bool CMagnetED2KConverter::CanConvertToED2K() const
{
	bool has_urn = false;
	bool has_xl = false;

	for (List_Type::const_iterator it = m_fields.begin(); it != m_fields.end(); ++it) {
		if (it->first.compare(_T("xl")) == 0) {
			has_xl = true;
			continue;
		}
		if (it->first.compare(_T("xt")) == 0) {
			if (it->second.compare(0, 9, _T("urn:ed2k:")) == 0) {
				has_urn = true;
				continue;
			}
		}
		if (has_urn && has_xl) break;
	}
	return has_urn && has_xl;
}

STRING CMagnetED2KConverter::GetED2KLink() const
{
	if (CanConvertToED2K()) {
		STRING dn;
		STRING hash;
		STRING len(GetField(_T("xl")).front());

		Value_List dn_val = GetField(_T("dn"));
		if (!dn_val.empty()) {
			dn = dn_val.front();
		} else {
			// This should never happen. Has anyone seen a link without a file name?
			// Just in case, assign a reasonable(?) name to that unnamed file.
			dn = _T("FileName.ext");
		}
		Value_List urn_list = GetField(_T("xt"));
		for (Value_List::iterator it = urn_list.begin(); it != urn_list.end(); ++it) {
			if (it->compare(0, 9, _T("urn:ed2k:")) == 0) {
				hash = it->substr(9);
				break;
			}
		}
		return STRING(_T("ed2k://|file|")).append(dn).append(1, _C('|')).append(len).append(1, _C('|')).append(hash).append(_T("|/"));
	} else {
		return STRING();
	}
}
