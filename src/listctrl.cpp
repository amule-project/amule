// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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
//
/////////////////////////////////////////////////////////////////////////////
// Name:        generic/listctrl.cpp
// Purpose:     generic implementation of wxODListCtrl
// Author:      Robert Roebling
//              Vadim Zeitlin (virtual list control support)
// Id:          $Id$
// Copyright:   (c) 1998 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
   TODO

   1. we need to implement searching/sorting for virtual controls somehow
  ?2. when changing selection the lines are refreshed twice
 */

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_LISTCTRL

#ifndef __WXMSW__

#ifndef WX_PRECOMP
    #include "wx/app.h"

    #include "wx/dynarray.h"

    #include "wx/dcscreen.h"

    #include "wx/textctrl.h"
#endif

// under Win32 we always use the native version and also may use the generic
// one, however some things should be done only if we use only the generic
// version
#if defined(__WIN32__) && !defined(__WXUNIVERSAL__)
    #define HAVE_NATIVE_LISTCTRL
#endif

// if we have the native control, wx/listctrl.h declares it and not this one
#ifdef HAVE_NATIVE_LISTCTRL
#include "wx/generic/listctrl.h"
//#include "listctrl_new.h"
#else // !HAVE_NATIVE_LISTCTRL
//#include "wx/listctrl.h"
#include "listctrl.h"

    // if we have a native version, its implementation file does all this
    //IMPLEMENT_DYNAMIC_CLASS(wxListItem, wxObject)
    //IMPLEMENT_DYNAMIC_CLASS(wxListEvent, wxNotifyEvent)
    IMPLEMENT_DYNAMIC_CLASS(wxODListView, wxODListCtrl)
    IMPLEMENT_DYNAMIC_CLASS(wxODListCtrl, wxODGenericListCtrl)
#endif // HAVE_NATIVE_LISTCTRL/!HAVE_NATIVE_LISTCTRL

#if defined(__WXGTK__)
    #include <gtk/gtk.h>
    #include "wx/gtk/win_gtk.h"
#endif

#define G_BLEND(a,b) ( ((int)a*b)/100>255?255:((int)a*b)/100 ) 

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

#if 0
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_RDRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_END_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_GET_INFO)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_SET_INFO)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_DESELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_KEY_DOWN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_INSERT_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_DRAGGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_END_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_MIDDLE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_ACTIVATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_FOCUSED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_CACHE_HINT)
#endif
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_MIDDLE_CLICK)

#include <wx/dcmemory.h>

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the height of the header window (FIXME: should depend on its font!)
static const int HEADER_HEIGHT = 23;

// the scrollbar units
static const int SCROLL_UNIT_X = 15;
static const int SCROLL_UNIT_Y = 15;

// the spacing between the lines (in report mode)
static const int LINE_SPACING = 0;

// extra margins around the text label
static const int EXTRA_WIDTH = 3;
static const int EXTRA_HEIGHT = 4;

// offset for the header window
static const int HEADER_OFFSET_X = 1;
static const int HEADER_OFFSET_Y = 1;

// when autosizing the columns, add some slack
static const int AUTOSIZE_COL_MARGIN = 10;

// default and minimal widths for the header columns
static const int WIDTH_COL_DEFAULT = 80;
static const int WIDTH_COL_MIN = 0;

// the space between the image and the text in the report mode
static const int IMAGE_MARGIN_IN_REPORT_MODE = 0;

// ============================================================================
// private classes
// ============================================================================

// ----------------------------------------------------------------------------
// wxODSelectionStore
// ----------------------------------------------------------------------------

static int CMPFUNC_CONV SizeTCmpFn(size_t n1, size_t n2) { return n1 - n2; }

WX_DEFINE_SORTED_EXPORTED_ARRAY_LONG(size_t, wxODIndexArray);

// this class is used to store the selected items in the virtual list control
// (but it is not tied to list control and so can be used with other controls
// such as wxListBox in wxUniv)
//
// the idea is to make it really smart later (i.e. store the selections as an
// array of ranes + individual items) but, as I don't have time to do it now
// (this would require writing code to merge/break ranges and much more) keep
// it simple but define a clean interface to it which allows it to be made
// smarter later
class WXDLLEXPORT wxODSelectionStore
{
public:
    wxODSelectionStore() : m_itemsSel(SizeTCmpFn) { Init(); }

    // set the total number of items we handle
    void SetItemCount(size_t count) { m_count = count; }

    // special case of SetItemCount(0)
    void Clear() { m_itemsSel.Clear(); m_count = 0; m_defaultState = FALSE; }

    // must be called when a new item is inserted/added
    void OnItemAdd(size_t item) { wxFAIL_MSG( _T("TODO") ); }

    // must be called when an item is deleted
    void OnItemDelete(size_t item);

    // select one item, use SelectRange() insted if possible!
    //
    // returns true if the items selection really changed
    bool SelectItem(size_t item, bool select = TRUE);

    // select the range of items
    //
    // return true and fill the itemsChanged array with the indices of items
    // which have changed state if "few" of them did, otherwise return false
    // (meaning that too many items changed state to bother counting them
    // individually)
    bool SelectRange(size_t itemFrom, size_t itemTo,
                     bool select = TRUE,
                     wxArrayInt *itemsChanged = NULL);

    // return true if the given item is selected
    bool IsSelected(size_t item) const;

    // return the total number of selected items
    size_t GetSelectedCount() const
    {
        return m_defaultState ? m_count - m_itemsSel.GetCount()
                              : m_itemsSel.GetCount();
    }

private:
    // (re)init
    void Init() { m_defaultState = FALSE; }

    // the total number of items we handle
    size_t m_count;

    // the default state: normally, FALSE (i.e. off) but maybe set to TRUE if
    // there are more selected items than non selected ones - this allows to
    // handle selection of all items efficiently
    bool m_defaultState;

    // the array of items whose selection state is different from default
    wxODIndexArray m_itemsSel;

    DECLARE_NO_COPY_CLASS(wxODSelectionStore)
};

//-----------------------------------------------------------------------------
//  wxODListItemData (internal)
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxODListItemData
{
public:
    wxODListItemData(wxODListMainWindow *owner);
    ~wxODListItemData();

    void SetItem( const wxListItem &info );
    void SetImage( int image ) { m_image = image; }
    void SetData( long data ) { m_data = data; }
    void SetPosition( int x, int y );
    void SetSize( int width, int height );

    bool HasText() const { return !m_text.empty(); }
    const wxString& GetText() const { return m_text; }
    void SetText(const wxString& text) { m_text = text; }

    // we can't use empty string for measuring the string width/height, so
    // always return something
    wxString GetTextForMeasuring() const
    {
        wxString s = GetText();
        if ( s.empty() )
            s = _T('H');

        return s;
    }

    bool IsHit( int x, int y ) const;

    int GetX() const;
    int GetY() const;
    int GetWidth() const;
    int GetHeight() const;

    int GetImage() const { return m_image; }
    bool HasImage() const { return GetImage() != -1; }

    void GetItem( wxListItem &info ) const;

    void SetAttr(wxListItemAttr *attr) { m_attr = attr; }
    wxListItemAttr *GetAttr() const { return m_attr; }

public:
    // the item image or -1
    int m_image;

    // user data associated with the item
    long m_data;

    // the item coordinates are not used in report mode, instead this pointer
    // is NULL and the owner window is used to retrieve the item position and
    // size
    wxRect *m_rect;

    // the list ctrl we are in
    wxODListMainWindow *m_owner;

    // custom attributes or NULL
    wxListItemAttr *m_attr;

protected:
    // common part of all ctors
    void Init();

    wxString m_text;
};

//-----------------------------------------------------------------------------
//  wxODListHeaderData (internal)
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxODListHeaderData : public wxObject
{
public:
    wxODListHeaderData();
    wxODListHeaderData( const wxListItem &info );
    void SetItem( const wxListItem &item );
    void SetPosition( int x, int y );
    void SetWidth( int w );
    void SetFormat( int format );
    void SetHeight( int h );
    bool HasImage() const;
  void SetArrow(int mode);

    bool HasText() const { return !m_text.empty(); }
    const wxString& GetText() const { return m_text; }
    void SetText(const wxString& text) { m_text = text; }

    void GetItem( wxListItem &item );

    bool IsHit( int x, int y ) const;
    int GetImage() const;
    int GetWidth() const;
    int GetFormat() const;
  int GetArrow() const {return m_arrowMode;}

protected:
    long      m_mask;
    int       m_image;
    wxString  m_text;
    int       m_format;
    int       m_width;
    int       m_xpos,
              m_ypos;
    int       m_height;
  int m_arrowMode;

private:
    void Init();
};

//-----------------------------------------------------------------------------
//  wxODListLineData (internal)
//-----------------------------------------------------------------------------

WX_DECLARE_LIST(wxODListItemData, wxODListItemDataList);
#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxODListItemDataList);

class WXDLLEXPORT wxODListLineData
{
public:
    // the list of subitems: only may have more than one item in report mode
    wxODListItemDataList m_items;

    // this is not used in report view
    struct GeometryInfo
    {
        // total item rect
        wxRect m_rectAll;

        // label only
        wxRect m_rectLabel;

        // icon only
        wxRect m_rectIcon;

        // the part to be highlighted
        wxRect m_rectHighlight;
    } *m_gi;

    // is this item selected? [NB: not used in virtual mode]
    bool m_highlighted;

    // back pointer to the list ctrl
    wxODListMainWindow *m_owner;

public:
    wxODListLineData(wxODListMainWindow *owner);

    ~wxODListLineData() { delete m_gi; }

    // are we in report mode?
    inline bool InReportView() const;

    // are we in virtual report mode?
    inline bool IsVirtual() const;

    // these 2 methods shouldn't be called for report view controls, in that
    // case we determine our position/size ourselves

    // calculate the size of the line
    void CalculateSize( wxDC *dc, int spacing );

    // remember the position this line appears at
    void SetPosition( int x, int y,  int window_width, int spacing );

    // wxODListCtrl API

    void SetImage( int image ) { SetImage(0, image); }
    int GetImage() const { return GetImage(0); }
    bool HasImage() const { return GetImage() != -1; }
    bool HasText() const { return !GetText(0).empty(); }

    void SetItem( int index, const wxListItem &info );
    void GetItem( int index, wxListItem &info );

    wxString GetText(int index) const;
    void SetText( int index, const wxString s );

    wxListItemAttr *GetAttr() const;
    void SetAttr(wxListItemAttr *attr);

    // return true if the highlighting really changed
    bool Highlight( bool on );

    void ReverseHighlight();

    bool IsHighlighted() const
    {
        wxASSERT_MSG( !IsVirtual(), _T("unexpected call to IsHighlighted") );

        return m_highlighted;
    }

    // draw the line on the given DC in icon/list mode
    void Draw( wxDC *dc );

    // the same in report mode
    void DrawInReportMode( wxDC *dc,
                           const wxRect& rect,
                           const wxRect& rectHL,
                           bool highlighted );

private:
    // set the line to contain num items (only can be > 1 in report mode)
    void InitItems( int num );

    // get the mode (i.e. style)  of the list control
    inline int GetMode() const;

    // prepare the DC for drawing with these item's attributes, return true if
    // we need to draw the items background to highlight it, false otherwise
    bool SetAttributes(wxDC *dc,
                       const wxListItemAttr *attr,
                       bool highlight);

    // draw the text on the DC with the correct justification; also add an
    // ellipsis if the text is too large to fit in the current width
    void DrawTextFormatted(wxDC *dc, const wxString &text, int col, int x, int y, int width);

    // these are only used by GetImage/SetImage above, we don't support images
    // with subitems at the public API level yet
    void SetImage( int index, int image );
    int GetImage( int index ) const;
};

WX_DECLARE_EXPORTED_OBJARRAY(wxODListLineData, wxODListLineDataArray);
#include "wx/arrimpl.cpp"
WX_DEFINE_OBJARRAY(wxODListLineDataArray);

//-----------------------------------------------------------------------------
//  wxODListHeaderWindow (internal)
//-----------------------------------------------------------------------------

#if 1
class WXDLLEXPORT wxODListHeaderWindow : public wxWindow
{
protected:
    wxODListMainWindow  *m_owner;
    wxCursor          *m_currentCursor;
    wxCursor          *m_resizeCursor;
    bool               m_isDragging;

    // column being resized or -1
    int m_column;

    // divider line position in logical (unscrolled) coords
    int m_currentX;

    // minimal position beyond which the divider line can't be dragged in
    // logical coords
    int m_minX;

public:
    wxODListHeaderWindow();

    wxODListHeaderWindow( wxWindow *win,
                        wxWindowID id,
                        wxODListMainWindow *owner,
                        const wxPoint &pos = wxDefaultPosition,
                        const wxSize &size = wxDefaultSize,
                        long style = 0,
                        const wxString &name = wxT("wxlistctrlcolumntitles") );

    virtual ~wxODListHeaderWindow();

    void DoDrawRect( wxDC *dc, int x, int y, int w, int h );
    void DrawCurrent();
    void AdjustDC(wxDC& dc);

    void OnPaint( wxPaintEvent &event );
    void OnMouse( wxMouseEvent &event );
    void OnSetFocus( wxFocusEvent &event );

    // needs refresh
    bool m_dirty;

private:
    // common part of all ctors
    void Init();

    // generate and process the list event of the given type, return true if
    // it wasn't vetoed, i.e. if we should proceed
    bool SendListEvent(wxEventType type, wxPoint pos);

    DECLARE_DYNAMIC_CLASS(wxODListHeaderWindow)
    DECLARE_EVENT_TABLE()
};
#endif

//-----------------------------------------------------------------------------
// wxODListRenameTimer (internal)
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxODListRenameTimer: public wxTimer
{
private:
    wxODListMainWindow *m_owner;

public:
    wxODListRenameTimer( wxODListMainWindow *owner );
    void Notify();
};

//-----------------------------------------------------------------------------
//  wxODListTextCtrl (internal)
//-----------------------------------------------------------------------------

class WXDLLEXPORT wxODListTextCtrl: public wxTextCtrl
{
public:
    wxODListTextCtrl(wxODListMainWindow *owner, size_t itemEdit);

protected:
    void OnChar( wxKeyEvent &event );
    void OnKeyUp( wxKeyEvent &event );
    void OnKillFocus( wxFocusEvent &event );

    bool AcceptChanges();
    void Finish();

private:
    wxODListMainWindow   *m_owner;
    wxString            m_startValue;
    size_t              m_itemEdited;
    bool                m_finished;

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
//  wxODListMainWindow (internal)
//-----------------------------------------------------------------------------

WX_DECLARE_LIST(wxODListHeaderData, wxODListHeaderDataList);
#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxODListHeaderDataList);

class WXDLLEXPORT wxODListMainWindow : public wxScrolledWindow
{
public:
    wxODListMainWindow();
    wxODListMainWindow( wxWindow *parent,
                      wxWindowID id,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0,
                      const wxString &name = _T("listctrlmainwindow") );

    virtual ~wxODListMainWindow();

    bool HasFlag(int flag) const { return m_parent->HasFlag(flag); }

    // return true if this is a virtual list control
    bool IsVirtual() const { return HasFlag(wxLC_VIRTUAL); }

    // return true if the control is in report mode
    bool InReportView() const { return HasFlag(wxLC_REPORT); }

    // return true if we are in single selection mode, false if multi sel
    bool IsSingleSel() const { return HasFlag(wxLC_SINGLE_SEL); }

    // do we have a header window?
    bool HasHeader() const
        { return HasFlag(wxLC_REPORT) && !HasFlag(wxLC_NO_HEADER); }

    void HighlightAll( bool on );

    // all these functions only do something if the line is currently visible

    // change the line "selected" state, return TRUE if it really changed
    bool HighlightLine( size_t line, bool highlight = TRUE);

    // as HighlightLine() but do it for the range of lines: this is incredibly
    // more efficient for virtual list controls!
    //
    // NB: unlike HighlightLine() this one does refresh the lines on screen
    void HighlightLines( size_t lineFrom, size_t lineTo, bool on = TRUE );

    // toggle the line state and refresh it
    void ReverseHighlight( size_t line )
        { HighlightLine(line, !IsHighlighted(line)); RefreshLine(line); }

    // return true if the line is highlighted
    bool IsHighlighted(size_t line) const;

    // refresh one or several lines at once
    void RefreshLine( size_t line );
    void RefreshLines( size_t lineFrom, size_t lineTo );

    // refresh all selected items
    void RefreshSelected();

    // refresh all lines below the given one: the difference with
    // RefreshLines() is that the index here might not be a valid one (happens
    // when the last line is deleted)
    void RefreshAfter( size_t lineFrom );

    // the methods which are forwarded to wxODListLineData itself in list/icon
    // modes but are here because the lines don't store their positions in the
    // report mode

    // get the bound rect for the entire line
    wxRect GetLineRect(size_t line) const;

    // get the bound rect of the label
    wxRect GetLineLabelRect(size_t line) const;

    // get the bound rect of the items icon (only may be called if we do have
    // an icon!)
    wxRect GetLineIconRect(size_t line) const;

    // get the rect to be highlighted when the item has focus
    wxRect GetLineHighlightRect(size_t line) const;

    // get the size of the total line rect
    wxSize GetLineSize(size_t line) const
        { return GetLineRect(line).GetSize(); }

    // return the hit code for the corresponding position (in this line)
    long HitTestLine(size_t line, int x, int y) const;

    // bring the selected item into view, scrolling to it if necessary
    void MoveToItem(size_t item);

    // bring the current item into view
    void MoveToFocus() { MoveToItem(m_current); }

    // start editing the label of the given item
    void EditLabel( long item );

    // suspend/resume redrawing the control
    void Freeze();
    void Thaw();

    void SetFocus();

    void OnRenameTimer();
    bool OnRenameAccept(size_t itemEdit, const wxString& value);
    void OnRenameCancelled(size_t itemEdit);

    void OnMouse( wxMouseEvent &event );

    // called to switch the selection from the current item to newCurrent,
    void OnArrowChar( size_t newCurrent, const wxKeyEvent& event );

    void OnChar( wxKeyEvent &event );
    void OnKeyDown( wxKeyEvent &event );
    void OnSetFocus( wxFocusEvent &event );
    void OnKillFocus( wxFocusEvent &event );
    void OnScroll(wxScrollWinEvent& event) ;

    void OnPaint( wxPaintEvent &event );
	void OnErase( wxEraseEvent& event )
	{
		if ( !HasFlag(wxLC_OWNERDRAW) || IsEmpty() )
			event.Skip();
	}

    void DrawImage( int index, wxDC *dc, int x, int y );
    void GetImageSize( int index, int &width, int &height ) const;
    int GetTextLength( const wxString &s ) const;

    void SetImageList( wxImageListType *imageList, int which );
    void SetItemSpacing( int spacing, bool isSmall = FALSE );
    int GetItemSpacing( bool isSmall = FALSE );

    void SetColumn( int col, wxListItem &item );
    void SetColumnWidth( int col, int width );
    void GetColumn( int col, wxListItem &item ) const;
    int GetColumnWidth( int col ) const;
    int GetColumnCount() const { return m_columns.GetCount(); }

  void SetSortArrow(int col, int mode);
  int GetSortArrow(int col);

    // returns the sum of the heights of all columns
    int GetHeaderWidth() const;

    int GetCountPerPage() const;

    void SetItem( wxListItem &item );
    void GetItem( wxListItem &item ) const;
    void SetItemState( long item, long state, long stateMask );
    int GetItemState( long item, long stateMask ) const;
    void GetItemRect( long index, wxRect &rect ) const;
    bool GetItemPosition( long item, wxPoint& pos ) const;
    int GetSelectedItemCount() const;

    wxString GetItemText(long item) const
    {
        wxListItem info;
        info.m_itemId = item;
        GetItem( info );
        return info.m_text;
    }

    void SetItemText(long item, const wxString& value)
    {
        wxListItem info;
        info.m_mask = wxLIST_MASK_TEXT;
        info.m_itemId = item;
        info.m_text = value;
        SetItem( info );
    }

    // set the scrollbars and update the positions of the items
    void RecalculatePositions(bool noRefresh = FALSE);

    // refresh the window and the header
    void RefreshAll();

    long GetNextItem( long item, int geometry, int state ) const;
    void DeleteItem( long index );
    void DeleteAllItems();
    void DeleteColumn( int col );
    void DeleteEverything();
    void EnsureVisible( long index );
    long FindItem( long start, const wxString& str, bool partial = FALSE );
    long FindItem( long start, long data);
    long HitTest( int x, int y, int &flags );
    void InsertItem( wxListItem &item );
    void InsertColumn( long col, wxListItem &item );
    void SortItems( wxListCtrlCompare fn, long data );

    size_t GetItemCount() const;
    bool IsEmpty() const { return GetItemCount() == 0; }
    void SetItemCount(long count);

    // change the current (== focused) item, send a notification event
    void ChangeCurrent(size_t current);
    void ResetCurrent() { ChangeCurrent((size_t)-1); }
    bool HasCurrent() const { return m_current != (size_t)-1; }

    // send out a wxListEvent
    void SendNotify( size_t line,
                     wxEventType command,
                     wxPoint point = wxDefaultPosition );

    // override base class virtual to reset m_lineHeight when the font changes
    virtual bool SetFont(const wxFont& font)
    {
        if ( !wxScrolledWindow::SetFont(font) )
            return FALSE;

        m_lineHeight = 0;

        return TRUE;
    }

    // these are for wxODListLineData usage only

    // get the backpointer to the list ctrl
    wxODGenericListCtrl *GetListCtrl() const
    {
        return wxStaticCast(GetParent(), wxODGenericListCtrl);
    }

    // get the height of all lines (assuming they all do have the same height)
    wxCoord GetLineHeight() const;

    // get the y position of the given line (only for report view)
    wxCoord GetLineY(size_t line) const;

    // get the brush to use for the item highlighting
    wxBrush *GetHighlightBrush() const
    {
        return m_hasFocus ? m_highlightBrush : m_highlightUnfocusedBrush;
    }

//protected:
    // the array of all line objects for a non virtual list control (for the
    // virtual list control we only ever use m_lines[0])
    wxODListLineDataArray  m_lines;

    // the list of column objects
    wxODListHeaderDataList m_columns;

  // currently focused item or -1
    size_t               m_current;

    // the number of lines per page
    int                  m_linesPerPage;

    // this flag is set when something which should result in the window
    // redrawing happens (i.e. an item was added or deleted, or its appearance
    // changed) and OnPaint() doesn't redraw the window while it is set which
    // allows to minimize the number of repaintings when a lot of items are
    // being added. The real repainting occurs only after the next OnIdle()
    // call
    bool                 m_dirty;

    wxColour            *m_highlightColour;
    int                  m_xScroll,
                         m_yScroll;
    wxImageListType         *m_small_image_list;
    wxImageListType         *m_normal_image_list;
    int                  m_small_spacing;
    int                  m_normal_spacing;
    bool                 m_hasFocus;

    bool                 m_lastOnSame;
    wxTimer             *m_renameTimer;
    bool                 m_isCreated;
    int                  m_dragCount;
    wxPoint              m_dragStart;

    // for double click logic
    size_t m_lineLastClicked,
           m_lineBeforeLastClicked;

protected:
    // the total count of items in a virtual list control
    size_t m_countVirt;

    // the object maintaining the items selection state, only used in virtual
    // controls
    wxODSelectionStore m_selStore;

    // common part of all ctors
    void Init();

    // intiialize m_[xy]Scroll
    void InitScrolling();

    // get the line data for the given index
    wxODListLineData *GetLine(size_t n) const
    {
        wxASSERT_MSG( n != (size_t)-1, _T("invalid line index") );

        if ( IsVirtual() )
        {
            wxConstCast(this, wxODListMainWindow)->CacheLineData(n);

            n = 0;
        }

        return &m_lines[n];
    }

    // get a dummy line which can be used for geometry calculations and such:
    // you must use GetLine() if you want to really draw the line
    wxODListLineData *GetDummyLine() const;

    // cache the line data of the n-th line in m_lines[0]
    void CacheLineData(size_t line);

    // get the range of visible lines
    void GetVisibleLinesRange(size_t *from, size_t *to);

    // force us to recalculate the range of visible lines
    void ResetVisibleLinesRange() { m_lineFrom = (size_t)-1; }

    // get the colour to be used for drawing the rules
    wxColour GetRuleColour() const
    {
#ifdef __WXMAC__
        return *wxWHITE;
#else
        return wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT);
#endif
    }

private:
    // initialize the current item if needed
    void UpdateCurrent();

    // delete all items but don't refresh: called from dtor
    void DoDeleteAllItems();

    // the height of one line using the current font
    wxCoord m_lineHeight;

    // the total header width or 0 if not calculated yet
    wxCoord m_headerWidth;

    // the first and last lines being shown on screen right now (inclusive),
    // both may be -1 if they must be calculated so never access them directly:
    // use GetVisibleLinesRange() above instead
    size_t m_lineFrom,
           m_lineTo;

    // the brushes to use for item highlighting when we do/don't have focus
    wxBrush *m_highlightBrush,
            *m_highlightUnfocusedBrush;

    // if this is > 0, the control is frozen and doesn't redraw itself
    size_t m_freezeCount;

    DECLARE_DYNAMIC_CLASS(wxODListMainWindow)
    DECLARE_EVENT_TABLE()

    friend class wxODGenericListCtrl;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxODSelectionStore
// ----------------------------------------------------------------------------

bool wxODSelectionStore::IsSelected(size_t item) const
{
    bool isSel = m_itemsSel.Index(item) != wxNOT_FOUND;

    // if the default state is to be selected, being in m_itemsSel means that
    // the item is not selected, so we have to inverse the logic
    return m_defaultState ? !isSel : isSel;
}

bool wxODSelectionStore::SelectItem(size_t item, bool select)
{
    // search for the item ourselves as like this we get the index where to
    // insert it later if needed, so we do only one search in the array instead
    // of two (adding item to a sorted array requires a search)
    size_t index = m_itemsSel.IndexForInsert(item);
    bool isSel = index < m_itemsSel.GetCount() && m_itemsSel[index] == item;

    if ( select != m_defaultState )
    {
        if ( !isSel )
        {
            m_itemsSel.AddAt(item, index);

            return TRUE;
        }
    }
    else // reset to default state
    {
        if ( isSel )
        {
            m_itemsSel.RemoveAt(index);
            return TRUE;
        }
    }

    return FALSE;
}

bool wxODSelectionStore::SelectRange(size_t itemFrom, size_t itemTo,
                                   bool select,
                                   wxArrayInt *itemsChanged)
{
    // 100 is hardcoded but it shouldn't matter much: the important thing is
    // that we don't refresh everything when really few (e.g. 1 or 2) items
    // change state
    static const size_t MANY_ITEMS = 100;

    wxASSERT_MSG( itemFrom <= itemTo, _T("should be in order") );

    // are we going to have more [un]selected items than the other ones?
    if ( itemTo - itemFrom > m_count/2 )
    {
        if ( select != m_defaultState )
        {
            // the default state now becomes the same as 'select'
            m_defaultState = select;

            // so all the old selections (which had state select) shouldn't be
            // selected any more, but all the other ones should
            wxODIndexArray selOld = m_itemsSel;
            m_itemsSel.Empty();

            // TODO: it should be possible to optimize the searches a bit
            //       knowing the possible range

            size_t item;
            for ( item = 0; item < itemFrom; item++ )
            {
                if ( selOld.Index(item) == wxNOT_FOUND )
                    m_itemsSel.Add(item);
            }

            for ( item = itemTo + 1; item < m_count; item++ )
            {
                if ( selOld.Index(item) == wxNOT_FOUND )
                    m_itemsSel.Add(item);
            }

            // many items (> half) changed state
            itemsChanged = NULL;
        }
        else // select == m_defaultState
        {
            // get the inclusive range of items between itemFrom and itemTo
            size_t count = m_itemsSel.GetCount(),
                   start = m_itemsSel.IndexForInsert(itemFrom),
                   end = m_itemsSel.IndexForInsert(itemTo);

            if ( start == count || m_itemsSel[start] < itemFrom )
            {
                start++;
            }

            if ( end == count || m_itemsSel[end] > itemTo )
            {
                end--;
            }

            if ( start <= end )
            {
                // delete all of them (from end to avoid changing indices)
                for ( int i = end; i >= (int)start; i-- )
                {
                    if ( itemsChanged )
                    {
                        if ( itemsChanged->GetCount() > MANY_ITEMS )
                        {
                            // stop counting (see comment below)
                            itemsChanged = NULL;
                        }
                        else
                        {
                            itemsChanged->Add(m_itemsSel[i]);
                        }
                    }

                    m_itemsSel.RemoveAt(i);
                }
            }
        }
    }
    else // "few" items change state
    {
        if ( itemsChanged )
        {
            itemsChanged->Empty();
        }

        // just add the items to the selection
        for ( size_t item = itemFrom; item <= itemTo; item++ )
        {
            if ( SelectItem(item, select) && itemsChanged )
            {
                itemsChanged->Add(item);

                if ( itemsChanged->GetCount() > MANY_ITEMS )
                {
                    // stop counting them, we'll just eat gobs of memory
                    // for nothing at all - faster to refresh everything in
                    // this case
                    itemsChanged = NULL;
                }
            }
        }
    }

    // we set it to NULL if there are many items changing state
    return itemsChanged != NULL;
}

void wxODSelectionStore::OnItemDelete(size_t item)
{
    size_t count = m_itemsSel.GetCount(),
           i = m_itemsSel.IndexForInsert(item);

    if ( i < count && m_itemsSel[i] == item )
    {
        // this item itself was in m_itemsSel, remove it from there
        m_itemsSel.RemoveAt(i);

        count--;
    }

    // and adjust the index of all which follow it
    while ( i < count )
    {
        // all following elements must be greater than the one we deleted
        wxASSERT_MSG( m_itemsSel[i] > item, _T("logic error") );

        m_itemsSel[i++]--;
    }
}

//-----------------------------------------------------------------------------
//  wxODListItemData
//-----------------------------------------------------------------------------

wxODListItemData::~wxODListItemData()
{
    // in the virtual list control the attributes are managed by the main
    // program, so don't delete them
    if ( !m_owner->IsVirtual() )
    {
        delete m_attr;
    }

    delete m_rect;
}

void wxODListItemData::Init()
{
    m_image = -1;
    m_data = 0;

    m_attr = NULL;
}

wxODListItemData::wxODListItemData(wxODListMainWindow *owner)
{
    Init();

    m_owner = owner;

    if ( owner->InReportView() )
    {
        m_rect = NULL;
    }
    else
    {
        m_rect = new wxRect;
    }
}

void wxODListItemData::SetItem( const wxListItem &info )
{
    if ( info.m_mask & wxLIST_MASK_TEXT )
        SetText(info.m_text);
    if ( info.m_mask & wxLIST_MASK_IMAGE )
        m_image = info.m_image;
    if ( info.m_mask & wxLIST_MASK_DATA )
        m_data = info.m_data;

    if ( info.HasAttributes() )
    {
        if ( m_attr )
            *m_attr = *info.GetAttributes();
        else
            m_attr = new wxListItemAttr(*info.GetAttributes());
    }

    if ( m_rect )
    {
        m_rect->x =
        m_rect->y =
        m_rect->height = 0;
        m_rect->width = info.m_width;
    }
}

void wxODListItemData::SetPosition( int x, int y )
{
    wxCHECK_RET( m_rect, _T("unexpected SetPosition() call") );

    m_rect->x = x;
    m_rect->y = y;
}

void wxODListItemData::SetSize( int width, int height )
{
    wxCHECK_RET( m_rect, _T("unexpected SetSize() call") );

    if ( width != -1 )
        m_rect->width = width;
    if ( height != -1 )
        m_rect->height = height;
}

bool wxODListItemData::IsHit( int x, int y ) const
{
    wxCHECK_MSG( m_rect, FALSE, _T("can't be called in this mode") );

    return wxRect(GetX(), GetY(), GetWidth(), GetHeight()).Inside(x, y);
}

int wxODListItemData::GetX() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->x;
}

int wxODListItemData::GetY() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->y;
}

int wxODListItemData::GetWidth() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->width;
}

int wxODListItemData::GetHeight() const
{
    wxCHECK_MSG( m_rect, 0, _T("can't be called in this mode") );

    return m_rect->height;
}

void wxODListItemData::GetItem( wxListItem &info ) const
{
    info.m_text = m_text;
    info.m_image = m_image;
    info.m_data = m_data;

    if ( m_attr )
    {
        if ( m_attr->HasTextColour() )
            info.SetTextColour(m_attr->GetTextColour());
        if ( m_attr->HasBackgroundColour() )
            info.SetBackgroundColour(m_attr->GetBackgroundColour());
        if ( m_attr->HasFont() )
            info.SetFont(m_attr->GetFont());
    }
}

//-----------------------------------------------------------------------------
//  wxODListHeaderData
//-----------------------------------------------------------------------------

void wxODListHeaderData::Init()
{
    m_mask = 0;
    m_image = -1;
    m_format = 0;
    m_width = 0;
    m_xpos = 0;
    m_ypos = 0;
    m_height = 0;
    m_arrowMode=0;
}

wxODListHeaderData::wxODListHeaderData()
{
    Init();
}

wxODListHeaderData::wxODListHeaderData( const wxListItem &item )
{
    Init();

    SetItem( item );
}

void wxODListHeaderData::SetArrow(int mode)
{
  m_arrowMode=(mode);
}

void wxODListHeaderData::SetItem( const wxListItem &item )
{
    m_mask = item.m_mask;

    if ( m_mask & wxLIST_MASK_TEXT )
        m_text = item.m_text;

    if ( m_mask & wxLIST_MASK_IMAGE )
        m_image = item.m_image;

    if ( m_mask & wxLIST_MASK_FORMAT )
        m_format = item.m_format;

    if ( m_mask & wxLIST_MASK_WIDTH )
        SetWidth(item.m_width);
}

void wxODListHeaderData::SetPosition( int x, int y )
{
    m_xpos = x;
    m_ypos = y;
}

void wxODListHeaderData::SetHeight( int h )
{
    m_height = h;
}

void wxODListHeaderData::SetWidth( int w )
{
    m_width = w;
    if (m_width < 0)
        m_width = WIDTH_COL_DEFAULT;
    else if (m_width < WIDTH_COL_MIN)
        m_width = WIDTH_COL_MIN;
}

void wxODListHeaderData::SetFormat( int format )
{
    m_format = format;
}

bool wxODListHeaderData::HasImage() const
{
    return m_image != -1;
}

bool wxODListHeaderData::IsHit( int x, int y ) const
{
    return ((x >= m_xpos) && (x <= m_xpos+m_width) && (y >= m_ypos) && (y <= m_ypos+m_height));
}

void wxODListHeaderData::GetItem( wxListItem& item )
{
    item.m_mask = m_mask;
    item.m_text = m_text;
    item.m_image = m_image;
    item.m_format = m_format;
    item.m_width = m_width;
}

int wxODListHeaderData::GetImage() const
{
    return m_image;
}

int wxODListHeaderData::GetWidth() const
{
    return m_width;
}

int wxODListHeaderData::GetFormat() const
{
    return m_format;
}

//-----------------------------------------------------------------------------
//  wxODListLineData
//-----------------------------------------------------------------------------

inline int wxODListLineData::GetMode() const
{
    return m_owner->GetListCtrl()->GetWindowStyleFlag() & wxLC_MASK_TYPE;
}

inline bool wxODListLineData::InReportView() const
{
    return m_owner->HasFlag(wxLC_REPORT);
}

inline bool wxODListLineData::IsVirtual() const
{
    return m_owner->IsVirtual();
}

wxODListLineData::wxODListLineData( wxODListMainWindow *owner )
{
    m_owner = owner;
    m_items.DeleteContents( TRUE );

    if ( InReportView() )
    {
        m_gi = NULL;
    }
    else // !report
    {
        m_gi = new GeometryInfo;
    }

    m_highlighted = FALSE;

    InitItems( GetMode() == wxLC_REPORT ? m_owner->GetColumnCount() : 1 );
}

void wxODListLineData::CalculateSize( wxDC *dc, int spacing )
{
    wxODListItemDataList::Node *node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    wxODListItemData *item = node->GetData();

    switch ( GetMode() )
    {
        case wxLC_ICON:
        case wxLC_SMALL_ICON:
            {
                m_gi->m_rectAll.width = spacing;

                wxString s = item->GetText();

                wxCoord lw, lh;
                if ( s.empty() )
                {
                    lh =
                    m_gi->m_rectLabel.width =
                    m_gi->m_rectLabel.height = 0;
                }
                else // has label
                {
                    dc->GetTextExtent( s, &lw, &lh );
                    if (lh < SCROLL_UNIT_Y)
                        lh = SCROLL_UNIT_Y;
                    lw += EXTRA_WIDTH;
                    lh += EXTRA_HEIGHT;

                    m_gi->m_rectAll.height = spacing + lh;
                    if (lw > spacing)
                        m_gi->m_rectAll.width = lw;

                    m_gi->m_rectLabel.width = lw;
                    m_gi->m_rectLabel.height = lh;
                }

                if (item->HasImage())
                {
                    int w, h;
                    m_owner->GetImageSize( item->GetImage(), w, h );
                    m_gi->m_rectIcon.width = w + 8;
                    m_gi->m_rectIcon.height = h + 8;

                    if ( m_gi->m_rectIcon.width > m_gi->m_rectAll.width )
                        m_gi->m_rectAll.width = m_gi->m_rectIcon.width;
                    if ( m_gi->m_rectIcon.height + lh > m_gi->m_rectAll.height - 4 )
                        m_gi->m_rectAll.height = m_gi->m_rectIcon.height + lh + 4;
                }

                if ( item->HasText() )
                {
                    m_gi->m_rectHighlight.width = m_gi->m_rectLabel.width;
                    m_gi->m_rectHighlight.height = m_gi->m_rectLabel.height;
                }
                else // no text, highlight the icon
                {
                    m_gi->m_rectHighlight.width = m_gi->m_rectIcon.width;
                    m_gi->m_rectHighlight.height = m_gi->m_rectIcon.height;
                }
            }
            break;

        case wxLC_LIST:
            {
                wxString s = item->GetTextForMeasuring();

                wxCoord lw,lh;
                dc->GetTextExtent( s, &lw, &lh );
                if (lh < SCROLL_UNIT_Y)
                    lh = SCROLL_UNIT_Y;
                lw += EXTRA_WIDTH;
                lh += EXTRA_HEIGHT;

                m_gi->m_rectLabel.width = lw;
                m_gi->m_rectLabel.height = lh;

                m_gi->m_rectAll.width = lw;
                m_gi->m_rectAll.height = lh;

                if (item->HasImage())
                {
                    int w, h;
                    m_owner->GetImageSize( item->GetImage(), w, h );
                    m_gi->m_rectIcon.width = w;
                    m_gi->m_rectIcon.height = h;

                    m_gi->m_rectAll.width += 4 + w;
                    if (h > m_gi->m_rectAll.height)
                        m_gi->m_rectAll.height = h;
                }

                m_gi->m_rectHighlight.width = m_gi->m_rectAll.width;
                m_gi->m_rectHighlight.height = m_gi->m_rectAll.height;
            }
            break;

        case wxLC_REPORT:
            wxFAIL_MSG( _T("unexpected call to SetSize") );
            break;

        default:
            wxFAIL_MSG( _T("unknown mode") );
    }
}

void wxODListLineData::SetPosition( int x, int y,
                                  int window_width,
                                  int spacing )
{
    wxODListItemDataList::Node *node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    wxODListItemData *item = node->GetData();

    switch ( GetMode() )
    {
        case wxLC_ICON:
        case wxLC_SMALL_ICON:
            m_gi->m_rectAll.x = x;
            m_gi->m_rectAll.y = y;

            if ( item->HasImage() )
            {
                m_gi->m_rectIcon.x = m_gi->m_rectAll.x + 4 +
                    (m_gi->m_rectAll.width - m_gi->m_rectIcon.width) / 2;
                m_gi->m_rectIcon.y = m_gi->m_rectAll.y + 4;
            }

            if ( item->HasText() )
            {
                if (m_gi->m_rectAll.width > spacing)
                    m_gi->m_rectLabel.x = m_gi->m_rectAll.x + 2;
                else
                    m_gi->m_rectLabel.x = m_gi->m_rectAll.x + 2 + (spacing/2) - (m_gi->m_rectLabel.width/2);
                m_gi->m_rectLabel.y = m_gi->m_rectAll.y + m_gi->m_rectAll.height + 2 - m_gi->m_rectLabel.height;
                m_gi->m_rectHighlight.x = m_gi->m_rectLabel.x - 2;
                m_gi->m_rectHighlight.y = m_gi->m_rectLabel.y - 2;
            }
            else // no text, highlight the icon
            {
                m_gi->m_rectHighlight.x = m_gi->m_rectIcon.x - 4;
                m_gi->m_rectHighlight.y = m_gi->m_rectIcon.y - 4;
            }
            break;

        case wxLC_LIST:
            m_gi->m_rectAll.x = x;
            m_gi->m_rectAll.y = y;

            m_gi->m_rectHighlight.x = m_gi->m_rectAll.x;
            m_gi->m_rectHighlight.y = m_gi->m_rectAll.y;
            m_gi->m_rectLabel.y = m_gi->m_rectAll.y + 2;

            if (item->HasImage())
            {
                m_gi->m_rectIcon.x = m_gi->m_rectAll.x + 2;
                m_gi->m_rectIcon.y = m_gi->m_rectAll.y + 2;
                m_gi->m_rectLabel.x = m_gi->m_rectAll.x + 6 + m_gi->m_rectIcon.width;
            }
            else
            {
                m_gi->m_rectLabel.x = m_gi->m_rectAll.x + 2;
            }
            break;

        case wxLC_REPORT:
            wxFAIL_MSG( _T("unexpected call to SetPosition") );
            break;

        default:
            wxFAIL_MSG( _T("unknown mode") );
    }
}

void wxODListLineData::InitItems( int num )
{
    for (int i = 0; i < num; i++)
        m_items.Append( new wxODListItemData(m_owner) );
}

void wxODListLineData::SetItem( int index, const wxListItem &info )
{
    wxODListItemDataList::Node *node = m_items.Item( index );
    wxCHECK_RET( node, _T("invalid column index in SetItem") );

    wxODListItemData *item = node->GetData();
    item->SetItem( info );
}

void wxODListLineData::GetItem( int index, wxListItem &info )
{
    wxODListItemDataList::Node *node = m_items.Item( index );
    if (node)
    {
        wxODListItemData *item = node->GetData();
        item->GetItem( info );
    }
}

wxString wxODListLineData::GetText(int index) const
{
    wxString s;

    wxODListItemDataList::Node *node = m_items.Item( index );
    if (node)
    {
        wxODListItemData *item = node->GetData();
        s = item->GetText();
    }

    return s;
}

void wxODListLineData::SetText( int index, const wxString s )
{
    wxODListItemDataList::Node *node = m_items.Item( index );
    if (node)
    {
        wxODListItemData *item = node->GetData();
        item->SetText( s );
    }
}

void wxODListLineData::SetImage( int index, int image )
{
    wxODListItemDataList::Node *node = m_items.Item( index );
    wxCHECK_RET( node, _T("invalid column index in SetImage()") );

    wxODListItemData *item = node->GetData();
    item->SetImage(image);
}

int wxODListLineData::GetImage( int index ) const
{
    wxODListItemDataList::Node *node = m_items.Item( index );
    wxCHECK_MSG( node, -1, _T("invalid column index in GetImage()") );

    wxODListItemData *item = node->GetData();
    return item->GetImage();
}

wxListItemAttr *wxODListLineData::GetAttr() const
{
    wxODListItemDataList::Node *node = m_items.GetFirst();
    wxCHECK_MSG( node, NULL, _T("invalid column index in GetAttr()") );

    wxODListItemData *item = node->GetData();
    return item->GetAttr();
}

void wxODListLineData::SetAttr(wxListItemAttr *attr)
{
    wxODListItemDataList::Node *node = m_items.GetFirst();
    wxCHECK_RET( node, _T("invalid column index in SetAttr()") );

    wxODListItemData *item = node->GetData();
    item->SetAttr(attr);
}

bool wxODListLineData::SetAttributes(wxDC *dc,
                                   const wxListItemAttr *attr,
                                   bool highlighted)
{
    wxWindow *listctrl = m_owner->GetParent();

    // fg colour

    // don't use foreground colour for drawing highlighted items - this might
    // make them completely invisible (and there is no way to do bit
    // arithmetics on wxColour, unfortunately)
    wxColour colText;
    if ( highlighted )
    {
        colText = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
    }
    else
    {
        if ( attr && attr->HasTextColour() )
        {
            colText = attr->GetTextColour();
        }
        else
        {
            colText = listctrl->GetForegroundColour();
        }
    }

    dc->SetTextForeground(colText);

    // font
    wxFont font;
    if ( attr && attr->HasFont() )
    {
        font = attr->GetFont();
    }
    else
    {
        font = listctrl->GetFont();
    }

    dc->SetFont(font);

    // bg colour
    bool hasBgCol = attr && attr->HasBackgroundColour();
    if ( highlighted || hasBgCol )
    {
        if ( highlighted )
        {
	  dc->SetBrush( *m_owner->GetHighlightBrush() );
        }
        else
        {
            dc->SetBrush(wxBrush(attr->GetBackgroundColour(), wxSOLID));
        }

        dc->SetPen( *wxTRANSPARENT_PEN );

        return TRUE;
    }

    return FALSE;
}

void wxODListLineData::Draw( wxDC *dc )
{
    wxODListItemDataList::Node *node = m_items.GetFirst();
    wxCHECK_RET( node, _T("no subitems at all??") );

    bool highlighted = IsHighlighted();

    wxListItemAttr *attr = GetAttr();

    if ( SetAttributes(dc, attr, highlighted) )
    {
        dc->DrawRectangle( m_gi->m_rectHighlight );
    }

    wxODListItemData *item = node->GetData();
    if (item->HasImage())
    {
        wxRect rectIcon = m_gi->m_rectIcon;
        m_owner->DrawImage( item->GetImage(), dc,
                            rectIcon.x, rectIcon.y );
    }

    if (item->HasText())
    {
        wxRect rectLabel = m_gi->m_rectLabel;

        wxDCClipper clipper(*dc, rectLabel);
        dc->DrawText( item->GetText(), rectLabel.x, rectLabel.y );
    }
}

void wxODListLineData::DrawInReportMode( wxDC *dc,
                                       const wxRect& rect,
                                       const wxRect& rectHL,
                                       bool highlighted )
{
    // TODO: later we should support setting different attributes for
    //       different columns - to do it, just add "col" argument to
    //       GetAttr() and move these lines into the loop below
    wxListItemAttr *attr = GetAttr();
    if ( SetAttributes(dc, attr, highlighted) )
    {
      wxPen mypen=*wxTRANSPARENT_PEN;
      if(highlighted) {
	wxColour old=m_owner->GetHighlightBrush()->GetColour();
	wxColour newcol=wxColour(((int)old.Red()*65)/100,
				 ((int)old.Green()*65)/100,
				 ((int)old.Blue()*65)/100);
	mypen=wxPen(newcol,1,wxSOLID);
      }
      dc->SetPen(mypen);
      dc->DrawRectangle( rectHL );
      dc->SetPen(*wxTRANSPARENT_PEN);
    }

    wxCoord x = rect.x + HEADER_OFFSET_X,
            y = rect.y + (LINE_SPACING + EXTRA_HEIGHT) / 2;

    size_t col = 0;
    for ( wxODListItemDataList::Node *node = m_items.GetFirst();
          node;
          node = node->GetNext(), col++ )
    {
        wxODListItemData *item = node->GetData();

        int width = m_owner->GetColumnWidth(col);
        int xOld = x;
        x += width;

        if ( item->HasImage() )
        {
            int ix, iy;
            m_owner->DrawImage( item->GetImage(), dc, xOld, y );
            m_owner->GetImageSize( item->GetImage(), ix, iy );

            ix += IMAGE_MARGIN_IN_REPORT_MODE;

            xOld += ix;
            width -= ix;
        }

        wxDCClipper clipper(*dc, xOld, y, width - 8, rect.height);

        if ( item->HasText() )
        {
            DrawTextFormatted(dc, item->GetText(), col, xOld, y, width - 8);
        }
    }
}

void wxODListLineData::DrawTextFormatted(wxDC *dc,
                                       const wxString &text,
                                       int col,
                                       int x,
                                       int y,
                                       int width)
{
    wxString drawntext, ellipsis;
    wxCoord w, h, base_w;
    wxListItem item;

    // determine if the string can fit inside the current width
    dc->GetTextExtent(text, &w, &h);

    // if it can, draw it
    if (w <= width)
    {
        m_owner->GetColumn(col, item);
        if (item.m_format == wxLIST_FORMAT_LEFT)
            dc->DrawText(text, x, y);
        else if (item.m_format == wxLIST_FORMAT_RIGHT)
            dc->DrawText(text, x + width - w, y);
        else if (item.m_format == wxLIST_FORMAT_CENTER)
            dc->DrawText(text, x + ((width - w) / 2), y);
    }
    else // otherwise, truncate and add an ellipsis if possible
    {
        // determine the base width
        ellipsis = wxString(wxT("..."));
        dc->GetTextExtent(ellipsis, &base_w, &h);

        // continue until we have enough space or only one character left
        drawntext = text.Left(text.Length() - 1);
        while (drawntext.Length() > 1)
        {
            dc->GetTextExtent(drawntext, &w, &h);
            if (w + base_w <= width)
                break;
            drawntext = drawntext.Left(drawntext.Length() - 1);
        }

        // if still not enough space, remove ellipsis characters
        while (ellipsis.Length() > 0 && w + base_w > width)
        {
            ellipsis = ellipsis.Left(ellipsis.Length() - 1);
            dc->GetTextExtent(ellipsis, &base_w, &h);
        }

        // now draw the text
        dc->DrawText(drawntext, x, y);
        dc->DrawText(ellipsis, x + w, y);
    }
}

bool wxODListLineData::Highlight( bool on )
{
    wxCHECK_MSG( !m_owner->IsVirtual(), FALSE, _T("unexpected call to Highlight") );

    if ( on == m_highlighted )
        return FALSE;

    m_highlighted = on;

    return TRUE;
}

void wxODListLineData::ReverseHighlight( void )
{
    Highlight(!IsHighlighted());
}

//-----------------------------------------------------------------------------
//  wxODListHeaderWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxODListHeaderWindow,wxWindow)

#include "pixmaps/sort_dn.xpm"
#include "pixmaps/sort_up.xpm"

BEGIN_EVENT_TABLE(wxODListHeaderWindow,wxWindow)
    EVT_PAINT         (wxODListHeaderWindow::OnPaint)
    EVT_MOUSE_EVENTS  (wxODListHeaderWindow::OnMouse)
    EVT_SET_FOCUS     (wxODListHeaderWindow::OnSetFocus)
END_EVENT_TABLE()

void wxODListHeaderWindow::Init()
{
    m_currentCursor = (wxCursor *) NULL;
    m_isDragging = FALSE;
    m_dirty = FALSE;
}

wxODListHeaderWindow::wxODListHeaderWindow()
{
    Init();

    m_owner = (wxODListMainWindow *) NULL;
    m_resizeCursor = (wxCursor *) NULL;
}

wxODListHeaderWindow::wxODListHeaderWindow( wxWindow *win,
                                        wxWindowID id,
                                        wxODListMainWindow *owner,
                                        const wxPoint& pos,
                                        const wxSize& size,
                                        long style,
                                        const wxString &name )
                  : wxWindow( win, id, pos, size, style, name )
{
    Init();

    m_owner = owner;
    m_resizeCursor = new wxCursor( wxCURSOR_SIZEWE );

    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
}

wxODListHeaderWindow::~wxODListHeaderWindow()
{
    delete m_resizeCursor;
}

#ifdef __WXUNIVERSAL__
#include "wx/univ/renderer.h"
#include "wx/univ/theme.h"
#endif

void wxODListHeaderWindow::DoDrawRect( wxDC *dc, int x, int y, int w, int h )
{
#if defined(__WXGTK__) && !defined(__WXUNIVERSAL__)
    GtkStateType state = m_parent->IsEnabled() ? GTK_STATE_NORMAL
                                               : GTK_STATE_INSENSITIVE;

    x = dc->XLOG2DEV( x );

    gtk_paint_box (m_wxwindow->style, GTK_PIZZA(m_wxwindow)->bin_window,
                   state, GTK_SHADOW_OUT,
                   (GdkRectangle*) NULL, m_wxwindow,
                   (char *)"button", // const_cast
                   x-1, y-1, w+2, h+2);
#elif defined(__WXUNIVERSAL__)
    wxTheme *theme = wxTheme::Get();
    wxRenderer *renderer = theme->GetRenderer();
    renderer->DrawBorder( *dc, wxBORDER_RAISED, wxRect(x,y,w,h), 0 );
#elif defined(__WXMAC__)
    const int m_corner = 1;

    dc->SetBrush( *wxTRANSPARENT_BRUSH );

    dc->SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNSHADOW ) , 1 , wxSOLID ) );
    dc->DrawLine( x+w-m_corner+1, y, x+w, y+h );  // right (outer)
    dc->DrawRectangle( x, y+h, w+1, 1 );          // bottom (outer)

    wxPen pen( wxColour( 0x88 , 0x88 , 0x88 ), 1, wxSOLID );

    dc->SetPen( pen );
    dc->DrawLine( x+w-m_corner, y, x+w-1, y+h );  // right (inner)
    dc->DrawRectangle( x+1, y+h-1, w-2, 1 );      // bottom (inner)

    dc->SetPen( *wxWHITE_PEN );
    dc->DrawRectangle( x, y, w-m_corner+1, 1 );   // top (outer)
    dc->DrawRectangle( x, y, 1, h );              // left (outer)
    dc->DrawLine( x, y+h-1, x+1, y+h-1 );
    dc->DrawLine( x+w-1, y, x+w-1, y+1 );
#else // !GTK, !Mac
    const int m_corner = 1;

    dc->SetBrush( *wxTRANSPARENT_BRUSH );

    dc->SetPen( *wxBLACK_PEN );
    dc->DrawLine( x+w-m_corner+1, y, x+w, y+h );  // right (outer)
    dc->DrawRectangle( x, y+h, w+1, 1 );          // bottom (outer)

    wxPen pen( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNSHADOW ), 1, wxSOLID );

    dc->SetPen( pen );
    dc->DrawLine( x+w-m_corner, y, x+w-1, y+h );  // right (inner)
    dc->DrawRectangle( x+1, y+h-1, w-2, 1 );      // bottom (inner)

    dc->SetPen( *wxWHITE_PEN );
    dc->DrawRectangle( x, y, w-m_corner+1, 1 );   // top (outer)
    dc->DrawRectangle( x, y, 1, h );              // left (outer)
    dc->DrawLine( x, y+h-1, x+1, y+h-1 );
    dc->DrawLine( x+w-1, y, x+w-1, y+1 );
#endif
}

// shift the DC origin to match the position of the main window horz
// scrollbar: this allows us to always use logical coords
void wxODListHeaderWindow::AdjustDC(wxDC& dc)
{
    int xpix;
    m_owner->GetScrollPixelsPerUnit( &xpix, NULL );

    int x;
    m_owner->GetViewStart( &x, NULL );

    // account for the horz scrollbar offset
    dc.SetDeviceOrigin( -x * xpix, 0 );
}

void wxODListHeaderWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
#if defined(__WXGTK__)
    wxClientDC dc( this );
#else
    wxPaintDC dc( this );
#endif

    PrepareDC( dc );
    AdjustDC( dc );

    dc.BeginDrawing();

    dc.SetFont( GetFont() );

    // width and height of the entire header window
    int w, h;
    GetClientSize( &w, &h );
    m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);

    dc.SetBackgroundMode(wxTRANSPARENT);

    // do *not* use the listctrl colour for headers - one day we will have a
    // function to set it separately
    //dc.SetTextForeground( *wxBLACK );

    dc.SetTextForeground(wxSystemSettings::
                            GetSystemColour( wxSYS_COLOUR_WINDOWTEXT ));

    int x = HEADER_OFFSET_X;

    int numColumns = m_owner->GetColumnCount();
    wxListItem item;
    for ( int i = 0; i < numColumns && x < w; i++ )
    {
        m_owner->GetColumn( i, item );
        int wCol = item.m_width;

        // the width of the rect to draw: make it smaller to fit entirely
        // inside the column rect
        int cw = wCol - 2;

        dc.SetPen( *wxWHITE_PEN );

        DoDrawRect( &dc, x, HEADER_OFFSET_Y, cw, h-2 );

        // if we have an image, draw it on the right of the label
        int image = item.m_image;
        if ( image != -1 )
        {
            wxImageListType *imageList = m_owner->m_small_image_list;
            if ( imageList )
            {
                int ix, iy;
                imageList->GetSize(image, ix, iy);

                imageList->Draw
                           (
                            image,
                            dc,
                            x + cw - ix - 1,
                            HEADER_OFFSET_Y + (h - 4 - iy)/2,
                            wxIMAGELIST_DRAW_TRANSPARENT
                           );

                cw -= ix + 2;
            }
            //else: ignore the column image
        }

        // draw the text clipping it so that it doesn't overwrite the column
        // boundary
        wxDCClipper clipper(dc, x, HEADER_OFFSET_Y, cw, h - 4 );

        if (item.GetWidth() > WIDTH_COL_MIN) {
            dc.DrawText( item.GetText(),
                         x + EXTRA_WIDTH, HEADER_OFFSET_Y + EXTRA_HEIGHT );
        }

	if(m_owner->GetSortArrow(i)==1) {
	  dc.DrawBitmap(wxBitmap(sort_dn_xpm),x+wCol-18,HEADER_OFFSET_Y+(h/2-3),TRUE);
	} else if(m_owner->GetSortArrow(i)==2) {
	  dc.DrawBitmap(wxBitmap(sort_up_xpm),x+wCol-18,HEADER_OFFSET_Y+(h/2-3),TRUE);
	}
        x += wCol;
    }

    dc.EndDrawing();
}

void wxODListHeaderWindow::DrawCurrent()
{
    int x1 = m_currentX;
    int y1 = 0;
    m_owner->ClientToScreen( &x1, &y1 );

    int x2 = m_currentX;
    int y2 = 0;
    m_owner->GetClientSize( NULL, &y2 );
    m_owner->ClientToScreen( &x2, &y2 );

    wxScreenDC dc;
    dc.SetLogicalFunction( wxINVERT );
    dc.SetPen( wxPen( *wxBLACK, 2, wxSOLID ) );
    dc.SetBrush( *wxTRANSPARENT_BRUSH );

    AdjustDC(dc);

    dc.DrawLine( x1, y1, x2, y2 );

    dc.SetLogicalFunction( wxCOPY );

    dc.SetPen( wxNullPen );
    dc.SetBrush( wxNullBrush );
}

void wxODListHeaderWindow::OnMouse( wxMouseEvent &event )
{
    // we want to work with logical coords
    int x;
    m_owner->CalcUnscrolledPosition(event.GetX(), 0, &x, NULL);
    int y = event.GetY();

    if (m_isDragging)
    {
        SendListEvent(wxEVT_COMMAND_LIST_COL_DRAGGING,
                      event.GetPosition());

        // we don't draw the line beyond our window, but we allow dragging it
        // there
        int w = 0;
        GetClientSize( &w, NULL );
        m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);
        w -= 6;

        // erase the line if it was drawn
        if ( m_currentX < w )
            DrawCurrent();

        if (event.ButtonUp())
        {
            ReleaseMouse();
            m_isDragging = FALSE;
            m_dirty = TRUE;
            m_owner->SetColumnWidth( m_column, m_currentX - m_minX );
            SendListEvent(wxEVT_COMMAND_LIST_COL_END_DRAG,
                          event.GetPosition());
        }
        else
        {
            if (x > m_minX + 7)
                m_currentX = x;
            else
                m_currentX = m_minX + 7;

            // draw in the new location
            if ( m_currentX < w )
                DrawCurrent();
        }
    }
    else // not dragging
    {
        m_minX = 0;
        bool hit_border = FALSE;

        // end of the current column
        int xpos = 0;

        // find the column where this event occured
        int col,
            countCol = m_owner->GetColumnCount();
        for (col = 0; col < countCol; col++)
        {
            xpos += m_owner->GetColumnWidth( col );
            m_column = col;

            if ( (abs(x-xpos) < 3) && (y < 22) )
            {
                // near the column border
                hit_border = TRUE;
                break;
            }

            if ( x < xpos )
            {
                // inside the column
                break;
            }

            m_minX = xpos;
        }

        if ( col == countCol )
            m_column = -1;

        if (event.LeftDown() || event.RightDown() || event.MiddleDown())
        {
            if (hit_border && event.LeftDown())
            {
                if ( SendListEvent(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG,
                                   event.GetPosition()) )
                {
                    m_isDragging = TRUE;
                    m_currentX = x;
                    DrawCurrent();
                    CaptureMouse();
                }
                //else: column resizing was vetoed by the user code
            }
			else // click on a column
            {
        if (event.LeftDown()) {
            SendListEvent(wxEVT_COMMAND_LIST_COL_CLICK, event.GetPosition());
        }
        if (event.RightDown()) {
            SendListEvent(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK, event.GetPosition());
        }
        if (event.MiddleDown()) {
            SendListEvent(wxEVT_COMMAND_LIST_COL_MIDDLE_CLICK, event.GetPosition());
        }
            }

        }
        else if (event.Moving())
        {
            bool setCursor;
            if (hit_border)
            {
                setCursor = m_currentCursor == wxSTANDARD_CURSOR;
                m_currentCursor = m_resizeCursor;
            }
            else
            {
                setCursor = m_currentCursor != wxSTANDARD_CURSOR;
                m_currentCursor = wxSTANDARD_CURSOR;
            }

            if ( setCursor )
                SetCursor(*m_currentCursor);
        }
    }
}

void wxODListHeaderWindow::OnSetFocus( wxFocusEvent &WXUNUSED(event) )
{
    m_owner->SetFocus();
}

bool wxODListHeaderWindow::SendListEvent(wxEventType type, wxPoint pos)
{
    wxWindow *parent = GetParent();
    wxListEvent le( type, parent->GetId() );
    le.SetEventObject( parent );
    le.m_pointDrag = pos;

    // the position should be relative to the parent window, not
    // this one for compatibility with MSW and common sense: the
    // user code doesn't know anything at all about this header
    // window, so why should it get positions relative to it?
    le.m_pointDrag.y -= GetSize().y;

    le.m_col = m_column;
    return !parent->GetEventHandler()->ProcessEvent( le ) || le.IsAllowed();
}

//-----------------------------------------------------------------------------
// wxODListRenameTimer (internal)
//-----------------------------------------------------------------------------

wxODListRenameTimer::wxODListRenameTimer( wxODListMainWindow *owner )
{
    m_owner = owner;
}

void wxODListRenameTimer::Notify()
{
    m_owner->OnRenameTimer();
}

//-----------------------------------------------------------------------------
// wxODListTextCtrl (internal)
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxODListTextCtrl,wxTextCtrl)
    EVT_CHAR           (wxODListTextCtrl::OnChar)
    EVT_KEY_UP         (wxODListTextCtrl::OnKeyUp)
    EVT_KILL_FOCUS     (wxODListTextCtrl::OnKillFocus)
END_EVENT_TABLE()

wxODListTextCtrl::wxODListTextCtrl(wxODListMainWindow *owner, size_t itemEdit)
              : m_startValue(owner->GetItemText(itemEdit)),
                m_itemEdited(itemEdit)
{
    m_owner = owner;
    m_finished = FALSE;

    wxRect rectLabel = owner->GetLineLabelRect(itemEdit);

    m_owner->CalcScrolledPosition(rectLabel.x, rectLabel.y,
                                  &rectLabel.x, &rectLabel.y);

    (void)Create(owner, wxID_ANY, m_startValue,
                 wxPoint(rectLabel.x-4,rectLabel.y-4),
                 wxSize(rectLabel.width+11,rectLabel.height+8));
}

void wxODListTextCtrl::Finish()
{
    if ( !m_finished )
    {
        wxPendingDelete.Append(this);

        m_finished = TRUE;

        m_owner->SetFocus();
    }
}

bool wxODListTextCtrl::AcceptChanges()
{
    const wxString value = GetValue();

    if ( value == m_startValue )
    {
        // nothing changed, always accept
        return TRUE;
    }

    if ( !m_owner->OnRenameAccept(m_itemEdited, value) )
    {
        // vetoed by the user
        return FALSE;
    }

    // accepted, do rename the item
    m_owner->SetItemText(m_itemEdited, value);

    return TRUE;
}

void wxODListTextCtrl::OnChar( wxKeyEvent &event )
{
    switch ( event.m_keyCode )
    {
        case WXK_RETURN:
            if ( !AcceptChanges() )
            {
                // vetoed by the user code
                break;
            }
            //else: fall through

        case WXK_ESCAPE:
            Finish();
            m_owner->OnRenameCancelled( m_itemEdited );
            break;

        default:
            event.Skip();
    }
}

void wxODListTextCtrl::OnKeyUp( wxKeyEvent &event )
{
    if (m_finished)
    {
        event.Skip();
        return;
    }

    // auto-grow the textctrl:
    wxSize parentSize = m_owner->GetSize();
    wxPoint myPos = GetPosition();
    wxSize mySize = GetSize();
    int sx, sy;
    GetTextExtent(GetValue() + _T("MM"), &sx, &sy);
    if (myPos.x + sx > parentSize.x)
        sx = parentSize.x - myPos.x;
    if (mySize.x > sx)
        sx = mySize.x;
    SetSize(sx, -1);

    event.Skip();
}

void wxODListTextCtrl::OnKillFocus( wxFocusEvent &event )
{
    if ( !m_finished )
    {
        // We must finish regardless of success, otherwise we'll get focus problems
        Finish();
    
        if ( !AcceptChanges() )
            m_owner->OnRenameCancelled( m_itemEdited );
    }
        
    event.Skip();
}

//-----------------------------------------------------------------------------
//  wxODListMainWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxODListMainWindow,wxScrolledWindow)

BEGIN_EVENT_TABLE(wxODListMainWindow,wxScrolledWindow)
  EVT_PAINT          (wxODListMainWindow::OnPaint)
  EVT_ERASE_BACKGROUND (wxODListMainWindow::OnErase)
  EVT_MOUSE_EVENTS   (wxODListMainWindow::OnMouse)
  EVT_CHAR           (wxODListMainWindow::OnChar)
  EVT_KEY_DOWN       (wxODListMainWindow::OnKeyDown)
  EVT_SET_FOCUS      (wxODListMainWindow::OnSetFocus)
  EVT_KILL_FOCUS     (wxODListMainWindow::OnKillFocus)
  EVT_SCROLLWIN      (wxODListMainWindow::OnScroll)
END_EVENT_TABLE()

void wxODListMainWindow::Init()
{
    m_columns.DeleteContents( TRUE );
    m_dirty = TRUE;
    m_countVirt = 0;
    m_lineFrom =
    m_lineTo = (size_t)-1;
    m_linesPerPage = 0;

    m_headerWidth =
    m_lineHeight = 0;

    m_small_image_list = (wxImageListType *) NULL;
    m_normal_image_list = (wxImageListType *) NULL;

    m_small_spacing = 30;
    m_normal_spacing = 40;

    m_hasFocus = FALSE;
    m_dragCount = 0;
    m_isCreated = FALSE;

    m_lastOnSame = FALSE;
    m_renameTimer = new wxODListRenameTimer( this );

    m_current =
    m_lineLastClicked =
    m_lineBeforeLastClicked = (size_t)-1;

    m_freezeCount = 0;
}

void wxODListMainWindow::InitScrolling()
{
    if ( HasFlag(wxLC_REPORT) )
    {
        m_xScroll = SCROLL_UNIT_X;
        m_yScroll = SCROLL_UNIT_Y;
    }
    else
    {
        m_xScroll = SCROLL_UNIT_Y;
        m_yScroll = 0;
    }
}

wxODListMainWindow::wxODListMainWindow()
{
    Init();

    m_highlightBrush =
    m_highlightUnfocusedBrush = (wxBrush *) NULL;

    m_xScroll =
    m_yScroll = 0;
}

wxODListMainWindow::wxODListMainWindow( wxWindow *parent,
                                    wxWindowID id,
                                    const wxPoint& pos,
                                    const wxSize& size,
                                    long style,
                                    const wxString &name )
                : wxScrolledWindow( parent, id, pos, size,
                                    style | wxHSCROLL | wxVSCROLL, name )
{
    Init();

    wxColour col=wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
    wxColour newcol=wxColour(G_BLEND(col.Red(),125),
			     G_BLEND(col.Green(),125),
			     G_BLEND(col.Blue(),125));
    m_highlightBrush=new wxBrush(newcol,wxSOLID);

    /*m_highlightBrush = new wxBrush
                           (
                            wxSystemSettings::GetColour
                            (
                                wxSYS_COLOUR_HIGHLIGHT
                            ),
                            wxSOLID
			    );*/

    col=wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    newcol=wxColour(G_BLEND(col.Red(),125),
		    G_BLEND(col.Green(),125),
		    G_BLEND(col.Blue(),125));
    m_highlightUnfocusedBrush=new wxBrush(newcol,wxSOLID);

    /*m_highlightUnfocusedBrush = new wxBrush
                                    (
                                       wxSystemSettings::GetColour
                                       (
                                           wxSYS_COLOUR_BTNSHADOW
                                       ),
                                       wxSOLID
				       );*/

    wxSize sz = size;
    sz.y = 25;

    InitScrolling();
    SetScrollbars( m_xScroll, m_yScroll, 0, 0, 0, 0 );

    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_LISTBOX ) );
}

wxODListMainWindow::~wxODListMainWindow()
{
    DoDeleteAllItems();

    delete m_highlightBrush;
    delete m_highlightUnfocusedBrush;

    delete m_renameTimer;
}

void wxODListMainWindow::CacheLineData(size_t line)
{
    wxODGenericListCtrl *listctrl = GetListCtrl();

    wxODListLineData *ld = GetDummyLine();

    size_t countCol = GetColumnCount();
    for ( size_t col = 0; col < countCol; col++ )
    {
        ld->SetText(col, listctrl->OnGetItemText(line, col));
    }

    ld->SetImage(listctrl->OnGetItemImage(line));
    ld->SetAttr(listctrl->OnGetItemAttr(line));
}

wxODListLineData *wxODListMainWindow::GetDummyLine() const
{
    wxASSERT_MSG( !IsEmpty(), _T("invalid line index") );

    wxASSERT_MSG( IsVirtual(), _T("GetDummyLine() shouldn't be called") );

    wxODListMainWindow *self = wxConstCast(this, wxODListMainWindow);

    // we need to recreate the dummy line if the number of columns in the
    // control changed as it would have the incorrect number of fields
    // otherwise
    if ( !m_lines.IsEmpty() &&
            m_lines[0].m_items.GetCount() != (size_t)GetColumnCount() )
    {
        self->m_lines.Clear();
    }

    if ( m_lines.IsEmpty() )
    {
        wxODListLineData *line = new wxODListLineData(self);
        self->m_lines.Add(line);

        // don't waste extra memory -- there never going to be anything
        // else/more in this array
        self->m_lines.Shrink();
    }

    return &m_lines[0];
}

// ----------------------------------------------------------------------------
// line geometry (report mode only)
// ----------------------------------------------------------------------------

wxCoord wxODListMainWindow::GetLineHeight() const
{
    wxASSERT_MSG( HasFlag(wxLC_REPORT), _T("only works in report mode") );

    // we cache the line height as calling GetTextExtent() is slow
    if ( !m_lineHeight )
    {
        wxODListMainWindow *self = wxConstCast(this, wxODListMainWindow);

        wxClientDC dc( self );
        dc.SetFont( GetFont() );

        wxCoord y;
        dc.GetTextExtent(_T("H"), NULL, &y);

        if ( y < SCROLL_UNIT_Y )
            y = SCROLL_UNIT_Y;

        if ( m_small_image_list && m_small_image_list->GetImageCount() )
        {
            int iw = 0;
            int ih = 0;
            m_small_image_list->GetSize(0, iw, ih);
            y = wxMax(y, ih);
        }

        y += EXTRA_HEIGHT;
        self->m_lineHeight = y + LINE_SPACING;
    }

    return m_lineHeight;
}

wxCoord wxODListMainWindow::GetLineY(size_t line) const
{
    wxASSERT_MSG( HasFlag(wxLC_REPORT), _T("only works in report mode") );

    return LINE_SPACING + line*GetLineHeight();
}

wxRect wxODListMainWindow::GetLineRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectAll;

    wxRect rect;
    rect.x = HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    rect.width = GetHeaderWidth();
    rect.height = GetLineHeight();

    return rect;
}

wxRect wxODListMainWindow::GetLineLabelRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectLabel;

    wxRect rect;
    rect.x = HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    rect.width = GetColumnWidth(0);
    rect.height = GetLineHeight();

    return rect;
}

wxRect wxODListMainWindow::GetLineIconRect(size_t line) const
{
    if ( !InReportView() )
        return GetLine(line)->m_gi->m_rectIcon;

    wxODListLineData *ld = GetLine(line);
    wxASSERT_MSG( ld->HasImage(), _T("should have an image") );

    wxRect rect;
    rect.x = HEADER_OFFSET_X;
    rect.y = GetLineY(line);
    GetImageSize(ld->GetImage(), rect.width, rect.height);

    return rect;
}

wxRect wxODListMainWindow::GetLineHighlightRect(size_t line) const
{
    return InReportView() ? GetLineRect(line)
                          : GetLine(line)->m_gi->m_rectHighlight;
}

long wxODListMainWindow::HitTestLine(size_t line, int x, int y) const
{
    wxASSERT_MSG( line < GetItemCount(), _T("invalid line in HitTestLine") );

    wxODListLineData *ld = GetLine(line);

    if ( ld->HasImage() && GetLineIconRect(line).Inside(x, y) )
        return wxLIST_HITTEST_ONITEMICON;

    // VS: Testing for "ld->HasText() || InReportView()" instead of
    //     "ld->HasText()" is needed to make empty lines in report view
    //     possible
    if ( ld->HasText() || InReportView() )
    {
        wxRect rect = InReportView() ? GetLineRect(line)
                                     : GetLineLabelRect(line);

        if ( rect.Inside(x, y) )
            return wxLIST_HITTEST_ONITEMLABEL;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// highlight (selection) handling
// ----------------------------------------------------------------------------

bool wxODListMainWindow::IsHighlighted(size_t line) const
{
    if ( IsVirtual() )
    {
        return m_selStore.IsSelected(line);
    }
    else // !virtual
    {
        wxODListLineData *ld = GetLine(line);
        wxCHECK_MSG( ld, FALSE, _T("invalid index in IsHighlighted") );

        return ld->IsHighlighted();
    }
}

void wxODListMainWindow::HighlightLines( size_t lineFrom,
                                       size_t lineTo,
                                       bool highlight )
{
    if ( IsVirtual() )
    {
        wxArrayInt linesChanged;
        if ( !m_selStore.SelectRange(lineFrom, lineTo, highlight,
                                     &linesChanged) )
        {
            // meny items changed state, refresh everything
            RefreshLines(lineFrom, lineTo);
        }
        else // only a few items changed state, refresh only them
        {
            size_t count = linesChanged.GetCount();
            for ( size_t n = 0; n < count; n++ )
            {
                RefreshLine(linesChanged[n]);
            }
        }
    }
    else // iterate over all items in non report view
    {
        for ( size_t line = lineFrom; line <= lineTo; line++ )
        {
            if ( HighlightLine(line, highlight) )
            {
                RefreshLine(line);
            }
        }
    }
}

bool wxODListMainWindow::HighlightLine( size_t line, bool highlight )
{
    bool changed;

    if ( IsVirtual() )
    {
        changed = m_selStore.SelectItem(line, highlight);
    }
    else // !virtual
    {
        wxODListLineData *ld = GetLine(line);
        wxCHECK_MSG( ld, FALSE, _T("invalid index in HighlightLine") );

        changed = ld->Highlight(highlight);
    }

    if ( changed )
    {
        SendNotify( line, highlight ? wxEVT_COMMAND_LIST_ITEM_SELECTED
                                    : wxEVT_COMMAND_LIST_ITEM_DESELECTED );
    }

    return changed;
}

void wxODListMainWindow::RefreshLine( size_t line )
{
    if ( HasFlag(wxLC_REPORT) )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( line < visibleFrom || line > visibleTo )
            return;
    }

    wxRect rect = GetLineRect(line);

    // this can't be right either
    int nullX;
    CalcScrolledPosition( rect.x, rect.y, &nullX, &rect.y );
    //RefreshRect( rect );

    //printf("**linerect %d %d %d %d\n",rect.x,rect.y,rect.width,rect.height);

    if(rect.x<0 || rect.y<0 || rect.width<0 || rect.height<0)
      return;
    Refresh(FALSE,&rect);
}

void wxODListMainWindow::RefreshLines( size_t lineFrom, size_t lineTo )
{
    // we suppose that they are ordered by caller
    wxASSERT_MSG( lineFrom <= lineTo, _T("indices in disorder") );

    wxASSERT_MSG( lineTo < GetItemCount(), _T("invalid line range") );

    if ( HasFlag(wxLC_REPORT) )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( lineFrom < visibleFrom )
            lineFrom = visibleFrom;
        if ( lineTo > visibleTo )
            lineTo = visibleTo;

        wxRect rect;
        rect.x = 0;
        rect.y = GetLineY(lineFrom);
        rect.width = GetClientSize().x;
        rect.height = GetLineY(lineTo) - rect.y + GetLineHeight();

	// hmm.. this must be wrong. can't use scrolled position.
	int nullX;
        CalcScrolledPosition( rect.x, rect.y, &nullX, &rect.y );
        //RefreshRect( rect );	
	//printf("**linesrect %d %d %d %d\n",rect.x,rect.y,rect.width,rect.height);
	
	if(rect.x<0||rect.y<0||rect.width<0||rect.height<0)
	  return;
	Refresh(FALSE,&rect);
    }
    else // !report
    {
        // TODO: this should be optimized...
        for ( size_t line = lineFrom; line <= lineTo; line++ )
        {
            RefreshLine(line);
        }
    }
}

void wxODListMainWindow::RefreshAfter( size_t lineFrom )
{
    if ( HasFlag(wxLC_REPORT) )
    {
        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        if ( lineFrom < visibleFrom )
            lineFrom = visibleFrom;
        else if ( lineFrom > visibleTo )
            return;

        wxRect rect;
        rect.x = 0;
        rect.y = GetLineY(lineFrom);
        CalcScrolledPosition( rect.x, rect.y, &rect.x, &rect.y );

        wxSize size = GetClientSize();
        rect.width = size.x;
        // refresh till the bottom of the window
        rect.height = size.y - rect.y;

        //RefreshRect( rect );
	if(rect.x<0||rect.y<0||rect.width<0||rect.height<0)
	  return;
	Refresh(FALSE,&rect);
    }
    else // !report
    {
        // TODO: how to do it more efficiently?
        m_dirty = TRUE;
    }
}

void wxODListMainWindow::RefreshSelected()
{
    if ( IsEmpty() )
        return;

    size_t from, to;
    if ( InReportView() )
    {
        GetVisibleLinesRange(&from, &to);
    }
    else // !virtual
    {
        from = 0;
        to = GetItemCount() - 1;
    }

    if ( HasCurrent() && m_current >= from && m_current <= to )
    {
        RefreshLine(m_current);
    }

    for ( size_t line = from; line <= to; line++ )
    {
        // NB: the test works as expected even if m_current == -1
        if ( line != m_current && IsHighlighted(line) )
        {
            RefreshLine(line);
        }
    }
}

void wxODListMainWindow::Freeze()
{
    m_freezeCount++;
}

void wxODListMainWindow::Thaw()
{
    wxCHECK_RET( m_freezeCount > 0, _T("thawing unfrozen list control?") );

    if ( !--m_freezeCount )
    {
        Refresh();
    }
}


void wxODListMainWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    // Note: a wxPaintDC must be constructed even if no drawing is
    // done (a Windows requirement).
    wxPaintDC dc( this );

    if ( IsEmpty() || m_freezeCount )
    {
        // nothing to draw or not the moment to draw it
        return;
    }

    if ( m_dirty )
    {
        // delay the repainting until we calculate all the items positions
        return;
    }

    PrepareDC( dc );

    int dev_x, dev_y;
    CalcScrolledPosition( 0, 0, &dev_x, &dev_y );

    dc.BeginDrawing();

    dc.SetFont( GetFont() );

    if ( HasFlag(wxLC_REPORT) )
    {
        int lineHeight = GetLineHeight();

        size_t visibleFrom, visibleTo;
        GetVisibleLinesRange(&visibleFrom, &visibleTo);

        wxRect rectLine;
        wxCoord xOrig, yOrig;
        CalcUnscrolledPosition(0, 0, &xOrig, &yOrig);

        // tell the caller cache to cache the data
        if ( IsVirtual() )
        {
            wxListEvent evCache(wxEVT_COMMAND_LIST_CACHE_HINT,
                                GetParent()->GetId());
            evCache.SetEventObject( GetParent() );
            evCache.m_oldItemIndex = visibleFrom;
            evCache.m_itemIndex = visibleTo;
            GetParent()->GetEventHandler()->ProcessEvent( evCache );
        }

		// Report view is drawn normally, owner-drawn is double-buffered.
		if ( !HasFlag(wxLC_OWNERDRAW) ) {
			for ( size_t line = visibleFrom; line <= visibleTo; line++ ) {
	            rectLine = GetLineRect(line);

    	        if ( !IsExposed(rectLine.x - xOrig, rectLine.y - yOrig,
        	                    rectLine.width, rectLine.height) )
            	{
                	// don't redraw unaffected lines to avoid flicker
	                continue;
    	        }

				GetLine(line)->DrawInReportMode( &dc, rectLine, GetLineHighlightRect(line), IsHighlighted(line) );
			}
		} else {
			for ( size_t line = visibleFrom; line <= visibleTo; line++ ) {
				rectLine = GetLineRect(line);

            	if ( !IsExposed(rectLine.x - xOrig, rectLine.y - yOrig,
                	            rectLine.width, rectLine.height) )
            	{
                	// don't redraw unaffected lines to avoid flicker
                	continue;
            	}

				// This DC is used to write to the bitmaps used to double-buffer
				wxMemoryDC dbDC( &dc );
				// The font doesn't get set for some reason ...
				dbDC.SetFont( GetFont() );

				// We use this for double-buffer
				wxBitmap buffer( rectLine.width, rectLine.height );

				wxRect rect = rectLine;
				wxRect highl = GetLineHighlightRect(line);

				// Map the highlight rect onto the buffer
				highl.x = highl.x - rectLine.x;
				highl.y = highl.y - rectLine.y;
			
				// Map the normal rect onto the buffer
				rect.x = 0;
				rect.y = 0;

				dbDC.SelectObject( buffer );

				((wxODListCtrl*)m_parent)->OnDrawItem( line, &dbDC, rect, highl, IsHighlighted(line) );

				dc.Blit( rectLine.x, rectLine.y, rectLine.width, rectLine.height, &dbDC, 0, 0 );
			
				dbDC.SelectObject( wxNullBitmap );
			}
	    		

			// Clean up empty spaces because we ignore ERASE_BACKGROUND events
			// Get the last visible item
			wxRect last = GetLineRect(visibleTo);
				
			// The the actual display size
			int width = 0, height = 0;
			GetClientSize( &width, &height );
		
			dc.SetPen( wxPen( GetBackgroundColour(), 1, wxSOLID) );
			dc.SetBrush( wxBrush( GetBackgroundColour(), wxSOLID ) );
		
		
			// Clear below if this is the last item
			if ( visibleTo + 1 == GetItemCount() ) {

				// Since visibleTo can include items partly past the bottom, we have
				// ensure that we dont overwrite items below the visible area
				if ( last.GetBottom() < height ) {
				
					// Clear the area below the last visible item
					dc.DrawRectangle( 0, last.GetBottom(), last.GetWidth(), height - last.GetBottom() );
				}
			}

			// Clear to the right of the last column if the columns dont fill everything
			if ( last.GetWidth() < width ) {
				// Clear the area below the last visible item
				dc.DrawRectangle( last.GetWidth(), 0, width - last.GetWidth(), height );
			}
		}
		
		

        if ( HasFlag(wxLC_HRULES) )
        {
            wxPen pen(GetRuleColour(), 1, wxSOLID);
            wxSize clientSize = GetClientSize();

            // Don't draw the first one
            for ( size_t i = visibleFrom+1; i <= visibleTo; i++ )
            {
                dc.SetPen(pen);
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.DrawLine(0 - dev_x, i*lineHeight,
                            clientSize.x - dev_x, i*lineHeight);
            }

            // Draw last horizontal rule
            if ( visibleTo == GetItemCount() - 1 )
            {
                dc.SetPen(pen);
                dc.SetBrush( *wxTRANSPARENT_BRUSH );
                dc.DrawLine(0 - dev_x, (m_lineTo+1)*lineHeight,
                            clientSize.x - dev_x , (m_lineTo+1)*lineHeight );
            }
        }

        // Draw vertical rules if required
        if ( HasFlag(wxLC_VRULES) && !IsEmpty() )
        {
            wxPen pen(GetRuleColour(), 1, wxSOLID);

            int col = 0;
            wxRect firstItemRect;
            wxRect lastItemRect;
            GetItemRect(visibleFrom, firstItemRect);
            GetItemRect(visibleTo, lastItemRect);
            int x = firstItemRect.GetX();
            dc.SetPen(pen);
            dc.SetBrush(* wxTRANSPARENT_BRUSH);
            for (col = 0; col < GetColumnCount(); col++)
            {
                int colWidth = GetColumnWidth(col);
                x += colWidth;
                dc.DrawLine(x - dev_x - 2, firstItemRect.GetY() - 1 - dev_y,
                            x - dev_x - 2, lastItemRect.GetBottom() + 1 - dev_y);
            }
        }
    }
    else // !report
    {
        size_t count = GetItemCount();
        for ( size_t i = 0; i < count; i++ )
        {
            GetLine(i)->Draw( &dc );
        }
    }

    if ( HasCurrent() )
    {
        // don't draw rect outline under Max if we already have the background
        // color but under other platforms only draw it if we do: it is a bit
        // silly to draw "focus rect" if we don't have focus!
#if 0
#ifdef __WXMAC__
        if ( !m_hasFocus )
#else // !__WXMAC__
        if ( m_hasFocus )
#endif // __WXMAC__/!__WXMAC__
        {
            dc.SetPen( *wxBLACK_PEN );
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            dc.DrawRectangle( GetLineHighlightRect(m_current) );
        }
#endif
    }

    dc.EndDrawing();
}

void wxODListMainWindow::HighlightAll( bool on )
{
    if ( IsSingleSel() )
    {
        wxASSERT_MSG( !on, _T("can't do this in a single sel control") );

        // we just have one item to turn off
        if ( HasCurrent() && IsHighlighted(m_current) )
        {
            HighlightLine(m_current, FALSE);
            RefreshLine(m_current);
        }
    }
    else // multi sel
    {
        HighlightLines(0, GetItemCount() - 1, on);
    }
}

void wxODListMainWindow::SendNotify( size_t line,
                                   wxEventType command,
                                   wxPoint point )
{
    wxListEvent le( command, GetParent()->GetId() );
    le.SetEventObject( GetParent() );
    le.m_itemIndex = line;

    // set only for events which have position
    if ( point != wxDefaultPosition )
        le.m_pointDrag = point;

    // don't try to get the line info for virtual list controls: the main
    // program has it anyhow and if we did it would result in accessing all
    // the lines, even those which are not visible now and this is precisely
    // what we're trying to avoid
    if ( !IsVirtual() && (command != wxEVT_COMMAND_LIST_DELETE_ITEM) )
    {
        if ( line != (size_t)-1 )
        {
            GetLine(line)->GetItem( 0, le.m_item );
        }
        //else: this happens for wxEVT_COMMAND_LIST_ITEM_FOCUSED event
    }
    //else: there may be no more such item

    GetParent()->GetEventHandler()->ProcessEvent( le );
}

void wxODListMainWindow::ChangeCurrent(size_t current)
{
    m_current = current;

    SendNotify(current, wxEVT_COMMAND_LIST_ITEM_FOCUSED);
}

void wxODListMainWindow::EditLabel( long item )
{
    wxCHECK_RET( (item >= 0) && ((size_t)item < GetItemCount()),
                 wxT("wrong index in wxODGenericListCtrl::EditLabel()") );

    size_t itemEdit = (size_t)item;

    wxListEvent le( wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT, GetParent()->GetId() );
    le.SetEventObject( GetParent() );
    le.m_itemIndex = item;
    wxODListLineData *data = GetLine(itemEdit);
    wxCHECK_RET( data, _T("invalid index in EditLabel()") );
    data->GetItem( 0, le.m_item );
    if ( GetParent()->GetEventHandler()->ProcessEvent( le ) && !le.IsAllowed() )
    {
        // vetoed by user code
        return;
    }

    // We have to call this here because the label in question might just have
    // been added and no screen update taken place.
    if ( m_dirty )
        wxSafeYield();

    wxODListTextCtrl *text = new wxODListTextCtrl(this, itemEdit);

    text->SetFocus();
}

void wxODListMainWindow::OnRenameTimer()
{
    wxCHECK_RET( HasCurrent(), wxT("unexpected rename timer") );

    EditLabel( m_current );
}

bool wxODListMainWindow::OnRenameAccept(size_t itemEdit, const wxString& value)
{
    wxListEvent le( wxEVT_COMMAND_LIST_END_LABEL_EDIT, GetParent()->GetId() );
    
    // These only exist for wxTreeCtrl, which should probably be changed
    // le.m_editCancelled = FALSE;
    // le.m_label = value;
    
    le.SetEventObject( GetParent() );
    le.m_itemIndex = itemEdit;

    wxODListLineData *data = GetLine(itemEdit);
    wxCHECK_MSG( data, FALSE, _T("invalid index in OnRenameAccept()") );

    data->GetItem( 0, le.m_item );
    le.m_item.m_text = value;
    
    return !GetParent()->GetEventHandler()->ProcessEvent( le ) ||
                le.IsAllowed();
}

void wxODListMainWindow::OnRenameCancelled(size_t itemEdit)
{
    // wxMSW seems not to notify the program about
    // cancelled label edits.
    return;

    // let owner know that the edit was cancelled
    wxListEvent le( wxEVT_COMMAND_LIST_END_LABEL_EDIT, GetParent()->GetId() );
    
    // These only exist for wxTreeCtrl, which should probably be changed
    // le.m_editCancelled = TRUE;
    // le.m_label = wxEmptyString;
    
    le.SetEventObject( GetParent() );
    le.m_itemIndex = itemEdit;

    wxODListLineData *data = GetLine(itemEdit);
    wxCHECK_RET( data, _T("invalid index in OnRenameCancelled()") );

    data->GetItem( 0, le.m_item );

    GetEventHandler()->ProcessEvent( le );
}

void wxODListMainWindow::OnMouse( wxMouseEvent &event )
{
    event.SetEventObject( GetParent() );
    if ( GetParent()->GetEventHandler()->ProcessEvent( event) )
        return;

    if ( !HasCurrent() || IsEmpty() )
        return;

    if (m_dirty)
        return;

    if ( !(event.Dragging() || event.ButtonDown() || event.LeftUp() ||
        event.ButtonDClick()) )
        return;

    int x = event.GetX();
    int y = event.GetY();
    CalcUnscrolledPosition( x, y, &x, &y );

    // where did we hit it (if we did)?
    long hitResult = 0;

    size_t count = GetItemCount(),
           current;

    if ( HasFlag(wxLC_REPORT) )
    {
        current = y / GetLineHeight();
        if ( current < count )
            hitResult = HitTestLine(current, x, y);
    }
    else // !report
    {
        // TODO: optimize it too! this is less simple than for report view but
        //       enumerating all items is still not a way to do it!!
        for ( current = 0; current < count; current++ )
        {
            hitResult = HitTestLine(current, x, y);
            if ( hitResult )
                break;
        }
    }

    if (event.Dragging())
    {
        if (m_dragCount == 0)
        {
            // we have to report the raw, physical coords as we want to be
            // able to call HitTest(event.m_pointDrag) from the user code to
            // get the item being dragged
            m_dragStart = event.GetPosition();
        }

        m_dragCount++;

        if (m_dragCount != 3)
            return;

        int command = event.RightIsDown() ? wxEVT_COMMAND_LIST_BEGIN_RDRAG
                                          : wxEVT_COMMAND_LIST_BEGIN_DRAG;

        wxListEvent le( command, GetParent()->GetId() );
        le.SetEventObject( GetParent() );
        le.m_itemIndex = current;
        le.m_pointDrag = m_dragStart;
        GetParent()->GetEventHandler()->ProcessEvent( le );

        return;
    }
    else
    {
        m_dragCount = 0;
    }

    if ( !hitResult )
    {
        // outside of any item
        return;
    }

    bool forceClick = FALSE;
    if (event.ButtonDClick())
    {
        m_renameTimer->Stop();
        m_lastOnSame = FALSE;

        if ( current == m_lineLastClicked )
        {
            SendNotify( current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );

            return;
        }
        else
        {
            // the first click was on another item, so don't interpret this as
            // a double click, but as a simple click instead
            forceClick = TRUE;
        }
    }

    if (event.LeftUp() && m_lastOnSame)
    {
        if ((current == m_current) &&
            (hitResult == wxLIST_HITTEST_ONITEMLABEL) &&
            HasFlag(wxLC_EDIT_LABELS)  )
        {
            m_renameTimer->Start( 100, TRUE );
        }
        m_lastOnSame = FALSE;
    }
    else if (event.RightDown())
    {
        SendNotify( current, wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK,
                    event.GetPosition() );
    }
    else if (event.MiddleDown())
    {
        SendNotify( current, wxEVT_COMMAND_LIST_ITEM_MIDDLE_CLICK );
    }
    else if ( event.LeftDown() || forceClick )
    {
        m_lineBeforeLastClicked = m_lineLastClicked;
        m_lineLastClicked = current;

        size_t oldCurrent = m_current;

        if ( IsSingleSel() || !(event.ControlDown() || event.ShiftDown()) )
        {
            HighlightAll( FALSE );

            ChangeCurrent(current);

            ReverseHighlight(m_current);
        }
        else // multi sel & either ctrl or shift is down
        {
            if (event.ControlDown())
            {
                ChangeCurrent(current);

                ReverseHighlight(m_current);
            }
            else if (event.ShiftDown())
            {
                ChangeCurrent(current);

                size_t lineFrom = oldCurrent,
                       lineTo = current;

                if ( lineTo < lineFrom )
                {
                    lineTo = lineFrom;
                    lineFrom = m_current;
                }

                HighlightLines(lineFrom, lineTo);
            }
            else // !ctrl, !shift
            {
                // test in the enclosing if should make it impossible
                wxFAIL_MSG( _T("how did we get here?") );
            }
        }

        if (m_current != oldCurrent)
        {
            RefreshLine( oldCurrent );
        }

        // forceClick is only set if the previous click was on another item
        m_lastOnSame = !forceClick && (m_current == oldCurrent);
    }
}

void wxODListMainWindow::MoveToItem(size_t item)
{
    if ( item == (size_t)-1 )
        return;

    wxRect rect = GetLineRect(item);

    int client_w, client_h;
    GetClientSize( &client_w, &client_h );

    int view_x = m_xScroll*GetScrollPos( wxHORIZONTAL );
    int view_y = m_yScroll*GetScrollPos( wxVERTICAL );

    if ( HasFlag(wxLC_REPORT) )
    {
        // the next we need the range of lines shown it might be different, so
        // recalculate it
        ResetVisibleLinesRange();

        if (rect.y < view_y )
            Scroll( -1, rect.y/m_yScroll );
        if (rect.y+rect.height+5 > view_y+client_h)
            Scroll( -1, (rect.y+rect.height-client_h+SCROLL_UNIT_Y)/m_yScroll );
    }
    else // !report
    {
        if (rect.x-view_x < 5)
            Scroll( (rect.x-5)/m_xScroll, -1 );
        if (rect.x+rect.width-5 > view_x+client_w)
            Scroll( (rect.x+rect.width-client_w+SCROLL_UNIT_X)/m_xScroll, -1 );
    }
}

// ----------------------------------------------------------------------------
// keyboard handling
// ----------------------------------------------------------------------------

void wxODListMainWindow::OnArrowChar(size_t newCurrent, const wxKeyEvent& event)
{
    wxCHECK_RET( newCurrent < (size_t)GetItemCount(),
                 _T("invalid item index in OnArrowChar()") );

    size_t oldCurrent = m_current;

    // in single selection we just ignore Shift as we can't select several
    // items anyhow
    if ( event.ShiftDown() && !IsSingleSel() )
    {
        ChangeCurrent(newCurrent);

        // select all the items between the old and the new one
        if ( oldCurrent > newCurrent )
        {
            newCurrent = oldCurrent;
            oldCurrent = m_current;
        }

        HighlightLines(oldCurrent, newCurrent);
    }
    else // !shift
    {
        // all previously selected items are unselected unless ctrl is held
        if ( !event.ControlDown() )
            HighlightAll(FALSE);

        ChangeCurrent(newCurrent);

        // refresh the old focus to remove it
        RefreshLine( oldCurrent );

        if ( !event.ControlDown() )
        {
            HighlightLine( m_current, TRUE );
        }
    }

    RefreshLine( m_current );

    MoveToFocus();
}

void wxODListMainWindow::OnKeyDown( wxKeyEvent &event )
{
    wxWindow *parent = GetParent();

    /* we propagate the key event up */
    wxKeyEvent ke( wxEVT_KEY_DOWN );
    ke.m_shiftDown = event.m_shiftDown;
    ke.m_controlDown = event.m_controlDown;
    ke.m_altDown = event.m_altDown;
    ke.m_metaDown = event.m_metaDown;
    ke.m_keyCode = event.m_keyCode;
    ke.m_x = event.m_x;
    ke.m_y = event.m_y;
    ke.SetEventObject( parent );
    if (parent->GetEventHandler()->ProcessEvent( ke )) return;

    event.Skip();
}

void wxODListMainWindow::OnChar( wxKeyEvent &event )
{
    wxWindow *parent = GetParent();

    /* we send a list_key event up */
    if ( HasCurrent() )
    {
        wxListEvent le( wxEVT_COMMAND_LIST_KEY_DOWN, GetParent()->GetId() );
        le.m_itemIndex = m_current;
        GetLine(m_current)->GetItem( 0, le.m_item );
        le.m_code = (int)event.GetKeyCode();
        le.SetEventObject( parent );
        parent->GetEventHandler()->ProcessEvent( le );
    }

    /* we propagate the char event up */
    wxKeyEvent ke( wxEVT_CHAR );
    ke.m_shiftDown = event.m_shiftDown;
    ke.m_controlDown = event.m_controlDown;
    ke.m_altDown = event.m_altDown;
    ke.m_metaDown = event.m_metaDown;
    ke.m_keyCode = event.m_keyCode;
    ke.m_x = event.m_x;
    ke.m_y = event.m_y;
    ke.SetEventObject( parent );
    if (parent->GetEventHandler()->ProcessEvent( ke )) return;

    if (event.GetKeyCode() == WXK_TAB)
    {
        wxNavigationKeyEvent nevent;
        nevent.SetWindowChange( event.ControlDown() );
        nevent.SetDirection( !event.ShiftDown() );
        nevent.SetEventObject( GetParent()->GetParent() );
        nevent.SetCurrentFocus( m_parent );
        if (GetParent()->GetParent()->GetEventHandler()->ProcessEvent( nevent ))
            return;
    }

    /* no item -> nothing to do */
    if (!HasCurrent())
    {
        event.Skip();
        return;
    }

    switch (event.GetKeyCode())
    {
        case WXK_UP:
            if ( m_current > 0 )
                OnArrowChar( m_current - 1, event );
            break;

        case WXK_DOWN:
            if ( m_current < (size_t)GetItemCount() - 1 )
                OnArrowChar( m_current + 1, event );
            break;

        case WXK_END:
            if (!IsEmpty())
                OnArrowChar( GetItemCount() - 1, event );
            break;

        case WXK_HOME:
            if (!IsEmpty())
                OnArrowChar( 0, event );
            break;

        case WXK_PRIOR:
            {
                int steps = 0;
                if ( HasFlag(wxLC_REPORT) )
                {
                    steps = m_linesPerPage - 1;
                }
                else
                {
                    steps = m_current % m_linesPerPage;
                }

                int index = m_current - steps;
                if (index < 0)
                    index = 0;

                OnArrowChar( index, event );
            }
            break;

        case WXK_NEXT:
            {
                int steps = 0;
                if ( HasFlag(wxLC_REPORT) )
                {
                    steps = m_linesPerPage - 1;
                }
                else
                {
                    steps = m_linesPerPage - (m_current % m_linesPerPage) - 1;
                }

                size_t index = m_current + steps;
                size_t count = GetItemCount();
                if ( index >= count )
                    index = count - 1;

                OnArrowChar( index, event );
            }
            break;

        case WXK_LEFT:
            if ( !HasFlag(wxLC_REPORT) )
            {
                int index = m_current - m_linesPerPage;
                if (index < 0)
                    index = 0;

                OnArrowChar( index, event );
            }
            break;

        case WXK_RIGHT:
            if ( !HasFlag(wxLC_REPORT) )
            {
                size_t index = m_current + m_linesPerPage;

                size_t count = GetItemCount();
                if ( index >= count )
                    index = count - 1;

                OnArrowChar( index, event );
            }
            break;

        case WXK_SPACE:
            if ( IsSingleSel() )
            {
                SendNotify( m_current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );

                if ( IsHighlighted(m_current) )
                {
                    // don't unselect the item in single selection mode
                    break;
                }
                //else: select it in ReverseHighlight() below if unselected
            }

            ReverseHighlight(m_current);
            break;

        case WXK_RETURN:
        case WXK_EXECUTE:
            SendNotify( m_current, wxEVT_COMMAND_LIST_ITEM_ACTIVATED );
            break;

        default:
            event.Skip();
    }
}

// ----------------------------------------------------------------------------
// focus handling
// ----------------------------------------------------------------------------

void wxODListMainWindow::SetFocus()
{
    // VS: wxODListMainWindow derives from wxPanel (via wxScrolledWindow) and wxPanel
    //     overrides SetFocus in such way that it does never change focus from
    //     panel's child to the panel itself. Unfortunately, we must be able to change
    //     focus to the panel from wxODListTextCtrl because the text control should
    //     disappear when the user clicks outside it.

    wxWindow *oldFocus = FindFocus();

    if ( oldFocus && oldFocus->GetParent() == this )
    {
        wxWindow::SetFocus();
    }
    else
    {
        wxScrolledWindow::SetFocus();
    }
}

void wxODListMainWindow::OnSetFocus( wxFocusEvent &WXUNUSED(event) )
{
    // wxGTK sends us EVT_SET_FOCUS events even if we had never got
    // EVT_KILL_FOCUS before which means that we finish by redrawing the items
    // which are already drawn correctly resulting in horrible flicker - avoid
    // it
    if ( !m_hasFocus )
    {
        m_hasFocus = TRUE;

        RefreshSelected();
    }

    if ( !GetParent() )
        return;

    wxFocusEvent event( wxEVT_SET_FOCUS, GetParent()->GetId() );
    event.SetEventObject( GetParent() );
    GetParent()->GetEventHandler()->ProcessEvent( event );
}

void wxODListMainWindow::OnKillFocus( wxFocusEvent &WXUNUSED(event) )
{
    m_hasFocus = FALSE;
    RefreshSelected();
}

void wxODListMainWindow::DrawImage( int index, wxDC *dc, int x, int y )
{
    if ( HasFlag(wxLC_ICON) && (m_normal_image_list))
    {
        m_normal_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( HasFlag(wxLC_SMALL_ICON) && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( HasFlag(wxLC_LIST) && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
    else if ( HasFlag(wxLC_REPORT) && (m_small_image_list))
    {
        m_small_image_list->Draw( index, *dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
    }
}

void wxODListMainWindow::GetImageSize( int index, int &width, int &height ) const
{
    if ( HasFlag(wxLC_ICON) && m_normal_image_list )
    {
        m_normal_image_list->GetSize( index, width, height );
    }
    else if ( HasFlag(wxLC_SMALL_ICON) && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else if ( HasFlag(wxLC_LIST) && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else if ( HasFlag(wxLC_REPORT) && m_small_image_list )
    {
        m_small_image_list->GetSize( index, width, height );
    }
    else
    {
        width =
        height = 0;
    }
}

int wxODListMainWindow::GetTextLength( const wxString &s ) const
{
    wxClientDC dc( wxConstCast(this, wxODListMainWindow) );
    dc.SetFont( GetFont() );

    wxCoord lw;
    dc.GetTextExtent( s, &lw, NULL );

    return lw + AUTOSIZE_COL_MARGIN;
}

void wxODListMainWindow::SetImageList( wxImageListType *imageList, int which )
{
    m_dirty = TRUE;

    // calc the spacing from the icon size
    int width = 0,
        height = 0;
    if ((imageList) && (imageList->GetImageCount()) )
    {
        imageList->GetSize(0, width, height);
    }

    if (which == wxIMAGE_LIST_NORMAL)
    {
        m_normal_image_list = imageList;
        m_normal_spacing = width + 8;
    }

    if (which == wxIMAGE_LIST_SMALL)
    {
        m_small_image_list = imageList;
        m_small_spacing = width + 14;
        m_lineHeight = 0;  // ensure that the line height will be recalc'd
    }
}

void wxODListMainWindow::SetItemSpacing( int spacing, bool isSmall )
{
    m_dirty = TRUE;
    if (isSmall)
    {
        m_small_spacing = spacing;
    }
    else
    {
        m_normal_spacing = spacing;
    }
}

int wxODListMainWindow::GetItemSpacing( bool isSmall )
{
    return isSmall ? m_small_spacing : m_normal_spacing;
}

// ----------------------------------------------------------------------------
// columns
// ----------------------------------------------------------------------------

int wxODListMainWindow::GetSortArrow(int col)
{
  wxODListHeaderDataList::Node* node=m_columns.Item(col);
  if(node==NULL) {
    return 0;
  }
  //wxCHECK_RET( node, _T("invalid column index in SetColumn") );

  wxODListHeaderData *column = node->GetData();
  return column->GetArrow();
}

void wxODListMainWindow::SetSortArrow(int col, int mode)
{
  for(int i = 0; i < (int)m_columns.GetCount(); ++i) {
    wxODListHeaderDataList::Node* node=m_columns.Item(i);
    
    wxCHECK_RET( node, _T("invalid column index in SetColumn") );
    
    wxODListHeaderData *column = node->GetData();
    if(i==col) {
      column->SetArrow(mode);
    } else {
      column->SetArrow(0); // reset the old
    }
  }

  wxODListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
  if ( headerWin )
    headerWin->m_dirty = TRUE;
  
  m_dirty = TRUE;  
  
  Refresh(FALSE);
}

void wxODListMainWindow::SetColumn( int col, wxListItem &item )
{
    wxODListHeaderDataList::Node *node = m_columns.Item( col );

    wxCHECK_RET( node, _T("invalid column index in SetColumn") );

    if ( item.m_width == wxLIST_AUTOSIZE_USEHEADER )
        item.m_width = GetTextLength( item.m_text );

    wxODListHeaderData *column = node->GetData();
    column->SetItem( item );

    wxODListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin )
        headerWin->m_dirty = TRUE;

    m_dirty = TRUE;

    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
}

void wxODListMainWindow::SetColumnWidth( int col, int width )
{
    wxCHECK_RET( col >= 0 && col < GetColumnCount(),
                 _T("invalid column index") );

    wxCHECK_RET( HasFlag(wxLC_REPORT),
                 _T("SetColumnWidth() can only be called in report mode.") );

    m_dirty = TRUE;
    wxODListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin )
        headerWin->m_dirty = TRUE;

    wxODListHeaderDataList::Node *node = m_columns.Item( col );
    wxCHECK_RET( node, _T("no column?") );

    wxODListHeaderData *column = node->GetData();

    size_t count = GetItemCount();

    if (width == wxLIST_AUTOSIZE_USEHEADER)
    {
        width = GetTextLength(column->GetText());
    }
    else if ( width == wxLIST_AUTOSIZE )
    {
        if ( IsVirtual() )
        {
            // TODO: determine the max width somehow...
            width = WIDTH_COL_DEFAULT;
        }
        else // !virtual
        {
            wxClientDC dc(this);
            dc.SetFont( GetFont() );

            int max = AUTOSIZE_COL_MARGIN;

            for ( size_t i = 0; i < count; i++ )
            {
                wxODListLineData *line = GetLine(i);
                wxODListItemDataList::Node *n = line->m_items.Item( col );

                wxCHECK_RET( n, _T("no subitem?") );

                wxODListItemData *item = n->GetData();
                int current = 0;

                if (item->HasImage())
                {
                    int ix, iy;
                    GetImageSize( item->GetImage(), ix, iy );
                    current += ix + 5;
                }

                if (item->HasText())
                {
                    wxCoord w;
                    dc.GetTextExtent( item->GetText(), &w, NULL );
                    current += w;
                }

                if (current > max)
                    max = current;
            }

            width = max + AUTOSIZE_COL_MARGIN;
        }
    }

    m_dirty = true;
    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
	column->SetWidth( width );
}

int wxODListMainWindow::GetHeaderWidth() const
{
    if ( !m_headerWidth )
    {
        wxODListMainWindow *self = wxConstCast(this, wxODListMainWindow);

        size_t count = GetColumnCount();
        for ( size_t col = 0; col < count; col++ )
        {
            self->m_headerWidth += GetColumnWidth(col);
        }
    }

    return m_headerWidth;
}

void wxODListMainWindow::GetColumn( int col, wxListItem &item ) const
{
    wxODListHeaderDataList::Node *node = m_columns.Item( col );
    wxCHECK_RET( node, _T("invalid column index in GetColumn") );

    wxODListHeaderData *column = node->GetData();
    column->GetItem( item );
}

int wxODListMainWindow::GetColumnWidth( int col ) const
{
    wxODListHeaderDataList::Node *node = m_columns.Item( col );
    wxCHECK_MSG( node, 0, _T("invalid column index") );

    wxODListHeaderData *column = node->GetData();
    return column->GetWidth();
}

// ----------------------------------------------------------------------------
// item state
// ----------------------------------------------------------------------------

// we can safely cache data in a hash map

void wxODListMainWindow::SetItem( wxListItem &item )
{
    long id = item.m_itemId;
    wxCHECK_RET( id >= 0 && (size_t)id < GetItemCount(),
                 _T("invalid item index in SetItem") );

    if ( !IsVirtual() )
    {
        wxODListLineData *line = GetLine((size_t)id);
        line->SetItem( item.m_col, item );
    }

    if ( InReportView() )
    {
        // just refresh the line to show the new value of the text/image
        RefreshLine((size_t)id);
    }
    else // !report
    {
        printf("refresh everything (resulting in horrible flicker - FIXME!)|n");
        m_dirty = TRUE;
    }
}

void wxODListMainWindow::SetItemState( long litem, long state, long stateMask )
{
     wxCHECK_RET( litem >= 0 && (size_t)litem < GetItemCount(),
                  _T("invalid list ctrl item index in SetItem") );

    size_t oldCurrent = m_current;
    size_t item = (size_t)litem;    // safe because of the check above

    // do we need to change the focus?
    if ( stateMask & wxLIST_STATE_FOCUSED )
    {
        if ( state & wxLIST_STATE_FOCUSED )
        {
            // don't do anything if this item is already focused
            if ( item != m_current )
            {
                ChangeCurrent(item);

                if ( oldCurrent != (size_t)-1 )
                {
                    if ( IsSingleSel() )
                    {
                        HighlightLine(oldCurrent, FALSE);
                    }

                    RefreshLine(oldCurrent);
                }

                RefreshLine( m_current );
            }
        }
        else // unfocus
        {
            // don't do anything if this item is not focused
            if ( item == m_current )
            {
                ResetCurrent();

                if ( IsSingleSel() )
                {
                    // we must unselect the old current item as well or we
                    // might end up with more than one selected item in a
                    // single selection control
                    HighlightLine(oldCurrent, FALSE);
                }

                RefreshLine( oldCurrent );
            }
        }
    }

    // do we need to change the selection state?
    if ( stateMask & wxLIST_STATE_SELECTED )
    {
        bool on = (state & wxLIST_STATE_SELECTED) != 0;

        if ( IsSingleSel() )
        {
            if ( on )
            {
                // selecting the item also makes it the focused one in the
                // single sel mode
                if ( m_current != item )
                {
                    ChangeCurrent(item);

                    if ( oldCurrent != (size_t)-1 )
                    {
                        HighlightLine( oldCurrent, FALSE );
                        RefreshLine( oldCurrent );
                    }
                }
            }
            else // off
            {
                // only the current item may be selected anyhow
                if ( item != m_current )
                    return;
            }
        }

        if ( HighlightLine(item, on) )
        {
            RefreshLine(item);
        }
    }
}

int wxODListMainWindow::GetItemState( long item, long stateMask ) const
{
    wxCHECK_MSG( item >= 0 && (size_t)item < GetItemCount(), 0,
                 _T("invalid list ctrl item index in GetItemState()") );

    int ret = wxLIST_STATE_DONTCARE;

    if ( stateMask & wxLIST_STATE_FOCUSED )
    {
        if ( (size_t)item == m_current )
            ret |= wxLIST_STATE_FOCUSED;
    }

    if ( stateMask & wxLIST_STATE_SELECTED )
    {
        if ( IsHighlighted(item) )
            ret |= wxLIST_STATE_SELECTED;
    }

    return ret;
}

void wxODListMainWindow::GetItem( wxListItem &item ) const
{
    wxCHECK_RET( item.m_itemId >= 0 && (size_t)item.m_itemId < GetItemCount(),
                 _T("invalid item index in GetItem") );

    wxODListLineData *line = GetLine((size_t)item.m_itemId);
    line->GetItem( item.m_col, item );
}

// ----------------------------------------------------------------------------
// item count
// ----------------------------------------------------------------------------

size_t wxODListMainWindow::GetItemCount() const
{
    return IsVirtual() ? m_countVirt : m_lines.GetCount();
}

void wxODListMainWindow::SetItemCount(long count)
{
    m_selStore.SetItemCount(count);
    m_countVirt = count;

    ResetVisibleLinesRange();

    // scrollbars must be reset
    m_dirty = TRUE;
}

int wxODListMainWindow::GetSelectedItemCount() const
{
    // deal with the quick case first
    if ( IsSingleSel() )
    {
        return HasCurrent() ? IsHighlighted(m_current) : FALSE;
    }

    // virtual controls remmebers all its selections itself
    if ( IsVirtual() )
        return m_selStore.GetSelectedCount();

    // TODO: we probably should maintain the number of items selected even for
    //       non virtual controls as enumerating all lines is really slow...
    size_t countSel = 0;
    size_t count = GetItemCount();
    for ( size_t line = 0; line < count; line++ )
    {
        if ( GetLine(line)->IsHighlighted() )
            countSel++;
    }

    return countSel;
}

// ----------------------------------------------------------------------------
// item position/size
// ----------------------------------------------------------------------------

void wxODListMainWindow::GetItemRect( long index, wxRect &rect ) const
{
    wxCHECK_RET( index >= 0 && (size_t)index < GetItemCount(),
                 _T("invalid index in GetItemRect") );

    rect = GetLineRect((size_t)index);

    CalcScrolledPosition(rect.x, rect.y, &rect.x, &rect.y);
}

bool wxODListMainWindow::GetItemPosition(long item, wxPoint& pos) const
{
    wxRect rect;
    GetItemRect(item, rect);

    pos.x = rect.x;
    pos.y = rect.y;

    return TRUE;
}

// ----------------------------------------------------------------------------
// geometry calculation
// ----------------------------------------------------------------------------

void wxODListMainWindow::RecalculatePositions(bool noRefresh)
{
    wxClientDC dc( this );
    dc.SetFont( GetFont() );

    int iconSpacing;
    if ( HasFlag(wxLC_ICON) )
        iconSpacing = m_normal_spacing;
    else if ( HasFlag(wxLC_SMALL_ICON) )
        iconSpacing = m_small_spacing;
    else
        iconSpacing = 0;

    // Note that we do not call GetClientSize() here but
    // GetSize() and substract the border size for sunken
    // borders manually. This is technically incorrect,
    // but we need to know the client area's size WITHOUT
    // scrollbars here. Since we don't know if there are
    // any scrollbars, we use GetSize() instead. Another
    // solution would be to call SetScrollbars() here to
    // remove the scrollbars and call GetClientSize() then,
    // but this might result in flicker and - worse - will
    // reset the scrollbars to 0 which is not good at all
    // if you resize a dialog/window, but don't want to
    // reset the window scrolling. RR.
    // Furthermore, we actually do NOT subtract the border
    // width as 2 pixels is just the extra space which we
    // need around the actual content in the window. Other-
    // wise the text would e.g. touch the upper border. RR.
    int clientWidth,
        clientHeight;
    GetSize( &clientWidth, &clientHeight );

    if ( HasFlag(wxLC_REPORT) )
    {
        // all lines have the same height
        int lineHeight = GetLineHeight();

        // scroll one line per step
        m_yScroll = lineHeight;

        size_t lineCount = GetItemCount();
        int entireHeight = lineCount*lineHeight + LINE_SPACING;

        m_linesPerPage = clientHeight / lineHeight;

        ResetVisibleLinesRange();

        SetScrollbars( m_xScroll, m_yScroll,
                       GetHeaderWidth() / m_xScroll,
                       (entireHeight + m_yScroll - 1)/m_yScroll,
                       GetScrollPos(wxHORIZONTAL),
                       GetScrollPos(wxVERTICAL),
                       TRUE );
    }
    else // !report
    {
        // at first we try without any scrollbar. if the items don't
        // fit into the window, we recalculate after subtracting an
        // approximated 15 pt for the horizontal scrollbar

        int entireWidth = 0;

        for (int tries = 0; tries < 2; tries++)
        {
            // We start with 4 for the border around all items
            entireWidth = 4;

            if (tries == 1)
            {
                // Now we have decided that the items do not fit into the
                // client area. Unfortunately, wxWindows sometimes thinks
                // that it does fit and therefore NO horizontal scrollbar
                // is inserted. This looks ugly, so we fudge here and make
                // the calculated width bigger than was actually has been
                // calculated. This ensures that wxScrolledWindows puts
                // a scrollbar at the bottom of its client area.
                entireWidth += SCROLL_UNIT_X;
            }

            // Start at 2,2 so the text does not touch the border
            int x = 2;
            int y = 2;
            int maxWidth = 0;
            m_linesPerPage = 0;
            int currentlyVisibleLines = 0;

            size_t count = GetItemCount();
            for (size_t i = 0; i < count; i++)
            {
                currentlyVisibleLines++;
                wxODListLineData *line = GetLine(i);
                line->CalculateSize( &dc, iconSpacing );
                line->SetPosition( x, y, clientWidth, iconSpacing );  // Why clientWidth? (FIXME)

                wxSize sizeLine = GetLineSize(i);

                if ( maxWidth < sizeLine.x )
                    maxWidth = sizeLine.x;

                y += sizeLine.y;
                if (currentlyVisibleLines > m_linesPerPage)
                    m_linesPerPage = currentlyVisibleLines;

                // Assume that the size of the next one is the same... (FIXME)
                if ( y + sizeLine.y >= clientHeight )
                {
                    currentlyVisibleLines = 0;
                    y = 2;
                    x += maxWidth+6;
                    entireWidth += maxWidth+6;
                    maxWidth = 0;
                }

                // We have reached the last item.
                if ( i == count - 1 )
                    entireWidth += maxWidth;

                if ( (tries == 0) && (entireWidth+SCROLL_UNIT_X > clientWidth) )
                {
                    clientHeight -= 15; // We guess the scrollbar height. (FIXME)
                    m_linesPerPage = 0;
                    currentlyVisibleLines = 0;
                    break;
                }

                if ( i == count - 1 )
                    tries = 1;  // Everything fits, no second try required.
            }
        }

        int scroll_pos = GetScrollPos( wxHORIZONTAL );
        SetScrollbars( m_xScroll, m_yScroll, (entireWidth+SCROLL_UNIT_X) / m_xScroll, 0, scroll_pos, 0, TRUE );
    }

    if ( !noRefresh )
    {
        // FIXME: why should we call it from here?
        UpdateCurrent();

        RefreshAll();
    }
}

void wxODListMainWindow::RefreshAll()
{
    m_dirty = FALSE;
    Refresh();

    wxODListHeaderWindow *headerWin = GetListCtrl()->m_headerWin;
    if ( headerWin && headerWin->m_dirty )
    {
        headerWin->m_dirty = FALSE;
        headerWin->Refresh();
    }
}

void wxODListMainWindow::UpdateCurrent()
{
    if ( !HasCurrent() && !IsEmpty() )
    {
        ChangeCurrent(0);
    }
}

long wxODListMainWindow::GetNextItem( long item,
                                    int WXUNUSED(geometry),
                                    int state ) const
{
    long ret = item,
         max = GetItemCount();
    wxCHECK_MSG( (ret == -1) || (ret < max), -1,
                 _T("invalid listctrl index in GetNextItem()") );

    // notice that we start with the next item (or the first one if item == -1)
    // and this is intentional to allow writing a simple loop to iterate over
    // all selected items
    ret++;
    if ( ret == max )
    {
        // this is not an error because the index was ok initially, just no
        // such item
        return -1;
    }

    if ( !state )
    {
        // any will do
        return (size_t)ret;
    }

    size_t count = GetItemCount();
    for ( size_t line = (size_t)ret; line < count; line++ )
    {
        if ( (state & wxLIST_STATE_FOCUSED) && (line == m_current) )
            return line;

        if ( (state & wxLIST_STATE_SELECTED) && IsHighlighted(line) )
            return line;
    }

    return -1;
}

// ----------------------------------------------------------------------------
// deleting stuff
// ----------------------------------------------------------------------------

void wxODListMainWindow::DeleteItem( long lindex )
{
    size_t count = GetItemCount();

    wxCHECK_RET( (lindex >= 0) && ((size_t)lindex < count),
                 _T("invalid item index in DeleteItem") );

    // take it off from hash
    wxODListLineData *line = GetLine(lindex);
    wxListItem item;
    line->GetItem( 0, item );

    size_t index = (size_t)lindex;

    // we don't need to adjust the index for the previous items
    if ( HasCurrent() && m_current >= index )
    {
        // if the current item is being deleted, we want the next one to
        // become selected - unless there is no next one - so don't adjust
        // m_current in this case
        if ( m_current != index || m_current == count - 1 )
        {
            m_current--;
        }
    }

    if ( InReportView() )
    {
        ResetVisibleLinesRange();
    }

    if ( IsVirtual() )
    {
        m_countVirt--;

        m_selStore.OnItemDelete(index);
    }
    else
    {
        m_lines.RemoveAt( index );
    }

    // we need to refresh the (vert) scrollbar as the number of items changed
    m_dirty = TRUE;

    SendNotify( index, wxEVT_COMMAND_LIST_DELETE_ITEM );

    RefreshAfter(index);
}

void wxODListMainWindow::DeleteColumn( int col )
{
    wxODListHeaderDataList::Node *node = m_columns.Item( col );

    wxCHECK_RET( node, wxT("invalid column index in DeleteColumn()") );

    m_dirty = TRUE;
    m_columns.DeleteNode( node );

    // invalidate it as it has to be recalculated
    m_headerWidth = 0;
}

void wxODListMainWindow::DoDeleteAllItems()
{
    if ( IsEmpty() )
    {
        // nothing to do - in particular, don't send the event
        return;
    }

    ResetCurrent();

    // to make the deletion of all items faster, we don't send the
    // notifications for each item deletion in this case but only one event
    // for all of them: this is compatible with wxMSW and documented in
    // DeleteAllItems() description

    wxListEvent event( wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS, GetParent()->GetId() );
    event.SetEventObject( GetParent() );
    GetParent()->GetEventHandler()->ProcessEvent( event );

    if ( IsVirtual() )
    {
        m_countVirt = 0;

        m_selStore.Clear();
    }

    if ( InReportView() )
    {
        ResetVisibleLinesRange();
    }

    m_lines.Clear();
}

void wxODListMainWindow::DeleteAllItems()
{
    DoDeleteAllItems();

    RecalculatePositions();
}

void wxODListMainWindow::DeleteEverything()
{
    m_columns.Clear();

    DeleteAllItems();
}

// ----------------------------------------------------------------------------
// scanning for an item
// ----------------------------------------------------------------------------

void wxODListMainWindow::EnsureVisible( long index )
{
    wxCHECK_RET( index >= 0 && (size_t)index < GetItemCount(),
                 _T("invalid index in EnsureVisible") );

    // We have to call this here because the label in question might just have
    // been added and its position is not known yet
    if ( m_dirty )
    {
        RecalculatePositions(TRUE /* no refresh */);
    }

    MoveToItem((size_t)index);
}

long wxODListMainWindow::FindItem(long start, const wxString& str, bool WXUNUSED(partial) )
{
    long pos = start;
    wxString tmp = str;
    if (pos < 0)
        pos = 0;

    size_t count = GetItemCount();
    for ( size_t i = (size_t)pos; i < count; i++ )
    {
        wxODListLineData *line = GetLine(i);
        if ( line->GetText(0) == tmp )
            return i;
    }

    return wxNOT_FOUND;
}

long wxODListMainWindow::FindItem(long start, long data)
{
    long pos = start;
    if (pos < 0)
        pos = 0;

    size_t count = GetItemCount();
    for (size_t i = (size_t)pos; i < count; i++)
    {
        wxODListLineData *line = GetLine(i);
        wxListItem item;
        line->GetItem( 0, item );
        if (item.m_data == data)
            return i;
    }
    return wxNOT_FOUND;
}

long wxODListMainWindow::HitTest( int x, int y, int &flags )
{
    CalcUnscrolledPosition( x, y, &x, &y );

    size_t count = GetItemCount();

    if ( HasFlag(wxLC_REPORT) )
    {
        size_t current = y / GetLineHeight();
        if ( current < count )
        {
            flags = HitTestLine(current, x, y);
            if ( flags )
                return current;
        }
    }
    else // !report
    {
        // TODO: optimize it too! this is less simple than for report view but
        //       enumerating all items is still not a way to do it!!
        for ( size_t current = 0; current < count; current++ )
        {
            flags = HitTestLine(current, x, y);
            if ( flags )
                return current;
        }
    }

    return wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// adding stuff
// ----------------------------------------------------------------------------

void wxODListMainWindow::InsertItem( wxListItem &item )
{
    wxASSERT_MSG( !IsVirtual(), _T("can't be used with virtual control") );

    size_t count = GetItemCount();
    wxCHECK_RET( item.m_itemId >= 0 && (size_t)item.m_itemId <= count,
                 _T("invalid item index") );

    size_t id = item.m_itemId;

    m_dirty = TRUE;

    int mode = 0;
    if ( HasFlag(wxLC_REPORT) )
    {
        mode = wxLC_REPORT;
        ResetVisibleLinesRange();
    }
    else if ( HasFlag(wxLC_LIST) )
        mode = wxLC_LIST;
    else if ( HasFlag(wxLC_ICON) )
        mode = wxLC_ICON;
    else if ( HasFlag(wxLC_SMALL_ICON) )
        mode = wxLC_ICON;  // no typo
    else
    {
        wxFAIL_MSG( _T("unknown mode") );
    }

    wxODListLineData *line = new wxODListLineData(this);

    line->SetItem( 0, item );

    m_lines.Insert( line, id );

    m_dirty = TRUE;

    // If an item is selected at or below the point of insertion, we need to
    // increment the member variables because the current row's index has gone
    // up by one
    if ( HasCurrent() && m_current >= id )
    {
        m_current++;
    }

    SendNotify(id, wxEVT_COMMAND_LIST_INSERT_ITEM);

    RefreshLines(id, GetItemCount() - 1);
}

void wxODListMainWindow::InsertColumn( long col, wxListItem &item )
{
    m_dirty = TRUE;
    if ( HasFlag(wxLC_REPORT) )
    {
        if (item.m_width == wxLIST_AUTOSIZE_USEHEADER)
            item.m_width = GetTextLength( item.m_text );
        wxODListHeaderData *column = new wxODListHeaderData( item );
        if ((col >= 0) && (col < (int)m_columns.GetCount()))
        {
            wxODListHeaderDataList::Node *node = m_columns.Item( col );
            m_columns.Insert( node, column );
        }
        else
        {
            m_columns.Append( column );
        }

        // invalidate it as it has to be recalculated
        m_headerWidth = 0;
    }
}

// ----------------------------------------------------------------------------
// sorting
// ----------------------------------------------------------------------------

static wxListCtrlCompare amule_list_ctrl_compare_func_2;
static long amule_list_ctrl_compare_data;

static int LINKAGEMODE amule_list_ctrl_compare_func_1( wxODListLineData **arg1, wxODListLineData **arg2 )
{
    wxODListLineData *line1 = *arg1;
    wxODListLineData *line2 = *arg2;
    wxListItem item;
    line1->GetItem( 0, item );
    long data1 = item.m_data;
    line2->GetItem( 0, item );
    long data2 = item.m_data;
    return amule_list_ctrl_compare_func_2( data1, data2, amule_list_ctrl_compare_data );
}

void wxODListMainWindow::SortItems( wxListCtrlCompare fn, long data )
{
    amule_list_ctrl_compare_func_2 = fn;
    amule_list_ctrl_compare_data = data;
    m_lines.Sort( amule_list_ctrl_compare_func_1 );
    m_dirty = TRUE;
}

// ----------------------------------------------------------------------------
// scrolling
// ----------------------------------------------------------------------------

void wxODListMainWindow::OnScroll(wxScrollWinEvent& event)
{
    // update our idea of which lines are shown when we redraw the window the
    // next time
    ResetVisibleLinesRange();

    // FIXME
#if defined(__WXGTK__) && !defined(__WXUNIVERSAL__)
    wxScrolledWindow::OnScroll(event);
#else
    HandleOnScroll( event );
#endif

    if ( event.GetOrientation() == wxHORIZONTAL && HasHeader() )
    {
        wxODGenericListCtrl* lc = GetListCtrl();
        wxCHECK_RET( lc, _T("no listctrl window?") );

        lc->m_headerWin->Refresh();
        lc->m_headerWin->Update();
    }
}

int wxODListMainWindow::GetCountPerPage() const
{
    if ( !m_linesPerPage )
    {
        wxConstCast(this, wxODListMainWindow)->
            m_linesPerPage = GetClientSize().y / GetLineHeight();
    }

    return m_linesPerPage;
}

void wxODListMainWindow::GetVisibleLinesRange(size_t *from, size_t *to)
{
    wxASSERT_MSG( HasFlag(wxLC_REPORT), _T("this is for report mode only") );

    if ( m_lineFrom == (size_t)-1 )
    {
        size_t count = GetItemCount();
        if ( count )
        {
            m_lineFrom = GetScrollPos(wxVERTICAL);

            // this may happen if SetScrollbars() hadn't been called yet
            if ( m_lineFrom >= count )
                m_lineFrom = count - 1;

            // we redraw one extra line but this is needed to make the redrawing
            // logic work when there is a fractional number of lines on screen
            m_lineTo = m_lineFrom + m_linesPerPage;
            if ( m_lineTo >= count )
                m_lineTo = count - 1;
        }
        else // empty control
        {
            m_lineFrom = 0;
            m_lineTo = (size_t)-1;
        }
    }

    wxASSERT_MSG( IsEmpty() ||
                  (m_lineFrom <= m_lineTo && m_lineTo < GetItemCount()),
                  _T("GetVisibleLinesRange() returns incorrect result") );

    if ( from )
        *from = m_lineFrom;
    if ( to )
        *to = m_lineTo;
}

// -------------------------------------------------------------------------------------
// wxODGenericListCtrl
// -------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxODGenericListCtrl, wxControl)

BEGIN_EVENT_TABLE(wxODGenericListCtrl,wxControl)
  EVT_SIZE(wxODGenericListCtrl::OnSize)
  EVT_IDLE(wxODGenericListCtrl::OnIdle)
END_EVENT_TABLE()

wxODGenericListCtrl::wxODGenericListCtrl()
{
    m_imageListNormal = (wxImageListType *) NULL;
    m_imageListSmall = (wxImageListType *) NULL;
    m_imageListState = (wxImageListType *) NULL;

    m_ownsImageListNormal =
    m_ownsImageListSmall =
    m_ownsImageListState = FALSE;

    m_mainWin = (wxODListMainWindow*) NULL;
    m_headerWin = (wxODListHeaderWindow*) NULL;
}

wxODGenericListCtrl::~wxODGenericListCtrl()
{
    if (m_ownsImageListNormal)
        delete m_imageListNormal;
    if (m_ownsImageListSmall)
        delete m_imageListSmall;
    if (m_ownsImageListState)
        delete m_imageListState;
}

void wxODGenericListCtrl::CreateHeaderWindow()
{
    m_headerWin = new wxODListHeaderWindow
                      (
                        this, -1, m_mainWin,
                        wxPoint(0, 0),
                        wxSize(GetClientSize().x, HEADER_HEIGHT),
                        wxTAB_TRAVERSAL
                      );
}

bool wxODGenericListCtrl::GetFocus()
{
  return m_mainWin->m_hasFocus;
}

bool wxODGenericListCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint &pos,
                        const wxSize &size,
                        long style,
                        const wxValidator &validator,
                        const wxString &name)
{
    m_imageListNormal =
    m_imageListSmall =
    m_imageListState = (wxImageListType *) NULL;
    m_ownsImageListNormal =
    m_ownsImageListSmall =
    m_ownsImageListState = FALSE;

    m_mainWin = (wxODListMainWindow*) NULL;
    m_headerWin = (wxODListHeaderWindow*) NULL;

    if ( !(style & wxLC_MASK_TYPE) )
    {
        style = style | wxLC_LIST;
    }

    if ( !wxControl::Create( parent, id, pos, size, style, validator, name ) )
        return FALSE;

    // don't create the inner window with the border
    style &= ~wxBORDER_MASK;

    m_mainWin = new wxODListMainWindow( this, -1, wxPoint(0,0), size, style );

    if ( HasFlag(wxLC_REPORT) )
    {
        CreateHeaderWindow();

        if ( HasFlag(wxLC_NO_HEADER) )
        {
            // VZ: why do we create it at all then?
            m_headerWin->Show( FALSE );
        }
    }

    return TRUE;
}

void wxODGenericListCtrl::SetSingleStyle( long style, bool add )
{
    wxASSERT_MSG( !(style & wxLC_VIRTUAL),
                  _T("wxLC_VIRTUAL can't be [un]set") );

    long flag = GetWindowStyle();

    if (add)
    {
        if (style & wxLC_MASK_TYPE)
            flag &= ~(wxLC_MASK_TYPE | wxLC_VIRTUAL);
        if (style & wxLC_MASK_ALIGN)
            flag &= ~wxLC_MASK_ALIGN;
        if (style & wxLC_MASK_SORT)
            flag &= ~wxLC_MASK_SORT;
    }

    if (add)
    {
        flag |= style;
    }
    else
    {
        flag &= ~style;
    }

    SetWindowStyleFlag( flag );
}

void wxODGenericListCtrl::SetWindowStyleFlag( long flag )
{
    if (m_mainWin)
    {
        m_mainWin->DeleteEverything();

        // has the header visibility changed?
        bool hasHeader = HasFlag(wxLC_REPORT) && !HasFlag(wxLC_NO_HEADER),
             willHaveHeader = (flag & wxLC_REPORT) && !(flag & wxLC_NO_HEADER);

        if ( hasHeader != willHaveHeader )
        {
            // toggle it
            if ( hasHeader )
            {
                if ( m_headerWin )
                {
                    // don't delete, just hide, as we can reuse it later
                    m_headerWin->Show(FALSE);
                }
                //else: nothing to do
            }
            else // must show header
            {
                if (!m_headerWin)
                {
                    CreateHeaderWindow();
                }
                else // already have it, just show
                {
                    m_headerWin->Show( TRUE );
                }
            }

            ResizeReportView(willHaveHeader);
        }
    }

    wxWindow::SetWindowStyleFlag( flag );
}

bool wxODGenericListCtrl::GetColumn(int col, wxListItem &item) const
{
    m_mainWin->GetColumn( col, item );
    return TRUE;
}

bool wxODGenericListCtrl::SetColumn( int col, wxListItem& item )
{
    m_mainWin->SetColumn( col, item );
    return TRUE;
}

int wxODGenericListCtrl::GetColumnWidth( int col ) const
{
    return m_mainWin->GetColumnWidth( col );
}

bool wxODGenericListCtrl::SetColumnWidth( int col, int width )
{
    m_mainWin->SetColumnWidth( col, width );
    return TRUE;
}

int wxODGenericListCtrl::GetCountPerPage() const
{
  return m_mainWin->GetCountPerPage();  // different from Windows ?
}

bool wxODGenericListCtrl::GetItem( wxListItem &info ) const
{
    m_mainWin->GetItem( info );
    return TRUE;
}

bool wxODGenericListCtrl::SetItem( wxListItem &info )
{
    m_mainWin->SetItem( info );
    return TRUE;
}

long wxODGenericListCtrl::SetItem( long index, int col, const wxString& label, int imageId )
{
    wxListItem info;
    info.m_text = label;
    info.m_mask = wxLIST_MASK_TEXT;
    info.m_itemId = index;
    info.m_col = col;
    if ( imageId > -1 )
    {
        info.m_image = imageId;
        info.m_mask |= wxLIST_MASK_IMAGE;
    };
    m_mainWin->SetItem(info);
    return TRUE;
}

int wxODGenericListCtrl::GetItemState( long item, long stateMask ) const
{
    return m_mainWin->GetItemState( item, stateMask );
}

bool wxODGenericListCtrl::SetItemState( long item, long state, long stateMask )
{
    m_mainWin->SetItemState( item, state, stateMask );
    return TRUE;
}

bool wxODGenericListCtrl::SetItemImage( long item, int image, int WXUNUSED(selImage) )
{
    wxListItem info;
    info.m_image = image;
    info.m_mask = wxLIST_MASK_IMAGE;
    info.m_itemId = item;
    m_mainWin->SetItem( info );
    return TRUE;
}

wxString wxODGenericListCtrl::GetItemText( long item ) const
{
    return m_mainWin->GetItemText(item);
}

void wxODGenericListCtrl::SetItemText( long item, const wxString& str )
{
    m_mainWin->SetItemText(item, str);
}

long wxODGenericListCtrl::GetItemData( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.m_data;
}

bool wxODGenericListCtrl::SetItemData( long item, long data )
{
    wxListItem info;
    info.m_mask = wxLIST_MASK_DATA;
    info.m_itemId = item;
    info.m_data = data;
    m_mainWin->SetItem( info );
    return TRUE;
}

bool wxODGenericListCtrl::GetItemRect( long item, wxRect &rect,  int WXUNUSED(code) ) const
{
    m_mainWin->GetItemRect( item, rect );
    if ( m_mainWin->HasHeader() )
        rect.y += HEADER_HEIGHT + 1;
    return TRUE;
}

bool wxODGenericListCtrl::GetItemPosition( long item, wxPoint& pos ) const
{
    m_mainWin->GetItemPosition( item, pos );
    return TRUE;
}

bool wxODGenericListCtrl::SetItemPosition( long WXUNUSED(item), const wxPoint& WXUNUSED(pos) )
{
    return 0;
}

int wxODGenericListCtrl::GetItemCount() const
{
    return m_mainWin->GetItemCount();
}

int wxODGenericListCtrl::GetColumnCount() const
{
    return m_mainWin->GetColumnCount();
}

void wxODGenericListCtrl::SetItemSpacing( int spacing, bool isSmall )
{
    m_mainWin->SetItemSpacing( spacing, isSmall );
}

int wxODGenericListCtrl::GetItemSpacing( bool isSmall ) const
{
    return m_mainWin->GetItemSpacing( isSmall );
}

void wxODGenericListCtrl::OnDrawItem( int /*item*/, wxDC* /*dc*/, const wxRect& /*rect*/, const wxRect& /*rectHL*/, bool /*highlighted*/ )
{
  printf("::GenericListCtrl::OnDrawItem\n");
}

void wxODGenericListCtrl::SetItemTextColour( long item, const wxColour &col )
{
    wxListItem info;
    info.m_itemId = item;
    info.SetTextColour( col );
    m_mainWin->SetItem( info );
}

wxColour wxODGenericListCtrl::GetItemTextColour( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.GetTextColour();
}

void wxODGenericListCtrl::SetItemBackgroundColour( long item, const wxColour &col )
{
    wxListItem info;
    info.m_itemId = item;
    info.SetBackgroundColour( col );
    m_mainWin->SetItem( info );
}

wxColour wxODGenericListCtrl::GetItemBackgroundColour( long item ) const
{
    wxListItem info;
    info.m_itemId = item;
    m_mainWin->GetItem( info );
    return info.GetBackgroundColour();
}

int wxODGenericListCtrl::GetSelectedItemCount() const
{
    return m_mainWin->GetSelectedItemCount();
}

wxColour wxODGenericListCtrl::GetTextColour() const
{
    return GetForegroundColour();
}

void wxODGenericListCtrl::SetTextColour(const wxColour& col)
{
    SetForegroundColour(col);
}

long wxODGenericListCtrl::GetTopItem() const
{
    size_t top;
    m_mainWin->GetVisibleLinesRange(&top, NULL);
    return (long)top;
}

void wxODGenericListCtrl::GetVisibleLines(long* first,long* last)
{
  m_mainWin->GetVisibleLinesRange((size_t*)first,(size_t*)last);
}

long wxODGenericListCtrl::GetNextItem( long item, int geom, int state ) const
{
    return m_mainWin->GetNextItem( item, geom, state );
}

wxImageListType *wxODGenericListCtrl::GetImageList(int which) const
{
    if (which == wxIMAGE_LIST_NORMAL)
    {
        return m_imageListNormal;
    }
    else if (which == wxIMAGE_LIST_SMALL)
    {
        return m_imageListSmall;
    }
    else if (which == wxIMAGE_LIST_STATE)
    {
        return m_imageListState;
    }
    return (wxImageListType *) NULL;
}

void wxODGenericListCtrl::SetImageList( wxImageListType *imageList, int which )
{
    if ( which == wxIMAGE_LIST_NORMAL )
    {
        if (m_ownsImageListNormal) delete m_imageListNormal;
        m_imageListNormal = imageList;
        m_ownsImageListNormal = FALSE;
    }
    else if ( which == wxIMAGE_LIST_SMALL )
    {
        if (m_ownsImageListSmall) delete m_imageListSmall;
        m_imageListSmall = imageList;
        m_ownsImageListSmall = FALSE;
    }
    else if ( which == wxIMAGE_LIST_STATE )
    {
        if (m_ownsImageListState) delete m_imageListState;
        m_imageListState = imageList;
        m_ownsImageListState = FALSE;
    }

    m_mainWin->SetImageList( imageList, which );
}

void wxODGenericListCtrl::AssignImageList(wxImageListType *imageList, int which)
{
    SetImageList(imageList, which);
    if ( which == wxIMAGE_LIST_NORMAL )
        m_ownsImageListNormal = TRUE;
    else if ( which == wxIMAGE_LIST_SMALL )
        m_ownsImageListSmall = TRUE;
    else if ( which == wxIMAGE_LIST_STATE )
        m_ownsImageListState = TRUE;
}

bool wxODGenericListCtrl::Arrange( int WXUNUSED(flag) )
{
    return 0;
}

bool wxODGenericListCtrl::DeleteItem( long item )
{
    m_mainWin->DeleteItem( item );
    return TRUE;
}

bool wxODGenericListCtrl::DeleteAllItems()
{
    m_mainWin->DeleteAllItems();
    return TRUE;
}

bool wxODGenericListCtrl::DeleteAllColumns()
{
    size_t count = m_mainWin->m_columns.GetCount();
    for ( size_t n = 0; n < count; n++ )
        DeleteColumn(0);

    return TRUE;
}

void wxODGenericListCtrl::ClearAll()
{
    m_mainWin->DeleteEverything();
}

bool wxODGenericListCtrl::DeleteColumn( int col )
{
    m_mainWin->DeleteColumn( col );

    // if we don't have the header any longer, we need to relayout the window
    if ( !GetColumnCount() )
    {
        ResizeReportView(FALSE /* no header */);
    }

    return TRUE;
}

void wxODGenericListCtrl::Edit( long item )
{
    m_mainWin->EditLabel( item );
}

bool wxODGenericListCtrl::EnsureVisible( long item )
{
    m_mainWin->EnsureVisible( item );
    return TRUE;
}

long wxODGenericListCtrl::FindItem( long start, const wxString& str,  bool partial )
{
    return m_mainWin->FindItem( start, str, partial );
}

long wxODGenericListCtrl::FindItem( long start, long data )
{
    return m_mainWin->FindItem( start, data );
}

long wxODGenericListCtrl::FindItem( long WXUNUSED(start), const wxPoint& WXUNUSED(pt),
                           int WXUNUSED(direction))
{
    return 0;
}

long wxODGenericListCtrl::HitTest( const wxPoint &point, int &flags )
{
    return m_mainWin->HitTest( (int)point.x, (int)point.y, flags );
}

long wxODGenericListCtrl::InsertItem( wxListItem& info )
{
    m_mainWin->InsertItem( info );
    return info.m_itemId;
}

long wxODGenericListCtrl::InsertItem( long index, const wxString &label )
{
    wxListItem info;
    info.m_text = label;
    info.m_mask = wxLIST_MASK_TEXT;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxODGenericListCtrl::InsertItem( long index, int imageIndex )
{
    wxListItem info;
    info.m_mask = wxLIST_MASK_IMAGE;
    info.m_image = imageIndex;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxODGenericListCtrl::InsertItem( long index, const wxString &label, int imageIndex )
{
    wxListItem info;
    info.m_text = label;
    info.m_image = imageIndex;
    info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE;
    info.m_itemId = index;
    return InsertItem( info );
}

long wxODGenericListCtrl::InsertColumn( long col, wxListItem &item )
{
    wxCHECK_MSG( m_headerWin, -1, _T("can't add column in non report mode") );

    m_mainWin->InsertColumn( col, item );

    // if we hadn't had header before and have it now we need to relayout the
    // window
    if ( GetColumnCount() == 1 )
    {
        ResizeReportView(TRUE /* have header */);
    }

    m_headerWin->Refresh();

    return 0;
}

long wxODGenericListCtrl::InsertColumn( long col, const wxString &heading,
                               int format, int width )
{
    wxListItem item;
    item.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT;
    item.m_text = heading;
    if (width >= -2)
    {
        item.m_mask |= wxLIST_MASK_WIDTH;
        item.m_width = width;
    }
    item.m_format = format;

    return InsertColumn( col, item );
}

bool wxODGenericListCtrl::ScrollList( int WXUNUSED(dx), int WXUNUSED(dy) )
{
    return 0;
}

// Sort items.
// fn is a function which takes 3 long arguments: item1, item2, data.
// item1 is the long data associated with a first item (NOT the index).
// item2 is the long data associated with a second item (NOT the index).
// data is the same value as passed to SortItems.
// The return value is a negative number if the first item should precede the second
// item, a positive number of the second item should precede the first,
// or zero if the two items are equivalent.
// data is arbitrary data to be passed to the sort function.

bool wxODGenericListCtrl::SortItems( wxListCtrlCompare fn, long data )
{
    m_mainWin->SortItems( fn, data );
    return TRUE;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

void wxODGenericListCtrl::OnSize(wxSizeEvent& WXUNUSED(event))
{
    if ( !m_mainWin )
        return;

    ResizeReportView(m_mainWin->HasHeader());

    m_mainWin->RecalculatePositions();
}

void wxODGenericListCtrl::ResizeReportView(bool showHeader)
{
    int cw, ch;
    GetClientSize( &cw, &ch );

    if ( showHeader )
    {
        m_headerWin->SetSize( 0, 0, cw, HEADER_HEIGHT );
        m_mainWin->SetSize( 0, HEADER_HEIGHT + 1, cw, ch - HEADER_HEIGHT - 1 );
    }
    else // no header window
    {
        m_mainWin->SetSize( 0, 0, cw, ch );
    }
}

void wxODGenericListCtrl::OnIdle( wxIdleEvent & event )
{
    event.Skip();

    // do it only if needed
    if ( !m_mainWin->m_dirty )
        return;

    m_mainWin->RecalculatePositions();
}

// ----------------------------------------------------------------------------
// font/colours
// ----------------------------------------------------------------------------

bool wxODGenericListCtrl::SetBackgroundColour( const wxColour &colour )
{
    if (m_mainWin)
    {
        m_mainWin->SetBackgroundColour( colour );
        m_mainWin->m_dirty = TRUE;
    }

    return TRUE;
}

bool wxODGenericListCtrl::SetForegroundColour( const wxColour &colour )
{
    if ( !wxWindow::SetForegroundColour( colour ) )
        return FALSE;

    if (m_mainWin)
    {
        m_mainWin->SetForegroundColour( colour );
        m_mainWin->m_dirty = TRUE;
    }

    if (m_headerWin)
    {
        m_headerWin->SetForegroundColour( colour );
    }

    return TRUE;
}

bool wxODGenericListCtrl::SetFont( const wxFont &font )
{
    if ( !wxWindow::SetFont( font ) )
        return FALSE;

    if (m_mainWin)
    {
        m_mainWin->SetFont( font );
        m_mainWin->m_dirty = TRUE;
    }

    if (m_headerWin)
    {
        m_headerWin->SetFont( font );
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// methods forwarded to m_mainWin
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

void wxODGenericListCtrl::SetDropTarget( wxDropTarget *dropTarget )
{
    m_mainWin->SetDropTarget( dropTarget );
}

wxDropTarget *wxODGenericListCtrl::GetDropTarget() const
{
    return m_mainWin->GetDropTarget();
}

#endif // wxUSE_DRAG_AND_DROP

bool wxODGenericListCtrl::SetCursor( const wxCursor &cursor )
{
    return m_mainWin ? m_mainWin->wxWindow::SetCursor(cursor) : FALSE;
}

wxColour wxODGenericListCtrl::GetBackgroundColour() const
{
    return m_mainWin ? m_mainWin->GetBackgroundColour() : wxColour();
}

wxColour wxODGenericListCtrl::GetForegroundColour() const
{
    return m_mainWin ? m_mainWin->GetForegroundColour() : wxColour();
}

bool wxODGenericListCtrl::DoPopupMenu( wxMenu *menu, int x, int y )
{
#if wxUSE_MENUS
    return m_mainWin->PopupMenu( menu, x, y );
#else
    return FALSE;
#endif // wxUSE_MENUS
}

void wxODGenericListCtrl::SetFocus()
{
    /* The test in window.cpp fails as we are a composite
       window, so it checks against "this", but not m_mainWin. */
    if ( FindFocus() != this )
        m_mainWin->SetFocus();
}

// ----------------------------------------------------------------------------
// virtual list control support
// ----------------------------------------------------------------------------

wxString wxODGenericListCtrl::OnGetItemText(long WXUNUSED(item), long WXUNUSED(col)) const
{
    // this is a pure virtual function, in fact - which is not really pure
    // because the controls which are not virtual don't need to implement it
    wxFAIL_MSG( _T("wxODGenericListCtrl::OnGetItemText not supposed to be called") );

    return wxEmptyString;
}

int wxODGenericListCtrl::OnGetItemImage(long WXUNUSED(item)) const
{
    // same as above
    wxFAIL_MSG( _T("wxODGenericListCtrl::OnGetItemImage not supposed to be called") );

    return -1;
}

wxListItemAttr *wxODGenericListCtrl::OnGetItemAttr(long item) const
{
    wxASSERT_MSG( item >= 0 && item < GetItemCount(),
                  _T("invalid item index in OnGetItemAttr()") );

    // no attributes by default
    return NULL;
}

void wxODGenericListCtrl::SetItemCount(long count)
{
    wxASSERT_MSG( IsVirtual(), _T("this is for virtual controls only") );

    m_mainWin->SetItemCount(count);
}

void wxODGenericListCtrl::RefreshItem(long item)
{
    m_mainWin->RefreshLine(item);
}

void wxODGenericListCtrl::RefreshItems(long itemFrom, long itemTo)
{
    m_mainWin->RefreshLines(itemFrom, itemTo);
}

void wxODGenericListCtrl::Freeze()
{
    m_mainWin->Freeze();
}

void wxODGenericListCtrl::Thaw()
{
    m_mainWin->Thaw();
}

void wxODGenericListCtrl::SetSortArrow(int col,int mode)
{
  m_mainWin->SetSortArrow(col,mode);
}

#endif // __WXMSW__

#endif // wxUSE_LISTCTRL
