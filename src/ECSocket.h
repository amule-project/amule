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
	
	virtual off_t Read(uint8&);
	virtual off_t Read(uint16&);
	virtual off_t Read(uint32&);
	virtual off_t Read(uint64&);
	virtual off_t Read(wxString&);

	virtual size_t Write(const uint8&);
	virtual size_t Write(const uint16&);
	virtual size_t Write(const uint32&);
	virtual size_t Write(const uint64&);
	virtual size_t Write(const wxString&);
	
	virtual off_t  ReadRaw(void* buffer,off_t length) { wxSocketClient::Read(buffer,length);};
	virtual size_t WriteRaw(const void* buffer,size_t length) { wxSocketClient::Write(buffer,length); };		
};


#endif // ECSOCKET_H
