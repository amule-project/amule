//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "AutostartManager.h"

#include <wx/app.h>		// Needed for wxTheApp
#include <wx/filename.h>	// Needed for wxFileName
#include <wx/log.h>		// Needed for wxLogDebug
#include <wx/regex.h>		// Needed for wxRegEx (macOS plist parsing)
#include <wx/stdpaths.h>	// Needed for wxStandardPaths

#ifdef __WXMSW__
	#include <windows.h>
	#include <cwchar>	// Needed for wcslen
	#include <vector>	// Needed for std::vector buffer in BackendReadTargetPath
#else
	#include <climits>	// Needed for PATH_MAX
	#include <stdlib.h>	// Needed for realpath
	#include <wx/utils.h>	// Needed for wxGetEnv / wxGetUserHome
	#include <wx/file.h>	// Needed for wxFile
	#include <wx/filefn.h>	// Needed for wxRemoveFile
	#include <wx/tokenzr.h>	// Needed for wxStringTokenizer
#endif

// Per-backend low-level helpers (return raw OS state, no policy).
// Defined in the platform-specific section near the bottom of this file.
namespace
{
	// Reads the autostart entry's target path from the OS store.
	// Returns wxEmptyString when the entry doesn't exist OR can't be
	// parsed. Doesn't validate the path against the filesystem.
	wxString BackendReadTargetPath();

	// Writes/overwrites the autostart entry to point at `executable`.
	// Returns true on success.
	bool BackendWrite(const wxString &executable);

	// Removes the autostart entry. Idempotent — returns true if the
	// entry didn't exist either.
	bool BackendRemove();
}


wxString AutostartManager::GetCanonicalExecutablePath()
{
	// wxStandardPaths::GetExecutablePath() wraps the OS native call
	// (GetModuleFileNameW on Windows, _NSGetExecutablePath on macOS,
	// /proc/self/exe readlink on Linux), then on POSIX we resolve
	// any intermediate symlinks via realpath() so AppImage / .app
	// bundle moves are detected correctly.
	wxString raw = wxStandardPaths::Get().GetExecutablePath();

#ifndef __WXMSW__
	// realpath() rejects empty input; guard.
	if (raw.empty()) {
		return raw;
	}
	char resolved[PATH_MAX];
	if (realpath(raw.mb_str(wxConvUTF8), resolved) != NULL) {
		return wxString::FromUTF8(resolved);
	}
	// realpath failed (binary unlinked? permission?) — fall through
	// to the raw path; the autostart entry will still work as long
	// as the OS can resolve it.
#endif

	return raw;
}


bool AutostartManager::IsEnabled()
{
	return !BackendReadTargetPath().empty();
}


bool AutostartManager::Enable()
{
	wxString exe = GetCanonicalExecutablePath();
	if (exe.empty()) {
		wxLogDebug(wxT("AutostartManager::Enable: no executable path resolved, refusing to write a broken entry"));
		return false;
	}
	return BackendWrite(exe);
}


bool AutostartManager::Disable()
{
	return BackendRemove();
}


void AutostartManager::SelfHealOnStartup()
{
	wxString registered = BackendReadTargetPath();
	if (registered.empty()) {
		// No entry — user chose not to autostart, or never enabled it.
		// Don't second-guess.
		return;
	}

	wxString canonical = GetCanonicalExecutablePath();
	if (canonical.empty()) {
		// Couldn't resolve the running binary's path; best to leave
		// the existing entry alone rather than blow it away.
		return;
	}

	if (registered == canonical) {
		// Already pointing at us, nothing to do.
		return;
	}

	// Path drifted (user moved AppImage / .app / install dir, or
	// upgraded via a tool that didn't rewrite the entry). Rewrite
	// to match the current canonical path so the next login launches
	// the right binary.
	wxLogDebug(wxT("AutostartManager::SelfHealOnStartup: rewriting autostart entry from '%s' to '%s'"),
		registered.c_str(), canonical.c_str());
	BackendWrite(canonical);
}


// --------------------------------------------------------------------
// Platform backends
// --------------------------------------------------------------------

namespace
{

#if defined(__WXMSW__)

	// Windows: per-user "Run on login" registry key. HKCU (not HKLM)
	// so autostart is a per-user choice on shared machines and toggling
	// never needs elevation. The same key Task Manager → Startup tab
	// reads.
	static const wchar_t *RUN_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	static const wchar_t *RUN_VALUE_NAME = L"aMule";

	wxString BackendReadTargetPath()
	{
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
			return wxEmptyString;
		}

		// First call returns the required buffer size; second call
		// reads the value. Sized in bytes, includes the trailing NUL.
		DWORD type = 0;
		DWORD cb = 0;
		LSTATUS rc = RegQueryValueExW(hKey, RUN_VALUE_NAME, NULL, &type, NULL, &cb);
		if (rc != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || cb == 0) {
			RegCloseKey(hKey);
			return wxEmptyString;
		}

		// cb is in bytes; convert to wide-character count, rounding up.
		size_t wlen = (cb + sizeof(wchar_t) - 1) / sizeof(wchar_t);
		std::vector<wchar_t> buf(wlen + 1, L'\0');
		rc = RegQueryValueExW(hKey, RUN_VALUE_NAME, NULL, &type,
			reinterpret_cast<LPBYTE>(buf.data()), &cb);
		RegCloseKey(hKey);
		if (rc != ERROR_SUCCESS) {
			return wxEmptyString;
		}

		wxString raw(buf.data());

		// Windows stores Run entries either bare ("C:\Foo\bar.exe")
		// or quoted ('"C:\Foo\bar.exe" --some-flag'). Strip surrounding
		// quotes and discard any argument tail so the path comparison
		// in SelfHealOnStartup matches the unadorned canonical path.
		if (!raw.empty() && raw[0] == wxT('"')) {
			size_t closing = raw.find(wxT('"'), 1);
			if (closing != wxString::npos) {
				return raw.SubString(1, closing - 1);
			}
		}
		// Unquoted: take the leading non-whitespace run as the path.
		size_t sp = raw.find_first_of(wxT(" \t"));
		if (sp != wxString::npos) {
			return raw.SubString(0, sp - 1);
		}
		return raw;
	}

	bool BackendWrite(const wxString &executable)
	{
		HKEY hKey;
		if (RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, NULL, 0,
				KEY_SET_VALUE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
			return false;
		}

		// Quote the path so spaces in "Program Files" don't get parsed
		// as argument separators by Windows' Run-key handler.
		wxString quoted = wxT("\"") + executable + wxT("\"");
		const wchar_t *wstr = quoted.wc_str();
		// cb counts the trailing NUL too, per RegSetValueExW contract.
		DWORD cb = static_cast<DWORD>((wcslen(wstr) + 1) * sizeof(wchar_t));
		LSTATUS rc = RegSetValueExW(hKey, RUN_VALUE_NAME, 0, REG_SZ,
			reinterpret_cast<const BYTE *>(wstr), cb);
		RegCloseKey(hKey);
		return rc == ERROR_SUCCESS;
	}

	bool BackendRemove()
	{
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
			// Key itself doesn't exist → nothing to remove, success.
			return true;
		}
		LSTATUS rc = RegDeleteValueW(hKey, RUN_VALUE_NAME);
		RegCloseKey(hKey);
		// ERROR_FILE_NOT_FOUND = value already absent, also success.
		return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
	}

#elif defined(__WXMAC__) || defined(__WXOSX__)

	// macOS: per-user LaunchAgent. Registered command is
	// `/usr/bin/open -a <aMule.app>` rather than launching the bare
	// Mach-O directly — sidesteps Gatekeeper / quarantine warnings
	// when launchd activates us at login.
	static const wxString PLIST_LABEL = wxT("org.amule.amule");

	static wxString PlistPath()
	{
		return wxGetUserHome() + wxT("/Library/LaunchAgents/") + PLIST_LABEL + wxT(".plist");
	}

	wxString BackendReadTargetPath()
	{
		wxString path = PlistPath();
		if (!wxFileName::FileExists(path)) {
			return wxEmptyString;
		}

		wxFile f(path, wxFile::read);
		if (!f.IsOpened()) {
			return wxEmptyString;
		}
		wxString content;
		f.ReadAll(&content, wxConvUTF8);
		f.Close();

		// ProgramArguments array layout we write below:
		//   <string>/usr/bin/open</string>
		//   <string>-a</string>
		//   <string>/path/to/aMule.app</string>
		// Take the third <string> as the registered target. Plist
		// XML is simple enough that regex extraction is safer than
		// pulling in a full plist parser dependency.
		wxRegEx re(wxT("<string>([^<]+)</string>"), wxRE_ADVANCED);
		if (!re.IsValid()) {
			return wxEmptyString;
		}
		wxString cursor = content;
		for (int i = 0; i < 3; ++i) {
			if (!re.Matches(cursor)) {
				return wxEmptyString;
			}
			if (i == 2) {
				return re.GetMatch(cursor, 1);
			}
			size_t start = 0, len = 0;
			re.GetMatch(&start, &len, 0);
			cursor = cursor.Mid(start + len);
		}
		return wxEmptyString;
	}

	bool BackendWrite(const wxString &canonicalExe)
	{
		// Convert the canonical exe path to its containing .app
		// bundle: `/Applications/aMule.app/Contents/MacOS/amule`
		// → `/Applications/aMule.app`. Strip the last two path
		// components plus the basename.
		wxString appBundle = canonicalExe;
		int idx = appBundle.Find(wxT(".app/"));
		if (idx != wxNOT_FOUND) {
			appBundle = appBundle.Mid(0, idx + 4);  // keep ".app"
		}

		wxString dir = wxGetUserHome() + wxT("/Library/LaunchAgents");
		if (!wxFileName::DirExists(dir)) {
			if (!wxFileName::Mkdir(dir, 0755, wxPATH_MKDIR_FULL)) {
				return false;
			}
		}

		wxString xml;
		xml << wxT("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		xml << wxT("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" ");
		xml << wxT("\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
		xml << wxT("<plist version=\"1.0\">\n");
		xml << wxT("<dict>\n");
		xml << wxT("    <key>Label</key>\n");
		xml << wxT("    <string>") << PLIST_LABEL << wxT("</string>\n");
		xml << wxT("    <key>ProgramArguments</key>\n");
		xml << wxT("    <array>\n");
		xml << wxT("        <string>/usr/bin/open</string>\n");
		xml << wxT("        <string>-a</string>\n");
		xml << wxT("        <string>") << appBundle << wxT("</string>\n");
		xml << wxT("    </array>\n");
		xml << wxT("    <key>RunAtLoad</key>\n");
		xml << wxT("    <true/>\n");
		// KeepAlive=false so user-quit means quit. launchd would
		// otherwise treat us as a service to be respawned, which
		// fights the user's deliberate close.
		xml << wxT("    <key>KeepAlive</key>\n");
		xml << wxT("    <false/>\n");
		xml << wxT("</dict>\n");
		xml << wxT("</plist>\n");

		wxFile f;
		if (!f.Create(PlistPath(), true /* overwrite */, 0644)) {
			return false;
		}
		bool ok = f.Write(xml, wxConvUTF8);
		f.Close();
		return ok;
	}

	bool BackendRemove()
	{
		wxString path = PlistPath();
		if (!wxFileName::FileExists(path)) {
			return true;
		}
		return wxRemoveFile(path);
	}

#else  // assumed Linux / *BSD with XDG-compliant desktop env

	// Linux: XDG Autostart spec. $XDG_CONFIG_HOME (falls back to
	// ~/.config) is where the DE's "Startup Applications" GUI looks,
	// so users can see and toggle the entry without touching a
	// terminal. systemd user units don't show up in those GUIs.
	// https://specifications.freedesktop.org/autostart-spec/latest/

	static wxString XdgAutostartDir()
	{
		wxString xdg;
		if (wxGetEnv(wxT("XDG_CONFIG_HOME"), &xdg) && !xdg.empty()) {
			return xdg + wxT("/autostart");
		}
		return wxGetUserHome() + wxT("/.config/autostart");
	}

	static wxString DesktopFilePath()
	{
		return XdgAutostartDir() + wxT("/amule.desktop");
	}

	wxString BackendReadTargetPath()
	{
		wxString path = DesktopFilePath();
		if (!wxFileName::FileExists(path)) {
			return wxEmptyString;
		}

		wxFile f(path, wxFile::read);
		if (!f.IsOpened()) {
			return wxEmptyString;
		}
		wxString content;
		f.ReadAll(&content, wxConvUTF8);
		f.Close();

		// Parse the Exec= line. .desktop syntax allows the field-code
		// expansion (%U, %f etc.) after the executable; the path is
		// always the first whitespace-delimited token and may be
		// quoted with double-quotes for paths containing spaces.
		wxStringTokenizer lines(content, wxT("\n"));
		while (lines.HasMoreTokens()) {
			wxString line = lines.GetNextToken().Trim(false).Trim(true);
			if (!line.StartsWith(wxT("Exec="))) {
				continue;
			}
			wxString value = line.Mid(5).Trim(false);
			if (value.empty()) {
				return wxEmptyString;
			}
			if (value[0] == wxT('"')) {
				size_t closing = value.find(wxT('"'), 1);
				if (closing != wxString::npos) {
					return value.SubString(1, closing - 1);
				}
			}
			size_t sp = value.find_first_of(wxT(" \t"));
			if (sp != wxString::npos) {
				return value.SubString(0, sp - 1);
			}
			return value;
		}
		return wxEmptyString;
	}

	bool BackendWrite(const wxString &executable)
	{
		wxString dir = XdgAutostartDir();
		if (!wxFileName::DirExists(dir)) {
			// Mkdir -p; the XDG dir may not exist yet on a fresh
			// install or on minimal DEs that don't ship anything
			// there by default.
			if (!wxFileName::Mkdir(dir, 0755, wxPATH_MKDIR_FULL)) {
				return false;
			}
		}

		// Quote the path so embedded spaces (e.g. "/home/user/My Apps/amule")
		// survive the .desktop Exec= parser's tokenisation.
		wxString quotedExec = wxT("\"") + executable + wxT("\"");

		// Standard XDG Autostart fields. X-GNOME-Autostart-enabled
		// is widely-recognised even outside GNOME and makes the
		// entry trivially toggleable from the user's DE settings GUI
		// without us having to rewrite the file.
		wxString content;
		content << wxT("[Desktop Entry]\n");
		content << wxT("Type=Application\n");
		content << wxT("Name=aMule\n");
		content << wxT("Comment=Start aMule when the user logs in\n");
		content << wxT("Exec=") << quotedExec << wxT("\n");
		content << wxT("Terminal=false\n");
		content << wxT("X-GNOME-Autostart-enabled=true\n");
		content << wxT("Hidden=false\n");

		wxFile f;
		if (!f.Create(DesktopFilePath(), true /* overwrite */, 0644)) {
			return false;
		}
		bool ok = f.Write(content, wxConvUTF8);
		f.Close();
		return ok;
	}

	bool BackendRemove()
	{
		wxString path = DesktopFilePath();
		if (!wxFileName::FileExists(path)) {
			return true;  // already absent → success
		}
		return wxRemoveFile(path);
	}

#endif

}  // namespace
