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

#include "TextFile.h"
#include "Path.h"

#include <wx/textbuf.h>


//! The maximum number of chars read at once in GetNextLine
const size_t TXTBUF_SIZE = 1024;


CTextFile::CTextFile()
{
}


bool CTextFile::Open(const wxString& path, EOpenMode mode)
{
	return Open(CPath(path), mode);
}


bool CTextFile::Open(const CPath& path, EOpenMode mode)
{
	// wxFFile doesn't call close itself, but asserts instead.
	Close();

	m_mode = mode;
	
	if (mode == read) {
		if (path.FileExists()) {
			m_file.Open(path.GetRaw(), wxT("r"));
		}
	} else if (mode == write) {
		m_file.Open(path.GetRaw(), wxT("w"));
	} else {
		wxFAIL;
	}

	return IsOpened();
}


CTextFile::~CTextFile()
{
}


bool CTextFile::IsOpened() const
{
	return m_file.IsOpened();
}


bool CTextFile::Eof() const
{
	// This is needed because feof will crash if the
	// underlying FILE pointer is NULL, as is the 
	// case when the file is closed.
	return m_file.IsOpened() ? m_file.Eof() : true;
}


bool CTextFile::Close()
{
	return m_file.Close();
}


wxString CTextFile::GetNextLine(EReadTextFile flags, const wxMBConv& conv, bool* result)
{
	wxCHECK_MSG(m_file.IsOpened(), wxEmptyString, wxT("Trying to read from closed file."));
	wxCHECK_MSG(!m_file.Eof(), wxEmptyString, wxT("Trying to read past EOF"));
	wxCHECK_MSG((m_mode == read), wxEmptyString, wxT("Trying to read from non-readable file."));

	bool is_filtered = false;

	wxString line;
	char buffer[TXTBUF_SIZE];

	// Loop until EOF (fgets will then return NULL) or a newline is read.
	while (fgets(buffer, TXTBUF_SIZE, m_file.fp())) {
		// Filters must be first applied here to avoid unnecessary CPU usage.
		
		if (line.IsEmpty()) {
			if (buffer[0] == '\0') {
				// Empty line.
				break;
			} else if (flags & txtIgnoreComments) {
				int i = 0;
				char t = buffer[i];
				while (t) {
					if ((t == ' ') || (t == '\t')) {
						++i;
						t = buffer[i];
					} else {
						is_filtered = (buffer[i] == '#');
						break;
					}
				}
			}
		}

		if (!is_filtered) {
			// NB: The majority of the time spent by this function is
			//     spent converting the multibyte string to wide-char.
			line += conv.cMB2WC(buffer);

			// Remove any newlines, carriage returns, etc.
			if (line.Length() && (line.Last() == wxT('\n'))) {
				if ((line.Length() > 1)) {
					if (line[line.Length() - 2] == wxT('\r')) {
						// Carriage return + newline
						line.RemoveLast(2);
					} else {
						// Only a newline.
						line.RemoveLast(1);					
					}
				} else {
					// Empty line
					line.Clear();
				}

				// We've read an entire line.
				break;
			}
		} else {
			// Filtered line.
			break;
		}
	}

	if (!is_filtered) {
		if (flags & txtStripWhitespace) {
			line = line.Strip(wxString::both);
		}

		if ((flags & txtIgnoreEmptyLines) && line.IsEmpty()) {
			is_filtered = true;
		}
	}
	
	if (result) {
		*result = !is_filtered;
	}
	
	return line;
}


bool CTextFile::WriteLine(const wxString& line, const wxMBConv& conv)
{
	wxCHECK_MSG(m_file.IsOpened(), false, wxT("Trying to read from closed file."));
	wxCHECK_MSG((m_mode == write), false, wxT("Trying to read from non-readable file."));

	// Ensures that use of newlines/carriage-returns matches the OS
	wxString result = wxTextBuffer::Translate(line);
	
	// Only add line-breaks between lines, as otherwise the number of
	// lines would grow as the result of the addition of an empty line,
	// at the end of the file.
	if (m_file.Tell() > 0) {
		result = wxTextBuffer::GetEOL() + result;
	}

	wxCharBuffer strBuffer = conv.cWC2MB(result);
	if (strBuffer) {
		const size_t length = strlen(strBuffer);

		return (m_file.Write(strBuffer, length) == length);
	}
	
	return false;
}


wxArrayString CTextFile::ReadLines(EReadTextFile flags, const wxMBConv& conv)
{
	wxArrayString lines;

	while (!Eof()) {
		bool result = true;
		
		wxString line = GetNextLine(flags, conv, &result);

		if (result) {
			lines.Add(line);
		}
	}

	return lines;
}


bool CTextFile::WriteLines(const wxArrayString& lines, const wxMBConv& conv)
{
	bool result = true;

	for (size_t i = 0; i < lines.GetCount(); ++i) {
		result &= WriteLine(lines[i], conv);
	}

	return result;
}

