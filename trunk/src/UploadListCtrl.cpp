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

// UploadListCtrl.cpp : implementation file
//

#include "UploadListCtrl.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CastItoXBytes
#include "amule.h"			// Needed for theApp
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "ChatWnd.h"		// Needed for CChatWnd
#include "PartFile.h"		// Needed for CPartFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "ClientCredits.h"	// Needed for CClientCredits
#include "updownclient.h"	// Needed for CUpDownClient
#include "opcodes.h"		// Needed for MP_DETAIL
#include "muuli_wdr.h"		// Needed for ID_UPLOADLIST
#include "color.h"		// Needed for G_BLEND and SYSCOLOR

// CUploadListCtrl

BEGIN_EVENT_TABLE(CUploadListCtrl, CMuleListCtrl)
	EVT_RIGHT_DOWN(CUploadListCtrl::OnNMRclick)
	EVT_LIST_COL_CLICK(ID_UPLOADLIST,CUploadListCtrl::OnColumnClick)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CUploadListCtrl, CMuleListCtrl)
CUploadListCtrl::CUploadListCtrl()
{
}

CUploadListCtrl::CUploadListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: CMuleListCtrl(parent,id,pos,siz,flags|wxLC_OWNERDRAW)
{
	m_ClientMenu=NULL;

	wxColour col=wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	wxColour newcol=wxColour(G_BLEND(col.Red(),125),G_BLEND(col.Green(),125),G_BLEND(col.Blue(),125));
	m_hilightBrush=new wxBrush(newcol,wxSOLID);

	col=wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
	newcol=wxColour(G_BLEND(col.Red(),125),G_BLEND(col.Green(),125),G_BLEND(col.Blue(),125));
	m_hilightUnfocusBrush=new wxBrush(newcol,wxSOLID);

	Init();
	
	for (uint32 i=0; i<22; i++) {
		imagelist.Add(wxBitmap(clientImages(i)));
	}
	#if 0
	// load images
	imagelist.Add(wxBitmap(clientImages(7)));
	imagelist.Add(wxBitmap(clientImages(5)));
	imagelist.Add(wxBitmap(clientImages(12)));
	imagelist.Add(wxBitmap(clientImages(13)));
	imagelist.Add(wxBitmap(clientImages(6)));
	imagelist.Add(wxBitmap(clientImages(8)));
	imagelist.Add(wxBitmap(clientImages(14)));
	imagelist.Add(wxBitmap(clientImages(11)));
	imagelist.Add(wxBitmap(clientImages(15)));
	imagelist.Add(wxBitmap(clientImages(16)));
	imagelist.Add(wxBitmap(clientImages(17)));
	imagelist.Add(wxBitmap(clientImages(18)));
	imagelist.Add(wxBitmap(clientImages(19)));
	imagelist.Add(wxBitmap(clientImages(20)));
	#endif
}

void CUploadListCtrl::Init()
{
	#if 0
	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	#endif
	InsertColumn(0,CString(_("Username")),wxLIST_FORMAT_LEFT,150); //,0);
	InsertColumn(1,CString(_("File")),wxLIST_FORMAT_LEFT,275);//,1);
	InsertColumn(2,CString(_("Speed")),wxLIST_FORMAT_LEFT,60);//,2);
	InsertColumn(3,CString(_("Transferred")),wxLIST_FORMAT_LEFT,65);//,3);
	InsertColumn(4,CString(_("Waited")),wxLIST_FORMAT_LEFT,60);//,4);
	InsertColumn(5,CString(_("Upload Time")),wxLIST_FORMAT_LEFT,60);//,6);
	InsertColumn(6,CString(_("Status")),wxLIST_FORMAT_LEFT,110);//,5);
	InsertColumn(7,CString(_("Obtained Parts")),wxLIST_FORMAT_LEFT,100);

	#if 0
	imagelist.Create(16,16,ILC_COLOR32|ILC_MASK,0,10);
	imagelist.SetBkColor(RGB(255,255,255));
	imagelist.Add(theApp.LoadIcon(IDI_USER0));
	imagelist.Add(theApp.LoadIcon(IDI_COMPPROT));
	imagelist.Add(theApp.LoadIcon(IDI_PLUS));
	imagelist.Add(theApp.LoadIcon(IDI_PLUSCOMPROT));
	SetImageList(&imagelist,LVSIL_SMALL);
	#endif
	// not here.. no preferences yet
	//LoadSettings(CPreferences::tableUpload);
}

void CUploadListCtrl::InitSort()
{
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableUpload);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableUpload);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
}

CUploadListCtrl::~CUploadListCtrl()
{
	delete m_hilightBrush;
	delete m_hilightUnfocusBrush;
}

void CUploadListCtrl::Localize()
{
}

void CUploadListCtrl::OnNMRclick(wxMouseEvent& evt)
{
	// Check if clicked item is selected. If not, unselect all and select it.
	long item=-1;
	int lips=0;
	int index=HitTest(evt.GetPosition(), lips);
	if (index != -1) {
		if (!GetItemState(index, wxLIST_STATE_SELECTED)) {
			for (;;) {
				item = GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				if (item==-1) {
					break;
				}
				SetItemState(item, 0, wxLIST_STATE_SELECTED);
			}
			SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		}
	}
	if(m_ClientMenu==NULL) {
		wxMenu* menu=new wxMenu(CString(_("Clients")));
		menu->Append(MP_DETAIL,CString(_("Show &Details")));
		menu->Append(MP_ADDFRIEND,CString(_("Add to Friends")));
		menu->Append(MP_SHOWLIST,CString(_("View Files")));
		menu->AppendSeparator();
		menu->Append(MP_SWITCHCTRL,CString(_("Show Queue")));
		m_ClientMenu=menu;
	}
	
	m_ClientMenu->Enable(MP_DETAIL,(index!=-1));
	m_ClientMenu->Enable(MP_ADDFRIEND,(index!=-1));
	m_ClientMenu->Enable(MP_SHOWLIST,(index!=-1));
	
	PopupMenu(m_ClientMenu,evt.GetPosition());
}

void CUploadListCtrl::AddClient(CUpDownClient* client)
{
	uint32 itemnr = GetItemCount();

	//itemnr = InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,client->GetUserName(),0,0,1,(LPARAM)client);
	itemnr=InsertItem(itemnr,client->GetUserName());
	SetItemData(itemnr,(long)client);

	wxListItem myitem;
	myitem.m_itemId=itemnr;
	// FIXME
	myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
	SetItem(myitem);

	RefreshClient(client);
}

void CUploadListCtrl::RemoveClient(CUpDownClient* client)
{
	//LVFINDINFO find;
	//find.flags = LVFI_PARAM;
	//find.lParam = (LPARAM)client;
	//sint32 result = FindItem(&find);
	sint32 result=FindItem(-1,(long)client);
	if (result != (-1)) {
		DeleteItem(result);
	}
}

void CUploadListCtrl::RefreshClient(CUpDownClient* client)
{
	sint32 itemnr=FindItem(-1,(long)client);
	if (itemnr == (-1)) {
		return;
	}

	char buffer[100];
	CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	if (file) {
		SetItem(itemnr,1,file->GetFileName());
	} else {
		SetItem(itemnr,1,"?");
	}
	sprintf(buffer,"%.1f kB/s",client->GetKBpsUp());
	SetItem(itemnr,2,buffer);
	SetItem(itemnr,3,CastItoXBytes(client->GetTransferedUp()));
	SetItem(itemnr,5,CastSecondsToHM((client->GetUpStartTimeDelay())/1000));
	wxString status;
	switch (client->GetUploadState()) {
		case US_CONNECTING:
			status = CString(_("Connecting"));
			break;
		case US_WAITCALLBACK:
			status = CString(_("Connecting via server"));
			break;
		case US_UPLOADING:
			status = CString(_("Transferring"));
			break;
		default:
			status = CString(_("Unknown"));
	}
	SetItem(itemnr,6,status);
	SetItem(itemnr,7,"Bar is missing :)");
}

bool CUploadListCtrl::ProcessEvent(wxEvent& evt)
{
	if(evt.GetEventType()!=wxEVT_COMMAND_MENU_SELECTED) {
		return CMuleListCtrl::ProcessEvent(evt);
	}
	
	wxCommandEvent& event=(wxCommandEvent&)evt;
	int cursel=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	if (cursel != (-1)) {
		CUpDownClient* client = (CUpDownClient*)GetItemData(cursel);
		switch (event.GetId()) {
			case MP_SHOWLIST:
				client->RequestSharedFileList();
				return true;
				break;
			case MP_ADDFRIEND: {
				// int pos;
				// while (pos!=-1) {
				theApp.amuledlg->chatwnd->AddFriend(client);
				// pos=GetNextItem(pos,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
				// }
				return true;
				break;
			}
			case MP_DETAIL: {
				CClientDetailDialog* dialog =new CClientDetailDialog(this,client);
				dialog->ShowModal();
				delete dialog;
				return true;				
				break;
			}
		}
	}
	switch(event.GetId()) {
		case MP_SWITCHCTRL: {
			theApp.amuledlg->transferwnd->SwitchUploadList(event);
			return true;			
		}
	}
	// Kry - Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);	
}

void CUploadListCtrl::OnColumnClick( wxListEvent& evt)
{
	// NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// if it's a second click on the same column then reverse the sort order,
	// Barry - Store sort order in preferences
        // Determine ascending based on whether already sorted on this column

	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableUpload);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableUpload);
	bool sortAscending = (sortItem != evt.GetColumn()) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = evt.GetColumn();
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableUpload, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableUpload, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	// otherwise sort the new column in ascending order.
	//*pResult = 0;
}

int CUploadListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
{
	CUpDownClient* item1 = (CUpDownClient*)lParam1;
	CUpDownClient* item2 = (CUpDownClient*)lParam2;
	CKnownFile* file1 = theApp.sharedfiles->GetFileByID(item1->GetUploadFileID());
	CKnownFile* file2 = theApp.sharedfiles->GetFileByID(item2->GetUploadFileID());
	switch(lParamSort) {
		case 0:
			return CString(item1->GetUserName()).CmpNoCase(item2->GetUserName());
		case 100:
			return CString(item2->GetUserName()).CmpNoCase(item1->GetUserName());
		case 1:
			if( (file1 != NULL) && (file2 != NULL)) {
				return CString(file1->GetFileName()).CmpNoCase(file2->GetFileName());
			} else if (file1 == NULL) {
				return 1;
			} else {
				return 0;
			}
		case 101:
			if( (file1 != NULL) && (file2 != NULL)) {
				return CString(file2->GetFileName()).CmpNoCase(file1->GetFileName());
			} else if (file1 == NULL) {
				return 1;
			} else {
				return 0;
			}
		case 2:
			return int(item1->GetKBpsUp() - item2->GetKBpsUp());
		case 102:
			return int(item2->GetKBpsUp() - item1->GetKBpsUp());
		case 3:
			return item1->GetTransferedUp() - item2->GetTransferedUp();
		case 103:
			return item2->GetTransferedUp() - item1->GetTransferedUp();
		case 4:
			return item1->GetWaitTime() - item2->GetWaitTime();
		case 104:
			return item2->GetWaitTime() - item1->GetWaitTime();
		case 5:
			return item1->GetUpStartTimeDelay() - item2->GetUpStartTimeDelay();
		case 105:
			return item2->GetUpStartTimeDelay() - item1->GetUpStartTimeDelay();
		case 6:
			return item1->GetUploadState() - item2->GetUploadState();
		case 106:
			return item2->GetUploadState() - item1->GetUploadState();
		case 7:
			return item1->GetUpPartCount() - item2->GetUpPartCount();
		case 107:
			return item2->GetUpPartCount() - item1->GetUpPartCount();
		default:
			return 0;
	}
}

void CUploadListCtrl::OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted)
{
	/* Don't do any drawing if we not being watched. */
	//if ((theApp.amuledlg->transferwnd->windowtransferstate) || (theApp.amuledlg->GetActiveDialog() != 2)) {
	if (!theApp.amuledlg->IsRunning() || (theApp.amuledlg->transferwnd->windowtransferstate) || (theApp.amuledlg->GetActiveDialog() != IDD_TRANSFER)) {
		return;
	}
	if(highlighted) {
		if(GetFocus()) {
			dc->SetBackground(*m_hilightBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		} else {
			dc->SetBackground(*m_hilightUnfocusBrush);
			dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		}
	} else {
		dc->SetBackground(*(wxTheBrushList->FindOrCreateBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX),wxSOLID)));
		dc->SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	}
	// fill it
	wxPen mypen;
	if(highlighted) {
		// set pen so that we'll get nice border
		wxColour old=GetFocus()?m_hilightBrush->GetColour():m_hilightUnfocusBrush->GetColour();
		wxColour newcol=wxColour(((int)old.Red()*65)/100, ((int)old.Green()*65)/100, ((int)old.Blue()*65)/100);
		mypen=wxPen(newcol,1,wxSOLID);
		dc->SetPen(mypen);
	} else {
		dc->SetPen(*wxTRANSPARENT_PEN);
	}
	dc->SetBrush(dc->GetBackground());
	dc->DrawRectangle(rectHL);
	dc->SetPen(*wxTRANSPARENT_PEN);

	// then stuff from original amule
	CUpDownClient* client = (CUpDownClient*)GetItemData(item);//lpDrawItemStruct->itemData;
	//CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC),&CRect(lpDrawItemStruct->rcItem));
	//dc.SelectObject(GetFont());
	//COLORREF crOldTextColor = dc->GetTextColor();
	//COLORREF crOldBkColor = dc->GetBkColor();
	RECT cur_rec;
	//memcpy(&cur_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));
	cur_rec.left=rect.x;
	cur_rec.top=rect.y;
	cur_rec.right=rect.x+rect.width;
	cur_rec.bottom=rect.y+rect.height;

	// lagloose
	CString rbuffer;
	// end lagloose

	CString Sbuffer;
	CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
	//CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = GetColumnCount(); //pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;

	for(int iCurrent = 0; iCurrent < iCount; iCurrent++) {
		int iColumn = iCurrent; //pHeaderCtrl->OrderToIndex(iCurrent);
		if(1) { // !IsColumnHidden(iColumn)) { (column's can't be hidden :)
			wxListItem listitem;
			GetColumn(iColumn,listitem);
			int cx=listitem.GetWidth();
			cur_rec.right += cx; //GetColumnWidth(iColumn);
			wxDCClipper* clipper=new wxDCClipper(*dc,cur_rec.left,cur_rec.top,cur_rec.right-cur_rec.left,cur_rec.bottom-cur_rec.top);
			switch(iColumn) {
				case 0: {
					
					
					uint8 clientImage;
					if (client->IsFriend()) {
						clientImage = 13;
					} else {
						switch (client->GetClientSoft()) {
							case SO_AMULE: 
								clientImage = 17;
								break;
							case SO_MLDONKEY:
							case SO_NEW_MLDONKEY:
							case SO_NEW2_MLDONKEY:
								clientImage = 15;
								break;
							case SO_EDONKEY:
							case SO_EDONKEYHYBRID:
								// Maybe we would like to make different icons?
								clientImage = 16;
								break;
							case SO_EMULE:
								clientImage = 14;
								break;
							case SO_LPHANT:
								clientImage = 18;
								break;
							case SO_SHAREAZA:
								clientImage = 19;
								break;
							case SO_LXMULE:
								clientImage = 20;
								break;
							default:
								// cDonkey, Compat Unk
								// No icon for those yet. Using the eMule one + '?'
								clientImage = 21;
								break;
						}	
					}
					
					imagelist.Draw(clientImage,*dc,cur_rec.left,cur_rec.top+1,wxIMAGELIST_DRAW_TRANSPARENT);
										
					if (client->credits->GetScoreRatio(client->GetIP()) > 1) {					
						// Has credits, draw the gold star
						// (wtf is the grey star?)
						imagelist.Draw(11,*dc,cur_rec.left,cur_rec.top+1,wxIMAGELIST_DRAW_TRANSPARENT);						
					} else if (client->ExtProtocolAvailable()) {
						// Ext protocol -> Draw the '+'
						imagelist.Draw(7,*dc,cur_rec.left,cur_rec.top+1,wxIMAGELIST_DRAW_TRANSPARENT);
					}		
					
					Sbuffer.Format("%s", client->GetUserName());
					cur_rec.left +=20;
					//dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
					dc->DrawText(Sbuffer,cur_rec.left,cur_rec.top+3);
					cur_rec.left -=20;
					break;
				}
				case 1:
					if(file) {
						Sbuffer.Format("%s", file->GetFileName().GetData());
					} else {
						Sbuffer = "?";
					}
					break;
				case 2:
					// lagloose
					if (client->GetDownloadState() == DS_DOWNLOADING) {
						Sbuffer = "";
						rbuffer.Format("%.1f",client->GetKBpsUp());
						Sbuffer.Append(rbuffer);
						Sbuffer.Append("/");
						rbuffer.Format("%.1f %s",client->GetKBpsDown(), CString(_("kB/s")).GetData());
						Sbuffer.Append(rbuffer);
					} else {
						Sbuffer.Format("%.1f %s",client->GetKBpsUp(),CString(_("kB/s")).GetData());
					}
					// end lagloose
					break;
				case 3:
					Sbuffer.Format("%s",CastItoXBytes(client->GetSessionUp()).GetData());
					break;
				case 4:
					Sbuffer.Format("%s",CastSecondsToHM((client->GetWaitTime())/1000).GetData());
					break;
				case 5:
					Sbuffer.Format("%s",CastSecondsToHM((client->GetUpStartTimeDelay())/1000).GetData());
					break;
				case 6:
					switch (client->GetUploadState()) {
						case US_CONNECTING:
							Sbuffer = CString(_("Connecting"));
							break;
						case US_WAITCALLBACK:
							Sbuffer = CString(_("Connecting via server"));
							break;
						case US_UPLOADING:
							// lagloose
							Sbuffer = "<-- ";
							Sbuffer.Append(CString(_("Transferring")));
							if (client->GetDownloadState() == DS_DOWNLOADING) {
								Sbuffer.Append(" -->");
							}
							// Sbuffer = CString(_("Transferring"));
							// end lagloose
							break;
						default:
							Sbuffer = CString(_("Unknown"));
					}
					break;
				case 7:
					if( client->GetUpPartCount()) {
						wxMemoryDC memdc;
						cur_rec.bottom--;
						cur_rec.top++;
						int iWidth = cur_rec.right - cur_rec.left;
						int iHeight = cur_rec.bottom - cur_rec.top;
						if(iWidth>0 && iHeight>0) {
							// don't draw if it is too narrow
							wxBitmap memBmp(iWidth,iHeight);
							memdc.SelectObject(memBmp);
							memdc.SetPen(*wxTRANSPARENT_PEN);
							memdc.SetBrush(*wxWHITE_BRUSH);
							memdc.DrawRectangle(wxRect(0,0,iWidth,iHeight));
							client->DrawUpStatusBar(&memdc,wxRect(0,0,iWidth,iHeight),false,false);
							dc->Blit(cur_rec.left,cur_rec.top,iWidth,iHeight,&memdc,0,0);
							memdc.SelectObject(wxNullBitmap);
						}
						cur_rec.bottom++;
						cur_rec.top--;
					// } else {
					}
					break;
			}
			if( iColumn != 7 && iColumn != 0 ) {
				//dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
				dc->DrawText(Sbuffer,cur_rec.left,cur_rec.top+3);
			}
			cur_rec.left += cx;//GetColumnWidth(iColumn);
			delete clipper;
		}
	}
}

void CUploadListCtrl::ShowSelectedUserDetails()
{
	if (GetSelectedItemCount()>0) {
		int sel=GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
		CUpDownClient* client = (CUpDownClient*)GetItemData(sel);
		if (client) {
			CClientDetailDialog dialog(this,client);
			dialog.ShowModal();
		}
	}
}
