// This file is part of the aMule project.
//
// Copyright (c) 2003 aMule Project ( http://www.amule-project.net )
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

#include "CMemFile.h"		// Needed for CMemFile
#include "otherfunctions.h" // Needed for ENDIAN_SWAP_xx

CMemFile::CMemFile(unsigned int growBytes)
{
  fGrowBytes=growBytes;
  fLength=0;
  fPosition=0;
  fBufferSize=0;
  fFileSize=0;
  fBuffer=NULL;
  deleteBuffer=TRUE;
}

CMemFile::CMemFile(BYTE* buffer,unsigned int bufferSize,unsigned int growBytes)
{
  fBufferSize=bufferSize;
  fPosition=0;
  fGrowBytes=growBytes;
  if(!growBytes)fFileSize=bufferSize; else fFileSize=0;
  fBuffer=buffer; // uh
  deleteBuffer=FALSE;
}

void CMemFile::Attach(BYTE* buffer,unsigned int bufferSize,unsigned int growBytes)
{
  fBufferSize=bufferSize;
  fPosition=0;
  fGrowBytes=growBytes;
  if(!growBytes)fFileSize=bufferSize; else fFileSize=0;
  fBuffer=buffer; // uh
  deleteBuffer=FALSE;
}

BYTE* CMemFile::Detach()
{
  BYTE* retval=fBuffer;
  fBuffer=NULL;
  fFileSize=fBufferSize=fPosition=fFileSize=0;
  return retval;
}

CMemFile::~CMemFile()
{
  fGrowBytes=fPosition=fBufferSize=fFileSize=0;
  // should the buffer be free'd ?
  if(fBuffer && deleteBuffer) free(fBuffer);
  fBuffer=NULL;
}


off_t CMemFile::Seek(off_t offset,wxSeekMode from)
{
  off_t newpos=0;
  switch(from) {
  case wxFromStart:
    newpos=offset;
    break;
  case wxFromCurrent:
    newpos=fPosition+offset;
    break;
  case wxFromEnd:
    newpos=fFileSize-offset;
    break;
  default:
    return -1;
  }
  if(newpos<0) {
    return -1;
  }

  // what if we seek over the end??
  fPosition=newpos;
  return fPosition;
}

void CMemFile::enlargeBuffer(unsigned long size)
{
  unsigned long newsize=fBufferSize;

  // hmm.. mit�h�n jos growbytes==0??
  // Kry - I can't understand you, traveler from far away,
  // but I'm kinnda sure of what you mean. Answer is: it hangs.
  while(newsize<size)
    newsize+=fGrowBytes;

  if(fBuffer)
    fBuffer=(BYTE*)realloc((void*)fBuffer,newsize);
  else fBuffer=(BYTE*)malloc(newsize);

  if(fBuffer==NULL) {
    // jaa-a. mit�h�n tekis
    printf("out of memory experience\n");
    exit(1);
  }

  fBufferSize=newsize;
}

void CMemFile::SetLength(unsigned long newLen)
{
  if(newLen>fBufferSize) {
    // enlarge buffer
    enlargeBuffer(newLen);
  }
  if(newLen<fPosition) {
    fPosition=newLen;
  }
  fFileSize=newLen;
}

off_t CMemFile::ReadRaw(void* buf,off_t length)
{
  if(length==0) 
    return 0;
  // dont' read over buffer end
  //printf("fPos %i, fFSize %i\n",fPosition,fFileSize);
  if (fPosition>fFileSize) {
	  	//printf("Read Over Buffer End!!!!\n");
		throw CInvalidPacket("Read over buffer end on MemFile (corrupted tag?)");
	  	return 0;
  }
  unsigned int readlen=length;
  if(length+fPosition>fFileSize) {
    //printf("Read After End Of File!!!!\n");
    //wxASSERT(0);
    throw CInvalidPacket("Read after End Of MemFile (corrupted tag?)");
    readlen=fFileSize-fPosition;
  }

  memcpy(buf,fBuffer+fPosition,readlen);
  fPosition+=readlen;
  return readlen;
}

size_t CMemFile::WriteRaw(const void* buf,size_t length)
{
  if(length==0)
    return 0;
  // need more space?
  if(fPosition+length>fBufferSize)
    enlargeBuffer(fPosition+length);
  memcpy(fBuffer+fPosition,buf,length);
  fPosition+=length;
  if(fPosition>fFileSize)
    fFileSize=fPosition;
  return length;
}

bool CMemFile::Close()
{
  // do-nothing :)
  return TRUE;
}

#if wxBYTE_ORDER == wxBIG_ENDIAN


inline off_t CMemFile::Read(uint16& v)
{
	off_t off = ReadRaw(&v, 2);
	ENDIAN_SWAP_I_16(v);
	return off;
}

inline off_t CMemFile::Read(uint32& v)
{
	off_t off = ReadRaw(&v, 4);
	ENDIAN_SWAP_I_32(v);
	return off;
}

#endif

inline off_t CMemFile::Read(uint8 v[16])
{
	return ReadRaw(v, 16);
}

inline off_t CMemFile::Read(void* buf,off_t length)
{
	return ReadRaw(buf, length);
}


#if wxBYTE_ORDER == wxBIG_ENDIAN

inline size_t CMemFile::Write(const uint16& v)
{
	int16 tmp = ENDIAN_SWAP_16(v);
	return WriteRaw(&tmp, 2);
}
	
inline size_t CMemFile::Write(const uint32& v)
{
	int32 tmp = ENDIAN_SWAP_32(v);
	return WriteRaw(&tmp, 4);
}

#endif

inline size_t CMemFile::Write(const uint8 v[16])
{
	return WriteRaw(v, 16);
}

inline size_t CMemFile::Write(const void* buf,size_t length)
{
	return WriteRaw(buf, length);
}
