// This file is part of the aMule Project
//
// aMule Copyright (C) 2003 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
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

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/textctrl.h>	// Needed for wxTextCtrl

#include "ExternalConn.h"	// Interface declarations
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "otherfunctions.h"	// Needed for EncodeBase16
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "updownclient.h"	// Needed for CUpDownClient
#include "ED2KLink.h"		// Needed for CED2KLink
#include "server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for CServerList
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "ServerWnd.h"		// Needed for CServerWnd
#include "muuli_wdr.h"		// Needed for ID_SERVERINFO
#include "PartFile.h"		// Needed for CPartFile
#include "sockets.h"		// Needed for CServerConnect
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "amule.h"			// Needed for theApp
#include "SearchList.h"		// Needed for GetWebList

//ExternalConn: listening server using wxSockets
enum
{	// id for sockets
	SERVER_ID = 1000,
	AUTH_ID,
	SOCKET_ID
};

BEGIN_EVENT_TABLE(ExternalConn, wxEvtHandler)
  EVT_SOCKET(SERVER_ID, ExternalConn::OnServerEvent)
  EVT_SOCKET(AUTH_ID,   ExternalConn::OnAuthEvent)
  EVT_SOCKET(SOCKET_ID, ExternalConn::OnSocketEvent)
END_EVENT_TABLE()


ExternalConn::ExternalConn() {
	m_ECServer = NULL;
	//Citroklar, looking if we are allowed to accept External Connections
	if (theApp.glob_prefs->AcceptExternalConnections()) {
		//can we use TCP port?
		if (theApp.glob_prefs->ECUseTCPPort()) {
			
			// Create the address - listen on localhost:ECPort
			wxIPV4address addr;
			addr.Service(theApp.glob_prefs->ECPort());

			// Create the socket
			m_ECServer = new wxSocketServer(addr);
	
			// We use Ok() here to see if the server is really listening
			if (! m_ECServer->Ok()) {
				printf("Could not listen for external connections at port %d!\n\n", theApp.glob_prefs->ECPort());
				return;
			} else {
				printf("ECServer listening on port %d.\n\n", theApp.glob_prefs->ECPort());
			}

			// Setup the event handler and subscribe to connection events
			m_ECServer->SetEventHandler(*this, SERVER_ID);
			m_ECServer->SetNotify(wxSOCKET_CONNECTION_FLAG);
			m_ECServer->Notify(TRUE);

			m_numClients = 0; 
		} else {
			//FIXME: shakraw, should we use *nix sockets here?
		}
	} else {
		printf("External connections disabled in .eMule\n");
		return;
	}
}


ExternalConn::~ExternalConn() {
	if ( m_ECServer )
		m_ECServer->Destroy();
}


void ExternalConn::OnServerEvent(wxSocketEvent& event) {
	wxSocketBase *sock;

	// Accept new connection if there is one in the pending
	// connections queue, else exit. We use Accept(FALSE) for
	// non-blocking accept (although if we got here, there
	// should ALWAYS be a pending connection).
	sock = m_ECServer->Accept(FALSE);

	if (sock) {
		printf(_("New external connection accepted\n"));
	} else {
		printf(_("Error: couldn't accept a new external connection\n\n"));
    	return;
	}

	sock->SetEventHandler(*this, AUTH_ID);
	sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	sock->Notify(TRUE);

	m_numClients++;
}

void ExternalConn::OnAuthEvent(wxSocketEvent& event) {
	wxSocketBase *sock = event.GetSocket();
	
	switch (event.GetSocketEvent()) {
		case wxSOCKET_INPUT: {
			// We disable input events, so that the test doesn't trigger
			// wxSocketEvent again.
			sock->SetNotify(wxSOCKET_LOST_FLAG);

			// Which command are we going to run?
			wxChar *buf = new wxChar[1024];
			uint16 len;
			
			sock->Read(&len, sizeof(uint16));
			sock->ReadMsg(buf, len);
			wxString response = Authenticate(wxString(buf));
			delete[] buf;

			len=(wxString(response).Length() + 1) * sizeof(wxChar);
			sock->Write(&len, sizeof(uint16));
			sock->WriteMsg(response.c_str(), len);
  			
			if (response=="Authenticated") {
	      		//Authenticated => change socket handler
				sock->SetEventHandler(*this, SOCKET_ID);
				sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
				sock->Notify(TRUE);
			} else {
				//Access denied! => Destroy the socket.
				sock->Destroy();
				printf("Unauthorized access attempt. Connection closed.\n");
			}
			
      		break;
    	}
				
    	case wxSOCKET_LOST: {
			//just destroy the socket here.
      		sock->Destroy();
      		break;
		}
	
		default: ;
	}
}

void ExternalConn::OnSocketEvent(wxSocketEvent& event) {
	wxSocketBase *sock = event.GetSocket();

	// Now we process the event
	switch(event.GetSocketEvent()) {
	    case wxSOCKET_INPUT: {
			// We disable input events, so that the test doesn't trigger
			// wxSocketEvent again.
			sock->SetNotify(wxSOCKET_LOST_FLAG);

			// Which command are we going to run?
			wxChar *buf = new wxChar[1024];
  			uint16 len;
			
			sock->Read(&len, sizeof(uint16));
			sock->ReadMsg(buf, len);

			wxString response = ProcessRequest(wxString(buf));
			len=(wxString(response).Length() + 1) * sizeof(wxChar);
			sock->Write(&len, sizeof(uint16));
			sock->WriteMsg(response.c_str(), len);
  			delete[] buf;
			
      		// Re-Enable input events again.
      		sock->SetNotify(wxSOCKET_LOST_FLAG | wxSOCKET_INPUT_FLAG);
      		break;
    	}
		
    	case wxSOCKET_LOST: {
      		m_numClients--;
			// Destroy() should be used instead of delete wherever possible,
			// due to the fact that wxSocket uses 'delayed events' (see the
			// documentation for wxPostEvent) and we don't want an event to
			// arrive to the event handler (the frame, here) after the socket
			// has been deleted. Also, we might be doing some other thing with
			// the socket at the same time; for example, we might be in the
			// middle of a test or something. Destroy() takes care of all
			// this for us.
			printf("Connection closed.\n\n");
      		//sock->Destroy();
			sock->Close();
      		break;
    	}
    	default: ;
  	}
}

//Authentication
wxString ExternalConn::Authenticate(const wxString& item) {
	if (item.Left(4) == "AUTH") {
		if (item.Mid(5) == wxString::Format("aMuleweb %s", theApp.glob_prefs->ECPassword().GetData())) {
			printf("Accepted Connection from amuleweb\n"); 
			return("Authenticated"); 
		}
		if (item.Mid(5) == wxString::Format("aMulecmd %s", theApp.glob_prefs->ECPassword().GetData())) {
			printf("Accepted Connection from amulecmd\n"); 
			return("Authenticated");
		}
	}
	return("Access Denied");
}
	
//TODO: do a function for each command
wxString ExternalConn::ProcessRequest(const wxString& item) {
		//WEBPAGE
		if (item == "WEBPAGE HEADER") {
			//returns one string formatted as:
			//%d\t%s\t%s\t%d\t%d\t%f\t%f\t%d\t%d
			wxString buffer("");
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetWebPageRefresh());
			
			if (theApp.serverconnect->IsConnected())
				buffer+=wxString("Connected\t");
			else if (theApp.serverconnect->IsConnecting())
				buffer+=wxString("Connecting\t");
			else
				buffer+=wxString("Disconnected\t");
			
			if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting()) {
				if (theApp.serverconnect->IsLowID())
					buffer+=wxString("Low ID\t");
				else
					buffer+=wxString("High ID\t");
				
				if (theApp.serverconnect->IsConnected()) {
					buffer+=wxString::Format("%s\t", theApp.serverconnect->GetCurrentServer()->GetListName());
					buffer+=wxString::Format("%d\t", theApp.serverconnect->GetCurrentServer()->GetUsers());
				}
			}
			
			buffer+=wxString::Format("%.1f\t", theApp.uploadqueue->GetKBps());
			buffer+=wxString::Format("%.1f\t", theApp.downloadqueue->GetKBps());
			
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxUpload());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxDownload());
			
			return((wxChar*)buffer.GetData());
		}
		if (item == "WEBPAGE GETGRAPH") {
			//returns one string formatted as:
			//%d\t%d\t%d\t%d
			wxString buffer("");
			
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetTrafficOMeterInterval());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxGraphDownloadRate());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxGraphUploadRate());
			buffer+=wxString::Format("%d", theApp.glob_prefs->GetMaxConnections());
			
			return((wxChar*) buffer.GetData());
		}
		if (item == "WEBPAGE STATISTICS") {
			wxString buffer("");
			
			int filecount = theApp.downloadqueue->GetFileCount();
			int stats[2]; // get the source count
			theApp.downloadqueue->GetDownloadStats(stats);

			buffer+=theApp.amuledlg->statisticswnd->GetHTML()+wxString("\t");
			buffer+=wxString::Format("Statistics: \n Downloading files: %d\n Found sources: %d\n Active downloads: %d\n Active Uploads: %d\n Users on upload queue: %d"			, filecount, stats[0], stats[1], theApp.uploadqueue->GetUploadQueueLength(), theApp.uploadqueue->GetWaitingUserCount());
			
			return((wxChar*) buffer.GetData());
		}
		if (item == "WEBPAGE GETPREFERENCES") {
			//returns one string formatted as:
			//%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d
			wxString buffer("");

			theApp.glob_prefs->GetWebUseGzip() ? buffer+=wxString("1\t") : buffer+=wxString("0\t");
			theApp.glob_prefs->GetPreviewPrio() ? buffer+=wxString("1\t") : buffer+=wxString("0\t");
			theApp.glob_prefs->TransferFullChunks() ? buffer+=wxString("1\t") : buffer+=wxString("0\t");
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetWebPageRefresh());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxSourcePerFile());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxConnections());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxConperFive());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxDownload());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxUpload());
			buffer+=wxString::Format("%d\t", theApp.glob_prefs->GetMaxGraphDownloadRate());
			buffer+=wxString::Format("%d", theApp.glob_prefs->GetMaxGraphUploadRate());
			
			return((wxChar*) buffer.GetData());
		}
		if (item.Left(22) == "WEBPAGE SETPREFERENCES") {
			if (item.Length() > 23) {
				wxString prefList = item.Mid(23);
				int brk=prefList.First("\t");
				
				theApp.glob_prefs->SetWebUseGzip(atoi(prefList.Left(brk).GetData())==1);
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetWebPageRefresh(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetMaxDownload(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetMaxUpload(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				
				if ((int32)atoi(prefList.Left(brk).GetData()) != theApp.glob_prefs->GetMaxGraphDownloadRate()) {
					theApp.amuledlg->statisticswnd->SetARange(true, theApp.glob_prefs->GetMaxGraphDownloadRate());
				}
				theApp.glob_prefs->SetMaxGraphDownloadRate(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
			
				if ((int32)atoi(prefList.Left(brk).GetData()) != theApp.glob_prefs->GetMaxGraphUploadRate()) {
					theApp.amuledlg->statisticswnd->SetARange(false, theApp.glob_prefs->GetMaxGraphUploadRate());
				}
				theApp.glob_prefs->SetMaxGraphUploadRate(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				
				theApp.glob_prefs->SetMaxSourcesPerFile(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetMaxConnections(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetMaxConsPerFive(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1); brk=prefList.First("\t");
				theApp.glob_prefs->SetTransferFullChunks(atoi(prefList.Left(brk).GetData()));
				prefList=prefList.Mid(brk+1);
				theApp.glob_prefs->SetPreviewPrio(atoi(prefList.GetData())==1);
			}
			return("");
		}
		if (item.Left(19) == "WEBPAGE PROGRESSBAR") {
			wxString buffer("");
			
			if (item.Length() > 20) {
				int idx = item.Mid(20).Find(" ");
				uint16 progressbarWidth = atoi(item.Mid(20,idx).GetData());
				wxString filehash = item.Mid(20+idx+1);
				
				uchar fileid[16];
				DecodeBase16(filehash.GetData(),filehash.Length(),fileid);
				CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
				if (cur_file && cur_file->IsPartFile()) {
					buffer.Append("0\t");
					buffer+=cur_file->GetProgressString(progressbarWidth)+wxString("\t");
					buffer+=wxString::Format("%f\t", cur_file->GetPercentCompleted());
				} else {
					buffer.Append("1\t");
				}
			}
			
			return((wxChar*) buffer.GetData());
		}
		
		//PREFS
		if (item == "PREFS GETWEBUSEGZIP") {
			wxString buffer("");
			
			if (theApp.glob_prefs->GetWebUseGzip())
				buffer+=wxString("1\t");
			else
				buffer+=wxString("0\t");
			
			return((wxChar*)buffer.GetData());
		}
		
		if (item.Left(15) == "PREFS GETWSPASS") {
			wxString pwdHash = item.Mid(16);
			if (pwdHash == theApp.glob_prefs->GetWSPass()) {
				theApp.amuledlg->AddLogLine(false, "Webserver-Admin-Login");
				return("AdminLogin");
			} else if (theApp.glob_prefs->GetWSIsLowUserEnabled() && 
					theApp.glob_prefs->GetWSLowPass()!="" && 
					pwdHash == theApp.glob_prefs->GetWSLowPass()) {
					theApp.amuledlg->AddLogLine(false, "Webserver-Guest-Login");
				return("GuestLogin");
			} else {
				theApp.amuledlg->AddLogLine(false, "Webserver: Failed Loginattempt");
				return("Access Denied");
			}
		}
			
		//SERVER
		if (item == "SERVER RE-CONNECT") {
			if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting())
				theApp.serverconnect->Disconnect();
			
			theApp.serverconnect->ConnectToAnyServer();
			theApp.amuledlg->ShowConnectionState(false);
			
			return("");
		}
		if (item == "SERVER DISCONNECT") {
			if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting())
				theApp.serverconnect->Disconnect();
			
			return("");
		}
		if (item == "SERVER LIST") {
			// returns one string where each line is formatted 
			// as: %s\t%s\t%d\t%s\t%d\t%d\t%d\n
			wxString buffer("");
			wxString tempBuf("");
			for (uint i=0; i < theApp.serverlist->GetServerCount(); i++) {
				CServer *server = theApp.serverlist->GetServerAt(i);
				if (server) {
					tempBuf.Printf("%s\t%s\t%d\t%s\t%d\t%d\t%d\n",
						server->GetListName(),
						server->GetDescription(),
						server->GetPort(),
						server->GetAddress(),
						server->GetUsers(),
						server->GetMaxUsers(),
						server->GetFiles()
					);
					buffer.Append(tempBuf);
				}
			}
			return((wxChar *)buffer.GetData());
		}
		if (item.Left(10) == "SERVER ADD") {
			if (item.Length() > 10) {
				int idx1 = item.Mid(11).Find(" ");
				wxString sIP = item.Mid(11, idx1);
				int idx2= item.Mid(11+idx1+1).Find(" ");
				wxString sPort = item.Mid(11+idx1+1,idx2);
				wxString sName = item.Mid(11+idx1+idx2+2);
					
				CServer *nsrv = new CServer(atoi(sPort.GetData()), (char*)sIP.GetData());
				nsrv->SetListName((char*)sName.GetData());
				theApp.amuledlg->serverwnd->serverlistctrl->AddServer(nsrv, true);
			}
			return("");
		}
		if (item.Left(16) == "SERVER UPDATEMET") {
			if (item.Length() > 16) {
				theApp.amuledlg->serverwnd->UpdateServerMetFromURL(item.Mid(17));
			}
			return("");
		}
		if (item == "SERVER STAT") {
			// returns one string where each line is formatted 
			// as: %s or as: %s\t%s\t%s\t%ld
			wxString buffer("");
			
			if (theApp.serverconnect->IsConnected()) {
				buffer.Append("Connected\t");
				theApp.serverconnect->IsLowID() ? buffer.Append("Low ID\t") : buffer.Append("High ID\t");
				buffer+=wxString::Format("%s\t", theApp.serverconnect->GetCurrentServer()->GetListName());
				buffer+=wxString::Format("%ld", (long)theApp.serverconnect->GetCurrentServer()->GetUsers());
			} else if (theApp.serverconnect->IsConnecting())
				buffer.Append("Connecting\t");
			else
				buffer.Append("Disconnected\t");
			
			return((wxChar*) buffer.GetData());
		}
		
		//TRANSFER
		if (item == "TRANSFER CLEARCOMPLETE") {
				theApp.amuledlg->transferwnd->downloadlistctrl->ClearCompleted();
				return("");
		}
		if (item.Left(20) == "TRANSFER ADDFILELINK") {
			wxString buffer="Bad Link";
			if (item.Length() > 20) {
				CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(item.Mid(21));
				if (pLink->GetKind() == CED2KLink::kFile) {
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink()); 
					buffer="Link Added";
				}
			}
			return((wxChar*) buffer.GetData());
		}
		if (item == "TRANSFER DL_FILEHASH") {
			// returns one string where each line is formatted as:
			// %s\t%s\t...\t%s
			wxString buffer("");
			for (int i=0; i < theApp.downloadqueue->GetFileCount(); i++) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
				if (cur_file) {
					buffer.Append(cur_file->GetFileHash());
					if (i != theApp.downloadqueue->GetFileCount()-1)
						buffer.Append("\t");
				}
			}
			return((wxChar *)buffer.GetData());
		}
		if (item.Left(21) == "TRANSFER DL_FILEPAUSE") {
			if ((item.Length() > 21) && (item.Mid(22).IsNumber())) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(22).GetData()));
				if (cur_file) {
					cur_file->PauseFile();
				}
			}
			return("");
		}
		if (item.Left(21) == "TRANSFER DL_FILERESUME") {
			if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(23).GetData()));
				if (cur_file) {
					cur_file->ResumeFile();
				}
			}
			return("");
		}
		if (item.Left(21) == "TRANSFER DL_FILEDELETE") {
			if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(23).GetData()));
				if (cur_file) {
					cur_file->Delete();
				}
			}
			return("");
		}
		if (item.Left(22) == "TRANSFER DL_FILEPRIOUP") {
			if ((item.Length() > 23) && (item.Mid(24).IsNumber())) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(24).GetData()));
				if (cur_file) {
					if (!cur_file->IsAutoDownPriority()) {
						switch (cur_file->GetDownPriority()) {
							case PR_LOW:
								cur_file->SetAutoDownPriority(false);
								cur_file->SetDownPriority(PR_NORMAL);
								break;
							case PR_NORMAL: 
								cur_file->SetAutoDownPriority(false);
								cur_file->SetDownPriority(PR_HIGH);
								break;
							case PR_HIGH: 
								cur_file->SetAutoDownPriority(true);
								cur_file->SetDownPriority(PR_HIGH);
								break;
						}
					} else {
						cur_file->SetAutoDownPriority(false);
						cur_file->SetDownPriority(PR_LOW);
					}
				}
			}
			return("");
		}
		if (item.Left(24) == "TRANSFER DL_FILEPRIODOWN") {
			if ((item.Length() > 25) && (item.Mid(26).IsNumber())) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(26).GetData()));
				if (cur_file) {
					if (!cur_file->IsAutoDownPriority()) {
						switch (cur_file->GetDownPriority()) {
							case PR_LOW:
								cur_file->SetAutoDownPriority(true);
								cur_file->SetDownPriority(PR_HIGH);
								break;
							case PR_NORMAL:
								cur_file->SetAutoDownPriority(false);
								cur_file->SetDownPriority(PR_LOW);
								break;
							case PR_HIGH:
								cur_file->SetAutoDownPriority(false);
								cur_file->SetDownPriority(PR_NORMAL);
								break;
						}
					} else {
						cur_file->SetAutoDownPriority(false);
						cur_file->SetDownPriority(PR_HIGH);
					}
				}
			}
			return("");
		}
		if (item == "TRANSFER DL_LIST") {
			// returns one string where each line is formatted as:
			// %s\t%ld\t%ld\t%d\t%ld\t%d\t%s\t%d\t%s\t%ld\t%ld\t%ld\t%s\t%s\t%d\n
			wxString buffer("");
			wxString tempFileInfo("");
			//int tempPrio;
			for (int i=0; i < theApp.downloadqueue->GetFileCount(); i++) {
				CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
				if (cur_file) {
					buffer+=wxString::Format("%s\t", cur_file->GetFileName().c_str());
					buffer+=wxString::Format("%li\t", (long)cur_file->GetFileSize());
					buffer+=wxString::Format("%li\t", (long)cur_file->GetTransfered());
					buffer+=wxString::Format("%f\t", cur_file->GetPercentCompleted());
#ifdef DOWNLOADRATE_FILTERED
					buffer+=wxString::Format("%li\t", (long)(cur_file->GetKBpsDown()*1024));
#else
					buffer+=wxString::Format("%u\t", cur_file->GetDatarate());
#endif
					buffer+=wxString::Format("%d\t", cur_file->GetStatus());
					buffer+=wxString::Format("%s\t", cur_file->getPartfileStatus().c_str());
					if (cur_file->IsAutoDownPriority()) {
						buffer+=wxString::Format("%d\t", cur_file->GetDownPriority()+10);
					} else {
						buffer+=wxString::Format("%d\t", cur_file->GetDownPriority());
					}
					buffer+=EncodeBase16(cur_file->GetFileHash(), 16)+wxString("\t");
					buffer+=wxString::Format("%d\t", cur_file->GetSourceCount());
					buffer+=wxString::Format("%d\t", cur_file->GetNotCurrentSourcesCount());
					buffer+=wxString::Format("%d\t", cur_file->GetTransferingSrcCount());
					if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID()) {
						buffer+=theApp.CreateED2kSourceLink(cur_file)+wxString("\t");
					} else {
						buffer+=theApp.CreateED2kLink(cur_file)+wxString("\t");
					}
					tempFileInfo = cur_file->GetDownloadFileInfo();
					tempFileInfo.Replace("\n","|");
					buffer+=tempFileInfo+wxString("\t");
					if (!cur_file->IsPartFile()) {
						buffer+=wxString("1\n");
					} else {
						buffer+=wxString("0\n");
					}
				}
			}
			return((wxChar *)buffer.GetData());
		}
		if (item == "TRANSFER UL_LIST") {
			// returns one string where each line is formatted as:
			// %s\t%s\t%s\t%f\t%f\t%li\n"
			wxString buffer("");
			wxString tempFileInfo("");
			for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
				 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
				CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
				if (cur_client) {
					buffer+=wxString::Format("%s\t", cur_client->GetUserName());
					tempFileInfo=cur_client->GetUploadFileInfo();
					tempFileInfo.Replace("\n", "|");
					buffer+=wxString::Format("%s\t", tempFileInfo.GetData());
					CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
					if (file) {
						buffer+=wxString::Format("%s\t", file->GetFileName().GetData());
					} else {
						buffer+=wxString("?\t");
					}
					buffer+=wxString::Format("%li\t", (long)cur_client->GetTransferedDown());
					buffer+=wxString::Format("%li\t", (long)cur_client->GetTransferedUp());
					buffer+=wxString::Format("%li\n", (long)(cur_client->GetKBpsUp()*1024.0));
				}
			}
			return((wxChar*)buffer.GetData());
		}
		if (item == "TRANSFER W_LIST") {
			// returns one string where each line is formatted as:
			// %s\n 
			// or as:
			// %s\t%s\t%d\t%d\n"
			wxString buffer("");
			for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList();
				 pos != 0;theApp.uploadqueue->GetNextFromWaitingList(pos)) {
				CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
				if (cur_client) {
					buffer.Append(cur_client->GetUserName());
					if (cur_client->reqfile) {
						buffer.Append("\t");
					} else {
						buffer.Append("\n");
						continue;
					}
					
					CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->reqfile->GetFileHash());
					if (file) {
						buffer.Append(wxString::Format("%s\t", cur_client->GetDownloadFile()->GetFileName().GetData()));
					} else {
						buffer.Append("?\t");
					}
					
					buffer+=wxString::Format("%d\t", cur_client->GetScore(false));
					if (cur_client->IsBanned())
						buffer.Append("1\n");
					else
						buffer.Append("0\n");
				}
			}
			return((wxChar*)buffer.GetData());
		}

		//SHAREDFILES
		if (item == "SHAREDFILES RELOAD") {
			theApp.sharedfiles->Reload(true, false);
			return("");
		}
		if (item == "SHAREDFILES LIST") {
			// returns one string where each line is formatted as:
			// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
			wxString buffer("");
			for (int i=0; i< theApp.sharedfiles->GetCount(); i++) {
				CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(i);
				if (cur_file) {
					buffer+=wxString::Format("%s\t", cur_file->GetFileName().c_str());
					buffer+=wxString::Format("%ld\t", (long)cur_file->GetFileSize());
					if (theApp.serverconnect->IsConnected()) {
						buffer+=theApp.CreateED2kSourceLink(cur_file)+wxString("\t");
					} else {
						buffer+=theApp.CreateED2kLink(cur_file)+wxString("\t");
					}
					buffer+=wxString::Format("%ld\t", (long)cur_file->statistic.GetTransfered());
					buffer+=wxString::Format("%ld\t", (long)cur_file->statistic.GetAllTimeTransfered());
					buffer+=wxString::Format("%d\t", cur_file->statistic.GetRequests());
					buffer+=wxString::Format("%d\t", cur_file->statistic.GetAllTimeRequests());
					buffer+=wxString::Format("%d\t", cur_file->statistic.GetAccepts());
					buffer+=wxString::Format("%d\t", cur_file->statistic.GetAllTimeAccepts());
					buffer+=EncodeBase16(cur_file->GetFileHash(), 16)+wxString("\t");
					
					int prio = cur_file->GetUpPriority();
					if (cur_file->IsAutoUpPriority()) {
						switch (prio) {
							case PR_LOW: 
								buffer.Append("Auto [Lo]"); break;
							case PR_NORMAL:
								buffer.Append("Auto [No]"); break;
							case PR_HIGH:
								buffer.Append("Auto [Hi]"); break;
							case PR_VERYHIGH:
								buffer.Append("Auto [Re]"); break;
							default:
								buffer.Append("-"); break;
						}
					} else {
						switch (prio) {
							case PR_VERYLOW:
								buffer.Append("Very Low"); break;
							case PR_LOW:
								buffer.Append("Low"); break;
							case PR_NORMAL:
								buffer.Append("Normal"); break;
							case PR_HIGH:
								buffer.Append("High"); break;
							case PR_VERYHIGH:
								buffer.Append("Very High"); break;
							case PR_POWERSHARE:
								buffer.Append("PowerShare[Release]"); break;
							default:
								buffer.Append("-"); break;
						}
					}
					buffer.Append("\t");
					buffer+=wxString::Format("%d\t", prio);
					if (cur_file->IsAutoUpPriority()) {
						buffer+=wxString("1\n");
					} else {
						buffer+=wxString("0\n");
					}
				}
			}
			return((wxChar*) buffer.GetData());
		}
		
		//LOG
		if (item == "LOG RESETLOG") {
			theApp.amuledlg->ResetLog();
			return("");
		}
		if (item == "LOG GETALLLOGENTRIES") {
			wxTextCtrl* logview =(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_LOGVIEW);
			return((char*)logview->GetValue().GetData());
		}
		if (item == "LOG CLEARSERVERINFO") {
			((wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_SERVERINFO))->Clear();
			return("");
		}
		if (item == "LOG GETSERVERINFO") {
			wxTextCtrl* cv=(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_SERVERINFO);
			return((wxChar*) cv->GetValue().GetData());
		}
		if (item == "LOG RESETDEBUGLOG") {
			theApp.amuledlg->ResetDebugLog();
			return("");
		}
		if (item == "LOG GETALLDEBUGLOGENTRIES") {
			wxTextCtrl* logview =(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_LOGVIEW);
			return((char*)logview->GetValue().GetData());
		}
		if (item.Left(14) == "LOG ADDLOGLINE") {
			if (item.Length() > 15) {
				theApp.amuledlg->AddLogLine(false,item.Mid(15));
			}
			return("");
		}
		
		//SEARCH
		if (item == "SEARCH GETCATCOUNT") {
			wxString buffer("");
			buffer=wxString::Format("%d", theApp.glob_prefs->GetCatCount());
			return((wxChar*) buffer.GetData());
		}
		if (item.Left(19) == "SEARCH DOWNLOADFILE") {
			if (item.Length() > 19) {
				uchar fileid[16];
				DecodeBase16(item.Mid(20).GetData(),item.Mid(20).Length(),fileid);
				theApp.searchlist->AddFileToDownloadByHash(fileid);
			}
			return("");
		}
		if (item.Left(18) == "SEARCH DONEWSEARCH") {
			if (item.Length() > 18) {
				//int curIndex, nextIndex;
				wxString sParams = item.Mid(19);
				
				int brk = sParams.First("\n");
				((wxTextCtrl*)wxWindow::FindWindowById(IDC_SEARCHNAME))->SetValue(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First("\n");
				((wxChoice*)wxWindow::FindWindowById(IDC_TypeSearch))->SetStringSelection(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First("\n");				
				((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHMIN))->SetValue(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First("\n");				
				((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHMAX))->SetValue(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First("\n");				
				((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHAVAIBILITY))->SetValue(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First("\n");				
				((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHEXTENSION))->SetValue(sParams.Left(brk));
				sParams=sParams.Mid(brk+1);
				if (sParams == "global") //Method
					((wxCheckBox*)wxWindow::FindWindowByName("globalSearch"))->SetValue(true);
				else
					((wxCheckBox*)wxWindow::FindWindowByName("globalSearch"))->SetValue(false);
				
				theApp.amuledlg->searchwnd->DeleteAllSearchs();
				
				wxCommandEvent evt;
				theApp.amuledlg->searchwnd->OnBnClickedStarts(evt); //do a new search
				return("");
			}
		}
		if (item.Left(14) == "SEARCH WEBLIST") {
			if (item.Length() > 14) {
				wxString sItem = item.Mid(15);
				int brk=sItem.First("\t");
				wxString sResultLine=sItem.Left(brk);
				sItem=sItem.Mid(brk+1); brk=sItem.First("\t");
				int sortBy = atoi(sItem.Left(brk).GetData());
				bool searchAsc = (atoi(sItem.Mid(brk+1).GetData()) == 0) ? false : true;
				return(theApp.searchlist->GetWebList((CString)sResultLine, sortBy, searchAsc));
			}
		}
		
		//Special command :)
		if (item == "AMULEDLG SHOW") {
			theApp.amuledlg->Show_aMule(true);
			return("");
		}
		
				
		//shakraw - TODO - amulecmd requests
		
		
		//
		//
		//shakraw - old requests code (to remove when all new code is done)
		//
		//
		if (item == "STATS") {
			int filecount = theApp.downloadqueue->GetFileCount();
			// get the source count
			int stats[2];
			theApp.downloadqueue->GetDownloadStats(stats);
			static char buffer[1024];
			sprintf(buffer, "Statistics: \n Downloading files: %d\n Found sources: %d\n Active downloads: %d\n Active Uploads: %d\n Users on upload queue: %d", filecount, stats[0], stats[1], theApp.uploadqueue->GetUploadQueueLength(), theApp.uploadqueue->GetWaitingUserCount());
			return buffer;
		}

		if (item == "DL_QUEUE") {
			return (char *) theApp.amuledlg->transferwnd->downloadlistctrl->getTextList().GetData();
		}

		if (item == "UL_QUEUE") {
			static char buffer[1024];
			sprintf(buffer, "We should be showing UL list here");
			return buffer;
		}
		
		if (item == "CONNSTAT") {
					if (theApp.serverconnect->IsConnected()) {
						return("Connected");
					//Start - Added by shakraw
					} else if (theApp.serverconnect->IsConnecting()) {
						return("Connecting");
					//End
					} else {
						return("Not Connected");
					}
		}

		if (item == "RECONN") { //shakraw, should be replaced by SERVER CONNECT [ip] below?
					if (theApp.serverconnect->IsConnected()) {
						return("Already Connected");
					} else {
						theApp.serverconnect->ConnectToAnyServer();
						theApp.amuledlg->ShowConnectionState(false);
						return("Reconected");
					}
		}

		if (item == "DISCONN") {
					if (theApp.serverconnect->IsConnected()) {
						theApp.serverconnect->Disconnect();
						theApp.OnlineSig(); // Added By Bouc7
						return("Disconnected");
					} else {
						return("Already conected");
					}
		}
		
		
		if (item.Left(5).Cmp( "PAUSE")==0) { 
			if (item.Mid(5).IsNumber()) {
				int fileID=theApp.downloadqueue->GetFileCount() - atoi(item.Mid(5).GetData());
				if ((fileID >= 0) &&  (fileID < theApp.downloadqueue->GetFileCount())) {
					if (theApp.downloadqueue->GetFileByIndex(fileID)->IsPartFile()) {
							theApp.downloadqueue->GetFileByIndex(fileID)->PauseFile();
							printf("Paused\n");
							return((char *) theApp.amuledlg->transferwnd->downloadlistctrl->getTextList().GetData());
					} else return("Not part file");
				} else return("Out of range");
			} else return("Not a number");
		} 
		
		if (item.Left(6).Cmp( "RESUME")==0) { 
			if (item.Mid(6).IsNumber()) {
				int fileID2 =theApp.downloadqueue->GetFileCount() - atoi(item.Mid(6).GetData());
				if ((fileID2 >= 0) && (fileID2 < theApp.downloadqueue->GetFileCount())) {
					if (theApp.downloadqueue->GetFileByIndex(fileID2)->IsPartFile()) {
							theApp.downloadqueue->GetFileByIndex(fileID2)->ResumeFile();
							theApp.downloadqueue->GetFileByIndex(fileID2)->SavePartFile();
							printf("Resumed\n");
							return((char *) theApp.amuledlg->transferwnd->downloadlistctrl->getTextList().GetData());
					} else return("Not part file");
				} else return("Out of range");				
			} else return("Not a number");
		} 
		
		//shakraw, amuleweb protocol communication start
		// PREFERENCES
		if (item.Left(11).Cmp("PREFERENCES") == 0) {
			if (item.Mid(12).Cmp("GETWSPORT") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetWSPort());
				return(buffer);
			}
			
			if (item.Mid(12,9).Cmp("GETWSPASS") == 0) {
				wxString pwdHash = item.Mid(22);
				if (pwdHash == theApp.glob_prefs->GetWSPass())
					return("Admin Session");
				else if (theApp.glob_prefs->GetWSIsLowUserEnabled() && 
						theApp.glob_prefs->GetWSLowPass()!="" && 
						pwdHash == theApp.glob_prefs->GetWSLowPass())
					return("LowUser Session");
				else
					return("Access Denied");
			}

			if (item.Mid(12,17).Cmp("SETWEBPAGEREFRESH") == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					theApp.glob_prefs->SetWebPageRefresh(atoi(item.Mid(30).GetData()));
					return("WebPageRefresh Saved");
				}
				return("Bad SETWEBPAGEREFRESH request");
			}

			if (item.Mid(12).Cmp("GETWEBPAGEREFRESH") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetWebPageRefresh());
				return(buffer);
			}
						
			if (item.Mid(12,13).Cmp("SETWEBUSEGZIP") == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					theApp.glob_prefs->SetWebUseGzip(atoi(item.Mid(26).GetData()));
					return("WebUseGzip Saved");
				}
				return("Bad SETWEBUSEGZIP request");
			}

			if (item.Mid(12).Cmp("GETWEBUSEGZIP") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetWebUseGzip());
				return(buffer);
			}
			
			if (item.Mid(12,14).Cmp("SETMAXDOWNLOAD") == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					theApp.glob_prefs->SetMaxDownload(atoi(item.Mid(27).GetData()));
					return("MaxDownload Saved");
				}
				return("Bad SETMAXDOWNLOAD request");
			}

			if (item.Mid(12).Cmp("GETMAXDOWNLOAD") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxDownload());
				return(buffer);
			}

			if (item.Mid(12,12).Cmp("SETMAXUPLOAD") == 0) {
				if ((item.Length() > 24) && item.Mid(25).IsNumber()) {
					theApp.glob_prefs->SetMaxUpload(atoi(item.Mid(25).GetData()));
					return("MaxUpload Saved");
				}
				return("Bad SETMAXUPLOAD request");
			}

			if (item.Mid(12).Cmp("GETMAXUPLOAD") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxUpload());
				return(buffer);
			}
			
			if (item.Mid(12,23).Cmp("SETMAXGRAPHDOWNLOADRATE") == 0) {
				if ((item.Length() > 35) && item.Mid(36).IsNumber()) {
					theApp.glob_prefs->SetMaxGraphDownloadRate(atoi(item.Mid(36).GetData()));
					return("MaxGraphDownload Saved");
				}
				return("Bad SETMAXGRAPHDOWNLOADRATE request");
			}

			if (item.Mid(12).Cmp("GETMAXGRAPHDOWNLOADRATE") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxGraphDownloadRate());
				return(buffer);
			}


			if (item.Mid(12,21).Cmp("SETMAXGRAPHUPLOADRATE") == 0) {
				if ((item.Length() > 33) && item.Mid(34).IsNumber()) {
					theApp.glob_prefs->SetMaxGraphUploadRate(atoi(item.Mid(34).GetData()));
					return("MaxGraphUpload Saved");
				}
				return("Bad SETMAXGRAPHUPLOADRATE request");
			}

			if (item.Mid(12).Cmp("GETMAXGRAPHUPLOADRATE") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxGraphUploadRate());
				return(buffer);
			}

			if (item.Mid(12,20).Cmp("SETMAXSOURCESPERFILE") == 0) {
				if ((item.Length() > 32) && item.Mid(33).IsNumber()) {
					theApp.glob_prefs->SetMaxSourcesPerFile(atoi(item.Mid(33).GetData()));
					return("MaxSourcesPerFile Saved");
				}
				return("Bad SETMAXSOURCESPERFILE request");
			}

			if (item.Mid(12).Cmp("GETMAXSOURCEPERFILE") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxSourcePerFile());
				return(buffer);
			}

			if (item.Mid(12,17).Cmp("SETMAXCONNECTIONS") == 0) {
				if ((item.Length() > 29) && item.Mid(29).IsNumber()) {
					theApp.glob_prefs->SetMaxConnections(atoi(item.Mid(30).GetData()));
					return("MaxConnections Saved");
				}
				return("Bad SETMAXCONNECTIONS request");
			}

			if (item.Mid(12).Cmp("GETMAXCONNECTIONS") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxConnections());
				return(buffer);
			}

			if (item.Mid(12,17).Cmp("SETMAXCONSPERFIVE") == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					theApp.glob_prefs->SetMaxConsPerFive(atoi(item.Mid(30).GetData()));
					return("MaxConsPerFive Saved");
				}
				return("Bad SETMAXCONSPERFIVE request");
			}

			if (item.Mid(12).Cmp("GETMAXCONPERFIVE") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetMaxConperFive());
				return(buffer);
			}

			if (item.Mid(12,21).Cmp("SETTRANSFERFULLCHUNKS") == 0) {
				if ((item.Length() > 33) && item.Mid(34).IsNumber()) {
					bool flag = (atoi(item.Mid(34).GetData()) == 0) ? false : true;
					theApp.glob_prefs->SetTransferFullChunks(flag);
					return("TransferFullChunks Saved");
				}
				return("Bad SETTRANSFERFULLCHUNKS request");
			}

			if (item.Mid(12).Cmp("GETTRANSFERFULLCHUNKS") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->TransferFullChunks());
				return(buffer);
			}
			
			if (item.Mid(12,14).Cmp("SETPREVIEWPRIO") == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					theApp.glob_prefs->SetTransferFullChunks((atoi(item.Mid(27).GetData()) == 0) ? false : true);
					return("PreviewPrio Saved");
				}
				return("Bad SETPREVIEWPRIO request");
			}

			if (item.Mid(12).Cmp("GETPREVIEWPRIO") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetPreviewPrio());
				return(buffer);
			}

			return("Bad PREFERENCES request");
		} //end - PREFERENCES
		
		
		// LOGGING
		if (item.Left(7).Cmp("LOGGING") == 0) {
			if (item.Mid(8, 10).Cmp("ADDLOGLINE") == 0) {
				if (item.Length() > 18) {
					bool flag;
					int separator = item.Mid(19).Find(" ");
					if (item.Mid(19, separator).IsNumber()) {
						flag = (atoi(item.Mid(19, separator).GetData()) == 0) ? false : true;
						theApp.amuledlg->AddLogLine(flag,item.Mid(19+separator+1));
						return("Line Logged");
					}
				}
				return("Bad ADDLOGLINE request");
			}
			
			if (item.Mid(8).Cmp("RESETLOG") == 0) {
				theApp.amuledlg->ResetLog();
				return("Log Cleared");
			}
			
			if (item.Mid(8).Cmp("RESETDEBUGLOG") == 0) {
				theApp.amuledlg->ResetDebugLog();
				return("DebugLog Cleared");
			}
			
			if (item.Mid(8).Cmp("SERVERINFO CLEAR") == 0) {
				//theApp.amuledlg->serverwnd->servermsgbox->SetWindowText("");
				wxTextCtrl* cv=(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_SERVERINFO);
				cv->Clear();
				return("ServerInfo Cleared");
			}

			if (item.Mid(8).Cmp("SERVERINFO GETTEXT") == 0) {
				//theApp.amuledlg->serverwnd->servermsgbox->GetText()));
				wxTextCtrl* cv=(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_SERVERINFO);
				static char buffer[1024];
				sprintf(buffer,"%s", cv->GetValue().GetData());
				return(buffer);
			}
			
			if (item.Mid(8).Cmp("GETALLLOGENTRIES") == 0) {
				wxTextCtrl* logview =(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_LOGVIEW);
				return((char*)logview->GetValue().GetData());
			}
			
			if (item.Mid(8).Cmp("GETALLDEBUGLOGENTRIES") == 0) {
				//shakraw, the same as above, but please DO NOT remove!
				//I like to have separated requests for this (say for future use)
				//also see comments in WebServer.cpp
				wxTextCtrl* logview =(wxTextCtrl*)theApp.amuledlg->serverwnd->FindWindowById(ID_LOGVIEW);
				return((char*)logview->GetValue().GetData());
			}

			return("Bad LOGGING request");
		} // end - LOGGING
		
		
		// STATISTICS
		if (item.Left(10).Cmp("STATISTICS") == 0) {
			if (item.Mid(11).Cmp("GETHTML") == 0) {
				return((wxChar*)theApp.amuledlg->statisticswnd->GetHTML().GetData());
			}
			
			if (item.Mid(11,11).Cmp("SETARANGEUL") == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					bool flag = (atoi(item.Mid(23).GetData()) == 0) ? false : true;
					theApp.amuledlg->statisticswnd->SetARange(flag,theApp.glob_prefs->GetMaxGraphUploadRate());
					return("SetARangeUL Saved");
				}
				return("Bad SETARANGE request");
			}

			if (item.Mid(11,11).Cmp("SETARANGEDL") == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					bool flag = (atoi(item.Mid(23).GetData()) == 0) ? false : true;
					theApp.amuledlg->statisticswnd->SetARange(flag,theApp.glob_prefs->GetMaxGraphDownloadRate());
					return("SetARangeDL Saved");
				}
				return("Bad SETARANGE request");
			}

			if (item.Mid(11).Cmp("GETTRAFFICOMETERINTERVAL") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.glob_prefs->GetTrafficOMeterInterval());
				return(buffer);
			}
			
		} //end - STATISTICS
		
		
		// SHAREDFILES
		if (item.Left(11).Cmp("SHAREDFILES") == 0) {
			if (item.Mid(12).Cmp("GETCOUNT") == 0) {
				static char buffer[1024];
				sprintf(buffer, "%d", theApp.sharedfiles->GetCount());
				return(buffer);
			}
			
			if (item.Mid(12,11).Cmp("GETFILENAME") == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(24).GetData()));
					if (cur_file) {
						return(cur_file->GetFileName());
					}
					return("Bad file");
				}
				return("Bad GETFILENAME request");
			}
			
			if (item.Mid(12,11).Cmp("GETFILESIZE") == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(24).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->GetFileSize());
						return(buffer);
					}
					return("Bad file");
				}
				return("Bad GETFILESIZE request");
			}

			if (item.Mid(12,16).Cmp("CREATEED2KSRCLNK") == 0) {
				if ((item.Length() > 28) && item.Mid(29).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(29).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%s", theApp.CreateED2kSourceLink(cur_file).GetData());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad CREATEED2KSRCLNK request");
			}

			if (item.Mid(12,13).Cmp("CREATEED2KLNK") == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(26).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%s", theApp.CreateED2kLink(cur_file).GetData());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad CREATEED2KLNK request");
			}
		
			if (item.Mid(12,13).Cmp("GETTRANSFERED") == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(26).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%lld", cur_file->statistic.GetTransfered());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETTRANSFERED request");
			}
			
			if (item.Mid(12,20).Cmp("GETALLTIMETRANSFERED") == 0) {
				if ((item.Length() > 32) && item.Mid(33).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(33).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%lld", cur_file->statistic.GetAllTimeTransfered());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETALLTIMETRANSFERED request");
			}

			if (item.Mid(12,11).Cmp("GETREQUESTS") == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(24).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->statistic.GetRequests());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETREQUESTS request");
			}
			
			if (item.Mid(12,18).Cmp("GETALLTIMEREQUESTS") == 0) {
				if ((item.Length() > 31) && item.Mid(32).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(32).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->statistic.GetAllTimeRequests());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETALLTIMEREQUESTS request");
			}
			
			if (item.Mid(12,10).Cmp("GETACCEPTS") == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(23).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->statistic.GetAccepts());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETACCEPTS request");
			}

			if (item.Mid(12,17).Cmp("GETALLTIMEACCEPTS") == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(30).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->statistic.GetAllTimeAccepts());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETALLTIMEACCEPTS request");
			}

			if (item.Mid(12,14).Cmp("GETENCFILEHASH") == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(27).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%s", EncodeBase16(cur_file->GetFileHash(), 16).GetData());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETENCFILEHASH request");
			}

			if (item.Mid(12,16).Cmp("ISAUTOUPPRIORITY") == 0) {
				if ((item.Length() > 28) && item.Mid(29).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(29).GetData()));
					if (cur_file) {
						if (cur_file->IsAutoUpPriority())
							return("Is AutoUp Priority");
						else
							return("Is Not AutoUp Priority");
					}
					return("Bad file");					
				}
				return("Bad ISAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12,13).Cmp("GETUPPRIORITY") == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					CKnownFile* cur_file = theApp.sharedfiles->GetFileByIndex(atoi(item.Mid(26).GetData()));
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%d", cur_file->GetUpPriority());
						return(buffer);
					}
					return("Bad file");					
				}
				return("Bad GETUPPRIORITY request");
			}
			
			if (item.Mid(12,17).Cmp("SETAUTOUPPRIORITY") == 0) {
				if (item.Length() > 29) {
					int separator = item.Mid(30).Find(" ");
					if (separator!=-1) {
						wxString hash = item.Mid(30,separator);
						if (item.Mid(30+separator+1).IsNumber()) {
							bool flag = (atoi(item.Mid(30+separator+1).GetData()) == 0) ? false : true;
						
							uchar fileid[16];
							DecodeBase16(hash.GetData(),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetAutoUpPriority(flag);
								return("AutoUpPriority Saved");
							}
							return("Bad file");
						}
					}
				}
				return("Bad SETAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12,13).Cmp("SETUPPRIORITY") == 0) {
				if (item.Length() > 25) {
					int separator = item.Mid(26).Find(" ");
					if (separator!=-1) {
						wxString hash = item.Mid(26,separator);
						if (item.Mid(26+separator+1).IsNumber()) {
							int priority = atoi(item.Mid(26+separator+1).GetData());
						
							uchar fileid[16];
							DecodeBase16(hash.GetData(),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetUpPriority(priority);
								return("UpPriority Saved");
							}
							return("Bad file");
						}
					}
				}
				return("Bad SETUPPRIORITY request");
			}

			if (item.Mid(12,20).Cmp("UPDATEAUTOUPPRIORITY") == 0) {
				if (item.Length() > 32) {
					int separator = item.Mid(33).Find(" ");
					if (separator!=-1) {
						wxString hash = item.Mid(33,separator);
						uchar fileid[16];
						DecodeBase16(hash.GetData(),hash.Length(),fileid);
						CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
						if (cur_file) {
							cur_file->UpdateAutoUpPriority();
							return("UpPriority Saved");
						}
						return("Bad file");
					}
				}
				return("Bad UPDATEAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12).Cmp("RELOAD") == 0) {
				theApp.sharedfiles->Reload(true, false);
				return("SharedFiles Reloaded");
			}

			return("Bad SHAREDFILES request");
		} //end - shared files
		
		
		// IDSTAT
		if (item.Left(6).Cmp("IDSTAT") == 0) { //get id type
			if (theApp.serverconnect->IsLowID())
				return("Low ID");
			else
				return("High ID");
		}
		
		
		// GETCATCOUNT
		if (item.Left(11).Cmp("GETCATCOUNT") == 0) { //get categories number
			static char buffer[10];
			sprintf(buffer, "%d", theApp.glob_prefs->GetCatCount());
			return(buffer);
		}
		
		
		// SPEED
		if (item.Left(5).Cmp("SPEED") == 0) { //get upload/download datarate
			static char buffer[1024];
			if (item.Mid(6).Cmp("UL") == 0) { //upload
				sprintf(buffer, "%.1f", theApp.uploadqueue->GetKBps());	
			} else if (item.Mid(6).Cmp("DL") == 0) { //download
				sprintf(buffer, "%.1f", theApp.downloadqueue->GetKBps());
			} else {
				sprintf(buffer, "Wrong speed requested");
			}
			return(buffer);
		}
		
		
		// MAX
		if (item.Left(3).Cmp("MAX") == 0) { //get max upload/download values
			static char buffer[1024];
			if (item.Mid(4).Cmp("UL") == 0) { //upload
				sprintf(buffer, "%i", theApp.glob_prefs->GetMaxUpload());
			} else if (item.Mid(4).Cmp("DL") == 0) { //download
				sprintf(buffer, "%i", theApp.glob_prefs->GetMaxDownload());
			} else {
				sprintf(buffer, "Wrong value requested");
			}
			return(buffer);
		}
		

		// SERVER		
		if (item.Left(6).Cmp("SERVER") == 0) {
			if (item.Mid(7).Cmp("LIST") == 0) {
				// shakraw - return a unique string where each line is formatted 
				// as: wxString\twxString\tint\twxString\tint\tint\tint\n
				wxString buffer("");
				wxString tempBuf("");
				for (uint i=0; i < theApp.serverlist->GetServerCount(); i++) {
					CServer *server = theApp.serverlist->GetServerAt(i);
					if (server) {
						tempBuf.Printf("%s\t%s\t%d\t%s\t%d\t%d\t%d\n",
							server->GetListName(),
							server->GetDescription(),
							server->GetPort(),
							server->GetAddress(),
							server->GetUsers(),
							server->GetMaxUsers(),
							server->GetFiles()
						);
						buffer.Append(tempBuf);
					}
				}
				return((wxChar *)buffer.GetData());
			}
			
			if (item.Mid(7).Cmp("COUNT") == 0) { //get number of server in serverlist
				static char buffer[1024];
				sprintf(buffer, "%i", theApp.serverlist->GetServerCount());
				return(buffer);
			}
			
			if (item.Mid(7,4).Cmp("NAME") == 0) { // get server name
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(12).GetData()));
					return(server->GetListName());
				} else if (theApp.serverconnect->IsConnected()) {
					return(theApp.serverconnect->GetCurrentServer()->GetListName());
				} else
					return("");
			}
			
			if (item.Mid(7,4).Cmp("DESC") == 0) { // get the server description
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(12).GetData()));
					return(server->GetDescription());
				} else if (theApp.serverconnect->IsConnected())
					return(theApp.serverconnect->GetCurrentServer()->GetDescription());
				else
					return("");
			}

			if (item.Mid(7,4).Cmp("PORT") == 0) { // get the server port
				static char buffer[1024];
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(12).GetData()));
					sprintf(buffer, "%i", server->GetPort());
				} else if (theApp.serverconnect->IsConnected())
					sprintf(buffer, "%i", theApp.serverconnect->GetCurrentServer()->GetPort());
				else 
					*buffer = 0;
				
				return(buffer);
			}

			if (item.Mid(7,2).Cmp("IP") == 0) { // get the server address
				if ((item.Length() > 9) && (item.Mid(10).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(10).GetData()));
					return(server->GetAddress());
				} else if (theApp.serverconnect->IsConnected())
					return(theApp.serverconnect->GetCurrentServer()->GetAddress());
				else	
					return("");
			}
			
			if (item.Mid(7,5).Cmp("USERS") == 0) { //get the number of users in the server we are connected to
				static char buffer[1024];
				if ((item.Length() > 12) && (item.Mid(13).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(13).GetData()));
					sprintf(buffer, "%i", server->GetUsers());
				} else if (theApp.serverconnect->IsConnected())
					sprintf(buffer, "%i", theApp.serverconnect->GetCurrentServer()->GetUsers());
				else 
					*buffer = 0;

				return(buffer);
			}
			
			if (item.Mid(7,8).Cmp("MAXUSERS") == 0) { // get the max number of users in a server
				static char buffer[1024];
				if ((item.Length() > 12) && (item.Mid(16).IsNumber())) {
					CServer* cur_file = theApp.serverlist->GetServerAt(atoi(item.Mid(16).GetData()));
					sprintf(buffer, "%i", cur_file->GetMaxUsers());
				} else if (theApp.serverconnect->IsConnected())
					sprintf(buffer, "%i", theApp.serverconnect->GetCurrentServer()->GetMaxUsers());
				else
				  *buffer = 0;
				
				return(buffer);
			}

			if (item.Mid(7,5).Cmp("FILES") == 0) { // get the number of file shared in a server
				static char buffer[1024];
				if ((item.Length() > 12) && (item.Mid(13).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt(atoi(item.Mid(13).GetData()));
					sprintf(buffer, "%i", server->GetFiles());
				} else if (theApp.serverconnect->IsConnected())
					sprintf(buffer, "%s", theApp.serverconnect->GetCurrentServer()->GetListName());
				else
					*buffer = 0;

				return(buffer);
			}
			
			if (item.Mid(7,7).Cmp("CONNECT") == 0) { //Connect to a server (if specified) or to any server
				if (item.Length() > 14) { // try to connect to the specified server
					int separator = item.Mid(15).Find(" ");
					wxString sIP = item.Mid(15,separator);
					wxString sPort = item.Mid(15+separator+1);
					CServer* server = theApp.serverlist->GetServerByAddress((char*)sIP.GetData(), atoi(sPort.GetData()));
					if (server != NULL) {
						theApp.serverconnect->ConnectToServer(server);
						theApp.amuledlg->ShowConnectionState(false);
						return("Connected");
					} else
						return("Not Connected");
				} else { //connect to any server
					if (theApp.serverconnect->IsConnected()) {
						return("Already connected");
					} else {
						theApp.serverconnect->ConnectToAnyServer();
						theApp.amuledlg->ShowConnectionState(false);
						return("Connected");
					}
				}
			}
			
			if (item.Mid(7,3).Cmp("ADD") == 0) { //add a new server
				if (item.Length() > 10) {
					int separator1 = item.Mid(11).Find(" ");
					wxString sIP = item.Mid(11, separator1);
					int separator2 = item.Mid(11+separator1+1).Find(" ");
					wxString sPort = item.Mid(11+separator1+1,separator2);
					wxString sName = item.Mid(11+separator1+separator2+2);
					
					CServer *nsrv = new CServer(atoi(sPort.GetData()), (char*)sIP.GetData());
					nsrv->SetListName((char*)sName.GetData());
					theApp.amuledlg->serverwnd->serverlistctrl->AddServer(nsrv, true);
					return("Server Added");
				}
				return("Server Not Added");
			}
			
			if (item.Mid(7,6).Cmp("REMOVE") == 0) { // remove selected server from serverlist
				if (item.Length() > 13) {
					int separator = item.Mid(14).Find(" ");
					wxString sIP = item.Mid(14, separator);
					wxString sPort = item.Mid(14+separator+1);
					CServer* server = theApp.serverlist->GetServerByAddress((char*)sIP.GetData(), atoi(sPort.GetData()));
					if (server != NULL) {
						theApp.serverlist->RemoveServer(server);
						return("Removed");
					}
				}
				return("Not removed");
			}
			
			if (item.Mid(7,9).Cmp("UPDATEMET") == 0) {
				if (item.Length() > 16) {
					theApp.amuledlg->serverwnd->UpdateServerMetFromURL(item.Mid(17));
					return("Updated");
				}
				return("Not Updated");
			}
			
			return("Bad SERVER Request");
		}
		
		
		// TRANSFER
		if (item.Left(8).Cmp("TRANSFER") == 0) {
			if (item.Mid(9) == "DL_LIST") {
				// shakraw - return a big string where each line is formatted as:
				// wxString\tlong\tlong\tdouble\tlong\tint\twxString\tint\twxString\tlong\tlong\tlong\twxString\twxString\tint\n
				wxString buffer("");
				wxString tempFileInfo("");
				//int tempPrio;
				for (int i=0; i < theApp.downloadqueue->GetFileCount(); i++) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
					if (cur_file) {
						buffer+=wxString::Format("%s\t", cur_file->GetFileName().c_str());
						buffer+=wxString::Format("%li\t", (long)cur_file->GetFileSize());
						buffer+=wxString::Format("%li\t", (long)cur_file->GetTransfered());
						buffer+=wxString::Format("%f\t", cur_file->GetPercentCompleted());
#ifdef DOWNLOADRATE_FILTERED
						buffer+=wxString::Format("%li\t", (long)(cur_file->GetKBpsDown()*1024));
#else
						buffer+=wxString::Format("%u\t", cur_file->GetDatarate());
#endif
						buffer+=wxString::Format("%d\t", cur_file->GetStatus());
						buffer+=wxString::Format("%s\t", cur_file->getPartfileStatus().c_str());
						if (cur_file->IsAutoDownPriority())
							buffer+=wxString::Format("%d\t", cur_file->GetDownPriority()+10);
						else
							buffer+=wxString::Format("%d\t", cur_file->GetDownPriority());
						buffer+=EncodeBase16(cur_file->GetFileHash(), 16)+wxString("\t");
						buffer+=wxString::Format("%d\t", cur_file->GetSourceCount());
						buffer+=wxString::Format("%d\t", cur_file->GetNotCurrentSourcesCount());
						buffer+=wxString::Format("%d\t", cur_file->GetTransferingSrcCount());
						if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID())
							buffer+=theApp.CreateED2kSourceLink(cur_file)+wxString("\t");
						else
							buffer+=theApp.CreateED2kLink(cur_file)+wxString("\t");

						tempFileInfo = cur_file->GetDownloadFileInfo();
						tempFileInfo.Replace("\n","|");
						buffer+=tempFileInfo+wxString("\t");
						if (!cur_file->IsPartFile())
							buffer+=wxString("1\n");
						else
							buffer+=wxString("0\n");
					}
				}
				return((wxChar *)buffer.GetData());
			}
			
			if (item.Mid(9) == "UL_LIST") {
				// shakraw - return one string where each line is formatted as:
				// %s\t%s\t%s\t%f\t%f\t%li\n"
				wxString buffer("");
				wxString tempFileInfo("");
				for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
					 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
					CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
					if (cur_client) {
						buffer+=wxString::Format("%s\t", cur_client->GetUserName());
						tempFileInfo=cur_client->GetUploadFileInfo();
						tempFileInfo.Replace("\n", "|");
						buffer+=wxString::Format("%s\t", tempFileInfo.GetData());
						CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
						if (file)
							buffer+=wxString::Format("%s\t", file->GetFileName().GetData());
						else
							buffer+=wxString("?\t");
						buffer+=wxString::Format("%li\t", (long)cur_client->GetTransferedDown());
						buffer+=wxString::Format("%li\t", (long)cur_client->GetTransferedUp());
						buffer+=wxString::Format("%li\n", (long)(cur_client->GetKBpsUp()*1024.0));
					}
				}
				return((wxChar*)buffer.GetData());
			}
			
			if (item.Mid(9) == "DL_FILEHASH") {
				// shakraw - return a big string where each line is formatted as:
				// fhash\tfhash\t...\tfhash
				wxString buffer("");
				for (int i=0; i < theApp.downloadqueue->GetFileCount(); i++) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
					if (cur_file) {
						buffer.Append(cur_file->GetFileHash());
						if (i != theApp.downloadqueue->GetFileCount()-1)
							buffer.Append("\t");
					}
				}
				return((wxChar *)buffer.GetData());
			}

			
			
			if (item.Mid(9).Cmp("CLEARCOMPLETE") == 0) { //clear completed transfers
				theApp.amuledlg->transferwnd->downloadlistctrl->ClearCompleted();
				return("Clear Completed");
			}
			
			if (item.Mid(9,11).Cmp("ADDFILELINK") == 0) { //add file link to download
				if (item.Length() > 20) {
					CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(item.Mid(21));
					if (pLink->GetKind() == CED2KLink::kFile) {
						theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink()); 
						delete pLink;
						return("Link Added");
					}
					delete pLink;
				}
				return("Bad ADDFILELINK request");
			}
			
			if (item.Mid(9).Cmp("DL_COUNT") == 0) { //get number of downloading files
				static char buffer[1024];
				sprintf(buffer, "%i", theApp.downloadqueue->GetFileCount());
				return(buffer);
			}
			
			if (item.Mid(9,11).Cmp("DL_FILENAME") == 0) { //get the n-th file name
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%s", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(21).GetData()))->GetFileName().GetData());
					return(buffer);
				}
				return("Bad DL_FILENAME request");
			}

			if (item.Mid(9,11).Cmp("DL_FILESIZE") == 0) { //get the n-th file size
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(21).GetData()))->GetFileSize());
					return(buffer);
				}
				return("Bad DL_FILESIZE request");
			}

			if (item.Mid(9,17).Cmp("DL_FILETRANSFERED") == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(27).GetData()))->GetTransfered());
					return(buffer);
				}
				return("Bad DL_FILETRANSFERED request");
			}

			if (item.Mid(9,14).Cmp("DL_FILEPERCENT") == 0) {
				if ((item.Length() > 23) && (item.Mid(24).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%f", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(24).GetData()))->GetPercentCompleted());
					return(buffer);
				}
				return("Bad DL_FILEPERCENT request");
			}	
			
			if (item.Mid(9,15).Cmp("DL_FILEDATARATE") == 0) {
				if ((item.Length() > 24) && (item.Mid(25).IsNumber())) {
					static char buffer[1024];
#ifdef DOWNLOADRATE_FILTERED
					sprintf(buffer, "%i", (int)(theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(25).GetData()))->GetKBpsDown())*1024);
#else
					sprintf(buffer, "%i", (int)(theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(25).GetData()))->GetDatarate()));
#endif
					return(buffer);
				}
				return("Bad DL_FILEDATARATE request");
			}	

			if (item.Mid(9,13).Cmp("DL_FILESTATUS") == 0) {
				if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(23).GetData()))->GetStatus());
					return(buffer);
				}
				return("Bad DL_FILESTATUS request");
			}	
			
			if (item.Mid(9,17).Cmp("DL_PARTFILESTATUS") == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%s", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(27).GetData()))->getPartfileStatus().GetData());
					return(buffer);
				}
				return("Bad DL_PARTFILESTATUS request");
			}
			
			if (item.Mid(9,18).Cmp("DL_FILEGETDOWNPRIO") == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(28).GetData()))->GetDownPriority());
					return(buffer);
				}
				return("Bad DL_FILEGETDOWNPRIO request");
			}	

			if (item.Mid(9,21).Cmp("DL_FILEISAUTODOWNPRIO") == 0) {
				if ((item.Length() > 30) && (item.Mid(31).IsNumber())) {
					if (theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(31).GetData()))->IsAutoDownPriority())
						return("AutoDown Prio");
					else
						return("Not AutoDown Prio");
				}
				return("Bad DL_FILEISAUTODOWNPRIO request");
			}	

			if (item.Mid(9,11).Cmp("DL_FILEHASH") == 0) {
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%s", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(21).GetData()))->GetFileHash());
					return(buffer);
				}
				return("Bad DL_FILEHASH request");
			}	
			
			if (item.Mid(9,17).Cmp("DL_FILEENCODEHASH") == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%s", EncodeBase16(theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(27).GetData()))->GetFileHash(), 16).GetData());
					return(buffer);
				}
				return("Bad DL_FILEENCODEHASH request");
			}	

			if (item.Mid(9,18).Cmp("DL_FILESOURCECOUNT") == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(28).GetData()))->GetSourceCount());
					return(buffer);
				}
				return("Bad DL_FILESOURCECOUNT request");
			}	
			
			if (item.Mid(9,19).Cmp("DL_FILENOCURRSOURCE") == 0) {
				if ((item.Length() > 28) && (item.Mid(29).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(29).GetData()))->GetNotCurrentSourcesCount());
					return(buffer);
				}
				return("Bad DL_FILENOCURRSOURCE request");
			}	
			
			if (item.Mid(9,17).Cmp("DL_FILETRSRCCOUNT") == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					static char buffer[1024];
					sprintf(buffer, "%i", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(27).GetData()))->GetTransferingSrcCount());
					return(buffer);
				}
				return("Bad DL_FILETRSRCCOUNT request");
			}	
			
			if (item.Mid(9,18).Cmp("DL_FILEED2KSRCLINK") == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					static char buffer[1024];					
					sprintf(buffer, "%s", theApp.CreateED2kSourceLink(theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(28)))).GetData());
					return(buffer);
				}
				return("Bad DL_FILEED2KSRCLINK request");
			}	

			if (item.Mid(9,15).Cmp("DL_FILEED2KLINK") == 0) {
				if ((item.Length() > 24) && (item.Mid(25).IsNumber())) {
					static char buffer[1024];					
					sprintf(buffer, "%s", theApp.CreateED2kLink(theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(25).GetData()))).GetData());
					return(buffer);
				}
				return("Bad DL_FILEED2KLINK request");
			}	
			
			if (item.Mid(9,11).Cmp("DL_FILEINFO") == 0) {
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(21).GetData()))->GetDownloadFileInfo();
					static char buffer[1024];					
					sprintf(buffer, "%s", theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(21).GetData()))->GetDownloadFileInfo().GetData());
					return(buffer);
				}
				return("Bad DL_FILEINFO request");
			}	

			if (item.Mid(9,17).Cmp("DL_FILEISPARTFILE") == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					if (theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(27).GetData()))->IsPartFile())
						return("Is PartFile");
					else
						return("Is Not PartFile");
				}
				return("Bad DL_FILEISPARTFILE request");
			}	

			if (item.Mid(9,12).Cmp("DL_FILEPAUSE") == 0) { //pause the n-th downloading file
				if ((item.Length() > 21) && (item.Mid(22).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(22).GetData()));
					if (cur_file) {
						cur_file->PauseFile();
						return("File Paused");
					}
				}
				return("Bad DL_FILEPAUSE request");
			}

			if (item.Mid(9,13).Cmp("DL_FILERESUME") == 0) { //resume the n-th downloading file
				if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(23).GetData()));
					if (cur_file) {
						cur_file->ResumeFile();
						return("File Resumed");
					}
				}
				return("Bad DL_FILERESUME request");
			}

			if (item.Mid(9,13).Cmp("DL_FILEDELETE") == 0) { //delete the n-th downloading file
				if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(23).GetData()));
					if (cur_file) {
						cur_file->Delete();
						return("File Deleted");
					}
				}
				return("Bad DL_FILEDELETE request");
			}

			if (item.Mid(9,21).Cmp("DL_FILESETAUTODOWNPRIO") == 0) { //set autodownprio for the file
				if (item.Length() > 30) {
					int separator = item.Mid(31).Find(" ");
					int file_Idx = -1; 
					bool flag = false;
					if (item.Mid(31, separator).IsNumber())
						file_Idx = atoi(item.Mid(31, separator).GetData());
					if (item.Mid(31+separator+1).IsNumber())
						flag = (atoi(item.Mid(31+separator+1).GetData()) == 0) ? false : true;

					if (file_Idx >= 0) {
						CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(file_Idx);
						if (cur_file) {
							cur_file->SetAutoDownPriority(flag);
							return("AutoDownPrio Set");
						}
					}
				}
				return("Bad DL_FILESETAUTODOWNPRIO request");
			}
			
			if (item.Mid(9,18).Cmp("DL_FILESETDOWNPRIO") == 0) { //set autodownprio for the file
				if (item.Length() > 27) {
					int separator = item.Mid(28).Find(" ");
					int file_Idx = -1; 
					int prio = -1;
					if (item.Mid(31, separator).IsNumber())
						file_Idx = atoi(item.Mid(28, separator).GetData());
					if (item.Mid(31+separator+1).IsNumber())
						prio = atoi(item.Mid(28+separator+1).GetData());

					if ((file_Idx >= 0) && (prio >= 0)) {
						CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(file_Idx);
						if (cur_file) {
							cur_file->SetDownPriority(prio);
							return("DownPrio Set");
						}
					}
				}
				return("Bad DL_FILESETDOWNPRIO request");
			}
			
			if (item.Mid(9,7).Cmp("DL_FILE") == 0) { //test if the n-th downloading file exists
				if ((item.Length() > 16) && (item.Mid(17).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(atoi(item.Mid(17).GetData()));
					if (cur_file) return("Got File");
				}
				return("Bad DL_FILE request");
			}
			
			return("Bad TRANSFER request");
		} // end - TRANSFER
		
		
		// FILE
		if (item.Left(4).Cmp("FILE") == 0) {
			if (item.Mid(5,11).Cmp("GETFILEBYID") == 0) {
				if (item.Length() > 16) {
					uchar fileid[16];
					wxString filehash = item.Mid(17);
					DecodeBase16(filehash.GetData(),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if (cur_file) return("Got File");
					
					return("No File");
				}
				return("Bad GETFILEBYID request");
			}
			
			if (item.Mid(5,10).Cmp("ISPARTFILE") == 0) { //return wether file is partfile or not
				if (item.Length() > 15) {
					uchar fileid[16];
					wxString filehash = item.Mid(16);
					DecodeBase16(filehash.GetData(),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if ((cur_file) && (cur_file->IsPartFile())){
						return("Is PartFile");
					}
					return("Is Not PartFile");
				}
				return("Bad ISPARTFILE request");
			}

			if (item.Mid(5,17).Cmp("GETPROGRESSSTRING") == 0) { //get file's progress bar string
				if (item.Length() > 22) {
					int separator = item.Mid(23).Find(" ");
					if (separator > -1) {
						wxString filehash = item.Mid(23,separator);
						uint16 progressbarWidth = 0;
						if (item.Mid(23+separator+1).IsNumber())
							progressbarWidth = atoi(item.Mid(23+separator+1).GetData());
						
						uchar fileid[16];
						DecodeBase16(filehash.GetData(),filehash.Length(),fileid);
						CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
						if (cur_file) {
							static char buffer[1024];
							sprintf(buffer, "%s", cur_file->GetProgressString(progressbarWidth).GetData());
							return(buffer);
						}
						
						return("No File");
					}
				}
				return("Bad GETPROGRESSSTRING request");
			}
			
			if (item.Mid(5,18).Cmp("GETPERCENTCOMPLETE") == 0) { //get file's progress bar string
				if (item.Length() > 23) {
					wxString filehash = item.Mid(24);
					uchar fileid[16];
					DecodeBase16(filehash.GetData(),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if (cur_file) {
						static char buffer[1024];
						sprintf(buffer, "%f", cur_file->GetPercentCompleted());
						return(buffer);
					}
					
					return("No File");
				}
				return("Bad GETPERCENTCOMPLETE request");
			}
			
			return("Bad FILE request");
		} // end - FILE
		
		
		// QUEUE
		if (item.Left(5).Cmp("QUEUE") == 0) { //Get queue data information
			if (item.Mid(6).Cmp("W_GETSHOWDATA") == 0) {
				wxString buffer = "";
				for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList();
					 pos != 0;theApp.uploadqueue->GetNextFromWaitingList(pos)) {
					CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
					if (cur_client) {
						buffer.Append(wxString::Format("%s", cur_client->GetUserName()));
						if (!cur_client->reqfile) {
							buffer.Append("\n");
							continue;
						} else {
							buffer.Append("\t");
						}
						CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->reqfile->GetFileHash());
						if (file)
							buffer.Append(wxString::Format("%s\t", cur_client->GetDownloadFile()->GetFileName().GetData()));
						else
							buffer.Append("?\t");
						buffer.Append(wxString::Format("%i\t", cur_client->GetScore(false)));
						if (cur_client->IsBanned())
							buffer.Append("1\n");
						else
							buffer.Append("0\n");
					}
				}
				return((wxChar*)buffer.GetData());
			}
			
			if (item.Mid(6).Cmp("UL_GETSHOWDATA") == 0) {
				wxString buffer = "";
				wxString temp = "";
				for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
					 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
					CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
					if (cur_client) {
						buffer.Append(wxString::Format("%s\t", cur_client->GetUserName()));
						temp = cur_client->GetUploadFileInfo(); temp.Replace("\n", " | ");
						buffer.Append(wxString::Format("%s\t", temp.GetData()));
						CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
						if (file)
							buffer.Append(wxString::Format("%s\t", file->GetFileName().GetData()));
						else
							buffer.Append("?\t");
						buffer.Append(wxString::Format("%i\t", cur_client->GetTransferedDown()));
						buffer.Append(wxString::Format("%i\t", cur_client->GetTransferedUp()));
						buffer.Append(wxString::Format("%i\n", (uint32)(cur_client->GetKBpsUp()*1024.0)));
					}
				}
				return((wxChar*)buffer.GetData());
			}

			if (item.Mid(6).Cmp("UL_GETLENGTH") == 0) {
				static char buffer[1024];
				sprintf(buffer,"%i", theApp.uploadqueue->GetUploadQueueLength());
				return(buffer);
			}
			
			return("Bad QUEUE request");
		} // end - QUEUE
		
		
		// SEARCH
		if (item.Left(6).Cmp("SEARCH")==0) {
			if (item.Mid(7,12).Cmp("DOWNLOADFILE") == 0) { //add file to download
				if (item.Length() > 19) {
					uchar fileid[16];
					DecodeBase16(item.Mid(20).GetData(),item.Mid(20).Length(),fileid);
					theApp.searchlist->AddFileToDownloadByHash(fileid);
					//theApp.downloadqueue->AddDownload(cur_file, (uint8)theApp.glob_prefs->AddNewFilesPaused());
					return("Download Added");
				}
				return("Bad DOWNLOADFILE request");
			}
			
			if (item.Mid(7,16).Cmp("DELETEALLSEARCHS") == 0) { //delete all searchs
				theApp.amuledlg->searchwnd->DeleteAllSearchs();
				return("Searchs Deleted");
			}
			
			if (item.Mid(7,11).Cmp("DONEWSEARCH") == 0) {
				if (item.Length() > 18) {
					int curIndex, nextIndex;
					wxString sParams = item.Mid(19);
					
					curIndex=0; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxTextCtrl*)wxWindow::FindWindowById(IDC_SEARCHNAME))->SetValue(sParams.Left(nextIndex)); //_ParseURL(Data.sURL, "tosearch");
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxChoice*)wxWindow::FindWindowById(IDC_TypeSearch))->SetStringSelection(sParams.Mid(curIndex, nextIndex)); //_ParseURL(Data.sURL, "type");
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHMIN))->SetValue(sParams.Mid(curIndex, nextIndex)); //atol(_ParseURL(Data.sURL, "min"))*1048576;
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHMAX))->SetValue(sParams.Mid(curIndex, nextIndex));
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHAVAIBILITY))->SetValue(sParams.Mid(curIndex, nextIndex)); //_ParseURL(Data.sURL, "avail")=="")?-1:atoi(_ParseURL(Data.sURL, "avail"));
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					((wxTextCtrl*)wxWindow::FindWindowById(IDC_EDITSEARCHEXTENSION))->SetValue(sParams.Mid(curIndex, nextIndex)); //_ParseURL(Data.sURL, "ext");
					curIndex=curIndex+nextIndex+1; nextIndex=sParams.Mid(curIndex).Find("\n");
					
					if (sParams.Mid(curIndex, nextIndex).Cmp("global") == 0) //Method
						((wxCheckBox*)wxWindow::FindWindowByName("globalSearch"))->SetValue(true); //SearchTypeGlobal;
					else
						((wxCheckBox*)wxWindow::FindWindowByName("globalSearch"))->SetValue(false); //SearchTypeServer;
					
					wxCommandEvent evt;
					theApp.amuledlg->searchwnd->OnBnClickedStarts(evt); //do a new search
					return("Search Done");
				}
				return("Bad DONEWSEARCH request");
			}
			
			if (item.Mid(7,7).Cmp("WEBLIST") == 0) {
				if (item.Length() > 14) {
					wxString sItem = item.Mid(15);
					int separator=sItem.Find("\t");
					if (separator != -1) {
						wxString sResultLine = sItem.Left(separator+1);
						sItem = sItem.Mid(separator+1);
						separator=sItem.Find("\t");
						int sortBy = atoi(sItem.Left(separator+1).GetData());
						if (sortBy >= 0) {
							bool searchAsc = (atoi(sItem.Mid(separator+1).GetData()) == 0) ? false : true;
							return((wxChar*)theApp.searchlist->GetWebList((CString)sResultLine, sortBy, searchAsc).GetData());
						}
					}
				}
				return("Bad WEBLIST request");
			}
			
			return("Bad SEARCH request");
		} // end - SEARCH
		//shakraw, amuleweb protocol communication START
		
		// New item: for release goups only, TOTALULREQ HASH replies the total upload for the file HASH
		
		if (item.Left(10).Cmp("TOTALULREQ") == 0) { 
			
			static char buffer[1024];
			#ifdef RELEASE_GROUP_MODE
			if (item.Mid(11).Length() == 16) {
				CKnownFile* shared_file = theApp.sharedfiles->GetFileByID((uchar*)item.Mid(11).c_str());
				if (shared_file!=NULL) {
					sprintf(buffer, "%i",shared_file->totalupload);
				} else {
					sprintf(buffer, "FILENOTSHARED");
				}
			} else {
				sprintf(buffer, "WRONGHASH",item.Mid(11).c_str());
			}	
			#else
				sprintf(buffer, "NOTRELEASEGRP");
			#endif	

			return(buffer);
		}		
		
		
		return "";
	}
