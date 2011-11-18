//
// This file is part of the aMule Project.
//  
// Copyright (c) 2004-2011 shakraw ( shakraw@users.sourceforge.net )
// Copyright (c) 2003-2011 Angel Vidal ( kry@amule.org )
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


#ifdef HAVE_CONFIG_H
	#include "config.h"	// For VERSION and ENABLE_NLS
#endif


#include <wx/stdpaths.h>


#ifdef __WXMAC__
	#include <CoreFoundation/CFBundle.h> // Do_not_auto_remove
	#include <ApplicationServices/ApplicationServices.h> // Do_not_auto_remove
	#if wxCHECK_VERSION(2, 9, 0)
		#include <wx/osx/core/cfstring.h>  // Do_not_auto_remove
	#else
		#include <wx/mac/corefoundation/cfstring.h>  // Do_not_auto_remove
	#endif
#endif


#include <ec/cpp/ECFileConfig.h>	// Needed for CECFileConfig
#include <common/MD5Sum.h>


#include "WebServer.h"


#include <wx/apptrait.h>
#include <wx/socket.h>


#ifdef ENABLE_NLS
#	include <libintl.h>
#endif

#ifdef AMULEWEB28
/*
 * Socket handling in wxBase (same as amuled)
 * 
 */
class CSocketSet
{
	int m_count;
	int m_fds[FD_SETSIZE], m_fd_idx[FD_SETSIZE];
	GSocket *m_gsocks[FD_SETSIZE];
	fd_set m_set;

public:
	CSocketSet();
	void AddSocket(GSocket *);
	void RemoveSocket(GSocket *);
	void FillSet(int &max_fd);
	void Detected(void (GSocket::*func)());
	fd_set *Set() { return &m_set; }
};


CSocketSet::CSocketSet()
{
	m_count = 0;
	for(int i = 0; i < FD_SETSIZE; i++) {
		m_fds[i] = 0;
		m_fd_idx[i] = 0xffff;
		m_gsocks[i] = 0;
	}
}


void CSocketSet::AddSocket(GSocket *socket)
{
	wxASSERT(socket);

	int fd = socket->m_fd;

	if (fd == -1) {
		return;
	}

	wxASSERT(fd >= 0 && fd < FD_SETSIZE);

	if (m_gsocks[fd]) {
		return;
	}
	m_fds[m_count] = fd;
	m_fd_idx[fd] = m_count;
	m_gsocks[fd] = socket;
	m_count++;
}


void CSocketSet::RemoveSocket(GSocket *socket)
{
	wxASSERT(socket);

	int fd = socket->m_fd;

	if (fd == -1) {
		return;
	}

	wxASSERT(fd >= 0 && fd < FD_SETSIZE);

	int i = m_fd_idx[fd];
	if (i == 0xffff) {
		return;
	}
	wxASSERT(m_fds[i] == fd);
	m_fds[i] = m_fds[m_count-1];
	m_gsocks[fd] = 0;
	m_fds[m_count-1] = 0;
	m_fd_idx[fd] = 0xffff;
	m_fd_idx[m_fds[i]] = i;
	m_count--;
}


void CSocketSet::FillSet(int &max_fd)
{
	FD_ZERO(&m_set);
	for(int i = 0; i < m_count; i++) {
		FD_SET(m_fds[i], &m_set);
		if (m_fds[i] > max_fd) {
			max_fd = m_fds[i];
		}
	}
}


void CSocketSet::Detected(void (GSocket::*func)())
{
	for (int i = 0; i < m_count; i++) {
		int fd = m_fds[i];
		if (FD_ISSET(fd, &m_set)) {
			GSocket *socket = m_gsocks[fd];
			(*socket.*func)();
		}
	}
}


class CWebserverGSocketFuncTable : public GSocketGUIFunctionsTable
{
private:
	CSocketSet *m_in_set;
	CSocketSet *m_out_set;
	wxMutex m_lock;

public:
	CWebserverGSocketFuncTable();

	void AddSocket(GSocket *socket, GSocketEvent event);
	void RemoveSocket(GSocket *socket, GSocketEvent event);
	void RunSelect();

	virtual bool OnInit();
	virtual void OnExit();
	virtual bool CanUseEventLoop();
	virtual bool Init_Socket(GSocket *socket);
	virtual void Destroy_Socket(GSocket *socket);
	virtual void Install_Callback(GSocket *socket, GSocketEvent event);
	virtual void Uninstall_Callback(GSocket *socket, GSocketEvent event);
	virtual void Enable_Events(GSocket *socket);
	virtual void Disable_Events(GSocket *socket);
};


CWebserverGSocketFuncTable::CWebserverGSocketFuncTable() : m_lock(wxMUTEX_RECURSIVE)
{
	m_in_set = new CSocketSet;
	m_out_set = new CSocketSet;
	m_lock.Unlock();
}

void CWebserverGSocketFuncTable::AddSocket(GSocket *socket, GSocketEvent event)
{
	wxMutexLocker lock(m_lock);
	if (event == GSOCK_INPUT) {
		m_in_set->AddSocket(socket);
	} else {
		m_out_set->AddSocket(socket);
	}
}

void CWebserverGSocketFuncTable::RemoveSocket(GSocket *socket, GSocketEvent event)
{
	wxMutexLocker lock(m_lock);
	if (event == GSOCK_INPUT) {
		m_in_set->RemoveSocket(socket);
	} else {
		m_out_set->RemoveSocket(socket);
	}
}

void CWebserverGSocketFuncTable::RunSelect()
{
	wxMutexLocker lock(m_lock);

	int max_fd = -1;
	m_in_set->FillSet(max_fd);
	m_out_set->FillSet(max_fd);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10000; // 10ms

	int result = select(max_fd + 1, m_in_set->Set(), m_out_set->Set(), 0, &tv);
	if (result > 0) {
		m_in_set->Detected(&GSocket::Detected_Read);
		m_out_set->Detected(&GSocket::Detected_Write);
	}
}


bool CWebserverGSocketFuncTable::OnInit()
{
	return true;
}


void CWebserverGSocketFuncTable::OnExit()
{
}


bool CWebserverGSocketFuncTable::CanUseEventLoop()
{
	return false;
}


bool CWebserverGSocketFuncTable::Init_Socket(GSocket *)
{
	return true;
}


void CWebserverGSocketFuncTable::Destroy_Socket(GSocket *)
{
}


void CWebserverGSocketFuncTable::Install_Callback(GSocket *sock, GSocketEvent e)
{
	AddSocket(sock, e);
}


void CWebserverGSocketFuncTable::Uninstall_Callback(GSocket *sock, GSocketEvent e)
{
	RemoveSocket(sock, e);
}


void CWebserverGSocketFuncTable::Enable_Events(GSocket *socket)
{
	Install_Callback(socket, GSOCK_INPUT);
	Install_Callback(socket, GSOCK_OUTPUT);
}


void CWebserverGSocketFuncTable::Disable_Events(GSocket *socket)
{
	Uninstall_Callback(socket, GSOCK_INPUT);
	Uninstall_Callback(socket, GSOCK_OUTPUT);
}


class CWebserverAppTraits : public wxConsoleAppTraits
{
private:
	CWebserverGSocketFuncTable *m_table;
	wxMutex m_lock;
	std::list<wxObject *> m_sched_delete;

public:
	CWebserverAppTraits(CWebserverGSocketFuncTable *table);
	virtual GSocketGUIFunctionsTable* GetSocketGUIFunctionsTable();
	virtual void ScheduleForDestroy(wxObject *object);
	virtual void RemoveFromPendingDelete(wxObject *object);
	void DeletePending();
};


CWebserverAppTraits::CWebserverAppTraits(CWebserverGSocketFuncTable *table)
:
wxConsoleAppTraits(),
m_table(table),m_lock(wxMUTEX_RECURSIVE),
m_sched_delete()
{
	m_lock.Unlock();
}


GSocketGUIFunctionsTable *CWebserverAppTraits::GetSocketGUIFunctionsTable()
{
	return m_table;
}


void CWebserverAppTraits::ScheduleForDestroy(wxObject *object)
{
	wxMutexLocker lock(m_lock);
	m_sched_delete.push_back(object);
}


void CWebserverAppTraits::RemoveFromPendingDelete(wxObject *object)
{
	wxMutexLocker lock(m_lock);

	for(std::list<wxObject *>::iterator i = m_sched_delete.begin();
	    i != m_sched_delete.end(); i++) {
		if (*i == object) {
			m_sched_delete.erase(i);
			return;
		}
	}
}


void CWebserverAppTraits::DeletePending()
{
	wxMutexLocker lock(m_lock);

	while (!m_sched_delete.empty()) {
		std::list<wxObject *>::iterator i = m_sched_delete.begin();
		wxObject *object = *i;
		delete object;
	}
}


CamulewebApp::CamulewebApp() : m_table(new CWebserverGSocketFuncTable)
{
	wxPendingEventsLocker = new wxCriticalSection;
}


wxAppTraits *CamulewebApp::CreateTraits()
{
	return new CWebserverAppTraits(m_table);
}

#endif

//-------------------------------------------------------------------
IMPLEMENT_APP(CamulewebApp)
//-------------------------------------------------------------------


BEGIN_EVENT_TABLE(CamulewebApp, CaMuleExternalConnector)
END_EVENT_TABLE()

void CamulewebApp::Post_Shell()
{
	m_webserver->StopServer();
	delete m_webserver;
	m_webserver = 0;
}


bool CamulewebApp::OnInit()
{
	return CaMuleExternalConnector::OnInit();
}

#ifndef VERSION
#include <common/ClientVersion.h>
#endif

int CamulewebApp::OnRun()
{
	ConnectAndRun(wxT("aMuleweb"), wxT(VERSION));
	return 0;
}


bool CamulewebApp::CheckDirForTemplate(wxString& dir, const wxString& tmpl)
{
	DebugShow(wxT("checking for directory '") + dir + wxT("'..."));
	if (wxFileName::DirExists(dir)) {
		DebugShow(wxT(" yes\n"));
		dir = JoinPaths(dir, tmpl);
		DebugShow(wxT("checking for directory '") + dir + wxT("'..."));
		if (wxFileName::DirExists(dir)) {
			DebugShow(wxT(" yes\n"));
			wxString tmplPath = JoinPaths(dir, wxT("login.php"));
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
	m_localTemplate = false;

	DebugShow(wxT("looking for template: ") + templateName + wxT("\n"));

#ifdef __WXMAC__
	CFURLRef amuleBundleUrl;
	OSStatus status = LSFindApplicationForInfo(
		kLSUnknownCreator,
		// This magic string is the bundle identifier in aMule.app's Info.plist
		CFSTR("org.amule.aMule"),
		NULL, NULL, &amuleBundleUrl);
	if (status == noErr && amuleBundleUrl) {
		CFBundleRef amuleBundle = CFBundleCreate(NULL, amuleBundleUrl);
		CFRelease(amuleBundleUrl);
		if (amuleBundle) {
			CFURLRef webserverDirUrl = CFBundleCopyResourceURL(
				amuleBundle,
				CFSTR("webserver"),
				NULL, NULL);
			CFRelease(amuleBundle);
			if (webserverDirUrl) {
				CFURLRef absoluteURL =
					CFURLCopyAbsoluteURL(webserverDirUrl);
				CFRelease(webserverDirUrl);
				if (absoluteURL) {
					CFStringRef pathString =
						CFURLCopyFileSystemPath(
							absoluteURL,
							kCFURLPOSIXPathStyle);
					CFRelease(absoluteURL);
					#if wxCHECK_VERSION(2, 9, 0)
						dir = wxCFStringRef(pathString).
						AsString(wxLocale::GetSystemEncoding());
					#else
						dir = wxMacCFStringHolder(pathString).
						AsString(wxLocale::GetSystemEncoding());
					#endif
					if (CheckDirForTemplate(dir, templateName)) {
						templateDir = dir;
						return true;
					}
				}
			}
		}
	}
#endif

	dir = GetConfigDir(wxT("remote.conf")) + wxT("webserver");
	if (CheckDirForTemplate(dir, templateName)) {
		templateDir = dir;
		m_localTemplate = true;
		return true;
	}
#ifdef WEBSERVERDIR
	dir = wxT(WEBSERVERDIR);
	if (CheckDirForTemplate(dir, templateName)) {
		templateDir = dir;
		return true;
	}
#endif
	
	dir = wxStandardPaths::Get().GetResourcesDir();	// Returns 'aMule' when we use 'amule' elsewhere
#if !defined(__WXMSW__) && !defined(__WXMAC__)
	dir = dir.BeforeLast(wxFileName::GetPathSeparator());
	dir = JoinPaths(dir, wxT("amule"));
#endif
	dir = JoinPaths(dir, wxT("webserver"));
	if (CheckDirForTemplate(dir, templateName)) {
		templateDir = dir;
		return true;
	}
	
	// template not found. reverting to default
	const wxChar* const defaultTemplateName = wxT("default");

	if ( templateName == defaultTemplateName ) {
		return false;
	}
	Show(wxT("Template ") + templateName + wxT(" not found, reverting to default\n\n"));
	return GetTemplateDir(defaultTemplateName, templateDir);
}


void CamulewebApp::OnInitCmdLine(wxCmdLineParser& amuleweb_parser)
{
	CaMuleExternalConnector::OnInitCmdLine(amuleweb_parser, "amuleweb");
	amuleweb_parser.AddOption(wxT("t"), wxT("template"), 
		_("Loads template <str>"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
		
	amuleweb_parser.AddOption(wxT("s"), wxT("server-port"), 
		_("Web server HTTP port"),
		wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("u"), wxT("enable-upnp"), 
		_("Use UPnP port forwarding on web server port"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddSwitch(wxT("U"), wxT("upnp-port"), 
		_("UPnP port"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddSwitch(wxT("z"), wxT("enable-gzip"), 
		_("Use gzip compression"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddSwitch(wxT("Z"), wxT("disable-gzip"), 
		wxT("Do not use gzip compression"),
		wxCMD_LINE_PARAM_OPTIONAL);
	
	amuleweb_parser.AddOption(wxT("A"), wxT("admin-pass"), 
		_("Full access password for web server"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddOption(wxT("G"), wxT("guest-pass"), 
		_("Guest password for web server"), 
		wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("a"), wxT("allow-guest"), 
		_("Allow guest access"),
		wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("d"), wxT("deny-guest"), 
		_("Deny guest access"),
		wxCMD_LINE_PARAM_OPTIONAL);

	amuleweb_parser.AddSwitch(wxT("L"), wxT("load-settings"), 
		_("Load/save web server settings from/to remote aMule"),
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
			fprintf(stderr, "FATAL ERROR: file '%s' does not exist.\n",
				(const char*)unicode2char(aMuleConfigFile));
			return false;
		}
		CECFileConfig cfg(aMuleConfigFile);
		LoadAmuleConfig(cfg);
		// do not process any other command-line parameters, use defaults instead

		m_Verbose = false;
		m_KeepQuiet = true;
		m_LoadSettingsFromAmule = true;

		if (!(m_TemplateOk = GetTemplateDir(m_TemplateName, m_TemplateDir))) {
			// no reason to run webserver without a template
			fprintf(stderr, "FATAL ERROR: Cannot find template: %s\n",
				(const char *)unicode2char(m_TemplateName));
			return false;
		}
		return true;
	}

	if (CaMuleExternalConnector::OnCmdLineParsed(parser)) {
		if ( parser.Found(wxT("no-php")) ) {
			fprintf(stderr, "WARNING: --no-php switch have no effect. Long live PHP\n");
		}

		parser.Found(wxT("template"), &m_TemplateName);
		if (m_TemplateName.IsEmpty()) {
			m_TemplateName = wxT("default");
		}
		if (!(m_TemplateOk = GetTemplateDir(m_TemplateName, m_TemplateDir))) {
			// no reason to run webserver without a template
			fprintf(stderr, "FATAL ERROR: Cannot find template: %s\n",
				(const char *)unicode2char(m_TemplateName));
			return true;
		}

		long port;
		if (parser.Found(wxT("server-port"), &port)) {
			m_WebserverPort = port;
		}
		if (parser.Found(wxT("enable-upnp"))) {
			m_UPnPWebServerEnabled = true;
		}
		if (parser.Found(wxT("upnp-port"), &port)) {
			m_UPnPTCPPort = port;
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


void CamulewebApp::Pre_Shell()
{
	// Creating the web server
	if (m_TemplateOk) {
		m_webserver = new CScriptWebServer(this, m_TemplateDir);
	} else {
		m_webserver = new CNoTemplateWebServer(this);
	}
	m_webserver->StartServer();
}


void CamulewebApp::TextShell(const wxString &)
{
#ifdef AMULEWEB28
	while (true) {
		m_table->RunSelect();
		ProcessPendingEvents();
		((CWebserverAppTraits *)GetTraits())->DeletePending();
	}
#else

#ifndef AMULEWEB_DUMMY
	wxApp::OnRun();
#endif

#endif
}


void CamulewebApp::LoadAmuleConfig(CECFileConfig& cfg)
{
	CaMuleExternalConnector::LoadAmuleConfig(cfg);
	m_UseGzip = (cfg.Read(wxT("/WebServer/UseGzip"), 0l) == 1l);
	m_AllowGuest = (cfg.Read(wxT("/WebServer/UseLowRightsUser"), 0l) == 1l);
	cfg.ReadHash(wxT("/WebServer/Password"), &m_AdminPass);
	cfg.ReadHash(wxT("/WebServer/PasswordLow"), &m_GuestPass);
	m_WebserverPort = cfg.Read(wxT("/WebServer/Port"), 4711l);
	m_UPnPWebServerEnabled =
		(cfg.Read(wxT("/Webserver/UPnPWebServerEnabled"), 0l) == 1l);
	m_UPnPTCPPort = cfg.Read(wxT("/WebServer/UPnPTCPPort"), 50001l);
	m_PageRefresh = cfg.Read(wxT("/WebServer/PageRefreshTime"), 120l);
	m_TemplateName = cfg.Read(wxT("/WebServer/Template"), wxT("default"));
}


void CamulewebApp::LoadConfigFile()
{
	CaMuleExternalConnector::LoadConfigFile();
	if (m_configFile) {
		wxString tmp;
		m_WebserverPort = m_configFile->Read(wxT("/Webserver/Port"), 4711l);
		m_configFile->Read(wxT("/Webserver/UPnPWebServerEnabled"),
			&m_UPnPWebServerEnabled, false);
		m_UPnPTCPPort = m_configFile->Read(wxT("/WebServer/UPnPTCPPort"), 50001l);
		m_TemplateName = m_configFile->Read(wxT("/Webserver/Template"), wxEmptyString);
		m_configFile->Read(wxT("/Webserver/UseGzip"), &m_UseGzip, false);
		m_configFile->Read(wxT("/Webserver/AllowGuest"), &m_AllowGuest, false);
		m_configFile->ReadHash(wxT("/Webserver/AdminPassword"), &m_AdminPass);
		m_configFile->ReadHash(wxT("/Webserver/GuestPassword"), &m_GuestPass);
		m_PageRefresh = m_configFile->Read(wxT("/Webserver/PageRefreshTime"), 120l);
	}
}


void CamulewebApp::SaveConfigFile()
{
	CaMuleExternalConnector::SaveConfigFile();
	if (m_configFile) {
		m_configFile->Write(wxT("/Webserver/Port"), m_WebserverPort);
		m_configFile->Write(wxT("/Webserver/UPnPWebServerEnabled"),
			m_UPnPWebServerEnabled);
		m_configFile->Write(wxT("/WebServer/UPnPTCPPort"), m_UPnPTCPPort);
		m_configFile->Write(wxT("/Webserver/Template"), m_TemplateName);
		m_configFile->Write(wxT("/Webserver/UseGzip"), m_UseGzip);
		m_configFile->Write(wxT("/Webserver/AllowGuest"), m_AllowGuest);
		m_configFile->WriteHash(wxT("/Webserver/AdminPassword"), m_AdminPass);
		m_configFile->WriteHash(wxT("/Webserver/GuestPassword"), m_GuestPass);
	}
}


#ifdef ENABLE_NLS
static inline bool CheckDirForMessageCatalog(const wxString& dir, const wxString& lang, const wxString& domain)
{
	return wxFileName::FileExists(JoinPaths(dir, JoinPaths(lang, JoinPaths(wxT("LC_MESSAGES"), domain + wxT(".mo")))));
}


static inline bool DirHasMessageCatalog(const wxString& dir, const wxString& lang, const wxString& domain)
{
	if (!CheckDirForMessageCatalog(dir, lang, domain)) {
		wxString lingua = lang.BeforeFirst(wxT('.')).BeforeFirst(wxT('@'));
		if (lingua == lang || !CheckDirForMessageCatalog(dir, lingua, domain)) {
			wxString lng = lingua.BeforeFirst(wxT('_'));
			wxString ctry = lingua.AfterFirst(wxT('_'));
			if (ctry.IsEmpty()) {
				ctry = lng.Upper();
				return CheckDirForMessageCatalog(dir, lng + wxT("_") + ctry, domain);
			} else if (ctry == lng.Upper()) {
				return CheckDirForMessageCatalog(dir, lng, domain);
			} else {
				return false;
			}
		}
	}
	return true;
}
#endif


wxString CamulewebApp::SetLocale(const wxString& language)
{
	wxString lang = CaMuleExternalConnector::SetLocale(language); // will call setlocale() for us

	// SetLocale() may indeed return an empty string, when no locale has been selected yet and
	// no locale change was requested, or, in the worst case, if the last locale change didn't succeed.
	if (!lang.IsEmpty()) {
		DebugShow(wxT("*** Language set to: ") + lang + wxT(" ***\n"));
#ifdef ENABLE_NLS
		wxString domain = wxT("amuleweb-") + m_TemplateName;
		Unicode2CharBuf domainBuf = unicode2char(domain);
		const char *c_domain = (const char *)domainBuf;

		// Try to find a message catalog
		// First look in ~/.aMule/webserver/<template>, but only if a local template was used
		wxString dir;
		if (m_localTemplate) {
			dir = JoinPaths(JoinPaths(JoinPaths(GetConfigDir(), wxT("webserver")), m_TemplateName), wxT("locale"));
			DebugShow(wxT("looking for message catalogs in ") + dir + wxT("... "));
		}
		if (!m_localTemplate || !DirHasMessageCatalog(dir, lang, domain)) {
			if (m_localTemplate) {
				DebugShow(wxT("no\n"));
			}
#if defined __WXMAC__  || defined __WXMSW__
			// on Mac, the bundle may be tried, too
			dir = wxStandardPaths::Get().GetDataDir();
#elif defined(__UNIX__)
			dir = JoinPaths(static_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetInstallPrefix(), JoinPaths(wxT("share"), wxT("locale")));
#endif
			DebugShow(wxT("looking for message catalogs in ") + dir + wxT("... "));
			if (!DirHasMessageCatalog(dir, lang, domain)) {
				DebugShow(wxT("no\n"));
				dir = wxEmptyString;
			} else {
				DebugShow(wxT("yes\n"));
			}
		} else {
			DebugShow(wxT("yes\n"));
		}
		// If we found something, then use it otherwise it may still be present at the system default location
		if (!dir.IsEmpty()) {
			Unicode2CharBuf buffer = unicode2char(dir);
			const char *c_dir = (const char *)buffer;
			bindtextdomain(c_domain, c_dir);
		}
		// We need to have the returned messages in UTF-8
		bind_textdomain_codeset(c_domain, "UTF-8");
		// And finally select the message catalog
		textdomain(c_domain);
#endif /* ENABLE_NLS */
	}

	return lang;
}

// File_checked_for_headers
