#ifndef FILENAME_H
#define FILENAME_H

#include "Format.h"


/**
 * Partially implemented class, to allow for transition to
 * use of CPath. Currently returns the passed path unchanged.
 */
class CPath : public CPrintable
{
public:
	//! Two possible sources of filenames
	enum Source {
		//! Filenames that are derived from filesystem calls,
		//! such as wxDir. Must not be mangled, as that would
		//! cause problem when trying to access the files.
		FromFS,
		//! Filenames from the user, or other sources, that do
		//! not refer to existing files. These can be prettyfied
		//! before use.
		FromUser
	};

	/**
	 * Default constructor.
	 *
	 * Filename must be set before use.
	 */
	CPath();

	/**
	 * Constuctor.
	 *
	 * @param filename The partial or complete path of a file.
	 * @param src The type of filename specified. See above.
	 */
	CPath(const wxString& filename, Source src);

	/**
	 * Copy constructor. Creates a deep-copy of the passed object.
	 */
	CPath(const CPath& other);
	

	/** Standard operators. */
	// \{
	CPath& operator=(const CPath& other);
	bool operator==(const CPath& other) const;
	bool operator!=(const CPath& other) const;
	bool operator<(const CPath& other) const;
	// \}

	
	/** Returns true if the filename is valid, false otherwise. */
	bool IsOk() const;

	/** Returns true if the path exists and is a file, false otherwise. */
	bool FileExists() const;
	/** Returns true if the path exists and is a directory, false otherwise. */
	bool DirExists() const;


	/** Returns the full path for use in wx system-calls. */
	wxString GetRaw() const;
	/** Returns the full path for use in the UI. */
	wxString GetPrintable() const;

	/** Returns the full path, exluding the filename. */
	CPath GetPath() const;
	/** Returns the full filename, excluding the path. */
	CPath GetFullName() const;


	/** Returns a CPath created from joining the two objects. */
	CPath JoinPaths(const CPath& other) const;
	/** Returns true if the the passed path makes up an prefix of this object. */
	bool StartsWith(const CPath& other) const;

	/** @see cprintable::getprintablestring */
	wxString GetPrintableString() const;	

private:
	//! Contains the printable filename, for use in the UI.
	wxString	m_printable;
	//! Contains the "raw" filename, for use in system-calls,
	//! as well as in wxWidgets file functions.
	wxString	m_filesystem;
};


#endif
