//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SHAREDFILEPEERSLISTCTRL_H
#define SHAREDFILEPEERSLISTCTRL_H

#include "GenericClientListCtrl.h"	// Needed for CGenericClientListCtrl

/**
 * This class is responsible for representing the peers for a shared file.
 */
class CSharedFilePeersListCtrl : public CGenericClientListCtrl
{
public:
	/**
	 * Constructor.
	 * 
	 * @see CGenericClientListCtrl::CGenericClientListCtrl for documentation of parameters.
	 */
	 CSharedFilePeersListCtrl(
	            wxWindow *parent,
                wxWindowID winid = -1,
                const wxPoint &pos = wxDefaultPosition,
                const wxSize &size = wxDefaultSize,
                long style = wxLC_ICON,
                const wxValidator& validator = wxDefaultValidator,
                const wxString &name = wxT("peerslistctrl") );
				
	/**
	 * Destructor.
	 */	 
	virtual	~CSharedFilePeersListCtrl();	

private:
	virtual CamuleDlg::DialogType GetParentDialog() { return CamuleDlg::DT_SHARED_WND; }

	virtual void SetShowSources(CKnownFile * f, bool b) const;

	static int wxCALLBACK SourceSortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData);
	
	bool IsShowingDownloadSources() const { return false; }
	
	DECLARE_EVENT_TABLE()
};

#endif
