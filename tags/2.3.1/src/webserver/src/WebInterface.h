//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2011 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
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

#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include "ExternalConnector.h"


#if !wxCHECK_VERSION(2, 9, 0)
	#ifdef __WXMSW__
		// MSW: can't run amuled with 2.8 anyway, just get it compiled
		#define AMULEWEB_DUMMY
	#else
		#define AMULEWEB28
	#endif
#endif


class CamulewebApp
:
public CaMuleExternalConnector
{
	class CWebServerBase *m_webserver;

#ifdef AMULEWEB28
	class CWebserverGSocketFuncTable *m_table;
public:
	wxAppTraits *CreateTraits();
	CamulewebApp();
#endif

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
	bool	m_UseGzip;
	CMD4Hash m_AdminPass, m_GuestPass;
	bool	m_AllowGuest;

	long		m_WebserverPort;
	bool		m_UPnPWebServerEnabled;
	int		m_UPnPTCPPort;
	unsigned int	m_PageRefresh;
	bool		m_LoadSettingsFromAmule;

	bool		m_TemplateOk;

public:
	virtual void Post_Shell();

	void TextShell(const wxString &prompt);

	virtual wxString SetLocale(const wxString& language);

	DECLARE_EVENT_TABLE();
	
private:
	virtual bool	OnInit();
	virtual int 	OnRun();

	bool	m_localTemplate;
};

#endif // WEBINTERFACE_H
// File_checked_for_headers
