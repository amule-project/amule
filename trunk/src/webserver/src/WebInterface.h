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

#include "ExternalConnector.h"

#include "MD4Hash.h"

class CamulewebApp : public CaMuleExternalConnector {
	class CWebServerBase *m_webserver;
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

	bool		m_TemplateOk;

public:
	void Post_Shell();
private:
	virtual bool	OnInit();
	virtual int 	OnRun();
};

#endif // WEBINTERFACE_H
