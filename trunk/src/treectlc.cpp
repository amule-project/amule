// This file contains aMule extensions to wxwindows classes.
// This file is part of the aMule project.
//
// Copyright (c) 1998 Robert Roebling, Julian Smart and Markus Holzem
// Copyright (c) 2004, Carlo Wood <carlo@alinoe.com>
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

#include "color.h"		// Needed for G_BLEND

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "treebasc.h"
#include "treectlc.h"
#include <wx/settings.h>
#include <wx/timer.h>

/* XPM */
static char const* check1_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #000000",
"+	c #FFFFFF",
"                ",
"                ",
"                ",
"                ",
"  ............  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  .++++++++++.  ",
"  ............  "};

/* XPM */
static char const* check2_xpm[] = {
"16 16 3 1",
" 	c None",
".	c #000000",
"+	c #FFFFFF",
"                ",
"                ",
"                ",
"                ",
"  ............  ",
"  .++++++++++.  ",
"  .+.++++++.+.  ",
"  .++.++++.++.  ",
"  .+++.++.+++.  ",
"  .++++..++++.  ",
"  .++++..++++.  ",
"  .+++.++.+++.  ",
"  .++.++++.++.  ",
"  .+.++++++.+.  ",
"  .++++++++++.  ",
"  ............  "};

class WXDLLEXPORT wxGenericCTreeItem;
WX_DEFINE_EXPORTED_ARRAY(wxGenericCTreeItem*, wxArrayGenericCTreeItems);

//-------------------------------------------------------------------
// Interface of class wxGenericTreeItem
//
// This code is a duplication of wxGTK version 2.4.2.
// wxGTK-2.4.2/src/generic/treectlg.cpp
//
// Only the *interface* is declared here, we must link with
// the real implementation of the library because different
// versions of the library might have reordered their members.
//
// Unfortunately, inlined methods of wxGenericTreeItem are
// not included in the library, therefore they have to be
// re-declared here.  This will only work when the data members
// of the class in the real library are the same as in
// the linked library :(.  The version below was checked to
// correspond with wxGTK 2.4.x and 2.5 (20040104).
//

// This is strictly not correct of course, but
// sizeof(wxArrayGenericCTreeItems) == sizeof(wxArrayGenericTreeItems)
// and the array basically only deals with pointers anyway.
// Casts are used to arrrive at the true type (xGenericCTreeItem*).
// On top of that, a wxGenericCTreeItem is a wxGenericTreeItem...
// And well, some major chicken waving involved here - but it works.
typedef wxArrayGenericCTreeItems wxArrayGenericTreeItems;

//----------------------------------------------------------------------
// Duplicated code, from wxGTK CVS 20040104, ./src/generic/treectlg.cpp.

class WXDLLEXPORT wxGenericTreeItem
{
public:
    // ctors & dtor
    wxGenericTreeItem() { m_data = NULL; }
    wxGenericTreeItem( wxGenericTreeItem *parent,
                       const wxString& text,
                       int image,
                       int selImage,
                       wxTreeItemData *data );

    ~wxGenericTreeItem();

    // trivial accessors
    wxArrayGenericTreeItems& GetChildren() { return m_children; }

    const wxString& GetText() const { return m_text; }
    int GetImage(wxTreeItemIcon which = wxTreeItemIcon_Normal) const
        { return m_images[which]; }
    wxTreeItemData *GetData() const { return m_data; }

    // returns the current image for the item (depending on its
    // selected/expanded/whatever state)
    int GetCurrentImage() const;

    void SetText( const wxString &text );
    void SetImage(int image, wxTreeItemIcon which) { m_images[which] = image; }
    void SetData(wxTreeItemData *data) { m_data = data; }

    void SetHasPlus(bool has = TRUE) { m_hasPlus = has; }

    void SetBold(bool bold) { m_isBold = bold; }

    int GetX() const { return m_x; }
    int GetY() const { return m_y; }

    void SetX(int x) { m_x = x; }
    void SetY(int y) { m_y = y; }

    int  GetHeight() const { return m_height; }
    int  GetWidth()  const { return m_width; }

    void SetHeight(int h) { m_height = h; }
    void SetWidth(int w) { m_width = w; }

    wxGenericTreeItem *GetParent() const { return m_parent; }

    // operations
        // deletes all children notifying the treectrl about it if !NULL
        // pointer given
    void DeleteChildren(wxGenericTreeCtrl *tree = NULL);

    // get count of all children (and grand children if 'recursively')
    size_t GetChildrenCount(bool recursively = TRUE) const;

    void Insert(wxGenericCTreeItem *child, size_t index)
	{ m_children.Insert(child, index); }

    void GetSize( int &x, int &y, const wxGenericTreeCtrl* );

        // return the item at given position (or NULL if no item), onButton is
        // TRUE if the point belongs to the item's button, otherwise it lies
        // on the button's label
    wxGenericTreeItem *HitTest( const wxPoint& point,
                                const wxGenericTreeCtrl *,
                                int &flags,
                                int level );

    void Expand() { m_isCollapsed = FALSE; }
    void Collapse() { m_isCollapsed = TRUE; }

    void SetHilight( bool set = TRUE ) { m_hasHilight = set; }

    // status inquiries
    bool HasChildren() const { return !m_children.IsEmpty(); }
    bool IsSelected()  const { return m_hasHilight != 0; }
    bool IsExpanded()  const { return !m_isCollapsed; }
    bool HasPlus()     const { return m_hasPlus || HasChildren(); }
    bool IsBold()      const { return m_isBold != 0; }

    // attributes
        // get them - may be NULL
    wxTreeItemAttr *GetAttributes() const { return m_attr; }
        // get them ensuring that the pointer is not NULL
    wxTreeItemAttr& Attr()
    {
        if ( !m_attr )
        {
            m_attr = new wxTreeItemAttr;
            m_ownsAttr = TRUE;
        }
        return *m_attr;
    }
        // set them
    void SetAttributes(wxTreeItemAttr *attr)
    {
        if ( m_ownsAttr ) delete m_attr;
        m_attr = attr;
        m_ownsAttr = FALSE;
    }
        // set them and delete when done
    void AssignAttributes(wxTreeItemAttr *attr)
    {
        SetAttributes(attr);
        m_ownsAttr = TRUE;
    }

private:
    // since there can be very many of these, we save size by chosing
    // the smallest representation for the elements and by ordering
    // the members to avoid padding.
    wxString            m_text;         // label to be rendered for item

    wxTreeItemData     *m_data;         // user-provided data

    wxArrayGenericTreeItems m_children; // list of children
    wxGenericTreeItem  *m_parent;       // parent of this item

    wxTreeItemAttr     *m_attr;         // attributes???

    // tree ctrl images for the normal, selected, expanded and
    // expanded+selected states
    short               m_images[wxTreeItemIcon_Max];

    wxCoord             m_x;            // (virtual) offset from top
    wxCoord             m_y;            // (virtual) offset from left
    short               m_width;        // width of this item
    unsigned char       m_height;       // height of this item

    // use bitfields to save size
    int                 m_isCollapsed :1;
    int                 m_hasHilight  :1; // same as focused
    int                 m_hasPlus     :1; // used for item which doesn't have
                                          // children but has a [+] button
    int                 m_isBold      :1; // render the label in bold font
    int                 m_ownsAttr    :1; // delete attribute when done

    DECLARE_NO_COPY_CLASS(wxGenericTreeItem)
};

// End of code duplication.
//-------------------------------------------------------------------

//
// This class may not add extra non-POD data members, it may not have
// (need) a destructor and a cast from a wxGenericCTreeItem to
// wxGenericTreeItem may not change the pointer Thus:
//
// wxGenericCTreeItem item;
// assert( &item == static_cast<wxGenericTreeItem*>(&item) );
//
// The reason for that is that wxwindows is using C-casts on this class.
// The following code should be valid:
//
// wxGenericCTreeItem* ip = new wxGenericCTreeItem;
// delete (wxGenericTreeItem*)ip;
//
class wxGenericCTreeItem : public wxGenericTreeItem {
private:
  bool m_checked;

public:
  // Constructors.
  wxGenericCTreeItem(void) : m_checked(FALSE) { }

  wxGenericCTreeItem(wxGenericCTreeItem* parent, wxString const& text,
      int image, int selImage, wxTreeItemData* data) :
      wxGenericTreeItem(parent, text, image, selImage, data), m_checked(FALSE) { }

  // Accessors that need a cast.
  wxArrayGenericCTreeItems& GetChildren(void)
      { return reinterpret_cast<wxArrayGenericCTreeItems&>(wxGenericTreeItem::GetChildren()); }
  wxGenericCTreeItem* GetParent(void) const
      { return static_cast<wxGenericCTreeItem*>(wxGenericTreeItem::GetParent()); }

  // aMule extensions.
  void SetChecked(bool checked) { m_checked = checked; }
  bool IsChecked(void) const { return m_checked; }

  // Overridden method (also exists in base class).
  wxGenericCTreeItem* HitTest(wxPoint const& point,
      wxGenericCTreeCtrl const*, int& flags, int level);
};

// =============================================================================
// implementation
// =============================================================================

// Duplicated constant from wxwindows 2.4.2.
static int const NO_IMAGE = -1;

wxGenericCTreeItem* wxGenericCTreeItem::HitTest(
    wxPoint const& point, wxGenericCTreeCtrl const* theCtrl, int& flags, int level)
{
    // for a hidden root node, don't evaluate it, but do evaluate children
    if ( !(level == 0 && theCtrl->HasFlag(wxTR_HIDE_ROOT)) )
    {
        // evaluate the item
        int h = theCtrl->GetLineHeight(this);
	int const x = GetX();
	int const y = GetY();
        if ((point.y > y) && (point.y < y + h))
        {
            int y_mid = y + h/2;
            if (point.y < y_mid )
                flags |= wxTREE_HITTEST_ONITEMUPPERPART;
            else
                flags |= wxTREE_HITTEST_ONITEMLOWERPART;

            // 5 is the size of the plus sign
            int xCross = x - theCtrl->GetSpacing();
            if ((point.x > xCross-5) && (point.x < xCross+5) &&
                (point.y > y_mid-5) && (point.y < y_mid+5) &&
                HasPlus() && theCtrl->HasButtons() )
            {
                flags |= wxTREE_HITTEST_ONITEMBUTTON;
                return this;
            }

	    int const width = GetWidth();
            if ((point.x >= x) && (point.x <= x + width))
            {
                int image_w = -1;
                int image_h;
		// wxGenericCTreeItem::HitTest
		int cboffset=theCtrl->HasFlag(wxTR_CHECKBOX)?16:0;

                // assuming every image (normal and selected) has the same size!
                if ( (GetImage() != NO_IMAGE) && theCtrl->m_imageListNormal )
                    theCtrl->m_imageListNormal->GetSize(GetImage(),
                                                        image_w, image_h);

                if ((image_w != -1) && (point.x>x+cboffset) &&(point.x <= x + cboffset +image_w + 1))
                    flags |= wxTREE_HITTEST_ONITEMICON;
                else {
		  if(theCtrl->HasFlag(wxTR_CHECKBOX)) {
		    if(point.x<=x+16) {
		      flags|=wxTREE_HITTEST_CHECKBOX;
		    }
		  } else 
                    flags |= wxTREE_HITTEST_ONITEMLABEL;
		}

                return this;
            }

            if (point.x < x)
                flags |= wxTREE_HITTEST_ONITEMINDENT;
            if (point.x > x+width)
                flags |= wxTREE_HITTEST_ONITEMRIGHT;

            return this;
        }

        // if children are expanded, fall through to evaluate them
        if (!IsExpanded()) return (wxGenericCTreeItem*) NULL;
    }

    // evaluate children
    wxArrayGenericTreeItems const& children(GetChildren());
    size_t count = children.Count();
    for ( size_t n = 0; n < count; n++ )
    {
        wxGenericCTreeItem* res =
	    static_cast<wxGenericCTreeItem*>(children[n])->
	    HitTest(point, theCtrl, flags, level + 1);
        if ( res != NULL )
            return res;
    }

    return (wxGenericCTreeItem*) NULL;
}

// -----------------------------------------------------------------------------
// wxGenericCTreeCtrl implementation
// -----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxGenericCTreeCtrl, wxGenericTreeCtrl)

BEGIN_EVENT_TABLE(wxGenericCTreeCtrl, wxGenericTreeCtrl)
    EVT_PAINT        (wxGenericCTreeCtrl::OnPaint)
    EVT_MOUSE_EVENTS (wxGenericCTreeCtrl::OnMouse)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxCTreeCtrl, wxGenericCTreeCtrl)

// -----------------------------------------------------------------------------
// construction/destruction
// -----------------------------------------------------------------------------

void wxGenericCTreeCtrl::Init(void)
{
  // This function is only called by the constructors.
  m_check1 = NULL;
  m_check2 = NULL;

  wxColour col = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
  wxColour newcol = wxColour(G_BLEND(col.Red(), 125),
			     G_BLEND(col.Green(), 125),
			     G_BLEND(col.Blue(), 125));

  // Replace the m_hilightBrush that was added by the base class.
  delete m_hilightBrush;
  m_hilightBrush = new wxBrush(newcol, wxSOLID);
  
  col = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
  newcol = wxColour(G_BLEND(col.Red(), 125),
		    G_BLEND(col.Green(), 125),
		    G_BLEND(col.Blue(), 125));

  // Replace the m_hilightUnfocusedBrush that was added by the base class.
  delete m_hilightUnfocusedBrush;
  m_hilightUnfocusedBrush = new wxBrush(newcol, wxSOLID);

  m_check1 = new wxBitmap(check1_xpm);
  m_check2 = new wxBitmap(check2_xpm);
}

// -----------------------------------------------------------------------------
// accessors
// -----------------------------------------------------------------------------

void wxGenericCTreeCtrl::SetChecked(wxTreeItemId const& item, bool mode)
{
  wxGenericCTreeItem* pItem = reinterpret_cast<wxGenericCTreeItem*>(item.m_pItem);
  pItem->SetChecked(mode);
  if (mode)
  { 
    wxGenericTreeItem* p = pItem; 
    while((p = p->GetParent()))
      SetItemBold(p, true);
  }
  else if (!pItem->IsBold())
  {
    wxGenericTreeItem* p = pItem; 
    while((p = p->GetParent()))
    {
      wxArrayGenericTreeItems& children(p->GetChildren());
      int c;
      for(c = children.Count() - 1; c >= 0; --c)
	if (children[c]->IsBold() || children[c]->IsChecked())
	  break;
      if (c >= 0)
        break;
      SetItemBold(p, false);
    }
  }
  RefreshLine(pItem);
}

// -----------------------------------------------------------------------------
// item status inquiries
// -----------------------------------------------------------------------------

bool wxGenericCTreeCtrl::IsChecked(wxTreeItemId const& item) const 
{
  wxCHECK_MSG( item.IsOk(), FALSE, wxT("invalid tree item") );

  wxGenericCTreeItem* pItem = reinterpret_cast<wxGenericCTreeItem*>(item.m_pItem);
  return pItem->IsChecked();
}

// -----------------------------------------------------------------------------
// helpers
// -----------------------------------------------------------------------------

void wxGenericCTreeCtrl::PaintItem(wxGenericCTreeItem* item, wxDC& dc)
{
  // TODO implement "state" icon on items

  wxTreeItemAttr* attr = item->GetAttributes();
  if (attr && attr->HasFont())
    dc.SetFont(attr->GetFont());
  else if (item->IsBold())
    dc.SetFont(m_boldFont);

  long text_w = 0;
  long text_h = 0;
  dc.GetTextExtent(item->GetText(), &text_w, &text_h);

  int image_h = 0;
  int image_w = 0;
  int image = item->GetCurrentImage();
  if (image != NO_IMAGE)
  {
    if (m_imageListNormal)
    {
      m_imageListNormal->GetSize(image, image_w, image_h);
      image_w += 4;
    }
    else
      image = NO_IMAGE;
  }

  int total_h = GetLineHeight(item);

  wxPen mypen;
  wxColour col;

  if (0 && item->IsSelected())
  {
    // emulate WinXP look :)
    if(m_hasFocus)
      col=m_hilightBrush->GetColour();	
    else
      col=m_hilightUnfocusedBrush->GetColour();
    // border colour is 10% less
    wxColour brd = wxColour(G_BLEND(col.Red(), 65),
                            G_BLEND(col.Green(), 65),
			    G_BLEND(col.Blue(), 65));
    
    mypen = wxPen(brd, 1, wxSOLID);
    dc.SetPen(mypen);
    dc.SetBrush(*(m_hasFocus ? m_hilightBrush : m_hilightUnfocusedBrush));
  }
  else
  {
    wxColour colBg;
    if (attr && attr->HasBackgroundColour())
      colBg = attr->GetBackgroundColour();
    else
      colBg = m_backgroundColour;
    dc.SetBrush(wxBrush(colBg, wxSOLID));
  }

  int offset = HasFlag(wxTR_ROW_LINES) ? 1 : 0;
  int cboffset = HasFlag(wxTR_CHECKBOX) ? 16 : 0;

  if (HasFlag(wxTR_FULL_ROW_HIGHLIGHT))
  {
    int x, y, w, h;

    DoGetPosition(&x, &y);
    DoGetSize(&w, &h);
    dc.DrawRectangle(x, item->GetY() + offset, w, total_h - offset);
  }
  else
  {
    if (item->IsSelected() && image != NO_IMAGE)
    {
      // If it's selected, and there's an image, then we should
      // take care to leave the area under the image painted in the
      // background colour.
      dc.DrawRectangle(item->GetX() + cboffset + image_w - 2, item->GetY() + offset,
          item->GetWidth() - image_w + 2, total_h-offset );
    }
    else
      dc.DrawRectangle( item->GetX() - 2 + cboffset, item->GetY() + offset,
          item->GetWidth() + 2, total_h - offset );
  }

  if (image != NO_IMAGE)
  {
    dc.SetClippingRegion(item->GetX() + cboffset, item->GetY(), image_w - 2, total_h);
    m_imageListNormal->Draw(image, dc,
			    item->GetX() + cboffset,
			    item->GetY() + ((total_h > image_h) ? ((total_h-image_h) / 2) : 0),
			    wxIMAGELIST_DRAW_TRANSPARENT);
    dc.DestroyClippingRegion();
  }

  if (HasFlag(wxTR_CHECKBOX))
  {
    if(item->IsChecked())
      dc.DrawBitmap(*m_check2, item->GetX(), item->GetY(), TRUE);
    else
      dc.DrawBitmap(*m_check1, item->GetX(), item->GetY(), TRUE);
  }

  dc.SetBackgroundMode(wxTRANSPARENT);
  int extraH = (total_h > text_h) ? (total_h - text_h) / 2 : 0;
  dc.DrawText(item->GetText(),
	      (wxCoord)(image_w + item->GetX() + cboffset),
	      (wxCoord)(item->GetY() + extraH));

  // restore normal font
  dc.SetFont(m_normalFont);
}

// -----------------------------------------------------------------------------
// wxWindows callbacks
// -----------------------------------------------------------------------------

wxTreeItemId wxGenericCTreeCtrl::HitTest(wxPoint const& point, int& flags)
{
  // JACS: removed wxYieldIfNeeded() because it can cause the window
  // to be deleted from under us if a close window event is pending

  int w, h;
  GetSize(&w, &h);
  flags=0;
  if (point.x<0) flags |= wxTREE_HITTEST_TOLEFT;
  if (point.x>w) flags |= wxTREE_HITTEST_TORIGHT;
  if (point.y<0) flags |= wxTREE_HITTEST_ABOVE;
  if (point.y>h) flags |= wxTREE_HITTEST_BELOW;
  if (flags) return wxTreeItemId();

  if (m_anchor == NULL)
  {
      flags = wxTREE_HITTEST_NOWHERE;
      return wxTreeItemId();
  }

  wxGenericCTreeItem* hit =
      static_cast<wxGenericCTreeItem*>(m_anchor)->
      HitTest(CalcUnscrolledPosition(point), this, flags, 0);

  if (hit == NULL)
  {
      flags = wxTREE_HITTEST_NOWHERE;
      return wxTreeItemId();
  }
  return hit;
}

// Duplicated function from wxwindows.
static void EventFlagsToSelType(long style,
                                bool shiftDown,
                                bool ctrlDown,
                                bool &is_multiple,
                                bool &extended_select,
                                bool &unselect_others)
{
  is_multiple = (style & wxTR_MULTIPLE) != 0;
  extended_select = shiftDown && is_multiple;
  unselect_others = !(extended_select || (ctrlDown && is_multiple));
}

void wxGenericCTreeCtrl::OnMouse( wxMouseEvent &event )
{
  // Most of the code in this function is duplicated from
  // wxGenericTreeCtrl::OnMouse(event).

  if ( !m_anchor ) return;

  if (event.LeftDown() || event.LeftDClick())
  {
    wxPoint pt = CalcUnscrolledPosition(event.GetPosition());
    int flags = 0;
    wxGenericCTreeItem* item =
	static_cast<wxGenericCTreeItem*>(m_anchor)->
	HitTest(pt, this, flags, 0);
    m_dragCount = 0;
    if (item == NULL) return;  /* we hit the blank area */

    if ( event.LeftDown() )
    {
	m_lastOnSame = item == m_current;
    }
    if ( flags & wxTREE_HITTEST_ONITEMBUTTON )
    {
	// only toggle the item for a single click, double click on
	// the button doesn't do anything (it toggles the item twice)
	if ( event.LeftDown() )
	{
	    Toggle( item );
	}

	// don't select the item if the button was clicked
	return;
    }

    //--------------------------------------------------------------
    // aMule extension start.

    if((flags & wxTREE_HITTEST_CHECKBOX) && event.LeftDown())
      SetChecked(item, !IsChecked(item));	// Toggle

    // aMule extension end.
    //--------------------------------------------------------------

    // how should the selection work for this event?
    bool is_multiple, extended_select, unselect_others;
    EventFlagsToSelType(GetWindowStyleFlag(),
			event.ShiftDown(),
			event.ControlDown(),
			is_multiple, extended_select, unselect_others);

    SelectItem(item, unselect_others, extended_select);

    //--------------------------------------------------------------
    // aMule extension start.
    if(event.LeftDown()) {
      wxCTreeEvent nevent(wxEVT_COMMAND_TREE_ITEM_LEFT_CLICK, GetId());
      nevent.SetItem(item);
      nevent.SetPoint(CalcScrolledPosition(pt));
      nevent.SetEventObject(this);
      nevent.SetControlDown(event.ControlDown());
      nevent.SetShiftDown(event.ShiftDown());
      GetEventHandler()->ProcessEvent(nevent);
    }
    // aMule extension end.
    //--------------------------------------------------------------

    // For some reason, Windows isn't recognizing a left double-click,
    // so we need to simulate it here.  Allow 200 milliseconds for now.
    if ( event.LeftDClick() )
    {
	// double clicking should not start editing the item label
	if ( m_renameTimer )
	    m_renameTimer->Stop();

	m_lastOnSame = FALSE;

	// send activate event first
	wxCTreeEvent nevent( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, GetId() );
	nevent.SetItem(item);
	nevent.SetPoint(CalcScrolledPosition(pt));
	nevent.SetEventObject( this );
	if ( !GetEventHandler()->ProcessEvent( nevent ) )
	{
	    // if the user code didn't process the activate event,
	    // handle it ourselves by toggling the item when it is
	    // double clicked
	    if ( item->HasPlus() )
	    {
		Toggle(item);
	    }
	}
    }
  }
  else
    return wxGenericTreeCtrl::OnMouse(event);
}

// Everything below is duplicated from wxGTK-2.4.2/src/generic/treectlg.cpp
// Extensions are marked with "aMule extension".

// The reason for this duplication is so we create a wxGenericCTreeItem instead of wxGenericTreeItem.
wxTreeItemId wxGenericCTreeCtrl::DoInsertItem(const wxTreeItemId& parentId,
                                      size_t previous,
                                      const wxString& text,
                                      int image, int selImage,
                                      wxTreeItemData *data)
{
    // aMule extension: Use wxGenericCTreeItem instead of wxGenericTreeItem.
    wxGenericCTreeItem *parent = reinterpret_cast<wxGenericCTreeItem*>(parentId.m_pItem);
    if ( !parent )
    {
        // should we give a warning here?
        return AddRoot(text, image, selImage, data);
    }

    m_dirty = TRUE;     // do this first so stuff below doesn't cause flicker

    // aMule extension: Use wxGenericCTreeItem instead of wxGenericTreeItem.
    wxGenericCTreeItem *item =
        new wxGenericCTreeItem( parent, text, image, selImage, data );

    if ( data != NULL )
    {
        data->SetId((long) item);
    }

    parent->Insert( item, previous );

    return item;
}

// The reason for this duplication is so we call wxGenericCTreeItem::DoInsertItem.
wxTreeItemId wxGenericCTreeCtrl::AppendItem(const wxTreeItemId& parentId,
                                    const wxString& text,
                                    int image, int selImage,
                                    wxTreeItemData *data)
{
    wxGenericTreeItem *parent = reinterpret_cast<wxGenericCTreeItem*>(parentId.m_pItem);
    if ( !parent )
    {
        // should we give a warning here?
        return AddRoot(text, image, selImage, data);
    }

    return DoInsertItem( parent, parent->GetChildren().Count(), text,
                         image, selImage, data);
}

// The reason for this duplication is so we create a wxGenericCTreeItem instead of wxGenericTreeItem.
wxTreeItemId
wxGenericCTreeCtrl::AddRoot(wxString const& text, int image, int selImage, wxTreeItemData* data)
{
  wxCHECK_MSG( !m_anchor, wxTreeItemId(), wxT("tree can have only one root") );

  m_dirty = TRUE;     // do this first so stuff below doesn't cause flicker

  // aMule extension: Use wxGenericCTreeItem instead of wxGenericTreeItem.
  m_anchor = new wxGenericCTreeItem((wxGenericCTreeItem *)NULL, text,
				 image, selImage, data);
  if ( data != NULL )
  {
      data->SetId(wxTreeItemId(m_anchor));
  }

  if (HasFlag(wxTR_HIDE_ROOT))
  {
      // if root is hidden, make sure we can navigate
      // into children
      m_anchor->SetHasPlus();
      m_anchor->Expand();
      CalculatePositions();
  }

  if (!HasFlag(wxTR_MULTIPLE))
  {
      m_current = m_key_current = m_anchor;
      m_current->SetHilight( TRUE );
  }

  return m_anchor;
}

// The only reason for this duplication is so that we will call wxGenericCTreeCtrl::PaintLevel.
// We also cast m_anchor to a wxGenericCTreeItem* as usual.
void wxGenericCTreeCtrl::OnPaint(wxPaintEvent&)
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  if (!m_anchor)
    return;

  dc.SetFont(m_normalFont);
  dc.SetPen(m_dottedPen);

  int y = 2;
  PaintLevel(static_cast<wxGenericCTreeItem*>(m_anchor), dc, 0, y);
}

// The only reason for this duplication is so that we will call wxGenericCTreeCtrl::PaintItem.
// We use wxGenericCTreeItem* instead of wxGenericTreeItem* as type of `item' though.
void wxGenericCTreeCtrl::PaintLevel( wxGenericCTreeItem *item, wxDC &dc, int level, int &y )
{
    int x = level*m_indent;
    if (!HasFlag(wxTR_HIDE_ROOT))
    {
        x += m_indent;
    }
    else if (level == 0)
    {
        // always expand hidden root
        int origY = y;
        wxArrayGenericTreeItems& children = item->GetChildren();
        int count = children.Count();
        if (count > 0)
        {
            int n = 0, oldY;
            do {
                oldY = y;
                PaintLevel(children[n], dc, 1, y);
            } while (++n < count);

            if (!HasFlag(wxTR_NO_LINES) && HasFlag(wxTR_LINES_AT_ROOT) && count > 0)
            {
                // draw line down to last child
                origY += GetLineHeight(children[0])>>1;
                oldY += GetLineHeight(children[n-1])>>1;
                dc.DrawLine(3, origY, 3, oldY);
            }
        }
        return;
    }

    item->SetX(x+m_spacing);
    item->SetY(y);

    int h = GetLineHeight(item);
    int y_top = y;
    int y_mid = y_top + (h>>1);
    y += h;

    int exposed_x = dc.LogicalToDeviceX(0);
    int exposed_y = dc.LogicalToDeviceY(y_top);

    if (IsExposed(exposed_x, exposed_y, 10000, h))  // 10000 = very much
    {
        wxPen *pen =
#ifndef __WXMAC__
            // don't draw rect outline if we already have the
            // background color under Mac
            (0 && item->IsSelected() && m_hasFocus) ? wxBLACK_PEN :
#endif // !__WXMAC__
            wxTRANSPARENT_PEN;

        wxColour colText;
        if (0 && item->IsSelected() )
        {
            colText = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
        }
        else
        {
            wxTreeItemAttr *attr = item->GetAttributes();
            if (attr && attr->HasTextColour())
                colText = attr->GetTextColour();
            else
                colText = GetForegroundColour();
        }

        // prepare to draw
        dc.SetTextForeground(colText);
        dc.SetPen(*pen);

        // draw
        PaintItem(item, dc);

        if (HasFlag(wxTR_ROW_LINES))
        {
            // if the background colour is white, choose a
            // contrasting color for the lines
            dc.SetPen(*((GetBackgroundColour() == *wxWHITE)
                         ? wxMEDIUM_GREY_PEN : wxWHITE_PEN));
            dc.DrawLine(0, y_top, 10000, y_top);
            dc.DrawLine(0, y, 10000, y);
        }

        // restore DC objects
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(m_dottedPen);
        dc.SetTextForeground(*wxBLACK);

        if (item->HasPlus() && HasButtons())  // should the item show a button?
        {
            if (!HasFlag(wxTR_NO_LINES))
            {
                if (x > (signed)m_indent)
                    dc.DrawLine(x - m_indent, y_mid, x - 5, y_mid);
                else if (HasFlag(wxTR_LINES_AT_ROOT))
                    dc.DrawLine(3, y_mid, x - 5, y_mid);
                dc.DrawLine(x + 5, y_mid, x + m_spacing, y_mid);
            }

            if (m_imageListButtons != NULL)
            {
                // draw the image button here
                int image_h = 0, image_w = 0, image = wxTreeItemIcon_Normal;
                if (item->IsExpanded()) image = wxTreeItemIcon_Expanded;
                if (item->IsSelected())
                    image += wxTreeItemIcon_Selected - wxTreeItemIcon_Normal;
                m_imageListButtons->GetSize(image, image_w, image_h);
                int xx = x - (image_w>>1);
                int yy = y_mid - (image_h>>1);
                dc.SetClippingRegion(xx, yy, image_w, image_h);
                m_imageListButtons->Draw(image, dc, xx, yy,
                                         wxIMAGELIST_DRAW_TRANSPARENT);
                dc.DestroyClippingRegion();
            }
            else if (HasFlag(wxTR_TWIST_BUTTONS))
            {
                // draw the twisty button here

                if (HasFlag(wxTR_AQUA_BUTTONS))
                {
                    // This causes update problems, so disabling for now.
#if 0 //#ifdef __WXMAC__
                    wxMacPortSetter helper(&dc) ;
                    wxMacWindowClipper clipper(this) ;
                    wxDC::MacSetupBackgroundForCurrentPort( MacGetBackgroundBrush() ) ;

                    int loc_x = x - 5 ;
                    int loc_y = y_mid - 6 ;
                    MacWindowToRootWindow( & loc_x , & loc_y ) ;
                    Rect bounds = { loc_y , loc_x , loc_y + 18 , loc_x + 12 } ;
                    ThemeButtonDrawInfo info = { kThemeStateActive , item->IsExpanded() ? kThemeDisclosureDown : kThemeDisclosureRight ,
                        kThemeAdornmentNone };
                    DrawThemeButton( &bounds, kThemeDisclosureButton ,
                        &info , NULL , NULL , NULL , NULL ) ;
#else
                    if (item->IsExpanded())
                        dc.DrawBitmap( *m_arrowDown, x-5, y_mid-6, TRUE );
                    else
                        dc.DrawBitmap( *m_arrowRight, x-5, y_mid-6, TRUE );
#endif
                }
                else
                {
                    dc.SetBrush(*m_hilightBrush);
                    dc.SetPen(*wxBLACK_PEN);
                    wxPoint button[3];

                    if (item->IsExpanded())
                    {
                        button[0].x = x-5;
                        button[0].y = y_mid-2;
                        button[1].x = x+5;
                        button[1].y = y_mid-2;
                        button[2].x = x;
                        button[2].y = y_mid+3;
                    }
                    else
                    {
                        button[0].y = y_mid-5;
                        button[0].x = x-2;
                        button[1].y = y_mid+5;
                        button[1].x = x-2;
                        button[2].y = y_mid;
                        button[2].x = x+3;
                    }
                    dc.DrawPolygon(3, button);
                    dc.SetPen(m_dottedPen);
                }
            }
            else // if (HasFlag(wxTR_HAS_BUTTONS))
            {
                // draw the plus sign here
                dc.SetPen(*wxGREY_PEN);
                dc.SetBrush(*wxWHITE_BRUSH);
                dc.DrawRectangle(x-5, y_mid-4, 11, 9);
                dc.SetPen(*wxBLACK_PEN);
                dc.DrawLine(x-2, y_mid, x+3, y_mid);
                if (!item->IsExpanded())
                    dc.DrawLine(x, y_mid-2, x, y_mid+3);
                dc.SetPen(m_dottedPen);
            }
        }
        else if (!HasFlag(wxTR_NO_LINES))  // no button; maybe a line?
        {
            // draw the horizontal line here
            int x_start = x;
            if (x > (signed)m_indent)
                x_start -= m_indent;
            else if (HasFlag(wxTR_LINES_AT_ROOT))
                x_start = 3;
            dc.DrawLine(x_start, y_mid, x + m_spacing, y_mid);
        }
    }

    if (item->IsExpanded())
    {
        wxArrayGenericTreeItems& children = item->GetChildren();
        int count = children.Count();
        if (count > 0)
        {
            int n = 0, oldY;
            ++level;
            do {
                oldY = y;
                PaintLevel(children[n], dc, level, y);
            } while (++n < count);

            if (!HasFlag(wxTR_NO_LINES) && count > 0)
            {
                // draw line down to last child
                oldY += GetLineHeight(children[n-1])>>1;
                if (HasButtons()) y_mid += 5;

                // Only draw the portion of the line that is visible, in case it is huge
                wxCoord	xOrigin=0, yOrigin=0, width, height;
                dc.GetDeviceOrigin(&xOrigin, &yOrigin);
                yOrigin = abs(yOrigin);
                GetClientSize(&width, &height);

                // Move end points to the begining/end of the view?
                if (y_mid < yOrigin)
                    y_mid = yOrigin;
                if (oldY > yOrigin + height)
                    oldY = yOrigin + height;

                // after the adjustments if y_mid is larger than oldY then the line
                // isn't visible at all so don't draw anything
                if (y_mid < oldY)
                    dc.DrawLine(x, y_mid, x, oldY);
            }
        }
    }
}

bool
wxGenericCTreeCtrl::SearchEventTable(wxEventTable& table, wxEvent& event)
{
#if 1
  return wxGenericTreeCtrl::SearchEventTable(table, event);
#else
  wxEventType eventType = event.GetEventType();
  int eventId = event.GetId();

  for (int i = 0; table.entries[i].m_fn != 0; ++i)
  {
    wxEventTableEntry const& entry(table.entries[i]);
    if (eventType == entry.m_eventType)
    {
      int tableId1 = entry.m_id;
      int tableId2 = entry.m_lastId;

      if ((tableId1 == -1) ||
          (tableId2 == -1 && eventId == tableId1) ||
	  (tableId2 != -1 && (eventId >= tableId1 && eventId <= tableId2)) )
      {
        event.Skip(FALSE);
	event.m_callbackUserData = entry.m_callbackUserData;
	(this->*((wxEventFunction) (entry.m_fn)))(event);
	return !event.GetSkipped();
      }
    }
  }

  return FALSE;
#endif
}

