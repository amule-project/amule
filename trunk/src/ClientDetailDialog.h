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

#ifndef CLIENTDETAILDIALOG_H
#define CLIENTDETAILDIALOG_H

#include <sys/time.h>		// Needed for gettimeofday(2)
#include <wx/defs.h>		// Needed before any other wx/*.h
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include <wx/dialog.h>		// Needed for wxDialog

#include "resource.h"		// Needed for IDD_SOURCEDETAILWND

class CUpDownClient;

// CClientDetailDialog dialog

class CClientDetailDialog : public wxDialog
{
public:
	CClientDetailDialog(wxWindow*parent,CUpDownClient* client);   // standard constructor
	virtual ~CClientDetailDialog();
	virtual bool OnInitDialog();
	enum { IDD = IDD_SOURCEDETAILWND };

protected:
	void OnBnClose(wxCommandEvent& evt);
	DECLARE_EVENT_TABLE()

private:
	CUpDownClient* m_client;
};
#endif // CLIENTDETAILDIALOG_H
