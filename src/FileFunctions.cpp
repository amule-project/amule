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

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "FileFunctions.h"
#endif

#include <wx/filename.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "FileFunctions.h"
#include "StringFunctions.h"
#include "CFile.h"
#include "Logger.h"


int UTF8_Stat(const wxString& file_name, struct stat *buf)
{
	Unicode2CharBuf tmpFile(unicode2char(file_name));
	
	int stat_error = -1;
	
	if (tmpFile) {
		stat_error = stat(tmpFile, buf);
	}
	
	if (stat_error) {
		stat_error = stat(unicode2UTF8(file_name), buf);
	}

	return stat_error;
}


//
// When moving file, first try an ANSI move, only then try UTF-8.
// 
bool UTF8_MoveFile(const wxString& from, const wxString& to)
{
	bool ret = false;
	Unicode2CharBuf tmpFrom(unicode2char(from));
	Unicode2CharBuf tmpTo(unicode2char(to));
	if (tmpFrom) {
		if (tmpTo) {
			ret = rename(tmpFrom, tmpTo) == 0;
		} else {
			ret = rename(tmpFrom, unicode2UTF8(to)) == 0;
		}
	} else {
		if (tmpTo) {
			ret = rename(unicode2UTF8(from), tmpTo) == 0;
		} else {
			ret = rename(unicode2UTF8(from), unicode2UTF8(to)) == 0;
		}
	}

	return ret;
}


#define FILE_COPY_BUFFER 5*1024

//
// When copying file, first try an ANSI name, only then try UTF-8.
// This is done in the CFile constructor.
// 
bool UTF8_CopyFile(const wxString& from, const wxString& to)
{
	char buffer[FILE_COPY_BUFFER];
	CFile input_file(from, CFile::read);
	if (!input_file.IsOpened()) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't open original file: ") + from );
		return false;
	}
	CFile output_file(to, CFile::write);
	if (!output_file.IsOpened()) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't create destination file: ") + to );
		return false;
	}
	
	int total_read, total_write;
	while ((total_read = input_file.Read(buffer,FILE_COPY_BUFFER))) {
		if (total_read == -1) {
			AddDebugLogLineM( true, logFileIO, wxT("Unexpected error copying file! (read error)") );
			return false;
		}
		total_write = output_file.Write(buffer,total_read);
		if (total_write != total_read) {
			AddDebugLogLineM( true, logFileIO, wxT("Unexpected error copying file! (write error)") );
			return false;			
		}	
	}
	
	return true;
}


off_t GetFileSize(const wxString& fullPath)
{
	struct stat buf;
	
	if (!UTF8_Stat(fullPath, &buf)) {
		return buf.st_size;
	}

	return -1;
}


// When iterating dir, first try an ANSI file name, then try an UTF-8 file name.
CDirIterator::CDirIterator(const wxString& dir)
{
	DirStr = dir;
	if (DirStr.Last() != wxFileName::GetPathSeparator()) {
		DirStr += wxFileName::GetPathSeparator();
	}
	
	DirPtr = NULL;
	
	Unicode2CharBuf tmpDir(unicode2char(dir));
	if (tmpDir) {
		DirPtr = opendir(tmpDir);
	}
	
	if (DirPtr == NULL) { // Wrong conversion or error opening
		// Try UTF8
		DirPtr = opendir(unicode2UTF8(dir));
	}
	
	if (!DirPtr) {
		AddDebugLogLineM( false, logFileIO, wxT("Error enumerating files for dir ") + dir + wxT(" (permissions?)") );
	}
}


CDirIterator::~CDirIterator()
{	
	if (DirPtr) {
		closedir (DirPtr);
	}
}


wxString CDirIterator::GetFirstFile(FileType search_type, const wxString& search_mask)
{
	if (!DirPtr) {
		return wxEmptyString;
	}
	seekdir(DirPtr, 0);// 2 if we want to skip . and ..
	FileMask = search_mask;
	type = search_type;
	return GetNextFile();
}


// First try an ANSI name, only then try UTF-8.
wxString  CDirIterator::GetNextFile()
{
	if (!DirPtr) {
		return wxEmptyString;
	}
	struct dirent *dp;
	dp = readdir(DirPtr);
	
	bool found = false;
	wxString FoundName;
	struct stat* buf = (struct stat*)malloc(sizeof(struct stat));
	while (dp!=NULL && !found) {
		if (type == CDirIterator::Any) {
			// return anything.
			found = true;
		} else {
#if 0
			switch (dp->d_type) {
			case DT_DIR:
				if (type == CDirIterator::Dir)  {
					found = true;
				} else {
					dp = readdir(DirPtr);	
				}
				break;
			case DT_REG:
				if (type == CDirIterator::File)  {
					found = true;
				} else {
					dp = readdir(DirPtr);					
				}
				break;
			default:
#endif
				// Fallback to stat
				//
				// The file name came from the OS, it is a sequence of
				// bytes ending in a zero. First try an UTF-8 conversion,
				// so that we don't loose information. Only then stick
				// to an ANSI name. UTF82Unicode might fail because
				// dp->name may not be a valid UTF-8 sequence.
				Char2UnicodeBuf tmpFoundName(UTF82unicode(dp->d_name));
				FoundName = tmpFoundName ?
					tmpFoundName : char2unicode(dp->d_name);
				wxString FullName(DirStr + FoundName);
				// First, we try to use an ANSI name, but it might not be
				// possible to use ANSI for the full name, so we test.
				Unicode2CharBuf tmpFullName(unicode2char(FullName));		
				int stat_error = -1;
				if (tmpFullName) {
					stat_error = stat(tmpFullName, buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpFullName, buf);
						if (!stat_error && S_ISLNK(buf->st_mode)) {
							// Ok, just a broken symlink. Next, please!
							dp = readdir(DirPtr);
							continue;
						}
					}
#endif
				}
				// Fallback to UTF-8
				if (stat_error) {
					Unicode2CharBuf tmpUTF8FullName(unicode2UTF8(FullName));
					stat_error = stat(tmpUTF8FullName, buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpUTF8FullName, buf);
						if (!stat_error && S_ISLNK(buf->st_mode)) {
							// Ok, just a broken symlink. Next, please!
							dp = readdir(DirPtr);
							continue;
						}
					}
#endif
				}
				
				if (!stat_error) {
					if (S_ISREG(buf->st_mode)) {
						if (type == CDirIterator::File) { 
							found = true; 
						} else { 
							dp = readdir(DirPtr);
						} 
					} else {
						if (S_ISDIR(buf->st_mode)) {
							if (type == CDirIterator::Dir) {
								found = true; 
							} else { 
								dp = readdir(DirPtr);
							}
						} else {				
							// unix socket, block device, etc
							dp = readdir(DirPtr);
						}
					}
				} else {
					// Stat failed. Assert.
					printf("CFile: serious error, stat failed\n");
					//wxASSERT(0);
					AddDebugLogLineM( true, logFileIO,
						wxT("Unexpected error calling stat on a file: ") + FullName);
					dp = readdir(DirPtr);
				}
#if 0
				break;
			}
#endif
		}
		if (found) {
			if (	(!FileMask.IsEmpty() && !FoundName.Matches(FileMask)) ||
				FoundName.IsSameAs(wxT(".")) ||
				FoundName.IsSameAs(wxT(".."))) {
				found = false;	
				dp = readdir(DirPtr);
			}
		}
	}
	free(buf);
	if (dp != NULL) {
		return DirStr + FoundName;	
	} else {
		return wxEmptyString;
	}
}


// First try an ANSI name, only then try UTF-8.
time_t GetLastModificationTime(const wxString& file)
{
	struct stat buf;
		
	int stat_error = UTF8_Stat(file, &buf);
	
	if (stat_error) {
		return 0;
	} else {
		return buf.st_mtime;
	}
}


bool CheckDirExists(const wxString& dir)
{
	struct stat st;
	return (UTF8_Stat(dir, &st) == 0 && ((st.st_mode & S_IFMT) == S_IFDIR));
}


bool BackupFile(const wxString& filename, const wxString& appendix)
{
	if ( !UTF8_CopyFile(filename, filename + appendix) ) {
		AddDebugLogLineM( false, logFileIO, wxT("Could not create backup of ") + filename);
		return false;
	}
	
	// Kry - Safe Backup
	CFile safebackupfile;
	safebackupfile.Open(filename + appendix,CFile::read_write);
	safebackupfile.Flush();
	safebackupfile.Close();
	// Kry - Safe backup end
	
	return true;
}

