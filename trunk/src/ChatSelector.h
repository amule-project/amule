//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#ifndef CHATSELECTOR_H
#define CHATSELECTOR_H

#include <list>
#include <vector>
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/notebook.h>	// Needed for wxNotebook
#include <wx/html/htmlwin.h>	// Needed for wxHtmlWindow
#include <wx/imaglist.h>	// Needed for wxImageList (at least on wx2.5)

#include "types.h"		// Needed for uint16
#include "CString.h"		// Needed for CString
#include "color.h"		// Needed for COLORREF
#include "CTypedPtrList.h"	// Needed for CList

class CUpDownClient;

//namespace HyperTextControl 
//{

#define HTC_WORDWRAP			1	// word wrap text
#define HTC_AUTO_SCROLL_BARS		2	// auto hide scroll bars
#define HTC_UNDERLINE_LINKS		4	// underline links
#define HTC_UNDERLINE_HOVER		8	// underline hover links
#define HTC_ENABLE_TOOLTIPS		16	// enable hyperlink tool tips

// --------------------------------------------------------------
// CHyperLink

class CHyperLink{
	friend class CPreparedHyperText;
public:
	CHyperLink(); // i_a 
	CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, const CString& sCommand, const CString& sDirectory);
	//CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam);
	CHyperLink(const CHyperLink& Src);

	void Execute();
	 bool operator < (const CHyperLink& Arg)		{return m_iEnd < Arg.m_iEnd;}
	 uint16 Begin()									{return m_iBegin;}
	 uint16 End()									{return m_iEnd;}
	 uint16 Len()									{return m_iEnd - m_iBegin + 1;}
	 CString Title()								{return m_sTitle;}
	 void SetBegin( uint16 m_iInBegin )				{m_iBegin = m_iInBegin;}
	 void SetEnd( uint16 m_iInEnd )					{m_iEnd = m_iInEnd;}

protected:
	int m_iBegin;
	int m_iEnd;
	CString m_sTitle;

	enum LinkType
	{
		lt_Unknown = 0,  // i_a 
		lt_Shell = 0, /* http:// mailto:*/
		lt_Message = 1 /* WM_COMMAND */
	} m_Type;

	// used for lt_Shell
	CString m_sCommand;
	CString m_sDirectory;
	// used for lt_Message
	//HWND m_hWnd; 
	unsigned int m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;
};

// --------------------------------------------------------------
// CKeyWord

class CKeyWord{
	friend class CPreparedHyperText;
public:
	CKeyWord(int iBegin, uint16 iEnd, COLORREF icolor);

	 bool operator < (const CKeyWord& Arg)		{return m_iEnd < Arg.m_iEnd;}
	 uint16 Begin()				{return m_iBegin;}
	 uint16 End()					{return m_iEnd;}
	 void SetBegin( uint16 m_iInBegin )		{m_iBegin = m_iInBegin;}
	 void SetEnd( uint16 m_iInEnd )		{m_iEnd = m_iInEnd;}
	 COLORREF Color()				{return color;}
	 uint16 Len()					{return m_iEnd - m_iBegin + 1;}
protected:
	int m_iBegin;
	int m_iEnd;
	COLORREF color;
};

// --------------------------------------------------------------
// CPreparedHyperText

class CPreparedHyperText{
public:
	CPreparedHyperText()						{}
	CPreparedHyperText(const CString& sText);
	CPreparedHyperText(const CPreparedHyperText& src);

	void Clear();
	void SetText(const CString& sText);
	void AppendText(const CString& sText);
	void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory);
	//void AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam);
	void AppendKeyWord(const CString& sText, COLORREF iColor);

	 CString& GetText()					{return m_sText;}
	 std::list<CHyperLink>& GetLinks()	{return m_Links;}
	 std::list<CKeyWord>& GetKeywords()	{return m_KeyWords;}
	//friend class CHyperTextCtrl;

protected:
	CString m_sText;
	std::list<CHyperLink> m_Links;
	std::list<CKeyWord> m_KeyWords;

	void RemoveLastSign(CString& sLink);
	void PrepareText(const CString& sText);
	bool tspace(TCHAR c)						{return isspace(c) || /*c < _T(' ') || */c == _T(';') || c == _T('!');}

};
// --------------------------------------------------------------
// CLinePartInfo
class CLinePartInfo{
public:
	uint16 m_xBegin;
	uint16 m_xEnd;
	CHyperLink* m_pHyperLink;
	CKeyWord* m_pKeyWord;

	 CLinePartInfo(int iBegin, uint16 iEnd, CHyperLink* pHyperLink = NULL, CKeyWord* pKeyWord = NULL);
	 CLinePartInfo(const CLinePartInfo& Src);
	 uint16 Begin()							{return m_xBegin;}
	 uint16 End()							{return m_xEnd;}
	 uint16 Len()							{return ((m_xEnd - m_xBegin) + 1);}
};

// --------------------------------------------------------------
// CLineInfo
class CLineInfo : public std::vector<CLinePartInfo>{
public:
	int m_iBegin;
	int m_iEnd;

	 CLineInfo(int iBegin, uint16 iEnd);
	 CLineInfo(const CLineInfo& Src);
	 uint16 Begin()						{return m_iBegin;}
	 uint16 End()						{return m_iEnd;}
	 uint16 Len()						{return m_iEnd - m_iBegin + 1;}
};

// --------------------------------------------------------------
// CVisPart
class CVisPart : public CLinePartInfo {
public:
	RECT m_rcBounds;
	int m_iRealBegin;
	int m_iRealLen;
	CVisPart* m_pPrev;
	CVisPart* m_pNext;

	 CVisPart(const CLinePartInfo& LinePartInfo, const RECT& rcBounds, 
		int iRealBegin, uint16 iRealLen,CVisPart* pPrev,CVisPart* pNext);
	 CVisPart(const CVisPart& Src);
};

class CVisLine : public std::vector<CVisPart>
{	};


class CChatItem{
public:
	CChatItem();
	~CChatItem()		{ delete log; }
	CUpDownClient*		client;
	CPreparedHyperText*	log;
	char*			messagepending;
	bool			notify;
};
// CChatSelector

class CChatSelector : public wxNotebook
{
	DECLARE_DYNAMIC_CLASS(CChatSelector)

public:
	CChatSelector();
	CChatSelector(wxWindow* parent,wxWindowID id,const wxPoint& pos,wxSize siz,long style);
	virtual		~CChatSelector();
	void		Init();
	CChatItem*	StartSession(CUpDownClient* client, bool show = true);
	void		EndSession(CUpDownClient* client = 0);
	uint16		GetTabByClient(CUpDownClient* client);
	CChatItem*	GetItemByClient(CUpDownClient* client);
	//CHyperTextCtrl chatout;
	wxHtmlWindow* chatout;
	void		ProcessMessage(CUpDownClient* sender, char* message);
	bool		SendMessage(char* message);
	void		DeleteAllItems();
	void		ShowChat();
	void		ConnectingResult(CUpDownClient* sender,bool success);
	void		Send();
protected:
	void		OnTimer(unsigned int* nIDEvent);
	CList<CChatItem*,CChatItem*> m_items;
#if 0
	afx_msg void OnTcnSelchangeChatsel(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCsend();
	afx_msg void OnBnClickedCclose();
	DECLARE_MESSAGE_MAP()
#endif	
	//virtual INT		InsertItem(int nItem/*,TCITEM* pTabCtrlItem*/);
	virtual bool	DeleteItem(int nItem);
private:
	void SetHyperText(CPreparedHyperText* htxt);
	CPreparedHyperText* GetHyperText() {return m_curText;};

	CPreparedHyperText* m_curText;
	wxImageList	imagelist;
	unsigned int*	m_Timer;
	bool		blinkstate;
	bool		lastemptyicon;

	//CWnd		*m_pMessageBox;
	//CWnd		*m_pCloseBtn;
	//CWnd		*m_pSendBtn;
public:
	//afx_msg void OnSize(unsigned int nType, int cx, int cy);
	//virtual bool PreTranslateMessage(MSG* pMsg);
	void Localize(void);
};

#endif // CHATSELECTOR_H
