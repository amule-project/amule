//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga (kry@users.sourceforge.net)
// Copyright (c) 2004-2006 Froenchenko Leonid (lfroen@users.sourceforge.net)
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

#ifndef GUIEVENTS_H
#define GUIEVENTS_H

#include "Types.h"

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
	SHAREDFILES_SORT,
	
	// download control
	DOWNLOAD_CTRL_UPDATEITEM,
	DOWNLOAD_CTRL_ADD_FILE,
	DOWNLOAD_CTRL_ADD_SOURCE,
	DOWNLOAD_CTRL_RM_FILE,
	DOWNLOAD_CTRL_RM_SOURCE,
	DOWNLOAD_CTRL_HIDE_SOURCE,
	DOWNLOAD_CTRL_SORT,
	// upload control
	UPLOAD_CTRL_ADD_CLIENT,
	UPLOAD_CTRL_REFRESH_CLIENT,
	UPLOAD_CTRL_RM_CLIENT,
	// client list
	CLIENT_CTRL_ADD_CLIENT,
	CLIENT_CTRL_REFRESH_CLIENT,
	CLIENT_CTRL_RM_CLIENT,

	// server
	SERVER_ADD,
	SERVER_RM,
	SERVER_RM_DEAD,
	SERVER_RM_ALL,
	SERVER_HIGHLIGHT,
	SERVER_REFRESH,
	SERVER_FREEZE,
	SERVER_THAW,
	SERVER_UPDATEED2KINFO,
	SERVER_UPDATEKADINFO,
	// search window
	SEARCH_CANCEL,
	SEARCH_LOCAL_END,
	// chat window
	CHAT_REFRESH_FRIEND,
	CHAT_FIND_FRIEND,
	CHAT_CONN_RESULT,
	CHAT_PROCESS_MSG,
	// notification
	SHOW_CONN_STATE,
	SHOW_USER_COUNT,
	SHOW_QUEUE_COUNT,
	SHOW_UPDATE_CAT_TABS,
	SHOW_GUI,
	// catigories
	CATEGORY_ADD,
	CATEGORY_UPDATE,
	CATEGORY_DELETE,
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
	KNOWNFILE_SET_COMMENT,
	// search
	SEARCH_ADD_TO_DLOAD,
	SEARCH_ADD_RESULT,
	SEARCH_UPDATE_SOURCES,
	SEARCH_UPDATE_PROGRESS,
	// download queue
	DLOAD_SET_CAT_PRIO,
	DLOAD_SET_CAT_STATUS
};

// lfroen : custom events for core internal messages
// 'cause - there's no wxCommand etc in wxBase
enum Core_Event_ID {
	NOTIFY_EVENT = 1,
	HTTP_DOWNLOAD_FINISHED,
	
	SOURCE_DNS_DONE,
	UDP_DNS_DONE,
	SERVER_DNS_DONE
};

enum HTTP_Download_File {
	HTTP_IPFilter = 1,
	HTTP_ServerMet,
	// Auto-updating server.met has a different callback.
	HTTP_ServerMetAuto,
	HTTP_VersionCheck,
	HTTP_NodesDat
};

DECLARE_LOCAL_EVENT_TYPE(wxEVT_MULE_NOTIFY_EVENT, wxEVT_USER_FIRST+NOTIFY_EVENT)

class GUIEvent : public wxEvent {
	public:
	GUIEvent(GUI_Event_ID new_id, byte value8, wxString value_s, uint32 value_long = 0) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
		ID 		= new_id;
		byte_value 	= value8;
		short_value	= 0;
		long_value 	= value_long;
		longlong_value 	= 0;
		string_value 	= value_s;
		ptr_value	= NULL;
		ptr_aux_value	= NULL;
	}

	GUIEvent(GUI_Event_ID new_id, byte value8, uint64 value_longlong, wxString value_s) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
		ID 		= new_id;
		byte_value 	= value8;
		short_value	= 0;
		long_value 	= 0;
		longlong_value 	= value_longlong;
		string_value 	= value_s;
		ptr_value	= NULL;
		ptr_aux_value	= NULL;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr = NULL, void* new_aux_ptr = NULL, byte value8 = 0) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = value8;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
		ptr_value       = new_ptr;
		ptr_aux_value   = new_aux_ptr;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr,  wxString &str) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = 0;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
                string_value    = str;
		ptr_value       = new_ptr;
		ptr_aux_value   = NULL;
	}
	
	GUIEvent(GUI_Event_ID new_id, void *new_ptr,  byte value8) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
		ID              = new_id;
		byte_value      = value8;
		short_value	= 0;
		long_value      = 0;
		longlong_value  = 0;
		ptr_value       = new_ptr;
		ptr_aux_value   = NULL;
	}

        GUIEvent(GUI_Event_ID new_id, void *new_ptr,  uint32 value32, uint64 value64) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
                ID              = new_id;
			byte_value      = 0;
			short_value	= 0;
                long_value      = value32;
                longlong_value  = value64;
                ptr_value       = new_ptr;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, uint32 new_val)  : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
                ID              = new_id;
                byte_value      = 0;
		short_value	= 0;
                long_value      = new_val;
                longlong_value  = 0;
                ptr_value       = NULL;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, uint32 value32, uint16 value16, const wxString& string = wxEmptyString) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
                ID              = new_id;
                byte_value      = 0;
			 short_value	= value16;
                long_value      = value32;
                longlong_value  = 0;
		   	string_value = string;
                ptr_value       = NULL;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, void *new_ptr, void *new_aux_ptr,
		 byte value8, uint32 value32, uint64 value64, wxChar *str) : wxEvent(-1, wxEVT_MULE_NOTIFY_EVENT) {
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
		if ( wxThread::IsMain() ) { \
			theApp.NotifyEvent(e); \
		} else { \
			wxPostEvent(&theApp,e); \
		} \
	} while (0)
	
#define Notify_1_ValEvent(id, val) \
	do { \
		GUIEvent e(id, val);\
		if ( wxThread::IsMain() ) { \
			theApp.NotifyEvent(e); \
		} else { \
			wxPostEvent(&theApp,e); \
		} \
	} while (0)
	
#define Notify_2_ValEvent(id, val0, val1) \
	do { \
		GUIEvent e(id, val0, val1);\
		if ( wxThread::IsMain() ) { \
			theApp.NotifyEvent(e); \
		} else { \
			wxPostEvent(&theApp,e); \
		} \
	} while (0)
	
#define Notify_3_ValEvent(id, val0, val1, val2) \
	do { \
		GUIEvent e(id, val0, val1, val2);\
		if ( wxThread::IsMain() ) { \
			theApp.NotifyEvent(e); \
		} else { \
			wxPostEvent(&theApp,e); \
		} \
	} while (0)

#define Notify_SharedFilesShowFile(file)            Notify_1_ValEvent(SHAREDFILES_SHOW_ITEM, file)
#define Notify_SharedFilesRemoveFile(file)          Notify_1_ValEvent(SHAREDFILES_REMOVE_ITEM, file)
#define Notify_SharedFilesRemoveAllItems()          Notify_0_ValEvent(SHAREDFILES_REMOVE_ALL_ITEMS)
#define Notify_SharedFilesShowFileList(list)        Notify_1_ValEvent(SHAREDFILES_SHOW_ITEM_LIST, list)
#define Notify_SharedFilesSort()                    Notify_0_ValEvent(SHAREDFILES_SORT)
#define Notify_SharedFilesUpdateItem(ptr)           Notify_1_ValEvent(SHAREDFILES_UPDATE_ITEM, ptr)

// download ctrl
#define Notify_DownloadCtrlUpdateItem(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_UPDATEITEM, ptr)
#define Notify_DownloadCtrlAddFile(ptr)             Notify_1_ValEvent(DOWNLOAD_CTRL_ADD_FILE, ptr)
#define Notify_DownloadCtrlAddSource(p0, p1, val)   Notify_3_ValEvent(DOWNLOAD_CTRL_ADD_SOURCE, p0, p1, val)
#define Notify_DownloadCtrlRemoveFile(ptr0)         Notify_1_ValEvent(DOWNLOAD_CTRL_RM_FILE, ptr0)
#define Notify_DownloadCtrlRemoveSource(ptr0, ptr1) Notify_2_ValEvent(DOWNLOAD_CTRL_RM_SOURCE, (void *)ptr0, (void *)ptr1)
#define Notify_DownloadCtrlHideSource(ptr)          Notify_1_ValEvent(DOWNLOAD_CTRL_HIDE_SOURCE, ptr)
#define Notify_DownloadCtrlSort()                   Notify_0_ValEvent(DOWNLOAD_CTRL_SORT)

// upload ctrl
#define Notify_UploadCtrlAddClient(ptr)             Notify_1_ValEvent(UPLOAD_CTRL_ADD_CLIENT, ptr)
#define Notify_UploadCtrlRefreshClient(ptr)         Notify_1_ValEvent(UPLOAD_CTRL_REFRESH_CLIENT, ptr)
#define Notify_UploadCtrlRemoveClient(ptr)          Notify_1_ValEvent(UPLOAD_CTRL_RM_CLIENT, ptr)

// client ctrl
#define Notify_ClientCtrlAddClient(ptr)             Notify_1_ValEvent(CLIENT_CTRL_ADD_CLIENT, ptr)
#define Notify_ClientCtrlRefreshClient(ptr)         Notify_1_ValEvent(CLIENT_CTRL_REFRESH_CLIENT, ptr)
#define Notify_ClientCtrlRemoveClient(ptr)          Notify_1_ValEvent(CLIENT_CTRL_RM_CLIENT, ptr)

// server
#define Notify_ServerAdd(ptr)                       Notify_1_ValEvent(SERVER_ADD, ptr)
#define Notify_ServerRemove(ptr)                    Notify_1_ValEvent(SERVER_RM, ptr)
#define Notify_ServerRemoveDead()                   Notify_0_ValEvent(SERVER_RM_DEAD)
#define Notify_ServerRemoveAll()                    Notify_0_ValEvent(SERVER_RM_ALL)
#define Notify_ServerHighlight(ptr, val)            Notify_2_ValEvent(SERVER_HIGHLIGHT, (void *)ptr, (byte)val)
#define Notify_ServerRefresh(ptr)                   Notify_1_ValEvent(SERVER_REFRESH, ptr)
#define Notify_ServerFreeze()                       Notify_0_ValEvent(SERVER_FREEZE)
#define Notify_ServerThaw()                         Notify_0_ValEvent(SERVER_THAW)
#define Notify_ServerUpdateED2KInfo()		    Notify_0_ValEvent(SERVER_UPDATEED2KINFO)
#define Notify_ServerUpdateKadKInfo()		    Notify_0_ValEvent(SERVER_UPDATEKADINFO)

// queue list
#define Notify_QlistAddClient(ptr)                  Notify_1_ValEvent(QLIST_CTRL_ADD_CLIENT, ptr)
#define Notify_QlistRemoveClient(ptr)               Notify_1_ValEvent(QLIST_CTRL_RM_CLIENT, ptr)
#define Notify_QlistRefreshClient(ptr)              Notify_1_ValEvent(QLIST_CTRL_REFRESH_CLIENT, ptr)
#define Notify_QlistThaw()                          Notify_0_ValEvent()

// search
#define Notify_SearchCancel()                       Notify_0_ValEvent(SEARCH_CANCEL)
#define Notify_SearchLocalEnd()                     Notify_0_ValEvent(SEARCH_LOCAL_END)
#define Notify_Search_Update_Sources(ptr)          Notify_1_ValEvent(SEARCH_UPDATE_SOURCES,(CSearchFile *)ptr);
#define Notify_Search_Add_Result(s)                 Notify_1_ValEvent(SEARCH_ADD_RESULT,(CSearchFile *)s);

// chat
#define Notify_ChatRefreshFriend(val0, val1, s)     Notify_3_ValEvent(CHAT_REFRESH_FRIEND, val0, val1, s)
#define Notify_ChatConnResult(val0, val1, s)             Notify_3_ValEvent(CHAT_CONN_RESULT, (byte)val0, (uint64)val1, s)
#define Notify_ChatProcessMsg(val0, s)             Notify_3_ValEvent(CHAT_PROCESS_MSG, (byte)0, (uint64)val0, s)

// misc
#define Notify_ShowConnState(val)             		Notify_1_ValEvent(SHOW_CONN_STATE, (uint32)val)
#define Notify_ShowUserCount(str)                   Notify_2_ValEvent(SHOW_USER_COUNT, (byte)0, str)
#define Notify_ShowQueueCount(val)                  Notify_1_ValEvent(SHOW_QUEUE_COUNT, (uint32)val)
#define Notify_ShowUpdateCatTabTitles()             Notify_0_ValEvent(SHOW_UPDATE_CAT_TABS)
#define Notify_ShowGUI()             					Notify_0_ValEvent(SHOW_GUI)

// categories
#define Notify_CategoryAdded(cat)						Notify_0_ValEvent(CATEGORY_ADD)
#define Notify_CategoryUpdate(cat)						Notify_1_ValEvent(CATEGORY_UPDATE, cat)
#define Notify_CategoryDelete(cat)						Notify_1_ValEvent(CATEGORY_DELETE, cat)

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
#define CoreNotify_KnownFile_Comment_Set(ptr, val)  Notify_2_ValEvent(KNOWNFILE_SET_COMMENT,(CKnownFile *)ptr, val);

// Search

#define CoreNotify_Search_Add_Download(ptr, val)    Notify_2_ValEvent(SEARCH_ADD_TO_DLOAD,(CSearchFile *)ptr, (uint8)val);

#define CoreNotify_Search_Update_Progress(val)      Notify_1_ValEvent(SEARCH_UPDATE_PROGRESS, (uint32)val);
// download queue
#define CoreNotify_Download_Set_Cat_Prio(cat, pri)  Notify_2_ValEvent(DLOAD_SET_CAT_PRIO, cat, pri);
#define CoreNotify_Download_Set_Cat_Status(cat, st) Notify_2_ValEvent(DLOAD_SET_CAT_STATUS, cat, st);



#endif // __GUIEVENTS_H__
