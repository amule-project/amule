// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Angel Vidal Veiga (kry@users.sourceforge.net)
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

#ifndef GUIEVENTS_H
#define GUIEVENTS_H

#include "types.h"
#include  <map>
#include  <list>


class wxSocketBase;
class wxSocketServer;
class wxSocketClient;

enum GUI_Event_ID {
	INVALID_EVENT = 0,
	// queue list
	QLIST_CTRL_ADD_CLIENT,
	QLIST_CTRL_RM_CLIENT,
	QLIST_CTRL_REFRESH_CLIENT,
	// shared files
	SHAREDFILES_UPDATE_ITEM,
	SHAREDFILES_REMOVE_ITEM,
	SHAREDFILES_REMOVE_ALL_ITEMS,
	SHAREDFILES_SHOW_ITEM,
	SHAREDFILES_SHOW_ITEM_LIST,
	SHAREDFILES_INIT_SORT,
	
	// download control
	DOWNLOAD_CTRL_UPDATEITEM,
	DOWNLOAD_CTRL_ADD_FILE,
	DOWNLOAD_CTRL_ADD_SOURCE,
	DOWNLOAD_CTRL_RM_FILE,
	DOWNLOAD_CTRL_RM_SOURCE,
	DOWNLOAD_CTRL_SHOW_HIDE_FILE,
	DOWNLOAD_CTRL_HIDE_SOURCE,
	DOWNLOAD_CTRL_INIT_SORT,
	DOWNLOAD_CTRL_SHOW_FILES_COUNT,
	// upload control
	UPLOAD_CTRL_ADD_CLIENT,
	UPLOAD_CTRL_REFRESH_CLIENT,
	UPLOAD_CTRL_RM_CLIENT,
	// server
	SERVER_ADD,
	SERVER_RM,
	SERVER_RM_DEAD,
	SERVER_RM_ALL,
	SERVER_HIGHLIGHT,
	SERVER_REFRESH,
	SERVER_FREEZE,
	SERVER_THAW,
	// search window
	SEARCH_CANCEL,
	SEARCH_LOCAL_END,
	// chat window
	CHAT_REFRESH_FRIEND,
	CHAT_FIND_FRIEND,
	CHAT_CONN_RESULT,
	CHAT_PROCESS_MSG,
	// notification
	SHOW_NOTIFIER,
	SHOW_CONN_STATE,
	SHOW_USER_COUNT,
	SHOW_QUEUE_COUNT,
	SHOW_UPDATE_CAT_TABS,
	SHOW_GUI,
	// logging
	ADDLOGLINE,
	ADDDEBUGLOGLINE,
	//
	// Those events are going to reverse direction: from gui->core
	//
	
	// PartFile
	PARTFILE_REMOVE_NO_NEEDED,
	PARTFILE_REMOVE_FULL_QUEUE,
	PARTFILE_REMOVE_HIGH_QUEUE,
	PARTFILE_CLEANUP_SOURCES,
	PARTFILE_SWAP_A4AF_THIS,
	PARTFILE_SWAP_A4AF_THIS_AUTO,
	PARTFILE_SWAP_A4AF_OTHERS,
	PARTFILE_PAUSE,
	PARTFILE_RESUME,
	PARTFILE_STOP,
	PARTFILE_PRIO_AUTO,
	PARTFILE_PRIO_SET,
	PARTFILE_DELETE,
	PARTFILE_SET_CAT,
	KNOWNFILE_SET_UP_PRIO,
	KNOWNFILE_SET_UP_PRIO_AUTO,
	KNOWNFILE_SET_PERM,
	KNOWNFILE_SET_COMMENT,
	// search
	SEARCH_REQ,
	SEARCH_ADD_TO_DLOAD,
	SEARCH_ADD_RESULT,
	SEARCH_UPDATE_SOURCES,
	SEARCH_UPDATE_PROGRESS,
	// download queue
	DLOAD_SET_CAT_PRIO,
	DLOAD_SET_CAT_STATUS,
};

// lfroen : custom events for core internal messages
// 'cause - there's no wxCommand etc in wxBase
enum Core_Event_ID {
	NOTIFY_EVENT = 1,
	FILE_HASHING_FINISHED,
	FILE_HASHING_SHUTDOWN,
	FILE_COMPLETION_FINISHED,
	
	SOURCE_DNS_DONE,
	DNS_DONE,
			
	EVENT_TIMER,
};

DECLARE_EVENT_TYPE(wxEVT_NOTIFY_EVENT, wxEVT_USER_FIRST+NOTIFY_EVENT)

class GUIEvent : public wxEvent {
	public:
	GUIEvent(GUI_Event_ID new_id, byte value8, wxString value_s, uint32 value_long = 0) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
		ID 		= new_id;
		byte_value 	= value8;
		short_value	= 0;
		long_value 	= value_long;
		longlong_value 	= 0;
		string_value 	= value_s;
		ptr_value	= NULL;
                ptr_aux_value	= NULL;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr = NULL, void* new_aux_ptr = NULL, byte value8 = 0) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = value8;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
		ptr_value       = new_ptr;
		ptr_aux_value   = new_aux_ptr;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr,  wxString &str) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = 0;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
                string_value    = str;
		ptr_value       = new_ptr;
		ptr_aux_value   = NULL;
	}
	
	GUIEvent(GUI_Event_ID new_id, void *new_ptr,  byte value8) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = value8;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
		ptr_value       = new_ptr;
		ptr_aux_value   = NULL;
	}

        GUIEvent(GUI_Event_ID new_id, void *new_ptr,  uint32 value32, uint64 value64) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
                ID              = new_id;
                byte_value      = 0;
		short_value	= 0;
                long_value      = value32;
                longlong_value  = value64;
                ptr_value       = new_ptr;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, uint32 new_val)  : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
                ID              = new_id;
                byte_value      = 0;
		short_value	= 0;
                long_value      = new_val;
                longlong_value  = 0;
                ptr_value       = NULL;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, uint32 value32, uint16 value16) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
                ID              = new_id;
                byte_value      = 0;
		short_value	= value16;
                long_value      = value32;
                longlong_value  = 0;
                ptr_value       = NULL;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, void *new_ptr, void *new_aux_ptr,
		 byte value8, uint32 value32, uint64 value64, wxChar *str) : wxEvent(-1, wxEVT_NOTIFY_EVENT) {
                ID              = new_id;       
                byte_value      = value8;
                long_value      = value32;
                longlong_value  = value64;
                string_value    = str;
                ptr_value       = new_ptr;
                ptr_aux_value   = new_aux_ptr;
        }
	wxEvent *Clone(void) const
	{
		return new GUIEvent(*this);
	}

	GUI_Event_ID ID;
	byte			byte_value;
	uint16			short_value;
	uint32			long_value;
	uint64			longlong_value;
	wxString		string_value;

	// this should NEVER be needed
	void*			ptr_value; 
	void*			ptr_aux_value; 
};


//
// macros for creation of notification events
#define Notify_0_ValEvent(id) \
	do { \
	GUIEvent e(id);\
	wxPostEvent(&theApp,e);\
	} while (0);
	
#define Notify_1_ValEvent(id, val) \
	do { \
	GUIEvent e(id, val);\
	wxPostEvent(&theApp,e);\
	} while (0);
	
#define Notify_2_ValEvent(id, val0, val1) \
	do { \
	GUIEvent e(id, val0, val1);\
	wxPostEvent(&theApp,e);\
	} while (0);
	
#define Notify_3_ValEvent(id, val0, val1, val2) \
	do { \
	GUIEvent e(id, val0, val1, val2);\
	wxPostEvent(&theApp,e);\
	} while (0);

#define Notify_SharedFilesShowFile(file)            Notify_1_ValEvent(SHAREDFILES_SHOW_ITEM, file)
#define Notify_SharedFilesRemoveFile(file)          Notify_1_ValEvent(SHAREDFILES_REMOVE_ITEM, file)
#define Notify_SharedFilesRemoveAllItems()          Notify_0_ValEvent(SHAREDFILES_REMOVE_ALL_ITEMS)
#define Notify_SharedFilesShowFileList(list)        Notify_1_ValEvent(SHAREDFILES_SHOW_ITEM_LIST, list)
#define Notify_SharedFilesInitSort()                Notify_0_ValEvent(SHAREDFILES_INIT_SORT)
#define Notify_SharedFilesUpdateItem(ptr)           Notify_1_ValEvent(SHAREDFILES_UPDATE_ITEM, ptr)

// download ctrl
#define Notify_DownloadCtrlUpdateItem(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_UPDATEITEM, ptr)
#define Notify_DownloadCtrlAddFile(ptr)             Notify_1_ValEvent(DOWNLOAD_CTRL_ADD_FILE, ptr)
#define Notify_DownloadCtrlAddSource(p0, p1, val)   Notify_3_ValEvent(DOWNLOAD_CTRL_ADD_SOURCE, p0, p1, val)
#define Notify_DownloadCtrlRemoveFile(ptr0)         Notify_1_ValEvent(DOWNLOAD_CTRL_RM_FILE, ptr0)
#define Notify_DownloadCtrlRemoveSource(ptr0, ptr1) Notify_2_ValEvent(DOWNLOAD_CTRL_RM_SOURCE, (void *)ptr0, (void *)ptr1)
#define Notify_DownloadCtrlShowHideFileStatus(ptr)  Notify_1_ValEvent(DOWNLOAD_CTRL_SHOW_HIDE_FILE, ptr)
#define Notify_DownloadCtrlHideSource(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_HIDE_SOURCE, ptr)
#define Notify_DownloadCtrlInitSort()               Notify_0_ValEvent(DOWNLOAD_CTRL_INIT_SORT)
#define Notify_DownloadCtrlShowFilesCount()         Notify_0_ValEvent(DOWNLOAD_CTRL_SHOW_FILES_COUNT)

// upload ctrl
#define Notify_UploadCtrlAddClient(ptr)             Notify_1_ValEvent(UPLOAD_CTRL_ADD_CLIENT, ptr)
#define Notify_UploadCtrlRefreshClient(ptr)         Notify_1_ValEvent(UPLOAD_CTRL_REFRESH_CLIENT, ptr)
#define Notify_UploadCtrlRemoveClient(ptr)          Notify_1_ValEvent(UPLOAD_CTRL_RM_CLIENT, ptr)

// server
#define Notify_ServerAdd(ptr)                       Notify_1_ValEvent(SERVER_ADD, ptr)
#define Notify_ServerRemove(ptr)                    Notify_1_ValEvent(SERVER_RM, ptr)
#define Notify_ServerRemoveDead()                   Notify_0_ValEvent(SERVER_RM_DEAD)
#define Notify_ServerRemoveAll()                    Notify_0_ValEvent(SERVER_RM_ALL)
#define Notify_ServerHighlight(ptr, val)            Notify_2_ValEvent(SERVER_HIGHLIGHT, (void *)ptr, (byte)val)
#define Notify_ServerRefresh(ptr)                   Notify_1_ValEvent(SERVER_REFRESH, ptr)
#define Notify_ServerFreeze()                       Notify_0_ValEvent(SERVER_FREEZE)
#define Notify_ServerThaw()                         Notify_0_ValEvent(SERVER_THAW)

// queue list
#define Notify_QlistAddClient(ptr)                  Notify_1_ValEvent(QLIST_CTRL_ADD_CLIENT, ptr)
#define Notify_QlistRemoveClient(ptr)               Notify_1_ValEvent(QLIST_CTRL_RM_CLIENT, ptr)
#define Notify_QlistRefreshClient(ptr)              Notify_1_ValEvent(QLIST_CTRL_REFRESH_CLIENT, ptr)
#define Notify_QlistThaw()                          Notify_0_ValEvent()

// search
#define Notify_SearchCancel()                       Notify_0_ValEvent(SEARCH_CANCEL)
#define Notify_SearchLocalEnd()                     Notify_0_ValEvent(SEARCH_LOCAL_END)
#define Notify_Search_Update_Sources(s, f)          Notify_2_ValEvent(SEARCH_UPDATE_SOURCES,(CSearchFile *)s, (CSearchFile *)f);
#define Notify_Search_Add_Result(s)                 Notify_1_ValEvent(SEARCH_ADD_RESULT,(CSearchFile *)s);

// chat
#define Notify_ChatRefreshFriend(ptr)               Notify_1_ValEvent(CHAT_REFRESH_FRIEND, ptr)
#define Notify_ChatConnResult(ptr, val)             Notify_2_ValEvent(CHAT_CONN_RESULT, (void *)ptr, (byte)val)
#define Notify_ChatProcessMsg(ptr, val)             Notify_2_ValEvent(CHAT_PROCESS_MSG, (CUpDownClient *)ptr, val)

// misc
#define Notify_ShowNotifier(str, val0, val1)        Notify_3_ValEvent(SHOW_NOTIFIER, val0, str, val1)
#define Notify_ShowConnState(val0, str)             Notify_2_ValEvent(SHOW_CONN_STATE, val0, str)
#define Notify_ShowUserCount(ptr)                   Notify_1_ValEvent(SHOW_USER_COUNT, ptr)
#define Notify_ShowQueueCount(val)                  Notify_1_ValEvent(SHOW_QUEUE_COUNT, (uint32)val)
#define Notify_ShowUpdateCatTabTitles()             Notify_0_ValEvent(SHOW_UPDATE_CAT_TABS)
#define Notify_ShowGUI()             					Notify_0_ValEvent(SHOW_GUI)

#define AddLogLineM(x,y); 			theApp.NotifyEvent(GUIEvent(ADDLOGLINE,(byte)x,y));
#define AddDebugLogLineM(x,y); 	theApp.NotifyEvent(GUIEvent(ADDDEBUGLOGLINE,(byte)x,y));

//
// GUI -> core notification
//

// PartFile
#define CoreNotify_PartFile_RemoveNoNeeded(ptr)     Notify_1_ValEvent(PARTFILE_REMOVE_NO_NEEDED,(CPartFile *)ptr);
#define CoreNotify_PartFile_RemoveFullQueue(ptr)    Notify_1_ValEvent(PARTFILE_REMOVE_FULL_QUEUE,(CPartFile *)ptr);
#define CoreNotify_PartFile_RemoveHighQueue(ptr)    Notify_1_ValEvent(PARTFILE_REMOVE_HIGH_QUEUE,(CPartFile *)ptr);
#define CoreNotify_PartFile_SourceCleanup(ptr)      Notify_1_ValEvent(PARTFILE_CLEANUP_SOURCES,(CPartFile *)ptr);
#define CoreNotify_PartFile_Swap_A4AF(ptr)          Notify_1_ValEvent(PARTFILE_SWAP_A4AF_THIS,(CPartFile *)ptr);
#define CoreNotify_PartFile_Swap_A4AF_Auto(ptr)     Notify_1_ValEvent(PARTFILE_SWAP_A4AF_THIS_AUTO,(CPartFile *)ptr);
#define CoreNotify_PartFile_Swap_A4AF_Others(ptr)   Notify_1_ValEvent(PARTFILE_SWAP_A4AF_OTHERS,(CPartFile *)ptr);
#define CoreNotify_PartFile_Pause(ptr)              Notify_1_ValEvent(PARTFILE_PAUSE,(CPartFile *)ptr);
#define CoreNotify_PartFile_Resume(ptr)             Notify_1_ValEvent(PARTFILE_RESUME,(CPartFile *)ptr);
#define CoreNotify_PartFile_Stop(ptr)               Notify_1_ValEvent(PARTFILE_STOP,(CPartFile *)ptr);
#define CoreNotify_PartFile_PrioAuto(ptr, val)      Notify_3_ValEvent(PARTFILE_PRIO_AUTO,(CPartFile *)ptr, (uint32)val, (uint64)0);
#define CoreNotify_PartFile_PrioSet(p, v0, v1)      Notify_3_ValEvent(PARTFILE_PRIO_SET,(CPartFile *)p, (uint32)v0, (uint64)v1);
#define CoreNotify_PartFile_Delete(ptr)             Notify_1_ValEvent(PARTFILE_DELETE,(CPartFile *)ptr);
#define CoreNotify_PartFile_SetCat(ptr, val)        Notify_2_ValEvent(PARTFILE_SET_CAT,(CPartFile *)ptr, val);
// KnownFile
#define CoreNotify_KnownFile_Up_Prio_Set(ptr, val)  Notify_2_ValEvent(KNOWNFILE_SET_UP_PRIO,(CKnownFile *)ptr, (uint8)val);
#define CoreNotify_KnownFile_Up_Prio_Auto(ptr)      Notify_1_ValEvent(KNOWNFILE_SET_UP_PRIO_AUTO,(CKnownFile *)ptr);
#define CoreNotify_KnownFile_Perm_Set(ptr, val)     Notify_2_ValEvent(KNOWNFILE_SET_PERM,(CKnownFile *)ptr, (uint8)val);
#define CoreNotify_KnownFile_Comment_Set(ptr, val)  Notify_2_ValEvent(KNOWNFILE_SET_PERM,(CKnownFile *)ptr, val);

// Search
#define CoreNotify_Search_Req(ptr, global)          Notify_2_ValEvent(SEARCH_REQ,(Packet *)ptr, global);

#define CoreNotify_Search_Add_Download(ptr, val)    Notify_2_ValEvent(SEARCH_ADD_TO_DLOAD,(CSearchFile *)ptr, (uint8)val);

#define CoreNotify_Search_Update_Progress(val)      Notify_1_ValEvent(SEARCH_UPDATE_PROGRESS, (uint32)val);
// download queue
#define CoreNotify_Download_Set_Cat_Prio(cat, pri)  Notify_2_ValEvent(DLOAD_SET_CAT_PRIO, cat, pri);
#define CoreNotify_Download_Set_Cat_Status(cat, st) Notify_2_ValEvent(DLOAD_SET_CAT_STATUS, cat, st);
//
// "Late bound pointers" lib
//
// Known issues: fix endiness, wxString xfer, compilation options
//


enum PtrXferCmd {
	PTR_XFER_AUTH = 0, // auth gui against core
	// ptr data xfer
	PTR_XFER_PTR_REQ,  // request ptr data
	PTR_XFER_STR_REQ,  // request string data
	PTR_XFER_PTR_ACK,  // ptr data
	PTR_XFER_STR_ACK,  // string data
	// notification
	PTR_XFER_NOTIFY,    // Core2Gui_Event_Msg
	// initial exchage
	PTR_XFER_INIT,
};


// this is packet from Notify_* macros as it travels thru ipc (network)
// assuming sizeof(void*) is equal on both client and server
class Notify_Event_Msg {
 public:
	byte cmd;  // PTR_XFER_*
	uint32 event_id;
	uint32 byte_value;
	uint32 long_value;
	uint64 longlong_value;
	void*  ptr_value; 
	void*  ptr_aux_value; 
	uint32 data_len;
	// string data in size string_len coming here in packet

	bool PtrLooksGood()
	{
		return ((data_len < 1024) && ( ((uint32)ptr_value > 0x04000000) ) );
	}
	Notify_Event_Msg(GUIEvent &event);
	Notify_Event_Msg() {}
	void print() { printf("Notify_Event_Msg: %p %d bytes type %d\n", ptr_value, data_len, long_value); }
};

//DECLARE_EVENT_TYPE(wxEVT_CORE_NOTIFY, wxEVT_USER_FIRST+1000)

class wxCoreNotifyEvent : public wxEvent {
 public:
	wxCoreNotifyEvent(Notify_Event_Msg &msg, wxChar *str);
	wxEvent *Clone(void) const;

	GUIEvent gui_evt;
};

//
// "login" gui to core
//
class Notify_Gui_Login {
 public:
	byte cmd;       // PTR_XFER_AUTH
	uint16 port;    // on this port client listening
	char user[256]; // fixed size, no unicode
	Notify_Gui_Login() { cmd = PTR_XFER_AUTH; }
};


//
// Connection to already running remote side.
class PtrsXferClient : public wxThread {
	// one for data, one for core notification
	wxSocketBase *sock, *notif_sock;

	uint32 data_buff_size;
	void *data_buff;

	typedef unsigned int ptr_type;
	// assuming that sizeof(ptr) == sizeof(int)
	std::map<ptr_type, ptr_type> ptr_hash;

	enum {
		PTR_LOCAL  = 1,
		PTR_STRING = 2,
	};
	std::map<ptr_type, int> ptr_hash_flags;

	wxEvtHandler *handler;

	virtual void *Entry();

 public:
	PtrsXferClient(wxEvtHandler *handler);
	~PtrsXferClient();

	void *LU(const void *ptr)
		{
			return (void *)ptr_hash[(ptr_type)ptr];
		}

	void NotifyHandler(Notify_Event_Msg &msg, wxChar *str);

	void *RemotePtrInsert(void *rem_ptr, int size);
	bool RemotePtrRemove(void *rem_ptr);

	wxString *StrPtrInsert(wxString *rem_ptr);
	void LocalPtrInsert(void *rem_ptr, void *loc_ptr);

	bool XferReady();

	// update local data associated with this ptr
	int UpdateReq(void *rem_ptr, void *loc_ptr, int size);
	int UpdateStrReq(wxString *rem_str, wxString *loc_str);

	int ReadSrvAck();
};

class PtrsXferServerCliThread;

class PtrsXferServer : public wxThread {
	wxSocketBase *sock;
	wxEvtHandler *handler;

	virtual void *Entry();

	// 1 core, many clients
	std::list<PtrsXferServerCliThread *> clients;
 public:
	PtrsXferServer(wxEvtHandler *handler, int port);
	~PtrsXferServer();

	bool InitOk() { return sock != 0; }

	void SendNotify(GUIEvent &evt);
	int HaveClients() { return clients.size(); }

	void OnClientGone(PtrsXferServerCliThread *client);
};

//
// no events without gui - thread per client is a must
//
class PtrsXferServerCliThread : public wxThread {
	wxSocketBase *sock;
	wxSocketClient *notif_sock;
	PtrsXferServer *server;

	virtual void *Entry();

	// dispatch functions
	void Cmd_PtrInitXchange(Notify_Event_Msg &);

	void Cmd_PtrReq(Notify_Event_Msg &);
	void Cmd_StrReq(Notify_Event_Msg &);
 public:
	PtrsXferServerCliThread(wxSocketBase *sock, PtrsXferServer *server);
	~PtrsXferServerCliThread();

	int SendNotify(GUIEvent &evt);
};

//DECLARE_EVENT_TYPE(wxEVT_CORE_NEW_CLIENT, wxEVT_USER_FIRST+2)

class wxCoreNewClent : public wxEvent {
 public:
	wxCoreNewClent(PtrsXferServerCliThread *client);
	wxEvent *Clone(void) const;

	PtrsXferServerCliThread *client;
};


//
// Representation of pointer on remote side. Object values on local side
// updated in ctor. T is type of pointer, U type of class that perform
// actual update.
//
// need template for proper casting
//
template <class T, class U>
class RPtr : public U {
	public:
	RPtr(T* rem_ptr) : U(rem_ptr, sizeof(T))
		{
			this->RunUpdate();
		}
	~RPtr()
		{
		}
	T *operator->()
		{
			return (T *)this->loc_ptr;
		}
	T *PtrCast(void *ptr)
		{
			return (T *)ptr;
		}
};

// Default "class U" for RPtr: simple class - all data is scalar values
// update ptr just by copying remote data here
class RPtrDefaultBase {
	protected:
	void *rem_ptr, *loc_ptr;
	int size;

	public:
        RPtrDefaultBase(void *rem_ptr, int size);

	// transfer data from server and update client side ptrs
	bool RunUpdate();

	static void *RemotePtrInsert(void *ptr, int size);
	static wxString *StrPtrInsert(wxString *str);
	static void RemotePtrRemove(void *ptr);

	// it's quite unreasonable to have more than one
	static PtrsXferClient *xfer;
};


#if !defined(offset_of)
#define offset_of(type,member) ((int)(&((type *)0)->member))
#endif


// "remote object" is pointer to object type C have field of type (T *) which have
//  corresponding entry in hash table. Object pointed by hash is updated
// it's updated same way as regular pointer

// update object fields that valid only on local side
inline void my_memcpy(void *dst, const void *src, int count)
{
	printf("xfercopy: %p -> %p %d bytes\n", src, dst, count);
	memcpy(dst, src, count);
}

//
// "local object" is something created locally: object type C have field of type T,
//  this object is not updated, and hash table holding pointer to valid copy of T
//  which is memcpy'ed after all data of C has been memcpy'ed
#define XFER_LOCAL_OBJ_UPDATE(loc_ptr, rem_ptr, field) my_memcpy(&loc_ptr->field, \
							      xfer->LU(&rem_ptr->field), \
							      sizeof((rem_ptr)->field) )

// wxString requires special attention (as any other lib class). Good thing is that
// it need to be update only once, when object pointer is first inserted
#define XFER_MSG_STR_UPDATE(field_ptr) xfer->UpdateStrReq(field_ptr, xfer->StrPtrInsert(field_ptr))

// avoid calling xfer->RemotePtr* directly: need to do it thru correct type
#define XFER_MSG_PTR_INSERT(ptr, type) type##RPtr::RemotePtrInsert((type *)ptr)
#define XFER_MSG_PTR_REMOVE(ptr, type) type##RPtr::RemotePtrRemove(ptr)

// FIXME:
// "smart" pointer behavior must be different depending on target:
//  1. On statically build client (core & gui together) it works like ordinary ptr
//  2. On remote gui client it must call to late binding
// Choosing behavior thru "define" requires to compile all sources twice: once per target
// Instead, behavior can be choosen by linking with different file.
//
// define it or not depending on target type
//#define REMOTE_GUI_BUILD

#ifdef REMOTE_GUI_BUILD
#define DEFINE_XFER_PTR(type, var) type##RPtr var
#else
#define DEFINE_XFER_PTR(type, var) type *var
#endif

// "RPtr" for real objects
class CPartFile;

class RPtrCPartFileBase : public RPtrDefaultBase {
	public:
        RPtrCPartFileBase(CPartFile *rem_ptr, int);
	bool RunUpdate();
	static CPartFile *RemotePtrInsert(CPartFile *ptr, int unused = 0);
	static void RemotePtrRemove(CPartFile *ptr);

};
typedef RPtr<CPartFile, RPtrCPartFileBase> CPartFileRPtr;

#endif // __GUIEVENTS_H__
