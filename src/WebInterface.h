/*  This file is part of aMule project
 *  
 *  aMule Copyright (C) 2003-2004 aMule Team (http://www.amule.org)
 *  This file Copyright (C) 2003 Kry (elkry@sourceforge.net)
 *  This file Copyright (C) 2004 shakraw <shakraw@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License 
 *  as published by the Free Software Foundation; either 
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif

#ifndef WIN32
	#include "config.h"
#endif

#pragma interface
//-------------------------------------------------------------------
//
// wxUSE_GUI will only be defined after this include
// 
#include "ExternalConnector.h"
//-------------------------------------------------------------------

#if wxUSE_GUI
#include <wx/textctrl.h>	// For wxTextCtrl
#include <wx/timer.h>		// For wxTimer
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
	void ShowHelp();
	void ShowGreet();
	void Pre_Shell();
	int ProcessCommand(int ID);

	// other command line switches
	void	OnInitCmdLine(wxCmdLineParser& amuleweb_parser);
	bool	OnCmdLineParsed(wxCmdLineParser& parser);
	wxString m_TemplateFileName;
	bool	m_HasTemplate;
	bool	m_UseGzip;
	wxString m_AdminPass, m_GuestPass;
	bool	m_AllowGuest;
	bool	m_bForcedUseGzip;
	bool	m_bForcedAdminPassword;
	bool	m_bForcedAllowGuest;
	bool	m_bForcedGuestPassword;
	
#if wxUSE_GUI
public:
	void LocalShow(const wxString &s);
	CamulewebFrame 	*frame;
private:
	// GUI Version
	virtual bool	OnInit();
	int		OnExit();
#else
public:
	void Post_Shell();
private:
	// Command line version
	virtual int 	OnRun();
#endif
};

/******************************************************************************
 * The next set of conditionals are used to find out the value of wxUSE_GUI
 ******************************************************************************/
#if 0
#ifdef wxUSE_GUI
	#warning wxUSE_GUI defined
#else
	#warning wxUSE_GUI undefined
	#error wxUSE_GUI cannot be undefined
#endif

#if wxUSE_GUI
	#warning wxUSE_GUI non-zero
	#error wxUSE_GUI cannot be non-zero
#else
	#warning wxUSE_GUI zero
#endif
#endif
/******************************************************************************/

#endif // WEBINTERFACE_H

