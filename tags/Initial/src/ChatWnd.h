//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef CHATWND_H
#define CHATWND_H

#include <wx/panel.h>		// Needed for wxPanel

#include "resource.h"		// Needed for IDD_CHAT

class CUpDownClient;
class CChatSelector;

// CChatWnd dialog

class CChatWnd : public wxPanel //CResizableDialog
{
  //DECLARE_DYNAMIC(CChatWnd)
public:
	CChatWnd(wxWindow* pParent = NULL);   // standard constructor
	virtual ~CChatWnd();

// Dialog Data
	enum { IDD = IDD_CHAT };
	void StartSession(CUpDownClient* client);
	void Localize();
	CChatSelector* chatselector;
protected:
	virtual bool OnInitDialog(); 
#if 0
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedCsend();
	void	OnShowWindow(bool bShow, unsigned int nStatus);
	virtual bool	PreTranslateMessage(MSG* pMsg);
public:
	afx_msg void OnBnClickedCclose();
#endif
	DECLARE_EVENT_TABLE()

	void OnBnClickedCsend(wxEvent& evt);
	void OnBnClickedCclose(wxEvent& evt);
};

#endif // CHATWND_H
