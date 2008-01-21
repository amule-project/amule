#include "Path.h"
#include "MuleDebug.h"
#include "StringFunctions.h"
#include "Format.h"

#include <wx/filename.h>


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


/**
 * Creates a deep copy of the string, making CPaths safe
 * to pass across threads (by avoiding the reference counting
 * of wxString).
 */
inline wxString DeepCopy(const wxString& str)
{
	return wxString(str.c_str(), str.Length());
}

/** Splits a full path into its path and filename component. */
inline void SplitPath(const wxString& strPath, wxString* path, wxString* name)
{
	bool hasExt = false;
	wxString ext, vol;

	wxString* pVol = (path ? &vol : NULL);
	wxString* pExt = (name ? &ext : NULL);

	wxFileName::SplitPath(strPath, pVol, path, name, pExt, &hasExt);

	if (hasExt && pExt) {
		*name += wxT(".") + ext;
	}

	if (path) {
		*path = vol + wxFileName::GetVolumeSeparator() + *path;
	}
}


CPath::CPath()
{
}


CPath::CPath(const wxString& filename, Source WXUNUSED(src))
{
	// Demangling not yet implemented
	// TODO: Normalize path
	m_filesystem = m_printable = filename;

	wxASSERT(filename.IsEmpty() || IsOk());
}


CPath::CPath(const CPath& other)
	: CPrintable()
	, m_printable(DeepCopy(other.m_printable))
	, m_filesystem(DeepCopy(other.m_filesystem))
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
	return m_printable.Length() && m_filesystem.Length();
}


bool CPath::FileExists() const
{
	return wxFileName::FileExists(m_filesystem);
}


bool CPath::DirExists() const
{
	return wxFileName::DirExists(m_filesystem);
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


CPath CPath::GetPath() const
{
	CPath path;
	::SplitPath(m_printable, &path.m_printable, NULL);
	::SplitPath(m_filesystem, &path.m_filesystem, NULL);

	return path;
}


CPath CPath::GetFullName() const
{
	CPath path;
	::SplitPath(m_printable, NULL, &path.m_printable);
	::SplitPath(m_filesystem, NULL, &path.m_filesystem);

	return path;

}


CPath CPath::JoinPaths(const CPath& other) const
{
	CPath joinedPath;

	joinedPath.m_printable = ::JoinPaths(m_printable, other.m_printable);
	joinedPath.m_filesystem = ::JoinPaths(m_filesystem, other.m_filesystem);

	return joinedPath;
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

