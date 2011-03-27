//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

// NTFS Sparse Files (only for MSW)
#ifdef __WXMSW__
#include "common/Format.h"
#include "Logger.h"
#include <winbase.h>
#include <winioctl.h>
#ifndef FSCTL_SET_SPARSE
#	define FSCTL_SET_SPARSE		CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif
#ifndef FSCTL_SET_ZERO_DATA
#	define FSCTL_SET_ZERO_DATA	CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA)
#endif

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
		AddDebugLogLineC(logPartFile, CFormat(wxT("converting %s to sparse failed (OPEN): %s ")) % name % SystemError());
		return false;
	}

	if (!DeviceIoControl(hd, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwReturnedBytes, NULL)) {
		AddDebugLogLineC(logPartFile, CFormat(wxT("converting %s to sparse failed (SET_SPARSE): %s ")) % name % SystemError());
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
			AddDebugLogLineC(logPartFile, CFormat(wxT("converting %s to sparse failed (ZERO): %s")) % name % SystemError());
		} else if (!SetFilePointerEx(hd, largo, NULL, FILE_BEGIN) || !SetEndOfFile(hd)) {
			AddDebugLogLineC(logPartFile, CFormat(wxT("converting %s to sparse failed (SEEK): %s")) % name % SystemError());
		}
	}
	CloseHandle(hd);
	return true;
}

#else  // non Windows systems don't need all this
#include "CFile.h"

bool PlatformSpecific::CreateSparseFile(const CPath& name, uint64_t WXUNUSED(size))
{
	CFile f;
	return f.Create(name.GetRaw(), true) && f.Close();
}

#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#include <wx/utils.h>

// Get the max number of connections that the OS supports, or -1 for default
int PlatformSpecific::GetMaxConnections()
{
	int maxconn = -1;
	// Try to get the max connection value in the registry
	wxRegKey key( wxT("HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Services\\VxD\\MSTCP\\MaxConnections") );
	wxString value;
	if ( key.Exists() ) {
		value = key.QueryDefaultValue();
	}
	if ( !value.IsEmpty() && value.IsNumber() ) {
		long mc;
		value.ToLong(&mc);
		maxconn = (int)mc;
	} else {
		switch (wxGetOsVersion()) {
		case wxOS_WINDOWS_9X:
			// This includes all Win9x versions
			maxconn = 50;
			break;
		case wxOS_WINDOWS_NT:
			// This includes NT based windows
			maxconn = 500;
			break;
		default:
			// Anything else. Let aMule decide...
			break;
		}
	}

	return maxconn;
}
#endif


#ifdef __WXMSW__
#include <winbase.h>
#include <shlwapi.h>

static PlatformSpecific::EFSType doGetFilesystemType(const CPath& path)
{
	wxWritableWCharBuffer pathRaw(path.GetRaw());
	LPWSTR volume = pathRaw;
	if (!PathStripToRootW(volume)) {
		return PlatformSpecific::fsOther;
	}
	PathAddBackslashW(volume);

	DWORD maximumComponentLength = 0;
	DWORD filesystemFlags = 0;
	WCHAR filesystemNameBuffer[128];
	if (!GetVolumeInformationW(volume, NULL, 0, NULL, &maximumComponentLength, &filesystemFlags, filesystemNameBuffer, 128)) {
		return PlatformSpecific::fsOther;
	}
	if (wxStrnicmp(filesystemNameBuffer, wxT("FAT"), 3) == 0) {
		return PlatformSpecific::fsFAT;
	} else if (wxStrcmp(filesystemNameBuffer, wxT("NTFS")) == 0) {
		return PlatformSpecific::fsNTFS;
	}
	return PlatformSpecific::fsOther;
};

#elif defined(HAVE_GETMNTENT) && defined(HAVE_MNTENT_H)
#include <stdio.h>
#include <string.h>
#include <mntent.h>
#ifndef _PATH_MOUNTED
#	define _PATH_MOUNTED	"/etc/mtab"
#endif
#include <common/StringFunctions.h>

static PlatformSpecific::EFSType doGetFilesystemType(const CPath& path)
{
	struct mntent *entry = NULL;
	PlatformSpecific::EFSType retval = PlatformSpecific::fsOther;
	FILE *mnttab = fopen(_PATH_MOUNTED, "r");
	unsigned bestPrefixLen = 0;

	if (mnttab == NULL) {
		return PlatformSpecific::fsOther;
	}

	while ((entry = getmntent(mnttab)) != NULL) {
		if (entry->mnt_dir) {
			wxString dir = char2unicode(entry->mnt_dir);
			if (dir == path.GetRaw().Mid(0, dir.Length())) {
				if (dir.Length() >= bestPrefixLen) {
					if (entry->mnt_type == NULL) {
						break;
					} else if (!strcmp(entry->mnt_type, "ntfs")) {
						retval = PlatformSpecific::fsNTFS;
					} else if (!strcmp(entry->mnt_type, "msdos") ||
						   !strcmp(entry->mnt_type, "umsdos") ||
						   !strcmp(entry->mnt_type, "vfat") ||
						   !strncmp(entry->mnt_type, "fat", 3)) {
						retval = PlatformSpecific::fsFAT;
					} else if (!strcmp(entry->mnt_type, "hfs")) {
						retval = PlatformSpecific::fsHFS;
					} else if (!strcmp(entry->mnt_type, "hpfs")) {
						retval = PlatformSpecific::fsHPFS;
					} else if (!strcmp(entry->mnt_type, "minix")) {
						retval = PlatformSpecific::fsMINIX;
					} /* Add more filesystem types here */
					else if (dir.Length() > bestPrefixLen) {
						retval = PlatformSpecific::fsOther;
					}
					bestPrefixLen = dir.Length();
				}
			}
		}
	}
	fclose(mnttab);
	return retval;
}

#elif defined(HAVE_GETMNTENT) && defined(HAVE_SYS_MNTENT_H) && defined(HAVE_SYS_MNTTAB_H)
#include <stdio.h>
#include <string.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#ifndef MNTTAB
#	define MNTTAB	"/etc/mnttab"
#endif
#include <common/StringFunctions.h>

static PlatformSpecific::EFSType doGetFilesystemType(const CPath& path)
{
	struct mnttab entryStatic;
	struct mnttab *entry = &entryStatic;
	PlatformSpecific::EFSType retval = PlatformSpecific::fsOther;
	FILE *fmnttab = fopen(MNTTAB, "r");
	unsigned bestPrefixLen = 0;

	if (fmnttab == NULL) {
		return PlatformSpecific::fsOther;
	}

	while (getmntent(fmnttab, entry) == 0) {
		if (entry->mnt_mountp) {
			wxString dir = char2unicode(entry->mnt_mountp);
			if (dir == path.GetRaw().Mid(0, dir.Length())) {
				if (dir.Length() >= bestPrefixLen) {
					if (entry->mnt_fstype == NULL) {
						break;
					} else if (!strcmp(entry->mnt_fstype, MNTTYPE_PCFS)) {
						retval = PlatformSpecific::fsFAT;
					} else if (hasmntopt(entry, MNTOPT_NOLARGEFILES)) {
						// MINIX is a file system that can handle special chars but has no large files.
						retval = PlatformSpecific::fsMINIX;
					} else if (dir.Length() > bestPrefixLen) {
						retval = PlatformSpecific::fsOther;
					}
					bestPrefixLen = dir.Length();
				}
			}
		}
	}
	fclose(fmnttab);
	return retval;
}

#else

// No way to determine filesystem type, no restrictions apply.
static inline PlatformSpecific::EFSType doGetFilesystemType(const CPath& WXUNUSED(path))
{
	return PlatformSpecific::fsOther;
}

#endif

#include <map>
#include <wx/thread.h>

PlatformSpecific::EFSType PlatformSpecific::GetFilesystemType(const CPath& path)
{
	typedef std::map<wxString, EFSType>	FSMap;
	// Caching previous results, to speed up further checks.
	static FSMap	s_fscache;
	// Lock used to ensure the integrity of the cache.
	static wxMutex	s_lock;

	wxCHECK_MSG(path.IsOk(), fsOther, wxT("Invalid path in GetFilesystemType()"));

	wxMutexLocker locker(s_lock);

	FSMap::iterator it = s_fscache.find(path.GetRaw());
	if (it != s_fscache.end()) {
		return it->second;
	}

	return s_fscache[path.GetRaw()] = doGetFilesystemType(path);
}


// Power event vetoing

static bool m_preventingSleepMode = false;

#if defined(__WXMAC__) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050	// 10.5 only
	#include <IOKit/pwr_mgt/IOPMLib.h>
	static IOPMAssertionID assertionID;
#endif

void PlatformSpecific::PreventSleepMode()
{
	if (!m_preventingSleepMode) {
		#ifdef _MSC_VER
			SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
			m_preventingSleepMode = true;
		#elif defined(__WXMAC__) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050	// 10.5 only
			IOReturn success = IOPMAssertionCreate(kIOPMAssertionTypeNoDisplaySleep, 
												kIOPMAssertionLevelOn, &assertionID); 
			if (success == kIOReturnSuccess) {
				// Correctly vetoed, flag so we don't do it again.
				m_preventingSleepMode = true;
			} else {
				// ??
			}
		#else
			#warning Power event vetoing not implemented.
			// Not implemented	
		#endif
	}
}

void PlatformSpecific::AllowSleepMode()
{
	if (m_preventingSleepMode) {
		#ifdef _MSC_VER
			SetThreadExecutionState(ES_CONTINUOUS); // Clear the system request flag.
			m_preventingSleepMode = false;
		#elif defined(__WXMAC__) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 1050	// 10.5 only
			IOReturn success = IOPMAssertionRelease(assertionID); 
			if (success == kIOReturnSuccess) {
				// Correctly restored, flag so we don't do it again.
				m_preventingSleepMode = false;
			} else {
				// ??
			}
		#else
			// Not implemented
		#endif
	}
}
