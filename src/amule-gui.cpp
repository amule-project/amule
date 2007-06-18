//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "amule.h"				// Interface declarations.

#include <include/common/EventIDs.h>

#ifdef HAVE_CONFIG_H
	#include "config.h"			// Needed for VERSION
#endif

#include <wx/clipbrd.h>			// Needed for wxClipBoard
#include <wx/tokenzr.h>			// Needed for wxStringTokenizer

#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "Timer.h"				// Needed for CTimer
#include "PartFile.h"			// Needed for CPartFile
#include "updownclient.h"		// Needed for CUpDownClient

#include "muuli_wdr.h"			// Needed for IDs
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "PartFileConvert.h"
#include "ThreadTasks.h"
#include "Logger.h"				// Needed for EVT_MULE_LOGGING
#include "GuiEvents.h"			// Needed for EVT_MULE_NOTIFY

#ifdef __WXMAC__
	#include <CoreFoundation/CFBundle.h>  // Do_not_auto_remove
	#include <ApplicationServices/ApplicationServices.h>	// For LSRegisterURL // Do_not_auto_remove
#endif

#ifndef CLIENT_GUI
#include "InternalEvents.h"		// Needed for wxEVT_*

BEGIN_EVENT_TABLE(CamuleGuiApp, wxApp)

	// Socket handlers
	// Listen Socket
	EVT_SOCKET(ID_LISTENSOCKET_EVENT, CamuleGuiApp::ListenSocketHandler)

	// UDP Socket (servers)
	EVT_SOCKET(ID_SERVERUDPSOCKET_EVENT, CamuleGuiApp::UDPSocketHandler)
	// UDP Socket (clients)
	EVT_SOCKET(ID_CLIENTUDPSOCKET_EVENT, CamuleGuiApp::UDPSocketHandler)

	// Socket timers (TCP + UDP)
	EVT_MULE_TIMER(ID_SERVER_RETRY_TIMER_EVENT, CamuleGuiApp::OnTCPTimer)

	// Core timer
	EVT_MULE_TIMER(ID_CORE_TIMER_EVENT, CamuleGuiApp::OnCoreTimer)

	EVT_MULE_NOTIFY(CamuleGuiApp::OnNotifyEvent)
	EVT_MULE_LOGGING(CamuleGuiApp::OnLoggingEvent)
	
	// Async dns handling
	EVT_MULE_INTERNAL(wxEVT_CORE_UDP_DNS_DONE, -1, CamuleGuiApp::OnUDPDnsDone)

	EVT_MULE_INTERNAL(wxEVT_CORE_SOURCE_DNS_DONE, -1, CamuleGuiApp::OnSourceDnsDone)

	EVT_MULE_INTERNAL(wxEVT_CORE_SERVER_DNS_DONE, -1, CamuleGuiApp::OnServerDnsDone)

	// Hash ended notifier
	EVT_MULE_HASHING(CamuleGuiApp::OnFinishedHashing)
	EVT_MULE_AICH_HASHING(CamuleGuiApp::OnFinishedAICHHashing)

	// File completion ended notifier
	EVT_MULE_FILE_COMPLETED(CamuleGuiApp::OnFinishedCompletion)

	// HTTPDownload finished
	EVT_MULE_INTERNAL(wxEVT_CORE_FINISHED_HTTP_DOWNLOAD, -1, CamuleGuiApp::OnFinishedHTTPDownload)
END_EVENT_TABLE()


IMPLEMENT_APP(CamuleGuiApp)

#endif // CLIENT_GUI

CamuleGuiBase::CamuleGuiBase()
{
	amuledlg = NULL;
}


CamuleGuiBase::~CamuleGuiBase()
{
	#ifndef CLIENT_GUI
	CPartFileConvert::StopThread();
	#endif
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
		// I plan on moving this to a separate function, as it just clutters up OnInit()
		/*
		This implementation might work with mac, provided that the
		SetSize() function works as expected.
		*/

		// Remove possible prefix
		if ( geom_string.GetChar(0) == '=' ) {
			geom_string.Remove( 0, 1 );
		}

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
	delete core_timer;
	
	return CamuleApp::OnExit();	
}


void CamuleGuiApp::ShutDown(wxCloseEvent &WXUNUSED(evt))
{		
	amuledlg->DlgShutDown();
	amuledlg->Destroy();
	CamuleApp::ShutDown();
}


bool CamuleGuiApp::OnInit()
{
	amuledlg = NULL;

	if ( !CamuleApp::OnInit() ) {
		return false;
	}

	// Create the Core timer
	core_timer = new CTimer(this,ID_CORE_TIMER_EVENT);
	if (!core_timer) {
		printf("Fatal Error: Failed to create Core Timer");
		OnExit();
	}

	// Start the Core and Gui timers

	// Note: wxTimer can be off by more than 10% !!!
	// In addition to the systematic error introduced by wxTimer, we are losing
	// timer cycles due to high CPU load.  I've observed about 0.5% random loss of cycles under
	// low load, and more than 6% lost cycles with heavy download traffic and/or other tasks
	// in the system, such as a video player or a VMware virtual machine.
	// The upload queue process loop has now been rewritten to compensate for timer errors.
	// When adding functionality, assume that the timer is only approximately correct;
	// for measurements, always use the system clock [::GetTickCount()].
	core_timer->Start(100);
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


void CamuleGuiApp::OnLoggingEvent(CLoggingEvent& evt)
{
	if (amuledlg) {
		while ( !m_logLines.empty() ) {
			QueuedLogLine entry = m_logLines.front();
			amuledlg->AddLogLine( entry.show, entry.line );
			m_logLines.pop_front();
		}
		
		amuledlg->AddLogLine(evt.IsCritical(), evt.Message());
	} else {
		QueuedLogLine entry = { evt.Message(), evt.IsCritical() };
		m_logLines.push_back( entry );
	}
			
	CamuleApp::AddLogLine( evt.Message() );
}

#endif /* CLIENT_GUI */
// File_checked_for_headers
