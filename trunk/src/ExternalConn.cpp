//
// This file is part of the aMule Project
//
// aMule Copyright (C) 2003 aMule Team ( http://www.amule-project.net )
// This file Copyright (C) 2003 Kry (elkry@sourceforge.net  http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/datetime.h>
#include <wx/textctrl.h>	// Needed for wxTextCtrl

#include "ExternalConn.h"	// Interface declarations
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "otherfunctions.h"	// Needed for EncodeBase16
#include "SearchDlg.h"		// Needed for CSearchDlg
#include "updownclient.h"	// Needed for CUpDownClient
#include "ED2KLink.h"		// Needed for CED2KLink
#include "server.h"		// Needed for CServer
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
#include "amule.h"		// Needed for theApp
#include "SearchList.h"		// Needed for GetWebList
#include "IPFilter.h"		// Needed for CIPFilter
#include "ClientList.h"
#include "gsocket-fix.h"	// Needed for wxSOCKET_REUSEADDR


ExternalConn::ExternalConn() {
	// Are we allowed to accept External Connections
	if (theApp.glob_prefs->AcceptExternalConnections()) {
		server = new ExternalConnServerThread(this);
		if ( server->Ready() ) {
			server->Run();
			return;
		} else {
			printf("ExternalConn: server thread not ready\n");
		}
	} else {
		printf("External connections disabled in .eMule\n");
		return;
	}
}

ExternalConn::~ExternalConn() {
	if ( server)
		server->Delete();
}

//
// Authentication
//
wxString ExternalConn::Authenticate(const wxString& item)
{
	if (item.Left(4) == wxT("AUTH")) {
		if (item.Mid(5) == wxT("aMuleweb ") + theApp.glob_prefs->ECPassword()) {
			printf("Accepted Connection from amuleweb\n"); 
			return wxT("Authenticated"); 
		}
		if (item.Mid(5) == wxT("aMulecmd ") + theApp.glob_prefs->ECPassword()) {
			printf("Accepted Connection from amulecmd\n"); 
			return wxT("Authenticated");
		}
	}
	return wxT("Access Denied");
}
	
//TODO: do a function for each command
wxString ExternalConn::ProcessRequest(const wxString& item) {
	unsigned int nChars = 0;
	wxString sOp;
	//---------------------------------------------------------------------
	// WEBPAGE
	//---------------------------------------------------------------------
	if (item == wxT("WEBPAGE HEADER")) {
		// returns one string formatted as:
		// %d\t%s\t%s\t%d\t%d\t%f\t%f\t%d\t%d
		wxString buffer = wxString() << theApp.glob_prefs->GetWebPageRefresh() << wxT("\t");		
		if (theApp.serverconnect->IsConnected()) {
			buffer += wxString(wxT("Connected\t"));
		} else if (theApp.serverconnect->IsConnecting()) {
			buffer += wxString(wxT("Connecting\t"));
		} else {
			buffer += wxString(wxT("Disconnected\t"));
		}		
		if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting()) {
			if (theApp.serverconnect->IsLowID()) {
				buffer += wxT("Low ID\t");
			} else {
				buffer += wxT("High ID\t");
			}			
			if (theApp.serverconnect->IsConnected()) {
				buffer += theApp.serverconnect->GetCurrentServer()->GetListName() +
					wxString::Format(wxT("\t%d\t"),
						theApp.serverconnect->GetCurrentServer()->GetUsers());
			}
		}		
		buffer += wxString::Format(wxT("%.1f\t%.1f\t%d\t%d\t"), 
			theApp.uploadqueue->GetKBps(),
			theApp.downloadqueue->GetKBps(),
			theApp.glob_prefs->GetMaxUpload(),
			theApp.glob_prefs->GetMaxDownload());
		
		return buffer;
	}
	if (item == wxT("WEBPAGE GETGRAPH")) {
		//returns one string formatted as:
		//%d\t%d\t%d\t%d
		wxString buffer = wxString::Format(wxT("%d\t%d\t%d\t%d"), 
			theApp.glob_prefs->GetTrafficOMeterInterval(),
			theApp.glob_prefs->GetMaxGraphDownloadRate(),
			theApp.glob_prefs->GetMaxGraphUploadRate(),
			theApp.glob_prefs->GetMaxConnections());
		
		return buffer;
	}
	if (item == wxT("WEBPAGE STATISTICS")) {
		int filecount = theApp.downloadqueue->GetFileCount();
		uint32 stats[2]; // get the source count
		theApp.downloadqueue->GetDownloadStats(stats);
		wxString buffer = theApp.amuledlg->statisticswnd->GetHTML() +
			wxString::Format(wxT(
			"\tStatistics: \n"
			"\t\tDownloading files: %d\n"
			"\t\tFound sources: %d\n"
			"\t\tActive downloads: %d\n"
			"\t\tActive Uploads: %d\n"
			"\t\tUsers on upload queue: %d"),
			filecount, stats[0], stats[1], 
			theApp.uploadqueue->GetUploadQueueLength(), 
			theApp.uploadqueue->GetWaitingUserCount());
		
		return buffer;
	}
	if (item == wxT("WEBPAGE GETPREFERENCES")) {
		// returns one string formatted as:
		// %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d
		wxString buffer(wxEmptyString);
		theApp.glob_prefs->GetWebUseGzip() ?
			buffer += wxT("1\t") : buffer += wxT("0\t");
		theApp.glob_prefs->GetPreviewPrio() ?
			buffer += wxT("1\t") : buffer += wxT("0\t");
		theApp.glob_prefs->TransferFullChunks() ?
			buffer += wxT("1\t") : buffer += wxT("0\t");
		buffer += wxString::Format(wxT("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d"),
			theApp.glob_prefs->GetWebPageRefresh(),
			theApp.glob_prefs->GetMaxSourcePerFile(),
			theApp.glob_prefs->GetMaxConnections(),
			theApp.glob_prefs->GetMaxConperFive(),
			theApp.glob_prefs->GetMaxDownload(),
			theApp.glob_prefs->GetMaxUpload(),
			theApp.glob_prefs->GetMaxGraphDownloadRate(),
			theApp.glob_prefs->GetMaxGraphUploadRate());
		
		return buffer;
	}
	sOp = wxT("WEBPAGE SETPREFERENCES");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if (item.Length() > nChars) {
			wxString prefList = item.Mid(nChars+1);
			int brk = prefList.First(wxT("\t"));
				
			theApp.glob_prefs->SetWebUseGzip( StrToLong(prefList.Left(brk)) == 1 );
			prefList = prefList.Mid(brk+1); brk = prefList.First(wxT("\t"));
			theApp.glob_prefs->SetWebPageRefresh( StrToLong(prefList.Left(brk)) );
			prefList = prefList.Mid(brk+1); brk = prefList.First(wxT("\t"));
			theApp.glob_prefs->SetMaxDownload( StrToLong(prefList.Left(brk)) );
			prefList = prefList.Mid(brk+1); brk = prefList.First(wxT("\t"));
			theApp.glob_prefs->SetMaxUpload( StrToLong(prefList.Left(brk)) );
			prefList = prefList.Mid(brk+1); brk = prefList.First(wxT("\t"));
			
			if ((int32)StrToLong(prefList.Left(brk)) != theApp.glob_prefs->GetMaxGraphDownloadRate()) {
				theApp.amuledlg->statisticswnd->SetARange(true, theApp.glob_prefs->GetMaxGraphDownloadRate());
			}
			theApp.glob_prefs->SetMaxGraphDownloadRate( StrToLong(prefList.Left(brk)) );
			prefList = prefList.Mid(brk+1); brk = prefList.First(wxT("\t"));
			
			if ((int32)StrToLong(prefList.Left(brk)) != theApp.glob_prefs->GetMaxGraphUploadRate()) {
				theApp.amuledlg->statisticswnd->SetARange(false, theApp.glob_prefs->GetMaxGraphUploadRate());
			}
			theApp.glob_prefs->SetMaxGraphUploadRate( StrToLong(prefList.Left(brk)) );
			prefList=prefList.Mid(brk+1); brk=prefList.First(wxT("\t"));
				
			theApp.glob_prefs->SetMaxSourcesPerFile( StrToLong(prefList.Left(brk)) );
			prefList=prefList.Mid(brk+1); brk=prefList.First(wxT("\t"));
			theApp.glob_prefs->SetMaxConnections( StrToLong(prefList.Left(brk)) );
			prefList=prefList.Mid(brk+1); brk=prefList.First(wxT("\t"));
			theApp.glob_prefs->SetMaxConsPerFive( StrToLong(prefList.Left(brk)) );
			prefList=prefList.Mid(brk+1); brk=prefList.First(wxT("\t"));
			theApp.glob_prefs->SetTransferFullChunks( StrToLong(prefList.Left(brk)) );
			prefList=prefList.Mid(brk+1);
			theApp.glob_prefs->SetPreviewPrio( StrToLong(prefList) == 1 );
		}
		return wxEmptyString;
	}
	sOp = wxT("WEBPAGE PROGRESSBAR");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		wxString buffer;			
		if (item.Length() > nChars+1) {
			int idx = item.Mid(nChars+1).Find(wxT(" "));
			uint16 progressbarWidth = StrToLong(item.Mid(nChars+1,idx));
			wxString filehash = item.Mid(nChars+1+idx+1);
			
			uchar fileid[16];
			DecodeBase16(unicode2char(filehash),filehash.Length(),fileid);
			CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
			if (cur_file && cur_file->IsPartFile()) {
				buffer.Append(wxT("0\t"));
				buffer+=cur_file->GetProgressString(progressbarWidth)+wxString(wxT("\t"));
				buffer+=wxString::Format(wxT("%f\t"), cur_file->GetPercentCompleted());
			} else {
				buffer.Append(wxT("1\t"));
			}
		}
		
		return buffer;
	}
	//---------------------------------------------------------------------
	// PREFS
	//---------------------------------------------------------------------
	if (item == wxT("PREFS GETWEBUSEGZIP")) {
		wxString buffer;
		if (theApp.glob_prefs->GetWebUseGzip()) {
			buffer += wxT("1");
		} else {
			buffer += wxT("0");
		}
		return buffer;
	}
	sOp = wxT("PREFS GETWSPASS");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		wxString pwdHash = item.Mid(nChars+1);
		if (pwdHash == theApp.glob_prefs->GetWSPass()) {
			AddLogLineM(false, _("Webserver-Admin-Login"));
			return wxT("AdminLogin");
		} else if (
		   theApp.glob_prefs->GetWSIsLowUserEnabled() && 
		   !theApp.glob_prefs->GetWSLowPass().IsEmpty() && 
		   pwdHash == theApp.glob_prefs->GetWSLowPass()) {
			AddLogLineM(false, _("Webserver-Guest-Login"));
			return wxT("GuestLogin");
		} else {
			AddLogLineM(false, _("Webserver: Failed Loginattempt"));
			return wxT("Access Denied");
		}
	}
	//---------------------------------------------------------------------
	// SERVER
	//---------------------------------------------------------------------
		if (item == wxT("SERVER RE-CONNECT")) {
			if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting()) {
				theApp.serverconnect->Disconnect();
			}
			theApp.serverconnect->ConnectToAnyServer();
			Notify_ShowConnState(false,wxT(""));
			return wxEmptyString;
		}
		if (item == wxT("SERVER DISCONNECT")) {
			if (theApp.serverconnect->IsConnected() || theApp.serverconnect->IsConnecting()) {
				theApp.serverconnect->Disconnect();
			}
			return wxEmptyString;
		}
		if (item == wxT("SERVER LIST")) {
			// returns one string where each line is formatted 
			// as: %s\t%s\t%d\t%s\t%d\t%d\t%d\n
			wxString buffer(wxT(""));
			for (uint i=0; i < theApp.serverlist->GetServerCount(); i++) {
				CServer *server = theApp.serverlist->GetServerAt(i);
				if (server) {
					buffer.Append(
						server->GetListName() + wxT("\t") +
						server->GetDescription() + wxT("\t") +
						wxString::Format(wxT("%i"),server->GetPort()) + wxT("\t") +
						server->GetAddress() + wxT("\t") +
						wxString::Format(wxT("%i"),server->GetUsers()) + wxT("\t") +
						wxString::Format(wxT("%i"),server->GetMaxUsers()) + wxT("\t") +
						wxString::Format(wxT("%i"),server->GetFiles()) + wxT("\n")
					);
				}
			}
			return buffer;
		}
		if (item.Left(10) == wxT("SERVER ADD")) {
			if (item.Length() > 10) {
				int idx1 = item.Mid(11).Find(wxT(" "));
				wxString sIP = item.Mid(11, idx1);
				int idx2= item.Mid(11+idx1+1).Find(wxT(" "));
				wxString sPort = item.Mid(11+idx1+1,idx2);
				wxString sName = item.Mid(11+idx1+idx2+2);
					
				CServer *nsrv = new CServer( StrToLong(sPort), sIP );
				nsrv->SetListName(sName);
				theApp.AddServer(nsrv);
			}
			return wxEmptyString;
		}
		if (item.Left(16) == wxT("SERVER UPDATEMET")) {
			if (item.Length() > 16) {
				theApp.amuledlg->serverwnd->UpdateServerMetFromURL(item.Mid(17));
			}
			return wxEmptyString;
		}
		if (item == wxT("SERVER STAT")) {
			// returns one string where each line is formatted 
			// as: %s or as: %s\t%s\t%s\t%ld
			wxString buffer;			
			if (theApp.serverconnect->IsConnected()) {
				buffer.Append(wxT("Connected\t"));
				theApp.serverconnect->IsLowID() ? buffer.Append(wxT("Low ID\t")) : buffer.Append(wxT("High ID\t"));
				buffer+=theApp.serverconnect->GetCurrentServer()->GetListName() + wxT("\t");
				buffer+=wxString::Format(wxT("%ld"), (long)theApp.serverconnect->GetCurrentServer()->GetUsers());
			} else if (theApp.serverconnect->IsConnecting()) {
				buffer.Append(wxT("Connecting\t"));
			} else {
				buffer.Append(wxT("Disconnected\t"));
			}			
			return buffer;
		}
	//---------------------------------------------------------------------
	// TRANSFER
	//---------------------------------------------------------------------
	if (item == wxT("TRANSFER CLEARCOMPLETE")) {
// lfroen - daemon doesn't need it
#ifndef AMULE_DAEMON
		theApp.amuledlg->transferwnd->downloadlistctrl->ClearCompleted();
#endif
		return wxEmptyString;
	}	
	sOp = wxT("TRANSFER ADDFILELINK");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		wxString buffer= wxT("Bad Link");
		if (item.Length() > nChars) {
			try {
				CED2KLink *pLink = CED2KLink::CreateLinkFromUrl(unicode2char(item.Mid(nChars+1)));
				if (pLink->GetKind() == CED2KLink::kFile) {
					// lfroen - using default category 0
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), 0); 
					buffer = wxT("Link Added");
				}
			} catch (...) {
				buffer = wxT("Bad Link!");
			}
		}
		return buffer;
	}
	if (item == wxT("TRANSFER DL_FILEHASH")) {
		// returns one string where each line is formatted as:
		// %s\t%s\t...\t%s
		wxString buffer;
		unsigned int n = theApp.downloadqueue->GetFileCount();
		for (unsigned int i = 0; i < n; ++i) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
			if (cur_file) {
				buffer += cur_file->GetFileHash().Encode();
				if ( (i + 1) != n )
					buffer += wxT("\t");
			}
		}
		return buffer;
	}
	sOp = wxT("TRANSFER DL_FILEPAUSE");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if ((item.Length() > nChars) && (item.Mid(nChars+1).IsNumber())) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(nChars+1)) );
			if (cur_file) {
				cur_file->PauseFile();
			}
		}
		return wxEmptyString;
	}
	sOp = wxT("TRANSFER DL_FILERESUME");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if ((item.Length() > nChars) && (item.Mid(nChars+1).IsNumber())) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(nChars+1)) );
			if (cur_file) {
				cur_file->ResumeFile();
			}
		}
		return wxEmptyString;
	}
	sOp = wxT("TRANSFER DL_FILEDELETE");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if ((item.Length() > nChars) && (item.Mid(nChars+1).IsNumber())) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(nChars+1)) );
			if (cur_file) {
				cur_file->Delete();
			}
		}
		return wxEmptyString;
	}
	sOp = wxT("TRANSFER DL_FILEPRIOUP");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if ((item.Length() > nChars) && (item.Mid(nChars+1).IsNumber())) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(nChars+1)) );
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
		return wxEmptyString;
	}
	sOp = wxT("TRANSFER DL_FILEPRIODOWN");
	nChars = sOp.Length();
	if (item.Left(nChars) == sOp) {
		if ((item.Length() > nChars) && (item.Mid(nChars+1).IsNumber())) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(nChars+1)) );
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
		return wxEmptyString;
	}
	if (item == wxT("TRANSFER DL_LIST")) {
		// returns one string where each line is formatted as:
		// %s\t%ul\t%ul\t%ul\t%f\t%li\t%u\t%s\t%u\t%s\t%u\t%u\t%u\t%s\t%s\t%d\n
		// 16 fields
		wxString buffer;
		wxString tempFileInfo;
		unsigned int i;
		for (i = 0; i < theApp.downloadqueue->GetFileCount(); ++i) {
			CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
			if (cur_file) {
				tempFileInfo = GetDownloadFileInfo(cur_file);
				tempFileInfo.Replace(wxT("\n"), wxT("|"));
				buffer +=
					cur_file->GetFileName() +
					wxString::Format(wxT("\t%ul\t%ul\t%ul\t%f\t%li\t%u\t"),
						cur_file->GetFileSize(),
						cur_file->GetCompletedSize(),
						cur_file->GetTransfered(),
						cur_file->GetPercentCompleted(),
						(long)(cur_file->GetKBpsDown()*1024),
						cur_file->GetStatus() 
					) +
					cur_file->getPartfileStatus() +
					wxString::Format(wxT("\t%u\t"),
						cur_file->IsAutoDownPriority() ? 
							cur_file->GetDownPriority() + 10 :
							cur_file->GetDownPriority()
					) +
					EncodeBase16(cur_file->GetFileHash(), 16) +
					wxString::Format(wxT("\t%u\t%u\t%u\t"),
						cur_file->GetSourceCount(),
						cur_file->GetNotCurrentSourcesCount(),
						cur_file->GetTransferingSrcCount()
					) +
					( ( theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() ) ?
						theApp.CreateED2kSourceLink(cur_file) :
						theApp.CreateED2kLink(cur_file) ) +
					wxT("\t") + tempFileInfo + wxT("\t") +
					( (!cur_file->IsPartFile()) ?
						wxT("1\n") : wxT("0\n") );
			}
		}
		return buffer;
	}
	if (item == wxT("TRANSFER UL_LIST")) {
		// returns one string where each line is formatted as:
		// %s\t%s\t%s\t%ul\t%ul\t%li\n"
		wxString buffer;
		wxString tempFileInfo;
		for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
			 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
			CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
			if (cur_client) {
				buffer += cur_client->GetUserName() + wxT("\t");
				tempFileInfo = cur_client->GetUploadFileInfo();
				tempFileInfo.Replace(wxT("\n"), wxT("|"));
				buffer += wxString::Format(wxT("%s\t"), tempFileInfo.GetData());
				CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
				if (file) {
					buffer+=wxString::Format(wxT("%s\t"), file->GetFileName().GetData());
				} else {
					buffer+=wxString(wxT("?\t"));
				}
				buffer+=wxString::Format(wxT("%ul\t"), cur_client->GetTransferedDown());
				buffer+=wxString::Format(wxT("%ul\t"), cur_client->GetTransferedUp());
				buffer+=wxString::Format(wxT("%li\n"), (long)(cur_client->GetKBpsUp()*1024.0));
			}
		}
		return buffer;
	}
	if (item == wxT("TRANSFER W_LIST")) {
		// returns one string where each line is formatted as:
		// %s\n 
		// or as:
		// %s\t%s\t%d\t%d\n"
		wxString buffer;
		for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList();
			pos != 0; theApp.uploadqueue->GetNextFromWaitingList(pos)) {
			CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
			if (cur_client) {
				buffer += cur_client->GetUserName();
				if (cur_client->GetRequestFile()) {
					buffer += wxT("\t");
				} else {
					buffer += wxT("\n");
					continue;
				}					
				CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetRequestFile()->GetFileHash());
				buffer +=
					file ? cur_client->GetDownloadFile()->GetFileName() : wxT("?") +
					wxString::Format(wxT("\t%d\t"), cur_client->GetScore(false)) +
					( cur_client->IsBanned() ? wxT("1\n") : wxT("0\n") );
			}
		}
		return buffer;
	}
	//
	// SHAREDFILES
	//
	if (item == wxT("SHAREDFILES RELOAD")) {
		theApp.sharedfiles->Reload(true, false);
		return wxEmptyString;
	}
	if (item == wxT("SHAREDFILES LIST")) {
		// returns one string where each line is formatted as:
		// %s\t%ld\t%s\t%ld\t%ll\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n
		wxString buffer;
		for (int i = 0; i < theApp.sharedfiles->GetCount(); ++i) {
			const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex(i);
			if (cur_file) {
				buffer += cur_file->GetFileName() + wxT("\t") << 
					(long)cur_file->GetFileSize() << wxT("\t") +
					( theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() ?
						theApp.CreateED2kSourceLink(cur_file) :
						theApp.CreateED2kLink(cur_file)	) + wxT("\t") +
						wxString::Format(wxT("%ld\t%ld\t%d\t%d\t%d\t%d\t"),
							(long)cur_file->statistic.GetTransfered(),
							(long)cur_file->statistic.GetAllTimeTransfered(),
							cur_file->statistic.GetRequests(),
							cur_file->statistic.GetAllTimeRequests(),
							cur_file->statistic.GetAccepts(),
							cur_file->statistic.GetAllTimeAccepts() ) +
						EncodeBase16(cur_file->GetFileHash(), 16) + wxT("\t");					
				int prio = cur_file->GetUpPriority();
				if (cur_file->IsAutoUpPriority()) {
					switch (prio) {
						case PR_LOW: 
							buffer.Append(wxT("Auto [Lo]")); break;
						case PR_NORMAL:
							buffer.Append(wxT("Auto [No]")); break;
						case PR_HIGH:
							buffer.Append(wxT("Auto [Hi]")); break;
						case PR_VERYHIGH:
							buffer.Append(wxT("Auto [Re]")); break;
						default:
							buffer.Append(wxT("-")); break;
					}
				} else {
					switch (prio) {
						case PR_VERYLOW:
							buffer.Append(wxT("Very Low")); break;
						case PR_LOW:
							buffer.Append(wxT("Low")); break;
						case PR_NORMAL:
							buffer.Append(wxT("Normal")); break;
						case PR_HIGH:
							buffer.Append(wxT("High")); break;
						case PR_VERYHIGH:
							buffer.Append(wxT("Very High")); break;
						case PR_POWERSHARE:
							buffer.Append(wxT("PowerShare[Release]")); break;
						default:
							buffer.Append(wxT("-")); break;
					}
				}
				buffer +=
					wxString::Format(wxT("\t%d\t"), prio) +
					( cur_file->IsAutoUpPriority() ? wxT("1\n") : wxT("0\n") );
			}
		}
		return buffer;
	}
/**********************************************/
/* Must still do a check from here to the end */
/**********************************************/
		//
		// LOG
		//
		if (item == wxT("LOG RESETLOG")) {
			theApp.GetLog(true);
			return wxEmptyString;
		}
		if (item == wxT("LOG GETALLLOGENTRIES")) {
			return theApp.GetLog();
		}
		if (item == wxT("LOG CLEARSERVERINFO")) {
			theApp.GetServerLog(true);
			return wxEmptyString;
		}
		if (item == wxT("LOG GETSERVERINFO")) {
			return theApp.GetServerLog();
		}
		if (item == wxT("LOG RESETDEBUGLOG")) {
			theApp.GetDebugLog(true);
			return wxEmptyString;
		}
		if (item == wxT("LOG GETALLDEBUGLOGENTRIES")) {
			return theApp.GetDebugLog();
		}
		if (item.Left(14) == wxT("LOG ADDLOGLINE")) {
			if (item.Length() > 15) {
				AddLogLineM(false, item.Mid(15));
			}
			return wxEmptyString;
		}
		
		//CATEGORIES
		if (item == wxT("CATEGORIES GETCATCOUNT")) {
			wxString buffer = wxString::Format(wxT("%d"), theApp.glob_prefs->GetCatCount());
			return buffer;
		}
		
		if (item.Left(22) == wxT("CATEGORIES GETCATTITLE")) {
			if (item.Length() > 22) {
				int catIndex = StrToLong(item.Mid(23));
				return theApp.glob_prefs->GetCategory(catIndex)->title;
			}
		}
		
		if (item.Left(22) == wxT("CATEGORIES GETCATEGORY")) {
			if (item.Length() > 22) {
				wxString fileHash = item.Mid(23);
				uchar fileid[16];
				DecodeBase16(unicode2char(fileHash), fileHash.Length(), fileid);				
				CPartFile *cur_file = theApp.downloadqueue->GetFileByID(fileid);
				if (cur_file) {
					// we've found the file
					wxString buffer = wxString::Format(wxT("%d"), cur_file->GetCategory());
					return buffer;
				}
				return wxT("0");
			}
		}
		
		if (item.Left(22) == wxT("CATEGORIES GETFILEINFO")) {
			if (item.Length() > 22) {
				wxString fileHash = item.Mid(23);
				uchar fileid[16];
				DecodeBase16(unicode2char(fileHash), fileHash.Length(), fileid);
				CPartFile *cur_file = theApp.downloadqueue->GetFileByID(fileid);
				if (cur_file) {
					// we've found the file
					//buffer:
					//int\tint\tint\twxstring\twxstring\twxstring\twxstring
					wxString buffer = wxString::Format(wxT("%d\t%d\t2\t"),
						cur_file->GetStatus(),
						cur_file->GetTransferingSrcCount()
						) +
					// Needed because GetFileType() no longer exists (and returned always returned 2 anyway)
					// buffer.Append(wxString::Format("%d\t", cur_file->GetFileType()));
					// buffer.Append(wxString::Format(wxT("%d\t"), 2));
						( cur_file->IsPartFile() ? wxT("Is PartFile\t") : wxT("Is Not PartFile\t") ) +
						( cur_file->IsStopped()  ? wxT("Is Stopped\t")  : wxT("Is Not Stopped\t")  ) +
						( GetFiletype(cur_file->GetFileName()) == ftVideo ? 
							wxT("Is Movie\t") : wxT("Is Not Movie\t") ) +
						( GetFiletype(cur_file->GetFileName()) == ftArchive ?
							wxT("Is Archive") : wxT("Is Not Archive") );
					return buffer;
				}
			}
			return wxEmptyString;
		}
		
		//SEARCH
		if (item.Left(19) == wxT("SEARCH DOWNLOADFILE")) {
			if (item.Length() > 19) {
				uchar fileid[16];
				DecodeBase16(unicode2char(item.Mid(20)),item.Mid(20).Length(),fileid);
				theApp.searchlist->AddFileToDownloadByHash(fileid);
			}
			return wxEmptyString;
		}
		if (item.Left(18) == wxT("SEARCH DONEWSEARCH")) {
			if (item.Length() > 18) {
				//int curIndex, nextIndex;
				wxString sParams = item.Mid(19);
				int brk = sParams.First(wxT("\n"));
				
				wxString searchString = sParams.Left(brk);
				searchString.Trim(true);
				searchString.Trim(false);
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				wxString typeText = sParams.Left(brk);
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
					
				uint32 min = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				uint32 max = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
					
				uint32 avaibility = StrToLong(sParams.Left(brk));
				sParams=sParams.Mid(brk+1); brk=sParams.First(wxT("\n"));
				
				wxString extension = sParams.Left(brk);
				sParams=sParams.Mid(brk+1);
				
				bool globalsearch = (sParams == wxT("global")) ? true : false;
					
				theApp.searchlist->RemoveResults(0xffff);
				theApp.searchlist->NewSearch(typeText, 0xffff);				
				Packet *packet = CreateSearchPacket(searchString, typeText, extension, min, max, avaibility);
				
				// this is internal core call, but macro is useful anyway
				CoreNotify_Search_Req(packet, globalsearch);
			}
		}
		if (item.Left(14) == wxT("SEARCH WEBLIST")) {
			if (item.Length() > 14) {
				wxString sItem = item.Mid(15);
				int brk=sItem.First(wxT("\t"));
				wxString sResultLine=sItem.Left(brk);
				sItem=sItem.Mid(brk+1); brk=sItem.First(wxT("\t"));
				int sortBy = StrToLong(sItem.Left(brk));
				bool searchAsc = StrToLong(sItem.Mid(brk+1));
				return(theApp.searchlist->GetWebList(sResultLine, sortBy, searchAsc));
			}
		}
#ifndef AMULE_DAEMON
		// lfroen - can't do it with daemon
		//Special command :)
		if (item == wxT("AMULEDLG SHOW")) {
			theApp.amuledlg->Show_aMule(true);
			return wxEmptyString;
		}
#endif
				
		//shakraw - TODO - amulecmd requests
		
		
		//
		//
		//shakraw - old requests code (to remove when all new code is done)
		//
		//
		if (item == wxT("STATS")) {
			unsigned int filecount = theApp.downloadqueue->GetFileCount();
			// get the source count
			uint32 stats[2];
			theApp.downloadqueue->GetDownloadStats(stats);
			return wxString::Format( wxT(
				"Statistics: \n"
				" Downloading files: %u\n"
				" Found sources: %d\n"
				" Active downloads: %d\n"
				" Active Uploads: %d\n"
				" Users on upload queue: %d"), 
					filecount, stats[0], stats[1],
					theApp.uploadqueue->GetUploadQueueLength(),
					theApp.uploadqueue->GetWaitingUserCount());
		}

		if (item == wxT("DL_QUEUE")) {
			return theApp.downloadqueue->getTextList();
		}

		if (item == wxT("UL_QUEUE")) {
			return wxT("We should be showing UL list here");
		}
		
		if (item == wxT("CONNSTAT")) {
			if (theApp.serverconnect->IsConnected()) {
				return wxT("Connected");
			//Start - Added by shakraw
			} else if (theApp.serverconnect->IsConnecting()) {
				return wxT("Connecting");
			//End
			} else {
				return wxT("Not Connected");
			}
		}

		if (item == wxT("RECONN")) { //shakraw, should be replaced by SERVER CONNECT [ip] below?
			if (theApp.serverconnect->IsConnected()) {
				return wxT("Already Connected");
			} else {
				theApp.serverconnect->ConnectToAnyServer();
				Notify_ShowConnState(false,wxT(""));
				return wxT("Reconected");
			}
		}

		if (item == wxT("DISCONN")) {
			if (theApp.serverconnect->IsConnected()) {
				theApp.serverconnect->Disconnect();
				theApp.OnlineSig(); // Added By Bouc7
				return wxT("Disconnected");
			} else {
				return wxT("Already conected");
			}
		}
		
		if (item == wxT("RELOADIPF")) {
			theApp.ipfilter->Reload();
			return wxT("IPFilter Reloaded");
		}
		
		if (item.Left(12) == wxT("SET IPFILTER")) {
			wxString param = item.Mid(13).Strip(wxString::both).MakeLower();
			theApp.glob_prefs->SetIPFilterOn(param == wxT("on"));
			if (theApp.glob_prefs->GetIPFilterOn()) {
				theApp.clientlist->FilterQueues();
			}
			wxString msg = wxString::Format(_("IPFilter state set to '%s'."), unicode2char(param));
			AddLogLineM(true, msg);
			return msg;
		}

		if (item == wxT("GETIPLEVEL") ) {
			wxString msg = wxString::Format(_("aMule IP Filter level is %d."), theApp.glob_prefs->GetIPFilterLevel());
			AddLogLineM(true, msg);
			return msg;
		}
		
		if (item.Left(10) == wxT("SETIPLEVEL") ) {
			wxString args = item.Mid(11);
			int32 level = StrToLong(args);
			int32 oldLevel = theApp.glob_prefs->GetIPFilterLevel();
			wxString msg;
			if ( level <= 255 ) {
				if ( level != oldLevel ) {
					theApp.glob_prefs->SetIPFilterLevel(level);
					msg = wxString::Format(_("aMule IP Filter level changed from %d to %d."), oldLevel, level);
				} else {
					msg = wxString::Format(_("aMule IP Filter level was already %d. Nothing changed."), level);
				}
			} else {
				msg = _("Invalid IP Filter level entered: ") + args + wxT(".");
			}
			AddLogLineM(true, msg);
			return msg;
		}
				
		if ( item.Left(5).Cmp(wxT("PAUSE")) == 0 ) {
			if ( item.Mid(5).IsNumber() ) {
				unsigned int fileID = StrToLong(item.Mid(5));
				if ( fileID < theApp.downloadqueue->GetFileCount() ) {
					if (theApp.downloadqueue->GetFileByIndex(fileID)->IsPartFile()) {
						theApp.downloadqueue->GetFileByIndex(fileID)->PauseFile();
						printf("Paused\n");
						return theApp.downloadqueue->getTextList();
					} else return wxT("Not part file");
				} else return wxT("Out of range");
			} else return wxT("Not a number");
		} 
		
		if ( item.Left(6).Cmp(wxT("RESUME")) == 0 ) {
			if (item.Mid(6).IsNumber()) {
				unsigned int fileID = StrToLong(item.Mid(6));
				if ( fileID < theApp.downloadqueue->GetFileCount() ) {
					if (theApp.downloadqueue->GetFileByIndex(fileID)->IsPartFile()) {
						theApp.downloadqueue->GetFileByIndex(fileID)->ResumeFile();
						theApp.downloadqueue->GetFileByIndex(fileID)->SavePartFile();
						printf("Resumed\n");
						return theApp.downloadqueue->getTextList();
					} else return wxT("Not part file");
				} else return wxT("Out of range");				
			} else return wxT("Not a number");
		} 
		
		//shakraw, amuleweb protocol communication start
		// PREFERENCES
		if (item.Left(11).Cmp(wxT("PREFERENCES")) == 0) {
			if (item.Mid(12).Cmp(wxT("GETWSPORT")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetWSPort() );
			}
			
			if (item.Mid(12,9).Cmp(wxT("GETWSPASS")) == 0) {
				wxString pwdHash = item.Mid(22);
				if (pwdHash == theApp.glob_prefs->GetWSPass())
					return wxT("Admin Session");
				else if (theApp.glob_prefs->GetWSIsLowUserEnabled() && 
						!theApp.glob_prefs->GetWSLowPass().IsEmpty() && 
						pwdHash == theApp.glob_prefs->GetWSLowPass())
					return wxT("LowUser Session");
				else
					return wxT("Access Denied");
			}

			if (item.Mid(12,17).Cmp(wxT("SETWEBPAGEREFRESH")) == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					theApp.glob_prefs->SetWebPageRefresh( StrToLong(item.Mid(30)));
					return wxT("WebPageRefresh Saved");
				}
				return wxT("Bad SETWEBPAGEREFRESH request");
			}

			if (item.Mid(12).Cmp(wxT("GETWEBPAGEREFRESH")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetWebPageRefresh() );
			}
						
			if (item.Mid(12,13).Cmp(wxT("SETWEBUSEGZIP")) == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					theApp.glob_prefs->SetWebUseGzip( StrToLong(item.Mid(26)));
					return wxT("WebUseGzip Saved");
				}
				return wxT("Bad SETWEBUSEGZIP request");
			}
			
			if (item.Mid(12,14).Cmp(wxT("SETMAXDOWNLOAD")) == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					theApp.glob_prefs->SetMaxDownload( StrToLong(item.Mid(27)) );
					return wxT("MaxDownload Saved");
				}
				return wxT("Bad SETMAXDOWNLOAD request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXDOWNLOAD")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxDownload() );
			}

			if (item.Mid(12,12).Cmp(wxT("SETMAXUPLOAD")) == 0) {
				if ((item.Length() > 24) && item.Mid(25).IsNumber()) {
					theApp.glob_prefs->SetMaxUpload( StrToLong(item.Mid(25)) );
					return wxT("MaxUpload Saved");
				}
				return wxT("Bad SETMAXUPLOAD request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXUPLOAD")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxUpload() );
			}
			
			if (item.Mid(12,23).Cmp(wxT("SETMAXGRAPHDOWNLOADRATE")) == 0) {
				if ((item.Length() > 35) && item.Mid(36).IsNumber()) {
					theApp.glob_prefs->SetMaxGraphDownloadRate( StrToLong(item.Mid(36)) );
					return wxT("MaxGraphDownload Saved");
				}
				return wxT("Bad SETMAXGRAPHDOWNLOADRATE request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXGRAPHDOWNLOADRATE")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxGraphDownloadRate() );
			}


			if (item.Mid(12,21).Cmp(wxT("SETMAXGRAPHUPLOADRATE")) == 0) {
				if ((item.Length() > 33) && item.Mid(34).IsNumber()) {
					theApp.glob_prefs->SetMaxGraphUploadRate( StrToLong(item.Mid(34)) );
					return wxT("MaxGraphUpload Saved");
				}
				return wxT("Bad SETMAXGRAPHUPLOADRATE request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXGRAPHUPLOADRATE")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxGraphUploadRate() );
			}

			if (item.Mid(12,20).Cmp(wxT("SETMAXSOURCESPERFILE")) == 0) {
				if ((item.Length() > 32) && item.Mid(33).IsNumber()) {
					theApp.glob_prefs->SetMaxSourcesPerFile( StrToLong(item.Mid(33)) );
					return wxT("MaxSourcesPerFile Saved");
				}
				return wxT("Bad SETMAXSOURCESPERFILE request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXSOURCEPERFILE")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxSourcePerFile() );
			}

			if (item.Mid(12,17).Cmp(wxT("SETMAXCONNECTIONS")) == 0) {
				if ((item.Length() > 29) && item.Mid(29).IsNumber()) {
					theApp.glob_prefs->SetMaxConnections( StrToLong(item.Mid(30)) );
					return wxT("MaxConnections Saved");
				}
				return wxT("Bad SETMAXCONNECTIONS request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXCONNECTIONS")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxConnections() );
			}

			if (item.Mid(12,17).Cmp(wxT("SETMAXCONSPERFIVE")) == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					theApp.glob_prefs->SetMaxConsPerFive( StrToLong(item.Mid(30)) );
					return wxT("MaxConsPerFive Saved");
				}
				return wxT("Bad SETMAXCONSPERFIVE request");
			}

			if (item.Mid(12).Cmp(wxT("GETMAXCONPERFIVE")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetMaxConperFive() );
			}

			if (item.Mid(12,21).Cmp(wxT("SETTRANSFERFULLCHUNKS")) == 0) {
				if ((item.Length() > 33) && item.Mid(34).IsNumber()) {
					bool flag = StrToLong(item.Mid(34));
					theApp.glob_prefs->SetTransferFullChunks(flag);
					return wxT("TransferFullChunks Saved");
				}
				return wxT("Bad SETTRANSFERFULLCHUNKS request");
			}

			if (item.Mid(12).Cmp(wxT("GETTRANSFERFULLCHUNKS")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->TransferFullChunks() );
			}
			
			if (item.Mid(12,14).Cmp(wxT("SETPREVIEWPRIO")) == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					theApp.glob_prefs->SetTransferFullChunks( StrToLong(item.Mid(27)) );
					return wxT("PreviewPrio Saved");
				}
				return wxT("Bad SETPREVIEWPRIO request");
			}

			if (item.Mid(12).Cmp(wxT("GETPREVIEWPRIO")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetPreviewPrio() );
			}

			return wxT("Bad PREFERENCES request");
		} //end - PREFERENCES
		
		
		// LOGGING
		if (item.Left(7).Cmp(wxT("LOGGING")) == 0) {
			if (item.Mid(8, 10).Cmp(wxT("ADDLOGLINE")) == 0) {
				if (item.Length() > 18) {
					bool flag;
					int separator = item.Mid(19).Find(wxT(" "));
					if (item.Mid(19, separator).IsNumber()) {
						flag = StrToLong(item.Mid(19, separator));
						AddLogLineM(flag, wxString::Format(wxT("%s"), (item.Mid(19+separator+1)).GetData()));
						return wxT("Line Logged");
					}
				}
				return wxT("Bad ADDLOGLINE request");
			}
			
			if (item.Mid(8).Cmp(wxT("RESETLOG")) == 0) {
				theApp.GetLog(true);
				return wxT("Log Cleared");
			}
			
			if (item.Mid(8).Cmp(wxT("RESETDEBUGLOG")) == 0) {
				theApp.GetDebugLog(true);
				return wxT("DebugLog Cleared");
			}
			
			if (item.Mid(8).Cmp(wxT("SERVERINFO CLEAR")) == 0) {
				theApp.GetServerLog(true);
				return wxT("ServerInfo Cleared");
			}

			if (item.Mid(8).Cmp(wxT("SERVERINFO GETTEXT")) == 0) {
				return theApp.GetServerLog();
			}
			
			if (item.Mid(8).Cmp(wxT("GETALLLOGENTRIES")) == 0) {
				return theApp.GetLog();
			}
			
			if (item.Mid(8).Cmp(wxT("GETALLDEBUGLOGENTRIES")) == 0) {
				//shakraw, the same as above, but please DO NOT remove!
				//I like to have separated requests for this (say for future use)
				//also see comments in WebServer.cpp
				return theApp.GetDebugLog();
			}

			return wxT("Bad LOGGING request");
		} // end - LOGGING
		
		
		// STATISTICS
		if (item.Left(10).Cmp(wxT("STATISTICS")) == 0) {
			if (item.Mid(11).Cmp(wxT("GETHTML")) == 0) {
				return theApp.amuledlg->statisticswnd->GetHTML();
			}
			
			if (item.Mid(11,11).Cmp(wxT("SETARANGEUL")) == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					bool flag = StrToLong(item.Mid(23));
					theApp.amuledlg->statisticswnd->SetARange(flag,theApp.glob_prefs->GetMaxGraphUploadRate());
					return wxT("SetARangeUL Saved");
				}
				return wxT("Bad SETARANGE request");
			}

			if (item.Mid(11,11).Cmp(wxT("SETARANGEDL")) == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					bool flag = StrToLong(item.Mid(23));
					theApp.amuledlg->statisticswnd->SetARange(flag,theApp.glob_prefs->GetMaxGraphDownloadRate());
					return wxT("SetARangeDL Saved");
				}
				return wxT("Bad SETARANGE request");
			}

			if (item.Mid(11).Cmp(wxT("GETTRAFFICOMETERINTERVAL")) == 0) {
				return wxString::Format( wxT("%d"), theApp.glob_prefs->GetTrafficOMeterInterval() );
			}
			
		} //end - STATISTICS
		
		
		// SHAREDFILES
		if (item.Left(11).Cmp(wxT("SHAREDFILES")) == 0) {
			if (item.Mid(12).Cmp(wxT("GETCOUNT")) == 0) {
				return wxString::Format( wxT("%d"), theApp.sharedfiles->GetCount() );
			}
			
			if (item.Mid(12,11).Cmp(wxT("GETFILENAME")) == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(24)) );
					if (cur_file) {
						return(cur_file->GetFileName());
					}
					return wxT("Bad file");
				}
				return wxT("Bad GETFILENAME request");
			}
			
			if (item.Mid(12,11).Cmp(wxT("GETFILESIZE")) == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(24)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->GetFileSize() );
					}
					return wxT("Bad file");
				}
				return wxT("Bad GETFILESIZE request");
			}

			if (item.Mid(12,16).Cmp(wxT("CREATEED2KSRCLNK")) == 0) {
				if ((item.Length() > 28) && item.Mid(29).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(29)) );
					if (cur_file) {
						return theApp.CreateED2kSourceLink(cur_file);
					}
					return wxT("Bad file");					
				}
				return wxT("Bad CREATEED2KSRCLNK request");
			}

			if (item.Mid(12,13).Cmp(wxT("CREATEED2KLNK")) == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(26)) );
					if (cur_file) {
						return theApp.CreateED2kLink(cur_file);
					}
					return wxT("Bad file");					
				}
				return wxT("Bad CREATEED2KLNK request");
			}
		
			if (item.Mid(12,13).Cmp(wxT("GETTRANSFERED")) == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(26)) );
					if (cur_file) {
						return wxString::Format( wxT("%lld"), cur_file->statistic.GetTransfered() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETTRANSFERED request");
			}
			
			if (item.Mid(12,20).Cmp(wxT("GETALLTIMETRANSFERED")) == 0) {
				if ((item.Length() > 32) && item.Mid(33).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(33)) );
					if (cur_file) {
						return wxString::Format( wxT("%lld"), cur_file->statistic.GetAllTimeTransfered() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETALLTIMETRANSFERED request");
			}

			if (item.Mid(12,11).Cmp(wxT("GETREQUESTS")) == 0) {
				if ((item.Length() > 23) && item.Mid(24).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(24)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->statistic.GetRequests() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETREQUESTS request");
			}
			
			if (item.Mid(12,18).Cmp(wxT("GETALLTIMEREQUESTS")) == 0) {
				if ((item.Length() > 31) && item.Mid(32).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(32)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->statistic.GetAllTimeRequests() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETALLTIMEREQUESTS request");
			}
			
			if (item.Mid(12,10).Cmp(wxT("GETACCEPTS")) == 0) {
				if ((item.Length() > 22) && item.Mid(23).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(23)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->statistic.GetAccepts() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETACCEPTS request");
			}

			if (item.Mid(12,17).Cmp(wxT("GETALLTIMEACCEPTS")) == 0) {
				if ((item.Length() > 29) && item.Mid(30).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(30)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->statistic.GetAllTimeAccepts() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETALLTIMEACCEPTS request");
			}

			if (item.Mid(12,14).Cmp(wxT("GETENCFILEHASH")) == 0) {
				if ((item.Length() > 26) && item.Mid(27).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(27)) );
					if (cur_file) {
						return cur_file->GetFileHash().Encode();
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETENCFILEHASH request");
			}

			if (item.Mid(12,16).Cmp(wxT("ISAUTOUPPRIORITY")) == 0) {
				if ((item.Length() > 28) && item.Mid(29).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(29)) );
					if (cur_file) {
						if (cur_file->IsAutoUpPriority())
							return wxT("Is AutoUp Priority");
						else
							return wxT("Is Not AutoUp Priority");
					}
					return wxT("Bad file");					
				}
				return wxT("Bad ISAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12,13).Cmp(wxT("GETUPPRIORITY")) == 0) {
				if ((item.Length() > 25) && item.Mid(26).IsNumber()) {
					const CKnownFile *cur_file = theApp.sharedfiles->GetFileByIndex( StrToLong(item.Mid(26)) );
					if (cur_file) {
						return wxString::Format( wxT("%d"), cur_file->GetUpPriority() );
					}
					return wxT("Bad file");					
				}
				return wxT("Bad GETUPPRIORITY request");
			}
			
			if (item.Mid(12,17).Cmp(wxT("SETAUTOUPPRIORITY")) == 0) {
				if (item.Length() > 29) {
					int separator = item.Mid(30).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(30,separator);
						if (item.Mid(30+separator+1).IsNumber()) {
							bool flag = StrToLong(item.Mid(30+separator+1));
						
							uchar fileid[16];
							DecodeBase16(unicode2char(hash),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetAutoUpPriority(flag);
								return wxT("AutoUpPriority Saved");
							}
							return wxT("Bad file");
						}
					}
				}
				return wxT("Bad SETAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12,13).Cmp(wxT("SETUPPRIORITY")) == 0) {
				if (item.Length() > 25) {
					int separator = item.Mid(26).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(26,separator);
						if (item.Mid(26+separator+1).IsNumber()) {
							int priority = StrToLong(item.Mid(26+separator+1));
						
							uchar fileid[16];
							DecodeBase16(unicode2char(hash),hash.Length(),fileid);
							CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
							if (cur_file) {
								cur_file->SetUpPriority(priority);
								return wxT("UpPriority Saved");
							}
							return wxT("Bad file");
						}
					}
				}
				return wxT("Bad SETUPPRIORITY request");
			}

			if (item.Mid(12,20).Cmp(wxT("UPDATEAUTOUPPRIORITY")) == 0) {
				if (item.Length() > 32) {
					int separator = item.Mid(33).Find(wxT(" "));
					if (separator!=-1) {
						wxString hash = item.Mid(33,separator);
						uchar fileid[16];
						DecodeBase16(unicode2char(hash),hash.Length(),fileid);
						CKnownFile* cur_file = theApp.sharedfiles->GetFileByID(fileid);
					
						if (cur_file) {
							cur_file->UpdateAutoUpPriority();
							return wxT("UpPriority Saved");
						}
						return wxT("Bad file");
					}
				}
				return wxT("Bad UPDATEAUTOUPPRIORITY request");
			}
			
			if (item.Mid(12).Cmp(wxT("RELOAD")) == 0) {
				theApp.sharedfiles->Reload(true, false);
				return wxT("SharedFiles Reloaded");
			}

			return wxT("Bad SHAREDFILES request");
		} //end - shared files
		
		
		// IDSTAT
		if (item.Left(6).Cmp(wxT("IDSTAT")) == 0) { //get id type
			if (theApp.serverconnect->IsLowID())
				return wxT("Low ID");
			else
				return wxT("High ID");
		}
		
		
		// GETCATCOUNT
		if (item.Left(11).Cmp(wxT("GETCATCOUNT")) == 0) { //get categories number
			return wxString::Format( wxT("%d"), theApp.glob_prefs->GetCatCount() );
		}
		
		
		// SPEED
		if (item.Left(5).Cmp(wxT("SPEED")) == 0) { //get upload/download datarate
			if (item.Mid(6).Cmp(wxT("UL")) == 0) { //upload
				return wxString::Format( wxT("%.1f"), theApp.uploadqueue->GetKBps() );	
			} else if (item.Mid(6).Cmp(wxT("DL")) == 0) { //download
				return wxString::Format( wxT("%.1f"), theApp.downloadqueue->GetKBps() );
			} else {
				return wxT("Wrong speed requested");
			}
		}
		
		
		// MAX
		if (item.Left(3).Cmp(wxT("MAX")) == 0) { //get max upload/download values
			if (item.Mid(4).Cmp(wxT("UL")) == 0) { //upload
				return wxString::Format( wxT("%i"), theApp.glob_prefs->GetMaxUpload() );
			} else if (item.Mid(4).Cmp(wxT("DL")) == 0) { //download
				return wxString::Format( wxT("%i"), theApp.glob_prefs->GetMaxDownload() );
			} else {
				return wxT("Wrong value requested");
			}
		}
		

		// SERVER		
		if (item.Left(6).Cmp(wxT("SERVER")) == 0) {
			if (item.Mid(7).Cmp(wxT("LIST")) == 0) {
				// shakraw - return a unique string where each line is formatted 
				// as: wxString\twxString\tint\twxString\tint\tint\tint\n
				wxString buffer;
				for (uint i=0; i < theApp.serverlist->GetServerCount(); i++) {
					CServer *server = theApp.serverlist->GetServerAt(i);
					if (server) {
						buffer +=
							server->GetListName() + wxT("\t") +
							server->GetDescription() + wxT("\t") +
							wxString::Format(wxT("%i"), server->GetPort()) + wxT("\t") +
							server->GetAddress() + wxT("\t") +
							wxString::Format(wxT("%i\t%i\t%i\n"),
								server->GetUsers(),
								server->GetMaxUsers(),
								server->GetFiles() );
					}
				}
				return buffer;
			}
			
			if (item.Mid(7).Cmp(wxT("COUNT")) == 0) { //get number of server in serverlist
				return wxString::Format( wxT("%i"), theApp.serverlist->GetServerCount() );
			}
			
			if (item.Mid(7,4).Cmp (wxT("NAME")) == 0) { // get server name
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(12)) );
					return(server->GetListName());
				} else if (theApp.serverconnect->IsConnected()) {
					return(theApp.serverconnect->GetCurrentServer()->GetListName());
				} else
					return wxEmptyString;
			}
			
			if (item.Mid(7,4).Cmp(wxT("DESC")) == 0) { // get the server description
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(12)) );
					return server->GetDescription();
				} else if (theApp.serverconnect->IsConnected())
					return theApp.serverconnect->GetCurrentServer()->GetDescription();
				else
					return wxEmptyString;
			}

			if (item.Mid(7,4).Cmp(wxT("PORT")) == 0) { // get the server port
				if ((item.Length() > 11) && (item.Mid(12).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(12)) );
					return wxString::Format( wxT("%i"), server->GetPort() );
				} else if (theApp.serverconnect->IsConnected())
					return wxString::Format( wxT("%i"), theApp.serverconnect->GetCurrentServer()->GetPort() );
				else 
					return wxT("0");
			}

			if (item.Mid(7,2).Cmp(wxT("IP")) == 0) { // get the server address
				if ((item.Length() > 9) && (item.Mid(10).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(10)) );
					return server->GetAddress();
				} else if (theApp.serverconnect->IsConnected())
					return theApp.serverconnect->GetCurrentServer()->GetAddress();
				else	
					return wxEmptyString;
			}
			
			if (item.Mid(7,5).Cmp(wxT("USERS")) == 0) { //get the number of users in the server we are connected to
				if ((item.Length() > 12) && (item.Mid(13).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(13)) );
					return wxString::Format( wxT("%i"), server->GetUsers() );
				} else if (theApp.serverconnect->IsConnected())
					return wxString::Format( wxT("%i"), theApp.serverconnect->GetCurrentServer()->GetUsers() );
				else 
					return wxEmptyString;
			}
			
			if (item.Mid(7,8).Cmp(wxT("MAXUSERS")) == 0) { // get the max number of users in a server
				if ((item.Length() > 12) && (item.Mid(16).IsNumber())) {
					CServer* cur_file = theApp.serverlist->GetServerAt( StrToLong(item.Mid(16)) );
					return wxString::Format( wxT("%i"), cur_file->GetMaxUsers() );
				} else if (theApp.serverconnect->IsConnected())
					return wxString::Format( wxT("%i"), theApp.serverconnect->GetCurrentServer()->GetMaxUsers() );
				else
				  return wxT("0");
			}

			if (item.Mid(7,5).Cmp(wxT("FILES")) == 0) { // get the number of file shared in a server
				wxString buffer;
				if ((item.Length() > 12) && (item.Mid(13).IsNumber())) {
					CServer* server = theApp.serverlist->GetServerAt( StrToLong(item.Mid(13)) );
					buffer = wxString::Format(wxT("%i"), server->GetFiles());
				} else if (theApp.serverconnect->IsConnected())
					buffer = theApp.serverconnect->GetCurrentServer()->GetListName();
				else
					buffer = wxEmptyString;

				return buffer;
			}
			
			if (item.Mid(7,7).Cmp(wxT("CONNECT")) == 0) { //Connect to a server (if specified) or to any server
				if (item.Length() > 14) { // try to connect to the specified server
					int separator = item.Mid(15).Find(wxT(" "));
					wxString sIP = item.Mid(15,separator);
					wxString sPort = item.Mid(15+separator+1);
					CServer* server = theApp.serverlist->GetServerByAddress(sIP, StrToLong(sPort) );
					if (server != NULL) {
						theApp.serverconnect->ConnectToServer(server);
						Notify_ShowConnState(false,wxT(""));
						return wxT("Connecting...");
					} else
						return wxT("Not Connected");
				} else { //connect to any server
					if (theApp.serverconnect->IsConnected()) {
						return wxT("Already connected");
					} else {
						theApp.serverconnect->ConnectToAnyServer();
						Notify_ShowConnState(false,wxT(""));
						return wxT("Connected");
					}
				}
			}
			
			if (item.Mid(7,3).Cmp(wxT("ADD")) == 0) { //add a new server
				if (item.Length() > 10) {
					int separator1 = item.Mid(11).Find(wxT(" "));
					wxString sIP = item.Mid(11, separator1);
					int separator2 = item.Mid(11+separator1+1).Find(wxT(" "));
					wxString sPort = item.Mid(11+separator1+1,separator2);
					wxString sName = item.Mid(11+separator1+separator2+2);
					
					CServer *nsrv = new CServer(StrToLong(sPort), sIP);
					nsrv->SetListName(sName);
					theApp.AddServer(nsrv);
					return wxT("Server Added");
				}
				return wxT("Server Not Added");
			}
			
			if (item.Mid(7,6).Cmp(wxT("REMOVE")) == 0) { // remove selected server from serverlist
				if (item.Length() > 13) {
					int separator = item.Mid(14).Find(wxT(" "));
					wxString sIP = item.Mid(14, separator);
					wxString sPort = item.Mid(14+separator+1);
					CServer* server = theApp.serverlist->GetServerByAddress(sIP, StrToLong(sPort));
					if (server != NULL) {
						theApp.serverlist->RemoveServer(server);
						return wxT("Removed");
					}
				}
				return wxT("Not removed");
			}
			
			if (item.Mid(7,9).Cmp(wxT("UPDATEMET")) == 0) {
				if (item.Length() > 16) {
					theApp.amuledlg->serverwnd->UpdateServerMetFromURL(item.Mid(17));
					return wxT("Updated");
				}
				return wxT("Not Updated");
			}
			
			return wxT("Bad SERVER Request");
		}
		
		
		// TRANSFER
		if (item.Left(8).Cmp(wxT("TRANSFER")) == 0) {
			if (item.Mid(9) == wxT("DL_LIST")) {
				// shakraw - return a big string where each line is formatted as:
				// wxString\tlong\tlong\tdouble\tlong\tint\twxString\tint\twxString\tlong\tlong\tlong\twxString\twxString\tint\n
				wxString buffer;
				wxString tempFileInfo;
				for (unsigned int i = 0; i < theApp.downloadqueue->GetFileCount(); ++i) {
					const CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(i);
					if (cur_file) {
						tempFileInfo = GetDownloadFileInfo(cur_file);
						tempFileInfo.Replace(wxT("\n"),wxT("|"));
						buffer += 
							cur_file->GetFileName() +
							wxString::Format(wxT("\t%li\t%li\t%f\t%li\t%d\t"),
								(long)cur_file->GetFileSize(),
								(long)cur_file->GetTransfered(),
								cur_file->GetPercentCompleted(),
								(long)(cur_file->GetKBpsDown()*1024),
								cur_file->GetStatus() ) +
							cur_file->getPartfileStatus() +
							wxString::Format(wxT("\t%d\t"), cur_file->GetDownPriority() +
								( cur_file->IsAutoDownPriority() ? 10 : 0 ) ) +
							EncodeBase16(cur_file->GetFileHash(), 16) +
							wxString::Format(wxT("\t%d\t%d\t%d\t"),
								cur_file->GetSourceCount(),
								cur_file->GetNotCurrentSourcesCount(),
								cur_file->GetTransferingSrcCount() ) +
							( theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() ?
								theApp.CreateED2kSourceLink(cur_file) :
								theApp.CreateED2kLink(cur_file) ) +
							wxT("\t") + tempFileInfo + wxT("\t") +
							( !cur_file->IsPartFile() ? wxT("1\n") : wxT("0\n") );
					}
				}
				return buffer;
			}
			
			if (item.Mid(9).Cmp(wxT("CLEARCOMPLETE")) == 0) { //clear completed transfers
#ifndef AMULE_DAEMON		
				theApp.amuledlg->transferwnd->downloadlistctrl->ClearCompleted();
#endif	
				return wxT("Clear Completed");
			}
			
			if (item.Mid(9,11).Cmp(wxT("ADDFILELINK")) == 0) { //add file link to download
				if (item.Length() > 20) {
					try {
						CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(unicode2char(item.Mid(21)));
						if (pLink->GetKind() == CED2KLink::kFile) {
							// lfroen - using default category 0
							theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), 0); 
							delete pLink;
							return wxT("Link Added");
						}
						delete pLink;
					} catch (...) {
						return wxT("Bad Link!");
					}
				}
				return wxT("Bad ADDFILELINK request");
			}
			
			if (item.Mid(9).Cmp(wxT("DL_COUNT")) == 0) { //get number of downloading files
				return wxString::Format( wxT("%i"), theApp.downloadqueue->GetFileCount() );
			}
			
			if (item.Mid(9,11).Cmp(wxT("DL_FILENAME")) == 0) { //get the n-th file name
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					return theApp.downloadqueue->GetFileByIndex(StrToLong(item.Mid(21)))->GetFileName();
				}
				return wxT("Bad DL_FILENAME request");
			}

			if (item.Mid(9,11).Cmp(wxT("DL_FILESIZE")) == 0) { //get the n-th file size
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(21)))->GetFileSize() );
				}
				return wxT("Bad DL_FILESIZE request");
			}

			if (item.Mid(9,17).Cmp(wxT("DL_FILETRANSFERED")) == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(27)))->GetTransfered() );
				}
				return wxT("Bad DL_FILETRANSFERED request");
			}

			if (item.Mid(9,14).Cmp(wxT("DL_FILEPERCENT")) == 0) {
				if ((item.Length() > 23) && (item.Mid(24).IsNumber())) {
					return wxString::Format( wxT("%f"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(24)) )->GetPercentCompleted() );
				}
				return wxT("Bad DL_FILEPERCENT request");
			}	
			
			if (item.Mid(9,15).Cmp(wxT("DL_FILEDATARATE")) == 0) {
				if ((item.Length() > 24) && (item.Mid(25).IsNumber())) {
					return wxString::Format( wxT("%i"), 
						(int)(theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(25)) )->GetKBpsDown())*1024);
				}
				return wxT("Bad DL_FILEDATARATE request");
			}	

			if (item.Mid(9,13).Cmp(wxT("DL_FILESTATUS")) == 0) {
				if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(23)))->GetStatus() );
				}
				return wxT("Bad DL_FILESTATUS request");
			}	
			
			if (item.Mid(9,17).Cmp(wxT("DL_PARTFILESTATUS")) == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					return theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(27)) )->getPartfileStatus();
				}
				return wxT("Bad DL_PARTFILESTATUS request");
			}
			
			if (item.Mid(9,18).Cmp(wxT("DL_FILEGETDOWNPRIO")) == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(28)) )->GetDownPriority() );
				}
				return wxT("Bad DL_FILEGETDOWNPRIO request");
			}	

			if (item.Mid(9,21).Cmp(wxT("DL_FILEISAUTODOWNPRIO")) == 0) {
				if ((item.Length() > 30) && (item.Mid(31).IsNumber())) {
					if (theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(31)) )->IsAutoDownPriority())
						return wxT("AutoDown Prio");
					else
						return wxT("Not AutoDown Prio");
				}
				return wxT("Bad DL_FILEISAUTODOWNPRIO request");
			}	

			if (item.Mid(9,11).Cmp(wxT("DL_FILEHASH")) == 0) {
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					// Why not use the encoded has? -- Xaignar
					char buffer[17];
					const unsigned char* hash = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(21)) )->GetFileHash();
					for ( int i = 0; i < 16; i++ ) {
						buffer[i] = hash[i];
					}
					buffer[16] = 0;
					return(char2unicode(buffer));
				}
				return wxT("Bad DL_FILEHASH request");
			}	
			
			if (item.Mid(9,17).Cmp(wxT("DL_FILEENCODEHASH")) == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					return theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(27)) )->GetFileHash().Encode();
				}
				return wxT("Bad DL_FILEENCODEHASH request");
			}	

			if (item.Mid(9,18).Cmp(wxT("DL_FILESOURCECOUNT")) == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(28)) )->GetSourceCount() );
				}
				return wxT("Bad DL_FILESOURCECOUNT request");
			}	
			
			if (item.Mid(9,19).Cmp(wxT("DL_FILENOCURRSOURCE")) == 0) {
				if ((item.Length() > 28) && (item.Mid(29).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(29)) )->GetNotCurrentSourcesCount() );
				}
				return wxT("Bad DL_FILENOCURRSOURCE request");
			}	
			
			if (item.Mid(9,17).Cmp(wxT("DL_FILETRSRCCOUNT")) == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					return wxString::Format( wxT("%i"),
						theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(27)) )->GetTransferingSrcCount() );
				}
				return wxT("Bad DL_FILETRSRCCOUNT request");
			}	
			
			if (item.Mid(9,18).Cmp(wxT("DL_FILEED2KSRCLINK")) == 0) {
				if ((item.Length() > 27) && (item.Mid(28).IsNumber())) {
					return theApp.CreateED2kSourceLink(theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(28))) );
				}
				return wxT("Bad DL_FILEED2KSRCLINK request");
			}	

			if (item.Mid(9,15).Cmp(wxT("DL_FILEED2KLINK")) == 0) {
				if ((item.Length() > 24) && (item.Mid(25).IsNumber())) {
					return theApp.CreateED2kLink(theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(25))) );
				}
				return wxT("Bad DL_FILEED2KLINK request");
			}	
			
			if (item.Mid(9,11).Cmp(wxT("DL_FILEINFO")) == 0) {
				if ((item.Length() > 20) && (item.Mid(21).IsNumber())) {
					return GetDownloadFileInfo(theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(21))) );
				}
				return wxT("Bad DL_FILEINFO request");
			}	

			if (item.Mid(9,17).Cmp(wxT("DL_FILEISPARTFILE")) == 0) {
				if ((item.Length() > 26) && (item.Mid(27).IsNumber())) {
					if (theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(27)) )->IsPartFile())
						return wxT("Is PartFile");
					else
						return wxT("Is Not PartFile");
				}
				return wxT("Bad DL_FILEISPARTFILE request");
			}	

			if (item.Mid(9,12).Cmp(wxT("DL_FILEPAUSE")) == 0) { //pause the n-th downloading file
				if ((item.Length() > 21) && (item.Mid(22).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(22)) );
					if (cur_file) {
						cur_file->PauseFile();
						return wxT("File Paused");
					}
				}
				return wxT("Bad DL_FILEPAUSE request");
			}

			if (item.Mid(9,13).Cmp(wxT("DL_FILERESUME")) == 0) { //resume the n-th downloading file
				if ((item.Length() > 22) && (item.Mid(23).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(23)) );
					if (cur_file) {
						cur_file->ResumeFile();
						return wxT("File Resumed");
					}
				}
				return wxT("Bad DL_FILERESUME request");
			}

			if (item.Mid(9,21).Cmp(wxT("DL_FILESETAUTODOWNPRIO")) == 0) { //set autodownprio for the file
				if (item.Length() > 30) {
					int separator = item.Mid(31).Find(wxT(" "));
					int file_Idx = -1; 
					bool flag = false;
					if (item.Mid(31, separator).IsNumber())
						file_Idx = StrToLong(item.Mid(31, separator));
					if (item.Mid(31+separator+1).IsNumber())
						flag = StrToLong(item.Mid(31+separator+1));

					if (file_Idx >= 0) {
						CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(file_Idx);
						if (cur_file) {
							cur_file->SetAutoDownPriority(flag);
							return wxT("AutoDownPrio Set");
						}
					}
				}
				return wxT("Bad DL_FILESETAUTODOWNPRIO request");
			}
			
			if (item.Mid(9,18).Cmp(wxT("DL_FILESETDOWNPRIO")) == 0) { //set autodownprio for the file
				if (item.Length() > 27) {
					int separator = item.Mid(28).Find(wxT(" "));
					int file_Idx = -1; 
					int prio = -1;
					if (item.Mid(31, separator).IsNumber())
						file_Idx = StrToLong(item.Mid(28, separator));
					if (item.Mid(31+separator+1).IsNumber())
						prio = StrToLong(item.Mid(28+separator+1));

					if ((file_Idx >= 0) && (prio >= 0)) {
						CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex(file_Idx);
						if (cur_file) {
							cur_file->SetDownPriority(prio);
							return wxT("DownPrio Set");
						}
					}
				}
				return wxT("Bad DL_FILESETDOWNPRIO request");
			}
			
			if (item.Mid(9,7).Cmp(wxT("DL_FILE")) == 0) { //test if the n-th downloading file exists
				if ((item.Length() > 16) && (item.Mid(17).IsNumber())) {
					CPartFile *cur_file = theApp.downloadqueue->GetFileByIndex( StrToLong(item.Mid(17)) );
					if (cur_file) return wxT("Got File");
				}
				return wxT("Bad DL_FILE request");
			}
			
			return wxT("Bad TRANSFER request");
		} // end - TRANSFER
		
		
		// FILE
		if (item.Left(4).Cmp(wxT("FILE")) == 0) {
			if (item.Mid(5,11).Cmp(wxT("GETFILEBYID")) == 0) {
				if (item.Length() > 16) {
					uchar fileid[16];
					wxString filehash = item.Mid(17);
					DecodeBase16(unicode2char(filehash),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if (cur_file) return wxT("Got File");
					
					return wxT("No File");
				}
				return wxT("Bad GETFILEBYID request");
			}
			
			if (item.Mid(5,10).Cmp(wxT("ISPARTFILE")) == 0) { //return wether file is partfile or not
				if (item.Length() > 15) {
					uchar fileid[16];
					wxString filehash = item.Mid(16);
					DecodeBase16(unicode2char(filehash),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if ((cur_file) && (cur_file->IsPartFile())){
						return wxT("Is PartFile");
					}
					return wxT("Is Not PartFile");
				}
				return wxT("Bad ISPARTFILE request");
			}

			if (item.Mid(5,17).Cmp(wxT("GETPROGRESSSTRING")) == 0) { //get file's progress bar string
				if (item.Length() > 22) {
					int separator = item.Mid(23).Find(wxT(" "));
					if (separator > -1) {
						wxString filehash = item.Mid(23,separator);
						uint16 progressbarWidth = 0;
						if (item.Mid(23+separator+1).IsNumber())
							progressbarWidth = StrToLong(item.Mid(23+separator+1));
						
						uchar fileid[16];
						DecodeBase16(unicode2char(filehash),filehash.Length(),fileid);
						CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
						if (cur_file) {
							return cur_file->GetProgressString(progressbarWidth);
						}
						
						return wxT("No File");
					}
				}
				return wxT("Bad GETPROGRESSSTRING request");
			}
			
			if (item.Mid(5,18).Cmp(wxT("GETPERCENTCOMPLETE")) == 0) { //get file's progress bar string
				if (item.Length() > 23) {
					wxString filehash = item.Mid(24);
					uchar fileid[16];
					DecodeBase16(unicode2char(filehash),filehash.Length(),fileid);
					CPartFile *cur_file=theApp.downloadqueue->GetFileByID(fileid);
					if (cur_file) {
						return wxString::Format( wxT("%f"), cur_file->GetPercentCompleted() );
					}
					
					return wxT("No File");
				}
				return wxT("Bad GETPERCENTCOMPLETE request");
			}
			
			return wxT("Bad FILE request");
		} // end - FILE
		
		
		// QUEUE
		if (item.Left(5).Cmp(wxT("QUEUE")) == 0) { //Get queue data information
			if (item.Mid(6).Cmp(wxT("W_GETSHOWDATA")) == 0) {
				wxString buffer;
				for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList();
					 pos != 0;theApp.uploadqueue->GetNextFromWaitingList(pos)) {
					CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
					if (cur_client) {
						buffer += cur_client->GetUserName();
						if (!cur_client->GetRequestFile()) {
							buffer += wxT("\n");
							continue;
						} else {
							buffer += wxT("\t");
						}
						CKnownFile* file = theApp.sharedfiles->GetFileByID(
							cur_client->GetRequestFile()->GetFileHash());
						buffer +=
							( file ? cur_client->GetDownloadFile()->GetFileName() : wxT("?") ) + wxT("\t") +
							wxString::Format(wxT("%i\t"), cur_client->GetScore(false)) +
							( cur_client->IsBanned() ? wxT("1\n") : wxT("0\n") );
					}
				}
				return buffer;
			}
			
			if (item.Mid(6).Cmp(wxT("UL_GETSHOWDATA")) == 0) {
				wxString buffer;
				wxString temp;
				for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
					 pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos)) {
					CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
					if (cur_client) {
						temp = cur_client->GetUploadFileInfo(); 
						temp.Replace(wxT("\n"), wxT(" | "));
						CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
						buffer +=
							cur_client->GetUserName() + wxT("\t") +
							temp + wxT("\t") +
							( file ? file->GetFileName() : wxT("?") ) + wxT("\t") +
							wxString::Format(wxT("%i\t%i\t%i\n"),
								cur_client->GetTransferedDown(),
								cur_client->GetTransferedUp(),
								(uint32)(cur_client->GetKBpsUp()*1024.0) );
					}
				}
				return buffer;
			}

			if (item.Mid(6).Cmp(wxT("UL_GETLENGTH")) == 0) {
				return wxString::Format( wxT("%i"), theApp.uploadqueue->GetUploadQueueLength() );
			}
			
			return wxT("Bad QUEUE request");
		} // end - QUEUE
		
		
		// SEARCH
		if (item.Left(6).Cmp(wxT("SEARCH"))==0) {
			if (item.Mid(7,12).Cmp(wxT("DOWNLOADFILE")) == 0) { //add file to download
				if (item.Length() > 19) {
					uchar fileid[16];
					DecodeBase16(unicode2char(item.Mid(20)),item.Mid(20).Length(),fileid);
					theApp.searchlist->AddFileToDownloadByHash(fileid);
					//theApp.downloadqueue->AddDownload(cur_file);
					return wxT("Download Added");
				}
				return wxT("Bad DOWNLOADFILE request");
			}
			// lfroen: why such command exist ?!
// 			if (item.Mid(7,16).Cmp(wxT("DELETEALLSEARCHS")) == 0) { //delete all searchs
// 				theApp.amuledlg->searchwnd->DeleteAllSearchs();
// 				return wxT("Searchs Deleted");
// 			}
			
			if (item.Mid(7,11).Cmp(wxT("DONEWSEARCH")) == 0) {
				if (item.Length() > 18) {
					int curIndex, nextIndex;
					wxString sParams = item.Mid(19);

					curIndex=0; nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					wxString searchString = sParams.Left(nextIndex);
					searchString.Trim(true);
					searchString.Trim(false);
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					wxString typeText = sParams.Mid(curIndex, nextIndex);
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					uint32 min = StrToLong(sParams.Mid(curIndex, nextIndex));
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					uint32 max = StrToLong(sParams.Mid(curIndex, nextIndex));
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					uint32 avaibility = StrToLong(sParams.Mid(curIndex, nextIndex));
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					wxString extension = sParams.Mid(curIndex, nextIndex);
					
					curIndex=curIndex+nextIndex+1;
					nextIndex=sParams.Mid(curIndex).Find(wxT("\n"));
					bool globalsearch = !sParams.Mid(curIndex, nextIndex).Cmp(wxT("global"));
				
					theApp.searchlist->RemoveResults(0xffff);
					theApp.searchlist->NewSearch(typeText, 0xffff);
					Packet *packet = CreateSearchPacket(searchString, typeText, extension, min, max, avaibility);
				
					// this is internal core call, but macro is useful anyway
					CoreNotify_Search_Req(packet, globalsearch);
				}
				return wxT("Bad DONEWSEARCH request");
			}
			
			if (item.Mid(7,7).Cmp(wxT("WEBLIST")) == 0) {
				if (item.Length() > 14) {
					wxString sItem = item.Mid(15);
					int separator=sItem.Find(wxT("\t"));
					if (separator != -1) {
						wxString sResultLine = sItem.Left(separator+1);
						sItem = sItem.Mid(separator+1);
						separator=sItem.Find(wxT("\t"));
						int sortBy = StrToLong(sItem.Left(separator+1));
						if (sortBy >= 0) {
							bool searchAsc = StrToLong(sItem.Mid(separator+1));
							return((wxChar*)theApp.searchlist->GetWebList(sResultLine, sortBy, searchAsc).GetData());
						}
					}
				}
				return wxT("Bad WEBLIST request");
			}
			return wxT("Bad SEARCH request");
		} // end - SEARCH
		
		//shakraw, amuleweb protocol communication START
		// New item: for release goups only, TOTALULREQ HASH replies the total upload for the file HASH
		if (item.Left(10).Cmp(wxT("TOTALULREQ")) == 0) { 
			wxString buffer;
			#ifdef RELEASE_GROUP_MODE
			if (item.Mid(11).Length() == 16) {
				CKnownFile* shared_file = theApp.sharedfiles->GetFileByID(item.Mid(11));
				if (shared_file!=NULL) {
					buffer.Printf( wxT("%i"), shared_file->totalupload );
				} else {
					buffer = wxT("FILENOTSHARED");
				}
			} else {
				buffer = wxT("WRONGHASH");
			}	
			#else
				buffer = wxT("NOTRELEASEGRP");
			#endif	

			return buffer;
		}		
		
		return wxEmptyString;
}

wxString ExternalConn::GetDownloadFileInfo(const CPartFile* file)
{
	wxString sRet;
	wxString strHash = EncodeBase16(file->GetFileHash(), 16);
	wxString strComplx = CastItoXBytes(file->GetCompletedSize()) + wxT("/") + CastItoXBytes(file->GetFileSize());
	wxString strLsc = _("Unknown");
	wxString strLastprogr = _("Unknown");
	
	if ( file->lastseencomplete ) {
		wxDateTime time = (time_t)file->lastseencomplete;
		strLsc = time.Format( theApp.glob_prefs->GetDateTimeFormat() );
	}
	
	if ( file->GetFileDate() ) {
		wxDateTime time = (time_t)file->GetFileDate();
		strLastprogr = time.Format( theApp.glob_prefs->GetDateTimeFormat() );
	}

	float availability = 0;
	if( file->GetPartCount() ) {
		availability = file->GetAvailablePartCount() * 100 / file->GetPartCount();
	}

	sRet << wxString(_("File Name")) << wxT(": ") << file->GetFileName() << wxT(" (") << CastItoXBytes(file->GetFileSize()) << wxT(" ") << wxString(_("Bytes")) << wxT(")\n\n");
	sRet << wxString(_("Status")) << wxT(": ") << file->getPartfileStatus() << wxT("\n\n");
	sRet << wxString(_("Hash :")) << wxT(" ") << strHash << wxT("\n");
	sRet << wxString(_("Partfilename: ")) << file->GetPartMetFileName() << wxT("\n");
	sRet << wxString(_("Parts: ")) << file->GetPartCount() << wxT(", ")
		<< wxString(_("Available")) << wxT(": ") << file->GetAvailablePartCount() << wxT(" ")
		<< wxString::Format(wxT( "(%.1f%%)"), availability);
	sRet << wxString::Format(wxT("%d%% done (%s) - Transferring from %d sources"), (int)file->GetPercentCompleted(), unicode2char(strComplx), file->GetTransferingSrcCount()) + wxT("\n");
	sRet << wxString(_("Last Seen Complete :")) << wxT(" ") << strLsc << wxT("\n");
	sRet << wxString(_("Last Reception:")) << wxT(" ") << strLastprogr << wxT("\n");
	
	return sRet;
}

ExternalConnServerThread::ExternalConnServerThread(ExternalConn *owner) : wxThread()
{
	m_owner = owner;	
	if (theApp.glob_prefs->ECUseTCPPort()) {
		int port = theApp.glob_prefs->ECPort();
		// Create the address - listen on localhost:ECPort
		wxIPV4address addr;
		addr.Service(port);
		// Create the socket
		m_ECServer = new wxSocketServer(addr, wxSOCKET_REUSEADDR);
		// We use Ok() here to see if the server is really listening
		if (! m_ECServer->Ok()) {
			printf("Could not listen for external connections at port %d!\n\n", port);
		} else {
			printf("ECServer listening on port %d.\n\n", port);
			if ( Create() != wxTHREAD_NO_ERROR ) {
				printf("ExternalConnServerThread: failed to Create thread\n");
			}
		}
	} else {
		// m_ECServer has not been created, this inhibits 
		// delete in the destructor from doing nasty things
		m_ECServer = 0;
	}
}

ExternalConnServerThread::~ExternalConnServerThread()
{
	delete m_ECServer;
}

void *ExternalConnServerThread::Entry()
{
        while ( !TestDestroy() ) {
		if ( m_ECServer->WaitForAccept() ) {
                        wxSocketBase *client = m_ECServer->Accept(false);
                        if ( !client ) {
				continue;
			}
			client->Notify(false);
			ExternalConnClientThread *cli_thread = new ExternalConnClientThread(m_owner, client);
			cli_thread->Run();
		}
	}
	return 0;
}

ExternalConnClientThread::ExternalConnClientThread(ExternalConn *owner, wxSocketBase *sock) : wxThread()
{
	m_owner = owner;
	m_sock = sock;
	if ( Create() != wxTHREAD_NO_ERROR ) {
		printf("ExternalConnClientThread: failed to Create thread\n");
	}
	sock->SetFlags(wxSOCKET_WAITALL);
}

ExternalConnClientThread::~ExternalConnClientThread()
{
	delete m_sock;
}

void *ExternalConnClientThread::Entry()
{
	wxString request;
	Read(m_sock, request);
	wxString response = m_owner->Authenticate(request);
	Write(m_sock, response);
	if (response != wxT("Authenticated")) {
		//
		// Access denied!
		//
		printf("Unauthorized access attempt. Connection closed.\n");
		return 0;
	}
	//
	// Authenticated
	//
	while ( !TestDestroy() ) {
		if ( m_sock->WaitForLost(0, 0) ) {
			printf("ExternalConnClientThread: connection closed\n");
			return 0;
		}
		if (m_sock->WaitForRead()) {
			Read(m_sock, request);
			//
			// lfroen: this one definitly must NOT be here. It is 'cause somewhere
			// in ProcessRequest we're going thru X event chain
			// 
			wxMutexGuiEnter();
			wxString response = m_owner->ProcessRequest(request);
			wxMutexGuiLeave();
			Write(m_sock, response);
		}
	}
	return 0;
}

void ExternalConnClientThread::Read(wxSocketBase *sock, wxString &s) {
	unsigned int tmp;
	sock->Read(&tmp, sizeof(unsigned int));
	unsigned int msgBytes = ENDIAN_SWAP_32(tmp);
	char *utf8 = new char[msgBytes+1];
	utf8[msgBytes] = 0;
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	char *iobuf = utf8;
	register int i = 0;
	while(msgRemain) {
		sock->Read(iobuf, msgRemain);
		LastIO = sock->LastCount();
		msgRemain -= LastIO;
		iobuf += LastIO;
		++i;
	}
	s = wxString(wxConvUTF8.cMB2WX(utf8));
// This is debug code, to be removed later
//printf("read loop=%d, bytes=%d, original=%d\n", i, msgBytes, s.size());
	delete [] utf8;
	if(sock->Error()) {
		printf("Wrong wxString Reading Packet!\n");
	}
}

void ExternalConnClientThread::Write(wxSocketBase *sock, const wxString &s) {
	const wxWX2MBbuf buf = wxConvUTF8.cWX2MB(s);
	const char *utf8 = (const char *)buf;
	unsigned int msgBytes = strlen(utf8);
	unsigned int tmp = ENDIAN_SWAP_32(msgBytes);
	sock->Write(&tmp, sizeof(unsigned int));
	unsigned int msgRemain = msgBytes;
	unsigned int LastIO;
	const char *iobuf = utf8;
	register int i = 0;
	while(msgRemain) {
		sock->Write(iobuf, msgBytes);
		LastIO = sock->LastCount();
		msgRemain -= LastIO;
		iobuf += LastIO;
		++i;
	}
// This is debug code, to be removed later
//printf("write loop=%d, bytes=%d, original=%d\n", i, msgBytes, s.size());
}


