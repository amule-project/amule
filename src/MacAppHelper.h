//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( admin@amule.org / http://www.amule.org )
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#ifndef MAC_APP_HELPER_H
#define MAC_APP_HELPER_H

#ifdef __WXMAC__

#ifdef __cplusplus
extern "C" {
#endif

// Toggles NSApp's activation policy. Passing true switches the app to
// NSApplicationActivationPolicyAccessory (no Dock icon, menu-bar /
// NSStatusItem only); false restores NSApplicationActivationPolicyRegular
// (normal Dock icon). Used to remove the Dock thumbnail when the main
// window is hidden via "minimize to tray".
void mac_set_accessory_mode(bool accessory);

#ifdef __cplusplus
}
#endif

#endif // __WXMAC__

#endif // MAC_APP_HELPER_H
