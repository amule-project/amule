//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Stu Redman ( sturedman@amule.org )
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

#include "FileAutoClose.h"
#include "GetTickCount.h"	// for TheTime
#include "Logger.h"			// Needed for AddDebugLogLineN


static const uint32 ReleaseTime = 600;	// close file after 10 minutes of not being used

CFileAutoClose::CFileAutoClose()
	: m_mode(CFile::read),
	  m_autoClosed(false),
	  m_locked(0),
	  m_size(0),
	  m_lastAccess(TheTime)
{}
	
CFileAutoClose::CFileAutoClose(const CPath& path, CFile::OpenMode mode)
{
	Open(path, mode);
}

bool CFileAutoClose::Open(const CPath& path, CFile::OpenMode mode)
{
	m_mode = mode;
	m_autoClosed = false;
	m_locked = 0;
	m_size = 0;
	m_lastAccess = TheTime;
	return m_file.Open(path, mode);
}
	
bool CFileAutoClose::Create(const CPath& path, bool overwrite)
{
	m_mode = CFile::write;
	m_autoClosed = false;
	m_lastAccess = TheTime;
	return m_file.Create(path, overwrite);
}
	
bool CFileAutoClose::Close()
{
	bool state = m_autoClosed ? true : m_file.Close();
	m_autoClosed = false;
	return state;
}

uint64 CFileAutoClose::GetLength() const
{
	return m_autoClosed ? m_size : m_file.GetLength();
}
	
bool CFileAutoClose::SetLength(uint64 newLength)
{
	Reopen();
	return m_file.SetLength(newLength);
}

const CPath& CFileAutoClose::GetFilePath() const
{
	return m_file.GetFilePath();
}

bool CFileAutoClose::IsOpened() const
{
	return m_autoClosed || m_file.IsOpened();
}
	
void CFileAutoClose::ReadAt(void* buffer, uint64 offset, size_t count)
{
	Reopen();
	m_file.Seek(offset);
	m_file.Read(buffer, count);
}

void CFileAutoClose::WriteAt(const void* buffer, uint64 offset, size_t count)
{
	Reopen();
	m_file.Seek(offset);
	m_file.Write(buffer, count);
}

bool CFileAutoClose::Eof()
{
	Reopen();
	return m_file.Eof();
}

int CFileAutoClose::fd()
{
	Reopen();
	m_locked++;
	return m_file.fd();
}

void CFileAutoClose::Unlock() 
{
	if (m_locked) {
		m_locked--;
	}
}

void CFileAutoClose::Reopen()
{
	if (m_autoClosed) {
		AddDebugLogLineN(logCFile, wxT("Reopen AutoClosed file ") + GetFilePath().GetPrintable());
		m_file.Reopen(m_mode); // throws on failure
		// On open error m_autoClosed stays true, so if the app tries again
		// it opens and throws again.
		// Otherwise it would assert on an operation on a closed file and probably die.
		m_autoClosed = false;
	}
	m_lastAccess = TheTime;
}

bool CFileAutoClose::Release(bool now)
{
	if (!m_autoClosed
			&& (now || TheTime - m_lastAccess >= ReleaseTime)
			&& !m_locked
			&& m_file.IsOpened()) {
		m_autoClosed = true;
		m_size = m_file.GetLength();
		m_file.Close();
		AddDebugLogLineN(logCFile, wxT("AutoClosed file ") + GetFilePath().GetPrintable()
			+ (now ? wxT("(immediately)") : wxT("(timed)")));
	}
	return m_autoClosed;
}

// File_checked_for_headers
