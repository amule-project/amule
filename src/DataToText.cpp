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
#include "KnownFile.h"

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


