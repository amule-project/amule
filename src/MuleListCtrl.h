// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

#ifndef MULELISTCTRL_H
#define MULELISTCTRL_H

#include "listctrl_gen.h"	// Needed for wxODListCtrl
#include "types.h"		// Needed for LPCTSTR
#include "color.h"		// Needed for COLORREF

//////////////////////////////////
// CMuleListCtrl

/* Minimum column width */
#if defined(__WXGTK__)
	#define COL_MIN_SIZE 10
#elif defined(__WXMSW__)
	#define COL_MIN_SIZE 0
#elif defined(__WXMAC__)
	#define COL_MIN_SIZE 0	
#else 
	#error Need to set COL_MIN_SIZE for your OS.
#endif

#ifdef __WXMSW__
#include <wx/listctrl.h>
class CMuleListCtrl : public wxListCtrl {
#else
class CMuleListCtrl : public wxODListCtrl {
#endif

  //  	DECLARE_DYNAMIC(CMuleListCtrl)
  DECLARE_DYNAMIC_CLASS(CMuleListCtrl)

public:
	CMuleListCtrl();
	CMuleListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);
	virtual ~CMuleListCtrl();

	// Sets the list name, used for hide/show menu
	void SetNamaMule(LPCTSTR lpszName);

	// Forces the control to repaint a specific item.
	virtual void Update(int nItem);

	// Hide the column
	void HideColumn(int iColumn);

	// Unhide the column
	void ShowColumn(int iColumn);

	// check to see if the column is hidden
	bool IsColumnHidden(int iColumn) const {
		if(iColumn < 1 || iColumn >= m_iColumnsTracked)
			return false;

		return m_aColumns[iColumn].bHidden;
	}

	// Get the correct column width even if column is hidden
	int GetColumnWidth(int iColumn) const {
		if(iColumn < 0 || iColumn >= m_iColumnsTracked)
			return 0;
		
		if(m_aColumns[iColumn].bHidden)
			return m_aColumns[iColumn].iWidth;
		else
	#ifdef __WXMSW__
			return wxListCtrl::GetColumnWidth(iColumn);
	#else
			return wxODListCtrl::GetColumnWidth(iColumn);
	#endif
	}

	#if 0
	// Call SetRedraw to allow changes to be redrawn or to prevent changes from being redrawn.
	void SetRedraw(bool bRedraw = TRUE) {
		if(bRedraw) {
			if(m_iRedrawCount > 0 && --m_iRedrawCount == 0)
				wxODListCtrl::SetRedraw(TRUE);
		} else {
			if(m_iRedrawCount++ == 0)
				wxODListCtrl::SetRedraw(FALSE);
		}
	}
	#endif

	//save to preferences
	void SaveSettings();

	//load from preferences
	void LoadSettings();

	enum ArrowType { arrowDown, arrowUp, arrowDoubleDown, arrowDoubleUp };
	DECLARE_EVENT_TABLE()
protected:
	virtual bool ProcessEvent(wxEvent& evt);
	virtual void PreSubclassWindow();
	//virtual bool OnWndMsg(unsigned int message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//virtual bool OnChildNotify(unsigned int message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual int TablePrefs();

#if 0
	DECLARE_MESSAGE_MAP()
	afx_msg void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg bool OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSysColorChange();
#endif
	void OnMouseWheel(wxMouseEvent &event);
	void SetColors();
	void SetSortArrow(int iColumn, ArrowType atType);
	void SetSortArrow(int iColumn, bool bAscending) {
		SetSortArrow(iColumn, bAscending ? arrowUp : arrowDown);
	}
#if 0
	DWORD        SetExtendedStyle(DWORD dwNewStyle) {
		return CListCtrl::SetExtendedStyle(dwNewStyle | LVS_EX_HEADERDRAGDROP);
	}
#endif

	wxString          m_Name;
	//NMLVCUSTOMDRAW   m_lvcd;
	wxMenu *m_ColumnMenu;
	bool             m_bCustomDraw;
	COLORREF         m_crWindow;
	COLORREF         m_crWindowText;
	COLORREF         m_crHighlight;
	COLORREF         m_crFocusLine;
	COLORREF         m_crNoHighlight;
	COLORREF         m_crNoFocusLine;
	void OnColumnRclick(wxListEvent& evt);
private:
	//static int IndexToOrder(CHeaderCtrl* pHeader, int iIndex);

	struct MULE_COLUMN {
		int iWidth;
		int iLocation;
		bool bHidden;
	};

	int          m_iColumnsTracked;
	MULE_COLUMN *m_aColumns;

	int GetHiddenColumnCount() const {
		int iHidden = 0;
		for(int i = 0; i < m_iColumnsTracked; i++)
			if(m_aColumns[i].bHidden)
				iHidden++;
		return iHidden;
	}

	int       m_iCurrentSortItem;
	ArrowType m_atSortArrow;

	int m_iRedrawCount;
	int m_col_minsize;
};

#endif // MULELISTCTRL_H
