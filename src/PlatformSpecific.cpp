//
// This file is part of the aMule Project.
//
// Copyright (c) 2008 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "PlatformSpecific.h"


// NTFS Sparse Files (only for MINGW)
#ifdef __MINGW32__
#	include "common/Format.h"
#	include "Logger.h"
#	include <winbase.h>
#	include <winioctl.h>
#	define FSCTL_SET_ZERO_DATA		CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA)

// Create a message from a Windows error code
static wxString SystemError()
{
	WCHAR * lpMsgBuf = NULL;

	FormatMessageW( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		0, // Default language
		(LPWSTR) &lpMsgBuf,
		0,
		NULL 
	);

	wxString ret(lpMsgBuf);
	LocalFree(lpMsgBuf);
	return ret;
} 

// Create a file in sparse mode
bool PlatformSpecific::CreateSparseFile(const CPath& name, uint64_t size)
{
	DWORD dwReturnedBytes=0;

	HANDLE hd = CreateFileW(name.GetRaw().c_str(),
		GENERIC_READ | GENERIC_WRITE, 
		0,       // share - not shareable
		NULL,    // security - not inheritable
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_ARCHIVE, 
		NULL);
	if (hd == INVALID_HANDLE_VALUE) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("converting %s to sparse failed (OPEN): %s ")) % name % SystemError());
		return false;
	}

	if (!DeviceIoControl(hd, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwReturnedBytes, NULL)) {
		AddDebugLogLineM(true, logPartFile, CFormat(wxT("converting %s to sparse failed (SET_SPARSE): %s ")) % name % SystemError());
	} else {
		// FILE_ZERO_DATA_INFORMATION is not defined here
		struct {
			uint64 FileOffset;
			uint64 BeyondFinalZero;
		} fzdi;
		fzdi.FileOffset = 0;
		fzdi.BeyondFinalZero = size;
		LARGE_INTEGER largo;
		largo.QuadPart = size;
		
		// zero the data
		if (!DeviceIoControl(hd, FSCTL_SET_ZERO_DATA, (LPVOID) &fzdi, sizeof(fzdi), NULL, 0, &dwReturnedBytes, NULL)) {
			AddDebugLogLineM(true, logPartFile, CFormat(wxT("converting %s to sparse failed (ZERO): %s")) % name % SystemError());
		} else if (!SetFilePointerEx(hd, largo, NULL, FILE_BEGIN) || !SetEndOfFile(hd)) {
			AddDebugLogLineM(true, logPartFile, CFormat(wxT("converting %s to sparse failed (SEEK): %s")) % name % SystemError());
		}
	}
	CloseHandle(hd);
	return true;
}

#else  // non Windows systems don't need all this
#	include "CFile.h"

bool PlatformSpecific::CreateSparseFile(const CPath& name, uint64_t WXUNUSED(size))
{
	CFile f;
	return f.Create(name.GetRaw(), true) && f.Close();
}

#endif
