// This file is part of the aMule Project.
//
// Copyright (c) 2004 aMule Team ( http://www.amule-project.net )
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
#include "endianfix.h"


class ECSocket : public wxSocketClient {

public:
	
	wxString SendRecvMsg(const wxChar *msg);
	
	inline ECSocket& Read(uint8& v) {
		return ReadRaw(&v, 1);
	};
	
	inline ECSocket& Read(uint16& v) {
		ReadRaw(&v, 2);
		ENDIAN_SWAP_I_16(v);
		return *this;
	};
	
	inline ECSocket& Read(uint32& v) {
		ReadRaw(&v, 4);
		ENDIAN_SWAP_I_32(v);
		return *this;
	};
	
	#if 0
	inline ECSocket& Read(uint64& v) {
		ReadRaw(&v, 8);
		ENDIAN_SWAP_I_64(v);
		return *this;
	};
	#endif
	
	inline ECSocket& Read(wxString& v) {
		uint16 len;
		Read(len);
		wxChar *buf = new wxChar[len+1];
		ReadRaw(buf, len);
		buf[len] = 0;
		v = wxString(buf,len);
		delete[] buf;
		if (Error()) {
			printf("Wrong wxString Reading Packet!!!\n");
		}
		return *this;
	};

	inline ECSocket& Write(const uint8& v) {
		return WriteRaw(&v, 1);
	};

	inline ECSocket& Write(const uint16& v) {
		int16 tmp = ENDIAN_SWAP_16(v);
		return WriteRaw(&tmp, 2);
	};
	
	inline ECSocket& Write(const uint32& v) {
		int32 tmp = ENDIAN_SWAP_32(v);
		return WriteRaw(&tmp, 4);
	};
	
	#if 0
	inline ECSocket& Write(const uint64& v) {
		int64 tmp = ENDIAN_SWAP_32(v);
		return WriteRaw(&tmp, 8);
	};
	#endif
	
	inline ECSocket& Write(const wxString& v) {
		Write((uint16)v.Length());
		return WriteRaw(v.c_str(), v.Length());
	};
	
	inline ECSocket& ReadRaw(void* buffer, off_t length) { 
		wxSocketClient::Read(buffer,length); 
		return *this;
	};
	
	inline ECSocket& WriteRaw(const void* buffer, size_t length) { 
		wxSocketClient::Write(buffer,length); 
		return *this;
	};		
};


#endif // ECSOCKET_H
