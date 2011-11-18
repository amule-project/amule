//
// This file is part of the aMule Project.
//
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

//
// This file is for functions common to all three apps (amule, amuled, amulegui),
// but preprocessor-dependent (using theApp, thePrefs), so it is compiled seperately for each app.
//


#include <wx/wx.h>
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/snglinst.h>		// Needed for wxSingleInstanceChecker
#include <wx/textfile.h>		// Needed for wxTextFile
#include <wx/config.h>			// Do_not_auto_remove (win32)
#include <wx/fileconf.h>

#include "amule.h"				// Interface declarations.
#include <common/Format.h>		// Needed for CFormat
#include "CFile.h"				// Needed for CFile
#include "ED2KLink.h"			// Needed for command line passing of links
#include "FileLock.h"			// Needed for CFileLock
#include "GuiEvents.h"			// Needed for Notify_*
#include "KnownFile.h"
#include "Logger.h"
#include "MagnetURI.h"			// Needed for CMagnetURI
#include "Preferences.h"
#include "ScopedPtr.h"

#ifndef CLIENT_GUI
#include "DownloadQueue.h"
#endif

CamuleAppCommon::CamuleAppCommon()
{
	m_singleInstance = NULL;
	ec_config = false;
	m_geometryEnabled = false;
	if (IsRemoteGui()) {
		m_appName		= wxT("aMuleGUI");
		m_configFile	= wxT("remote.conf");
		m_logFile		= wxT("remotelogfile");
	} else {
		m_configFile	= wxT("amule.conf");
		m_logFile		= wxT("logfile");

		if (IsDaemon()) {
			m_appName	= wxT("aMuleD");
		} else {
			m_appName	= wxT("aMule");
		}
	}
}

CamuleAppCommon::~CamuleAppCommon()
{
#if defined(__WXMAC__) && defined(AMULE_DAEMON)
	//#warning TODO: fix wxSingleInstanceChecker for amuled on Mac (wx link problems)
#else
	delete m_singleInstance;
#endif
}

void CamuleAppCommon::RefreshSingleInstanceChecker()
{
#if defined(__WXMAC__) && defined(AMULE_DAEMON)
	//#warning TODO: fix wxSingleInstanceChecker for amuled on Mac (wx link problems)
#else
	delete m_singleInstance;
	m_singleInstance = new wxSingleInstanceChecker(wxT("muleLock"), ConfigDir);
#endif
}

void CamuleAppCommon::AddLinksFromFile()
{
	const wxString fullPath = ConfigDir + wxT("ED2KLinks");
	if (!wxFile::Exists(fullPath)) {
		return;
	}
	
	// Attempt to lock the ED2KLinks file.
	CFileLock lock((const char*)unicode2char(fullPath));

	wxTextFile file(fullPath);
	if ( file.Open() ) {
		for ( unsigned int i = 0; i < file.GetLineCount(); i++ ) {
			wxString line = file.GetLine( i ).Strip( wxString::both );
			
			if ( !line.IsEmpty() ) {
				// Special case! used by a secondary running mule to raise this one.
				if (line == wxT("RAISE_DIALOG")) {
					Notify_ShowGUI();
					continue;
				}
				unsigned long category = 0;
				if (line.AfterLast(wxT(':')).ToULong(&category) == true) {
					line = line.BeforeLast(wxT(':'));
				} else { // If ToULong returns false the category still can have been changed!
						 // This is fixed in wx 2.9
					category = 0;
				}
				theApp->downloadqueue->AddLink(line, category);
			}
		}

		file.Close();
	} else {
		AddLogLineNS(_("Failed to open ED2KLinks file."));
	}
	
	// Delete the file.
	wxRemoveFile(theApp->ConfigDir + wxT("ED2KLinks"));
}


// Returns a magnet ed2k URI
wxString CamuleAppCommon::CreateMagnetLink(const CAbstractFile *f)
{
	CMagnetURI uri;

	uri.AddField(wxT("dn"), f->GetFileName().Cleanup(false).GetPrintable());
	uri.AddField(wxT("xt"), wxString(wxT("urn:ed2k:")) + f->GetFileHash().Encode().Lower());
	uri.AddField(wxT("xt"), wxString(wxT("urn:ed2khash:")) + f->GetFileHash().Encode().Lower());
	uri.AddField(wxT("xl"), CFormat(wxT("%d")) % f->GetFileSize());

	return uri.GetLink();
}

// Returns a ed2k file URL
wxString CamuleAppCommon::CreateED2kLink(const CAbstractFile *f, bool add_source, bool use_hostname, bool addcryptoptions)
{
	wxASSERT(!(!add_source && (use_hostname || addcryptoptions)));
	// Construct URL like this: ed2k://|file|<filename>|<size>|<hash>|/
	wxString strURL = CFormat(wxT("ed2k://|file|%s|%i|%s|/"))
		% f->GetFileName().Cleanup(false)
		% f->GetFileSize() % f->GetFileHash().Encode();
	
	if (add_source && theApp->IsConnected() && !theApp->IsFirewalled()) {
		// Create the first part of the URL
		strURL << wxT("|sources,");
		if (use_hostname) {
			strURL << thePrefs::GetYourHostname();
		} else {
			uint32 clientID = theApp->GetID();
			strURL = CFormat(wxT("%s%u.%u.%u.%u"))
				% strURL
				% (clientID & 0xff)
				% ((clientID >> 8) & 0xff)
				% ((clientID >> 16) & 0xff)
				% ((clientID >> 24) & 0xff);
		}
		
 		strURL << wxT(":") <<
			thePrefs::GetPort();
		
		if (addcryptoptions) {
			uint8 uSupportsCryptLayer = thePrefs::IsClientCryptLayerSupported() ? 1 : 0;
			uint8 uRequestsCryptLayer = thePrefs::IsClientCryptLayerRequested() ? 1 : 0;
			uint8 uRequiresCryptLayer = thePrefs::IsClientCryptLayerRequired() ? 1 : 0;
			uint16 byCryptOptions = (uRequiresCryptLayer << 2) | (uRequestsCryptLayer << 1) | (uSupportsCryptLayer << 0) | (uSupportsCryptLayer ? 0x80 : 0x00);
			
			strURL << wxT(":") << byCryptOptions;
			
			if (byCryptOptions & 0x80) {
				strURL << wxT(":") << thePrefs::GetUserHash().Encode();
			}
		}
		strURL << wxT("|/");
	} else if (add_source) {
		AddLogLineC(_("WARNING: You can't add yourself as a source for an eD2k link while having a lowid."));
	}

	// Result is "ed2k://|file|<filename>|<size>|<hash>|/|sources,[(<ip>|<hostname>):<port>[:cryptoptions[:hash]]]|/"
	return strURL;
}

// Returns a ed2k link with AICH info if available
wxString CamuleAppCommon::CreateED2kAICHLink(const CKnownFile* f)
{
	// Create the first part of the URL
	wxString strURL = CreateED2kLink(f);
	// Append the AICH info
	if (f->HasProperAICHHashSet()) {
		strURL.RemoveLast();		// remove trailing '/'
		strURL << wxT("h=") << f->GetAICHMasterHash() << wxT("|/");
	}	

	// Result is "ed2k://|file|<filename>|<size>|<hash>|h=<AICH master hash>|/"
	return strURL;
}

bool CamuleAppCommon::InitCommon(int argc, wxChar ** argv)
{
	theApp->SetAppName(wxT("aMule"));
	wxString FullMuleVersion = GetFullMuleVersion();
	wxString OSDescription = wxGetOsDescription();
	strFullMuleVersion = strdup((const char *)unicode2char(FullMuleVersion));
	strOSDescription = strdup((const char *)unicode2char(OSDescription));
	OSType = OSDescription.BeforeFirst( wxT(' ') );
	if ( OSType.IsEmpty() ) {
		OSType = wxT("Unknown");
	}

	// Parse cmdline arguments.
	wxCmdLineParser cmdline(argc, argv);

	// Handle these arguments.
	cmdline.AddSwitch(wxT("v"), wxT("version"), wxT("Displays the current version number."));
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("Displays this information."));
	cmdline.AddOption(wxT("c"), wxT("config-dir"), wxT("read config from <dir> instead of home"));
#ifdef AMULE_DAEMON
	cmdline.AddSwitch(wxT("f"), wxT("full-daemon"), wxT("Fork to background."));
	cmdline.AddOption(wxT("p"), wxT("pid-file"), wxT("After fork, create a pid-file in the given fullname file."));
	cmdline.AddSwitch(wxT("e"), wxT("ec-config"), wxT("Configure EC (External Connections)."));
#else

#ifdef __WXMSW__
	// MSW shows help otions in a dialog box, and the formatting doesn't fit there
#define HELPTAB wxT("\t")
#else
#define HELPTAB wxT("\t\t\t")
#endif

	cmdline.AddOption(wxT("geometry"), wxEmptyString,
			wxT("Sets the geometry of the app.\n")
			HELPTAB wxT("<str> uses the same format as standard X11 apps:\n")
			HELPTAB wxT("[=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]"));
#endif // !AMULE_DAEMON

	cmdline.AddSwitch(wxT("o"), wxT("log-stdout"), wxT("Print log messages to stdout."));
	cmdline.AddSwitch(wxT("r"), wxT("reset-config"), wxT("Resets config to default values."));

#ifdef CLIENT_GUI
	cmdline.AddSwitch(wxT("s"), wxT("skip"), wxT("Skip connection dialog."));
#else
	// Change webserver path. This is also a config option, so this switch will go at some time.
	cmdline.AddOption(wxT("w"), wxT("use-amuleweb"), wxT("Specify location of amuleweb binary."));
#endif
#ifndef __WXMSW__
	cmdline.AddSwitch(wxT("d"), wxT("disable-fatal"), wxT("Do not handle fatal exception."));
// Keep stdin open to run valgrind --gen_suppressions
	cmdline.AddSwitch(wxT("i"), wxT("enable-stdin"), wxT("Do not disable stdin."));
#endif

	// Allow passing of links to the app
	cmdline.AddOption(wxT("t"), wxT("category"), wxT("Set category for passed ED2K links."), wxCMD_LINE_VAL_NUMBER);
	cmdline.AddParam(wxT("ED2K link"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);

	// wx asserts in debug mode if there is a check for an option that wasn't added.
	// So we have to wrap around the same #ifdefs as above. >:(

	// Show help on --help or invalid commands
	if ( cmdline.Parse() ) {
		return false;		
	} else if (cmdline.Found(wxT("help"))) {
		cmdline.Usage();
		return false;
	}	

	if ( cmdline.Found(wxT("version"))) {
		// This looks silly with logging macros that add a timestamp.
		printf("%s\n", (const char*)unicode2char(wxString(CFormat(wxT("%s (OS: %s)")) % FullMuleVersion % OSType)));
		return false;
	}
	
	if ( cmdline.Found(wxT("config-dir"), &ConfigDir) ) {
		// Make an absolute path from the config dir
		wxFileName fn(ConfigDir);
		fn.MakeAbsolute();
		ConfigDir = fn.GetFullPath();
		if (ConfigDir.Last() != wxFileName::GetPathSeparator()) {
			ConfigDir += wxFileName::GetPathSeparator();
		}
	} else {
		ConfigDir = GetConfigDir();
	}

#ifndef __WXMSW__
	#if wxUSE_ON_FATAL_EXCEPTION
		if ( !cmdline.Found(wxT("disable-fatal")) ) {
			// catch fatal exceptions
			wxHandleFatalExceptions(true);
		}
	#endif
#endif

	theLogger.SetEnabledStdoutLog(cmdline.Found(wxT("log-stdout")));
#ifdef AMULE_DAEMON		
	enable_daemon_fork = cmdline.Found(wxT("full-daemon"));
	if ( cmdline.Found(wxT("pid-file"), &m_PidFile) ) {
		// Remove any existing PidFile
		if ( wxFileExists (m_PidFile) ) wxRemoveFile (m_PidFile);
	}
	ec_config = cmdline.Found(wxT("ec-config"));
#else
	enable_daemon_fork = false;

	// Default geometry of the GUI. Can be changed with a cmdline argument...
	if ( cmdline.Found(wxT("geometry"), &m_geometryString) ) {
		m_geometryEnabled = true;
	}
#endif

	if (theLogger.IsEnabledStdoutLog()) {
		if ( enable_daemon_fork ) {
			AddLogLineNS(wxT("Daemon will fork to background - log to stdout disabled"));	// localization not active yet
			theLogger.SetEnabledStdoutLog(false);
		} else {
			AddLogLineNS(wxT("Logging to stdout enabled"));
		}
	}
	
	AddLogLineNS(wxT("Initialising ") + FullMuleVersion);

	// Ensure that "~/.aMule/" is accessible.
	CPath outDir;
	if (!CheckMuleDirectory(wxT("configuration"), CPath(ConfigDir), wxEmptyString, outDir)) {
		return false;
	}
	
	if (cmdline.Found(wxT("reset-config"))) {
		// Make a backup first.
		wxRemoveFile(ConfigDir + m_configFile + wxT(".backup"));
		wxRenameFile(ConfigDir + m_configFile, ConfigDir + m_configFile + wxT(".backup"));
		AddLogLineNS(CFormat(wxT("Your settings have been reset to default values.\nThe old config file has been saved as %s.backup\n")) % m_configFile);
	}

	size_t linksPassed = cmdline.GetParamCount();	// number of links from the command line
	int linksActuallyPassed = 0;					// number of links that pass the syntax check
	if (linksPassed) {
		long cat = 0;
		if (!cmdline.Found(wxT("t"), &cat)) {
			cat = 0;
		}

		wxTextFile ed2kFile(ConfigDir + wxT("ED2KLinks"));
		if (!ed2kFile.Exists()) {
			ed2kFile.Create();
		}
		if (ed2kFile.Open()) {
			for (size_t i = 0; i < linksPassed; i++) {
				wxString link;
				if (CheckPassedLink(cmdline.GetParam(i), link, cat)) {
					ed2kFile.AddLine(link);
					linksActuallyPassed++;
				}
			}
			ed2kFile.Write();
		} else {
			AddLogLineCS(wxT("Failed to open 'ED2KLinks', cannot add links."));
		}
	}
	
#if defined(__WXMAC__) && defined(AMULE_DAEMON)
	//#warning TODO: fix wxSingleInstanceChecker for amuled on Mac (wx link problems)
	AddLogLineCS(wxT("WARNING: The check for other instances is currently disabled in amuled.\n"
		"Please make sure that no other instance of aMule is running or your files might be corrupted.\n"));
#else
	AddLogLineNS(wxT("Checking if there is an instance already running..."));

	m_singleInstance = new wxSingleInstanceChecker();
	wxString lockfile = IsRemoteGui() ? wxT("muleLockRGUI") : wxT("muleLock");
	if (m_singleInstance->Create(lockfile, ConfigDir)
		&& m_singleInstance->IsAnotherRunning()) {
		AddLogLineCS(CFormat(wxT("There is an instance of %s already running")) % m_appName);
		AddLogLineNS(CFormat(wxT("(lock file: %s%s)")) % ConfigDir % lockfile);
		if (linksPassed) {
			AddLogLineNS(CFormat(wxT("passed %d %s to it, finished")) % linksActuallyPassed 
				% (linksPassed == 1 ? wxT("link") : wxT("links")));
			return false;
		}
		
		// This is very tricky. The most secure way to communicate is via ED2K links file
		wxTextFile ed2kFile(ConfigDir + wxT("ED2KLinks"));
		if (!ed2kFile.Exists()) {
			ed2kFile.Create();
		}
			
		if (ed2kFile.Open()) {
			ed2kFile.AddLine(wxT("RAISE_DIALOG"));
			ed2kFile.Write();
			
			AddLogLineNS(wxT("Raising current running instance."));
		} else {
			AddLogLineCS(wxT("Failed to open 'ED2KFile', cannot signal running instance."));
		}
			
		return false;
	} else {
		AddLogLineNS(wxT("No other instances are running."));
	}
#endif

#ifndef __WXMSW__
	// Close standard-input
	if ( !cmdline.Found(wxT("enable-stdin")) ) 	{
		// The full daemon will close all std file-descriptors by itself,
		// so closing it here would lead to the closing on the first open
		// file, which is the logfile opened below
		if (!enable_daemon_fork) {
			close(0);
		}
	}
#endif

	// Create the CFG file we shall use and set the config object as the global cfg file
	wxConfig::Set(new wxFileConfig( wxEmptyString, wxEmptyString, ConfigDir + m_configFile));
	
	// Make a backup of the log file
	CPath logfileName = CPath(ConfigDir + m_logFile);
	if (logfileName.FileExists()) {
		CPath::BackupFile(logfileName, wxT(".bak"));
	}

	// Open the log file
	if (!theLogger.OpenLogfile(logfileName.GetRaw())) {
		// use std err as last resolt to indicate problem
		fputs("ERROR: unable to open log file\n", stderr);
		// failure to open log is serious problem
		return false;
	}

	// Load Preferences
	CPreferences::BuildItemList(ConfigDir);
	CPreferences::LoadAllItems( wxConfigBase::Get() );

#ifdef CLIENT_GUI
	m_skipConnectionDialog = cmdline.Found(wxT("skip"));
#else
	wxString amulewebPath;
	if (cmdline.Found(wxT("use-amuleweb"), &amulewebPath)) {
		thePrefs::SetWSPath(amulewebPath);
		AddLogLineNS(CFormat(wxT("Using amuleweb in '%s'.")) % amulewebPath);
	}
#endif

	return true;
}

/**
 * Returns a description of the version of aMule being used.
 *
 * @return A detailed description of the aMule version, including application
 *         name and wx information.
 */
const wxString CamuleAppCommon::GetFullMuleVersion() const
{
	return GetMuleAppName() + wxT(" ") + GetMuleVersion();
}

bool CamuleAppCommon::CheckPassedLink(const wxString &in, wxString &out, int cat)
{
	wxString link(in);

	// restore ASCII-encoded pipes
	link.Replace(wxT("%7C"), wxT("|"));
	link.Replace(wxT("%7c"), wxT("|"));

	if (link.compare(0, 7, wxT("magnet:")) == 0) {
		link = CMagnetED2KConverter(link);
		if (link.empty()) {
			AddLogLineCS(CFormat(wxT("Cannot convert magnet link to eD2k: %s")) % in);
			return false;
		}
	}

	try {
		CScopedPtr<CED2KLink> uri(CED2KLink::CreateLinkFromUrl(link));
		out = uri.get()->GetLink();
		if (cat && uri.get()->GetKind() == CED2KLink::kFile) {
			out += CFormat(wxT(":%d")) % cat;
		}
		return true;
	} catch ( const wxString& err ) {
		AddLogLineCS(CFormat(wxT("Invalid eD2k link \"%s\" - ERROR: %s")) % link % err);
	}
	return false;
}


/**
 * Checks permissions on a aMule directory, creating if needed.
 *
 * @param desc A description of the directory in question, used for error messages.
 * @param directory The directory in question.
 * @param alternative If the dir specified with 'directory' could not be created, try this instead.
 * @param outDir Returns the used path.
 * @return False on error.
 */
bool CamuleAppCommon::CheckMuleDirectory(const wxString& desc, const CPath& directory, const wxString& alternative, CPath& outDir)
{
	wxString msg;

	if (directory.IsDir(CPath::readwritable)) {
		outDir = directory;
		return true;
	} else if (directory.DirExists()) {
		// Strings are not translated here because translation isn't up yet.
		msg = CFormat(wxT("Permissions on the %s directory too strict!\n")
			wxT("aMule cannot proceed. To fix this, you must set read/write/exec\n")
			wxT("permissions for the folder '%s'"))
				% desc % directory;
	} else if (CPath::MakeDir(directory)) {
		outDir = directory;
		return true;
	} else {
		msg << CFormat(wxT("Could not create the %s directory at '%s'."))
			% desc % directory;
	}

	// Attempt to use fallback directory.
	const CPath fallback(alternative);
	if (fallback.IsOk() && (directory != fallback)) {
		msg << wxT("\nAttempting to use default directory at location \n'")
			<< alternative << wxT("'.");
		if (theApp->ShowAlert(msg, wxT("Error accessing directory."), wxICON_ERROR | wxOK | wxCANCEL) == wxCANCEL) {
			outDir = CPath(wxEmptyString);
			return false;
		}

		return CheckMuleDirectory(desc, fallback, wxEmptyString, outDir);
	} 
	
	theApp->ShowAlert(msg, wxT("Fatal error."), wxICON_ERROR | wxOK);
	outDir = CPath(wxEmptyString);
	return false;
}
