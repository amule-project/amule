// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Team
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

#ifndef CMEMFILE_H
#define CMEMFILE_H

// something that looks like the MFC CMemFile

#include "CFile.h"		// Needed for CFile
#include "types.h"		// Needed for uint8, uint16, uint32
#include "packets.h"

class CMemFile : public CFile {
public:  
	CMemFile(unsigned int growBytes=1024);
	CMemFile(BYTE* buffer,unsigned int bufferSize,unsigned int growBytes=0);
	void Attach(BYTE* buffer,unsigned int buffserSize,unsigned int growBytes=0);
	BYTE* Detach();
	virtual ~CMemFile();
	virtual unsigned long GetPosition() {return fPosition;};
	virtual bool GetStatus(unsigned long none) const {return 1;};
	off_t Seek(off_t offset,wxSeekMode from=wxFromStart);
	virtual void SetLength(unsigned long newLen);
	unsigned long GetLength() { return fFileSize; };
//	virtual void Abort();
//	virtual void Flush();
	virtual bool Close();
	
	// Kry - DANGEROUS! JUST FOR USE ON DEBUG BUILD TO DUMP MEMFILES!
	BYTE* GetCurrentBuffer() { return (fBuffer + fPosition); };

	off_t Read(uint8& v) 	{ return ReadRaw(&v, 1);	}
  	size_t Write(const uint8& v)	{ return WriteRaw(&v, 1);	}
	
	#if wxBYTE_ORDER == wxLITTLE_ENDIAN
 		
  		off_t Read(uint16& v)	{ return ReadRaw(&v, 2);	}
 		off_t Read(uint32& v)	{ return ReadRaw(&v, 4);	}
  		//off_t Read(uint8[16] v)	{ return ReadRaw(v, 16);	}
	
  		size_t Write(const uint16& v)	{ return WriteRaw(&v, 2);	}
		size_t Write(const uint32& v) { return WriteRaw(&v, 4);	}
  		//size_t Write(const uint8[16] v) { return WriteRaw(v, 16);	}
	
	#else 
	
  		virtual off_t Read(uint16& v);
 		virtual off_t Read(uint32& v);
	
  		virtual size_t Write(const uint16& v);
		virtual size_t Write(const uint32& v);
		
	#endif

	virtual off_t Read(uint8[16]);
		
	virtual size_t Write(const uint8[16]);	
		
	size_t Write(const wxString& v) {
		size_t Len = Write((uint16)v.Length());
		return Len + WriteRaw(v.c_str(), v.Length());
	}
	

	off_t Read(wxString& v) {
		uint16 len;
		off_t off = Read(len);
		off += ReadRaw(v.GetWriteBuf(len), len);
		v.UngetWriteBuf(len);
		if (off != (len + 2)) {
			throw CInvalidPacket("short packet reading string");
		}
		return off;
	}	

	virtual off_t  ReadRaw(void* buf,off_t length);
	virtual size_t WriteRaw(const void* buf,size_t length);
	
	

protected:
	virtual off_t  Read(void* buf,off_t length);
	virtual size_t Write(const void* buf,size_t length);
	
private:
	void enlargeBuffer(unsigned long size);
	
	BYTE* fBuffer;  
	unsigned long fLength;
	unsigned int fGrowBytes;
	unsigned long fPosition;
	unsigned long fBufferSize;
	unsigned long fFileSize;
	int deleteBuffer;

};

#endif // CMEMFILE_H
