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

// QueueListCtrl.cpp : implementation file
//

#include "muuli_wdr.h"		// Needed for ID_QUEUELIST
#include "QueueListCtrl.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CastSecondsToHM
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "ClientDetailDialog.h"	// Needed for CClientDetailDialog
#include "ChatWnd.h"		// Needed for CChatWnd
#include "PartFile.h"		// Needed for CPartFile
#include "ClientCredits.h"	// Needed for CClientCredits
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "updownclient.h"	// Needed for CUpDownClient
#include "opcodes.h"		// Needed for MP_DETAIL
#include "amule.h"			// Needed for theApp
#include "color.h"		// Needed for SYSCOLOR

// CQueueListCtrl

//IMPLEMENT_DYNAMIC(CQueueListCtrl, CMuleListCtrl)
BEGIN_EVENT_TABLE(CQueueListCtrl,CMuleListCtrl)
  EVT_RIGHT_DOWN(CQueueListCtrl::OnNMRclick)
  EVT_LIST_COL_CLICK(ID_QUEUELIST,CQueueListCtrl::OnColumnClick)
  EVT_TIMER(2349,CQueueListCtrl::OnTimer)
END_EVENT_TABLE()

CQueueListCtrl::CQueueListCtrl(){
}

CQueueListCtrl::CQueueListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
  : CMuleListCtrl(parent,id,pos,siz,flags|wxLC_OWNERDRAW)
{
  m_ClientMenu=NULL;

  wxColour col=wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
  wxColour newcol=wxColour(((int)col.Red()*125)/100,
			   ((int)col.Green()*125)/100,
			   ((int)col.Blue()*125)/100);
  m_hilightBrush=new wxBrush(newcol,wxSOLID);

  col=wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
  newcol=wxColour(((int)col.Red()*125)/100,
		  ((int)col.Green()*125)/100,
		  ((int)col.Blue()*125)/100);
  m_hilightUnfocusBrush=new wxBrush(newcol,wxSOLID);

  Init();

  // load images
#warning As with uploadlistctrl and downloadlistctrl, this list should be centralised on amuleDlg, or even better, not used at all.
	
	for (uint32 i=0; i<22; i++) {
		imagelist.Add(wxBitmap(clientImages(i)));
	}
	
  m_hTimer.SetOwner(this,2349);
  m_hTimer.Start(10000);
}

void CQueueListCtrl::InitSort()
{
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
}

// paha lametus :)
#define LVCFMT_LEFT wxLIST_FORMAT_LEFT

void CQueueListCtrl::Init(){
	InsertColumn(0,CString(_("Username")),LVCFMT_LEFT,150);
	InsertColumn(1,CString(_("File")),LVCFMT_LEFT,275);
	InsertColumn(2,CString(_("File Priority")),LVCFMT_LEFT,110);
	InsertColumn(3,CString(_("Rating")),LVCFMT_LEFT,60);
	InsertColumn(4,CString(_("Score")),LVCFMT_LEFT,60);
	InsertColumn(5,CString(_("Asked")),LVCFMT_LEFT,60);
	InsertColumn(6,CString(_("Last Seen")),LVCFMT_LEFT,110);
	InsertColumn(7,CString(_("Entered Queue")),LVCFMT_LEFT,110);
	InsertColumn(8,CString(_("Banned")),LVCFMT_LEFT,60);
	InsertColumn(9,CString(_("Obtained Parts")),LVCFMT_LEFT,100);

#if 0
	// something that can't be done here and now. moved to InitSort()
	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:10));
#endif

#if 0
	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	InsertColumn(0,CString(_("Username")),LVCFMT_LEFT,150,0);
	InsertColumn(1,CString(_("File")),LVCFMT_LEFT,275,1);
	InsertColumn(2,CString(_("File Priority")),LVCFMT_LEFT,110,2);
	InsertColumn(3,CString(_("Rating")),LVCFMT_LEFT,60,3);
	InsertColumn(4,CString(_("Score")),LVCFMT_LEFT,60,4);
	InsertColumn(5,CString(_("Asked")),LVCFMT_LEFT,60,5);
	InsertColumn(6,CString(_("Last Seen")),LVCFMT_LEFT,110,6);
	InsertColumn(7,CString(_("Entered Queue")),LVCFMT_LEFT,110,7);
	InsertColumn(8,CString(_("Banned")),LVCFMT_LEFT,60,8);
	imagelist.Create(16,16,ILC_COLOR32|ILC_MASK,0,10);
	imagelist.SetBkColor(RGB(255,255,255));
	imagelist.Add(theApp.LoadIcon(IDI_USER0));
	imagelist.Add(theApp.LoadIcon(IDI_COMPPROT));
	imagelist.Add(theApp.LoadIcon(IDI_PLUS));
	imagelist.Add(theApp.LoadIcon(IDI_PLUSCOMPROT));
	SetImageList(&imagelist,LVSIL_SMALL);

	m_ClientMenu.CreatePopupMenu();
	m_ClientMenu.AddMenuTitle(CString(_("Clients")));
	m_ClientMenu.AppendMenu(MF_STRING,MP_DETAIL, CString(_("Show &Details")));
	m_ClientMenu.AppendMenu(MF_STRING,MP_UNBAN, CString(_("Unban")));

	m_ClientMenu.AppendMenu(MF_SEPARATOR);
	m_ClientMenu.AppendMenu(MF_STRING,MP_SWITCHCTRL, CString(_("Show Uploads")));
	SetMenu(&m_ClientMenu);
	LoadSettings(CPreferences::tableQueue);
#endif
}

CQueueListCtrl::~CQueueListCtrl(){
  m_hTimer.Stop();

  delete m_hilightBrush;
  delete m_hilightUnfocusBrush;
}

void CQueueListCtrl::OnNMRclick(wxMouseEvent& evt)
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
		menu->Append(MP_UNBAN,CString(_("Unban")));
		menu->Append(MP_SHOWLIST,CString(_("View Files")));
		menu->AppendSeparator();
		menu->Append(MP_SWITCHCTRL,CString(_("Show Uploads")));
		m_ClientMenu=menu;
	} 

	m_ClientMenu->Enable(MP_DETAIL,(index!=-1));
	m_ClientMenu->Enable(MP_ADDFRIEND,(index!=-1));
	m_ClientMenu->Enable(MP_SHOWLIST,(index!=-1));	
	m_ClientMenu->Enable(MP_UNBAN,(index!=-1));	// Need to check if it's banned next time
	
	PopupMenu(m_ClientMenu,evt.GetPosition());
}

void CQueueListCtrl::Localize() {
#if 0
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;

	if(pHeaderCtrl->GetItemCount() != 0) {
		CString strRes;

		strRes = CString(_("Username"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(0, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("File"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(1, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("File Priority"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(2, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Rating"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(3, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Score"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(4, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Asked"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(5, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Last Seen"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(6, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Entered Queue"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(7, &hdi);
		strRes.ReleaseBuffer();

		strRes = CString(_("Banned"));
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(8, &hdi);
		strRes.ReleaseBuffer();
	}
#endif
}

void CQueueListCtrl::AddClient(CUpDownClient* client){
  uint32 itemnr=GetItemCount();
  itemnr=InsertItem(itemnr,client->GetUserName());
  SetItemData(itemnr,(long)client);

  wxListItem myitem;
  myitem.m_itemId=itemnr;
  myitem.SetBackgroundColour(SYSCOLOR(wxSYS_COLOUR_LISTBOX));
  SetItem(myitem);

  RefreshClient(client);
}

void CQueueListCtrl::RemoveClient(CUpDownClient* client){
  sint32 itemnr=FindItem(-1,(long)client);
  if(itemnr!=(-1)) 
    DeleteItem(itemnr);
}

void CQueueListCtrl::RefreshClient(CUpDownClient* client) {
  sint16 result=FindItem(-1,(long)client);
  if(result!=-1) {
    long first,last;
    first=last=0;
    #ifdef __WXMSW__ // Lets hope it won't be too flickery/cpuheavy
	RefreshItem(result);
    #else
        GetVisibleLines(&first,&last);
        if(result>=first && result<=last) {
           RefreshItems(result,result);
        }
    #endif
  }
}

void CQueueListCtrl::OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted)
{
	/* Don't do any drawing if there's nobody watching. */
	if ((!theApp.amuledlg->transferwnd->windowtransferstate) || (theApp.amuledlg->GetActiveDialog() != IDD_TRANSFER)) {
		return;
	}

  if(!theApp.amuledlg->IsRunning())
    return;

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
    wxColour newcol=wxColour(((int)old.Red()*65)/100,
			     ((int)old.Green()*65)/100,
			     ((int)old.Blue()*65)/100);
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
    
    CString Sbuffer;
    //	char buffer[100];
    
    CKnownFile* file = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
    //CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
    int iCount =GetColumnCount();// pHeaderCtrl->GetItemCount();
    cur_rec.right = cur_rec.left - 8;
    cur_rec.left += 4;
    
    for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
      int iColumn = iCurrent;//pHeaderCtrl->OrderToIndex(iCurrent);
      if( 1 ){ // columns can't be hidden
	wxListItem listitem;
	GetColumn(iColumn,listitem);
	int cx=listitem.GetWidth();
	cur_rec.right += cx; //GetColumnWidth(iColumn);
	wxDCClipper* clipper=new wxDCClipper(*dc,cur_rec.left,cur_rec.top,cur_rec.right-cur_rec.left,
					     cur_rec.bottom-cur_rec.top);
	switch(iColumn){
	case 0:{
		
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
		dc->DrawText(Sbuffer,cur_rec.left,cur_rec.top+3);
		cur_rec.left -=20;
		break;
	}
	case 1:
	  if(file)
	    Sbuffer.Format("%s", file->GetFileName().GetData());
	  else
	    Sbuffer = "?";
	  break;
	case 2:
	  if(file){
	    switch(file->GetUpPriority()){
	    case PR_POWERSHARE:                    //added for powershare (deltaHF)
	      Sbuffer.Format("%s",CString(_("PowerShare[Release]")).GetData());
	      break; //end
	    case PR_VERYHIGH:
	      Sbuffer.Format("%s",CString(_("Very High")).GetData());
	      break;
	    case PR_HIGH: 
	      Sbuffer.Format("%s",CString(_("High")).GetData());
	      break; 
	    case PR_LOW: 
	      Sbuffer.Format("%s",CString(_("Low")).GetData());
	      break; 
	    case PR_VERYLOW:
	      Sbuffer.Format("%s",CString(_("Very low")).GetData());
	      break;
	    default: 
	      Sbuffer.Format("%s",CString(_("Normal")).GetData());
	      break; 
	    }
	  }
	  else
	    Sbuffer = "?";
	  break;
	case 3:
	  Sbuffer.Format("%.1f",(float)client->GetScore(false,false,true));
	  break;
	case 4:
		if (client->HasLowID()){
			if (client->m_bAddNextConnect) {
				Sbuffer.Format("%i ****",client->GetScore(false));
			} else {
                  	Sbuffer.Format("%i "+CString(_("LowID")),client->GetScore(false));
			}
		} else {
			Sbuffer.Format("%i",client->GetScore(false));
		}
		break;		
	  break;
	case 5:
	  Sbuffer.Format("%i",client->GetAskedCount());
	  break;
	case 6:
	  Sbuffer.Format("%s", CastSecondsToHM((::GetTickCount() - client->GetLastUpRequest())/1000).GetData());
	  break;
	case 7:
	  Sbuffer.Format("%s", CastSecondsToHM((::GetTickCount() - client->GetWaitStartTime())/1000).GetData());
	  break;
	case 8:
	  if(client->IsBanned())
	    Sbuffer.Format("%s",CString(_("Yes")).GetData());
	  else
	    Sbuffer.Format("%s",CString(_("No")).GetData());
	  break;
	case 9:
	  if( client->GetUpPartCount()){
	    wxMemoryDC memdc;
	    cur_rec.bottom--;
	    cur_rec.top++;
	    int iWidth = cur_rec.right - cur_rec.left;	
	    int iHeight = cur_rec.bottom - cur_rec.top;
	    wxBitmap memBmp(iWidth,iHeight);
	    memdc.SelectObject(memBmp);
	    memdc.SetPen(*wxTRANSPARENT_PEN);
	    memdc.SetBrush(*wxWHITE_BRUSH);
	    memdc.DrawRectangle(wxRect(0,0,iWidth,iHeight));
	    client->DrawUpStatusBar(&memdc,wxRect(0,0,iWidth,iHeight),false,false);
	    dc->Blit(cur_rec.left,cur_rec.top,iWidth,iHeight,&memdc,0,0);
	    memdc.SelectObject(wxNullBitmap);
	    cur_rec.bottom++;
	    cur_rec.top--;
	  }
	  break;
	}
	if( iColumn != 9 && iColumn != 0)
	  //dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS);
	  dc->DrawText(Sbuffer,cur_rec.left,cur_rec.top+3);
	cur_rec.left += cx;//GetColumnWidth(iColumn);
	delete clipper;
      }
    }
}

#if 0
BEGIN_MESSAGE_MAP(CQueueListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT (NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
END_MESSAGE_MAP()
#endif


#if 0
// CQueueListCtrl message handlers
void CQueueListCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult){	
	POINT point;
	::GetCursorPos(&point);	
	m_ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = 0;
}
#endif

bool CQueueListCtrl::ProcessEvent(wxEvent& evt)
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
				theApp.amuledlg->chatwnd->AddFriend(client);
				return true;				
				break;
			}
			case MP_UNBAN: {
				if(client->IsBanned()) {
					client->UnBan();
				}
				return true;				
				break;
			}
			case MP_DETAIL: {
				CClientDetailDialog* dialog=new CClientDetailDialog(this,client);
				dialog->ShowModal();
				delete dialog;
				return true;				
				break;
			}
		}
	}
	switch(event.GetId()) {
		case MP_SWITCHCTRL: {
			//((CTransferWnd*)GetParent())->SwitchUploadList();
			theApp.amuledlg->transferwnd->SwitchUploadList(event);
			return true;			
			break;
		}
	}
	// Kry - Column hiding & misc events
	return CMuleListCtrl::ProcessEvent(evt);	
}

void CQueueListCtrl::OnColumnClick( wxListEvent& evt)
{

//	theApp.uploadqueue->ResetClientPos();
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);

	while(update) {
		RefreshClient( update);
		update = theApp.uploadqueue->GetNextClient(update);
	}
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableQueue);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableQueue);
	bool sortAscending = (sortItem != evt.GetColumn()) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = evt.GetColumn();
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableQueue, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableQueue, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
}

int CQueueListCtrl::SortProc(long lParam1, long lParam2, long lParamSort)
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
			} else if(file1 == NULL) {
				return 1;
			} else {
				return 0;
			}
		case 101:
			if( (file1 != NULL) && (file2 != NULL)) {
				return CString(file2->GetFileName()).CmpNoCase(file1->GetFileName());
			} else if(file1 == NULL) {
				return 1;
			} else {
				return 0;
			}
		case 2:
			if( (file1 != NULL) && (file2 != NULL)) {
				return file1->GetUpPriority() - file2->GetUpPriority();
			} else if(file1 == NULL) {
				return 1;
			} else {
				return 0;
			}
		case 102:
			if( (file1 != NULL) && (file2 != NULL)) {
				return file2->GetUpPriority() - file1->GetUpPriority();
			} else if( file1 == NULL ) {
				return 1;
			} else {
				return 0;
			}
		case 3:
			return (int)((float)item1->GetScore(false,false,true) - (float)item2->GetScore(false,false,true));
		case 103:
			return (int)((float)item2->GetScore(false,false,true) - (float)item1->GetScore(false,false,true));
		case 4:
			return item1->GetScore(false) - item2->GetScore(false);
		case 104:
			return item2->GetScore(false) - item1->GetScore(false);
		case 5:
			return item1->GetAskedCount() - item2->GetAskedCount();
		case 105:
			return item2->GetAskedCount() - item1->GetAskedCount();
		case 6:
			return item1->GetLastUpRequest() - item2->GetLastUpRequest();
		case 106:
			return item2->GetLastUpRequest() - item1->GetLastUpRequest();
		case 7:
			return item1->GetWaitStartTime() - item2->GetWaitStartTime();
		case 107:
			return item2->GetWaitStartTime() - item1->GetWaitStartTime();
		case 8:
			return item1->IsBanned() - item2->IsBanned();
		case 108:
			return item2->IsBanned() - item1->IsBanned();

		default:
			return 0;
	}
}

void CQueueListCtrl::OnTimer(wxTimerEvent& evt)
{
	// Don't do anything if the app is shutting down - can cause unhandled exceptions
	if (!theApp.amuledlg->IsRunning() || !theApp.glob_prefs->GetUpdateQueueList()) {
		return;
	}
  
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
	while(update) {
		theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(update);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}

// Barry - Refresh the queue every 10 secs
void  CQueueListCtrl::QueueUpdateTimer() //HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime)
{
	// Don't do anything if the app is shutting down - can cause unhandled exceptions
	if (!theApp.amuledlg->IsRunning() || !theApp.glob_prefs->GetUpdateQueueList()) {
		return;
	}
  
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);
	while(update) {
		theApp.amuledlg->transferwnd->queuelistctrl->RefreshClient(update);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}

void CQueueListCtrl::ShowSelectedUserDetails()
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
