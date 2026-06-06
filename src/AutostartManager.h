//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

#ifndef AUTOSTARTMANAGER_H
#define AUTOSTARTMANAGER_H

#include <wx/string.h>

// Cross-platform "start aMule when the user logs in" toggle. The
// per-OS store of record is:
//
//   Windows : HKCU\Software\Microsoft\Windows\CurrentVersion\Run\aMule
//   Linux   : $XDG_CONFIG_HOME/autostart/amule.desktop (per XDG spec)
//   macOS   : ~/Library/LaunchAgents/org.amule.amule.plist
//
// All three are per-user — no elevation required. Toggling reads/writes
// the OS directly, never aMule.conf, so the OS is always the source of
// truth (matches what the user sees in Task Manager / Login Items /
// `systemctl --user list-unit-files` etc.).
class AutostartManager
{
public:
	// Returns true if an autostart entry for aMule exists in the
	// OS's per-user store. Doesn't validate the registered path
	// against the running binary (use SelfHealOnStartup() for that).
	static bool IsEnabled();

	// Writes/overwrites the autostart entry to point at the running
	// binary's canonical path. Idempotent; returns true on success.
	static bool Enable();

	// Removes the autostart entry if present. Idempotent (no-op if
	// already disabled); returns true on success.
	static bool Disable();

	// Called once from CamuleApp::OnInit. If an autostart entry
	// exists AND its registered path differs from the canonical
	// path of the currently-running binary, rewrites it so the
	// next login launches the right binary. Handles the
	// "user moved the AppImage / .app / install dir" case without
	// requiring them to re-toggle the checkbox.
	//
	// Does nothing if no entry exists — disabling autostart is
	// always a deliberate user choice we don't second-guess.
	static void SelfHealOnStartup();

	// Resolves argv[0] to its canonical absolute path (realpath()
	// on POSIX, GetModuleFileNameW() on Windows). Used by both the
	// Enable() write and the SelfHealOnStartup() comparison.
	static wxString GetCanonicalExecutablePath();
};

#endif // AUTOSTARTMANAGER_H
// File_checked_for_headers
