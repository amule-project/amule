//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#ifndef ADDFRIEND_H
#define ADDFRIEND_H

#include <wx/dialog.h>		// Needed for wxDialog

#include "resource.h"		// Needed for IDD_ADDFRIEND

// CAddFriend dialog

class CAddFriend : public wxDialog
{
	DECLARE_DYNAMIC_CLASS(CAddFriend)

public:
	CAddFriend() {};
	CAddFriend(wxWindow* parent);   // standard constructor
	virtual ~CAddFriend() {};
	enum { IDD = IDD_ADDFRIEND };

protected:
	DECLARE_EVENT_TABLE()

private:
	void OnAddBtn(wxEvent& evt);
	void OnCloseBtn(wxEvent& evt);
};

#endif // ADDFRIEND_H
