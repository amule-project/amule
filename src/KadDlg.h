//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal (Kry) ( kry@amule.org / http://www.amule.org )
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

#ifndef KADDLG_H
#define KADDLG_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "KadDlg.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include "Types.h"

class CNodeListCtrl;	
class wxListEvent;
class wxCommandEvent;
class wxMouseEvent;
	
class CMD4Hash;

namespace Kademlia {
	class CContact;
}

class CKadDlg : public wxPanel {
public:
	CKadDlg(wxWindow* pParent);   
	~CKadDlg() {};
		
	void ShowNodes() const;
	void HideNodes() const;
	void AddNode(const Kademlia::CContact* contact) const;
	void RemoveNode(const Kademlia::CContact* contact) const;
	void RefreshNode(const Kademlia::CContact* contact) const;

private:

	CNodeListCtrl* NodesList;
		
	// Event handlers
	void		OnBnClickedBootstrapClient(wxCommandEvent& evt);
	void		OnBnClickedBootstrapKnown(wxCommandEvent& evt);
	void		OnBnClickedDisconnectKad(wxCommandEvent& evt);
	void		OnBnClickedUpdateNodeList(wxCommandEvent& evt);
	void		OnFieldsChange(wxCommandEvent& evt);

	DECLARE_EVENT_TABLE()
};

#endif // KADDLG_H
