/*
  Alternative to wxTimer implementation
*/
#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>
#include <wx/wx.h>

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for HAVE_GETRLIMIT, HAVE_SETRLIMIT, HAVE_SYS_RESOURCE_H,
#endif				//   LOCALEDIR, PACKAGE, PACKAGE_STRING and VERSION

#include "types.h"
#include "amule.h"

CTimer::~CTimer()
{
	Stop();
}

CTimer::CTimer(wxEvtHandler *owner, int id)
{
	printf("CTimer:: created  %p with id = %d\n",this, id);
	if ( owner ) {
		SetOwner(owner, id);
	} else {
		SetOwner(&theApp, id);
	}
	thread = 0;
}

void CTimer::SetOwner(wxEvtHandler *owner, int id)
{
	printf("CTimer:: %p owner set, with id = %d\n", this, id);
	CTimer::owner = owner;
	CTimer::id = id;
}

bool CTimer::IsRunning() const
{
	return thread ? true : false;
}

bool CTimer::Start( int millisecs, bool oneShot )
{
	if ( thread ) {
		return false;
	} else {
		thread = new CTimerThread(owner, millisecs, oneShot, id);
		thread->Run();
	}
	return true;
}

void CTimer::Stop()
{
	if ( thread ) {
		thread->Delete();
		thread = 0;
	}
}

CTimer::CTimerThread::CTimerThread(wxEvtHandler *owner,
	unsigned long period, bool oneShot, int id) : wxThread(wxTHREAD_JOINABLE)
{
	m_owner = owner;
	m_period = period;
	m_oneShot = oneShot;
	m_id = id;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("CTimer::CTimerThread: create failed\n");
	}
}

void *CTimer::CTimerThread::Entry()
{
	if ( m_oneShot ) {
		Sleep(m_period);
		wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);
		wxPostEvent(m_owner, evt);
	} else {
		while ( !TestDestroy() ) {
			Sleep(m_period);
			wxMuleInternalEvent evt(wxEVT_AMULE_TIMER, m_id);
			if ( m_id != -1 ) {
				printf("CTimer:: %p sending event with id %d\n", this, m_id);
			}
			wxPostEvent(m_owner, evt);
		}
	}
	return 0;
}

