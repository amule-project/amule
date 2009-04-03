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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "FileArea.h"		// Interface declarations.
#include "Logger.h"		// Needed for AddDebugLogLineM

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if !defined(HAVE_SIGACTION) || !defined(SA_SIGINFO)

class CFileAreaSigHandler
{
public:
	static void Init() {};
	static void Add(CFileArea& area) {};
	static void Remove(CFileArea& area) {};
};

#else

class CFileAreaSigHandler
{
public:
	static void Init();
	static void Add(CFileArea& area);
	static void Remove(CFileArea& area);
private:
	static wxMutex mutex;
	static CFileArea *first;
	static bool initialized;
	static struct sigaction old_segv, old_bus;
	static void Handler(int sig, siginfo_t *info, void *ctx);
};

wxMutex          CFileAreaSigHandler::mutex;
CFileArea *      CFileAreaSigHandler::first;
bool             CFileAreaSigHandler::initialized = false;
struct sigaction CFileAreaSigHandler::old_segv;
struct sigaction CFileAreaSigHandler::old_bus;

#define PAGE_SIZE 8192u

/* define MAP_ANONYMOUS for Mac OS X */
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

void CFileAreaSigHandler::Handler(int sig, siginfo_t *info, void *ctx)
{
	wxMutexLocker lock(mutex);
	CFileArea* cur = first;
	while (cur) {
		if (cur->m_mmap_buffer && info->si_addr >= cur->m_mmap_buffer && info->si_addr < cur->m_mmap_buffer + cur->m_length) {
			cur->m_error = true;
			char *start_addr = ((char *) info->si_addr) - (((unsigned long) info->si_addr) % PAGE_SIZE);
			if (mmap(start_addr, PAGE_SIZE, PROT_READ, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
				struct sigaction* sa = (sig == SIGSEGV) ? &old_segv : &old_bus;
				if (sa->sa_flags & SA_SIGINFO)
					sa->sa_sigaction(sig, info, ctx);
				else if (sa->sa_handler == SIG_DFL || sa->sa_handler == SIG_IGN)
					abort();
				else
					sa->sa_handler(sig);
			}
			break;
		}
		cur = cur->m_next;
	}
}

void CFileAreaSigHandler::Init()
{
	// init error handler if needed
	wxMutexLocker lock(mutex);
	if (initialized)
		return;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = Handler;
	sa.sa_flags = SA_NODEFER|SA_SIGINFO;
	if (sigaction(SIGSEGV, &sa, &old_segv))
		return;
	if (sigaction(SIGBUS, &sa, &old_bus)) {
		sigaction(SIGSEGV, &old_segv, NULL);
		return;
	}
	initialized = true;
}

void CFileAreaSigHandler::Add(CFileArea& area)
{
	wxMutexLocker lock(mutex);
	area.m_next = first;
	first = &area;
}

void CFileAreaSigHandler::Remove(CFileArea& area)
{
	wxMutexLocker lock(mutex);
	CFileArea **cur = &first;
	while (*cur) {
		if (*cur == &area) {
			*cur = area.m_next;
			area.m_next = NULL;
			break;
		}
		cur = &(*cur)->m_next;
	}
}
#endif

CFileArea::CFileArea()
	: m_buffer(NULL), m_mmap_buffer(NULL), m_length(0), m_next(NULL), m_error(false)
{
	CFileAreaSigHandler::Init();
}


CFileArea::~CFileArea()
{
	Close();
	CheckError();
}

bool CFileArea::Close()
{
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
		// remove from list
		CFileAreaSigHandler::Remove(*this);
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
	uint64 offEnd = offset + count;
	m_length = offEnd - offStart;
	void *p = mmap(NULL, m_length, PROT_READ, MAP_SHARED, file.fd(), offStart);
	if (p != MAP_FAILED)
	{
		m_mmap_buffer = (byte*) p;
		m_buffer = m_mmap_buffer + (offset - offStart);
		file.Seek(offset + count);

		// add to list to catch errors correctly
		CFileAreaSigHandler::Add(*this);
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

void CFileArea::CheckError()
{
	bool err = m_error;
	m_error = false;
	if (err)
		throw CIOFailureException(wxT("Read error, failed to read from file."));
}

