// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Team ( http://www.amule-project.net )
// This fle Copyright (c) 2003 Kry ( elkry@users.sourceforge.net   http://www.amule-project.net )
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

#ifndef TEXTCLIENT_H
#define TEXTCLIENT_H

#include <wx/string.h>
#include <wx/intl.h>
#include <wx/wx.h>
#define wxUSE_DDE_FOR_IPC  0
#include <wx/ipc.h>

#ifdef AMULECMDDLG
#include <wx/textdlg.h>
#else
#include <wx/cmdline.h>
#endif

#ifndef WIN32
#include "config.h"
#endif

#define theApp (*((CamulecmdApp*)wxTheApp))

#ifndef AMULECMDDLG
 static const wxCmdLineEntryDesc cmdLineDesc[] =
{
//	{ wxCMD_LINE_OPTION, "h", "help",  "show this help" },
	{ wxCMD_LINE_OPTION, wxT("rh"), wxT("remote-host"),  wxT("host where aMule is running (default localhost)")},
	{ wxCMD_LINE_OPTION, wxT("p"), wxT("port"),   wxT("aMule's port for External Connection"),wxCMD_LINE_VAL_NUMBER},

	{ wxCMD_LINE_NONE }
};
#endif


#ifdef AMULECMDDLG
class CamulecmdFrame : public wxFrame
{
public:
    // ctor(s)
    CamulecmdFrame(const wxString& title, const wxPoint& pos, const wxSize& size,
            long style = wxDEFAULT_FRAME_STYLE);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnComandEnter(wxCommandEvent& event);
    void OnSize( wxSizeEvent& event );
    wxTextCtrl    *log_text;
    wxTextCtrl    *cmd_control;
private:
    wxLog*	logTargetOld;
    // any class wishing to process wxWindows events must use this macro
    DECLARE_EVENT_TABLE()
};
#endif

class CamulecmdApp : public wxApp {
public:
	
#ifndef AMULECMDDLG
	virtual int 			OnRun();
	virtual void OnInitCmdLine(wxCmdLineParser& amulecmd_parser) {
		amulecmd_parser.SetDesc(cmdLineDesc); 	
	}
	virtual bool OnCmdLineParsed(wxCmdLineParser& amulecmd_parser);
	void ParseCommandLine();
#else
	virtual bool 			OnInit();
	virtual int			OnExit();
	CamulecmdFrame *frame;
#endif
	wxString sPort;
	wxString hostName;
};

/*
class MuleConnection:public wxConnection {
  public:
    bool OnAdvise(const wxString& topic, const wxString& item, wxChar *data, int size, wxIPCFormat format);
    bool OnDisconnect();
};

class MuleClient:public wxClient {
  public:
      wxConnectionBase *OnMakeConnection();
 };
*/
#endif // TEXTCLIENT_H
