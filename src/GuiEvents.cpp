// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Froenchenko Leonid (lfroen@users.sourceforge.net)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include <assert.h>
#include <stdio.h>
#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#ifdef __BSD__
     	  #include <sys/types.h>
	#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif
#include <wx/app.h>
#include <wx/config.h>
#include <wx/socket.h>		// Needed for wxSocket
#include <wx/utils.h>
#include <wx/timer.h>

#include "types.h"		// Needed for int32, uint16 and uint64
#include "GuiEvents.h"


DEFINE_EVENT_TYPE(wxEVT_NOTIFY_EVENT)
DEFINE_EVENT_TYPE(wxEVT_AMULE_TIMER)

DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_CORE_FILE_HASHING_SHUTDOWN)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_FILE_COMPLETION)
DEFINE_EVENT_TYPE(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD)
DEFINE_EVENT_TYPE(wxEVT_CORE_SOURCE_DNS_DONE)
DEFINE_EVENT_TYPE(wxEVT_CORE_DNS_DONE)
