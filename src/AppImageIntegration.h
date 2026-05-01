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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifndef APPIMAGEINTEGRATION_H
#define APPIMAGEINTEGRATION_H

#include <wx/window.h>


namespace AppImageIntegration {

// Whether we're running from inside an AppImage AND the user hasn't yet
// installed launcher integration AND hasn't opted out via prefs. When true,
// the GUI app should call PromptAndInstall() once the main frame is realised.
bool ShouldPrompt();

// Show a wxRichMessageDialog asking whether to install
// ~/.local/share/applications + ~/.local/share/icons entries pointing at the
// running AppImage. The dialog includes a "Don't ask again" checkbox.
//
// On Yes: copies the bundled .desktop and hicolor icons into the user's home,
// rewrites Exec= to point at $APPIMAGE, and runs update-desktop-database /
// gtk-update-icon-cache best-effort.
// On No: nothing this run; the prompt fires again on next launch.
// "Don't ask again" checked: sets the prefs flag so we never prompt again,
// regardless of which button was clicked.
void PromptAndInstall(wxWindow* parent);

} // namespace AppImageIntegration

#endif // APPIMAGEINTEGRATION_H
