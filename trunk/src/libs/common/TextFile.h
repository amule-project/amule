//
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef TEXTFILE_H
#define TEXTFILE_H

#include <wx/ffile.h>
#include <wx/string.h>
#include <wx/strconv.h>


class CPath;

/** Criteria used when reading an entire file to an array of strings. */
enum EReadTextFile
{
	/** Do not filter anything */
	txtReadAll = 0,	
	/** Do not return empty lines. Can be combined with txtStripWhiteSpace */
	txtIgnoreEmptyLines = 1,
	/** Do not return lines starting with a '#' */
	txtIgnoreComments = 2,
	/** Strip whitespace from the beginning/end of lines. */
	txtStripWhitespace = 4,

	/** Default parameters for file reading. */
	txtReadDefault = txtIgnoreEmptyLines | txtIgnoreComments | txtStripWhitespace
};


/**
 * Text file class.
 *
 * This class is a wrapper around wxFFile, letting an text file be read
 * or written line-by-line. The class provides transparent and automatic
 * EOL-style handling.
 * 
 * Note that it is not possible to seek in a CTextFile, only sequential
 * reading or writing is possible. Also note that the maximum length of a
 * line is fixed (see CTextFile::GetNextLine), however this shouldn't be
 * a problem, given the uses of this class.
 */
class CTextFile
{
public:
	// Open modes. Note that these are mutually exclusive!
	enum EOpenMode {
		//! Opens the file for reading, if it exists.
		read,
		//! Opens the file for writing, overwriting old contents.
		write
	};

	/* Constructor. */
	CTextFile();
	/** Destructor. Closes the file if still open. */
	~CTextFile();

	/** Opens the specified file, returning true on success. */
	//\{
	bool Open(const wxString& path, EOpenMode mode);
	bool Open(const CPath& path, EOpenMode mode);
	//\}

	/** Returns true if the file is opened. */
	bool		IsOpened() const;
	/** Returns true if GetNextLine has reached the end of the file. */
	bool		Eof() const;
	/** Closes the file, returning true on success. */
	bool		Close();


	/**
	 * Returns the next line of a readable file.
	 *
	 * @param conv The converter used to convert from multibyte to widechar.
	 *
	 * Note that GetNextLine will return an empty string if the file has reached
	 * EOF, or if the file is closed, or not readable. However, empty lines in
	 * the file will also be returned unless otherwise specified, so this cannot be used to test for EOF.
	 * Instead, use the function Eof().
	 **/
	wxString	GetNextLine(EReadTextFile flags = txtReadAll, const wxMBConv& conv = wxConvLibc, bool* result = NULL);

	/** 
	 * Writes the line to a writable file, returning true on success.
	 *
	 * @param conv The converter used to convert from widechar to multibyte.
	 */
	bool		WriteLine(const wxString& line, const wxMBConv& conv = wxConvLibc);


	/** Reads and returns the contents of a text-file, using the specifed criteria and converter. */
	wxArrayString ReadLines(EReadTextFile flags = txtReadDefault, const wxMBConv& conv = wxConvLibc);

	/** Writes the lines to the file, using the given converter, returning true if no errors occured. */
	bool WriteLines(const wxArrayString& lines, const wxMBConv& conv = wxConvLibc);

private:
	//! The actual file object.
	wxFFile		m_file;
	//! The mode in with which the file was opened.
	EOpenMode	m_mode;
};

#endif /* TEXTFILE_H */
