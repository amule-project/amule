//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2005 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2005 Kry ( elkry@sourceforge.net / http://www.amule.org )
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

#ifdef HAVE_CONFIG_H
	#include "config.h"	// For VERSION
#endif

#ifndef __WXMSW__
	#include <unistd.h>
#endif

#include <wx/filename.h>	// Needed for wxFileName

#if wxUSE_GUI
	#include <wx/statline.h>
#endif

#include <cstdio>

#ifdef __WXMAC__
	#include <CoreFoundation/CFBundle.h>
	#include <ApplicationServices/ApplicationServices.h>
	#include <wx/mac/corefoundation/cfstring.h>
#endif

#include "ECFileConfig.h"	// Needed for CECFileConfig
#include "MD5Sum.h"
#include "OtherFunctions.h"
#include "WebInterface.h"
#include "WebServer.h"

//-------------------------------------------------------------------

#define APP_INIT_SIZE_X 640
#define APP_INIT_SIZE_Y 480

#define theApp (*((CamulewebApp*)wxTheApp))

static CWebServerBase *webserver = NULL;

#if wxUSE_GUI && wxUSE_TIMER
	class MyTimer *mytimer;
#endif

//-------------------------------------------------------------------
IMPLEMENT_APP(CamulewebApp)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
#if wxUSE_GUI
//-------------------------------------------------------------------
// IDs for the controls and the menu commands
enum {
    // menu items
    amuleweb_Quit = 1,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    amuleweb_About = wxID_ABOUT,
    Event_Comand_ID = 32001,
    amuleFrame_ID = 32000,
    Timer_ID
};

BEGIN_EVENT_TABLE(CamulewebFrame, wxFrame)
	EVT_MENU(amuleweb_Quit,  CamulewebFrame::OnQuit)
	EVT_MENU(amuleweb_About, CamulewebFrame::OnAbout)
	EVT_TEXT_ENTER(Event_Comand_ID, CamulewebFrame::OnCommandEnter)
	EVT_IDLE(CamulewebFrame::OnIdle)
	EVT_TIMER(Timer_ID, CamulewebFrame::OnTimerEvent)
END_EVENT_TABLE()


CamulewebFrame::CamulewebFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, amuleFrame_ID, title, pos, size, style)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(amuleweb_Quit, _("E&xit\tAlt-X"), _("Quit amuleweb"));

	wxMenu *helpMenu = new wxMenu;
	helpMenu->Append(amuleweb_About, _("&About...\tF1"), _("Show about dialog"));

	// now append the freshly created menu to the menu bar...
	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(helpMenu, _("&Help"));

	// ... and attach this menu bar to the frame
	SetMenuBar(menuBar);

	// Text controls and sizer
	wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);
	log_text = new wxTextCtrl(this, -1, wxEmptyString,
		wxDefaultPosition, wxSize(APP_INIT_SIZE_X,APP_INIT_SIZE_Y),
		wxTE_MULTILINE|wxVSCROLL|wxTE_READONLY);
	log_text->SetBackgroundColour(wxT("wheat"));
	log_text->SetDefaultStyle(
		wxTextAttr(
			wxNullColour, 
			wxT("wheat"), 
			wxFont(10, wxMODERN, wxNORMAL, wxNORMAL)
		)
	);
	vsizer->Add( log_text, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0 );
	wxStaticLine *line = new wxStaticLine( this, -1,
		wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	vsizer->Add( line, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	cmd_control = new wxTextCtrl(this, Event_Comand_ID, wxEmptyString,
			wxDefaultPosition, wxSize(APP_INIT_SIZE_X,-1), wxTE_PROCESS_ENTER);
	cmd_control->SetBackgroundColour(wxT("wheat"));
	vsizer->Add(cmd_control, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

	SetSizer(vsizer);
	vsizer->SetSizeHints(this);

	m_timer = new wxTimer(this, Timer_ID);
	m_timer->Start(5000);
}

void CamulewebFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	// true is to force the frame to close
	Show(_("\nOk, exiting Web Client...\n"));
	Close(true);
}

void CamulewebFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxString msg;
	#ifdef CVSDATE
		msg = wxString::Format( 
			_("amuleweb [DLG version] %s %s\n"
			"Using %s\n"
			"(c) aMule Dev Team"),
			wxT(VERSION), wxT(CVSDATE), wxVERSION_STRING);
	#else
		msg = wxString::Format( 
			_("amuleweb [DLG version] %s\n"
			"Using %s\n"
			"(c) aMule Dev Team"),
			wxT(VERSION), wxVERSION_STRING);
	#endif
	wxMessageBox(msg, _("About amuleweb"), wxOK | wxICON_INFORMATION, this);
}

void CamulewebFrame::OnCommandEnter(wxCommandEvent& WXUNUSED(event)){
	if (cmd_control->GetLineLength(0) == 0) {
		return; 
	}
	wxString buffer = cmd_control->GetLineText(0);
	if (theApp.Parse_Command(buffer)) {
		Close(true);
	}
	cmd_control->Clear();
}

void CamulewebFrame::OnIdle(wxIdleEvent &WXUNUSED(event))
{
	theApp.MainThreadIdleNow();
}

void CamulewebFrame::OnTimerEvent(wxTimerEvent &WXUNUSED(event))
{
	wxWakeUpIdle();
}

void CamulewebApp::LocalShow(const wxString &s)
{
	if (!frame) {
		return;
	}
	frame->log_text->AppendText(s);
}

int CamulewebApp::OnExit() {
	frame = NULL;
	if (webserver) {
		webserver->StopServer();
		delete webserver;
		webserver = NULL;
	}
	return 0;
}

#else

void CamulewebApp::Post_Shell() {
	webserver->StopServer();
	delete webserver;
	webserver = NULL;
}

#endif

bool CamulewebApp::OnInit() {
	if (CaMuleExternalConnector::OnInit()) {
#if wxUSE_GUI
		#ifdef CVSDATE
		frame = new CamulewebFrame(wxString::Format(_("amuleweb [DLG version] %s %s"), wxT(VERSION), wxT(CVSDATE)), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
		#else
		frame = new CamulewebFrame(wxString::Format(_("amuleweb [DLG version] %s"), wxT(VERSION)), wxPoint(50, 50), wxSize(APP_INIT_SIZE_X, APP_INIT_SIZE_Y));
		#endif
		frame->Show(true);
#endif
		return true;
	} else {
		return false;
	}
}

int CamulewebApp::OnRun() {
	ConnectAndRun(wxT("aMuleweb"), wxT(VERSION));
#if wxUSE_GUI
	return CaMuleExternalConnector::OnRun();
#else
	return 0;
#endif
}

bool CamulewebApp::CheckDirForTemplate(wxString& dir, const wxString& tmpl)
{
	DebugShow(wxT("checking for directory '") + dir + wxT("'..."));
	if (wxFileName::DirExists(dir)) {
		DebugShow(wxT(" yes\n"));
		dir += wxFileName::GetPathSeparator() + tmpl;
		DebugShow(wxT("checking for directory '") + dir + wxT("'..."));
		if (wxFileName::DirExists(dir)) {
			DebugShow(wxT(" yes\n"));

			wxString tmplPath(dir + wxFileName::GetPathSeparator() +
				(m_UsePhp ? wxT("login.html") : wxT("aMule.tmpl")) );

			DebugShow(wxT("checking for file '") + tmplPath + wxT("'..."));
			if (wxFileName::FileExists(tmplPath)) {
				DebugShow(wxT(" yes\n"));
				// dir is already set to the directory component of the template path
				return true;
			} else {
				DebugShow(wxT(" no\n"));
			}
		} else {
			DebugShow(wxT(" no\n"));
		}
	} else {
		DebugShow(wxT(" no\n"));
	}
	return false;
}

bool CamulewebApp::GetTemplateDir(const wxString& templateName, wxString& templateDir)
{
	wxString dir;

	DebugShow(wxT("looking for template: ") + templateName + wxT("\n"));

#ifdef __WXMAC__
	CFURLRef amuleBundleUrl;
	OSStatus status = LSFindApplicationForInfo(
		kLSUnknownCreator,
		// This magic string is the bundle identifier in aMule.app's Info.plist
		CFSTR("org.amule.aMule"),
		NULL,
		NULL,
		&amuleBundleUrl
		);
	if (status == noErr && amuleBundleUrl) {
		CFBundleRef amuleBundle = CFBundleCreate(NULL, amuleBundleUrl);
		CFRelease(amuleBundleUrl);
		
		if (amuleBundle) {
			CFURLRef webserverDirUrl = CFBundleCopyResourceURL(
				amuleBundle,
				CFSTR("webserver"),
				NULL,
				NULL
				);
			CFRelease(amuleBundle);
			if (webserverDirUrl) {
				CFURLRef absoluteURL = CFURLCopyAbsoluteURL(webserverDirUrl);
				CFRelease(webserverDirUrl);
				if (absoluteURL) {
					CFStringRef pathString = CFURLCopyFileSystemPath(absoluteURL, kCFURLPOSIXPathStyle);
					CFRelease(absoluteURL);
					dir = wxMacCFStringHolder(pathString).AsString(wxLocale::GetSystemEncoding());
					if (CheckDirForTemplate(dir, templateName)) {
						templateDir = dir;
						return true;
					}
				}
			}
		}
	}
#endif

	dir = GetConfigDir() + wxT("webserver");
	if (CheckDirForTemplate(dir, templateName)) {
		templateDir = dir;
		return true;
	}
	dir = wxT(WEBSERVERDIR);
	if (CheckDirForTemplate(dir, templateName)) {
		templateDir = dir;
		return true;
	}
	
	// template not found. reverting to default
	const wxChar* const defaultTemplateName =
		m_UsePhp ? wxT("php-default") : wxT("default");
	if ( templateName == defaultTemplateName ) {
		return false;
	}
	Show(wxT("Template ") + templateName + wxT(" not found, reverting to default\n\n"));
	return GetTemplateDir(defaultTemplateName, templateDir);
}

void CamulewebApp::OnInitCmdLine(wxCmdLineParser& amuleweb_parser)
{
	CaMuleExternalConnector::OnInitCmdLine(amuleweb_parser);
	amuleweb_parser.AddOption(wxT("t"), wxT("template"), 
		_("Loads template <str>"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
		
	amuleweb_parser.AddOption(wxT("s"), wxT("server-port"), 
		_("Webserver HTTP port"),
		wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("z"), wxT("enable-gzip"), 
		_("Use gzip compression"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddSwitch(wxT("Z"), wxT("disable-gzip"), 
		wxT("Do not use gzip compression"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddOption(wxT("A"), wxT("admin-pass"), 
		_("Full access password for webserver"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddOption(wxT("G"), wxT("guest-pass"), 
		_("Guest password for webserver"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("a"), wxT("allow-guest"), 
		_("Allow guest access"),
		wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("d"), wxT("deny-guest"), 
		_("Deny guest access"),
		wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("L"), wxT("load-settings"), 
		_("Load/save webserver settings from/to remote aMule"),
		wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddOption(wxEmptyString, wxT("amule-config-file"),
		_("aMule config file path. DO NOT USE DIRECTLY!"),
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	/*
	 * In this mode, internal PHP interpreter is activated, and
	 * amuleweb will forward there requests for .php pages
	 */
	amuleweb_parser.AddSwitch(wxEmptyString, wxT("no-php"), 
		_("Disable PHP interpreter (deprecated)"),
		wxCMD_LINE_PARAM_OPTIONAL);

	/*
	 * Reload .php page each time it's requested - don't cache
	 * compilation results. Used for script development.
	 */
	amuleweb_parser.AddSwitch(wxT("N"), wxT("no-script-cache"), 
		_("Recompile PHP pages on each request"),
		wxCMD_LINE_PARAM_OPTIONAL);

}

bool CamulewebApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	wxString aMuleConfigFile;
	if (parser.Found(wxT("amule-config-file"), &aMuleConfigFile)) {
		aMuleConfigFile = FinalizeFilename(aMuleConfigFile);
		if (!::wxFileExists(aMuleConfigFile)) {
			fprintf(stderr, (const char *)unicode2char(wxT("FATAL ERROR: ") + aMuleConfigFile + wxT(" does not exist.\n")));
			return false;
		}
		CECFileConfig cfg(aMuleConfigFile);
		LoadAmuleConfig(cfg);
		// do not process any other command-line parameters, use defaults instead
		m_UsePhp = true; // This is used by GetTemplateDir, so set it first
		if (!(m_TemplateOk = GetTemplateDir(m_TemplateName, m_TemplateDir))) {
			// no reason to run webserver without a template
			fprintf(stderr, (const char *)unicode2char(wxT("FATAL ERROR: Cannot find template: ") + m_TemplateName + wxT("\n")));
			return true;
		}
		m_TemplateFileName = m_TemplateDir + wxFileName::GetPathSeparator() + wxT("aMule.tmpl");
		m_Verbose = false;
		m_KeepQuiet = true;
		m_LoadSettingsFromAmule = true;
		return true;
	}

	if (CaMuleExternalConnector::OnCmdLineParsed(parser)) {

		m_UsePhp = !parser.Found(wxT("no-php"));

		parser.Found(wxT("template"), &m_TemplateName);
		if (m_TemplateName.IsEmpty()) {
			if ( m_UsePhp ) {
				m_TemplateName = wxT("php-default");
			} else {
				m_TemplateName = wxT("default");
			}
		}
		if (!(m_TemplateOk = GetTemplateDir(m_TemplateName, m_TemplateDir))) {
			// no reason to run webserver without a template
			fprintf(stderr, (const char *)unicode2char(wxT("FATAL ERROR: Cannot find template: ") + m_TemplateName + wxT("\n")));
			return true;
		}
		m_TemplateFileName = m_TemplateDir + wxFileName::GetPathSeparator() + wxT("aMule.tmpl");
		DebugShow(wxT("*** Using template: ") + m_TemplateFileName + wxT("\n"));

		long port;
		if (parser.Found(wxT("server-port"), &port)) {
			m_WebserverPort = port;
		}
		if (parser.Found(wxT("enable-gzip"))) {
			m_UseGzip = true;
		}
		if (parser.Found(wxT("disable-gzip"))) {
			m_UseGzip = false;
		}

		if (parser.Found(wxT("allow-guest"))) {
			m_AllowGuest = true;
		}
		if (parser.Found(wxT("deny-guest"))) {
			m_AllowGuest = false;
		}

		wxString tmp;
		if ( parser.Found(wxT("admin-pass"), &tmp) ) {
			if (tmp.IsEmpty()) {
				m_AdminPass.Clear();
			} else {
				m_AdminPass.Decode(MD5Sum(tmp).GetHash());
			}
		}
		if ( parser.Found(wxT("guest-pass"), &tmp) ) {
			if (tmp.IsEmpty()) {
				m_GuestPass.Clear();
			} else {
				m_GuestPass.Decode(MD5Sum(tmp).GetHash());
			}
		}

		m_LoadSettingsFromAmule = parser.Found(wxT("load-settings"));
		return true;
	} else {
		return false;
	}
}

const wxString CamulewebApp::GetGreetingTitle()
{
	return _("aMule Web Server");
}

void CamulewebApp::Pre_Shell() {
	//Creating the web server
	if ( m_TemplateOk ) {
		if ( m_UsePhp ) {
			webserver = new CScriptWebServer(this, m_TemplateDir);
		} else {
			webserver = new CWebServer(this, m_TemplateDir);
		}
	} else {
		webserver = new CNoTemplateWebServer(this);
	}
	webserver->StartServer();
}

void CamulewebApp::LoadAmuleConfig(CECFileConfig& cfg)
{
	CaMuleExternalConnector::LoadAmuleConfig(cfg);
	m_UseGzip = (cfg.Read(wxT("/Webserver/UseGzip"), 0l) == 1l);
	m_AllowGuest = (cfg.Read(wxT("/Webserver/UseLowRightsUser"), 0l) == 1l);
	cfg.ReadHash(wxT("/Webserver/Password"), &m_AdminPass);
	cfg.ReadHash(wxT("/Webserver/PasswordLow"), &m_GuestPass);
	m_WebserverPort = cfg.Read(wxT("/Webserver/Port"), -1l);
	m_PageRefresh = cfg.Read(wxT("/Webserver/PageRefreshTime"), 120l);
	m_TemplateName = wxT("default");
}

void CamulewebApp::LoadConfigFile()
{
	CaMuleExternalConnector::LoadConfigFile();
	if (m_configFile) {
		wxString tmp;
		m_WebserverPort = m_configFile->Read(wxT("/Webserver/Port"), -1l);
		m_TemplateName = m_configFile->Read(wxT("/Webserver/Template"), wxEmptyString);
		m_configFile->Read(wxT("/Webserver/UseGzip"), &m_UseGzip, false);
		m_configFile->Read(wxT("/Webserver/AllowGuest"), &m_AllowGuest, false);
		m_configFile->ReadHash(wxT("/Webserver/AdminPassword"), &m_AdminPass);
		m_configFile->ReadHash(wxT("/Webserver/GuestPassword"), &m_GuestPass);
	}
}

void CamulewebApp::SaveConfigFile()
{
	CaMuleExternalConnector::SaveConfigFile();
	if (m_configFile) {
		m_configFile->Write(wxT("/Webserver/Port"), m_WebserverPort);
		m_configFile->Write(wxT("/Webserver/Template"), m_TemplateName);
		m_configFile->Write(wxT("/Webserver/UseGzip"), m_UseGzip);
		m_configFile->Write(wxT("/Webserver/AllowGuest"), m_AllowGuest);
		m_configFile->WriteHash(wxT("/Webserver/AdminPassword"), m_AdminPass);
		m_configFile->WriteHash(wxT("/Webserver/GuestPassword"), m_GuestPass);
	}
}
