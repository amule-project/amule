//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


#ifndef UPNPCOMPATIBILITY_H
#define UPNPCOMPATIBILITY_H

#include "extern/upnp/upnp.h"


#include <sstream>


#include <libs/common/MuleDebug.h>		// for CMuleException::
#include <libs/common/StringFunctions.h>	// for char2unicode()


#include "Logger.h"				// for Add(Debug)LogLineM()


namespace CLogger
{
	void AddLogLine(
		bool critical,
		const std::ostringstream &msg);
	void AddLogLine(
		bool critical,
		DebugType type,
		const std::ostringstream &msg);
}


class CUPnPException : public CMuleException
{
public:
	CUPnPException(const std::ostringstream& msg)
	:
	CMuleException(
		wxT("CUPnPException"),
		wxString(char2unicode(msg.str().c_str()))) {}
};


#endif // UPNPCOMPATIBILITY_H

