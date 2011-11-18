//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include <wx/log.h>		// Needed for wxSysErrorMsg

#ifdef __WXMAC__
#include <zlib.h> // Do_not_auto_remove
#endif
#include <memory>		// Needed for std::auto_ptr

#include "FileFunctions.h"
#include "StringFunctions.h"

//
// This class assumes that the following line has been executed:
//
// 	wxConvFileName = &aMuleConvBrokenFileNames;
//
// This line is necessary so that wxWidgets handles unix file names correctly.
//
CDirIterator::CDirIterator(const CPath& dir)
	: wxDir(dir.GetRaw())
{
}


CDirIterator::~CDirIterator()
{	
}


CPath CDirIterator::GetFirstFile(FileType type, const wxString& mask)
{
	if (!IsOpened()) {
		return CPath();
	}
	
	wxString fileName;
	if (!GetFirst(&fileName, mask, type)) {
		return CPath();
	}

	return CPath(fileName);
}


CPath CDirIterator::GetNextFile()
{
	wxString fileName;
	if (!GetNext(&fileName)) {
		return CPath();
	}

	return CPath(fileName);
}


bool CDirIterator::HasSubDirs(const wxString& spec)
{
	// Checking IsOpened() in case we don't have permissions to read that dir.
	return IsOpened() && wxDir::HasSubDirs(spec);
}


EFileType GuessFiletype(const wxString& file)
{
	wxFile archive(file, wxFile::read);
	if (!archive.IsOpened()) {
		return EFT_Error;
	}
	static const uint8 UTF8bom[3] = {0xEF, 0xBB, 0xBF};
	uint8 head[10] = {0, 0};
	int read = archive.Read(head, std::min<off_t>(10, archive.Length()));

	if (read == wxInvalidOffset || read == 0) {
		return EFT_Unknown;
	} else if ((head[0] == 'P') && (head[1] == 'K')) {
		// Zip-archives have a header of "PK".
		return EFT_Zip;
	} else if (head[0] == 0x1F && head[1] == 0x8B) {
		// Gzip-archives have a header of 0x1F8B
		return EFT_GZip;
	} else if (head[0] == 0xE0 || head[0] == 0x0E) {
		// MET files have either of these headers
		return EFT_Met;
	}

	// Check at most the first ten chars, if all are printable, 
	// then we can probably assume it is ascii text-file.
	for (int i = 0; i < read; ++i) {
		if (!(		isprint(head[i]) 
				||	isspace(head[i])
				||	(i < 3 && head[i] == UTF8bom[i]))) {
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
	
	wxTempFile target(file);

	while (!filename.IsEmpty() && !target.Length()) {
		// Extract the filename part of the URI
		filename = filename.AfterLast(wxT(':')).Lower();
	
		// We only care about the files specified in the array
		for (size_t i = 0; files[i] && !target.Length(); ++i) {
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
						while (!zip.Eof()) {
							zip.Read(buffer, sizeof(buffer));
							target.Write(buffer, zip.LastRead());
						}						
						break;
					}
				}
			}
		}

		filename = archive.FindNext();
	}
	
	if (target.Length()) {
		target.Commit();
		return true;
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
	// AddDebugLogLineN( logFileIO, wxT("Reading gzip stream") );

	gzFile inputFile = gzopen(filename2char(file), "rb");
	if (inputFile != NULL) {
		write = true;

		while (int bytesRead = gzread(inputFile, buffer, sizeof(buffer))) {
			if (bytesRead > 0) {
				// AddDebugLogLineN(logFileIO, CFormat(wxT("Read %u bytes")) % bytesRead);
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
				
				// AddDebugLogLineN( logFileIO, wxT("Error reading gzip stream (") + errString + wxT(")") );
				write = false;
				break;
			}
		}

		// AddDebugLogLineN( logFileIO, wxT("End reading gzip stream") );
		gzclose(inputFile);
	} else {
		// AddDebugLogLineN( logFileIO, wxT("Error opening gzip file (") + wxString(wxSysErrorMsg()) + wxT(")") );
	}
#else
	{
		// AddDebugLogLineN( logFileIO, wxT("Reading gzip stream") );

		wxFileInputStream source(file);
		wxZlibInputStream inputStream(source);

		while (!inputStream.Eof()) {
			inputStream.Read(buffer, sizeof(buffer));

			// AddDebugLogLineN(logFileIO, CFormat(wxT("Read %u bytes")) % inputStream.LastRead());
			if (inputStream.LastRead()) {
				target.Write(buffer, inputStream.LastRead());
			} else {
				break;
			}
		};

		// AddDebugLogLineN( logFileIO, wxT("End reading gzip stream") );

		write = inputStream.IsOk() || inputStream.Eof();
	}
#endif

	if (write) {
		target.Commit();
		// AddDebugLogLineN( logFileIO, wxT("Commited gzip stream") );
	}

	return write;
}


UnpackResult UnpackArchive(const CPath& path, const wxChar* files[])
{
	const wxString file = path.GetRaw();

	// Attempt to discover the filetype and uncompress
	EFileType type = GuessFiletype(file);
	switch (type) {
		case EFT_Zip:
			if (UnpackZipFile(file, files)) {
				// Unpack nested archives if needed.
				return UnpackResult(true, UnpackArchive(path, files).second);
			} else {
				return UnpackResult(false, EFT_Error);
			}

		case EFT_GZip:
			if (UnpackGZipFile(file)) {
				// Unpack nested archives if needed.
				return UnpackResult(true, UnpackArchive(path, files).second);
			} else {
				return UnpackResult(false, EFT_Error);
			}

		default:
			return UnpackResult(false, type);
	}
}
// File_checked_for_headers
