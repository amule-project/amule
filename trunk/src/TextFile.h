//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/file.h>
#include <wx/string.h>


/**
 * Text file reader class.
 *
 * This class is a wrapper around wxFile, letting an ASCII text file be read
 * line-by-line. The class provides transparent and automatic EOL-style
 * handling.
 *
 * @note This class provides read-only, unseekable access to text files. That
 * means, you cannot read lines in random order (such as with wxTextFile), only
 * sequentially.
 */
class CTextFile
{
 public:
	CTextFile();
	CTextFile(const wxString& path);

	~CTextFile();

	bool		Open(const wxString& path)	{ return m_buffer ? m_file.Open(path, wxFile::read) : false; }
	bool		IsOpened() const 	{ return m_file.IsOpened(); }
	bool		Eof() const		{ return m_file.IsOpened() ? m_eof && m_pos == m_size : true; }
	bool		Close()			{ return m_file.Close(); }
	wxString&	GetNextLine();

 private:
	wxFile		m_file;
	char *		m_buffer;
	size_t		m_size;
	unsigned int	m_pos;
	bool		m_eof;
	bool		m_mayBeCrLf;
	wxString	m_currentLine;

	void		Init();
};

#endif /* TEXTFILE_H */
