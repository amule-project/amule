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

#include "TextFile.h"
#include <wx/strconv.h>


CTextFile::CTextFile()
	: m_file()
{
	Init();
}

CTextFile::CTextFile(const wxString& path)
	: m_file(path, wxFile::read)
{
	Init();
	if (!m_buffer && IsOpened()) {
		Close();
	}
}

CTextFile::~CTextFile()
{
	delete [] m_buffer;
}

wxString CTextFile::GetNextLine()
{
	wxString retval;

	if (IsOpened()) {
		bool eol = false;
		unsigned int start;
		unsigned int len;
		do {
			if (!m_eof && (m_pos == m_size)) {
				m_size = m_file.Read(m_buffer, 32768);
				if (m_size == wxInvalidOffset) {
					m_size = 0;
				}
				m_pos = 0;
				if (m_size < 32768) {
					m_eof = true;
				}
				m_buffer[m_size] = '\0';
			} else {
				if (m_mayBeCrLf) {
					if (m_pos < m_size && m_buffer[m_pos] == '\n') {
						++m_pos;
					}
					m_mayBeCrLf = false;
				}
				start = m_pos;
				while (m_pos < m_size) {
					if (m_buffer[m_pos] == '\n') {
						len = m_pos - start;
						++m_pos;
						eol = true;
						break;
					} else if (m_buffer[m_pos] == '\r') {
						len = m_pos - start;
						++m_pos;
						m_mayBeCrLf = true;
						eol = true;
						break;
					}
					++m_pos;
				}
				if (!eol && m_pos == m_size) {
					len = m_pos - start;
					if (m_eof) {
						eol = true;
					}
				}
				retval += wxString((const char *)(m_buffer + start), wxConvLibc, len);
			}
		} while (!eol);
	}
	return retval;
}

void CTextFile::Init()
{
	m_buffer = new char[32769];
	m_size = 0;
	m_pos = 0;
	m_mayBeCrLf = false;
	m_eof = (m_buffer == NULL);
}
