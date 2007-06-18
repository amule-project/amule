//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef FILEDETAILLISTCTRL_H
#define FILEDETAILLISTCTRL_H

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CFileDetailListCtrl : public CMuleListCtrl
{

public: 
	CFileDetailListCtrl(wxWindow * &parent, int id, const wxPoint & pos, wxSize siz, int flags);

private:
	struct SourcenameItem {
		wxString	name;
		long		count;
	};

	static int wxCALLBACK SortProc(long item1, long item2, long sortData);
	void OnSelect(wxListEvent& event);

	DECLARE_EVENT_TABLE()
};
#endif // FILEDETAILLISTCTRL_H
// File_checked_for_headers
