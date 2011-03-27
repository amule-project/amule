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

#include "Path.h"
#include "StringFunctions.h"		// Needed for filename2char()

#include <wx/file.h>
#if defined __WXMSW__ || defined __IRIX__
#	include <wx/ffile.h>
#endif
#include <wx/utils.h>
#include <wx/filename.h>


// This is required in order to ensure that wx can "handle" filenames
// using a different encoding than the current system-wide setting. If
// this is not done, such filenames will fail during conversion to/from
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


// Windows has case-insensitive paths, so we use a
// case-insensitive cmp for that platform. TODO:
// Perhaps it would be better to simply lowercase
// m_filesystem in the constructor ...
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


wxString Demangle(const wxCharBuffer& fn, const wxString& filename)
{
	wxString result = wxConvUTF8.cMB2WC(fn);

	// FIXME: Is this actually needed for osx/msw?
	if (!result) {
		// We only try to further demangle if the current locale is
		// UTF-8, C or POSIX. This is because in any other case, the
		// current locale is probably the best choice for printing.
		static wxFontEncoding enc = wxLocale::GetSystemEncoding();

		switch (enc) {
			// SYSTEM is needed for ANSI encodings such as
			// "POSIX" and "C", which are only 7bit.
			case wxFONTENCODING_SYSTEM:
			case wxFONTENCODING_UTF8:
				result = wxConvISO8859_1.cMB2WC(fn);
				break;

			default:
				// Nothing to do, the filename is probably Ok.
				result = DeepCopy(filename);
		}
	}

	return result;
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
					result += wxT("%20");
				} else if (c >= 32) {
					// Many illegal for filenames in windows
					// below the 32th char (which is space).
					result += filename[i];
				}
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
	wxString cleanPath = StripSeparators(path, wxString::trailing);
	
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


/** Returns true if the two paths are equal. */
bool IsSameAs(const wxString& a, const wxString& b)
{
	// Cache the current directory
	const wxString cwd = wxGetCwd();

	// We normalize everything, except env. variables, which
	// can cause problems when the string is not encodable
	// using wxConvLibc which wxWidgets uses for the purpose.
	const int flags = (wxPATH_NORM_ALL | wxPATH_NORM_CASE) & ~wxPATH_NORM_ENV_VARS;

	// Let wxFileName handle the tricky stuff involved in actually
	// comparing two paths ... Currently, a path ending with a path-
	// seperator will be unequal to the same path without a path-
	// seperator, which is probably for the best, but can could
	// lead to some unexpected behavior.
	wxFileName fn1(a);
	wxFileName fn2(b);
	
	fn1.Normalize(flags, cwd);
	fn2.Normalize(flags, cwd);

	return (fn1.GetFullPath() == fn2.GetFullPath());
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
		m_printable  = Demangle(fn, filename);
	} else {
		// It's not a valid filename in the current locale, so we'll
		// have to do some magic. This ensures that the filename is
		// saved as UTF8, even if the system is not unicode enabled,
		// preserving the original filename till the user has fixed
		// his system ...
#ifdef __WXMSW__
		// Magic fails on Windows where we always work with wide char file names.
		m_filesystem = DeepCopy(filename);
		m_printable = m_filesystem;
#else
		fn = wxConvUTF8.cWC2MB(filename);
		m_filesystem = wxConvFile.cMB2WC(fn);

		// There's no need to try to unmangle the filename here.
		m_printable = DeepCopy(filename);
#endif
	}

	wxASSERT(m_filesystem.Length());
	wxASSERT(m_printable.Length());
}


CPath::CPath(const CPath& other)
	: m_printable(DeepCopy(other.m_printable))
	, m_filesystem(DeepCopy(other.m_filesystem))
{}



CPath CPath::FromUniv(const wxString& path)
{
	wxCharBuffer fn = wxConvISO8859_1.cWC2MB(path);

	return CPath(wxConvFile.cMB2WC(fn));

}


wxString CPath::ToUniv(const CPath& path)
{
	// The logic behind this is that by saving the filename
	// as a raw bytestream (which is what ISO8859-1 amounts
	// to), we can always recreate the on-disk filename, as
	// if we had read it using wx functions.
	wxCharBuffer fn = wxConvFile.cWC2MB(path.m_filesystem);

	return wxConvISO8859_1.cMB2WC(fn);
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
	return ::IsSameAs(m_filesystem, other.m_filesystem);
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
	} else if ((mode & writable) && !wxIsWritable(path)) {
		return false;
	} else if ((mode & readable) && !wxIsReadable(path)) {
		return false;
	}

	return true;
}


bool CPath::IsFile(EAccess mode) const
{
	if (!wxFileName::FileExists(m_filesystem)) {
		return false;
	} else if ((mode & writable) && !wxIsWritable(m_filesystem)) {
		return false;
	} else if ((mode & readable) && !wxIsReadable(m_filesystem)) {
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

	return ::IsSameAs(a, b);
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

	// Loop until all extensions are removed
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


bool CPath::CloneFile(const CPath& src, const CPath& dst, bool overwrite)
{
	return ::wxCopyFile(src.m_filesystem, dst.m_filesystem, overwrite);
}


bool CPath::RemoveFile(const CPath& file)
{
	return ::wxRemoveFile(file.m_filesystem);
}


bool CPath::RenameFile(const CPath& src, const CPath& dst, bool overwrite)
{
	return ::wxRenameFile(src.m_filesystem, dst.m_filesystem, overwrite);
}


bool CPath::BackupFile(const CPath& src, const wxString& appendix)
{
	wxASSERT(appendix.IsAscii());

	CPath dst = CPath(src.m_filesystem + appendix);

	if (CPath::CloneFile(src, dst, true)) {
		// Try to ensure that the backup gets physically written 
#if defined __WXMSW__ || defined __IRIX__
		wxFFile backupFile;
#else
		wxFile backupFile;
#endif
		if (backupFile.Open(dst.m_filesystem)) {
			backupFile.Flush();
		}

		return true;
	}

	return false;
}


bool CPath::RemoveDir(const CPath& file)
{
	return ::wxRmdir(file.m_filesystem);
}


bool CPath::MakeDir(const CPath& file)
{
	return ::wxMkdir(file.m_filesystem);
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
	return ::wxFileModificationTime(file.m_filesystem);
}


sint64 CPath::GetFreeSpaceAt(const CPath& path)
{
	wxLongLong free;
	if (::wxGetDiskSpace(path.m_filesystem, NULL, &free)) {
		return free.GetValue();
	}

	return wxInvalidOffset;
}


wxString CPath::TruncatePath(size_t length, bool isFilePath) const
{
	wxString file = GetPrintable();

	// Check if there's anything to do
	if (file.Length() <= length) {
		return file;
	}

	// If the path is a file name, then prefer to remove from the path, rather than the filename
	if (isFilePath) {
		wxString path = wxFileName(file).GetPath();
		file          = wxFileName(file).GetFullName();

		if (path.Length() >= length) {
			path.Clear();
		} else if (file.Length() >= length) {
			path.Clear();
		} else {
			// Minus 6 for "[...]" + separator
			int pathlen = (int)(length - file.Length() - 6);

			if (pathlen > 0) {
				path = wxT("[...]") + path.Right( pathlen );
			} else {
				path.Clear();
			}
		}

		file = ::JoinPaths(path, file);
	}

	if (file.Length() > length) {
		if (length > 5) {		
			file = file.Left(length - 5) + wxT("[...]");
		} else {
			file.Clear();
		}
	}

	return file;
}


wxString StripSeparators(wxString path, wxString::stripType type)
{
	wxASSERT((type == wxString::leading) || (type == wxString::trailing));
	const wxString seps = wxFileName::GetPathSeparators();

	while (!path.IsEmpty()) {
		size_t pos = ((type == wxString::leading) ? 0 : path.Length() - 1);

		if (seps.Contains(path.GetChar(pos))) {
			path.Remove(pos, 1);
		} else {
			break;
		}
	}

	return path;
}


wxString JoinPaths(const wxString& path, const wxString& file)
{
	if (path.IsEmpty()) {
		return file;
	} else if (file.IsEmpty()) {
		return path;
	}

	return StripSeparators(path, wxString::trailing)
	   + wxFileName::GetPathSeparator()
	   + StripSeparators(file, wxString::leading);
}
