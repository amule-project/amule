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


#ifndef SERVERWND_H
#define SERVERWND_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ServerWnd.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel
#include <wx/splitter.h>	// Needed for wxSplitter

class CServerListCtrl;

class CServerWnd : public wxPanel //CResizableDialog
{
  DECLARE_DYNAMIC_CLASS(CServerWnd)
    
    CServerWnd() {};

public:
	CServerWnd(wxWindow* pParent, int splitter_pos);   // standard constructor
	virtual ~CServerWnd();
	void Localize();
	void UpdateServerMetFromURL(wxString strURL);
	void UpdateMyInfo();

	CServerListCtrl* serverlistctrl;
protected:
	void OnSashPositionChanged(wxSplitterEvent& evt);
	
public:
	
	void OnBnClickedAddserver(wxCommandEvent& evt);
	void OnBnClickedUpdateservermetfromurl(wxCommandEvent& evt);
	void OnBnClickedResetLog(wxCommandEvent& evt);
	void OnBnClickedResetServerLog(wxCommandEvent& evt);
	
private:

	DECLARE_EVENT_TABLE()
};

#endif // SERVERWND_H
