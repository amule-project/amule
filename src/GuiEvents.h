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

enum GUI_Event_ID {
	INVALID_EVENT = 0,
	// queue list
	QLIST_CTRL_ADD_CLIENT,
	QLIST_CTRL_RM_CLIENT,
	QLIST_CTRL_REFRESH_CLIENT,
	// shared files
	SHAREDFILES_UPDATEITEM,
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
	// chat window
	CHAT_REFRESH_FRIEND,
	CHAT_FIND_FRIEND,
	CHAT_CONN_RESULT,
	// notification
	SHOW_NOTIFIER,
	SHOW_CONN_STATE,
	SHOW_QUEUE_COUNT,
	SHOW_UPDATE_CAT_TABS,
	// logging
	ADDLOGLINE,
	ADDDEBUGLOGLINE
};

class GUIEvent {
	public:
	GUIEvent(GUI_Event_ID new_id, byte value8, wxString value_s, uint32 value_long = 0) {
		ID 					= new_id;	
		byte_value 		= value8;
		long_value 	= value_long;
		longlong_value 	= 0;
		string_value 	= value_s;
		ptr_value			= NULL;
                ptr_aux_value                   = NULL;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr = NULL, void* new_aux_ptr = NULL, byte value8 = 0) {
		ID              = new_id;       
		byte_value      = value8;
		long_value      = 0;
		longlong_value  = 0;
		string_value    = wxEmptyString;
		ptr_value       = new_ptr;
		ptr_aux_value   = new_aux_ptr;
	}

	GUIEvent(GUI_Event_ID new_id, void *new_ptr,  byte value8) {
		ID              = new_id;       
		byte_value      = value8;
		long_value      = 0;
		longlong_value  = 0;
		string_value    = wxEmptyString;
		ptr_value       = new_ptr;
		ptr_aux_value   = NULL;
	}

        GUIEvent(GUI_Event_ID new_id, void *new_ptr,  uint32 value32, uint64 value64) {
                ID              = new_id;       
                byte_value      = 0;
                long_value      = value32;
                longlong_value  = value64;
                string_value    = wxEmptyString;
                ptr_value       = new_ptr;
                ptr_aux_value   = NULL;
        }

        GUIEvent(GUI_Event_ID new_id, uint32 new_val) {
                ID              = new_id;       
                byte_value      = 0;
                long_value      = new_val;
                longlong_value  = 0;
                string_value    = wxEmptyString;
                ptr_value       = NULL;
                ptr_aux_value   = NULL;
        }

	GUI_Event_ID ID;
	byte			byte_value;
	uint32		long_value;
	uint64		longlong_value;
	wxString		string_value;

	// this should NEVER be needed
	void*			ptr_value; 
	void*			ptr_aux_value; 
};

//
// macros for creation of notification events
#define Notify_0_ValEvent(id) theApp.NotifyEvent(GUIEvent(id))
#define Notify_1_ValEvent(id, val) theApp.NotifyEvent(GUIEvent(id, val))
#define Notify_2_ValEvent(id, val0, val1) theApp.NotifyEvent(GUIEvent(id, val0, val1))
#define Notify_3_ValEvent(id, val0, val1, val2) theApp.NotifyEvent(GUIEvent(id, val0, val1, val2))


#define Notify_SharedFilesUpdateItem(ptr)           Notify_1_ValEvent(SHAREDFILES_UPDATEITEM, ptr)

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
#define Notify_SearchCancel(ptr)                    Notify_1_ValEvent(SEARCH_CANCEL, ptr)

// chat
#define Notify_ChatConnResult(ptr, val)             Notify_2_ValEvent(CHAT_CONN_RESULT, (void *)ptr, (byte)val)

#define Notify_ChatRefreshFriend(ptr)               Notify_1_ValEvent(CHAT_REFRESH_FRIEND, ptr)

// misc
#define Notify_ShowNotifier(str, val0, val1)        Notify_3_ValEvent(SHOW_NOTIFIER, val0, str, val1)
#define Notify_ShowConnState(val0, str, val1)       Notify_3_ValEvent(SHOW_CONN_STATE, val0, str, val1)
#define Notify_ShowQueueCount(val)                  Notify_1_ValEvent(SHOW_QUEUE_COUNT, (uint32)val)
#define Notify_ShowUpdateCatTabTitles()             Notify_0_ValEvent(SHOW_UPDATE_CAT_TABS)


#define AddLogLineM(x,y); 			theApp.NotifyEvent(GUIEvent(ADDLOGLINE,x,y));
#define AddDebugLogLineM(x,y); 	theApp.NotifyEvent(GUIEvent(ADDDEBUGLOGLINE,x,y));

#endif // __GUIEVENTS_H__
