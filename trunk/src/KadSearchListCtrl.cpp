//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "KadSearchListCtrl.h"
#endif

#include "KadSearchListCtrl.h"
#include "muuli_wdr.h"
#include "kademlia/kademlia/Search.h"


BEGIN_EVENT_TABLE(CKadSearchListCtrl, CMuleListCtrl)
	EVT_LIST_COL_CLICK( -1, CKadSearchListCtrl::OnColumnLClick)
	EVT_LIST_ITEM_ACTIVATED(ID_KADSEARCHLIST, CKadSearchListCtrl::OnItemActivated)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_KADSEARCHLIST, CKadSearchListCtrl::OnRightClick)
END_EVENT_TABLE()

CKadSearchListCtrl::CKadSearchListCtrl(
	wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
	long style, const wxValidator& validator, const wxString& name )
:
CMuleListCtrl( parent, winid, pos, size, style, validator, name )
{
	#warning KAD TODO
}

CKadSearchListCtrl::~CKadSearchListCtrl() {
	#warning KAD TODO
}

void CKadSearchListCtrl::OnColumnLClick(wxListEvent& WXUNUSED(evt)) {
	#warning KAD TODO
}

void CKadSearchListCtrl::OnItemActivated(wxListEvent& WXUNUSED(evt)) {
	#warning KAD TODO
}

void CKadSearchListCtrl::OnRightClick(wxListEvent& WXUNUSED(evt)) {
	#warning KAD TODO
}

void CKadSearchListCtrl::SearchAdd(const Kademlia::CSearch* WXUNUSED(search)) {
	#warning KAD TODO
}

void CKadSearchListCtrl::SearchRem(const Kademlia::CSearch* WXUNUSED(search)) {
	#warning KAD TODO
}

void CKadSearchListCtrl::SearchRef(const Kademlia::CSearch* WXUNUSED(search)) {
	#warning KAD TODO
}
