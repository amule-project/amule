//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
// Copyright (c) 2005 Dévai Tamás ( gonosztopi@amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include "ECSocket.h"

#include <wx/intl.h>			// Needed for i18n
#include <zlib.h>		// Needed for packet (de)compression
#include <cstring>		// Needed for memcpy()/memmove()

#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR
#include "ArchSpecific.h"	// Needed for ENDIAN_NTOHL

#include "ECVersion.h"		// Needed for EC_VERSION_ID
#include "ECCodes.h"		// Needed for the EC_FLAG_* values
#include "ECPacket.h"		// Needed for CECPacket

#include <wx/app.h>		// Needed for wxTheApp

#if ECSOCKET_USE_EVENTS
#	include <wx/event.h>		// Needed for wxEvtHandler
#	include <wx/debug.h>		// Needed for wxCHECK_RET
#	if wxCHECK_VERSION(2,6,0)
#		include <wx/stopwatch.h>	// Needed for wxStopWatch
#	else
#		include <wx/timer.h>		// Needed for wxStopWatch in wxWidgets-2.4.2
#	endif
#	include <wx/utils.h>		// Needed for wxMilliSleep
#endif

#if !wxCHECK_VERSION(2,5,0)
#	if wxUSE_GUI
#		include "wx/gdicmn.h"	// for wxPendingDelete
#	endif
#else
#	include <wx/apptrait.h>	// Needed for wxAppTraits
#endif

#ifdef __DEBUG__
#	include "common/StringFunctions.h"	// Needed for unicode2char()
#endif

#define EC_SOCKET_BUFFER_SIZE	32768
#define EC_COMPRESSION_LEVEL	Z_BEST_COMPRESSION
#define EC_MAX_UNCOMPRESSED	1024

#ifndef __GNUC__
#define __attribute__(x)
#endif

// If your compiler gives errors on these lines, just remove them.
int utf8_mbtowc(wchar_t *p, const unsigned char *s, int n) __attribute__((__visibility__("internal")));
int utf8_wctomb(unsigned char *s, wchar_t wc, int maxlen) __attribute__((__visibility__("internal")));
int utf8_mb_remain(char c) __attribute__((__pure__));

/*----------=> Import from the Linux kernel <=----------*/
/*
 * linux/fs/nls_base.c
 */

/*
 * Sample implementation from Unicode home page.
 * http://www.stonehand.com/unicode/standard/fss-utf.html
 */
struct utf8_table {
	int     cmask;
	int     cval;
	int     shift;
	uint32  lmask;
	uint32  lval;
};

static struct utf8_table utf8_table[] =
{
    {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
    {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
    {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
    {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
    {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
    {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
    {0,     0,      0,      0,              0,         /* end of table    */}
};

int utf8_mbtowc(uint32 *p, const unsigned char *s, int n)
{
	uint32 l;
	int c0, c, nc;
	struct utf8_table *t;

	nc = 0;
	c0 = *s;
	l = c0;
	for (t = utf8_table; t->cmask; t++) {
		nc++;
		if ((c0 & t->cmask) == t->cval) {
			l &= t->lmask;
			if (l < t->lval)
				return -1;
			*p = l;
			return nc;
		}
		if (n <= nc)
			return -1;
		s++;
		c = (*s ^ 0x80) & 0xFF;
		if (c & 0xC0)
			return -1;
		l = (l << 6) | c;
	}
	return -1;
}

int utf8_wctomb(unsigned char *s, uint32 wc, int maxlen)
{
	uint32 l;
	int c, nc;
	struct utf8_table *t;

	l = wc;
	nc = 0;
	for (t = utf8_table; t->cmask && maxlen; t++, maxlen--) {
		nc++;
		if (l <= t->lmask) {
			c = t->shift;
			*s = t->cval | (l >> c);
			while (c > 0) {
				c -= 6;
				s++;
				*s = 0x80 | ((l >> c) & 0x3F);
			}
			return nc;
		}
	}
	return -1;
}
/*----------=> End of Import <=----------*/

int utf8_mb_remain(char c)
{
	int i;
	for (i = 0; i < 5; ++i) {
		if ((c & utf8_table[i].cmask) == utf8_table[i].cval) break;
	}
	return i;
}


#if ECSOCKET_USE_EVENTS

//-------------------- CECSocketHandler --------------------

#define ECSOCKET_SENDPACKET	(GSOCK_MAX_EVENT + 1)
#define ECSOCKET_RECEIVEDPACKET	(GSOCK_MAX_EVENT + 2)
#define ECSOCKET_RECEIVEDDATA	(GSOCK_MAX_EVENT + 3)

#define	EC_SOCKET_HANDLER	(wxID_HIGHEST + 644)

class CECSocketHandler: public wxEvtHandler {
 public:
        CECSocketHandler() {};

 private:
        void SocketHandler(wxSocketEvent& event);

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CECSocketHandler, wxEvtHandler)
        EVT_SOCKET(EC_SOCKET_HANDLER, CECSocketHandler::SocketHandler)
END_EVENT_TABLE()

void CECSocketHandler::SocketHandler(wxSocketEvent& event)
{
        CECSocket *socket = dynamic_cast<CECSocket *>(event.GetSocket());
        wxCHECK_RET(socket, wxT("Socket event with a NULL socket!"));

        switch(event.GetSocketEvent()) {
                case wxSOCKET_LOST:
                        socket->DoLost();
                        break;
                case wxSOCKET_INPUT:
                        socket->DoInput();
                        break;
                case wxSOCKET_OUTPUT:
                        socket->DoOutput();
                        break;
                case wxSOCKET_CONNECTION:
                        socket->DoConnect();
                        break;
		case ECSOCKET_SENDPACKET:
			socket->DoSendPacket();
			break;
		case ECSOCKET_RECEIVEDPACKET:
			socket->HandlePacket();
			break;
		case ECSOCKET_RECEIVEDDATA:
			socket->DoReceivePacket();
			break;
                default:
                        // Nothing should arrive here...
                        wxFAIL;
                        break;
        }
}

static CECSocketHandler	g_ECSocketHandler;


//-------------------- CQueuedData --------------------

class CQueuedData {
      public:
	CQueuedData(const void *data, size_t len);
	~CQueuedData()			{ delete [] m_data; }
	void	Advance(size_t len)
	{
		if (len > m_len) len = m_len;
		m_current += len;
		m_len -= len;
	}
	const void *	GetData()	{ return m_current; }
	size_t		GetLength()	{ return m_len; }
      private:
	const char *	m_data;
	const char *	m_current;
	size_t		m_len;
};

CQueuedData::CQueuedData(const void *data, size_t len)
{
	m_data = new char [len];
	m_current = m_data;
	m_len = len;
	memcpy((void *)m_data, data, len);
}


//-------------------- CECSocket --------------------

#define POST_INPUT_EVENT()                                \
	{                                                 \
		wxSocketEvent event(EC_SOCKET_HANDLER);   \
		event.m_event = wxSOCKET_INPUT;           \
		event.m_clientData = GetClientData();     \
		event.SetEventObject(this);               \
		g_ECSocketHandler.AddPendingEvent(event); \
	}

#if wxUSE_THREADS
#	define PROCESS_EVENTS()                           \
	{                                                 \
		if ( wxThread::IsMain() ) {               \
                        POST_INPUT_EVENT();               \
			wxTheApp->ProcessPendingEvents(); \
		} else                                    \
			wxThread::Yield();                \
	}
#else
#	define PROCESS_EVENTS()                   \
	{                                         \
		POST_INPUT_EVENT();               \
		wxTheApp->ProcessPendingEvents(); \
	}
#endif


#endif /* ECSOCKET_USE_EVENTS */


//
// CECSocket API - User interface functions
//

CECSocket::CECSocket()
	: wxSocketClient(wxSOCKET_NOWAIT)
{
#if wxUSE_THREADS
	m_transfer_in_progress = false;
#endif
	m_firsttransfer = true;
	m_my_flags = 0x20 | EC_FLAG_ZLIB | EC_FLAG_UTF8_NUMBERS
#if ECSOCKET_USE_EVENTS
		| EC_FLAG_HAS_ID
#endif
		;
	m_accepts = 0;
	m_in_ptr = NULL;
	m_out_ptr = NULL;
	m_used_flags = 0;
	m_z.zalloc = Z_NULL;
	m_z.zfree = Z_NULL;
	m_z.opaque = Z_NULL;
	m_lastError = wxSOCKET_NOERROR;
	m_destroying = false;
	m_isWorking = false;
#if ECSOCKET_USE_EVENTS
	m_canUseIDs = false;
	m_canSend = IsConnected();
	m_id = 0;
	SetEventHandler(g_ECSocketHandler, EC_SOCKET_HANDLER);
	SetNotify(wxSOCKET_CONNECTION_FLAG |
		  wxSOCKET_INPUT_FLAG |
		  wxSOCKET_OUTPUT_FLAG |
		  wxSOCKET_LOST_FLAG);
	Notify(true);
#endif
}

CECSocket::~CECSocket()
{
	if (m_in_ptr) {
		delete [] m_in_ptr;
	}
	if (m_out_ptr) {
		delete [] m_out_ptr;
	}

#if ECSOCKET_USE_EVENTS
	while (!m_input_queue.empty()) {
		CQueuedData *data = m_input_queue.front();
		m_input_queue.pop_front();
		delete data;
	}

	while (!m_output_queue.empty()) {
		CQueuedData *data = m_output_queue.front();
		m_output_queue.pop_front();
		delete data;
	}

	while (!m_output_packet_queue.empty()) {
		packet_desc data = m_output_packet_queue.front();
		m_output_queue.pop_front();
		delete data.first;
	}

	m_waited_ids.clear();
	m_abandoned_ids.clear();
#endif
}

void CECSocket::SendPacket(const CECPacket *packet)
{
#if ECSOCKET_USE_EVENTS
	wxENTER_CRIT_SECT(m_cs_packet_out);

	m_output_packet_queue.push_back(packet_desc(new CECPacket(*packet), 0));

	wxLEAVE_CRIT_SECT(m_cs_packet_out);

	wxSocketEvent event(EC_SOCKET_HANDLER);
	event.m_event = (wxSocketNotify)ECSOCKET_SENDPACKET;
	event.m_clientData = GetClientData();
	event.SetEventObject(this);
	g_ECSocketHandler.AddPendingEvent(event);
#else
	m_isWorking = true;
	WritePacket(packet);
	delete ReadPacket();
	m_isWorking = false;
	CheckDestroy();
#endif
}

const CECPacket *CECSocket::SendRecvPacket(const CECPacket *packet)
{
	const CECPacket *reply = NULL;
	m_isWorking = true;

#if ECSOCKET_USE_EVENTS
	wxENTER_CRIT_SECT(m_cs_packet_out);
	uint32 id = m_canUseIDs ? ++m_id : 0;
	m_output_packet_queue.push_front(packet_desc(new CECPacket(*packet), id));
	wxLEAVE_CRIT_SECT(m_cs_packet_out);

	wxENTER_CRIT_SECT(m_cs_id_list);
	m_waited_ids.insert(id);
	wxLEAVE_CRIT_SECT(m_cs_id_list);

	DoSendPacket();

	wxStopWatch swatch;
	while (!reply) {
		wxMilliSleep(100);
		PROCESS_EVENTS();
		wxENTER_CRIT_SECT(m_cs_waited_packets);
		for (std::deque<packet_desc>::iterator it = m_waited_packets.begin(); it != m_waited_packets.end(); ++it) {
			if (it->second == id) {
				reply = it->first;
				m_waited_packets.erase(it);
				break;
			}
		}
		wxLEAVE_CRIT_SECT(m_cs_waited_packets);
		if (reply) break;
		// Wait at most 10 sec
		if (swatch.Time() > 10000) {
			if (id != 0) {
				wxENTER_CRIT_SECT(m_cs_id_list);
				m_abandoned_ids.insert(id);
				m_waited_ids.erase(id);
				wxLEAVE_CRIT_SECT(m_cs_id_list);
			}
			m_lastError = wxSOCKET_TIMEDOUT;
			OnError();
			break;
		}
	}

#else
	WritePacket(packet);
	reply = ReadPacket();
#endif
	m_isWorking = false;
	CheckDestroy();
	return reply;
}

wxString CECSocket::GetErrorMsg(wxSocketError code)
{
	switch(code) {
		case wxSOCKET_NOERROR:
			return wxT("No error happened");
		case wxSOCKET_INVOP:
			return wxT("Invalid operation");
		case wxSOCKET_IOERR:
			return wxT("Input/Output error");
		case wxSOCKET_INVADDR:
			return wxT("Invalid address passed to wxSocket");
		case wxSOCKET_INVSOCK:
			return wxT("Invalid socket (uninitialized)");
		case wxSOCKET_NOHOST:
			return wxT("No corresponding host");
		case wxSOCKET_INVPORT:
			return wxT("Invalid port");
		case wxSOCKET_WOULDBLOCK:
			return wxT("The socket is non-blocking and the operation would block");
		case wxSOCKET_TIMEDOUT:
			return wxT("The timeout for this operation expired");
		case wxSOCKET_MEMERR:
			return wxT("Memory exhausted");
		case wxSOCKET_DUMMY:
			return wxT("Dummy code - should not happen");
	}
	return wxString::Format(wxT("Error code 0x%08x unknown"), code);
}

void CECSocket::OnError()
{
#ifdef __DEBUG__
	printf("CECSocket error: %s\n", (const char *)unicode2char(GetErrorMsg(GetLastError())));
#endif
	Destroy(true);
}

void CECSocket::OnLost()
{
	Destroy();
}

void CECSocket::Destroy(bool raiseLostEvent)
{
	if (!m_destroying) {
		m_destroying = true;
		Close();
		Notify(false);
		if (raiseLostEvent) {
			OnLost();
		}
		if (!m_isWorking) {
			// Do the real thing if it's safe.
			CheckDestroy();
		}
	}
}

//
// Implementation from now on
//

void CECSocket::CheckDestroy()
{
	if (m_destroying) {
#if !wxCHECK_VERSION(2,5,0)
#	if wxUSE_GUI
		if ( !wxPendingDelete.Member(this) )
			wxPendingDelete.Append(this);
#	else
		delete this;
#	endif
#else
		wxAppTraits *traits = wxTheApp ? wxTheApp->GetTraits() : NULL;
		if (traits) {
			traits->ScheduleForDestroy(this);
		} else {
			delete this;
		}
#endif
	}
}

#if ECSOCKET_USE_EVENTS

//
// Event handlers
//

void CECSocket::DoInput()
{
	char *buffer = new char [EC_SOCKET_BUFFER_SIZE];
	Read(buffer, EC_SOCKET_BUFFER_SIZE);
	if (Error()) {
		m_lastError = LastError();
		if (m_lastError != wxSOCKET_WOULDBLOCK) {
			OnError();
			delete [] buffer;
			return;
		} else {
			m_lastError = wxSOCKET_NOERROR;
		}
	}
	size_t LastIO = LastCount();
	if (LastIO > 0) {
		CQueuedData *data = new CQueuedData(buffer, LastIO);
		m_input_queue.push_back(data);

		wxSocketEvent event(EC_SOCKET_HANDLER);
		event.m_event = (wxSocketNotify)ECSOCKET_RECEIVEDDATA;
		event.m_clientData = GetClientData();
		event.SetEventObject(this);
		g_ECSocketHandler.AddPendingEvent(event);
	}
	delete [] buffer;
}

void CECSocket::DoOutput()
{
	while (!m_output_queue.empty()) {
		CQueuedData* data = m_output_queue.front();
		Write(data->GetData(), data->GetLength());
		data->Advance(LastCount());
		if (!data->GetLength()) {
			m_output_queue.pop_front();
			delete data;
		}
		if (Error()) {
			m_lastError = LastError();
			if (m_lastError == wxSOCKET_WOULDBLOCK) {
				m_lastError = wxSOCKET_NOERROR;
				return;
			} else {
				OnError();
				return;
			}
		}
	}
	m_canSend = true;
}

void CECSocket::DoReceivePacket()
{
#if wxUSE_THREADS
	if (m_transfer_in_progress || (m_transfer_lock.TryLock() == wxMUTEX_BUSY)) {
		return;
	}
	m_transfer_in_progress = true;
	m_transfer_lock.Unlock();
#endif

	if (!m_input_queue.empty()) {
		ReadPacket();
		if (!m_input_queue.empty()) {
			wxSocketEvent event(EC_SOCKET_HANDLER);
			event.m_event = (wxSocketNotify)ECSOCKET_RECEIVEDDATA;
			event.m_clientData = GetClientData();
			event.SetEventObject(this);
			g_ECSocketHandler.AddPendingEvent(event);
		}
	}

#if wxUSE_THREADS
	m_transfer_in_progress = false;
#endif
}

void CECSocket::HandlePacket()
{
	packet_desc packet = m_input_packet_queue.front();
	m_input_packet_queue.pop_front();

	if (m_canUseIDs) {
		wxENTER_CRIT_SECT(m_cs_id_list);
		if (m_abandoned_ids.erase(packet.second) > 0) {
			delete packet.first;
		} else if (m_waited_ids.erase(packet.second) > 0) {
			wxENTER_CRIT_SECT(m_cs_waited_packets);
			m_waited_packets.push_back(packet);
			wxLEAVE_CRIT_SECT(m_cs_waited_packets);
		} else {
			wxLEAVE_CRIT_SECT(m_cs_id_list);
			const CECPacket *reply = OnPacketReceived(packet.first);
			if (packet.second) {
				if (!reply) {
					reply = new CECPacket(EC_OP_NOOP);
				}
				wxENTER_CRIT_SECT(m_cs_packet_out);
				m_output_packet_queue.push_front(packet_desc(reply, packet.second));
				wxLEAVE_CRIT_SECT(m_cs_packet_out);
				wxSocketEvent event(EC_SOCKET_HANDLER);
				event.m_event = (wxSocketNotify)ECSOCKET_SENDPACKET;
				event.m_clientData = GetClientData();
				event.SetEventObject(this);
				g_ECSocketHandler.AddPendingEvent(event);
			} else {
				delete reply;
			}
			wxENTER_CRIT_SECT(m_cs_id_list);
		}
		wxLEAVE_CRIT_SECT(m_cs_id_list);
	} else {
		wxENTER_CRIT_SECT(m_cs_id_list);
		if (m_waited_ids.empty()) {
			wxLEAVE_CRIT_SECT(m_cs_id_list);
			const CECPacket *reply = OnPacketReceived(packet.first);
			if (!reply) {
				reply = new CECPacket(EC_OP_NOOP);
			}
			wxENTER_CRIT_SECT(m_cs_packet_out);
			m_output_packet_queue.push_front(packet_desc(reply, packet.second));
			wxLEAVE_CRIT_SECT(m_cs_packet_out);
			wxSocketEvent event(EC_SOCKET_HANDLER);
			event.m_event = (wxSocketNotify)ECSOCKET_SENDPACKET;
			event.m_clientData = GetClientData();
			event.SetEventObject(this);
			g_ECSocketHandler.AddPendingEvent(event);
			wxENTER_CRIT_SECT(m_cs_id_list);
		} else {
			wxENTER_CRIT_SECT(m_cs_waited_packets);
			m_waited_packets.push_back(packet);
			wxLEAVE_CRIT_SECT(m_cs_waited_packets);
			m_waited_ids.erase(m_waited_ids.begin());
		}
		wxLEAVE_CRIT_SECT(m_cs_id_list);
	}

	m_canUseIDs = (m_accepts & EC_FLAG_HAS_ID);
}

void CECSocket::DoSendPacket()
{
#if wxUSE_THREADS
	if (m_transfer_in_progress || (m_transfer_lock.TryLock() == wxMUTEX_BUSY)) {
		return;
	}
	m_transfer_in_progress = true;
	m_transfer_lock.Unlock();
#endif

	wxENTER_CRIT_SECT(m_cs_packet_out);
	if (!m_output_packet_queue.empty()) {
		packet_desc this_packet = m_output_packet_queue.front();
		m_output_packet_queue.pop_front();
		wxLEAVE_CRIT_SECT(m_cs_packet_out);
		WritePacket(this_packet);
		delete this_packet.first;
		wxENTER_CRIT_SECT(m_cs_packet_out);
		if (!m_output_packet_queue.empty()) {
			wxSocketEvent event(EC_SOCKET_HANDLER);
			event.m_event = (wxSocketNotify)ECSOCKET_SENDPACKET;
			event.m_clientData = GetClientData();
			event.SetEventObject(this);
			g_ECSocketHandler.AddPendingEvent(event);
		}
	}
	wxLEAVE_CRIT_SECT(m_cs_packet_out);

#if wxUSE_THREADS
	m_transfer_in_progress = false;
#endif
}


//
// Socket I/O
//

bool CECSocket::CanReadNBytes(size_t len)
{
	if (len == 0) return true;
	for (std::deque<CQueuedData*>::iterator it = m_input_queue.begin(); it != m_input_queue.end(); ++it) {
		if ((*it)->GetLength() >= len) {
			return true;
		} else {
			len -= (*it)->GetLength();
		}
	}
	return false;
}

#endif /* ECSOCKET_USE_EVENTS */

size_t CECSocket::ReadBufferFromSocket(void *buffer, size_t required_len, size_t max_len)
{
	char *iobuf = (char *)buffer;
	size_t ReadBytes = 0;

#if ECSOCKET_USE_EVENTS
	wxStopWatch swatch;
	// Wait at most 10 seconds
	while (!CanReadNBytes(required_len) && (swatch.Time() < 10000)) {
		// GTK's event processing is halted as long as we're in an event handler.
		// So, we cannot wait for a wxSOCKET_INPUT event. Therefore we could use the
		// WaitForXXX functions with a 0 timeout, so wxYield wouldn't get called,
		// or post a fake wxSOCKET_INPUT event and prepare the handler for it.
		wxMilliSleep(100);
		PROCESS_EVENTS();
	}
	if (!CanReadNBytes(required_len)) {
		m_lastError = wxSOCKET_TIMEDOUT;
		OnError();
		return 0;
	}

	// If required_len == 0, CanReadNBytes() returns immediately with true.
	if (m_input_queue.empty()) {
		return 0;
	}

	do {
		CQueuedData *data = m_input_queue.front();
		size_t len = data->GetLength();
		if (len > max_len) len = max_len;
		memcpy(iobuf, data->GetData(), len);
		data->Advance(len);
		iobuf += len;
		if (len > required_len) {
			required_len = 0;
		} else {
			required_len -= len;
		}
		max_len -= len;
		ReadBytes += len;
		if (!data->GetLength()) {
			m_input_queue.pop_front();
			delete data;
		}
	} while (required_len > 0);
	m_lastError = wxSOCKET_NOERROR;
#else
	do {
		//
		// Give socket a 10 sec chance to recv more data.
		if (required_len > 0) {
			if ( !WaitForRead(10, 0) ) {
				m_lastError = LastError();
				OnError();
				break;
			}
		}
		Read(iobuf, max_len);
		size_t LastIO = LastCount();
		ReadBytes += LastIO;
		iobuf += LastIO;
		max_len -= LastIO;
		if (Error()) {
			m_lastError = LastError();
			if (m_lastError == wxSOCKET_WOULDBLOCK) {
				m_lastError = wxSOCKET_NOERROR;
			} else {
				OnError();
				break;
			}
		}
	} while (ReadBytes < required_len);
#endif
	return ReadBytes;
}

void CECSocket::WriteBufferToSocket(const void *buffer, size_t len)
{
	const char *iobuf = (const char *)buffer;

#if ECSOCKET_USE_EVENTS
	if (m_canSend) {
		Write(buffer, len);
		size_t sentBytes = LastCount();
		if (Error()) {
			m_lastError = LastError();
			if (m_lastError == wxSOCKET_WOULDBLOCK) {
				m_lastError = wxSOCKET_NOERROR;
				m_canSend = false;
			} else {
				OnError();
				// Hmmmm.....
			}
		}
		if (sentBytes == len) {
			return;
		} else {
			iobuf += sentBytes;
			len -= sentBytes;
		}
	}
	CQueuedData *data = new CQueuedData(iobuf, len);
	m_output_queue.push_back(data);
#else
	while (len > 0) {
		//
		// Give socket a 10 sec chance to send more data.
		if ( !WaitForWrite(10, 0) ) {
			m_lastError = LastError();
			if (m_lastError == wxSOCKET_WOULDBLOCK) {
				m_lastError = wxSOCKET_NOERROR;
				continue;
			} else {
				OnError();
				break;
			}
		}
		Write(iobuf, len);
		size_t LastIO = LastCount();
		len -= LastIO;
		iobuf += LastIO;
		if (Error()) {
			m_lastError = LastError();
			if (m_lastError == wxSOCKET_WOULDBLOCK) {
				m_lastError = wxSOCKET_NOERROR;
			} else {
				OnError();
				break;
			}
		}
	}
#endif
}


//
// ZLib "error handler"
//

#ifdef __DEBUG__
void ShowZError(int zerror, z_streamp strm)
{
	char *p = NULL;

	switch (zerror) {
		case Z_STREAM_END: p = "Z_STREAM_END"; break;
		case Z_NEED_DICT: p = "Z_NEED_DICT"; break;
		case Z_ERRNO: p = "Z_ERRNO"; break;
		case Z_STREAM_ERROR: p = "Z_STREAM_ERROR"; break;
		case Z_DATA_ERROR: p = "Z_DATA_ERROR"; break;
		case Z_MEM_ERROR: p = "Z_MEM_ERROR"; break;
		case Z_BUF_ERROR: p = "Z_BUF_ERROR"; break;
		case Z_VERSION_ERROR: p = "Z_VERSION_ERROR"; break;
	}
	printf("ZLib operation returned %s\n", p);
	printf("ZLib error message: %s\n", strm->msg);
	printf("zstream state:\n\tnext_in=%p\n\tavail_in=%u\n\ttotal_in=%lu\n\tnext_out=%p\n\tavail_out=%u\n\ttotal_out=%lu\n",
		strm->next_in, strm->avail_in, strm->total_in, strm->next_out, strm->avail_out, strm->total_out);
}
#else
#define ShowZError(a, b)
#endif


//
// Working buffers
//

void CECSocket::InitBuffers()
{
	if (!m_in_ptr) {
		m_in_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	}
	if (!m_out_ptr) {
		m_out_ptr = new unsigned char[EC_SOCKET_BUFFER_SIZE];
	}

	wxASSERT(m_in_ptr && m_out_ptr);

	m_z.next_in = m_in_ptr;
	m_z.avail_in = 0;
	m_z.total_in = 0;
	m_z.next_out = m_out_ptr;
	m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
	m_z.total_out = 0;
}

bool CECSocket::ReadNumber(void *buffer, size_t len)
{
	if (m_used_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32 wc;
		if (!ReadBuffer(mb, 1)) return false;
		int remains = utf8_mb_remain(mb[0]);
		if (remains) if (!ReadBuffer(&(mb[1]), remains)) return false;
		if (utf8_mbtowc(&wc, mb, 6) == -1) return false;	// Invalid UTF-8 code sequence
		switch (len) {
			case 1: PokeUInt8( buffer,  wc ); break;
			case 2:	RawPokeUInt16( buffer, wc ); break;
			case 4: RawPokeUInt32( buffer, wc ); break;
		}
	} else {
		if ( !ReadBuffer(buffer, len) ) {
			return false;
		}
		switch (len) {
			case 2:
				RawPokeUInt16( buffer, ENDIAN_NTOHS( RawPeekUInt16( buffer ) ) );
				break;
			case 4:
				RawPokeUInt32( buffer, ENDIAN_NTOHL( RawPeekUInt32( buffer ) ) );
				break;
		}
	}
	return true;
}

bool CECSocket::WriteNumber(const void *buffer, size_t len)
{
	if (m_used_flags & EC_FLAG_UTF8_NUMBERS) {
		unsigned char mb[6];
		uint32 wc = 0;
		int mb_len;
		switch (len) {
			case 1: wc = PeekUInt8( buffer ); break;
			case 2: wc = RawPeekUInt16( buffer ); break;
			case 4: wc = RawPeekUInt32( buffer ); break;
			default: return false;
		}
		if ((mb_len = utf8_wctomb(mb, wc, 6)) == -1) return false;	// Something is terribly wrong...
		return WriteBuffer(mb, mb_len);
	} else {
		char tmp[8];

		switch (len) {
			case 1: PokeUInt8( tmp, PeekUInt8( buffer ) ); break;
			case 2: RawPokeUInt16( tmp, ENDIAN_NTOHS( RawPeekUInt16( buffer ) ) ); break;
			case 4: RawPokeUInt32( tmp, ENDIAN_NTOHL( RawPeekUInt32( buffer ) ) ); break;
		}
		return WriteBuffer(tmp, len);
	}
}

bool CECSocket::ReadBuffer(void *buffer, size_t len)
{
	if (m_used_flags & EC_FLAG_ZLIB) {
		// using zlib compressed i/o
		// Here we pretty much misuse the z.next_out and z.avail_out values,
		// but it doesn't matter as long a we always call inflate() with an
		// empty output buffer.
		while (m_z.avail_out < len) {
			if (m_z.avail_out) {
				memcpy(buffer, m_z.next_out, m_z.avail_out);
				buffer = (unsigned char *)buffer + m_z.avail_out;
				len -= m_z.avail_out;
			}
			// consumed all output
			m_z.next_out = m_out_ptr;
			m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
			unsigned min_read = 1;
			if (m_z.avail_in) {
				memmove(m_in_ptr, m_z.next_in, m_z.avail_in);
				min_read = 0;
			}
			m_z.next_in = m_in_ptr;
			m_z.avail_in += ReadBufferFromSocket(m_z.next_in + m_z.avail_in, min_read, EC_SOCKET_BUFFER_SIZE - m_z.avail_in);
			if (GetLastError() != wxSOCKET_NOERROR) {
				return false;
			}
			int zerror = inflate(&m_z, Z_SYNC_FLUSH);
			if ((zerror != Z_OK) && (zerror != Z_STREAM_END)) {
				ShowZError(zerror, &m_z);
				return false;
			}
			m_z.next_out = m_out_ptr;
			m_z.avail_out = EC_SOCKET_BUFFER_SIZE - m_z.avail_out;
		}
		memcpy(buffer, m_z.next_out, len);
		m_z.next_out += len;
		m_z.avail_out -= len;
		return true;
	} else {
		// using uncompressed buffered i/o
		if (m_z.avail_in < len) {
			// get more data
			if (m_z.avail_in) {
				memcpy(buffer, m_z.next_in, m_z.avail_in);
				len -= m_z.avail_in;
				buffer = (Bytef*)buffer + m_z.avail_in;
			}
			if (len >= EC_SOCKET_BUFFER_SIZE) {
				// read directly to app mem, to avoid unnecessary memcpy()s
				m_z.avail_in = 0;
				m_z.next_in = m_in_ptr;
				return ReadBufferFromSocket(buffer, len, len) == len;
			} else {
				m_z.avail_in = ReadBufferFromSocket(m_in_ptr, len, EC_SOCKET_BUFFER_SIZE);
				m_z.next_in = m_in_ptr;
				if (GetLastError() != wxSOCKET_NOERROR) {
					m_z.avail_in = 0;
					return false;
				}
			}
		}
		memcpy(buffer, m_z.next_in, len);
		m_z.next_in += len;
		m_z.avail_in -= len;
		return true;
	}
}

bool CECSocket::WriteBuffer(const void *buffer, size_t len)
{
	unsigned int remain_in;

	if (m_used_flags & EC_FLAG_ZLIB) {
		// using zlib compressed i/o
		while ((remain_in = (EC_SOCKET_BUFFER_SIZE - (m_z.next_in - m_in_ptr) - m_z.avail_in)) < len) {
			memcpy(m_z.next_in + m_z.avail_in, buffer, remain_in);
			buffer = (Bytef*)buffer + remain_in;
			len -= remain_in;
			m_z.avail_in += remain_in;
			int zerror = deflate(&m_z, Z_NO_FLUSH);
			if ( zerror != Z_OK ) {
				ShowZError(zerror, &m_z);
				return false;
			}
			if (!m_z.avail_out) {
				WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE);
				if (GetLastError() != wxSOCKET_NOERROR) {
					return false;
				}
				m_z.next_out = m_out_ptr;
				m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
			}
			if (m_z.next_in != m_in_ptr) {
				if (m_z.avail_in) {
					memmove(m_in_ptr, m_z.next_in, m_z.avail_in);
				}
				m_z.next_in = m_in_ptr;
			}
		}
		memcpy(m_z.next_in + m_z.avail_in, buffer, len);
		m_z.avail_in += len;
		return true;
	} else {
		// using uncompressed buffered i/o
		if (m_z.avail_out < len) {
			// send some data
			if (m_z.avail_out) {
				memcpy(m_z.next_out, buffer, m_z.avail_out);
				len -= m_z.avail_out;
				buffer = (Bytef*)buffer + m_z.avail_out;
			}
			m_z.next_out = m_out_ptr;
			m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
			WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE);
			if (GetLastError() != wxSOCKET_NOERROR) {
				return false;
			}
			if (len >= EC_SOCKET_BUFFER_SIZE) {
				// direct write from app mem, to avoid unnecessary memcpy()s
				WriteBufferToSocket(buffer, len);
				return true;
			}
		}
		memcpy(m_z.next_out, buffer, len);
		m_z.next_out += len;
		m_z.avail_out -= len;
		return true;
	}
}

bool CECSocket::FlushBuffers()
{
	if (m_used_flags & EC_FLAG_ZLIB) {
		int zerror;
		do {
			zerror = deflate(&m_z, Z_FINISH);
			WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
			m_z.next_out = m_out_ptr;
			m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
		} while (zerror == Z_OK);
		if (zerror == Z_STREAM_END) return true;
		else {
			ShowZError(zerror, &m_z);
			return false;
		}
	} else {
		if (m_z.avail_out != EC_SOCKET_BUFFER_SIZE) {
			WriteBufferToSocket(m_out_ptr, EC_SOCKET_BUFFER_SIZE - m_z.avail_out);
			m_z.next_out = m_out_ptr;
			m_z.avail_out = EC_SOCKET_BUFFER_SIZE;
		}
	}
	// report success if there's nothing to do
	return true;
}

uint32 CECSocket::ReadFlags()
{
	int i = 0;
	uint32 flags = 0;
	uint8 b;

	do {
		if (!ReadBufferFromSocket(&b, 1, 1)) return 0;
		flags += (uint32)b << i;
		i += 8;
	} while ((b & 0x80) && (i < 32));
	return flags;
}

void CECSocket::WriteFlags(uint32 flags)
{
	uint8 b;

	flags |= 0x20;		// Setting bit 5
	flags &= 0xffffffbf;	// Clearing bit 6
	do {
		b = flags & 0xff;
		flags >>= 8;
		if (flags) b |= 0x80;
		WriteBufferToSocket(&b, 1);
	} while (flags);
}


//
// Packet I/O
//

#if ECSOCKET_USE_EVENTS
void CECSocket::WritePacket(packet_desc packet)
#define PACKET	(packet.first)
#else
void CECSocket::WritePacket(const CECPacket *packet)
#define PACKET	(packet)
#endif
{
	uint32 flags = 0x20;

	if ((PACKET->GetPacketLength() > EC_MAX_UNCOMPRESSED) && (m_my_flags & EC_FLAG_ZLIB)) {
		flags |= EC_FLAG_ZLIB;
	} else {
		flags |= EC_FLAG_UTF8_NUMBERS;
	}

#if ECSOCKET_USE_EVENTS
	if (packet.second != 0) flags |= EC_FLAG_HAS_ID;
#endif

	InitBuffers();

	// ensure we won't use anything the other end cannot accept
	flags &= m_accepts;

	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateInit(&m_z, EC_COMPRESSION_LEVEL);
		if (zerror != Z_OK) {
			// don't use zlib if init failed
			flags &= ~EC_FLAG_ZLIB;
			ShowZError(zerror, &m_z);
		}
	}

	m_used_flags = flags;

	if (m_firsttransfer) {
		m_firsttransfer = false;
		flags |= EC_FLAG_ACCEPTS;
		WriteFlags(flags);
		WriteFlags(m_my_flags);
	} else {
		WriteFlags(flags);
	}

#if ECSOCKET_USE_EVENTS
	if (flags & EC_FLAG_HAS_ID) {
		WriteBuffer(&packet.second, 4);
	}
#endif

	PACKET->WritePacket(*this);

	FlushBuffers();

	if (flags & EC_FLAG_ZLIB) {
		int zerror = deflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &m_z);
			return;
		}
	}
}


#if ECSOCKET_USE_EVENTS
void CECSocket::ReadPacket()
#define INVALID_PACKET
#else
const CECPacket *CECSocket::ReadPacket()
#define INVALID_PACKET	NULL
#endif
{
	CECPacket *tmp_packet;
#if ECSOCKET_USE_EVENTS
	uint32 tmp_id = 0;
#endif

	InitBuffers();

	uint32 flags = ReadFlags();

	if ((flags & 0x60) != 0x20) {
		// Protocol error - other end might use an older protocol
		Close();
		return INVALID_PACKET;
	}

	// check if the other end sends an "accepts" value
	if (flags & EC_FLAG_ACCEPTS) {
		m_accepts = ReadFlags();
	}

	if (flags & EC_FLAG_UNKNOWN_MASK) {
		// Received a packet with an unknown extension
		Close();
		return INVALID_PACKET;
	}

	if (((flags & m_my_flags) ^ flags) & ~EC_FLAG_ACCEPTS) {
		// Received packet uses a flag that we cannot accept
		Close();
		return INVALID_PACKET;
	}

	m_used_flags = flags;
	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateInit(&m_z);
		if (zerror != Z_OK) {
			// unable to uncompress compressed input
			ShowZError(zerror, &m_z);
			Close();
			return INVALID_PACKET;
		} else {
			// misusing z parameters - read more at ReadBuffer()
			m_z.avail_out = 0;
		}
	}

#if ECSOCKET_USE_EVENTS
	if (flags & EC_FLAG_HAS_ID) {
		ReadBuffer(&tmp_id, 4);
	}
#endif

	tmp_packet = new CECPacket(*this);
#ifndef KEEP_PARTIAL_PACKETS
	if (tmp_packet->m_error != 0) {
		delete tmp_packet;
		tmp_packet = NULL;
		Close();
	}
#endif
	if (flags & EC_FLAG_ZLIB) {
		int zerror = inflateEnd(&m_z);
		if ( zerror != Z_OK ) {
			ShowZError(zerror, &m_z);
			Close();
		}
	}

#if ECSOCKET_USE_EVENTS
	if (tmp_packet) {
		m_input_packet_queue.push_back(packet_desc(tmp_packet, tmp_id));
		wxSocketEvent event(EC_SOCKET_HANDLER);
		event.m_event = (wxSocketNotify)ECSOCKET_RECEIVEDPACKET;
		event.m_clientData = GetClientData();
		event.SetEventObject(this);
		g_ECSocketHandler.AddPendingEvent(event);
	}
#else
	return tmp_packet;
#endif
}
