//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 1998 Vadim Zeitlin ( zeitlin@dptmaths.ens-cachan.fr )
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


#include "FileArea.h"		// Interface declarations.
#include "Logger.h"		// Needed for AddDebugLogLineM


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif


CFileArea::CFileArea()
	: m_buffer(NULL), m_mmap_buffer(NULL), m_length(0),
	  m_fd(CFile::fd_invalid)
{
}


CFileArea::~CFileArea()
{
	Close();
}

bool CFileArea::Close()
{
	if (m_fd != CFile::fd_invalid)
	{
		close(m_fd);
		m_fd = CFile::fd_invalid;
	}
	if (m_buffer != NULL && m_mmap_buffer == NULL) 
	{
		delete[] m_buffer;
		m_buffer = NULL;
	}
#ifdef HAVE_MMAP
	#warning using experimental MMAP file reading
	if (m_mmap_buffer)
	{
		munmap(m_mmap_buffer, m_length);
		m_mmap_buffer = NULL;
	}
#endif
	return true;
}


void CFileArea::Read(const CFile& file, size_t count)
{
	Close();

#ifdef HAVE_MMAP
	const uint64 pageSize = 8192u;
	uint64 offset = file.GetPosition();
	uint64 offStart = offset & (~(pageSize-1));
	uint64 offEnd = (offset + count + pageSize - 1) & (~(pageSize-1));
	m_length = offEnd - offStart;
	void *p = mmap(NULL, m_length, PROT_READ, MAP_SHARED, file.fd(), offStart);
	if (p != MAP_FAILED)
	{
		m_mmap_buffer = (byte*) p;
		m_buffer = m_mmap_buffer + (offset - offStart);
		file.Seek(offset + count);
		return;
	}
#endif
	m_buffer = new byte[count];
	file.Read(m_buffer, count);
}

bool CFileArea::Flush()
{
	/* currently we don't support write */
	return true;
}

