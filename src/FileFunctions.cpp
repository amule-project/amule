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

#include <wx/filename.h>
#include <wx/zipstrm.h>		// Needed for wxZipInputStream
#include <wx/zstream.h>		// Needed for wxZlibInputStream
#include <wx/wfstream.h>	// wxFileInputStream
#include <wx/fs_zip.h>		// Needed for wxZipFSHandler
#include <wx/file.h>		// Needed for wxFile
#include <wx/thread.h>		// Needed for wxMutex

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <cctype>
#include <map>
#ifdef __WXMAC__
#include <zlib.h>
#endif

#include "FileFunctions.h"
#include <common/StringFunctions.h>
#include "CFile.h"
#include "Logger.h"


#ifdef __WSMSW__
	#define STAT_FUNC _stat64
	#define STAT_STRUCT struct __stat64
#else
	#define STAT_FUNC stat
	#define STAT_STRUCT struct stat
#endif


int UTF8_Stat(const wxString& file_name, STAT_STRUCT *buf)
{
	Unicode2CharBuf tmpFile(unicode2char(file_name));
	
	int stat_error = -1;
	
	if (tmpFile) {
		stat_error = STAT_FUNC(tmpFile, buf);
	}
	
	if (stat_error) {
		stat_error = STAT_FUNC(unicode2UTF8(file_name), buf);
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


bool UTF8_RemoveFile(const wxString& fileName)
{
	// Test if it is possible to use an ANSI name
	Unicode2CharBuf tmpFileName = unicode2char(fileName);
	if (tmpFileName and (unlink(tmpFileName) != -1)) {
		return true;
	} 
	
	// Try an UTF-8 name
	return (unlink(unicode2UTF8(fileName)) != -1);
}


const unsigned int FILE_COPY_BUFFER = 5*1024;

//
// When copying file, first try an ANSI name, only then try UTF-8.
// This is done in the CFile constructor.
// 
bool UTF8_CopyFile(const wxString& from, const wxString& to)
{
	// Get file permissions
	STAT_STRUCT fileStats;
	if (UTF8_Stat(from, &fileStats)) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't stat original file: ") + from );
	}
	
	char buffer[FILE_COPY_BUFFER];
	CFile input_file(from, CFile::read);
	if (!input_file.IsOpened()) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't open original file: ") + from );
		return false;
	}
	
	CFile output_file;
	if (!output_file.Open(to, CFile::write, fileStats.st_mode & 0777)) {
		AddDebugLogLineM( true, logFileIO, wxT("Error on file copy. Can't create destination file: ") + to );
		return false;
	}
	
	while (!input_file.Eof()) {
		uint64 toReadWrite = std::min<uint64>(sizeof(buffer), input_file.GetLength() - input_file.GetPosition());
		
		try {
			input_file.Read(buffer, toReadWrite);
			output_file.Write(buffer, toReadWrite);
		} catch (const CIOFailureException& e) {
			AddDebugLogLineM(true, logFileIO, wxT("IO failure while copying file '") + from + wxT("' to '") + to + wxT("': ") + e.what());

			output_file.Close();
			if (not UTF8_RemoveFile(to)) {
				AddDebugLogLineM(true, logFileIO, wxT("Failed to remove incomplete copy of file '") + from + wxT("' at '") + to + wxT("'"));
			}
			
			return false;
		}
	}

	return true;
}


sint64 GetFileSize(const wxString& fullPath)
{
	STAT_STRUCT buf;
	
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
	rewinddir(DirPtr);
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
	STAT_STRUCT buf;
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
					stat_error = STAT_FUNC(tmpFullName, &buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpFullName, &buf);
						if (!stat_error && S_ISLNK(buf.st_mode)) {
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
					stat_error = STAT_FUNC(tmpUTF8FullName, &buf);
#ifndef __WXMSW__
					// Check if it is a broken symlink
					if (stat_error) {
						stat_error = lstat(tmpUTF8FullName, &buf);
						if (!stat_error && S_ISLNK(buf.st_mode)) {
							// Ok, just a broken symlink. Next, please!
							dp = readdir(DirPtr);
							continue;
						}
					}
#endif
				}
				
				if (!stat_error) {
					if (S_ISREG(buf.st_mode)) {
						if (type == CDirIterator::File) { 
							found = true; 
						} else { 
							dp = readdir(DirPtr);
						} 
					} else {
						if (S_ISDIR(buf.st_mode)) {
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
	if (dp != NULL) {
		return DirStr + FoundName;	
	} else {
		return wxEmptyString;
	}
}


// First try an ANSI name, only then try UTF-8.
time_t GetLastModificationTime(const wxString& file)
{
	STAT_STRUCT buf;
		
	int stat_error = UTF8_Stat(file, &buf);
	
	if (stat_error) {
		return 0;
	} else {
		return buf.st_mtime;
	}
}


bool CheckDirExists(const wxString& dir)
{
#ifdef __WXMSW__
	// UTF8_Stat fails on windows if there are trailing path-seperators.
	wxString cleanPath = StripSeperators(dir, wxString::trailing);
	
	// Root paths must end with a seperator (X:\ rather than X:).
	// See comments in wxDirExists.
	if ((cleanPath.Length() == 2) and (cleanPath.Last() == wxT(':'))) {
		cleanPath += wxFileName::GetPathSeparator();
	}
#else
	const wxString& cleanPath = dir;
#endif
	
	if (not cleanPath.IsEmpty()) {
		STAT_STRUCT st;
		if (UTF8_Stat(cleanPath, &st) == 0) {
			return ((st.st_mode & S_IFMT) == S_IFDIR);
		}
	}
	
	return false;
}


bool CheckFileExists(const wxString& file)
{
	STAT_STRUCT st;
	return (UTF8_Stat(file, &st) == 0 && ((st.st_mode & S_IFMT) == S_IFREG));
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


/**
 * Returns true if the file is a zip-archive.
 */
EFileType GuessFiletype(const wxString& file)
{
	wxFile archive(file, wxFile::read);
	char head[10] = {0};
	int read = archive.Read(head, std::min<off_t>(10, archive.Length()));

	if (read == wxInvalidOffset) {
		return EFT_Unknown;
	} else if ((head[0] == 'P') && (head[1] == 'K')) {
		// Zip-archives have a header of "PK".
		return EFT_Zip;
	} else if (head[0] == (char)0x1F && head[1] == (char)0x8B) {
		// Gzip-archives have a header of 0x1F8B
		return EFT_GZip;
	} else if (head[0] == (char)0xE0 || head[0] == (char)0x0E) {
		// MET files have either of these headers
		return EFT_Met;
	}

	// Check at most the first ten chars, if all are printable, 
	// then we can probably assume it is ascii text-file.
	for (int i = 0; i < read; ++i) {
		if (!isprint(head[i]) && !isspace(head[i])) {
			return EFT_Unknown;
		}
	}
	
	return EFT_Text;
}


/**
 * Replaces the zip-archive with "guarding.p2p" or "ipfilter.dat",
 * if either of those files are found in the archive.
 */
bool UnpackZipFile(const wxString& file, const wxChar* files[])
{
	wxZipFSHandler archive; 
	wxString filename = archive.FindFirst(file + wxT("#file:/*"), wxFILE);

	while (!filename.IsEmpty()) {
		// Extract the filename part of the URI
		filename = filename.AfterLast(wxT(':')).Lower();
	
		// We only care about the files specified in the array
		for (size_t i = 0; files[i]; ++i) {
			if (files[i] == filename) {
				wxZipInputStream inputStream(file, filename);
			
				char buffer[10240];
				wxTempFile target(file);
				
				while (!inputStream.Eof()) {
					inputStream.Read(buffer, sizeof(buffer));

					target.Write(buffer, inputStream.LastRead());
				}
				
				target.Commit();
				
				return true;
			}
		}
		
		filename = archive.FindNext();
	}

	return false;
}


/**
 * Unpacks a GZip file and replaces the archive.
 */
bool UnpackGZipFile(const wxString& file)
{
	char buffer[10240];
	wxTempFile target(file);

	bool write = false;

#ifdef __WXMAC__
	AddDebugLogLineM( false, logFileIO, wxT("Reading gzip stream") );

	gzFile inputFile = gzopen(unicode2char(file), "rb");
	if (inputFile != NULL) {
		write = true;

		while (int bytesRead = gzread(inputFile, buffer, sizeof(buffer))) {
			if (bytesRead > 0) {
				AddDebugLogLineM( false, logFileIO, wxString::Format(wxT("Read %u bytes"), bytesRead) );
				target.Write(buffer, bytesRead);
			} else if (bytesRead < 0) {
				wxString errString;
				int gzerrnum;
				const char* gzerrstr = gzerror(inputFile, &gzerrnum);
				if (gzerrnum == Z_ERRNO) {
					errString = wxSysErrorMsg();
				} else {
					errString = wxString::FromAscii(gzerrstr);
				}
				AddDebugLogLineM( false, logFileIO, wxT("Error reading gzip stream (") + errString + wxT(")") );
				write = false;
				break;
			}
		}

		AddDebugLogLineM( false, logFileIO, wxT("End reading gzip stream") );
		gzclose(inputFile);
	} else {
		AddDebugLogLineM( false, logFileIO, wxT("Error opening gzip file (") + wxString(wxSysErrorMsg()) + wxT(")") );
	}
#else
	{
		AddDebugLogLineM( false, logFileIO, wxT("Reading gzip stream") );

		wxFileInputStream source(file);
		wxZlibInputStream inputStream(source);

		while (!inputStream.Eof()) {
			inputStream.Read(buffer, sizeof(buffer));

			AddDebugLogLineM( false, logFileIO, wxString::Format(wxT("Read %u bytes"),inputStream.LastRead()) );
			if (inputStream.LastRead()) {
				target.Write(buffer, inputStream.LastRead());
			} else {
				break;
			}
		};

		AddDebugLogLineM( false, logFileIO, wxT("End reading gzip stream") );

		write = inputStream.IsOk() || inputStream.Eof();
	}
#endif

	if (write) {
		target.Commit();
		AddDebugLogLineM( false, logFileIO, wxT("Commited gzip stream") );
	}

	return write;
}


UnpackResult UnpackArchive(const wxString& file, const wxChar* files[])
{
	// Attempt to discover the filetype and uncompress
	EFileType type = GuessFiletype(file);
	switch (type) {
		case EFT_Zip:
			if (UnpackZipFile(file, files)) {
				// Unpack nested archives if needed.
				return UnpackResult(true, UnpackArchive(file, files).second);
			} else {
				return UnpackResult(false, EFT_Zip);
			}

		case EFT_GZip:
			if (UnpackGZipFile(file)) {
				// Unpack nested archives if needed.
				return UnpackResult(true, UnpackArchive(file, files).second);
			} else {
				return UnpackResult(false, EFT_GZip);
			}

		default:
			return UnpackResult(false, type);
	}
}


#ifdef __WXMSW__

FSCheckResult CheckFileSystem(const wxString& path) 
{
	return FS_IsFAT32;
}

#else

FSCheckResult DoCheckFileSystem(const wxString& path)
{
	// This is an invalid filename on FAT32
	wxString fullName = JoinPaths(path, wxT(":"));

	// Try to open the file, without overwriting existing files.
	int fd = open(fullName.fn_str(), O_WRONLY | O_CREAT | O_EXCL);
	if (fd != -1) {
		// Success, the file-system cant be FAT32
		close(fd);
		unlink(fullName.fn_str());
		
		return FS_NotFAT32;
	}

	switch (errno) {
		case EINVAL:
			// File-name was invalid, file-system is FAT32
			return FS_IsFAT32;
			
		case EEXIST:
			// File already exists, file-system cant be FAT32
			return FS_NotFAT32;

		default:
			// Something else failed, couldn't check
			return FS_Failed;
	}
}


typedef std::map<wxString, FSCheckResult> CPathCache;

//! Lock used to ensure the integrity of the cache.
static wxMutex		s_lock;
//! Cache of results from checking various paths.
static CPathCache	s_cache;


FSCheckResult CheckFileSystem(const wxString& path) 
{
	wxCHECK_MSG(path.Length(), FS_Failed, wxT("Invalid path in CheckFileSystem!"));

	wxMutexLocker locker(s_lock);
	CPathCache::iterator it = s_cache.find(path);
	if (it != s_cache.end()) {
		return it->second;
	}

	return s_cache[path] = DoCheckFileSystem(path);
}

#endif
