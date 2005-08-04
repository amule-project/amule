//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef PACKET_H
#define PACKET_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Packet.h"
#endif

#include <exception>
#include <stdexcept>
#include <list>

#include <wx/string.h>

#include "Types.h"		// Needed for int8, int32, uint8 and uint32
#include "OPCodes.h"		// Needed for OP_EDONKEYPROT
#include "SafeFile.h"		// Needed for CFileDataIO
#include "ArchSpecific.h"
#include "OtherFunctions.h"

class CMemFile;
class CFile;

using namespace otherfunctions;

//			CLIENT TO SERVER

//			PACKET CLASS
// TODO some parts could need some work to make it more efficient

class CPacket {
public:
	CPacket(CPacket &p);
	CPacket(uint8 protocol = OP_EDONKEYPROT);
	CPacket(char* header); // only used for receiving packets
	CPacket(CMemFile* datafile, uint8 protocol = OP_EDONKEYPROT, uint8 ucOpcode = 0x00, bool detach = true);
	CPacket(int8 in_opcode, uint32 in_size, uint8 protocol = OP_EDONKEYPROT, bool bFromPF = true);
	CPacket(char* pPacketPart, uint32 nSize, bool bLast, bool bFromPF = true); // only used for splitted packets!

	~CPacket();
	
	void 			AllocDataBuffer();	
	char*			GetHeader();
	char*			GetUDPHeader();
	char*			GetPacket();
	char*			DetachPacket();
	inline uint32 GetRealPacketSize() const	{ return size + 6; }
	bool			IsSplitted()		{ return m_bSplitted; }
	bool			IsLastSplitted()	{ return m_bLastSplitted; }
	void			PackPacket();
	bool			UnPackPacket(uint32 uMaxDecompressedSize = 50000);
	// -khaos--+++> Returns either -1, 0 or 1.  -1 is unset, 0 is from complete file, 1 is from part file
	bool			IsFromPF()		{ return m_bFromPF; }
	
	inline uint8 	GetOpCode() const	{ return opcode; }
	void			SetOpCode(uint8 oc)	{ opcode = oc; }
	inline uint32 GetPacketSize() const	{ return size; }
	inline uint8 	GetProtocol() const	{ return prot; }
	inline void		SetProtocol(uint8 p)	{ prot = p; }
	inline const char * 	GetDataBuffer(void) const { return pBuffer; }
	inline void 		Copy16ToDataBuffer(const char *data) { md4cpy( pBuffer, data ); }
	void 			CopyToDataBuffer(unsigned int offset, const char *data, unsigned int n);
	void			CopyUInt32ToDataBuffer(uint32 data, unsigned int offset = 0) { 
		wxASSERT(offset <= size - sizeof(uint32) );
		PokeUInt32( pBuffer + offset, data );
	}
	
private:
	uint32		size;
	uint8		opcode;
	uint8		prot;
	bool		m_bSplitted;
	bool		m_bLastSplitted;
	bool		m_bPacked;
	bool		m_bFromPF;
	char		head[6];
	char 		*tempbuffer;
	char 		*completebuffer;
	char		*pBuffer;
};


class CInvalidPacket
{
public:
	CInvalidPacket( const wxString& arg = wxT("Invalid packet") )
		: m_what(arg)
	{ }

	const wxString& what() const {
		return m_what;
	}
	
private:
	wxString m_what;
};


///////////////////////////////////////////////////////////////////////////////
// CTag

class CTag
{
public:
	CTag(char* pszName, uint32 uVal);
	CTag(uint8 uName, uint32 uVal);
	CTag(char* pszName, const wxString& rstrVal);
	CTag(uint8 uName, const wxString& rstrVal);
	CTag(uint8 uName, const unsigned char* pucHash);
	CTag(uint8 uName, uint32 nSize, const unsigned char* pucData);
	CTag(const CTag& rTag);
	CTag(const CFileDataIO& data, bool bOptUTF8);
	~CTag();

	uint8 GetType() const			{ return m_uType; }
	uint8 GetNameID() const			{ return m_uName; }
	char* GetName() const			{ return m_pszName; }
	
	bool IsStr() const				{ return m_uType == TAGTYPE_STRING; }
	bool IsInt() const				{ return m_uType == TAGTYPE_UINT32; }
	bool IsFloat() const			{ return m_uType == TAGTYPE_FLOAT32; }
	bool IsHash() const				{ return m_uType == TAGTYPE_HASH; }
	bool IsBlob() const				{ return m_uType == TAGTYPE_BLOB; }
	
	uint32 GetInt() const			{ wxCHECK(IsInt(), 0);			return m_uVal; }
	const wxString& GetStr() const	{ wxCHECK(IsStr(), s_emptyStr);	return *m_pstrVal; }
	float GetFloat() const			{ wxCHECK(IsFloat(), 0.0f);		return m_fVal; }
	const byte* GetHash() const		{ wxCHECK(IsHash(), NULL);		return m_pData; }
	uint32 GetBlobSize() const		{ wxCHECK(IsBlob(), 0);			return m_nBlobSize; }
	const byte* GetBlob() const		{ wxCHECK(IsBlob(), NULL);		return m_pData; }

	void SetInt(uint32 uVal);
	
	CTag* CloneTag()				{ return new CTag(*this); }
	
	bool WriteTagToFile(CFileDataIO* file, EUtf8Str eStrEncode = utf8strNone) const;	// old eD2K tags
	bool WriteNewEd2kTag(CFileDataIO* file, EUtf8Str eStrEncode = utf8strNone) const;	// new eD2K tags
	
	wxString GetFullInfo() const;

protected:
	uint8	m_uType;
	uint8	m_uName;
	char*	m_pszName;
	uint32	m_nBlobSize;
	union{
	  wxString*	m_pstrVal;
	  uint32	m_uVal;
	  float		m_fVal;
	  unsigned char*		m_pData;
	};

	//! Needed to be able to return a valid string in case of
	//! GetStr on a non-string tag.
	static wxString s_emptyStr;
};

typedef std::list<CTag*> TagPtrList;

///////////////////////////////////////////////////////////////////////////////
// CTag and tag string helpers

inline int CmpED2KTagName(const char* pszTagName1, const char* pszTagName2){
	// string compare is independant from any codepage and/or LC_CTYPE setting.
	return strcasecmp(pszTagName1, pszTagName2);
}
void ConvertED2KTag(CTag*& pTag);

bool WriteOptED2KUTF8Tag(CFileDataIO* data, const wchar_t* pwsz, uint8 uTagName);

#endif // PACKET_H
