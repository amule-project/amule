// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Froenchenko Leonid (lfroen@users.sourceforge.net)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include <assert.h>
#include <stdio.h>
#include <unistd.h>           // Needed for close(2) and sleep(3)
#include <wx/defs.h>

#ifdef __WXMSW__
	#include <winsock.h>
	#include <wx/msw/winundef.h>
#else
	#ifdef __BSD__
     	  #include <sys/types.h>
	#endif /* __BSD__ */
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif
#include <wx/app.h>
#include <wx/config.h>
#include <wx/socket.h>		// Needed for wxSocket
#include <wx/utils.h>
#include <wx/timer.h>

#include "types.h"		// Needed for int32, uint16 and uint64
#include "GuiEvents.h"

#include "PartFile.h"
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR

//int sock_flags = wxSOCKET_WAITALL|wxSOCKET_BLOCK|wxSOCKET_REUSEADDR;
int sock_flags = wxSOCKET_NONE|wxSOCKET_REUSEADDR;

char *SockStrError(wxSocketError e)
{
	switch(e) {
	case wxSOCKET_NOERROR : return "wxSOCKET_NOERROR";
	case wxSOCKET_INVOP : return "wxSOCKET_INVOP";
	case wxSOCKET_IOERR : return "wxSOCKET_IOERR";
	case wxSOCKET_INVADDR : return "wxSOCKET_INVADDR";
	case wxSOCKET_INVSOCK : return "wxSOCKET_INVSOCK";
	case wxSOCKET_NOHOST : return "wxSOCKET_NOHOST";
	case wxSOCKET_INVPORT : return "wxSOCKET_INVPORT";
	case wxSOCKET_WOULDBLOCK : return "wxSOCKET_WOULDBLOCK";
	case wxSOCKET_TIMEDOUT : return "wxSOCKET_TIMEDOUT";
	case wxSOCKET_MEMERR : return "wxSOCKET_MEMERR";
	case wxSOCKET_DUMMY : return "wxSOCKET_DUMMY";
	}
	return "UNKNOWN";
}

void dump_mem_b(void *ptr, int size)
{
	byte *cp = (byte *)ptr;
	printf("%p [", ptr);
	for(int i = 0;i < size;i++) {
		printf(" %02x ", cp[i]);
	}
	printf("]\n");
}



//
// Server - thread running in core side
//
PtrsXferServer::PtrsXferServer(wxEvtHandler *handler, int port)
{
	wxIPV4address addr;
	addr.Service(port);
	sock = new wxSocketServer(addr);
	sock->SetFlags(sock_flags);
	sock->Notify(FALSE);
	if ( !sock->Ok() ) {
		printf("Could not listen at the specified port !\n\n");
		delete sock;
		sock = 0;
	} else {
		printf("Server listening.\n\n");
	}
	PtrsXferServer::handler = handler;

	if ( Create() != wxTHREAD_NO_ERROR ) {
		sock->Close();
		delete sock;
		sock = 0;
	}

	if ( Run()  != wxTHREAD_NO_ERROR ) {
		sock->Close();
		delete sock;
		sock = 0;
	}
}

PtrsXferServer::~PtrsXferServer()
{
	for(std::list<PtrsXferServerCliThread *>::iterator it = clients.begin();
	    it != clients.end();it++) {
		(*it)->Delete();
	}
}

void PtrsXferServer::SendNotify(GUIEvent &evt)
{
	for(std::list<PtrsXferServerCliThread *>::iterator it = clients.begin();
	    it != clients.end();it++) {
		(*it)->SendNotify(evt);
	}
}

void *PtrsXferServer::Entry()
{
	wxSocketServer *server = (wxSocketServer *)sock;
	while ( !TestDestroy() ) {
		if ( server->WaitForAccept() ) {
			wxSocketBase *client = server->Accept(FALSE);
			if ( client ) {
				printf("PtrsXferServer: New client connection accepted\n\n");
				client->Notify(FALSE);
				client->SetFlags(sock_flags);
				PtrsXferServerCliThread *cli_thread = new PtrsXferServerCliThread(client, this);
				if ( (cli_thread->Create() == wxTHREAD_NO_ERROR) &&
				     (cli_thread->Run() == wxTHREAD_NO_ERROR) ) {
					clients.push_back(cli_thread);
					wxCoreNewClent evt(cli_thread);
					handler->AddPendingEvent(evt);
				} else {
					printf("PtrsXferServer: thread creation error\n");
					delete cli_thread;
				}
			}
		}
	}
	sock->Destroy();
	return 0;
}


void PtrsXferServer::OnClientGone(PtrsXferServerCliThread *client)
{
	
}


//
// core-side per-client thread
//
PtrsXferServerCliThread::PtrsXferServerCliThread(wxSocketBase *sk, PtrsXferServer *srv) :
	wxThread(wxTHREAD_DETACHED)
{
	sock = sk;
	server = srv;

	// wait for client
	Notify_Gui_Login ev;
	if ( !sock->WaitForRead(5) ) {
		printf("PtrsXferServerCliThread: client doesnt send login\n");
		sock->Destroy();
		sock = 0;
		return;
	}
	sock->Read(&ev, sizeof(ev));
	printf("PtrsXferServerCliThread: client [%s] waiting on %d\n", ev.user, ev.port);
	if ( ev.cmd != PTR_XFER_AUTH ) {
		printf("PtrsXferServerCliThread: client send invalid login\n");
		sock->Destroy();
		sock = 0;
		return;		
	}

	// open backward connection
	wxIPV4address addr;
	if ( !sock->GetPeer(addr) ) {
		printf("PtrsXferServerCliThread: cant get peer address\n");
		assert(0);
	}
	addr.Service(ev.port);
	notif_sock = new wxSocketClient();

	notif_sock->Connect(addr, FALSE);

	if ( !notif_sock->WaitOnConnect(60) ) {
		printf("PtrsXferServerCliThread: cant open reverse connection\n");
		assert(0);
	}
	assert(notif_sock->IsConnected());
}

PtrsXferServerCliThread::~PtrsXferServerCliThread()
{
	server->OnClientGone(this);
}

void *PtrsXferServerCliThread::Entry()
{
	printf("LOG:New xfer client thread started\n\n");
	while ( !TestDestroy() ) {
		if ( sock->WaitForLost(0, 0) ) {
			printf("PtrsXferServerCliThread: connection LOST\n");
			exit(0);
		}
		if ( sock->WaitForRead() ) {
			Notify_Event_Msg req;
			if ( sock->Read(&req, sizeof(Notify_Event_Msg)).Error() ) {
				printf("ERROR: while reading request %s\n", SockStrError(sock->LastError()));
				sock->Close();
				return 0;
			}
			printf("PtrsXferServerCliThread: cmd = %d\n", req.cmd);
			switch(req.cmd) {
			case PTR_XFER_PTR_REQ:
				Cmd_PtrReq(req);
				break;
			case PTR_XFER_STR_REQ:
				Cmd_StrReq(req);
				break;
			case PTR_XFER_INIT:
				Cmd_PtrInitXchange(req);
				break;
			case PTR_XFER_AUTH:
			case PTR_XFER_PTR_ACK:
			case PTR_XFER_STR_ACK:
				break;
			default:
				break;
			};
		} else {
			// WaitForRead timed out
		}
	}
	return 0;
}

void PtrsXferServerCliThread::Cmd_PtrInitXchange(Notify_Event_Msg &req)
{
}


void PtrsXferServerCliThread::Cmd_PtrReq(Notify_Event_Msg &req)
{
	req.print();
	/*
	if ( !req.PtrLooksGood() ) {
		return;
	}
	*/
	printf("PtrsXferServerCliThread: sending ack (Ptr = %p) with %d bytes data\n", req.ptr_value, req.data_len);
	req.cmd = PTR_XFER_PTR_ACK;
	if ( sock->Write(&req, sizeof(req)).Error() ) {
		printf("PtrsXferServerCliThread: error sending ack\n");
	}
	// here data following
	if ( req.data_len && sock->Write(req.ptr_value, req.data_len).Error() ) {
		printf("PtrsXferServerCliThread: error %s sending data\n", SockStrError(sock->LastError()));
	}
}

void PtrsXferServerCliThread::Cmd_StrReq(Notify_Event_Msg &req)
{
	wxString *s = (wxString *)req.ptr_value;
	req.cmd = PTR_XFER_STR_ACK;
	req.data_len = s->Length() + 1;
	printf("PtrsXferServerCliThread: sending ack (Str = %p) with %d bytes data\n", req.ptr_value, req.data_len);
	if ( sock->Write(&req, sizeof(req)).Error() ) {
		printf("PtrsXferServerCliThread: error sending ack\n");
	}
	if ( sock->Write(s->c_str(), req.data_len).Error() ) {
		printf("PtrsXferServerCliThread: error %s sending str data\n", SockStrError(sock->LastError()));
	}
}

int PtrsXferServerCliThread::SendNotify(GUIEvent &evt)
{
	Notify_Event_Msg msg(evt);
	if ( notif_sock->Write(&msg, sizeof(Notify_Event_Msg)).Error() ) {
		notif_sock->Close();
		sock->Close();
		return 0;
	}
	if ( notif_sock->LastCount() != sizeof(Notify_Event_Msg) ) {
		printf("PtrsXferServerCliThread: send %d of %d\n",notif_sock->LastCount(), sizeof(Notify_Event_Msg));
	}
	if ( (msg.data_len != 0) && notif_sock->Write(evt.string_value.c_str(), msg.data_len).Error()) {
		printf("PtrsXferServerCliThread: failed to send notify data\n");
		notif_sock->Close();
		sock->Close();
		return 0;
	}
	return notif_sock->LastCount() + sizeof(Notify_Event_Msg);
}

//
// Client side thread: send requests to server, wait for each one to be ack'ed
// When ack comes, update local data
PtrsXferClient::PtrsXferClient(wxEvtHandler *handler) :
	wxThread(wxTHREAD_DETACHED)
{
	data_buff_size = 256;
	data_buff = new char[data_buff_size];

	// bind address for reverse connection
	wxIPV4address laddr;
	laddr.AnyAddress();
	wxSocketServer rconn(laddr);
	rconn.SetFlags(sock_flags);
	rconn.Notify(FALSE);

	rconn.GetLocal(laddr);
	if ( !rconn.Ok() ) {
		printf("PtrsXferClient::Could not listen at the specified port !\n\n");
		return;
	} else {
		printf("PtrsXferClient:: listening on %d\n\n", laddr.Service());
	}
	
	sock = new wxSocketClient;
	sock->Notify(FALSE);
	sock->SetFlags(sock_flags);
	wxSocketClient *client = (wxSocketClient *)sock;
	wxIPV4address addr;
	addr.Hostname("localhost");
	addr.Service(3000);

	PtrsXferClient::handler = handler;

	client->Connect(addr, FALSE);

	bool waitmore = TRUE; 
	while ( !client->WaitOnConnect(3, 0) && waitmore ) {
		// possibly give some feedback to the user,
		// and update waitmore as needed.
		waitmore = FALSE;
	}
	bool success = client->IsConnected();
	if ( !success ) {
		printf("ERROR: sock failed to connect !\n");
	}
	printf("PtrsXferClient: FWD - connected! Sending login\n");
	Notify_Gui_Login ev;
	ev.port = laddr.Service();
	strcpy(ev.user, "lfroen");
	client->Write(&ev, sizeof(ev));

	// wait for reverse connection to come up
	if ( (notif_sock = rconn.Accept()) == 0 ) {
		printf("PtrsXferClient: accepting reverse connection failed!\n");		
	}
	printf("PtrsXferClient: RVS - connected!\n");

	RPtrDefaultBase::xfer = this;

	if ( Create() != wxTHREAD_NO_ERROR ) {
		sock->Close();
		delete sock;
		sock = 0;
	}

	if ( Run()  != wxTHREAD_NO_ERROR ) {
		sock->Close();
		delete sock;
		sock = 0;
	}
}

PtrsXferClient::~PtrsXferClient()
{
	delete [] (char *)data_buff;
        // release all mem allocated for local (client side) objects
	exit(0);
        for(std::map<ptr_type, ptr_type>::iterator it = ptr_hash.begin();
            it != ptr_hash.end(); it++) {
		int flags = ptr_hash_flags[it->first];
		if (flags == 0) {
			printf("PtrsXferClient::~PtrsXferClient delete %p\n", (unsigned char *)it->second);
			delete (unsigned char *)it->second;
		} else {
			if (flags & PTR_LOCAL) {
				continue;
			}
			if (flags & PTR_STRING) {
				wxString *s = (wxString *)it->second;
				delete s;
			}
		}
        }
}

void *PtrsXferClient::Entry()
{
	printf("PtrsXferClient: Xfer client started\n\n");
	while ( !TestDestroy() ) {
		printf("PtrsXferClient: waiting ...\n");
		if ( notif_sock->WaitForRead() ) {
			Notify_Event_Msg req;
			if ( notif_sock->Read(&req, sizeof(Notify_Event_Msg)).Error() ) {
				printf("PtrsXferClient: error reading msg %s\n", SockStrError(sock->LastError()));
				//continue;
				notif_sock->Close();
				return 0;
			}
			// sanity check:
			if ( req.data_len != 0 ) {
				printf("PtrsXferClient: data in notification\n");
				notif_sock->Close();
				return 0;
			}
			printf("PtrsXferClient: cmd = %d\n", req.cmd);
			switch(req.cmd) {
			case PTR_XFER_NOTIFY:
				// Notification from core: put event in fifo to process it
				// from GUI thread, where it belongs
				NotifyHandler(req, (wxChar *)data_buff);
				break;
			case PTR_XFER_STR_ACK:
			case PTR_XFER_PTR_ACK:
				printf("PtrsXferClient: cmd %d in wrong socket\n", req.cmd);
				break;
			case PTR_XFER_AUTH:
				break;
			default:
				break;
			};
		} else {
			// WaitForRead timed out - will not get here
		}
	}
	return 0;
}

int PtrsXferClient::ReadSrvAck()
{
	
	if ( sock->WaitForLost(0, 0) ) {
		printf("PtrsXferClient: connection LOST\n");
		exit(0);
	}
		
	Notify_Event_Msg req;
	//dump_mem_b(sock, sizeof(*sock));
	if ( sock->Read(&req, sizeof(Notify_Event_Msg)).Error() ) {
		printf("PtrsXferClient: error reading ACK %s\n", SockStrError(sock->LastError()));
		dump_mem_b(sock, sizeof(*sock));
		assert(0);
		sock->Close();
		return 0;
	}
	printf("PtrsXferClient: got ack, reading %d bytes data\n", req.data_len);
	// sanity check:
	if ( req.data_len > 1024 ) {
		printf("PtrsXferClient: data too big - %d bytes\n", req.data_len);
		sock->Close();
		return 0;
	}
	if ( data_buff_size < req.data_len ) {
		delete [] (char *)data_buff;
		data_buff_size = req.data_len;
		data_buff = new char[data_buff_size];
	}
	if ( req.data_len && sock->Read(data_buff, req.data_len).Error() ) {
		printf("PtrsXferClient: error reading %d bytes of data: %s\n",
		       req.data_len, SockStrError(sock->LastError()));
		sock->Close();
		return 0;
		}
	printf("PtrsXferClient: cmd = %d\n", req.cmd);
	switch(req.cmd) {
	case PTR_XFER_STR_ACK:
	case PTR_XFER_PTR_ACK:
		printf("PtrsXferClient: ack is here - signalling\n");
		return 1;
	case PTR_XFER_NOTIFY:
	case PTR_XFER_AUTH:
		printf("PtrsXferClient: cmd %d in wrong socket\n", req.cmd);
		break;
	default:
		break;
	};
	return 0;
}

void PtrsXferClient::NotifyHandler(Notify_Event_Msg &msg, wxChar *str)
{
	if ( handler ) {
		wxCoreNotifyEvent evt(msg, str);
		handler->AddPendingEvent(evt);
	}
}

void *PtrsXferClient::RemotePtrInsert(void *rem_ptr, int size)
{
        if ( LU(rem_ptr) ) {
                return 0;
        }
        unsigned char *loc_ptr = new (unsigned char)[size];
        ptr_hash[(ptr_type)rem_ptr] = (ptr_type)loc_ptr;
	ptr_hash_flags[(ptr_type)rem_ptr] = 0;

        printf("RemotePtrInsert (%p): added %p -> %p (%d bytes)\n", this, rem_ptr, loc_ptr, size);
        return loc_ptr;
}
bool PtrsXferClient::RemotePtrRemove(void *rem_ptr)
{
        unsigned char *loc_ptr = (unsigned char *)ptr_hash[(ptr_type)rem_ptr];
        if ( !loc_ptr ) {
                return false;
        }
        delete [] loc_ptr;
        ptr_hash.erase((ptr_type)rem_ptr);
        return true;
}

wxString *PtrsXferClient::StrPtrInsert(wxString *rem_ptr)
{
	if ( LU(rem_ptr) ) {
		return 0;
	}
	wxString *s = new wxString();
	ptr_hash[(ptr_type)rem_ptr] = (ptr_type)s;
	ptr_hash_flags[(ptr_type)rem_ptr] = PTR_STRING;

        printf("StrPtrInsert (%p): added %p -> %p (%d bytes)\n", this, rem_ptr, s, sizeof(wxString));
	return s;
}

bool PtrsXferClient::XferReady()
{
	return (sock && sock->IsConnected()) ? true : false;
}

//
// called in context of GUI thread
int PtrsXferClient::UpdateReq(void *rem_ptr, void *loc_ptr, int size)
{
        printf("UpdateReq (%p) %p -> %p %d ( 0x%x ) bytes\n", this,
	       rem_ptr, loc_ptr, size, size);

	Notify_Event_Msg req;
	req.ptr_value = rem_ptr;
	req.data_len = size;
	req.long_value = 0; // FIXME: class type ?

	// sending request
	req.cmd = PTR_XFER_PTR_REQ;
	sock->Write(&req, sizeof(req));

	// waiting for reply
	printf("UpdateReq waiting for reply\n");
	ReadSrvAck();
	// data has arrived, so buffer must be valid

 	// only aligned objects pls
 	assert((size % sizeof(int)) == 0);

 	unsigned long *t = (unsigned long *)loc_ptr;
 	unsigned long *s = (unsigned long *)data_buff;
 	for(int i = 0;i < size/4;i++) {
 		t[i] = s[i];
		
 	}

	//memcpy(loc_ptr, data_buff, size);
        return 0;
}

int PtrsXferClient::UpdateStrReq(wxString *rem_str, wxString *loc_str)
{
        printf("UpdateStrReq %p -> %p\n", rem_str, loc_str);

	Notify_Event_Msg req;
	req.ptr_value = rem_str;
	req.data_len = 0;
	req.long_value = 0; // FIXME: class type ?

	// sending request
	req.cmd = PTR_XFER_STR_REQ;
	sock->Write(&req, sizeof(req));

	// waiting for reply
	// data has arrived, so buffer must be valid
	ReadSrvAck();
        printf("UpdateStrReq data is here: '%s'\n", (char*)data_buff);
	(*loc_str) = (wxChar *)data_buff;
        return 0;
}

PtrsXferClient *RPtrDefaultBase::xfer = 0;

wxCoreNotifyEvent::wxCoreNotifyEvent(Notify_Event_Msg &msg, wxChar *str) : wxEvent(-1, wxEVT_CORE_NOTIFY),
	gui_evt((GUI_Event_ID)msg.event_id, msg.ptr_value, msg.ptr_aux_value,  msg.byte_value,
		msg.long_value, msg.longlong_value, str)
{
}

Notify_Event_Msg::Notify_Event_Msg(GUIEvent &event)
{
	cmd = PTR_XFER_NOTIFY;
	event_id = event.ID;
	byte_value = event.byte_value;
	long_value = event.long_value;
	longlong_value = event.longlong_value;
	ptr_value = event.ptr_value;
	ptr_aux_value = event.ptr_aux_value;
	data_len = event.string_value.Length();
}



DEFINE_EVENT_TYPE(wxEVT_CORE_NOTIFY)
DEFINE_EVENT_TYPE(wxEVT_CORE_NEW_CLIENT)

wxEvent *wxCoreNotifyEvent::Clone(void) const
{
	return new wxCoreNotifyEvent(*this);
}

wxCoreNewClent::wxCoreNewClent(PtrsXferServerCliThread *cli) : wxEvent(-1, wxEVT_CORE_NEW_CLIENT)
{
	client = cli;
}

wxEvent *wxCoreNewClent::Clone(void) const
{
	return new wxCoreNewClent(*this);
}



RPtrDefaultBase::RPtrDefaultBase(void *rem_ptr, int size)
{
        RPtrDefaultBase::size = size;
        RPtrDefaultBase::rem_ptr = rem_ptr;
        loc_ptr = xfer->LU(rem_ptr);
        printf("RPtrDefaultBase::ctor %p -> %p\n", rem_ptr, loc_ptr);
        assert(loc_ptr);

}

bool RPtrDefaultBase::RunUpdate()
{
        printf("RPtrDefaultBase::RunUpdate %p -> %p\n", rem_ptr, loc_ptr);
        xfer->UpdateReq(rem_ptr, loc_ptr, size);
        return true;
}

void *RPtrDefaultBase::RemotePtrInsert(void *ptr, int size)
{
        return xfer->RemotePtrInsert(ptr, size);
}

wxString *RPtrDefaultBase::StrPtrInsert(wxString *str)
{
	return xfer->StrPtrInsert(str);
}

void RPtrDefaultBase::RemotePtrRemove(void *ptr)
{
        xfer->RemotePtrRemove(ptr);
}


//
// Real data comes here: implement "updater" for each type i know
//

RPtrCPartFileBase::RPtrCPartFileBase(CPartFile *rem_ptr, int) : RPtrDefaultBase(rem_ptr, sizeof(CPartFile))
{
}


bool RPtrCPartFileBase::RunUpdate()
{
	RPtrDefaultBase::RunUpdate();

	CPartFile *typed_loc_ptr = (CPartFile *)loc_ptr;
	CPartFile *typed_rem_ptr = (CPartFile *)rem_ptr;

	XFER_LOCAL_OBJ_UPDATE(typed_loc_ptr, typed_rem_ptr, m_fullname);

	return true;
}

CPartFile *RPtrCPartFileBase::RemotePtrInsert(CPartFile *ptr, int)
{
	CPartFile *p = (CPartFile *) ptr;
	RPtrDefaultBase::RemotePtrInsert(ptr, sizeof(CPartFile));

	// register string field like this. Warning: calling network here
	XFER_MSG_STR_UPDATE(&ptr->m_fullname);
	return p;
}

