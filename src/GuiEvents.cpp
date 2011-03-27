//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "GuiEvents.h"
#include "amule.h"
#include "PartFile.h"
#include "DownloadQueue.h"
#include "ServerList.h"
#include "Preferences.h"
#include "ExternalConn.h"
#include "SearchFile.h"
#include "SearchList.h"
#include "IPFilter.h"
#include "Friend.h"

#ifndef AMULE_DAEMON
#	include "ChatWnd.h"
#	include "amuleDlg.h"
#	include "ServerWnd.h"
#	include "SearchDlg.h"
#	include "TransferWnd.h"
#	include "SharedFilesWnd.h"
#	include "ServerListCtrl.h"
#	include "SourceListCtrl.h"
#	include "SharedFilesCtrl.h"
#	include "DownloadListCtrl.h"
#	include "muuli_wdr.h"
#	include "SharedFilePeersListCtrl.h"
#	ifndef CLIENT_GUI
#		include "PartFileConvertDlg.h"
#		include "PartFileConvert.h"
#	endif
#endif

#ifndef CLIENT_GUI
#	include "UploadQueue.h"
#endif

#include <common/MacrosProgramSpecific.h>

DEFINE_LOCAL_EVENT_TYPE(MULE_EVT_NOTIFY)


namespace MuleNotify
{

	void HandleNotification(const CMuleNotiferBase& ntf)
	{
		if (wxThread::IsMain()) {
#ifdef AMULE_DAEMON
			ntf.Notify();
#else
			if (theApp->amuledlg) {
				ntf.Notify();
			}
#endif
		} else {
			CMuleGUIEvent evt(ntf.Clone());
			wxPostEvent(wxTheApp, evt);
		}
	}


	void HandleNotificationAlways(const CMuleNotiferBase& ntf)
	{
		CMuleGUIEvent evt(ntf.Clone());
		wxPostEvent(wxTheApp, evt);
	}


	void Search_Add_Download(CSearchFile* file, uint8 category)
	{
		theApp->downloadqueue->AddSearchToDownload(file, category);
	}
	

	void ShowUserCount(wxString NOT_ON_DAEMON(str))
	{
#ifndef AMULE_DAEMON
		theApp->amuledlg->ShowUserCount(str);
#endif
	}


	void Search_Update_Progress(uint32 NOT_ON_DAEMON(val))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_searchwnd) {
			if (val == 0xffff) {
				// Global search ended
				theApp->amuledlg->m_searchwnd->ResetControls();
			} else if (val == 0xfffe) {
				// Kad search ended
				theApp->amuledlg->m_searchwnd->KadSearchEnd(0);
			} else {
				theApp->amuledlg->m_searchwnd->UpdateProgress(val);
			}
		}
#endif
	}

	
	void DownloadCtrlUpdateItem(const void* item)
	{
#ifndef CLIENT_GUI
		theApp->ECServerHandler->m_ec_notifier->DownloadFile_SetDirty((CPartFile *)item);
#endif
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->downloadlistctrl) {
			theApp->amuledlg->m_transferwnd->downloadlistctrl->UpdateItem(item);
		}
#endif
	}


	void NodesURLChanged(wxString NOT_ON_DAEMON(url))
	{
#ifndef AMULE_DAEMON
		CastByID(IDC_NODESLISTURL, NULL, wxTextCtrl)->SetValue(url);
#endif
	}

	void ServersURLChanged(wxString NOT_ON_DAEMON(url))
	{
#ifndef AMULE_DAEMON
		CastByID(IDC_SERVERLISTURL, NULL, wxTextCtrl)->SetValue(url);
#endif
	}

	void ShowGUI()
	{
#ifndef AMULE_DAEMON
		theApp->amuledlg->Iconize(false);
#endif
	}
	
	void SourceCtrlUpdateSource(uint32 NOT_ON_DAEMON(source), SourceItemType NOT_ON_DAEMON(type))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->clientlistctrl) {
			theApp->amuledlg->m_transferwnd->clientlistctrl->UpdateItem(source, type);
		}
#endif
	}
	
	void SourceCtrlAddSource(CPartFile* NOT_ON_DAEMON(owner), CClientRef NOT_ON_DAEMON(source), SourceItemType NOT_ON_DAEMON(type))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->clientlistctrl) {
			theApp->amuledlg->m_transferwnd->clientlistctrl->AddSource(owner, source, type);
		}
#endif
	}
	
	void SourceCtrlRemoveSource(uint32 NOT_ON_DAEMON(source), const CPartFile* NOT_ON_DAEMON(owner))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->clientlistctrl) {
			theApp->amuledlg->m_transferwnd->clientlistctrl->RemoveSource(source, owner);
		}
#endif
	}
	
	void SharedCtrlAddClient(CKnownFile* NOT_ON_DAEMON(owner), CClientRef NOT_ON_DAEMON(source), SourceItemType NOT_ON_DAEMON(type))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->peerslistctrl) {
			theApp->amuledlg->m_sharedfileswnd->peerslistctrl->AddSource(owner, source, type);
		}
#endif
	}
	
	void SharedCtrlRefreshClient(uint32 NOT_ON_DAEMON(client), SourceItemType NOT_ON_DAEMON(type))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->peerslistctrl) {
			theApp->amuledlg->m_sharedfileswnd->peerslistctrl->UpdateItem(client, type);
		}
#endif
	}
	
	void SharedCtrlRemoveClient(uint32 NOT_ON_DAEMON(source), const CKnownFile* NOT_ON_DAEMON(owner))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->peerslistctrl) {
			theApp->amuledlg->m_sharedfileswnd->peerslistctrl->RemoveSource(source, owner);
		}
#endif
	}
	
	void ServerRefresh(CServer* NOT_ON_DAEMON(server))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->RefreshServer(server);
		}
#endif
	}
	
	void ChatUpdateFriend(CFriend * NOT_ON_DAEMON(toupdate))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_chatwnd) {
			theApp->amuledlg->m_chatwnd->UpdateFriend(toupdate);
		}
#endif
	}
	
	void ChatRemoveFriend(CFriend * toremove)
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_chatwnd) {
			theApp->amuledlg->m_chatwnd->RemoveFriend(toremove);
		}
#endif
		delete toremove;
	}
	
#ifdef CLIENT_GUI
	
	void PartFile_Swap_A4AF(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_SWAP_A4AF_THIS);
	}

	void PartFile_Swap_A4AF_Auto(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_SWAP_A4AF_THIS_AUTO);
	}
	
	void PartFile_Swap_A4AF_Others(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_SWAP_A4AF_OTHERS);
	}

	void PartFile_Pause(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_PAUSE);
	}
	
	void PartFile_Resume(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_RESUME);
	}
	
	void PartFile_Stop(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_STOP);
	}

	void PartFile_PrioAuto(CPartFile* file, bool val)
	{
		theApp->downloadqueue->AutoPrio(file, val);
	}

	void PartFile_PrioSet(CPartFile* file, uint8 newDownPriority, bool)
	{
		theApp->downloadqueue->Prio(file, newDownPriority);
	}

	void PartFile_Delete(CPartFile* file)
	{
		theApp->downloadqueue->SendFileCommand(file, EC_OP_PARTFILE_DELETE);
	}
	
	void PartFile_SetCat(CPartFile* file, uint32 val)
	{
		theApp->downloadqueue->Category(file, val);
	}
	
	void KnownFile_Up_Prio_Set(CKnownFile* file, uint8 val)
	{
		theApp->sharedfiles->SetFilePrio(file, val);
	}
	
	void KnownFile_Up_Prio_Auto(CKnownFile* file)
	{
		theApp->sharedfiles->SetFilePrio(file, PR_AUTO);
	}

	void KnownFile_Comment_Set(CKnownFile* file, wxString comment, int8 rating)
	{
		theApp->sharedfiles->SetFileCommentRating(file, comment, rating);
	}
	
	void Download_Set_Cat_Prio(uint8, uint8)
	{
	}
	
	void Download_Set_Cat_Status(uint8, int)
	{
	}
	
	void Upload_Resort_Queue()
	{
	}

#else

	void SharedFilesShowFile(CKnownFile* NOT_ON_DAEMON(file))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl) {
			theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFile(file);
		}
#endif
	}

	void SharedFilesRemoveFile(CKnownFile* NOT_ON_DAEMON(file))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl) {
			theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->RemoveFile(file);
		}
#endif
	}
	
	void SharedFilesRemoveAllFiles()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd) {
			theApp->amuledlg->m_sharedfileswnd->RemoveAllSharedFiles();
		}
#endif
	}

	
	void SharedFilesShowFileList()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl) {
			theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->ShowFileList();
		}
#endif
	}


	void SharedFilesUpdateItem(CKnownFile* NOT_ON_DAEMON(file))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_sharedfileswnd && theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl) {
			theApp->amuledlg->m_sharedfileswnd->sharedfilesctrl->UpdateItem(file);
		}
#endif
	}


	void DownloadCtrlAddFile(CPartFile* file)
	{
		theApp->ECServerHandler->m_ec_notifier->DownloadFile_AddFile(file);
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->downloadlistctrl ) {
			theApp->amuledlg->m_transferwnd->downloadlistctrl->AddFile(file);
		}
#endif
	}

	void DownloadCtrlRemoveFile(CPartFile* file)
	{
		theApp->ECServerHandler->m_ec_notifier->DownloadFile_RemoveFile(file);
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->downloadlistctrl) {
			theApp->amuledlg->m_transferwnd->downloadlistctrl->RemoveFile(file);
		}
#endif
	}

	void DownloadCtrlSort()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd && theApp->amuledlg->m_transferwnd->downloadlistctrl) {
			theApp->amuledlg->m_transferwnd->downloadlistctrl->SortList();
		}
#endif
	}
	
	void ServerAdd(CServer* NOT_ON_DAEMON(server))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->AddServer(server);
		}
#endif
	}
	
	void ServerRemove(CServer* NOT_ON_DAEMON(server))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->RemoveServer(server);
		}
#endif
	}

	void ServerRemoveDead()
	{
		if (theApp->serverlist) {
			theApp->serverlist->RemoveDeadServers();
		}		
	}
	
	void ServerRemoveAll()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->DeleteAllItems();
		}
#endif
	}
	
	void ServerHighlight(CServer* NOT_ON_DAEMON(server), bool NOT_ON_DAEMON(highlight))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->HighlightServer(server, highlight);
		}
#endif
	}
	
	void ServerFreeze()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->Freeze();
		}
#endif
	}
	
	void ServerThaw()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd && theApp->amuledlg->m_serverwnd->serverlistctrl) {
			theApp->amuledlg->m_serverwnd->serverlistctrl->Thaw();
		}
#endif
	}
	
	void ServerUpdateED2KInfo()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd) {
			theApp->amuledlg->m_serverwnd->UpdateED2KInfo();
		}
#endif
	}
	
	void ServerUpdateKadKInfo()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_serverwnd) {
			theApp->amuledlg->m_serverwnd->UpdateKadInfo();
		}		
#endif
	}


	void SearchCancel()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->ResetControls();
		}
#endif
	}
	
	void SearchLocalEnd()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->LocalSearchEnd();
		}
#endif
	}

	void KadSearchEnd(uint32 NOT_ON_DAEMON(id))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->KadSearchEnd(id);
		}
#endif
		theApp->searchlist->SetKadSearchFinished();
	}
	
	void Search_Update_Sources(CSearchFile* result)
	{
		result->SetDownloadStatus();
#ifndef AMULE_DAEMON
		if (theApp->amuledlg && theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->UpdateResult(result);
		}
#endif
	}
	
	void Search_Add_Result(CSearchFile* NOT_ON_DAEMON(result))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg && theApp->amuledlg->m_searchwnd) {
			theApp->amuledlg->m_searchwnd->AddResult(result);
		}
#endif
	}

	
	void ChatConnResult(bool NOT_ON_DAEMON(success), uint64 NOT_ON_DAEMON(id), wxString NOT_ON_DAEMON(message))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_chatwnd) {
			theApp->amuledlg->m_chatwnd->ConnectionResult(success, message, id);
		}
#endif
	}
	
	void ChatProcessMsg(uint64 NOT_ON_DAEMON(sender), wxString NOT_ON_DAEMON(message))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_chatwnd) {
			theApp->amuledlg->m_chatwnd->ProcessMessage(sender, message);
		}
#endif
	}
	

	void ChatSendCaptcha(wxString NOT_ON_DAEMON(captcha), uint64 NOT_ON_DAEMON(to_id))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_chatwnd) {
			theApp->amuledlg->m_chatwnd->SendMessage(captcha, wxEmptyString, to_id);
		}
#endif
	}
	

	void ShowConnState(long NOT_ON_DAEMON(forceUpdate))
	{
#ifndef AMULE_DAEMON
		theApp->amuledlg->ShowConnectionState(forceUpdate != 0);
#endif
	}
	
	void ShowUpdateCatTabTitles()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd) {
			theApp->amuledlg->m_transferwnd->UpdateCatTabTitles();
		}
#endif
	}

	void CategoryAdded()
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd) {
			theApp->amuledlg->m_transferwnd->
				AddCategory(theApp->glob_prefs->GetCategory(
					theApp->glob_prefs->GetCatCount()-1));
		}
#endif
	}
	
	void CategoryUpdate(uint32 NOT_ON_DAEMON(cat))
	{
#ifndef AMULE_DAEMON
		if (theApp->amuledlg->m_transferwnd) {
			theApp->amuledlg->m_transferwnd->UpdateCategory(cat);
			theApp->amuledlg->m_transferwnd->downloadlistctrl->Refresh();
			theApp->amuledlg->m_searchwnd->UpdateCatChoice();
		}
#endif
	}
	
	void CategoryDelete(uint32 cat)
	{
#ifdef AMULE_DAEMON
		if (cat > 0) {
			theApp->downloadqueue->ResetCatParts(cat);
			theApp->glob_prefs->RemoveCat(cat);
			if ( theApp->glob_prefs->GetCatCount() == 1 ) {
				thePrefs::SetAllcatFilter( acfAll );
			}
			theApp->glob_prefs->SaveCats();
		}
#else
		if (theApp->amuledlg->m_transferwnd) {
			theApp->amuledlg->m_transferwnd->RemoveCategory(cat);
		}
#endif
	}

	
	void PartFile_Swap_A4AF(CPartFile* file)
	{
		if ((file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)) {
			CPartFile::SourceSet::const_iterator it = file->GetA4AFList().begin();
			for ( ; it != file->GetA4AFList().end(); ) {
				it++->SwapToAnotherFile(true, false, false, file);
			}
		}
	}
	
	void PartFile_Swap_A4AF_Auto(CPartFile* file)
	{
		file->SetA4AFAuto(!file->IsA4AFAuto());
	}
	
	void PartFile_Swap_A4AF_Others(CPartFile* file)
	{
		if ((file->GetStatus(false) == PS_READY) || (file->GetStatus(false) == PS_EMPTY)) {
			CPartFile::SourceSet::const_iterator it = file->GetSourceList().begin();
			for( ; it != file->GetSourceList().end(); ) {
				it++->SwapToAnotherFile(false, false, false, NULL);
			}
		}
	}
	
	void PartFile_Pause(CPartFile* file)
	{
		file->PauseFile();
	}

	void PartFile_Resume(CPartFile* file)
	{
		file->ResumeFile();
		file->SavePartFile();
	}

	void PartFile_Stop(CPartFile* file)
	{
		file->StopFile();
	}

	void PartFile_PrioAuto(CPartFile* file, bool val)
	{
		file->SetAutoDownPriority(val);
	}

	void PartFile_PrioSet(CPartFile* file, uint8 newDownPriority, bool bSave)
	{
		file->SetDownPriority(newDownPriority, bSave);
	}
	
	void PartFile_Delete(CPartFile* file)
	{
		file->Delete();
	}
	
	void PartFile_SetCat(CPartFile* file, uint32 val)
	{
		file->SetCategory(val);
	}

	
	void KnownFile_Up_Prio_Set(CKnownFile* file, uint8 val)
	{
		file->SetAutoUpPriority(false);
		file->SetUpPriority(val);
	}
	
	void KnownFile_Up_Prio_Auto(CKnownFile* file)
	{
		file->SetAutoUpPriority(true);
		file->UpdateAutoUpPriority();
	}
	
	void KnownFile_Comment_Set(CKnownFile* file, wxString comment, int8 rating)
	{
		file->SetFileCommentRating(comment, rating);
		SharedFilesUpdateItem(file);
	}
	

	void Download_Set_Cat_Prio(uint8 cat, uint8 newprio)
	{
		theApp->downloadqueue->SetCatPrio(cat, newprio);
	}
	
	void Download_Set_Cat_Status(uint8 cat, int newstatus)
	{
		theApp->downloadqueue->SetCatStatus(cat, newstatus);
	}

	void Upload_Resort_Queue()
	{
		theApp->uploadqueue->ResortQueue();
	}

	void IPFilter_Reload()
	{
		theApp->ipfilter->Reload();
	}

	void IPFilter_Update(wxString url)
	{
		theApp->ipfilter->Update(url);
	}

	void Client_Delete(CClientRef client)
	{
		client.Safe_Delete();
	}

#ifndef AMULE_DAEMON
	void ConvertUpdateProgress(float percent, wxString text, wxString header)
	{
		CPartFileConvertDlg::UpdateProgress(percent, text, header);
	}

	void ConvertUpdateJobInfo(ConvertInfo info)
	{
		CPartFileConvertDlg::UpdateJobInfo(info);
	}

	void ConvertRemoveJobInfo(unsigned id)
	{
		CPartFileConvertDlg::RemoveJobInfo(id);
	}

	void ConvertClearInfos()
	{
		CPartFileConvertDlg::ClearInfo();
	}

	void ConvertRemoveJob(unsigned id)
	{
		CPartFileConvert::RemoveJob(id);
	}

	void ConvertRetryJob(unsigned id)
	{
		CPartFileConvert::RetryJob(id);
	}

	void ConvertReaddAllJobs()
	{
		CPartFileConvert::ReaddAllJobs();
	}
#endif	// #ifndef AMULE_DAEMON

#endif	// #ifndef CLIENT_GUI
}
// File_checked_for_headers
