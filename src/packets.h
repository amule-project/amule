// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef PACKETS_H
#define PACKETS_H

#include <cstdio>		// Needed for FILE
#include <exception>

#include "types.h"		// Needed for int8, int32, uint8 and uint32
#include "opcodes.h"		// Needed for OP_EDONKEYPROT
#include "CString.h"		

class CMemFile;
class CFile;

//			CLIENT TO SERVER

//			PACKET CLASS
// TODO some parts could need some work to make it more efficient

class Packet {
public:
	Packet(uint8 protocol = OP_EDONKEYPROT);
	Packet(char* header); // only used for receiving packets
	Packet(CMemFile* datafile, uint8 protocol = OP_EDONKEYPROT);
	
	Packet(int8 in_opcode,wxInt32 in_size,uint8 protocol = OP_EDONKEYPROT,bool bFromPF = true);
	Packet(char* pPacketPart,wxUint32 nSize,bool bLast,bool bFromPF = true); // only used for splitted packets!

	~Packet();
	char*		GetHeader();
	char*		GetUDPHeader();
	char*		GetPacket();
	char*		DetachPacket();
	wxUint32		GetRealPacketSize()		{ return size + 6; }
	bool		IsSplitted()				{ return m_bSplitted; }
	bool		IsLastSplitted()			{ return m_bLastSplitted; }
	void		PackPacket();
	bool		UnPackPacket(wxUint32 uMaxDecompressedSize = 50000);
	char*		pBuffer;
	wxUint32		size;
	uint8		opcode;
	uint8		prot;
	// -khaos--+++> Returns either -1, 0 or 1.  -1 is unset, 0 is from complete file, 1 is from part file
	bool	IsFromPF()				{return m_bFromPF;}
	
private:
	bool		m_bSplitted;
	bool		m_bLastSplitted;
	char		head[6];
	char*		completebuffer;
	char*		tempbuffer;
	bool		m_bPacked;
	int			m_bFromPF;
};

class CInvalidPacket : public std::exception
{
public:

	CInvalidPacket(const char* = "not specified");
	virtual const char* what() const throw();

private:
	
	char 	m_acWhat[256];

};

class CStrangePacket : public CInvalidPacket
{
public:
	
	CStrangePacket(const char* w = "not specified") : CInvalidPacket(w) { }
	
};

struct STag{
	STag();
	STag(const STag& in);
	~STag();

	int8	type;
	LPSTR	tagname;
	union{
	  LPSTR	stringvalue;
	  uint32	intvalue;
	  float floatvalue;
	};
	uint8	specialtag;
};

class CTag {
public:
	CTag(LPCSTR name, uint32 intvalue);
	CTag(int8 special, uint32 intvalue);
	CTag(LPCSTR name, LPCSTR strvalue);
	CTag(int8 special, LPCSTR strvalue);
	CTag(LPCSTR name,   const wxString& strvalue);
	CTag(int8   special,const wxString& strvalue);
	CTag(const STag& in_tag);
	CTag(CFile* in_data);
	//CTag(FILE* in_data);
	~CTag();
	
	CTag* CloneTag() { return new CTag(tag); }
	
	bool WriteTagToFile(CFile* file); //used for CMemfiles
	bool WriteTagToFile(FILE* file); //used for CMemfiles
	
	STag tag;
	CString GetFullInfo() const;
};

#endif // PACKETS_H
