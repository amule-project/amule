// This file is part of the aMule project.
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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/menu.h>		// Needed for wxMenu
#include <wx/window.h>
#include <wx/msgdlg.h>

#include "MuleListCtrl.h"	// Interface declarations
#include "opcodes.h"		// Needed for MP_LISTCOL_1
#include "amule.h"			// Needed for theApp

#if 0
#define MLC_BLEND(A, B, X) ((A + B * (X-1) + ((X+1)/2)) / X)

#define MLC_RGBBLEND(A, B, X) (                   \
	RGB(MLC_BLEND(GetRValue(A), GetRValue(B), X), \
	MLC_BLEND(GetGValue(A), GetGValue(B), X),     \
	MLC_BLEND(GetBValue(A), GetBValue(B), X))     \
)

#define MLC_DT_TEXT (DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS)
#endif

#define MLC_MENU 650

//////////////////////////////////
// CMuleListCtrl

//IMPLEMENT_DYNAMIC(CMuleListCtrl, wxODListCtrl)
#ifdef __WXMSW__
IMPLEMENT_DYNAMIC_CLASS(CMuleListCtrl, wxListCtrl)
#else
IMPLEMENT_DYNAMIC_CLASS(CMuleListCtrl,wxODListCtrl)
#endif

CMuleListCtrl::CMuleListCtrl()
{
	m_bCustomDraw = false;
	m_iCurrentSortItem = -1;
	m_iColumnsTracked = 0;
	m_aColumns = NULL;

	#if defined(__WXGTK__)
		m_col_minsize = 10;
	#elif defined(__WXMSW__)
		m_col_minsize = 0;
	#elif defined(__WXMAC__)
		m_col_minsize = 0;
	#else
		#error Need to set col_minsize for ur OS
	#endif
}

#ifdef __WXMSW__
CMuleListCtrl::CMuleListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: wxListCtrl(parent,id,pos,siz,flags)
#else
CMuleListCtrl::CMuleListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags)
: wxODListCtrl(parent,id,pos,siz,flags)
#endif
{
	m_bCustomDraw = false;
	m_iCurrentSortItem = -1;
	m_iColumnsTracked = 0;
	m_aColumns = NULL;
	#if defined(__WXGTK__)
		m_col_minsize = 10;
	#elif defined(__WXMSW__)
		m_col_minsize = 0;
	#elif defined(__WXMAC__)
		m_col_minsize = 0;
	#else
		#error Need to set col_minsize for ur OS
	#endif
}

CMuleListCtrl::~CMuleListCtrl()
{
	if(m_aColumns != NULL) {
		delete[] m_aColumns;
	}
}

void CMuleListCtrl::SetNamaMule(LPCTSTR lpszName)
{
	m_Name = char2unicode(lpszName);
}

//new fix for old problem... normally Update(int) causes entire list to redraw
void CMuleListCtrl::Update(int iItem)
{
#if 0
	RECT rcItem;
	bool bResult = this->GetItemRect(iItem, &rcItem, LVIR_BOUNDS);
	if(bResult) {
		InvalidateRect(&rcItem, FALSE);
	}
	return bResult;
#endif
}

void CMuleListCtrl::PreSubclassWindow()
{
	#if 0
	SetColors();
	CListCtrl::PreSubclassWindow();
	ModifyStyle(LVS_SINGLESEL|LVS_LIST|LVS_ICON|LVS_SMALLICON,LVS_REPORT|TVS_LINESATROOT|TVS_HASBUTTONS); 
	SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
	#endif
}

#if 0
int CMuleListCtrl::IndexToOrder(CHeaderCtrl* pHeader, int iIndex)
{
	int iCount = pHeader->GetItemCount();
	int *piArray = new int[iCount];
	Header_GetOrderArray( pHeader->m_hWnd, iCount, piArray);
	for(int i=0; i < iCount; i++ ) {
		if(piArray[i] == iIndex) {
			delete[] piArray;
			return i;
		}
	}
	delete[] piArray;
	return -1;
}
#endif

void CMuleListCtrl::HideColumn(int iColumn)
{
	#if 0
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	if(iColumn < 1 || iColumn >= iCount || m_aColumns[iColumn].bHidden) {
		return;
	}
	//stop it from redrawing
	SetRedraw(FALSE);

	//shrink width to 0
	HDITEM item;
	item.mask = HDI_WIDTH;
	pHeaderCtrl->GetItem(iColumn, &item);
	m_aColumns[iColumn].iWidth = item.cxy;
	item.cxy = 0;
	pHeaderCtrl->SetItem(iColumn, &item);

	//move to front of list
	INT *piArray = new INT[m_iColumnsTracked];
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);

	int iFrom = m_aColumns[iColumn].iLocation;
	for(int i = 0; i < m_iColumnsTracked; i++) {
		if(m_aColumns[i].iLocation > m_aColumns[iColumn].iLocation && m_aColumns[i].bHidden) {
			iFrom++;
		}
	}

	for(int i = iFrom; i > 0; i--) {
		piArray[i] = piArray[i - 1];
	}
	piArray[0] = iColumn;
	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	delete[] piArray;

	//update entry
	m_aColumns[iColumn].bHidden = true;

	//redraw
	SetRedraw(TRUE);
	Invalidate(FALSE);
	#endif
}

void CMuleListCtrl::ShowColumn(int iColumn)
{
	#if 0
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	if(iColumn < 1 || iColumn >= iCount || !m_aColumns[iColumn].bHidden) {
		return;
	}

	//stop it from redrawing
	SetRedraw(FALSE);

	//restore position in list
	INT *piArray = new INT[m_iColumnsTracked];
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);
	int iCurrent = IndexToOrder(pHeaderCtrl, iColumn);

	for(; iCurrent < IndexToOrder(pHeaderCtrl, 0) && iCurrent < m_iColumnsTracked - 1; iCurrent++) {
		piArray[iCurrent] = piArray[iCurrent + 1];
	}
	for(; m_aColumns[iColumn].iLocation > m_aColumns[pHeaderCtrl->OrderToIndex(iCurrent + 1)].iLocation && iCurrent < m_iColumnsTracked - 1; iCurrent++) {
		piArray[iCurrent] = piArray[iCurrent + 1];
	}
	piArray[iCurrent] = iColumn;
	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	delete[] piArray;

	//and THEN restore original width
	HDITEM item;
	item.mask = HDI_WIDTH;
	item.cxy = m_aColumns[iColumn].iWidth;
	pHeaderCtrl->SetItem(iColumn, &item);

	//update entry
	m_aColumns[iColumn].bHidden = false;

	//redraw
	SetRedraw(TRUE);
	Invalidate(FALSE);
	#endif
}

void CMuleListCtrl::SaveSettings()
{
	CPreferences::Table tID = TablePrefs();
	int colTrack=GetColumnCount();

	INT *piArray = new INT[colTrack];

	for(int i = 0; i < colTrack; i++) {
		wxListItem mycol;
		GetColumn(i,mycol);
		// wxMessageBox(wxString::Format("%s - %i",mycol.GetText().c_str(), mycol.GetWidth()));
		theApp.glob_prefs->SetColumnWidth(tID, i, mycol.GetWidth());
		//theApp.glob_prefs->SetColumnHidden(tID, i, IsColumnHidden(i));
		//piArray[i] = m_aColumns[i].iLocation;
	}

	//theApp.glob_prefs->SetColumnOrder(tID, piArray);
	delete[] piArray;
}

void CMuleListCtrl::LoadSettings()
{
	//CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	CPreferences::Table tID = TablePrefs();
	int colTrack=GetColumnCount();

	INT *piArray = new INT[colTrack];
	for(int i = 0; i < colTrack; i++) {
		int iWidth = theApp.glob_prefs->GetColumnWidth(tID, i);
		if(iWidth != DEFAULT_COL_SIZE) {
			SetColumnWidth(i, iWidth);
			//wxListItem mycol;
			//mycol.SetWidth(iWidth);
			//SetColumn(i,mycol);
		}
		if(i == 0) {
			piArray[0] = 0;
		} else {
			int iOrder = theApp.glob_prefs->GetColumnOrder(tID, i);
			if(iOrder == 0) {
				piArray[i] = i;
			} else {
				piArray[i] = iOrder;
			}
		}
		//m_aColumns[i].iLocation = piArray[i];
	}

	//pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	delete[] piArray;

	#if 0
	for(int i = 1; i < m_iColumnsTracked; i++) {
		if(theApp.glob_prefs->GetColumnHidden(tID, i)) {
			HideColumn(i);
		}
	}
	#endif
}

void CMuleListCtrl::SetColors()
{
	#if 0
	m_crWindow      = ::GetSysColor(COLOR_WINDOW);
	m_crWindowText  = ::GetSysColor(COLOR_WINDOWTEXT);

	COLORREF crHighlight = ::GetSysColor(COLOR_HIGHLIGHT);
	m_crFocusLine   = crHighlight;
	m_crNoHighlight = MLC_RGBBLEND(crHighlight, m_crWindow, 8);
	m_crNoFocusLine = MLC_RGBBLEND(crHighlight, m_crWindow, 2);
	m_crHighlight   = MLC_RGBBLEND(crHighlight, m_crWindow, 4);
	#endif
}

void CMuleListCtrl::SetSortArrow(int iColumn, ArrowType atType)
{
	// integrated in listctrl..
	switch(atType) {
		case 263:
			#ifndef __WXMSW__
			wxODListCtrl::SetSortArrow(iColumn,(int)1);
			#endif
			break;
		default:
			#ifndef __WXMSW__
			wxODListCtrl::SetSortArrow(iColumn,(int)2);
			#endif
			break;
	}
	#if 0
	HDITEM headerItem;
	headerItem.mask = HDI_FORMAT | HDI_BITMAP;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	//delete old image if column has changed
	if(iColumn != m_iCurrentSortItem) {
		pHeaderCtrl->GetItem(m_iCurrentSortItem, &headerItem);
		headerItem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
		if (headerItem.hbm != 0) {
			DeleteObject(headerItem.hbm);
			headerItem.hbm = 0;
		}
		pHeaderCtrl->SetItem(m_iCurrentSortItem, &headerItem);
		m_iCurrentSortItem = iColumn;
	}

	//place new arrow unless we were given an invalid column
	if(iColumn >= 0 && pHeaderCtrl->GetItem(iColumn, &headerItem)) {
		m_atSortArrow = atType;
		if (headerItem.hbm != 0) {
			DeleteObject(headerItem.hbm);
			headerItem.hbm = 0;
		}
		headerItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
		headerItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(),MAKEINTRESOURCE(m_atSortArrow), IMAGE_BITMAP, 0, 0,LR_LOADMAP3DCOLORS);
		pHeaderCtrl->SetItem(iColumn, &headerItem);
	}
	#endif
}

#if 0
//lower level than everything else so poorly overriden functions don't break us
bool CMuleListCtrl::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	//lets look for the important messages that are essential to handle
	switch(message) {
	case WM_NOTIFY:
		if(wParam == 0) {
			if(((NMHDR*)lParam)->code == NM_RCLICK) {
				//catch right click on headers and show column menu

				POINT point;
				GetCursorPos (&point);

				CTitleMenu tmColumnMenu;
				tmColumnMenu.CreatePopupMenu();
				if(m_Name.GetLength() != 0)
					tmColumnMenu.AddMenuTitle(m_Name);

				CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
				int iCount = pHeaderCtrl->GetItemCount();
				for(int iCurrent = 1; iCurrent < iCount; iCurrent++) {
					HDITEM item;
					char text[255];
					item.pszText = text;
					item.mask = HDI_TEXT;
					item.cchTextMax = 255;
					pHeaderCtrl->GetItem(iCurrent, &item);

					tmColumnMenu.AppendMenu(MF_STRING | m_aColumns[iCurrent].bHidden ? 0 : MF_CHECKED,
						MLC_MENU + iCurrent, item.pszText);
				}
				tmColumnMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this); 
				tmColumnMenu.DestroyMenu();

				return *pResult = TRUE;

			} else if(((NMHDR*)lParam)->code == HDN_BEGINTRACKA || ((NMHDR*)lParam)->code == HDN_BEGINTRACKW) {
				//stop them from changeing the size of anything "before" first column

				HD_NOTIFY *pHDN = (HD_NOTIFY*)lParam;
				if(m_aColumns[pHDN->iItem].bHidden)
					return *pResult = TRUE;

			} else if(((NMHDR*)lParam)->code == HDN_ENDDRAG) {
				//stop them from moving first column

				NMHEADER *pHeader = (NMHEADER*)lParam;
				if(pHeader->iItem != 0 && pHeader->pitem->iOrder != 0) {

					int iNewLoc = pHeader->pitem->iOrder - GetHiddenColumnCount();
					if(iNewLoc > 0) {

						if(m_aColumns[pHeader->iItem].iLocation != iNewLoc) {

							if(m_aColumns[pHeader->iItem].iLocation > iNewLoc) {
								int iMax = m_aColumns[pHeader->iItem].iLocation;
								int iMin = iNewLoc;
								for(int i = 0; i < m_iColumnsTracked; i++) {
									if(m_aColumns[i].iLocation >= iMin && m_aColumns[i].iLocation < iMax)
										m_aColumns[i].iLocation++;
								}
							}

							else if(m_aColumns[pHeader->iItem].iLocation < iNewLoc) {
								int iMin = m_aColumns[pHeader->iItem].iLocation;
								int iMax = iNewLoc;
								for(int i = 0; i < m_iColumnsTracked; i++) {
									if(m_aColumns[i].iLocation > iMin && m_aColumns[i].iLocation <= iMax)
										m_aColumns[i].iLocation--;
								}
							}

							m_aColumns[pHeader->iItem].iLocation = iNewLoc;

							Invalidate(FALSE);
							break;
						}
					}
				}

				return *pResult = 1;
			}
		}


	case WM_COMMAND:
		//deal with menu clicks

		if(wParam >= MLC_MENU) {

			CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
			int iCount = pHeaderCtrl->GetItemCount();

			int iToggle = wParam - MLC_MENU;
			if(iToggle >= iCount)
				break;

			if(m_aColumns[iToggle].bHidden)
				ShowColumn(iToggle);
			else
				HideColumn(iToggle);

			return *pResult = 1;
		}
		break;


	case LVM_DELETECOLUMN:
		//book keeping!

		if(m_aColumns != NULL) {
			for(int i = 0; i < m_iColumnsTracked; i++)
				if(m_aColumns[i].bHidden)
					ShowColumn(i);

			delete[] m_aColumns;
		}
		m_aColumns = new MULE_COLUMN[--m_iColumnsTracked];
		for(int i = 0; i < m_iColumnsTracked; i++) {
			m_aColumns[i].iLocation = i;
			m_aColumns[i].bHidden = false;
		}
		break;

	//case LVM_INSERTCOLUMN:
	case LVM_INSERTCOLUMNA:
	case LVM_INSERTCOLUMNW:
		//book keeping!

		if(m_aColumns != NULL) {
			for(int i = 0; i < m_iColumnsTracked; i++)
				if(m_aColumns[i].bHidden)
					ShowColumn(i);

			delete[] m_aColumns;
		}
		m_aColumns = new MULE_COLUMN[++m_iColumnsTracked];
		for(int i = 0; i < m_iColumnsTracked; i++) {
			m_aColumns[i].iLocation = i;
			m_aColumns[i].bHidden = false;
		}
		break;

	}

	return CListCtrl::OnWndMsg(message, wParam, lParam, pResult);
}
#endif

#if 0
bool CMuleListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if(message != WM_DRAWITEM) {
		//catch the prepaint and copy struct
		if(message == WM_NOTIFY && ((NMHDR*)lParam)->code == NM_CUSTOMDRAW &&
		  ((LPNMLVCUSTOMDRAW)lParam)->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {

			m_bCustomDraw = CListCtrl::OnChildNotify(message, wParam, lParam, pResult);
			if(m_bCustomDraw)
				memcpy(&m_lvcd, (void*)lParam, sizeof(NMLVCUSTOMDRAW));

			return m_bCustomDraw;
		}

		return CListCtrl::OnChildNotify(message, wParam, lParam, pResult);
	}

	ASSERT(pResult == NULL); // no return value expected
	UNUSED(pResult);         // unused in release builds

	DrawItem((LPDRAWITEMSTRUCT)lParam);
	return TRUE;
}
#endif

//////////////////////////////////
// CMuleListCtrl message map

#if 0
BEGIN_MESSAGE_MAP(CMuleListCtrl, CListCtrl)
	ON_WM_DRAWITEM()
	ON_WM_ERASEBKGND()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()
#endif

//////////////////////////////////
// CMuleListCtrl message handlers

#if 0
void CMuleListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	//set up our ficker free drawing
	CRect rcItem(lpDrawItemStruct->rcItem);
	CDC *oDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	oDC->SetBkColor(m_crWindow);
	CMemDC pDC(oDC, &rcItem);
	pDC->SelectObject(GetFont());
	if(m_bCustomDraw)
		pDC->SetTextColor(m_lvcd.clrText);
	else
		pDC->SetTextColor(m_crWindowText);

	int iOffset = pDC->GetTextExtent(_T(" "), 1 ).cx*2;
	int iItem = lpDrawItemStruct->itemID;
	CImageList* pImageList;
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();

	//gets the item image and state info
	LV_ITEM lvi;
	lvi.mask = LVIF_IMAGE | LVIF_STATE;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
	GetItem(&lvi);

	//see if the item be highlighted
	bool bHighlight = ((lvi.state & LVIS_DROPHILITED) || (lvi.state & LVIS_SELECTED));
	bool bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));

	//get rectangles for drawing
	CRect rcBounds, rcLabel, rcIcon;
	GetItemRect(iItem, rcBounds, LVIR_BOUNDS);
	GetItemRect(iItem, rcLabel, LVIR_LABEL);
	GetItemRect(iItem, rcIcon, LVIR_ICON);
	CRect rcCol(rcBounds);

	//the label!
	CString sLabel = GetItemText(iItem, 0);
	//labels are offset by a certain amount 
	//this offset is related to the width of a space character
	CRect rcHighlight;
	CRect rcWnd;

	//should I check (GetExtendedStyle() & LVS_EX_FULLROWSELECT) ?
	rcHighlight.top    = rcBounds.top;
	rcHighlight.bottom = rcBounds.bottom;
	rcHighlight.left   = rcBounds.left  + 1;
	rcHighlight.right  = rcBounds.right - 1;

	//draw the background color
	if(bHighlight) {
		if(bCtrlFocused) {
			pDC->FillRect(rcHighlight, &CBrush(m_crHighlight));
			pDC->SetBkColor(m_crHighlight);
		} else {
			pDC->FillRect(rcHighlight, &CBrush(m_crNoHighlight));
			pDC->SetBkColor(m_crNoHighlight);
		}
	} else {
		pDC->FillRect(rcHighlight, &CBrush(m_crWindow));
		pDC->SetBkColor(GetBkColor());
	}

	//update column
	rcCol.right = rcCol.left + GetColumnWidth(0);

	//draw state icon
	if(lvi.state & LVIS_STATEIMAGEMASK) {
		int nImage = ((lvi.state & LVIS_STATEIMAGEMASK)>>12) - 1;
		pImageList = GetImageList(LVSIL_STATE);
		if (pImageList) {
			COLORREF crOld = pImageList->SetBkColor(CLR_NONE);
			pImageList->Draw(pDC, nImage, rcCol.TopLeft(), ILD_NORMAL);
			pImageList->SetBkColor(crOld);
		}
	}

	//draw the item's icon
	pImageList = GetImageList(LVSIL_SMALL);
	if(pImageList) {
		COLORREF crOld = pImageList->SetBkColor(CLR_NONE);
		pImageList->Draw(pDC, lvi.iImage, rcIcon.TopLeft(), ILD_NORMAL);
		pImageList->SetBkColor(crOld);
	}

	//draw item label (column 0)
	rcLabel.left += iOffset / 2;
	rcLabel.right -= iOffset;
	pDC->DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | DT_LEFT | DT_NOCLIP);

	//draw labels for remaining columns
	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	rcBounds.right = rcHighlight.right > rcBounds.right ? rcHighlight.right : rcBounds.right;

	int iCount = pHeaderCtrl->GetItemCount();
	for(int iCurrent = 1; iCurrent < iCount; iCurrent++) {
		
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		//don't draw column 0 again
		if(iColumn == 0)
			continue;

		GetColumn(iColumn, &lvc);
		//don't draw anything with 0 width
		if(lvc.cx == 0)
			continue;

		rcCol.left = rcCol.right;
		rcCol.right += lvc.cx;

		sLabel = GetItemText(iItem, iColumn);
		if (sLabel.GetLength() == 0)
			continue;

		//get the text justification
		UINT nJustify = DT_LEFT;
		switch(lvc.fmt & LVCFMT_JUSTIFYMASK) {
		case LVCFMT_RIGHT:
			nJustify = DT_RIGHT;
			break;
		case LVCFMT_CENTER:
			nJustify = DT_CENTER;
			break;
		default:
			break;
		}

		rcLabel = rcCol;
		rcLabel.left += iOffset;
		rcLabel.right -= iOffset;

		pDC->DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | nJustify);
	}

	//draw focus rectangle if item has focus
	if((lvi.state & LVIS_FOCUSED) && (bCtrlFocused || (lvi.state & LVIS_SELECTED))) {
		if(!bCtrlFocused || !(lvi.state & LVIS_SELECTED))
			pDC->FrameRect(rcHighlight, &CBrush(m_crNoFocusLine));
		else
			pDC->FrameRect(rcHighlight, &CBrush(m_crFocusLine));
	}
}
#endif

#if 0
bool CMuleListCtrl::OnEraseBkgnd(CDC* pDC)
{
	int itemCount = GetItemCount();
	if (!itemCount)
		return CListCtrl::OnEraseBkgnd(pDC);

	RECT clientRect;
	RECT itemRect;
	int topIndex = GetTopIndex();
	int maxItems = GetCountPerPage();
	int drawnItems = itemCount < maxItems ? itemCount : maxItems;

	//draw top portion
	GetClientRect(&clientRect);
	GetItemRect(topIndex, &itemRect, LVIR_BOUNDS);
	clientRect.bottom = itemRect.top;
	pDC->FillSolidRect(&clientRect,GetBkColor());

	//draw bottom portion if we have to
	if(topIndex + maxItems >= itemCount) {
		GetClientRect(&clientRect);
		GetItemRect(topIndex + drawnItems - 1, &itemRect, LVIR_BOUNDS);
		clientRect.top = itemRect.bottom;
		pDC->FillSolidRect(&clientRect, GetBkColor());
	}

	//draw right half if we need to
	if (itemRect.right < clientRect.right) {
		GetClientRect(&clientRect);
		clientRect.left = itemRect.right;
		pDC->FillSolidRect(&clientRect, GetBkColor());
	}

	return TRUE;
}
#endif

#if 0
void CMuleListCtrl::OnSysColorChange()
{
	//adjust colors
	CListCtrl::OnSysColorChange();
	SetColors();

	//redraw the up/down sort arrow (if it's there)
	if(m_iCurrentSortItem >= 0) {
		CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
		HDITEM headerItem;
		headerItem.mask = HDI_FORMAT | HDI_BITMAP;
		if(pHeaderCtrl->GetItem(m_iCurrentSortItem, &headerItem) && headerItem.hbm != 0) {
			DeleteObject(headerItem.hbm);
			headerItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
			headerItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
				MAKEINTRESOURCE(m_atSortArrow), IMAGE_BITMAP, 0, 0,
				LR_LOADMAP3DCOLORS);
			pHeaderCtrl->SetItem(m_iCurrentSortItem, &headerItem);
		}
	}
}
#endif

bool CMuleListCtrl::ProcessEvent(wxEvent& evt)
{
	if ((evt.GetEventType()==wxEVT_COMMAND_MENU_SELECTED) && (evt.GetId() >= MP_LISTCOL_1) && (evt.GetId() <= MP_LISTCOL_15)) {
		int col = evt.GetId() - MP_LISTCOL_1;

		#ifdef __WXMSW__
		if (wxListCtrl::GetColumnWidth(col) > m_col_minsize) {
		#else
		if (wxODListCtrl::GetColumnWidth(col) > m_col_minsize) {
		#endif
			SetColumnWidth(col, 0);
		} else {
			SetColumnWidth(col, wxLIST_AUTOSIZE);
		}
	} else if (evt.GetEventType()==wxEVT_COMMAND_LIST_COL_END_DRAG) {
		// Lazy code to save space, saving all column widths when only one was resized.
		// Should we be changing the widths of existing covered search lists?  
		// Let's say no and the user won't get surprised.
		SaveSettings();	
	}
	#ifdef __WXMSW__
	return wxListCtrl::ProcessEvent(evt);
	#else
	return wxODListCtrl::ProcessEvent(evt);
	#endif
}

void CMuleListCtrl::OnColumnRclick(wxListEvent& evt)
{
	m_ColumnMenu = new wxMenu();
	wxListItem item;
	for (int a=0; a<GetColumnCount(); a++) {
		GetColumn(a, item);
		m_ColumnMenu->AppendCheckItem(a+MP_LISTCOL_1, item.GetText());
		#ifdef __WXMSW__
		m_ColumnMenu->Check(a+MP_LISTCOL_1, wxListCtrl::GetColumnWidth(a)>m_col_minsize ? true : false);
		#else
		m_ColumnMenu->Check(a+MP_LISTCOL_1, wxODListCtrl::GetColumnWidth(a)>m_col_minsize ? true : false);
		#endif
	}

	PopupMenu(m_ColumnMenu, evt.GetPoint());
}

#ifdef __WXMSW__
BEGIN_EVENT_TABLE(CMuleListCtrl, wxListCtrl)
#else
BEGIN_EVENT_TABLE(CMuleListCtrl, wxODListCtrl)
#endif
	EVT_LIST_COL_RIGHT_CLICK(-1, CMuleListCtrl::OnColumnRclick)
END_EVENT_TABLE()
