// This file is part of the aMule project.
//
// Copyright (c) 2004, aMule team
//
// Copyright (c) Angel Vidal Veiga (kry@users.sourceforge.net)
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

#ifndef ECSOCKET_H
#define ECSOCKET_H

#include <wx/string.h>
#include <wx/socket.h>
#include "types.h"


class ECSocket : public wxSocketClient {

public:	
	wxString SendRecvMsg(const wxChar *msg);
	
	virtual ECSocket& Read(uint8&);
	virtual ECSocket& Read(uint16&);
	virtual ECSocket& Read(uint32&);
	#ifndef __WXMAC__
	virtual ECSocket& Read(uint64&);
	#endif
	virtual ECSocket& Read(wxString&);

	virtual ECSocket& Write(const uint8&);
	virtual ECSocket& Write(const uint16&);
	virtual ECSocket& Write(const uint32&);
	#ifndef __WXMAC__
	virtual ECSocket& Write(const uint64&);
	#endif
	virtual ECSocket& Write(const wxString&);
	
	virtual ECSocket& ReadRaw(void* buffer,off_t length) { wxSocketClient::Read(buffer,length); return *this; };
	virtual ECSocket& WriteRaw(const void* buffer,size_t length) { wxSocketClient::Write(buffer,length); return *this;};		
};


#endif // ECSOCKET_H
