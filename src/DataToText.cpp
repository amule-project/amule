// This file is part of the aMule project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "DataToText.h"

#include "KnownFile.h"		// Needed by PriorityToStr
#include "updownclient.h"	// Needed by DownloadStateToStr

#include <wx/string.h>
#include <wx/intl.h>

wxString PriorityToStr( int priority, bool isAuto )
{
	if ( isAuto ) {
		switch ( priority ) {
			case PR_LOW:		return _("Auto [Lo]");
			case PR_NORMAL:		return _("Auto [No]");
			case PR_HIGH:		return _("Auto [Hi]");
		}
	} else {
		switch ( priority ) {
			case PR_VERYLOW:	return _("Very low");
			case PR_LOW:		return _("Low");
			case PR_NORMAL:		return _("Normal");
			case PR_HIGH:		return _("High");
			case PR_VERYHIGH:	return _("Very High");
			case PR_POWERSHARE:	return _("Release");
		}
	}

	wxASSERT( false );

	return _("Unknown");
}


wxString DownloadStateToStr( int state, bool queueFull )
{
	switch ( state ) {
		case DS_CONNECTING:		return  _("Connecting");
		case DS_CONNECTED:		return _("Asking");
		case DS_WAITCALLBACK:	return _("Connecting via server");
		case DS_ONQUEUE:		return ( queueFull ? _("Queue Full") : _("On Queue") );
		case DS_DOWNLOADING:	return _("Transferring");
		case DS_REQHASHSET:		return _("Receiving hashset");
		case DS_NONEEDEDPARTS:	return _("No needed parts");
		case DS_LOWTOLOWIP:		return _("Cannot connect LowID to LowID");
		case DS_TOOMANYCONNS:	return _("Too many connections");
		case DS_NONE:					return _("Unknown");
	}
	
	wxASSERT( false );
	
	return _("Unknown");
}
