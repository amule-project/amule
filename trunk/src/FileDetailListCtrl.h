// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef FILEDETAILLISTCTRL_H
#define FILEDETAILLISTCTRL_H

#include "MuleListCtrl.h"	// Needed for CMuleListCtrl

class CFileDetailListCtrl : public CMuleListCtrl
{

public: 
	CFileDetailListCtrl(wxWindow * &parent, int id, const wxPoint & pos, wxSize siz, int flags);
	void UpdateSort(void);

private:
	bool m_SortAscending[2];
	int sortItem;
	struct SourcenameItem {
		wxString	name;
		long		count;
	};

	static int wxCALLBACK CompareListNameItems(long lParam1, long lParam2, long lParamSort);
	void OnColumnClick(wxListEvent& evt);
	void OnSelect(wxListEvent& event);

	DECLARE_EVENT_TABLE()
};
#endif // FILEDETAILLISTCTRL_H
