//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2008 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2004-2008 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H


#include "ExternalConnector.h"


class CamulewebApp
:
public CaMuleExternalConnector
{
	class CWebserverGSocketFuncTable *m_table;
	class CWebServerBase *m_webserver;

private:
	//Assert Stuff
	enum APPState {
		APP_STATE_RUNNING = 0,
		APP_STATE_STARTING
	};

public:
	CamulewebApp();
	~CamulewebApp() {}
	
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
	bool	m_UseGzip;
	CMD4Hash m_AdminPass, m_GuestPass;
	bool	m_AllowGuest;

	long	m_WebserverPort;
	bool	m_UPnPWebServerEnabled;
	int	m_UPnPTCPPort;
	unsigned int	m_PageRefresh;
	bool	m_LoadSettingsFromAmule;
	bool	m_TemplateOk;

public:
	virtual void Post_Shell();

	wxAppTraits *CreateTraits();

	void TextShell(const wxString &prompt);

	virtual wxString SetLocale(const wxString& language);

	DECLARE_EVENT_TABLE();

	// Assert Stuff
	bool IsRunning() const
	{
		return (m_app_state == APP_STATE_RUNNING);
	}

private:
	virtual bool OnInit();
	virtual int OnRun();
	void OnFatalException();
	void OnAssertFailure(
		const wxChar* file,
		int line, 
		const wxChar* func,
		const wxChar* cond,
		const wxChar* msg);

	bool m_localTemplate;
	wxString m_FullMuleVersion;
	wxString m_OsDescription;
	// Assert Stuff
	APPState m_app_state;
};

#endif // WEBINTERFACE_H
// File_checked_for_headers
