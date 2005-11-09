//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <unistd.h>			// Needed for close(2) and sleep(3)
#include <wx/defs.h>
#include <wx/gauge.h>
#include <wx/textctrl.h>

#ifdef __WXGTK__
	#include <X11/Xlib.h>		// Needed for XParseGeometry
#endif

#ifdef HAVE_CONFIG_H
	#include "config.h"			//   VERSION
#endif

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>		// Needed for wxFileName::GetPathSeparator()
#include <wx/log.h>
#include <wx/timer.h>
#include <wx/config.h>
#include <wx/clipbrd.h>			// Needed for wxClipBoard
#include <wx/socket.h>			// Needed for wxSocket
#include <wx/splash.h>			// Needed for wxSplashScreen
#include <wx/utils.h>
#include <wx/ipc.h>
#include <wx/intl.h>			// Needed for i18n
#include <wx/mimetype.h>		// For launching default browser
#include <wx/textfile.h>		// Needed for wxTextFile
#include <wx/cmdline.h>			// Needed for wxCmdLineParser
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer
#include <wx/msgdlg.h>			// Needed for wxMessageBox
#include <wx/url.h>
#include <wx/dataobj.h> 		// Needed on wxMotif

#include "amule.h"			// Interface declarations.
#include "GetTickCount.h"		// Needed for GetTickCount
#include "Server.h"			// Needed for GetListName
#include "OtherFunctions.h"		// Needed for GetTickCount
#include "TransferWnd.h"		// Needed for CTransferWnd
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "ServerWnd.h"			// Needed for CServerWnd
#include "StatisticsDlg.h"		// Needed for CStatisticsDlg
#include "UploadQueue.h"		// Needed for CUploadQueue
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "ClientCredits.h"		// Needed for CClientCreditsList
#include "ClientUDPSocket.h"		// Needed for CClientUDPSocket
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "ServerConnect.h"		// Needed for CServerConnect
#include "ServerList.h"			// Needed for CServerList
#include "KnownFileList.h"		// Needed for CKnownFileList
#include "SearchList.h"			// Needed for CSearchList
#include "ClientList.h"			// Needed for CClientList
#include "Preferences.h"		// Needed for CPreferences
#include "ListenSocket.h"		// Needed for CListenSocket
#include "ExternalConn.h"		// Needed for ExternalConn & MuleConnection
#include "ServerSocket.h"		// Needed for CServerSocket
#include "ServerUDPSocket.h"		// Needed for CServerUDPSocket
#include "PartFile.h"			// Needed for CPartFile
#include "AddFileThread.h"		// Needed for CAddFileThread
#include "updownclient.h"		// Needed for CUpDownClient
#include "Packet.h"
#include "AICHSyncThread.h"

#include "muuli_wdr.h"			// Needed for IDs
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "SearchDlg.h"			// Needed for CSearchDlg
#include "ServerListCtrl.h"		// Needed for CServerListCtrl
#include "SharedFilesCtrl.h"		// Needed for CSharedFilesCtrl
#include "DownloadListCtrl.h"		// Needed for CDownloadListCtrl
#include "ClientListCtrl.h"
#include "ChatWnd.h"
#include "Format.h"

#ifdef __WXMAC__
	#include <CoreFoundation/CFBundle.h>
	#include <ApplicationServices/ApplicationServices.h>	// For LSRegisterURL
#endif

#ifndef CLIENT_GUI
#include "InternalEvents.h"		// Needed for wxEVT_*

BEGIN_EVENT_TABLE(CamuleGuiApp, wxApp)

	// Socket handlers
	// Listen Socket
	EVT_SOCKET(LISTENSOCKET_HANDLER, CamuleGuiApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(SERVERUDPSOCKET_HANDLER, CamuleGuiApp::UDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(CLIENTUDPSOCKET_HANDLER, CamuleGuiApp::UDPSocketHandler)

	// Socket timers (TCP + UDP)
	EVT_TIMER(TM_TCPSOCKET, CamuleGuiApp::OnTCPTimer)

	// Core timer
	EVT_TIMER(ID_CORETIMER, CamuleGuiApp::OnCoreTimer)

	EVT_CUSTOM(wxEVT_MULE_NOTIFY_EVENT, -1, CamuleGuiApp::OnNotifyEvent)

	// Async dns handling
	EVT_CUSTOM(wxEVT_CORE_UDP_DNS_DONE, -1, CamuleGuiApp::OnUDPDnsDone)

	EVT_CUSTOM(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleGuiApp::OnSourceDnsDone)

	EVT_CUSTOM(wxEVT_CORE_SERVER_DNS_DONE, -1, CamuleGuiApp::OnServerDnsDone)

	// Hash ended notifier

	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_FINISHED, -1, CamuleGuiApp::OnFinishedHashing)

	// Hashing thread finished and dead

	EVT_CUSTOM(wxEVT_CORE_FILE_HASHING_SHUTDOWN, -1, CamuleGuiApp::OnHashingShutdown)

	// File completion ended notifier
	EVT_CUSTOM(wxEVT_CORE_FINISHED_FILE_COMPLETION, -1, CamuleGuiApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_CUSTOM(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleGuiApp::OnFinishedHTTPDownload)

END_EVENT_TABLE()


IMPLEMENT_APP(CamuleGuiApp)

#endif // CLIENT_GUI

#ifndef __WXMSW__
// Initialization of the static MyTimer member variables.
uint32 MyTimer::tic32 = 0;
uint64 MyTimer::tic64 = 0;


// Global timer. Used to cache GetTickCount() results for better performance.
class MyTimer* mytimer = NULL;
#endif

CamuleGuiBase::CamuleGuiBase()
{
#ifndef __WXMSW__
	// Madcat - Initialize timer as the VERY FIRST thing to avoid any issues later.
	// Kry - I love to init the vars on init, even before timer.
	mytimer = new MyTimer();
#endif
}


CamuleGuiBase::~CamuleGuiBase()
{

}


void CamuleGuiBase::ShowAlert(wxString msg, wxString title, int flags)
{
	wxMessageBox(msg, title, flags);
}


int CamuleGuiBase::InitGui(bool geometry_enabled, wxString &geom_string)
{
	// Standard size is 800x600 at position (0,0)
	int geometry_x = 0;
	int geometry_y = 0;
	unsigned int geometry_width = 800;
	unsigned int geometry_height = 600;

	if ( geometry_enabled ) {
		// I plan on moving this to a seperate function, as it just clutters up OnInit()
#ifdef __WXGTK__
		// Nothing we can do against this unicode2char either.
		XParseGeometry(unicode2char(geom_string), &geometry_x, &geometry_y, &geometry_width, &geometry_height);
		geometry_enabled = true;
#elif defined (__WXMSW__)
		/*
		This implementation might work with mac, provided that the
		SetSize() function works as expected.
		*/

		// Remove possible prefix
		if ( geom_string.GetChar(0) == '=' )
			geom_string.Remove( 0, 1 );

		// Stupid ToLong functions forces me to use longs =(
		long width = geometry_width;
		long height = geometry_height;

		// Get the avilable display area
		wxRect display = wxGetClientDisplayRect();

		// We want to place aMule inside the client area by default
		long x = display.x;
		long y = display.y;

		// Tokenize the string
		wxStringTokenizer tokens(geom_string, wxT("xX+-"));

		// First part: Program width
		if ( tokens.GetNextToken().ToLong( &width ) ) {
			wxString prefix = geom_string[ tokens.GetPosition() - 1 ];
			if ( prefix == wxT("x") || prefix == wxT("X") ) {
				// Second part: Program height
				if ( tokens.GetNextToken().ToLong( &height ) ) {
					prefix = geom_string[ tokens.GetPosition() - 1 ];
					if ( prefix == wxT("+") || prefix == wxT("-") ) {
						// Third part: X-Offset
						if ( tokens.GetNextToken().ToLong( &x ) ) {
							if ( prefix == wxT("-") )
								x = display.GetRight() - ( width + x );
							prefix = geom_string[ tokens.GetPosition() - 1 ];
							if ( prefix == wxT("+") || prefix == wxT("-") ) {
								// Fourth part: Y-Offset
								if ( tokens.GetNextToken().ToLong( &y ) ) {
									if ( prefix == wxT("-") )
										y = display.GetBottom() - ( height + y );
								}
							}
						}
					}
					// We need at least height and width to override default geometry
					geometry_enabled = true;
					geometry_x = x;
					geometry_y = y;
					geometry_width = width;
					geometry_height = height;
				}
			}
		}
#else
		#warning Need to parse the geometry for non-GTK/WIN platforms
#endif
	}
	
	// Should default/last-used position be overridden?
#ifdef CVSDATE
	#ifdef CLIENT_GUI
		m_FrameTitle = wxString::Format(wxT("aMule remote control %s %s"), wxT(VERSION), wxT(CVSDATE));
	#else
		m_FrameTitle = wxString::Format(wxT("aMule %s %s"), wxT(VERSION), wxT(CVSDATE));
	#endif
#else
	#ifdef CLIENT_GUI
		m_FrameTitle = wxString::Format(wxT("aMule remote control %s"), wxT(VERSION));
	#else
		m_FrameTitle = wxString::Format(wxT("aMule %s"), wxT(VERSION));
	#endif
#endif
	if ( geometry_enabled ) {
		amuledlg = new CamuleDlg(NULL, m_FrameTitle,
		                         wxPoint(geometry_x,geometry_y),
		                         wxSize( geometry_width, geometry_height - 58 ));
	} else {
		amuledlg = new CamuleDlg(NULL, m_FrameTitle);
	}

	amuledlg->Init();

	return 0;
}


// Sets the contents of the clipboard. Prior content  erased.
bool CamuleGuiBase::CopyTextToClipboard(wxString strText)
{
	bool ClipBoardOpen = wxTheClipboard->Open();
	if (ClipBoardOpen) {
		wxTheClipboard->UsePrimarySelection(TRUE);
		wxTheClipboard->SetData(new wxTextDataObject(strText));
		wxTheClipboard->Close();
	}
	
	return ClipBoardOpen;
}


void CamuleGuiBase::NotifyEvent(const GUIEvent& WXUNUSED(event))
{

}


#ifndef CLIENT_GUI

int CamuleGuiApp::InitGui(bool geometry_enable, wxString &geometry_string)
{
	CamuleGuiBase::InitGui(geometry_enable, geometry_string);
	SetTopWindow(amuledlg);
	return 0;
}


void CamuleGuiApp::ShowAlert(wxString msg, wxString title, int flags)
{
	CamuleGuiBase::ShowAlert(msg, title, flags);
}


int CamuleGuiApp::OnExit()
{
	if (core_timer) {
		// Stop the Core Timer
		delete core_timer;
	}
	if (amuledlg) {
		amuledlg->StopGuiTimer();
	}
	return CamuleApp::OnExit();
}


void CamuleGuiApp::ShutDown(wxCloseEvent &WXUNUSED(evt))
{
	amuledlg->DlgShutDown();
	amuledlg->Destroy();
	CamuleApp::ShutDown();

#ifndef __WXMSW__
	delete mytimer;
	mytimer = NULL;
#endif
}


bool CamuleGuiApp::OnInit()
{
	amuledlg = NULL;

	if ( !CamuleApp::OnInit() ) {
		return false;
	}

	// Create the Core timer
	core_timer=new wxTimer(this,ID_CORETIMER);
	if (!core_timer) {
		printf("Fatal Error: Failed to create Core Timer");
		OnExit();
	}

	// Start the Core Timer

	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	core_timer->Start(100);

	// Start the Gui Timer

	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	amuledlg->StartGuiTimer();

#ifdef __WXMAC__
	// This tells the OS to notice the ed2kHelperScript.app inside aMule.app.
	// ed2kHelperScript.app describes itself (Info.plist) as handling ed2k URLs.
	// So, from then on the OS will know to pass ed2k URLs to the helper app.
	CFURLRef ed2kHelperUrl = CFBundleCopyAuxiliaryExecutableURL(
		CFBundleGetMainBundle(), CFSTR("ed2kHelperScript.app"));
	if (ed2kHelperUrl) {
		LSRegisterURL(ed2kHelperUrl, true);
		CFRelease(ed2kHelperUrl);
	}
#endif

	return true;
}

void CamuleGuiApp::NotifyEvent(const GUIEvent& event)
{
	if (!amuledlg && (event.ID!=ADDLOGLINE)) {
		return;
	}
	
	switch (event.ID) {
		// GUI->CORE events
		// no need to check pointers: if event is here, gui must be running

		
		// search
		case SEARCH_ADD_TO_DLOAD:
			downloadqueue->AddSearchToDownload((CSearchFile *)event.ptr_value, event.byte_value);
			break;

			
		// PartFile
		case PARTFILE_REMOVE_NO_NEEDED:
			((CPartFile *)event.ptr_value)->CleanUpSources( true,  false, false );
			break;
		case PARTFILE_REMOVE_FULL_QUEUE:
			((CPartFile *)event.ptr_value)->CleanUpSources( false, true,  false );
			break;
		case PARTFILE_REMOVE_HIGH_QUEUE:
			((CPartFile *)event.ptr_value)->CleanUpSources( false, false, true  );
			break;
		case PARTFILE_CLEANUP_SOURCES:
			((CPartFile *)event.ptr_value)->CleanUpSources( true,  true,  true  );
			break;
		case PARTFILE_SWAP_A4AF_THIS: {
				CPartFile *file = (CPartFile *)event.ptr_value;
				if ((file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)) {
					CPartFile::SourceSet::iterator it = file->A4AFsrclist.begin();
					for ( ; it != file->A4AFsrclist.end(); ) {
						CUpDownClient *cur_source = *it++;

						cur_source->SwapToAnotherFile(true, false, false, file);
					}
				}
			}
			break;
		case PARTFILE_SWAP_A4AF_OTHERS: {
				CPartFile *file = (CPartFile *)event.ptr_value;
				if ((file->GetStatus(false) == PS_READY) || (file->GetStatus(false) == PS_EMPTY)) {
					CPartFile::SourceSet::iterator it = file->m_SrcList.begin();
					for( ; it != file->m_SrcList.end(); ) {
						CUpDownClient* cur_source = *it++;

						cur_source->SwapToAnotherFile(false, false, false, NULL);
					}
				}
			}
			break;
		case PARTFILE_SWAP_A4AF_THIS_AUTO:
			((CPartFile *)event.ptr_value)->SetA4AFAuto(!((CPartFile *)event.ptr_value)->IsA4AFAuto());
			break;
		case PARTFILE_PAUSE:
			((CPartFile *)event.ptr_value)->PauseFile();
			break;
		case PARTFILE_RESUME:
			((CPartFile *)event.ptr_value)->ResumeFile();
			((CPartFile *)event.ptr_value)->SavePartFile();
			break;
		case PARTFILE_STOP:
			((CPartFile *)event.ptr_value)->StopFile();
			break;
		case PARTFILE_PRIO_AUTO:
			((CPartFile *)event.ptr_value)->SetAutoDownPriority(event.long_value);
			break;
		case PARTFILE_PRIO_SET:
			((CPartFile *)event.ptr_value)->SetDownPriority(event.long_value,
					event.longlong_value);
			break;
		case PARTFILE_SET_CAT:
			((CPartFile *)event.ptr_value)->SetCategory(event.byte_value);
			break;
		case PARTFILE_DELETE:
			((CPartFile *)event.ptr_value)->Delete();
			break;
		case KNOWNFILE_SET_UP_PRIO:
			((CKnownFile *)event.ptr_value)->SetAutoUpPriority(false);
			((CKnownFile *)event.ptr_value)->SetUpPriority(event.byte_value);
			break;
		case KNOWNFILE_SET_UP_PRIO_AUTO:
			((CKnownFile *)event.ptr_value)->SetAutoUpPriority(true);
			((CKnownFile *)event.ptr_value)->UpdateAutoUpPriority();
			break;
		case KNOWNFILE_SET_COMMENT:
			((CKnownFile *)event.ptr_value)->SetFileComment(event.string_value);
			break;
		
			
		// download queue
		case DLOAD_SET_CAT_PRIO:
			downloadqueue->SetCatPrio(event.long_value, event.short_value);
			break;
		case DLOAD_SET_CAT_STATUS:
			downloadqueue->SetCatStatus(event.long_value, event.short_value);
			break;

			// CORE->GUI
			// queue list
		case QLIST_CTRL_ADD_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->InsertClient((CUpDownClient*)event.ptr_value, vtQueued);
			}
			break;
		case QLIST_CTRL_RM_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->RemoveClient((CUpDownClient*)event.ptr_value, vtQueued);
			}
			break;
		case QLIST_CTRL_REFRESH_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->UpdateClient((CUpDownClient*)event.ptr_value, vtQueued);
			}
			break;

		
		// shared files
		case SHAREDFILES_UPDATE_ITEM:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->UpdateItem((CKnownFile*)event.ptr_value);
			}
			break;
		case SHAREDFILES_SHOW_ITEM:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->ShowFile((CKnownFile*)event.ptr_value);
			}
			break;

		case SHAREDFILES_REMOVE_ITEM:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->RemoveFile((CKnownFile*)event.ptr_value);
			}
			break;
		case SHAREDFILES_REMOVE_ALL_ITEMS:
			if ( amuledlg->sharedfileswnd ) {
				amuledlg->sharedfileswnd->RemoveAllSharedFiles();
			}
			break;
		case SHAREDFILES_SORT:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->SortList();
			}
			break;
		case SHAREDFILES_SHOW_ITEM_LIST:
			if ( amuledlg->sharedfileswnd && amuledlg->sharedfileswnd->sharedfilesctrl ) {
				amuledlg->sharedfileswnd->sharedfilesctrl->ShowFileList((CSharedFileList*)event.ptr_value);
			}
			break;

		// download ctrl
		case DOWNLOAD_CTRL_UPDATEITEM:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->UpdateItem((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_ADD_FILE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->AddFile((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_ADD_SOURCE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				CPartFile* file = (CPartFile*)event.ptr_value;

				if ( file->ShowSources() ) {
					amuledlg->transferwnd->downloadlistctrl->AddSource( file,
							(CUpDownClient*)event.ptr_aux_value,
							(DownloadItemType)event.byte_value);
				}
			}
			break;
		case DOWNLOAD_CTRL_RM_FILE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->RemoveFile((CPartFile*)event.ptr_value);
			}
			break;
		case DOWNLOAD_CTRL_RM_SOURCE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				CPartFile* file = (CPartFile*)event.ptr_aux_value;

				if ( !file || file->ShowSources() ) {
					amuledlg->transferwnd->downloadlistctrl->RemoveSource((CUpDownClient*)event.ptr_value,
							file );
				}
			}
			break;

		case DOWNLOAD_CTRL_HIDE_SOURCE:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->ShowSources((CPartFile*)event.ptr_value, false);
			}
			break;
		case DOWNLOAD_CTRL_SORT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->downloadlistctrl ) {
				amuledlg->transferwnd->downloadlistctrl->SortList();
			}
			break;

		
		// upload ctrl
		case UPLOAD_CTRL_ADD_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->InsertClient((CUpDownClient*)event.ptr_value, vtUploading);
			}
			break;
		case UPLOAD_CTRL_REFRESH_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->UpdateClient((CUpDownClient*)event.ptr_value, vtUploading);
			}
			break;
		case UPLOAD_CTRL_RM_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->RemoveClient((CUpDownClient*)event.ptr_value, vtUploading);
			}
			break;
			// client ctrl
		case CLIENT_CTRL_ADD_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->InsertClient((CUpDownClient*)event.ptr_value, vtClients);
			}
			break;
		case CLIENT_CTRL_REFRESH_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->UpdateClient((CUpDownClient*)event.ptr_value, vtClients);
			}
			break;
		case CLIENT_CTRL_RM_CLIENT:
			if ( amuledlg->transferwnd && amuledlg->transferwnd->clientlistctrl ) {
				amuledlg->transferwnd->clientlistctrl->RemoveClient((CUpDownClient*)event.ptr_value, vtClients);
			}
			break;
		
			
		// server
		case SERVER_ADD:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->AddServer((CServer*)event.ptr_value);
			}
			break;
		case SERVER_RM:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->RemoveServer((CServer*)event.ptr_value);
			}
			break;
		case SERVER_RM_DEAD:
			if ( serverlist ) {
				serverlist->RemoveDeadServers();
			}
			break;
		case SERVER_RM_ALL:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->DeleteAllItems();
			}
			break;
		case SERVER_HIGHLIGHT:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->HighlightServer((CServer*)event.ptr_value,
						event.byte_value);
			}
			break;
		case SERVER_REFRESH:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->RefreshServer((CServer*)event.ptr_value);
			}
			break;
		case SERVER_FREEZE:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->Freeze();
			}
			break;
		case SERVER_THAW:
			if ( amuledlg->serverwnd && amuledlg->serverwnd->serverlistctrl ) {
				amuledlg->serverwnd->serverlistctrl->Thaw();
			}
			break;
		case SERVER_UPDATEED2KINFO:
			if ( amuledlg->serverwnd ) {
				amuledlg->serverwnd->UpdateED2KInfo();
			}
			break;
		case SERVER_UPDATEKADINFO:
			if ( amuledlg->serverwnd ) {
				amuledlg->serverwnd->UpdateKadInfo();
			}
			break;
		
		// notification
		case SHOW_NOTIFIER:
			amuledlg->ShowNotifier(event.string_value,event.long_value,event.byte_value);
			break;
		case SHOW_CONN_STATE:
			#ifdef CLIENT_GUI
				theApp.m_KadConnected = (event.long_value & CONNECTED_KAD_OK) || (event.long_value & CONNECTED_KAD_FIREWALLED);
			#endif
			amuledlg->ShowConnectionState(event.long_value);
			break;
		case SHOW_QUEUE_COUNT:
			if ( amuledlg->transferwnd ) {
				amuledlg->transferwnd->ShowQueueCount(event.long_value);
			}
			break;
		case SHOW_UPDATE_CAT_TABS:
			if ( amuledlg->transferwnd ) {
				amuledlg->transferwnd->UpdateCatTabTitles();
			}
			break;
		case SHOW_USER_COUNT:
			amuledlg->ShowUserCount(event.string_value);
			break;
		case SHOW_GUI:
			amuledlg->Show_aMule(true);
			break;

		
		// search window
		case SEARCH_CANCEL:
			if ( amuledlg->searchwnd ) {
				amuledlg->searchwnd->ResetControls();
			}
			break;
		case SEARCH_LOCAL_END:
			if ( amuledlg->searchwnd ) {
				amuledlg->searchwnd->LocalSearchEnd();
			}
			break;
		case SEARCH_UPDATE_PROGRESS:
			if ( amuledlg->searchwnd ) {
				switch (event.long_value) {
					case 0xffff:
						// Global search ended
						amuledlg->searchwnd->ResetControls();
						break;
					default:
						amuledlg->searchwnd->UpdateProgress(event.long_value);
				}
			}
			break;
		case SEARCH_UPDATE_SOURCES:
			amuledlg->searchwnd->UpdateResult( (CSearchFile *)event.ptr_value );
			break;
		case SEARCH_ADD_RESULT:
			amuledlg->searchwnd->AddResult( (CSearchFile *)event.ptr_value );
			break;

			
		// chat window
		case CHAT_REFRESH_FRIEND:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->RefreshFriend(CMD4Hash(), event.string_value, event.long_value, event.short_value);
			}
			break;
		case CHAT_CONN_RESULT:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->ConnectionResult(event.byte_value, event.string_value, event.longlong_value);
			}
			break;
		case CHAT_PROCESS_MSG:
			if ( amuledlg->chatwnd ) {
				amuledlg->chatwnd->ProcessMessage(event.longlong_value, event.string_value);
			}
			break;
		case CATEGORY_ADD:
			if ( amuledlg->transferwnd ) {
				uint32 cat = event.long_value;
				
		        amuledlg->transferwnd->AddCategory(glob_prefs->GetCategory(cat));
			}
			break;
		case CATEGORY_UPDATE:
			if ( amuledlg->transferwnd ) {
				uint32 cat = event.long_value;
				
				amuledlg->transferwnd->UpdateCategory(cat);
				amuledlg->transferwnd->downloadlistctrl->Refresh();
				amuledlg->searchwnd->UpdateCatChoice();
			}
			break;
		case CATEGORY_DELETE:
			if ( amuledlg->transferwnd ) {
				uint32 cat = event.long_value;
				
				amuledlg->transferwnd->RemoveCategory(cat);
				
				amuledlg->searchwnd->UpdateCatChoice();
			}
			break;

		// logging
		case ADDDEBUGLOGLINE:
		case ADDLOGLINE:
			if (amuledlg) {
				while ( !m_logLines.empty() ) {
					QueuedLogLine entry = m_logLines.front();
					amuledlg->AddLogLine( entry.show, entry.line );
					m_logLines.pop_front();
				}
				
				amuledlg->AddLogLine(event.byte_value,event.string_value);
			} else {
				QueuedLogLine entry = { event.string_value, event.byte_value };
				m_logLines.push_back( entry );
			}
					
			CamuleApp::AddLogLine( event.string_value );
			
			break;
		default:
			printf("Unknown event notified to wxApp\n");
			wxASSERT(0);
	}
}


wxString CamuleGuiApp::GetLog(bool reset)
{
	if ( reset ) {
		amuledlg->ResetLog(ID_LOGVIEW);
	}
	return CamuleApp::GetLog(reset);
}


wxString CamuleGuiApp::GetServerLog(bool reset)
{
	if ( reset ) {
		amuledlg->ResetLog(ID_SERVERINFO);
	}
	return CamuleApp::GetServerLog(reset);
}


void CamuleGuiApp::AddServerMessageLine(wxString &msg)
{
	amuledlg->AddServerMessageLine(msg);
	CamuleApp::AddServerMessageLine(msg);
}

#endif /* CLIENT_GUI */
