// This file is part of the aMule project.
//
// Copyright (c) 2003,
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef PPGTWEAKS_H
#define PPGTWEAKS_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/panel.h>		// Needed for wxPanel

#include "types.h"		// Needed for uint8
#include "resource.h"		// Needed for IDD_PPG_TWEAKS

class CPreferences;

// CPPgTweaks dialog

class CPPgTweaks : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(CPPgTweaks)

	CPPgTweaks() {};
public:
	CPPgTweaks(wxWindow* parent);
	virtual ~CPPgTweaks();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs;}

// Dialog Data

	enum { IDD = IDD_PPG_TWEAKS };

protected:
	CPreferences *app_prefs;
	uint8 m_iFileBufferSize;
	uint8 m_iQueueSize;
	unsigned int m_uServerKeepAliveTimeout;
	unsigned int m_uListRefresh;
	wxPanel* _panel;

	DECLARE_EVENT_TABLE()

	void OnHScroll(wxScrollEvent& evt);

public:
	virtual bool OnInitDialog();
	void LoadSettings(void);
	virtual bool OnApply();

	void Localize(void);

};

#endif // PPGTWEAKS_H
