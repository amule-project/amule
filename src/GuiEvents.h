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

#include <wx/app.h>

#include "Types.h"
#include "Constants.h"


class CKnownFile;
class CSearchFile;
class CPartFile;
class CServer;


DECLARE_LOCAL_EVENT_TYPE(MULE_EVT_NOTIFY, -1);


/**
 * This namespaces contains a number of functions and classes
 * related to defered function calls, allowing a notification
 * call to be delayed till it can be initiated from the main
 * thread.
 */
namespace MuleNotify
{
	////////////////////////////////////////////////////////////
	// Notification handlers
	//
	// These functions should not be called directly, but
	// through the Notify_*, etc. macros.
	//

	void SharedFilesShowFile(CKnownFile* file);
	void SharedFilesRemoveFile(CKnownFile* file);
	void SharedFilesRemoveAllFiles();
	void SharedFilesShowFileList();
	void SharedFilesUpdateItem(CKnownFile* file);

	void DownloadCtrlUpdateItem(const void* item);
	void DownloadCtrlAddFile(CPartFile* file);
	void DownloadCtrlAddSource(CPartFile* owner, CUpDownClient* source, DownloadItemType type);
	void DownloadCtrlRemoveFile(CPartFile* file);
	void DownloadCtrlRemoveSource(const CUpDownClient* source, const CPartFile* owner);
	void DownloadCtrlHideSource(CPartFile* file);
	void DownloadCtrlSort();

	void ClientCtrlAddClient(CUpDownClient* client, ViewType type);
	void ClientCtrlRefreshClient(CUpDownClient* client, ViewType type);
	void ClientCtrlRemoveClient(CUpDownClient* client, ViewType type);

	void ServerAdd(CServer* server);
	void ServerRemove(CServer* server);
	void ServerRemoveDead();
	void ServerRemoveAll();
	void ServerHighlight(CServer* server, bool highlight);
	void ServerRefresh(CServer* server);
	void ServerFreeze();
	void ServerThaw();
	void ServerUpdateED2KInfo();
	void ServerUpdateKadKInfo();

	void SearchCancel();
	void SearchLocalEnd();
	void KadSearchEnd(uint32 id);
	void Search_Update_Sources(CSearchFile* result);
	void Search_Add_Result(CSearchFile* result);

	void ChatRefreshFriend(uint32 lastUsedIP, uint32 lastUsedPort, wxString name);
	void ChatConnResult(bool success, uint64 id, wxString message);
	void ChatProcessMsg(uint64 sender, wxString message);
	
	void ShowConnState(long state);
	void ShowUserCount(wxString str);
	void ShowQueueCount(uint32 count);
	void ShowUpdateCatTabTitles();
	void ShowGUI();

	void CategoryAdded();
	void CategoryUpdate(uint32 cat);
	void CategoryDelete(uint32 cat);

	
	//
	// GUI -> core notification
	//

	void PartFile_RemoveNoNeeded(CPartFile* file);
	void PartFile_RemoveFullQueue(CPartFile* file);
	void PartFile_RemoveHighQueue(CPartFile* file);
	void PartFile_SourceCleanup(CPartFile* file);
	void PartFile_Swap_A4AF(CPartFile* file);
	void PartFile_Swap_A4AF_Auto(CPartFile* file);
	void PartFile_Swap_A4AF_Others(CPartFile* file);
	void PartFile_Pause(CPartFile* file);
	void PartFile_Resume(CPartFile* file);
	void PartFile_Stop(CPartFile* file);
	void PartFile_PrioAuto(CPartFile* file, bool val);
	void PartFile_PrioSet(CPartFile* file, uint8 newDownPriority, bool bSave);
	void PartFile_Delete(CPartFile* file);
	void PartFile_SetCat(CPartFile* file, uint32 val);

	void KnownFile_Up_Prio_Set(CKnownFile* file, uint8 val);
	void KnownFile_Up_Prio_Auto(CKnownFile* file);
	void KnownFile_Comment_Set(CKnownFile* file, wxString comment);

	void Search_Add_Download(CSearchFile* result, uint8 category);
	void Search_Update_Progress(uint32 value);

	void Download_Set_Cat_Prio(uint8 cat, uint8 newprio);
	void Download_Set_Cat_Status(uint8 cat, int newstatus);


	////////////////////////////////////////////////////////////
	// Notification utilities
	
	/** 
	 * The base class of the functions.
	 *
	 * This class allows the the notification call to be executed
	 * without knowing the exact specifics of a given functor.
	 */
	class CMuleNotiferBase
	{
	public:
		/** The constructor does nothing. */
		CMuleNotiferBase() {};
		/** The destructor is virtual since we will be deleting pointers to this type. */
		virtual ~CMuleNotiferBase() {};

		/** Executes the actual notification call. */
		virtual void Notify() const = 0;
		/** Returns a copy of the functor (function + arguments). */
		virtual CMuleNotiferBase* Clone() const = 0;
	};


	/** Notification functor for functions taking no arguments. */
	class CMuleNotifier0 : public CMuleNotiferBase
	{
	public:
		typedef void (*FuncType)();
		
		/** Creates a functor from the given function. */
		CMuleNotifier0(FuncType func)
			: m_func(func) {}

		/** @see CMuleNotifierBase::Notify */
		virtual void Notify() const {
			m_func();
		}

		/** @see CMuleNotifierBase::Clone */
		virtual CMuleNotiferBase* Clone() const {
			return new CMuleNotifier0(m_func);
		}		

	private:
		FuncType	m_func;
	};


	/** Notification functor for functions taking 1 arguments. */
	template <typename ARG>
	class CMuleNotifier1 : public CMuleNotiferBase
	{
	public:
		typedef void (*FuncType)(ARG);
		
		/** Creates a functor from the given function and arguments. */
		CMuleNotifier1(FuncType func, ARG arg)
			: m_func(func), m_arg(arg) {}

		/** @see CMuleNotifierBase::Notify */
		virtual void Notify() const {
			m_func(m_arg);
		}

		/** @see CMuleNotifierBase::Clone */
		virtual CMuleNotiferBase* Clone() const {
			return new CMuleNotifier1<ARG>(m_func, m_arg);
		}

	private:
		FuncType	m_func;
		ARG			m_arg;
	};


	/** Notification functor for functions taking 2 arguments. */
	template <typename ARG_1, typename ARG_2>
	class CMuleNotifier2 : public CMuleNotiferBase
	{
	public:
		typedef void (*FuncType)(ARG_1, ARG_2);
		
		/** Creates a functor from the given function and arguments. */
		CMuleNotifier2(FuncType func, ARG_1 arg1, ARG_2 arg2)
			: m_func(func), m_arg1(arg1), m_arg2(arg2) {}

		/** @see CMuleNotifierBase:: Notify */
		virtual void Notify() const {
			m_func(m_arg1, m_arg2);
		}

		/** @see CMuleNotifierBase::Clone */
		virtual CMuleNotiferBase* Clone() const {
			return new CMuleNotifier2<ARG_1, ARG_2>(m_func, m_arg1, m_arg2);
		}

	private:
		FuncType	m_func;
		ARG_1		m_arg1;
		ARG_2		m_arg2;
	};


	/** Notification functor for functions taking 3 arguments. */
	template <typename ARG_1, typename ARG_2, typename ARG_3>
	class CMuleNotifier3 : public CMuleNotiferBase
	{
	public:
		typedef void (*FuncType)(ARG_1, ARG_2, ARG_3);
		
		/** Creates a functor from the given function and arguments. */
		CMuleNotifier3(FuncType func, ARG_1 arg1, ARG_2 arg2, ARG_3 arg3)
			: m_func(func), m_arg1(arg1), m_arg2(arg2), m_arg3(arg3) {}

		/** @see CMuleNotifierBase:: Notify */
		virtual void Notify() const {
			m_func(m_arg1, m_arg2, m_arg3);
		}

		/** @see CMuleNotifierBase::Clone */
		virtual CMuleNotiferBase* Clone() const {
			return new CMuleNotifier3<ARG_1, ARG_2, ARG_3>(m_func, m_arg1, m_arg2, m_arg3);
		}

	private:
		FuncType	m_func;
		ARG_1		m_arg1;
		ARG_2		m_arg2;
		ARG_3		m_arg3;
	};


	/** 
	 * This event is sent when a worker-thread makes use of a notify-macro.
	 *
	 * This insures that all notifications are executed on the main thread,
	 * thereby improving overall threadsafety. The events are currently
	 * sent to wxTheApp.
	 */
	class CMuleGUIEvent : public wxEvent
	{
	public:
		/** Takes ownership a notifier functor. */
		CMuleGUIEvent(CMuleNotiferBase* ntf)
			: wxEvent(-1, MULE_EVT_NOTIFY)
			, m_functor(ntf)
		{
			wxASSERT(m_functor);
		}
		
		/** Destructor, frees the functor object. */
		virtual ~CMuleGUIEvent() {
			delete m_functor;
		}
		
		/** Executes the notification. */
		void Notify() const {
			m_functor->Notify();
		}

		/** @see wxEvent::Clone */
		virtual wxEvent* Clone() const {
			return new CMuleGUIEvent(m_functor->Clone());
		}
		
	private:
		/** Not copyable. */
		CMuleGUIEvent(const CMuleGUIEvent&);
		/** Not assignable. */
		CMuleGUIEvent& operator=(const CMuleGUIEvent&);
		
		//! The actual functor object, 
		CMuleNotiferBase* m_functor;
	};

	
	/**
	 * This function will execute or queue a given notification functor.
	 *
	 * If the caller is the main thread, the functor is executed immediatly,
	 * thus acting like a regular function call. OTOH, if the caller is a 
	 * worker thread, the functor is cloned and sent via an event to
	 * wxTheApp.
	 */
	void HandleNotification(const CMuleNotiferBase& ntf);
	
	/** 
	 * These functions take a function pointer and a set of arguments,
	 * matching those of the function-pointer. A functor is created
	 * from these and either executed immediatly, or sent as an event
	 * in the case of non-main threads calling the functions.
	 *
	 * Note that the return-value of the function must be void.
	 *
	 * IMPORTANT: Note that the functions passed to DoNotify must not
	 * take arguments via references, since this causes the functors
	 * to store references to the arguments, rather than a copy and
	 * thus ends up with dangling references.
	 */
	//@{
	inline void DoNotify(void (*func)()) {
		HandleNotification(CMuleNotifier0(func));
	}
	template <typename A1A, typename A1B>
	inline void DoNotify(void (*func)(A1A), A1B arg1) {
		HandleNotification(CMuleNotifier1<A1A>(func, arg1));
	}
	template <typename A1A, typename A1B, typename A2A, typename A2B>
	inline void DoNotify(void (*func)(A1A, A2A), A1B arg1, A2B arg2) {
		HandleNotification(CMuleNotifier2<A1A, A2A>(func, arg1, arg2));
	}
	template <typename A1A, typename A1B, typename A2A, typename A2B, typename A3A, typename A3B>
	inline void DoNotify(void (*func)(A1A, A2A, A3A), A1B arg1, A2B arg2, A3B arg3) {
		HandleNotification(CMuleNotifier3<A1A, A2A, A3A>(func, arg1, arg2, arg3));
	}
	//@}
};


//! Placing CMuleGUIEvent in the global namespace.
using MuleNotify::CMuleGUIEvent;

//! The event-handler type that takes a CMuleGUIEvent.
typedef void (wxEvtHandler::*MuleNotifyEventFunction)(CMuleGUIEvent&);

//! Event-handler for completed hashings of new shared files and partfiles.
#define EVT_MULE_NOTIFY(func) \
	DECLARE_EVENT_TABLE_ENTRY(MULE_EVT_NOTIFY, -1, -1, \
	(wxObjectEventFunction) (wxEventFunction) \
	wxStaticCastEvent(MuleNotifyEventFunction, &func), (wxObject*) NULL),




// SharedFilesCtrl
#define Notify_SharedFilesShowFile(file)		MuleNotify::DoNotify(&MuleNotify::SharedFilesShowFile, file)
#define Notify_SharedFilesRemoveFile(file)		MuleNotify::DoNotify(&MuleNotify::SharedFilesRemoveFile, file)
#define Notify_SharedFilesRemoveAllItems()		MuleNotify::DoNotify(&MuleNotify::SharedFilesRemoveAllFiles)
#define Notify_SharedFilesShowFileList()        	MuleNotify::DoNotify(&MuleNotify::SharedFilesShowFileList)
#define Notify_SharedFilesSort()			MuleNotify::DoNotify(&MuleNotify::SharedFilesSort)
#define Notify_SharedFilesUpdateItem(file)		MuleNotify::DoNotify(&MuleNotify::SharedFilesUpdateItem, file)

// download ctrl
#define Notify_DownloadCtrlUpdateItem(ptr)		MuleNotify::DoNotify(&MuleNotify::DownloadCtrlUpdateItem, ptr)
#define Notify_DownloadCtrlAddFile(file)		MuleNotify::DoNotify(&MuleNotify::DownloadCtrlAddFile, file)
#define Notify_DownloadCtrlAddSource(p0, p1, val)	MuleNotify::DoNotify(&MuleNotify::DownloadCtrlAddSource, p0, p1, val)
#define Notify_DownloadCtrlRemoveFile(file)		MuleNotify::DoNotify(&MuleNotify::DownloadCtrlRemoveFile, file)
#define Notify_DownloadCtrlRemoveSource(ptr0, ptr1)	MuleNotify::DoNotify(&MuleNotify::DownloadCtrlRemoveSource, ptr0, ptr1)
#define Notify_DownloadCtrlHideSource(ptr)		MuleNotify::DoNotify(&MuleNotify::DownloadCtrlHideSource, ptr)
#define Notify_DownloadCtrlSort()			MuleNotify::DoNotify(&MuleNotify::DownloadCtrlSort)

// upload ctrl
#define Notify_UploadCtrlAddClient(ptr)			MuleNotify::DoNotify(&MuleNotify::ClientCtrlAddClient, ptr, vtUploading)
#define Notify_UploadCtrlRefreshClient(ptr)		MuleNotify::DoNotify(&MuleNotify::ClientCtrlRefreshClient, ptr, vtUploading)
#define Notify_UploadCtrlRemoveClient(ptr)		MuleNotify::DoNotify(&MuleNotify::ClientCtrlRemoveClient, ptr, vtUploading)

// client ctrl
#define Notify_ClientCtrlAddClient(ptr)			MuleNotify::DoNotify(&MuleNotify::ClientCtrlAddClient, ptr, vtClients)
#define Notify_ClientCtrlRefreshClient(ptr)		MuleNotify::DoNotify(&MuleNotify::ClientCtrlRefreshClient, ptr, vtClients)
#define Notify_ClientCtrlRemoveClient(ptr)		MuleNotify::DoNotify(&MuleNotify::ClientCtrlRemoveClient, ptr, vtClients)

// queue list
#define Notify_QlistAddClient(ptr)			MuleNotify::DoNotify(&MuleNotify::ClientCtrlAddClient, ptr, vtQueued)
#define Notify_QlistRemoveClient(ptr)			MuleNotify::DoNotify(&MuleNotify::ClientCtrlRefreshClient, ptr, vtQueued)
#define Notify_QlistRefreshClient(ptr)			MuleNotify::DoNotify(&MuleNotify::ClientCtrlRemoveClient, ptr, vtQueued)

// server
#define Notify_ServerAdd(ptr)				MuleNotify::DoNotify(&MuleNotify::ServerAdd, ptr)
#define Notify_ServerRemove(ptr)			MuleNotify::DoNotify(&MuleNotify::ServerRemove, ptr)
#define Notify_ServerRemoveDead()			MuleNotify::DoNotify(&MuleNotify::ServerRemoveDead)
#define Notify_ServerRemoveAll()			MuleNotify::DoNotify(&MuleNotify::ServerRemoveAll)
#define Notify_ServerHighlight(ptr, val)		MuleNotify::DoNotify(&MuleNotify::ServerHighlight, ptr, val)
#define Notify_ServerRefresh(ptr)			MuleNotify::DoNotify(&MuleNotify::ServerRefresh, ptr)
#define Notify_ServerFreeze()				MuleNotify::DoNotify(&MuleNotify::ServerFreeze)
#define Notify_ServerThaw()				MuleNotify::DoNotify(&MuleNotify::ServerThaw)
#define Notify_ServerUpdateED2KInfo()			MuleNotify::DoNotify(&MuleNotify::ServerUpdateED2KInfo)
#define Notify_ServerUpdateKadKInfo()			MuleNotify::DoNotify(&MuleNotify::ServerUpdateKadKInfo)

// search
#define Notify_SearchCancel()				MuleNotify::DoNotify(&MuleNotify::SearchCancel)
#define Notify_SearchLocalEnd()				MuleNotify::DoNotify(&MuleNotify::SearchLocalEnd)
#define Notify_KadSearchEnd(val)			MuleNotify::DoNotify(&MuleNotify::KadSearchEnd, val)
#define Notify_Search_Update_Sources(ptr)		MuleNotify::DoNotify(&MuleNotify::Search_Update_Sources, ptr)
#define Notify_Search_Add_Result(s)			MuleNotify::DoNotify(&MuleNotify::Search_Add_Result, s)

// chat
#define Notify_ChatRefreshFriend(val0, val1, s)		MuleNotify::DoNotify(&MuleNotify::ChatRefreshFriend, val0, val1, s)
#define Notify_ChatConnResult(val0, val1, s)		MuleNotify::DoNotify(&MuleNotify::ChatConnResult, val0, val1, s)
#define Notify_ChatProcessMsg(val0, s)			MuleNotify::DoNotify(&MuleNotify::ChatProcessMsg, val0, s)

// misc
#define Notify_ShowConnState(val)			MuleNotify::DoNotify(&MuleNotify::ShowConnState, val)
#define Notify_ShowUserCount(str)			MuleNotify::DoNotify(&MuleNotify::ShowUserCount, str)
#define Notify_ShowQueueCount(val)			MuleNotify::DoNotify(&MuleNotify::ShowQueueCount, val)
#define Notify_ShowUpdateCatTabTitles()			MuleNotify::DoNotify(&MuleNotify::ShowUpdateCatTabTitles)
#define Notify_ShowGUI()				MuleNotify::DoNotify(&MuleNotify::ShowGUI)

// categories
#define Notify_CategoryAdded()				MuleNotify::DoNotify(&MuleNotify::CategoryAdded)
#define Notify_CategoryUpdate(cat)			MuleNotify::DoNotify(&MuleNotify::CategoryUpdate, cat)
#define Notify_CategoryDelete(cat)			MuleNotify::DoNotify(&MuleNotify::CategoryDelete, cat)

//
// GUI -> core notification
//

// PartFile
#define CoreNotify_PartFile_RemoveNoNeeded(ptr)		MuleNotify::DoNotify(&MuleNotify::PartFile_RemoveNoNeeded, ptr)
#define CoreNotify_PartFile_RemoveFullQueue(ptr)	MuleNotify::DoNotify(&MuleNotify::PartFile_RemoveFullQueue, ptr)
#define CoreNotify_PartFile_RemoveHighQueue(ptr)	MuleNotify::DoNotify(&MuleNotify::PartFile_RemoveHighQueue, ptr)
#define CoreNotify_PartFile_SourceCleanup(ptr)		MuleNotify::DoNotify(&MuleNotify::PartFile_SourceCleanup, ptr)
#define CoreNotify_PartFile_Swap_A4AF(ptr)		MuleNotify::DoNotify(&MuleNotify::PartFile_Swap_A4AF, ptr)
#define CoreNotify_PartFile_Swap_A4AF_Auto(ptr)		MuleNotify::DoNotify(&MuleNotify::PartFile_Swap_A4AF_Auto, ptr)
#define CoreNotify_PartFile_Swap_A4AF_Others(ptr)	MuleNotify::DoNotify(&MuleNotify::PartFile_Swap_A4AF_Others, ptr)
#define CoreNotify_PartFile_Pause(ptr)			MuleNotify::DoNotify(&MuleNotify::PartFile_Pause, ptr)
#define CoreNotify_PartFile_Resume(ptr)			MuleNotify::DoNotify(&MuleNotify::PartFile_Resume, ptr)
#define CoreNotify_PartFile_Stop(ptr)			MuleNotify::DoNotify(&MuleNotify::PartFile_Stop, ptr)
#define CoreNotify_PartFile_PrioAuto(ptr, val)		MuleNotify::DoNotify(&MuleNotify::PartFile_PrioAuto, ptr, val)
#define CoreNotify_PartFile_PrioSet(p, v0, v1)		MuleNotify::DoNotify(&MuleNotify::PartFile_PrioSet, p, v0, v1)
#define CoreNotify_PartFile_Delete(ptr)			MuleNotify::DoNotify(&MuleNotify::PartFile_Delete, ptr)
#define CoreNotify_PartFile_SetCat(ptr, val)		MuleNotify::DoNotify(&MuleNotify::PartFile_SetCat, ptr, val)

// KnownFile
#define CoreNotify_KnownFile_Up_Prio_Set(ptr, val)	MuleNotify::DoNotify(&MuleNotify::KnownFile_Up_Prio_Set, ptr, val)
#define CoreNotify_KnownFile_Up_Prio_Auto(ptr)		MuleNotify::DoNotify(&MuleNotify::KnownFile_Up_Prio_Auto, ptr)
#define CoreNotify_KnownFile_Comment_Set(ptr, val)	MuleNotify::DoNotify(&MuleNotify::KnownFile_Comment_Set, ptr, val)

// Search
#define CoreNotify_Search_Add_Download(ptr, val)	MuleNotify::DoNotify(&MuleNotify::Search_Add_Download, ptr, val)
#define CoreNotify_Search_Update_Progress(val)		MuleNotify::DoNotify(&MuleNotify::Search_Update_Progress, val)

// download queue
#define CoreNotify_Download_Set_Cat_Prio(cat, pri)	MuleNotify::DoNotify(&MuleNotify::Download_Set_Cat_Prio, cat, pri)
#define CoreNotify_Download_Set_Cat_Status(cat, st)	MuleNotify::DoNotify(&MuleNotify::Download_Set_Cat_Status, cat, st)


#endif // __GUIEVENTS_H__

// File_checked_for_headers
