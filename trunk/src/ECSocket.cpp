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

#include "ECSocket.h"
#include "endianfix.h"


//shakraw - sends and receive string data to/from ECServer
wxString ECSocket::SendRecvMsg(const wxChar *msg) {

	wxString response("");

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

inline ECSocket& ECSocket::Read(uint8& v)
{
	return ReadRaw(&v, 1);
}

inline ECSocket& ECSocket::Read(uint16& v)
{
	ReadRaw(&v, 2);
	ENDIAN_SWAP_I_16(v);
	return *this;
}

inline ECSocket& ECSocket::Read(uint32& v)
{
	ReadRaw(&v, 4);
	ENDIAN_SWAP_I_32(v);
	return *this;
}

inline ECSocket& ECSocket::Read(uint64& v)
{
	ReadRaw(&v, 8);
	ENDIAN_SWAP_I_64(v);
	return *this;
}

inline ECSocket& ECSocket::Read(wxString& v)
{
	uint16 len;
	Read(len);
	ReadRaw(v.GetWriteBuf(len), len);
	v.UngetWriteBuf(len);
	if (Error()) {
		printf("Wrong wxString Reading Packet!!!\n");
	}
	return *this;
}

inline ECSocket& ECSocket::Write(const uint8& v)
{
	return WriteRaw(&v, 1);
}

inline ECSocket& ECSocket::Write(const uint16& v)
{
	int16 tmp = ENDIAN_SWAP_16(v);
	return WriteRaw(&tmp, 2);
}
	
inline ECSocket& ECSocket::Write(const uint32& v)
{
	int32 tmp = ENDIAN_SWAP_32(v);
	return WriteRaw(&tmp, 4);
}
	
inline ECSocket& ECSocket::Write(const uint64& v)
{
	int64 tmp = ENDIAN_SWAP_32(v);
	return WriteRaw(&tmp, 8);
}

inline ECSocket& ECSocket::Write(const wxString& v)
{
	Write((uint16)v.Length());
	return WriteRaw(v.c_str(), v.Length());
}
