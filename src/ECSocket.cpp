// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Project ( http://www.amule-prpject.net )
// Copyright (c) 2004 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#include "ECSocket.h"


//shakraw - sends and receive string data to/from ECServer
wxString ECSocket::SendRecvMsg(const wxChar *msg) {

	wxString response(wxT(""));

  	uint16 len  = (wxStrlen(msg) + 1) * sizeof(wxChar);

	this->SetFlags(wxSOCKET_WAITALL);
	this->Write(len);
	this->WriteMsg(msg, len);
	if (!this->Error()) {
		// Wait until data available (will also return if the connection is lost)
		this->WaitForRead(10);
	
		if (this->IsData()) {
			//lenbuf 
			this->Read(len);
			wxChar *result = new wxChar[len]; // read 10Kb at time
			this->ReadMsg(result, len);
			if (!this->Error()) {
				response.Append(result);
			}
			delete[] result;
		}
	}
	return(response);
}
