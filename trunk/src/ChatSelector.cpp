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

// ChatSelector.cpp : implementation file
//

#ifdef __WXMAC__
	#include <wx/wx.h>
#endif
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _

#include "pixmaps/chat.ico.xpm"
#include "ChatSelector.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for nstrdup
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "packets.h"		// Needed for Packet
#include "opcodes.h"		// Needed for OP_MESSAGE
#include "ListenSocket.h"	// Needed for CClientReqSocket
#include "SysTray.h"		// Needed for TBN_CHAT
#include "Preferences.h"	// Needed for CPreferences
#include "ChatWnd.h"		// Needed for CChatWnd
#ifdef __WXMSW__
	#include <wx/msw/winundef.h> // Needed to be able to include wx headers
#endif
#include "muuli_wdr.h"		// Needed for IDC_CHATSELECTOR
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "updownclient.h"	// Needed for CUpDownClient
#include "color.h"		// Needed for RGB
#include "SafeFile.h"	// Needed for CSafeMemFile

CChatItem::CChatItem(){
	log = 0;
	messagepending = 0;
	notify = false;
}

// CChatSelector

IMPLEMENT_DYNAMIC_CLASS(CChatSelector, wxNotebook)
CChatSelector::CChatSelector()
{
	//m_pCloseBtn = NULL;
	//m_pMessageBox = NULL;
	//m_pSendBtn = NULL;
	lastemptyicon=false;
}

CChatSelector::CChatSelector(wxWindow* parent,wxWindowID id,const wxPoint& pos,wxSize siz,long style)
: wxNotebook(parent,id,pos,siz,style)
{
}

CChatSelector::~CChatSelector()
{
	//KillTimer();
}


#if 0
BEGIN_MESSAGE_MAP(CChatSelector, CTabCtrl)
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelchangeChatsel)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CCLOSE, OnBnClickedCclose)
	ON_BN_CLICKED(IDC_CSEND, OnBnClickedCsend)
END_MESSAGE_MAP()
#endif



// CChatSelector message handlers
void CChatSelector::Init()
{
	imagelist.Add(wxBitmap(chat_ico_xpm));
	SetImageList(&imagelist);
}

CChatItem* CChatSelector::StartSession(CUpDownClient* client, bool show){
	if (GetTabByClient(client) != 0xFFFF){
		if (show){
		  //SetCurSel(GetTabByClient(client));
		  SetSelection(GetTabByClient(client));
		  //chatout.SetHyperText(GetItemByClient(client)->log);
		  SetHyperText(GetItemByClient(client)->log);
		}
		return 0;
	}

	CChatItem* chatitem = new CChatItem();
	chatitem->client = client;
	chatitem->log = new CPreparedHyperText();

	m_items.AddTail(chatitem);

	//CTime theTime = CTime::GetCurrentTime();
	wxString sessions = CString(_("*** Chatsession Start : "))+(client->GetUserName()) + " - "+" (aMule client)\n";//+theTime.Format("%c")+ "\n";
	chatitem->log->AppendKeyWord(CString(sessions),RGB(255,0,0));
	client->SetChatState(MS_CHATTING);

	/*
	TCITEM newitem;
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)chatitem;
	newitem.pszText = client->GetUserName();
	newitem.cchTextMax = (int)strlen(client->GetUserName())+1;
	newitem.iImage = 0;
	uint16 itemnr = InsertItem(GetItemCount(),&newitem);
	*/
	wxPanel* nullPanel=new wxPanel(theApp.amuledlg->chatwnd->FindWindowById(IDC_CHATSELECTOR),-1);
	//int itemnr=
	    AddPage(nullPanel,client->GetUserName(),false,0);

	if (show){
	  //SetCurSel(itemnr);
	  SetSelection(GetPageCount()-1);
	  SetHyperText(chatitem->log);
	  //chatout.SetHyperText(chatitem->log);
	}
	return chatitem;
}

void CChatSelector::SetHyperText(CPreparedHyperText* htxt)
{
	m_curText=htxt;

	wxHtmlWindow* wnd=(wxHtmlWindow*)theApp.amuledlg->chatwnd->FindWindowById(ID_HTMLWIN);
	
	if(htxt==NULL) {
		wnd->SetPage("<html><body></body></html>");
		return;
	}
	// format prepared hypertext into html
	wxString text="<html><body>";
	wxString body=htxt->GetText();

	for (unsigned int i=0; i < body.Length(); ++i) {
		std::list<CKeyWord>::iterator itK=htxt->GetKeywords().begin();
		while(itK!=htxt->GetKeywords().end()) {
			if(i==itK->Begin()) {
				wxChar colref[256];
				sprintf(colref,"#%06x",itK->Color());
				text+="<font color=\""+wxString(colref)+"\">";
			}
			if(i==itK->End()) {
				text+="</font>";
			}
			itK++;
		}
		if(body.GetChar(i)=='\n') {
			text+="<br>\n";
		} else {
			text+=body.GetChar(i);
		}
	}
	text+="</body></html>";
	wnd->SetPage(text);
	
	int x,y;
	wnd->GetVirtualSize(&x,&y);
	wnd->Scroll(0,y);

}

uint16 CChatSelector::GetTabByClient(CUpDownClient* client)
{
	if (m_items.GetSize() > 0)
	{
		for (int i = 0; i != GetPageCount();i++) {
			CChatItem *item=m_items.GetAt(m_items.FindIndex(i));
			if(item->client==client) {
				return i;
			}
		}
	}
	return (uint16)-1;
}

CChatItem* CChatSelector::GetItemByClient(CUpDownClient* client)
{
	if(m_items.GetSize()<1) {
		return NULL;
	}

	for (int i = 0; i != GetPageCount();i++) {
		CChatItem* item=m_items.GetAt(m_items.FindIndex(i));
		if(item->client==client) {
			return item;
		}
	}
	return 0;
}

void CChatSelector::ProcessMessage(CUpDownClient* sender, char* message)
{
	//filter me?
	CString Cmessage=CString(CString(message).MakeLower());
	CString resToken;

	// continue
	CChatItem* ci = GetItemByClient(sender);
	bool isNewChatWindow = false;
	if (!ci) {
		ci = StartSession(sender,true);
		isNewChatWindow = true;
	}
	ci->log->AppendKeyWord(CString(sender->GetUserName()),RGB(50,200,250));
	ci->log->AppendText(CString(": "));
	ci->log->AppendText(CString(CString(message)+ CString("\n")));
	if (GetSelection() == GetTabByClient(sender) && GetParent()->IsShown()) {
		SetHyperText(ci->log);
	} else {
		ci->notify = true;
	        if (isNewChatWindow || theApp.glob_prefs->GetNotifierPopsEveryChatMsg()) {
			theApp.amuledlg->ShowNotifier(CString(_("Message from"))+CString(" ")+CString(sender->GetUserName()) + CString(":'") + CString(message)+ CString("'\n"), TBN_CHAT);
		}
		isNewChatWindow = false;
	}
}

bool CChatSelector::SendMessage(char* message)
{
	
	sint16 to = GetSelection();
	if (to == (-1)) {
		return false;
	}
	if(GetPageCount()<1) {
		return false;
	}
	CChatItem* ci=m_items.GetAt(m_items.FindIndex(to));
	if (ci->client->GetChatState() == MS_CONNECTING) {
		return false;
	}
	if (ci->client->socket && ci->client->socket->IsConnected()) {
		CMemFile data;
		data.Write(wxString(message));
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		//uint16 mlen = (uint16)strlen(message);
		//Packet* packet = new Packet(OP_MESSAGE,mlen+2);
		//memcpy(packet->pBuffer+2,message,mlen);
		//memcpy(packet->pBuffer,&mlen,2);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet,true,true);
		ci->log->AppendKeyWord(CString(theApp.glob_prefs->GetUserNick()),RGB(1,180,20));
		ci->log->AppendText(CString(": "));
		ci->log->AppendText(CString(CString(message)+CString("\n")));
	} else {
		printf("Not connected to Chat. Trying to connect...\n");
		ci->log->AppendKeyWord(CString(CString("*** ")+CString(_("Connecting"))),RGB(255,0,0));
		ci->messagepending = nstrdup(message);
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect();
		printf("Chat Connected\n");
	}
	if(GetHyperText()==ci->log) {
		SetHyperText(ci->log);
	}
#warning Creteil?  I know you are here Creteil... follow the white rabbit.
/* Madcat - knock knock ...
	   ,-.,-.
            \ \\ \
             \ \\_\
             /     \
          __|    a a|
        /`   `'. = y)=
       /        `"`}
     _|    \       }
    { \     ),   //
     '-',  /__\ ( (
   jgs (______)\_)_)
*/
// FIXME!
	//if (chatout.GetHyperText() == ci->log) {
	//  //chatout.UpdateSize(true);
	//}


	return true;
}

void CChatSelector::ConnectingResult(CUpDownClient* sender,bool success)
{
	CChatItem* ci = GetItemByClient(sender);
	if (!ci) {
		return;
	}
	ci->client->SetChatState(MS_CHATTING);
	if (!success) {
		if (ci->messagepending) {
			ci->log->AppendKeyWord(CString(CString(" ")+CString(_("failed")) +CString("\n")),RGB(255,0,0));
			delete[] ci->messagepending;
		} else {
			ci->log->AppendKeyWord(CString(CString(_("*** Disconnected")) +CString("\n")),RGB(255,0,0));
		}
		ci->messagepending = 0;
	} else {
		ci->log->AppendKeyWord(CString(" ok\n"),RGB(255,0,0));
		CMemFile data;
		data.Write(wxString(ci->messagepending));
		Packet* packet = new Packet(&data);
		packet->opcode = OP_MESSAGE;
		//uint16 mlen = (uint16)strlen(ci->messagepending);
		//Packet* packet = new Packet(OP_MESSAGE,mlen+2);
		//memcpy(packet->pBuffer+2,ci->messagepending,mlen);
		//memcpy(packet->pBuffer,&mlen,2);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		ci->client->socket->SendPacket(packet,true,true);
		ci->log->AppendKeyWord(CString(theApp.glob_prefs->GetUserNick()),RGB(1,180,20));
		ci->log->AppendText(CString(": "));
		ci->log->AppendText(CString(CString(ci->messagepending)+CString("\n")));
		delete[] ci->messagepending;
		ci->messagepending = 0;
	}
	if(GetHyperText()==ci->log) {
		SetHyperText(ci->log);
	}
	//	if (chatout.GetHyperText() == ci->log)
	//	chatout.UpdateSize(true);
}

void CChatSelector::DeleteAllItems()
{
#if 0
	for (int i = 0; i != GetItemCount();i++) {
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		GetItem(i,&cur_item);
		delete (CChatItem*)cur_item.lParam;
	}
#endif
}

void CChatSelector::ShowChat()
{
}


#if 0
INT CChatSelector::InsertItem(int nItem/*,TCITEM* pTabCtrlItem*/)
{
  return itemnumber.
}
#endif

bool CChatSelector::DeleteItem(int nItem)
{
	return true;
}

void CChatSelector::EndSession(CUpDownClient* client)
{
	sint16 usedtab;
	if (client) {
		usedtab = GetTabByClient(client);
	} else {
		usedtab = GetSelection();
	}
	if(GetPageCount()<1) {
		usedtab=(-1);
	}

	if (usedtab == (-1)) {
		return;
	}
	CChatItem* ci=m_items.GetAt(m_items.FindIndex(usedtab));
	ci->client->SetChatState(MS_NONE);
	DeletePage(usedtab);
	m_items.RemoveAt(m_items.FindIndex(usedtab));
	if (GetHyperText() == ci->log) {
		SetHyperText(NULL);
	}
	delete ci;
}

void CChatSelector::Localize(void)
{
}


// hypertext stuff...
// CHyperLink

CHyperLink::CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, const CString& sCommand, const CString& sDirectory)
{
	m_Type = lt_Shell;
	m_iBegin = iBegin;
	m_iEnd = iEnd;
	m_sTitle = sTitle;
	m_sCommand = sCommand;
	m_sDirectory = sDirectory;
	// [i_a] used for lt_Message
	m_uMsg = 0;
	m_wParam = 0;
	m_lParam = 0;
}

CHyperLink::CHyperLink()
{
	m_Type = lt_Unknown;
	m_iBegin = 0;
	m_iEnd = 0;
	m_sTitle.Empty();
	m_sCommand.Empty();
	m_sDirectory.Empty();
	m_uMsg = 0;
	m_wParam = 0;
	m_lParam = 0;
}

CHyperLink::CHyperLink(const CHyperLink& Src){
	m_Type = Src.m_Type;
	m_iBegin = Src.m_iBegin;
	m_iEnd = Src.m_iEnd;
	m_sTitle = Src.m_sTitle;
	m_sCommand = Src.m_sCommand;
	m_sDirectory = Src.m_sDirectory;
	m_uMsg = Src.m_uMsg;
	m_wParam = Src.m_wParam;
	m_lParam = Src.m_lParam;
}

void CHyperLink::Execute()
{
}

// CKeyWord

CKeyWord::CKeyWord(int iBegin, uint16 iEnd, COLORREF icolor)
{
	color = icolor;
	m_iBegin = iBegin;
	m_iEnd = iEnd;
}

// CPreparedHyperText

void CPreparedHyperText::PrepareText(const CString& sText)
{
	m_sText = sText;
	m_Links.clear();

	enum {
		unknown,
		space,
		http0,		/* http://		*/
		http1, http2, http3, http4, http5, http6,
		ftp0,		/* ftp://		*/
		ftp1, ftp2, ftp3, ftp4, ftp5,
		ftp,		/* ftp.			*/
		www0,		/* www.			*/
		www1, www2, www3,
		mailto0, 	/* mailto:		*/
		mailto1, mailto2, mailto3, mailto4, mailto5, mailto6,
		mail,		/* xxx@yyy		*/
		ed2k0,		/* ed2k://		*/
		ed2k1, ed2k2, ed2k3, ed2k4, ed2k5, ed2k6
	} state = space;

	int WordPos = 0;
	wxChar sz[2];
	wxChar& c = sz[0];
	sz[1] = 0;
	int last = m_sText.GetLength() -1;
	for(int i = 0; i <= last; i++) {
		c = m_sText.GetChar(i);
		//_tcslwr(sz);

		switch(state) {
			case unknown:
				if(tspace(c)) {
					state = space;
				} else if(c == _T('@') && WordPos != i) {
					state = mail;
				}
				break;

			case space:
				WordPos = i;
				switch(c)
				{
			case _T('h'): state = http0; break;
			case _T('f'): state = ftp0; break;
			case _T('w'): state = www0; break;
			case _T('m'): state = mailto0; break;
			case _T('e'): state = ed2k0; break;
			default:
				if(!tspace(c))
					state = unknown;
			}
			break;

			/*----------------- http -----------------*/
		case http0:
			if(c == _T('t'))
				state = http1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http1:
			if(c == _T('t'))
				state = http2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http2:
			if(c == _T('p'))
				state = http3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http3:
			if(c == _T(':'))
				state = http4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http4:
			if(c == _T('/'))
				state = http5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http5:
			if(c == _T('/'))
				state = http6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case http6:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;

			/*----------------- ed2k -----------------*/
		case ed2k0:
			if(c == _T('d'))
				state = ed2k1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k1:
			if(c == _T('2'))
				state = ed2k2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k2:
			if(c == _T('k'))
				state = ed2k3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k3:
			if(c == _T(':'))
				state = ed2k4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k4:
			if(c == _T('/'))
				state = ed2k5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k5:
			if(c == _T('/'))
				state = ed2k6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ed2k6:{
			if(/*tspace(c)*/ i == last || (isspace(c) && m_sText.GetChar(i-1)==_T('/')) )
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
}
			break;
			/*----------------- ftp -----------------*/
		case ftp0:
			if(c == _T('t'))
				state = ftp1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp1:
			if(c == _T('p'))
				state = ftp2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp2:
			if(c == _T(':'))
				state = ftp3;
			else
				if(c == _T('.'))
					state = ftp;
				else
					if(tspace(c))
						state = space;
					else
						state = unknown;
			break;

		case ftp3:
			if(c == _T('/'))
				state = ftp4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp4:
			if(c == _T('/'))
				state = ftp5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case ftp:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString((_T("ftp://")) + m_sText.Mid(WordPos, len));
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;

		case ftp5:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;

			/*----------------- www -----------------*/
		case www0:
			if(c == _T('w'))
				state = www1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www1:
			if(c == _T('w'))
				state = www2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www2:
			if(c == _T('.'))
				state = www3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case www3:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString((_T("http://")) + m_sText.Mid(WordPos, len));
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;

			/*----------------- mailto -----------------*/
		case mailto0:
			if(c == _T('a'))
				state = mailto1;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto1:
			if(c == _T('i'))
				state = mailto2;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto2:
			if(c == _T('l'))
				state = mailto3;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto3:
			if(c == _T('t'))
				state = mailto4;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto4:
			if(c == _T('o'))
				state = mailto5;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto5:
			if(c == _T(':'))
				state = mailto6;
			else
				if(tspace(c))
					state = space;
				else
					state = unknown;
			break;

		case mailto6:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = m_sText.Mid(WordPos, len);
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;

			/*----------------- mailto -----------------*/
		case mail:
			if(tspace(c) || i == last)
			{
				int len = i == last ? i - WordPos + 1 : i - WordPos;
				CString s = CString((_T("mailto:")) + m_sText.Mid(WordPos, len));
				RemoveLastSign(s);
				m_Links.push_back(CHyperLink(WordPos, WordPos + len - 1, s, s, ""));
				state = space;
			}
			break;
		}
	}

	m_Links.sort();
}

 void CPreparedHyperText::RemoveLastSign(CString& sLink)
{
	int len = sLink.GetLength();
	if(len > 0)
	{
	  TCHAR c = sLink.GetChar(len-1);//sLink[len-1];
		switch(c)
		{
		case _T('.'):
		case _T(','):
		case _T(';'):
		case _T('\"'):
		case _T('\''):
		case _T('('):
		case _T(')'):
		case _T('['):
		case _T(']'):
		case _T('{'):
		case _T('}'):
			sLink.Remove(len -1, 1);
			break;
		}
	}
}

CPreparedHyperText::CPreparedHyperText(const CString& sText){
	PrepareText(sText);
}

CPreparedHyperText::CPreparedHyperText(const CPreparedHyperText& src){
	m_sText = src.m_sText;
	m_Links.assign(src.m_Links.begin(), src.m_Links.end());
}

void CPreparedHyperText::Clear(){
	m_sText.Empty();
	m_Links.erase(m_Links.begin(), m_Links.end());
}

void CPreparedHyperText::SetText(const CString& sText){
	Clear();
	PrepareText(sText);
}

void CPreparedHyperText::AppendText(const CString& sText){
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	CPreparedHyperText ht(sText);
	m_sText+=sText;
	for(std::list<CHyperLink>::iterator it = ht.m_Links.begin(); it != ht.m_Links.end(); it++)
	{
		CHyperLink hl = *it;
		hl.m_iBegin += len;
		hl.m_iEnd += len;
		m_Links.push_back(hl);
	}
}

void CPreparedHyperText::AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory){
	if (!(sText.GetLength() && sCommand.GetLength()))
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_Links.push_back(CHyperLink(len, len + sText.GetLength() - 1, sTitle, sCommand, sDirectory));
}

/*
void CPreparedHyperText::AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	if (!sText.GetLength())
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_Links.push_back(CHyperLink(len, len + sText.GetLength() - 1, sTitle, hWnd, uMsg, wParam, lParam));
}
*/

void CPreparedHyperText::AppendKeyWord(const CString& sText, COLORREF iColor){
	if (!sText.GetLength())
		return;
	int len = m_sText.GetLength();
	////////////////////////////////////////////////
	//Top:The Original code didn't check to see if the buffer was full..
	////////////////////////////////////////////////
	bool flag = true;
	if( len > 60000 ){
		m_sText = m_sText.Right(50000);
		int shift = len - m_sText.GetLength();
		while( flag == true ){
			CHyperLink &test = m_Links.front();
			if( !m_Links.empty() ){
				if( test.Begin() < shift )
					m_Links.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		flag = true;
		while( flag == true ){
			CKeyWord &test = m_KeyWords.front();
			if( !m_KeyWords.empty() ){
				if( test.Begin() < shift )
					m_KeyWords.pop_front();
				else
					flag = false;
			}
			else
				flag = false;
		}
		len = m_sText.GetLength();
		CHyperLink &ltest = m_Links.front();
		int litest = ltest.Begin() - shift;
		CKeyWord &wtest = m_KeyWords.front();
		int witest = wtest.Begin() - shift;
		flag = true;
		while( flag == true && !m_Links.empty() ){
			CHyperLink &temp = m_Links.front();
			CHyperLink backup( temp);
			backup.SetBegin( backup.Begin() - shift );
			backup.SetEnd( backup.End() - shift );
			m_Links.pop_front();
			m_Links.push_back( backup );
			if( ((CHyperLink)m_Links.front()).Begin() == litest )
				flag = false;
		}
		flag = true;
		while( flag == true && !m_KeyWords.empty() ){
			CKeyWord &temp = m_KeyWords.front();
			CKeyWord backup( temp.Begin()-shift, temp.End()-shift, temp.Color());
			m_KeyWords.pop_front();
			m_KeyWords.push_back( backup );
			if( ((CKeyWord)m_KeyWords.front()).Begin() == witest )
				flag = false;
		}
	}
	////////////////////////////////////////////////
	//Bottom: May not be the nicest code but it works.
	////////////////////////////////////////////////
	m_sText+=sText;
	m_KeyWords.push_back(CKeyWord(len, len + sText.GetLength() - 1, iColor));
}

//CLinePartInfo

 CLinePartInfo::CLinePartInfo(int iBegin, uint16 iEnd, CHyperLink* pHyperLink, CKeyWord* pKeyWord){
	m_xBegin = iBegin;
	m_xEnd = iEnd;
	m_pHyperLink = pHyperLink;
	m_pKeyWord = pKeyWord;
}

 CLinePartInfo::CLinePartInfo(const CLinePartInfo& Src){
	m_xBegin = Src.m_xBegin;
	m_xEnd = Src.m_xEnd;
	m_pHyperLink = Src.m_pHyperLink;
	m_pKeyWord = Src.m_pKeyWord;
}


//CLineInfo

 CLineInfo::CLineInfo(int iBegin, uint16 iEnd){
	m_iBegin = iBegin;
	m_iEnd = iEnd;
}

 CLineInfo::CLineInfo(const CLineInfo& Src){
	m_iBegin = Src.m_iBegin;
	m_iEnd = Src.m_iEnd;
	assign(Src.begin(), Src.end());
}


//CVisPart
 CVisPart::CVisPart(const CLinePartInfo& LinePartInfo, const RECT& rcBounds, int iRealBegin, uint16 iRealLen,CVisPart* pPrev,CVisPart* pNext) : CLinePartInfo(LinePartInfo)
{
	m_rcBounds = rcBounds;
	m_iRealBegin = iRealBegin;
	m_iRealLen = iRealLen;
	m_pPrev = pPrev;
	m_pNext = pNext;
}

 CVisPart::CVisPart(const CVisPart& Src) : CLinePartInfo(Src){
	m_rcBounds = Src.m_rcBounds;
	m_iRealBegin = Src.m_iRealBegin;
	m_iRealLen = Src.m_iRealLen;
	m_pPrev = Src.m_pPrev;
	m_pNext = Src.m_pNext;
}
