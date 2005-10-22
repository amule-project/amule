//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2005 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2005 Kry ( elkry@sourceforge.net )
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

//-------------------------------------------------------------------
//
// wxUSE_GUI will only be defined after this include
// 
#include "ExternalConnector.h"
//-------------------------------------------------------------------
#include "CMD4Hash.h"

#if wxUSE_GUI
#include <wx/textctrl.h>	// For wxTextCtrl
#include <wx/timer.h>		// For wxTimer
#include <wx/frame.h>		// For wxFrame

class CamulewebFrame : public wxFrame
{
public:
	CamulewebFrame(const wxString& title, const wxPoint& pos,
		const wxSize& size, long style = wxDEFAULT_FRAME_STYLE);

	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnCommandEnter(wxCommandEvent &event);
	void OnIdle(wxIdleEvent &e);
	void OnTimerEvent(wxTimerEvent &WXUNUSED(event));
	wxTextCtrl *log_text;
	wxTextCtrl *cmd_control;
	wxTimer *m_timer;

private:
	wxLog *logTargetOld;
	// any class wishing to process wxWindows events must use this macro
	DECLARE_EVENT_TABLE()
};
#endif

class CamulewebApp : public CaMuleExternalConnector
{
public:
	const wxString GetGreetingTitle();
	void Pre_Shell();
	void LoadConfigFile();
	void SaveConfigFile();
	void LoadAmuleConfig(CECFileConfig& cfg);
	bool GetTemplateDir(const wxString& templateName, wxString& templateDir);
	bool CheckDirForTemplate(wxString& dir, const wxString& tmpl);

	// other command line switches
	void	OnInitCmdLine(wxCmdLineParser& amuleweb_parser);
	bool	OnCmdLineParsed(wxCmdLineParser& parser);

	// class data
	wxString m_TemplateName;
	wxString m_TemplateDir;
	wxString m_TemplateFileName;
	bool	m_UseGzip;
	CMD4Hash m_AdminPass, m_GuestPass;
	bool	m_AllowGuest;

	long		m_WebserverPort;
	unsigned int	m_PageRefresh;
	bool		m_LoadSettingsFromAmule;

	bool		m_UsePhp;

	bool		m_TemplateOk;

#if wxUSE_GUI
public:
	void LocalShow(const wxString &s);
	CamulewebFrame 	*frame;
private:
	int		OnExit();
#else
public:
	void Post_Shell();
private:
#endif
	virtual bool	OnInit();
	virtual int 	OnRun();
};

#endif // WEBINTERFACE_H

