//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include <wx/dir.h>		// Needed for wxDir
#include <wx/fs_zip.h>		// Needed for wxZipFSHandler
#include <wx/wfstream.h>	// wxFileInputStream
#include <wx/zipstrm.h>		// Needed for wxZipInputStream
#include <wx/zstream.h>		// Needed for wxZlibInputStream


#include <errno.h>
#include <map>
#ifdef __WXMAC__
#include <zlib.h> // Do_not_auto_remove
#endif


#include "FileFunctions.h"
#include "CFile.h"
#include "Logger.h"


bool UTF8_MoveFile(const wxString& from, const wxString& to, bool overwrite)
{
	
	if (overwrite) {
		UTF8_RemoveFile(to);
	}
	
	bool ok = !rename( (const char*)from.mb_str(wxConvLocal), 
						(const char*)to.mb_str(wxConvLocal));

	if (!ok) {
		   AddDebugLogLineM( false, logFileIO,
				  wxT("Error on file move from: ") +
						 from + wxT(" to: ") + to);
	}

	return ok;
}


bool UTF8_RemoveFile(const wxString& fileName)
{
	bool ok = !remove((const char*)fileName.mb_str(wxConvLocal));
	if (!ok) {
		AddDebugLogLineM( false, logFileIO,
			wxT("Error on file remove: ") +
				fileName);
	}

	return ok;
}


bool UTF8_CopyFile(const wxString& from, const wxString& to)
{

	bool ok = false;

	// Dont' remove this scope.
	{
		wxFFileInputStream in(from);
		wxFFileOutputStream out(to);

		ok = in.IsOk() && out.IsOk();
		if (!ok) {
			AddDebugLogLineM( false, logFileIO,
				wxT("Error on file copy from: ") +
					from + wxT(" to: ") + to);
		} else {
			out << in;
			ok &= (out.GetLastError() != wxSTREAM_WRITE_ERROR) && 
				(in.GetLastError() != wxSTREAM_READ_ERROR);
		}
	}
	
	if (!ok) {
		UTF8_RemoveFile(to);
	}

	return ok;
}


sint64 GetFileSize(const wxString& fullPath)
{
	wxFile f(fullPath);
	return f.Length();
}


//
// This class assumes that the following line has been executed:
//
// 	wxConvFileName = &aMuleConvBrokenFileNames;
//
// This line is necessary so that wxWidgets handles unix file names correctly.
//
CDirIterator::CDirIterator(const wxString& dir)
:
wxDir(dir),
m_dir(dir)
{
	if (m_dir.Last() != wxFileName::GetPathSeparator()) {
		m_dir += wxFileName::GetPathSeparator();
	}
}


CDirIterator::~CDirIterator()
{	
}


wxString CDirIterator::GetFirstFile(FileType search_type, const wxString& search_mask)
{
	wxString fileName;
	wxString retFileName;
	bool ok = IsOpened();
	if (!ok) {
		return wxEmptyString;
	}
	ok = GetFirst(&fileName, search_mask, search_type);
	if (!ok) {
		return wxEmptyString;
	}
	// Try to convert it to UTF-8. If it fails, revert to ISO-8859-1.
	retFileName = UTF82unicode(char2UTF8(unicode_2_broken(fileName)));
	if (retFileName.IsEmpty()) {
		retFileName = fileName;
	}
	
	return m_dir + retFileName;
}


wxString CDirIterator::GetNextFile()
{
	wxString fileName;
	wxString retFileName;
	bool ok = GetNext(&fileName);
	if (!ok) {
		return wxEmptyString;
	}
	// Try to convert it to UTF-8. If it fails, revert to ISO-8859-1.
	retFileName = UTF82unicode(char2UTF8(unicode_2_broken(fileName)));
	if (retFileName.IsEmpty()) {
		retFileName = fileName;
	}
	
	return m_dir + retFileName;
}


time_t GetLastModificationTime(const wxString& file)
{
	time_t ret = wxFileModificationTime(file);
	// Notice that wxFileModificationTime() documentation is wrong.
	// This function returns -1 if there was an error, not zero.
	if (ret == -1) {
		AddDebugLogLineM( true, logFileIO,
			wxT("Error on GetLastModificationTime from `") +
			file + wxT("'"));
	}
	
	return ret;
}


bool CheckDirExists(const wxString& dir)
{
#ifdef __WXMSW__
	// stat fails on windows if there are trailing path-separators.
	wxString cleanPath = StripSeparators(dir, wxString::trailing);
	
	// Root paths must end with a separator (X:\ rather than X:).
	// See comments in wxDirExists.
	if ((cleanPath.Length() == 2) and (cleanPath.Last() == wxT(':'))) {
		cleanPath += wxFileName::GetPathSeparator();
	}
#else
	const wxString& cleanPath = dir;
#endif

	return wxDir::Exists(cleanPath);
}


bool CheckFileExists(const wxString& file)
{
	return wxFile::Exists(file);
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
	wxString filename = archive.FindFirst(
		wxT("file:") + file + wxT("#zip:/*"), wxFILE);
	while (!filename.IsEmpty()) {
		// Extract the filename part of the URI
		filename = filename.AfterLast(wxT(':')).Lower();
	
		// We only care about the files specified in the array
		for (size_t i = 0; files[i]; ++i) {
			if (files[i] == filename) {
				std::auto_ptr<wxZipEntry> entry;
				wxFFileInputStream fileInputStream(file);
				wxZipInputStream zip(fileInputStream);
				while (entry.reset(zip.GetNextEntry()), entry.get() != NULL) {
					// access meta-data
					wxString name = entry->GetName();
					// read 'zip' to access the entry's data
					if (name == filename) {
						char buffer[10240];
						wxTempFile target(file);
						while (!zip.Eof()) {
							zip.Read(buffer, sizeof(buffer));
							target.Write(buffer, zip.LastRead());
						}
						target.Commit();
						
						return true;
					}
				}
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
// File_checked_for_headers
