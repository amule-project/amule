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

#include "Path.h"
#include "MuleDebug.h"
#include "StringFunctions.h"

#include <wx/file.h>
#include <wx/utils.h>
#include <wx/filename.h>


// This is required in order to ensure that wx can "handle" filenames
// using a different encoding than the current system-wide setting. If
// this is not done, such filenames will fail during convertion to/from
// multibyte (as in cWC2MB/cMB2WC).
#if !wxUSE_GUI && !defined(__WXMSW__)
void* setFNConv()
{
	// This uses the same method as wxApp::Initialize under GTK2
	wxString encName = wxLocale::GetSystemEncodingName().Upper();
	if (encName.IsEmpty() || (encName == wxT("US-ASCII"))) {
		encName = wxT("UTF-8");
	}

	return wxConvFileName = new wxConvBrokenFileNames(encName);
}

// Ensure intialization
static void* s_foo = setFNConv();
#endif


/** Returns the converter to use for non-utf8 filenames. */
wxMBConv* GetDemangler()
{
	// We try to use the system locale, This ensures that
	// demangling works for ISO8859-* and the like.
	static wxMBConv* s_demangler = NULL;
	if (s_demangler == NULL) {
		wxFontEncoding enc = wxLocale::GetSystemEncoding();

		switch (enc) {
			// System is needed for ANSI encodings such
			// as "POSIX" and "C".
			case wxFONTENCODING_SYSTEM:
			case wxFONTENCODING_UTF8:
				// Fall back to ISO8859-1.
				s_demangler = &wxConvISO8859_1;
				break;

			default:
				// Use the system locale.
				s_demangler = &wxConvLocal;
		}
	}

	return s_demangler;
}




// Windows has case-insensitive paths, so we use a
// case-insensitive cmp for that platform. TODO:
// Perhaps it would be better to simply lowercase
// m_filesystem in the contructor ...
#ifdef __WXMSW__
	#define PATHCMP(a, b)		wxStricmp(a, b)
	#define PATHNCMP(a, b, n)	wxStrnicmp(a, b, n)
#else
	#define PATHCMP(a, b)		wxStrcmp(a, b)
	#define PATHNCMP(a, b, n)	wxStrncmp(a, b, n)
#endif


////////////////////////////////////////////////////////////
// Helper functions


/** Creates a deep copy of the string, avoiding its ref. counting. */
inline wxString DeepCopy(const wxString& str)
{
	return wxString(str.c_str(), str.Length());
}

/** Splits a full path into its path and filename component. */
inline void DoSplitPath(const wxString& strPath, wxString* path, wxString* name)
{
	bool hasExt = false;
	wxString ext, vol;

	wxString* pVol = (path ? &vol : NULL);
	wxString* pExt = (name ? &ext : NULL);

	wxFileName::SplitPath(strPath, pVol, path, name, pExt, &hasExt);

	if (hasExt && pExt) {
		*name += wxT(".") + ext;
	}

	if (path && vol.Length()) {
		*path = vol + wxFileName::GetVolumeSeparator() + *path;
	}
}


/** Removes invalid chars from a filename. */
wxString DoCleanup(const wxString& filename, bool keepSpaces, bool isFAT32)
{
	wxString result;
	for (size_t i = 0; i < filename.Length(); i++) {
		const wxChar c = filename[i];

		switch (c) {
			case wxT('/'):
				continue;

			case wxT('\"'):
			case wxT('*'):
			case wxT('<'):
			case wxT('>'):
			case wxT('?'):
			case wxT('|'):
			case wxT('\\'):
			case wxT(':'):
				if (isFAT32) {
					continue;
				}
				
			default:
				if ((c == wxT(' ')) && !keepSpaces) {
					continue;
				} else if (c < 32) {
					// Many illegal for filenames in windows
					// below the 32th char (which is space).
					continue;
				}

				result += filename[i];
		}
	}

	return result;
}


/** Does the actual work of adding a postfix ... */
wxString DoAddPostfix(const wxString& src, const wxString& postfix)
{
	const wxFileName srcFn(src);
	wxString result = srcFn.GetName() + postfix;

	if (srcFn.HasExt()) {
		result += wxT(".") + srcFn.GetExt();
	}

	wxString path = srcFn.GetPath();
	if (path.Length()) {
		return path + wxFileName::GetPathSeparator() + result;
	}

	return result;
}

/** Removes the last extension of a filename. */
wxString DoRemoveExt(const wxString& path)
{
	// Using wxFilename which handles paths, etc.
	wxFileName tmp(path);
	tmp.ClearExt();

	return tmp.GetFullPath();
}


/** Readies a path for use with wxAccess.. */
wxString DoCleanPath(const wxString& path)
{
#ifdef __WXMSW__
	// stat fails on windows if there are trailing path-separators.
	wxString cleanPath = StripSeparators(m_filesystem, wxString::trailing);
	
	// Root paths must end with a separator (X:\ rather than X:).
	// See comments in wxDirExists.
	if ((cleanPath.Length() == 2) && (cleanPath.Last() == wxT(':'))) {
		cleanPath += wxFileName::GetPathSeparator();
	}

	return cleanPath;
#else
	return path;
#endif
}


////////////////////////////////////////////////////////////
// CPath implementation

CPath::CPath()
{
}


CPath::CPath(const wxString& filename)
{
	// Equivalent to the default constructor ...
	if (!filename) {
		return;
	}

	wxCharBuffer fn = filename2char(filename);
	if (fn) {
		// Filename is valid in the current locale. This means that
		// it either originated from a (wx)system-call, or from a
		// user with a properly setup system.
		m_filesystem = DeepCopy(filename);
	} else {
		// It's not a valid filename in the current locale, so we'll
		// have to do some magic. This ensures that the filename is
		// saved as UTF8, even if the system is not unicode enabled,
		// preserving the original filename till the user has fixed
		// his system ...
		fn = wxConvUTF8.cWC2MB(filename);
		m_filesystem = wxConvFile.cMB2WC(fn);
	}

	// FIXME: Is this actually needed for osx/msw?
	// Try to unmangle the filename for printing.
	m_printable = wxConvUTF8.cMB2WC(fn);
	if (!m_printable) {
		m_printable = GetDemangler()->cMB2WC(fn);
	}

	wxASSERT(m_filesystem.Length());
	wxASSERT(m_printable.Length());
}


CPath::CPath(const CPath& other)
	: CPrintable()
	, m_printable(DeepCopy(other.m_printable))
	, m_filesystem(DeepCopy(other.m_filesystem))
{
}


CPath::~CPath()
{
}


CPath& CPath::operator=(const CPath& other)
{
	if (this != &other) {
		m_printable = DeepCopy(other.m_printable);
		m_filesystem = DeepCopy(other.m_filesystem);
	}

	return *this;
}


bool CPath::operator==(const CPath& other) const
{
	// Let wxFileName handle the tricky stuff involved in actually
	// comparing two paths ... Currently, a path ending with a path-
	// seperator will be unequal to the same path without a path-
	// seperator, which is probably for the best, but can could
	// lead to some unexpected behavior.
	return wxFileName(m_filesystem).SameAs(other.m_filesystem);
}


bool CPath::operator!=(const CPath& other) const
{
	return !(*this == other);
}


bool CPath::operator<(const CPath& other) const
{
	return PATHCMP(m_filesystem.c_str(), other.m_filesystem.c_str()) < 0;
}


bool CPath::IsOk() const
{
	// Something is very wrong if one of the two is empty.
	return m_printable.Length() && m_filesystem.Length();
}


bool CPath::FileExists() const
{
	return wxFileName::FileExists(m_filesystem);
}


bool CPath::DirExists() const
{
	return wxFileName::DirExists(DoCleanPath(m_filesystem));
}


bool CPath::IsDir(EAccess mode) const
{
	wxString path = DoCleanPath(m_filesystem);
	if (!wxFileName::DirExists(path)) {
		return false;
	}
	
	if ((mode & writable) && wxIsWritable(path)) {
		return false;
	}

	if ((mode & readable) && wxIsReadable(path)) {
		return false;
	}

	return true;
}


bool CPath::IsFile(EAccess mode) const
{
	if (!wxFileName::FileExists(m_filesystem)) {
		return false;
	}
	
	if ((mode & writable) && wxIsWritable(m_filesystem)) {
		return false;
	}

	if ((mode & readable) && wxIsReadable(m_filesystem)) {
		return false;
	}

	return true;
}


wxString CPath::GetRaw() const
{
	// Copy as c-strings to ensure that the CPath objects can safely
	// be passed across threads (avoiding wxString ref. counting).
	return DeepCopy(m_filesystem);
}


wxString CPath::GetPrintable() const
{
	// Copy as c-strings to ensure that the CPath objects can safely
	// be passed across threads (avoiding wxString ref. counting).
	return DeepCopy(m_printable);
}


wxString CPath::GetExt() const
{
	return wxFileName(m_filesystem).GetExt();
}


CPath CPath::GetPath() const
{
	CPath path;
	::DoSplitPath(m_printable, &path.m_printable, NULL);
	::DoSplitPath(m_filesystem, &path.m_filesystem, NULL);

	return path;
}


CPath CPath::GetFullName() const
{
	CPath path;
	::DoSplitPath(m_printable, NULL, &path.m_printable);
	::DoSplitPath(m_filesystem, NULL, &path.m_filesystem);

	return path;

}


sint64 CPath::GetFileSize() const
{
	if (FileExists()) {
		wxFile f(m_filesystem);
		if (f.IsOpened()) {
			return f.Length();
		}
	}

	return wxInvalidOffset;
}


bool CPath::IsSameDir(const CPath& other) const
{
	wxString a = m_filesystem;
	wxString b = other.m_filesystem;

	// This check is needed to avoid trouble in the
	// case where one path is empty, and the other
	// points to the root dir.
	if (a.Length() && b.Length()) {
		a = StripSeparators(a, wxString::trailing);
		b = StripSeparators(b, wxString::trailing);
	}

	// We use wxFileName::SameAs, as this function handles
	// platform specific issues (case-sensitivity, etc).
	return wxFileName(a).SameAs(b);
}


CPath CPath::JoinPaths(const CPath& other) const
{
	if (!IsOk()) {
		return CPath(other);
	} else if (!other.IsOk()) {
		return CPath(*this);
	} 

	CPath joinedPath;
	// DeepCopy shouldn't be needed, as JoinPaths results in the creation of a new string.
	joinedPath.m_printable = ::JoinPaths(m_printable, other.m_printable);
	joinedPath.m_filesystem = ::JoinPaths(m_filesystem, other.m_filesystem);

	return joinedPath;
}


CPath CPath::Cleanup(bool keepSpaces, bool isFAT32) const
{
	CPath result;
	result.m_printable = ::DoCleanup(m_printable, keepSpaces, isFAT32);
	result.m_filesystem = ::DoCleanup(m_filesystem, keepSpaces, isFAT32);

	return result;
}


CPath CPath::AddPostfix(const wxString& postfix) const
{
	wxASSERT(postfix.IsAscii());

	CPath result;
	result.m_printable = ::DoAddPostfix(m_printable, postfix);
	result.m_filesystem = ::DoAddPostfix(m_filesystem, postfix);

	return result;
}


CPath CPath::AppendExt(const wxString& ext) const
{
	wxASSERT(ext.IsAscii());

	// Though technically, and empty extension would simply 
	// be another . at the end of the filename, we ignore them.
	if (ext.IsEmpty()) {
		return *this;
	}

	CPath result(*this);
	if (ext[0] == wxT('.')) {
		result.m_printable << ext;
		result.m_filesystem << ext;
	} else {
		result.m_printable << wxT(".") << ext;
		result.m_filesystem << wxT(".") << ext;
	}

	return result;
}


CPath CPath::RemoveExt() const
{
	CPath result;
	result.m_printable = DoRemoveExt(m_printable);
	result.m_filesystem = DoRemoveExt(m_filesystem);

	return result;
}


CPath CPath::RemoveAllExt() const
{
	CPath last, current = RemoveExt();

	// Loop untill all extensions are removed
	do {
		last = current;

		current = last.RemoveExt();
	} while (last != current);

	return current;
}


bool CPath::StartsWith(const CPath& other) const
{
	// It doesn't make sense comparing invalid paths,
	// especially since if 'other' was empty, it would
	// be considered a prefix of any path.
	if ((IsOk() && other.IsOk()) == false) {
		return false;
	}

	// Adding an seperator to avoid partial matches, such as
	// "/usr/bi" matching "/usr/bin". TODO: Paths should be
	// normalized first (in the constructor).
	const wxString a = StripSeparators(m_filesystem, wxString::trailing) + wxFileName::GetPathSeparator();
	const wxString b = StripSeparators(other.m_filesystem, wxString::trailing) + wxFileName::GetPathSeparator();
	
	if (a.Length() < b.Length()) {
		// Cannot possibly be a prefix.
		return false;
	}

	const size_t checkLen = std::min(a.Length(), b.Length());
	return PATHNCMP(a.c_str(), b.c_str(), checkLen) == 0;
}


wxString CPath::GetPrintableString() const
{
	return m_printable;
}


bool CPath::CloneFile(const CPath& src, const CPath& dst, bool overwrite)
{
	return ::wxCopyFile(src.GetRaw(), dst.GetRaw(), overwrite);
}


bool CPath::RemoveFile(const CPath& file)
{
	return ::wxRemoveFile(file.GetRaw());
}


bool CPath::RenameFile(const CPath& src, const CPath& dst, bool overwrite)
{
	return ::wxRenameFile(src.GetRaw(), dst.GetRaw(), overwrite);
}


bool CPath::BackupFile(const CPath& src, const wxString& appendix)
{
	wxASSERT(appendix.IsAscii());

	CPath dst = CPath(src.GetRaw() + appendix);

	if (CPath::CloneFile(src, dst, true)) {
		// Try to ensure that the backup gets physically written 
		wxFile backupFile;
		if (backupFile.Open(dst.GetRaw())) {
			backupFile.Flush();
		}

		return true;
	}

	return false;
}


bool CPath::RemoveDir(const CPath& file)
{
#ifndef __VMS__
	return ::wxRmdir(file.GetRaw());
#else
	//#warning wxRmdir does not work under VMS !
	return false;
#endif
}


bool CPath::MakeDir(const CPath& file)
{
	return ::wxMkdir(file.GetRaw());
}


bool CPath::FileExists(const wxString& file)
{
	return CPath(file).FileExists();
}


bool CPath::DirExists(const wxString& path)
{
	return CPath(path).DirExists();
}


sint64 CPath::GetFileSize(const wxString& file)
{
	return CPath(file).GetFileSize();
}


time_t CPath::GetModificationTime(const CPath& file)
{
	return ::wxFileModificationTime(file.GetRaw());
}


sint64 CPath::GetFreeSpace(const CPath& path)
{
	wxLongLong free;
	if (::wxGetDiskSpace(path.GetRaw(), NULL, &free)) {
		return free.GetValue();
	}

	return wxInvalidOffset;
}

