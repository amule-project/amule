//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include "FileAutoClose.h"	// Needed for CFileAutoClose

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_MMAP
#	if defined(HAVE_SYSCONF) && defined(HAVE__SC_PAGESIZE)
static const long gs_pageSize = sysconf(_SC_PAGESIZE);
#	elif defined(HAVE_SYSCONF) && defined(HAVE__SC_PAGE_SIZE)
static const long gs_pageSize = sysconf(_SC_PAGE_SIZE);
#	elif defined(HAVE_GETPAGESIZE)
static const int gs_pageSize = getpagesize();
#	else
#		error "Should use memory mapped files but don't know how to determine page size!"
#	endif
#endif

#if !defined(HAVE_SIGACTION) || !defined(SA_SIGINFO) || !defined(HAVE_MMAP) || defined(__UCLIBC__)

class CFileAreaSigHandler
{
public:
	static void Init() {};
	static void Add(CFileArea&) {};
	static void Remove(CFileArea&) {};
private:
	CFileAreaSigHandler() {};
};

#else

class CFileAreaSigHandler
{
public:
	static void Init();
	static void Add(CFileArea& area);
	static void Remove(CFileArea& area);
private:
	CFileAreaSigHandler() {};
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

/* define MAP_ANONYMOUS for Mac OS X */
#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

// Handle signals.
// The idea is to replace faulted memory with zeroes and mark
// the error in proper CFileArea
void CFileAreaSigHandler::Handler(int sig, siginfo_t *info, void *ctx)
{
	CFileArea *cur;
	// find the mapped section where violation occurred (if any)
	{
		wxMutexLocker lock(mutex);
		cur = first;
		while (cur) {
			if (cur->m_mmap_buffer && info->si_addr >= cur->m_mmap_buffer && info->si_addr < cur->m_mmap_buffer + cur->m_length)
				break;
			cur = cur->m_next;
		}
	}

	// mark error if found
	if (cur && gs_pageSize > 0) {
		cur->m_error = true;
		char *start_addr = ((char *) info->si_addr) - (((unsigned long) info->si_addr) % gs_pageSize);
		if (mmap(start_addr, gs_pageSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) != MAP_FAILED)
			return;
	}

	// call old handler
	struct sigaction* sa = (sig == SIGSEGV) ? &old_segv : &old_bus;
	if (sa->sa_flags & SA_SIGINFO)
		sa->sa_sigaction(sig, info, ctx);
	else if (sa->sa_handler == SIG_DFL || sa->sa_handler == SIG_IGN)
		abort();
	else
		sa->sa_handler(sig);
}

void CFileAreaSigHandler::Init()
{
	// init error handler if needed
	wxMutexLocker lock(mutex);
	if (initialized)
		return;

	// Set our new signal handler.
	// Note that we safe old handlers (propably wx ones) in order
	// to be able to call them if signal not handled as desired.
	// These handler will be removed by wx code when wx will restore
	// old ones
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
	: m_buffer(NULL), m_mmap_buffer(NULL), m_length(0), m_next(NULL), m_file(NULL), m_error(false)
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
	if (m_mmap_buffer)
	{
		munmap(m_mmap_buffer, m_length);
		// remove from list
		CFileAreaSigHandler::Remove(*this);
		m_buffer = NULL;
		m_mmap_buffer = NULL;
		if (m_file) {
			m_file->Unlock();
			m_file = NULL;
		}
	}
#endif
	return true;
}


void CFileArea::ReadAt(CFileAutoClose& file, uint64 offset, size_t count)
{
	Close();

#ifdef HAVE_MMAP
	uint64 offEnd = offset + count;
	if (gs_pageSize > 0 && offEnd < 0x100000000ull) {
		uint64 offStart = offset & (~((uint64)gs_pageSize-1));
		m_length = offEnd - offStart;
		void *p = mmap(NULL, m_length, PROT_READ, MAP_SHARED, file.fd(), offStart);
		if (p != MAP_FAILED) {
			m_file = &file;
			m_mmap_buffer = (byte*) p;
			m_buffer = m_mmap_buffer + (offset - offStart);

			// add to list to catch errors correctly
			CFileAreaSigHandler::Add(*this);
			return;
		}
	}
	file.Unlock();
#endif
	m_buffer = new byte[count];
	file.ReadAt(m_buffer, offset, count);
}

#ifdef HAVE_MMAP
void CFileArea::StartWriteAt(CFileAutoClose& file, uint64 offset, size_t count)
{
	Close();

	uint64 offEnd = offset + count;
	if (file.GetLength() >= offEnd && gs_pageSize > 0 && offEnd < 0x100000000ull) {
		uint64 offStart = offset & (~((uint64)gs_pageSize-1));
		m_length = offEnd - offStart;
		void *p = mmap(NULL, m_length, PROT_READ|PROT_WRITE, MAP_SHARED, file.fd(), offStart);
		if (p != MAP_FAILED)
		{
			m_file = &file;
			m_mmap_buffer = (byte*) p;
			m_buffer = m_mmap_buffer + (offset - offStart);

			// add to list to catch errors correctly
			CFileAreaSigHandler::Add(*this);
			return;
		}
		file.Unlock();
	}
	m_buffer = new byte[count];
}
#else
void CFileArea::StartWriteAt(CFileAutoClose&, uint64, size_t count)
{
	Close();
	m_buffer = new byte[count];
}
#endif


bool CFileArea::FlushAt(CFileAutoClose& file, uint64 offset, size_t count)
{
	if (!m_buffer)
		return false;

#ifdef HAVE_MMAP
	if (m_mmap_buffer) {
		if (msync(m_mmap_buffer, m_length, MS_SYNC))
			return false;
		Close();
		return true;
	}
#endif
	file.WriteAt(m_buffer, offset, count);
	Close();
	return true;
}

void CFileArea::CheckError()
{
	bool err = m_error;
	m_error = false;
	if (err)
		throw CIOFailureException(wxT("Read error, failed to read from file."));
}

