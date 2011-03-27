//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Stu Redman ( sturedman@amule.org / http://www.amule.org )
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

#ifndef PCH_H
#define PCH_H

#include <wx/wxprec.h>

#include <csignal>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

#include "CryptoPP_Inc.h"

#include "amule.h"
#include "config.h"

#include <common/ClientVersion.h>
#include <common/EventIDs.h>
#include <common/FileFunctions.h>
#include <common/Format.h>
#include <common/Macros.h>
#include <common/MacrosProgramSpecific.h>
#include <common/MD5Sum.h>
#include <common/MuleDebug.h>
#include <common/Path.h>
#include <common/StringFunctions.h>
#include <common/TextFile.h>

#include "ClientCreditsList.h"
#include "ClientList.h"
#include "ClientRef.h"
#include "ClientUDPSocket.h"
#include "ExternalConn.h"
#include "InternalEvents.h"
#include "KnownFileList.h"
#include "ListenSocket.h"
#include "Logger.h"
#include "MD4Hash.h"
#include "OtherFunctions.h"
#include "PartFile.h"
#include "Preferences.h"
#include "ScopedPtr.h"
#include "Server.h"
#include "Statistics.h"
#include "ThreadTasks.h"
#include "UserEvents.h"

#ifndef CLIENT_GUI
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "IPFilter.h"
#endif

#ifndef AMULE_DAEMON
#include "amuleDlg.h"
#include "muuli_wdr.h"
#endif

#endif
