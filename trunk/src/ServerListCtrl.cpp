// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


// ServerListCtrl.cpp : implementation file
//

#include <wx/textfile.h>
#include <wx/event.h>

#include "amuleDlg.h"		// Needed for CamuleDlg
#include "ServerListCtrl.h"	// Interface declarations
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "PartFile.h"		// Needed for SRV_PR_LOW
#include "ServerList.h"		// Needed for CServerList
#include "ServerWnd.h"		// Needed for CServerWnd
#include "sockets.h"		// Needed for CServerConnect
#include "amule.h"		// Needed for theApp
#include "server.h"		// Needed for CServer
#include "opcodes.h"		// Needed for MP_PRIOLOW
#include "muuli_wdr.h"		// Needed for ID_SERVERLIST

// CServerListCtrl

#define SYSCOLOR(x) (wxSystemSettings::GetColour(x))

BEGIN_EVENT_TABLE(CServerListCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK(ID_SERVERLIST,CServerListCtrl::OnRclickServlist)
	EVT_LIST_COL_CLICK(ID_SERVERLIST,CServerListCtrl::OnLvnColumnclickServlist)
	EVT_LEFT_DCLICK(CServerListCtrl::OnLDclick)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CServerListCtrl, CMuleListCtrl/*CTreeCtrl*/)

CServerListCtrl::CServerListCtrl()
{
	memset(&asc_sort,0,8);
}

CServerListCtrl::CServerListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags)
{
	memset(&asc_sort,0,8);
	m_ServerPrioMenu=NULL;
	m_ServerMenu=NULL;
	connected = -1;
}

void CServerListCtrl::OnLDclick(wxMouseEvent& event)
{
	int lips=0;
	int index=HitTest(event.GetPosition(),lips);
	if(index>=0) {
		SetItemState(index,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
	}
	wxCommandEvent nulEvt;
	OnConnectTo(nulEvt);
}

void CServerListCtrl::OnRclickServlist(wxListEvent& event)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long item=-1;
	if (!GetItemState(event.GetIndex(), wxLIST_STATE_SELECTED)) {
		for (;;) {
			item = GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if (item==-1) {
				break;
			}
			SetItemState(item, 0, wxLIST_STATE_SELECTED);
		}
		SetItemState(event.GetIndex(), wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
	}

	bool priority_enabled, static_enabled, show_connect;

	// set states
	CServer* test=NULL;
	int selidx;
	selidx=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if(selidx!=-1) {
		priority_enabled = TRUE;
	} else {
		priority_enabled = FALSE;
	}

	// Create up-to-date popupmenu
	if(m_ServerMenu==NULL) {
		m_ServerMenu=new wxMenu(_("Server"));
		m_ServerPrioMenu=new wxMenu();
		m_ServerPrioMenu->Append(MP_PRIOLOW,_("Low"));
		m_ServerPrioMenu->Append(MP_PRIONORMAL,_("Normal"));
		m_ServerPrioMenu->Append(MP_PRIOHIGH,_("High"));

		m_ServerMenu->Append(MP_CONNECTTO,_("Connect to this server"));

		m_ServerMenu->Append(999999,_("Priority"),m_ServerPrioMenu);
		m_ServerMenu->Append(MP_ADDTOSTATIC,_("Add to static"));
		m_ServerMenu->Append(MP_REMOVEFROMSTATIC, _("Remove from static server list"));
		m_ServerMenu->AppendSeparator();
		m_ServerMenu->Append(MP_REMOVE,_("Remove server"));
		m_ServerMenu->Append(MP_REMOVEALL,_("Remove all servers"));
		m_ServerMenu->AppendSeparator();
		m_ServerMenu->Append(MP_GETED2KLINK,_("Copy ED2k &link to clipboard"));
	}

	if(selidx!=-1) {
		test=(CServer*)GetItemData(selidx);
		if(test) {
			if(test->IsStaticMember()) {
				static_enabled = TRUE;
			} else {
				static_enabled = FALSE;
			}
		} else {
			static_enabled = FALSE;
		}
		if((long)test==connected) {
			show_connect = FALSE;
		} else {
			show_connect = TRUE;
		}
	} else {
		static_enabled = FALSE;
	}


	m_ServerMenu->Enable(MP_REMOVEFROMSTATIC,static_enabled);
	m_ServerMenu->Enable(MP_ADDTOSTATIC,!static_enabled);
	m_ServerMenu->Enable(999999, priority_enabled);

	if (show_connect) {
		m_ServerMenu->SetLabel(MP_CONNECTTO,_("Connect to this server"));
	} else {
		m_ServerMenu->SetLabel(MP_CONNECTTO,_("Reconnect to this server"));
	}


	PopupMenu(m_ServerMenu,event.GetPoint());
}

void CServerListCtrl::OnConnectTo(wxCommandEvent& event)
{
	int connectcounter=0;
	long item=-1;

	if (this->GetSelectedItemCount()>1) {
		theApp.serverconnect->Disconnect();
		item=-1;
		do {
			item=GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if(item>-1) {
				connectcounter++;
				theApp.serverconnect->ConnectToServer((CServer*)this->GetItemData(item),true);
			}
		} while(item!=-1 && connectcounter<10);
	} else {
		item=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		if(item>-1) {
			theApp.serverconnect->ConnectToServer((CServer*)GetItemData(item));
		}
	}
	theApp.amuledlg->ShowConnectionState(false);
}

void CServerListCtrl::OnRemove(wxEvent& event)
{
	long item=-1;
	if (this->GetSelectedItemCount()>1) {
		item=-1;
		do {
			item=GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
			if(item>-1) {
				theApp.amuledlg->serverwnd->serverlistctrl->RemoveServer((CServer*)this->GetItemData(item));
			}
		} while(item!=-1);
	} else {
		item=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		if(item>-1) {
			theApp.amuledlg->serverwnd->serverlistctrl->RemoveServer((CServer*)this->GetItemData(item));
		}
	}
}

void CServerListCtrl::OnCopyLink(wxCommandEvent& event)
{
	long item=-1;
	wxString buffer,link;
	while((item=GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)) != -1) {
		CServer* change = (CServer*) this->GetItemData(item);
		buffer=buffer.Format(wxT("ed2k:|server|%s|%d|"), change->GetFullIP(), change->GetPort());
		if(link.Length()>0) {
			buffer=wxT("\n")+buffer;
		}
		link += buffer;
	}
	theApp.CopyTextToClipboard(link);
}

void CServerListCtrl::InitSort()
{
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableServer);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableServer);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	ShowFilesCount();
}

bool CServerListCtrl::Init(CServerList* in_list)
{
	printf("Serverlist loaded.\n");
	server_list = in_list;

	InsertColumn(0,_("Server Name"),wxLIST_FORMAT_LEFT,150);
	InsertColumn(1,_("IP"),wxLIST_FORMAT_LEFT,140);
	InsertColumn(2,_("Description") ,wxLIST_FORMAT_LEFT, 150);
	InsertColumn(3,_("Ping"),wxLIST_FORMAT_LEFT, 25);
	InsertColumn(4,_("Users"),wxLIST_FORMAT_LEFT, 40);
	InsertColumn(5,_("Files"),wxLIST_FORMAT_LEFT, 45);
	InsertColumn(6,_("Preference"),wxLIST_FORMAT_LEFT, 60);
	InsertColumn(7,_("Failed"),wxLIST_FORMAT_LEFT, 40);
	InsertColumn(8,_("Static"),wxLIST_FORMAT_LEFT, 40);
	InsertColumn(9,_("Version"),wxLIST_FORMAT_LEFT, 80);

	asc_sort[3]=true;asc_sort[4]=true;asc_sort[5]=true;asc_sort[7]=true;
	// perhaps not yet
	//LoadSettings(CPreferences::tableServer);
	return true;
}

void CServerListCtrl::Localize()
{
}

void CServerListCtrl::RemoveServer(CServer* todel,bool bDelToList)
{
	//LVFINDINFO find;
	//find.flags = LVFI_PARAM;
	//find.lParam = (LPARAM)todel;


	sint32 result = FindItem(-1,(long)todel);
	if (result != (-1) ) {
		server_list->RemoveServer((CServer*)GetItemData(result));
		DeleteItem(result);
	}


	return;
}
void CServerListCtrl::RemoveAllServers(int state)
{
	int pos=GetNextItem(-1,wxLIST_NEXT_ALL,state);
	int found = -1;
	while (pos != -1) {
		if (GetItemData(pos) == connected) {
			wxMessageBox(wxT("You are connected to a server you are trying to delete. Please disconnect first. The server was NOT deleted."), wxT("Info"), wxOK);
			found = pos;
			} else {
				server_list->RemoveServer((CServer*)this->GetItemData(pos));
				DeleteItem(pos);
			}
				pos=GetNextItem(found,wxLIST_NEXT_ALL,state);
		}

	ShowFilesCount();
}


// Remove Dead Servers
void CServerListCtrl::RemoveDeadServer()
{
	if(theApp.glob_prefs->DeadServer()) {
		for(POSITION pos = server_list->list.GetHeadPosition(); pos != NULL;server_list->list.GetNext(pos)) {
			CServer* cur_server = server_list->list.GetAt(pos);
			if(cur_server->GetFailedCount() > theApp.glob_prefs->GetDeadserverRetries()) {	// MAX_SERVERFAILCOUNT
				RemoveServer(cur_server);
				pos = server_list->list.GetHeadPosition();
			}
		}
	}
}

bool CServerListCtrl::AddServer(CServer* toadd,bool bAddToList)
{
	if (!server_list->AddServer(toadd)) {
		return false;
	}
	if (bAddToList) {
		uint32 itemnr=GetItemCount();
		uint32 newid=InsertItem(itemnr,char2unicode(toadd->GetListName()));
		SetItemData(newid,(long)toadd);
		wxListItem myitem;
		myitem.m_itemId=newid;
		myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
		SetItem(myitem);
		RefreshServer(toadd);
	}
	ShowFilesCount();
	return true;
}

void CServerListCtrl::HighlightServer(const CServer* server, bool highlight)
// Copyright (C) 2004 pure_ascii ( pure_ascii@users.sourceforge.net / http://www.amule.org )
{

	long itemnr=FindItem(-1,(long)server);

	if (itemnr!=-1) {
//	Kry - Gui hangs while we do this...
//	A safer hack it to change only the connecting server and handle the disconections
//	for(long pos = 0; pos < GetItemCount(); ++pos) {
		connected = (long)server;
		wxListItem myitem;
		myitem.SetId(itemnr);
		GetItem(myitem);
		wxFont myfont = myitem.GetFont();
		myfont.SetWeight((highlight) ? wxBOLD : wxNORMAL);
		myitem.SetFont(myfont);
		SetItem(myitem);
	}
}

void CServerListCtrl::RefreshServer(CServer* server)
{
	long itemnr=FindItem(-1,(long)server);
	if(itemnr==(-1)) {
		return;
	}
	wxString temp;
	if(!server) {
		return;
	}
	temp = char2unicode(server->GetAddress()) + wxString::Format(wxT(" : %i"),server->GetPort());
	SetItem(itemnr,1,temp);
	if(server->GetListName()) {
		temp=char2unicode(server->GetListName());
		SetItem(itemnr,0,temp);
	}
	if(server->GetDescription()) {
		temp=char2unicode(server->GetDescription());
		SetItem(itemnr,2,temp);
	}
	if(1) {
		if(server->GetPing()) {
			temp=wxString::Format( wxT("%i"),server->GetPing());
		} else {
			temp=wxT("");
		}
		SetItem(itemnr,3,temp);
	} else {
		printf("%lx: ei ping\n",(long)server);
		SetItem(itemnr,3,wxT("Ei ei"));
	}
	if(server->GetUsers()) {
		temp=wxString::Format( wxT("%i"),server->GetUsers());
		SetItem(itemnr,4,temp);
	}
	if(server->GetFiles()) {
		temp=wxString::Format( wxT("%i"),server->GetFiles());
		SetItem(itemnr,5,temp);
	}
	if(server->GetPreferences()) {
		temp=wxString::Format( wxT("%i"),server->GetPreferences());
		SetItem(itemnr,6,temp);
	}
	switch(server->GetPreferences()) {
		case SRV_PR_LOW:
			temp.Printf( _("Low"));
			SetItem(itemnr,6,temp);
			break;
		case SRV_PR_NORMAL:
			temp.Printf( _("Normal"));
			SetItem(itemnr,6,temp);
			break;
		case SRV_PR_HIGH:
			temp.Printf( _("High"));
			SetItem(itemnr,6,temp);
			break;
		default:
			temp.Printf( _("No Pref"));
			SetItem(itemnr,6,temp);
	}
	if(server->GetFailedCount() < 0) {
		server->ResetFailedCount();
	}
	temp=wxString::Format( wxT("%i"),server->GetFailedCount());
	SetItem(itemnr,7,temp);
	if (server->IsStaticMember()) {
		SetItem(itemnr,8,_("Yes"));
	} else {
		SetItem(itemnr,8,_("No"));
	}

	temp = server->GetVersion();
	SetItem(itemnr,9,temp);

}

bool CServerListCtrl::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}

	wxCommandEvent& event=(wxCommandEvent&)evt;
	int item=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);

	if (event.GetId()==MP_REMOVEALL) {
		if(theApp.serverconnect->IsConnecting()) {
			theApp.downloadqueue->StopUDPRequests();
			theApp.serverconnect->StopConnectionTry();
			theApp.serverconnect->Disconnect();
			theApp.amuledlg->ShowConnectionState(false);
		}

		RemoveAllServers(wxLIST_STATE_DONTCARE);
		return true;
	}

	if (item != -1) {
		if (((CServer*)GetItemData(item)) != NULL) {
			switch (event.GetId()) {
				case MP_CONNECTTO: {
					if (this->GetSelectedItemCount()>1) {
						CServer* aServer;
						theApp.serverconnect->Disconnect();
						int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
						while (pos != -1) {
							item = pos;
							if (item>-1) {
								aServer=(CServer*)this->GetItemData(item);
								theApp.serverlist->MoveServerDown(aServer);
							}
							pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
						}
						theApp.serverconnect->ConnectToAnyServer(theApp.serverlist->GetServerCount() - this->GetSelectedItemCount(),false, false);
					} else {
						theApp.serverconnect->ConnectToServer((CServer*)GetItemData(item));
					}
					theApp.amuledlg->ShowConnectionState(false);
					return true;
					break;
				}
				case MP_REMOVE: {
					RemoveAllServers(wxLIST_STATE_SELECTED);
					return true;
					break;
				}

				case MP_ADDTOSTATIC: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						if (!StaticServerFileAppend(change)) {
							return false;
						}
						change->SetIsStaticMember(true);
						theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(change);
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					return true;
					break;
				}
				// Remove Static Servers [Barry]
				case MP_REMOVEFROMSTATIC: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						if (!StaticServerFileRemove(change)) {
							return false;
						}
						change->SetIsStaticMember(false);
						theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(change);
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					return true;
					break;
				}
				case MP_PRIOLOW: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						change->SetPreference( SRV_PR_LOW);
						// if (change->IsStaticMember()) {
							// StaticServerFileAppend(change); //Why are you adding to static when changing prioity? If I want it static I set it static.. I set server to LOW because I HATE this server, not because I like it!!!
						// }
						theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(change);
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					return true;
					break;
				}
				case MP_PRIONORMAL: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						change->SetPreference(SRV_PR_NORMAL);
						// if (change->IsStaticMember()) {
							// StaticServerFileAppend(change);
						// }
						theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(change);
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					return true;
					break;
				}
				case MP_PRIOHIGH: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						change->SetPreference( SRV_PR_HIGH );
						// if (change->IsStaticMember()) {
							// StaticServerFileAppend(change);
						// }
						theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(change);
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					return true;
					break;
				}
				case MP_GETED2KLINK: {
					int pos=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					wxString buffer, link;
					while(pos != -1) {
						CServer* change = (CServer*)this->GetItemData(pos);
						buffer.Printf(wxT("ed2k://|server|%s|%d|/"), change->GetFullIP(), change->GetPort());
						if (link.Length()>0) {
							buffer=wxT("\n")+buffer;
						}
						link += buffer;
						pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
					}
					theApp.CopyTextToClipboard(link);
					return true;
					break;
				}
			}
		}
	}
	// Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);
}

bool CServerListCtrl::AddServermetToList(wxString strFile)
{
	Freeze();
	bool flag=server_list->AddServermetToList(strFile.GetData());
	RemoveDeadServer();
	ShowFilesCount();
	Thaw();
	return flag;
}

void CServerListCtrl::OnLvnColumnclickServlist(wxListEvent& evt) //NMHDR *pNMHDR, LRESULT *pResult)
{
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableServer);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableServer);
	bool sortAscending = (sortItem != evt.GetColumn()) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = evt.GetColumn();
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableServer, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableServer, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
}

int CServerListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CServer* item1 = (CServer*)lParam1;
	CServer* item2 = (CServer*)lParam2;
	if((item1 == NULL) || (item2 == NULL)) {
		return 0;
	}
	int iTemp=0;
	int counter1;
	int counter2;
	switch(lParamSort) {
		case 0: //(List) Server-name asc
			return wxString(char2unicode(item1->GetListName())).CmpNoCase(char2unicode(item2->GetListName()));
		case 100: //(List) Server-name desc
			return wxString(char2unicode(item2->GetListName())).CmpNoCase(char2unicode(item1->GetListName()));
		case 1: { //IP asc
			if (item1->HasDynIP() && item2->HasDynIP()) {
				return wxString(char2unicode(item1->GetDynIP())).CmpNoCase(char2unicode(item2->GetDynIP()));
			} else if (item1->HasDynIP()) {
				return 1;
			} else if (item2->HasDynIP()) {
				return 0;
			} else {
				wxString sIP1, sIP2, sTemp1, sTemp2;
				counter1 = counter2 = iTemp = 0;
				sIP1 = item2->GetFullIP();
				sIP2 = item1->GetFullIP();
				int a[4],b[4];
				sscanf(unicode2char(sIP1),"%d.%d.%d.%d",&a[0],&a[1],&a[2],&a[3]);
				sscanf(unicode2char(sIP2),"%d.%d.%d.%d",&b[0],&b[1],&b[2],&b[3]);
				for(int i=0;iTemp==0;i++) {
					iTemp=b[i]-a[i];
					if(i>3) {
						return item1->GetPort()-item2->GetPort();
					}
				}
				return iTemp;
			  }
		}
		case 101: { //IP desc
			if(item1->HasDynIP() && item2->HasDynIP()) {
				return wxString(char2unicode(item2->GetDynIP())).CmpNoCase(char2unicode(item1->GetDynIP()));
			} else if(item1->HasDynIP()) {
				return 0;
			} else if(item2->HasDynIP()) {
				return 1;
			} else {
				wxString s2IP1, s2IP2, s2Temp1, s2Temp2;
				counter1 = counter2 = iTemp = 0;
				s2IP1 = item2->GetFullIP();
				s2IP2 = item1->GetFullIP();
				int a[4],b[4];
				sscanf(unicode2char(s2IP1),"%d.%d.%d.%d",&a[0],&a[1],&a[2],&a[3]);
				sscanf(unicode2char(s2IP2),"%d.%d.%d.%d",&b[0],&b[1],&b[2],&b[3]);
				for(int i=0;iTemp==0;i++) {
					iTemp=a[i]-b[i];
					if(i>3) {
						return item2->GetPort()-item1->GetPort();
					}
				}
				return iTemp;
			}
		}
		case 2: { //Description asc
			if((item1->GetDescription() != NULL) && (item2->GetDescription() != NULL)) {
				//the 'if' is necessary, because the Description-String is not
				//always initialisized in server.cpp
				return wxString(char2unicode(item2->GetDescription())).CmpNoCase(char2unicode(item1->GetDescription()));
			} else if (item1->GetDescription() == NULL) {
				return 1;
			} else {
				return 0;
			}
		}
		case 102: { //Desciption desc
			if((item1->GetDescription() != NULL) && (item2->GetDescription() != NULL)) {
				return wxString(char2unicode(item1->GetDescription())).CmpNoCase(char2unicode(item2->GetDescription()));
			} else if (item1->GetDescription() == NULL) {
				return 1;
			} else {
				return 0;
			}
		}
		case 3: //Ping asc
			return item1->GetPing() - item2->GetPing();
		case 103: //Ping desc
			return item2->GetPing() - item1->GetPing();
		case 4: //Users asc
			return item1->GetUsers() - item2->GetUsers();
		case 104: //Users desc
			return item2->GetUsers() - item1->GetUsers();
		case 5: //Files asc
			return item1->GetFiles() - item2->GetFiles();
		case 105: //Files desc
			return item2->GetFiles() - item1->GetFiles();
		case 6: //Preferences asc
			return item2->GetPreferences() - item1->GetPreferences();
		case 106: //Preferences desc
			return item1->GetPreferences() - item2->GetPreferences();
		case 7: //failed asc
			return item1->GetFailedCount() - item2->GetFailedCount();
		case 107: //failed desc
			return item2->GetFailedCount() - item1->GetFailedCount();
		case 8: //staticservers
			return item2->IsStaticMember() - item1->IsStaticMember();
		case 108: //staticservers-
			return item1->IsStaticMember() - item2->IsStaticMember();
		case 9: // version
			return wxString(item1->GetVersion()).CmpNoCase(item2->GetVersion());
		case 109: //version-
			return wxString(item2->GetVersion()).CmpNoCase(item1->GetVersion());
		default:
			return 0;
	}
}

bool CServerListCtrl::StaticServerFileAppend(CServer *server)
{
	try {
		// Remove any entry before writing to avoid duplicates
		StaticServerFileRemove(server);
		wxString staticsfile(theApp.ConfigDir +wxT("staticservers.dat"));
		wxTextFile staticservers(staticsfile);
		bool error;
		if (wxFileExists(staticsfile)) {
			error = staticservers.Open();
		} else {
			error = staticservers.Create();
		}
		if (error) {
			theApp.amuledlg->AddLogLine( false, wxString(_("Failed to open staticservers.dat")));
			return false;
		}
		staticservers.AddLine(wxString::Format(wxT("%s:%i,%i,%s"),server->GetAddress(),server->GetPort(), server->GetPreferences(),server->GetListName()));
		theApp.amuledlg->AddLogLine(false, wxT("'%s:%i,%s' %s"), server->GetAddress(), server->GetPort(), server->GetListName(), _("Added to static server list"));
		server->SetIsStaticMember(true);
		theApp.amuledlg->serverwnd->serverlistctrl->RefreshServer(server);
		staticservers.Write();
		staticservers.Close();
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool CServerListCtrl::StaticServerFileRemove(CServer *server)
{
	try {
		if (!server->IsStaticMember()) {
			return true;
		}
		wxString strLine;
		wxString strTest;
		int pos;
		wxString StaticFilePath(theApp.ConfigDir +wxT("staticservers.dat"));
		wxString StaticTempPath(theApp.ConfigDir +wxT("statictemp.dat"));

		wxTextFile staticservers(StaticFilePath);
		wxTextFile statictemp(StaticTempPath);
		if (wxFileExists(StaticTempPath)) {
			wxRemoveFile(StaticTempPath);
		}
		if (!staticservers.Open() || !statictemp.Create()) {
			if ( staticservers.IsOpened() ) {
				staticservers.Close();
			}
			if ( statictemp.IsOpened() ) {
				statictemp.Close();
			}

			theApp.amuledlg->AddLogLine( false, _("Failed to open staticservers.dat"));
			return false;
		}
		for (wxString strLine = staticservers.GetFirstLine(); !staticservers.Eof(); strLine = staticservers.GetNextLine() ) {

			// ignore comments or invalid lines
			if (strLine.GetChar(0) == '#' || strLine.GetChar(0) == '/') {
				continue;
			}
			if (strLine.Length() < 5) {
				continue;
			}
			// Only interested in "host:port"
			pos = strLine.Find(wxT(","));
			if (pos == -1) {
				continue;
			}
			strLine = strLine.Left(pos);
			// Get host and port from given server
			strTest.Printf(wxT("%s:%i"), server->GetAddress(), server->GetPort());
			// Compare, if not the same server write original line to temp file
			if (strLine.Cmp(strTest) != 0) {
				statictemp.AddLine(strLine);
			}
		}
		staticservers.Close();
		statictemp.Write();
		statictemp.Close();

		// All ok, remove the existing file and replace with the new one
		wxRemoveFile( StaticFilePath );
		wxRenameFile( StaticTempPath, StaticFilePath );
	}
	catch (...) {
		return false;
	}
	return true;
}

void CServerListCtrl::ShowFilesCount()
{
	wxString fmtstr = wxString::Format(_("Servers (%i)"), GetItemCount());
	wxStaticCast(FindWindowByName(wxT("serverListLabel")),wxStaticText)->SetLabel(fmtstr);
}
