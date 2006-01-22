//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef PACKET_H
#define PACKET_H

#include "Types.h"		// Needed for int8, int32, uint8 and uint32
#include "OPCodes.h"		// Needed for OP_EDONKEYPROT

class CMemFile;
class wxString;

//			CLIENT TO SERVER

//			PACKET CLASS
// TODO some parts could need some work to make it more efficient

class CPacket {
public:
	CPacket(CPacket &p);
	CPacket(uint8 protocol = OP_EDONKEYPROT);
	CPacket(byte* header); // only used for receiving packets
	CPacket(CMemFile* datafile, uint8 protocol = OP_EDONKEYPROT, uint8 ucOpcode = 0x00);
	CPacket(int8 in_opcode, uint32 in_size, uint8 protocol = OP_EDONKEYPROT, bool bFromPF = true);
	CPacket(byte* pPacketPart, uint32 nSize, bool bLast, bool bFromPF = true); // only used for splitted packets!

	~CPacket();
	
	void 			AllocDataBuffer();	
	byte*			GetHeader();
	byte*			GetUDPHeader();
	byte*			GetPacket();
	byte*			DetachPacket();
	uint32 			GetRealPacketSize() const	{ return size + 6; }
	bool			IsSplitted()		{ return m_bSplitted; }
	bool			IsLastSplitted()	{ return m_bLastSplitted; }
	void			PackPacket();
	bool			UnPackPacket(uint32 uMaxDecompressedSize = 50000);
	// -khaos--+++> Returns either -1, 0 or 1.  -1 is unset, 0 is from complete file, 1 is from part file
	bool			IsFromPF()		{ return m_bFromPF; }
	
	uint8			GetOpCode() const	{ return opcode; }
	void			SetOpCode(uint8 oc)	{ opcode = oc; }
	uint32			GetPacketSize() const	{ return size; }
	uint8			GetProtocol() const	{ return prot; }
	void			SetProtocol(uint8 p)	{ prot = p; }
	const byte* 	GetDataBuffer(void) const { return pBuffer; }
	void 			Copy16ToDataBuffer(const void* data);
	void 			CopyToDataBuffer(unsigned int offset, const byte* data, unsigned int n);
	void			CopyUInt32ToDataBuffer(uint32 data, unsigned int offset = 0);
	
private:
	//! CPacket is not assignable.
	CPacket& operator=(const CPacket&);
	
	uint32		size;
	uint8		opcode;
	uint8		prot;
	bool		m_bSplitted;
	bool		m_bLastSplitted;
	bool		m_bPacked;
	bool		m_bFromPF;
	byte		head[6];
	byte*		tempbuffer;
	byte*		completebuffer;
	byte*		pBuffer;
};

#endif // PACKET_H
