//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef COMMONCONSTANTS_H
#define COMMONCONSTANTS_H

const unsigned UNLIMITED = 0;

// Internal sentinel for "no upload throttling at all" — used when the user
// has set MaxUpload=0 in prefs. Distinct from UNLIMITED=0 (the user-facing
// pref value) so the throttle loop can skip the per-iteration rate cap math
// entirely rather than dividing a budget by zero or by a meaningless ramp.
#include <climits>
const unsigned UNLIMITED_RATE = UINT_MAX;

#define	MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE	1500
#define	DISKSPACERECHECKTIME			60000	// checkDiskspace

#endif // COMMONCONSTANTS_H
