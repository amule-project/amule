//this file is part of aMule
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


#include "SafeFile.h"		// Interface declarations.

CSafeFile::CSafeFile(LPCSTR lpszFileName,UINT nOpenFlags)
:CFile(lpszFileName,(CFile::OpenMode)nOpenFlags)
//:CFile(lpszFileName,nOpenFlags)
{
	// Nothing to do here
}

CSafeFile::CSafeFile() : CFile() {}


UINT CSafeFile::Read(void* lpBuf,UINT nCount) {
	if (Tell()+nCount > Length()) {
//		AfxThrowFileException(CFileException::endOfFile,0,GetFileName());
		return (UINT)-1;
	}
	return CFile::Read(lpBuf,nCount);
}


CSafeMemFile::CSafeMemFile(BYTE* lpBuffer,UINT nBufferSize,UINT nGrowBytes)
:CMemFile(lpBuffer,nBufferSize,nGrowBytes)
{
	// Nothing to do here
}

CSafeMemFile::CSafeMemFile(UINT nGrowBytes) : CMemFile(nGrowBytes) {}

off_t CSafeMemFile::ReadRaw(void* lpBuf,UINT nCount) {
	if (GetPosition()+nCount > this->GetLength()) {
//		AfxThrowFileException(CFileException::endOfFile,0,GetFileName());
		return 0;
	}
	return CMemFile::ReadRaw(lpBuf,nCount);
}

 
//Kry - This is just a workaround for Buffered files
 
CSafeBufferedFile::CSafeBufferedFile(LPCSTR lpszFileName,UINT nOpenFlags)
	:CFile(lpszFileName,(CFile::OpenMode)nOpenFlags)
{}

CSafeBufferedFile::CSafeBufferedFile() : CFile() {}
 
 	
UINT CSafeBufferedFile::Read(void* lpBuf,UINT nCount){
// that's terrible slow
//	if (GetPosition()+nCount > this->GetLength())
//		AfxThrowFileException(CFileException::endOfFile,0,GetFileName());

	off_t uRead = CFile::Read(lpBuf,nCount);
	if (uRead != nCount) {		
		//AfxThrowFileException(CFileException::endOfFile,0,GetFileName());
		return (off_t)-1;
	}
	return uRead;
}
