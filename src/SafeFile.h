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

#ifndef SAFEFILE_H
#define SAFEFILE_H

#include "CFile.h"		// Needed for CFile
#include "types.h"		// Needed for LPCSTR
#include "CMemFile.h"		// Needed for CMemFile

class CSafeFile : public CFile{
public:
	CSafeFile();
	CSafeFile::CSafeFile(LPCSTR lpszFileName, unsigned int nOpenFlags);
	unsigned int Read(void* lpBuf, unsigned int nCount);
};

class CSafeMemFile : public CMemFile{ // memFile
public:
	CSafeMemFile(unsigned int nGrowBytes = 0);
	CSafeMemFile::CSafeMemFile(BYTE* lpBuffer, unsigned int nBufferSize, unsigned int nGrowBytes = 0);
/*
  virtual off_t Read(int8& v)             { return CMemFile::Read(v); };
  virtual off_t Read(int16& v)            { return CMemFile::Read(v); };
  virtual off_t Read(int32& v)            { return CMemFile::Read(v); };
  virtual off_t Read(unsigned char v[16]) { return CMemFile::Read(v); };
  virtual off_t Read(wxString& v)         { return CMemFile::Read(v); };

  virtual size_t Write(const int8& v)             { return CMemFile::Write(v); };
  virtual size_t Write(const int16& v)            { return CMemFile::Write(v); };
  virtual size_t Write(const int32& v)            { return CMemFile::Write(v); };
  virtual size_t Write(const unsigned char v[16]) { return CMemFile::Write(v); };
  virtual size_t Write(const wxString& v)         { return CMemFile::Write(v); };
*/	
	off_t ReadRaw(void* lpBuf, unsigned int nCount);
	
protected:
};

// This is just a workaround
class CSafeBufferedFile : public CFile{
public:
	CSafeBufferedFile();
	CSafeBufferedFile::CSafeBufferedFile(LPCSTR lpszFileName,UINT nOpenFlags);
	virtual UINT Read(void* lpBuf,UINT nCount);
};
#endif // SAFEFILE_H
