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

#ifndef __KADSEARCHLISTCTRL__

#define __KADSEARCHLISTCTRL__

#include "MuleListCtrl.h"
#include "kademlia/kademlia/Search.h"

class CKadSearchListCtrl : public CMuleListCtrl
{
public:
	/**
	 * Constructor.
	 * 
	 * @see CMuleListCtrl::CMuleListCtrl for documentation of parameters.
	 */
	 CKadSearchListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("kadsearchlistctrl") );
				
	/**
	 * Destructor.
	 */	 
	virtual	~CKadSearchListCtrl();	

	/*
	 * Absolutely void. Filler.
	 */
	void SearchAdd(const Kademlia::CSearch* search);

	/*
	 * Absolutely void. Filler.
	 */
	 void SearchRem(const Kademlia::CSearch* search);

	/*
	 * Absolutely void. Filler.
	 */
	 void SearchRef(const Kademlia::CSearch* search);


private:
	
	DECLARE_EVENT_TABLE();

	void OnColumnLClick(wxListEvent& evt);
	void OnItemActivated(wxListEvent& evt);
	void OnRightClick(wxListEvent& evt);

};


#endif // __KADSEARCHLISTCTRL__
