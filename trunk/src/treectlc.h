// Definition of classes wxGenericCTreeCtrl and wxCTreeCtrl.
// This file is part of the aMule project.
//
// Copyright (c) 2004 aMule Team ( http://www.amule-project.net )
// Copyright (c) 1997, 1998, Robert Roebling
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


#ifndef TREECTLC_H
#define TREECTLC_H

#include <wx/setup.h>
#include <wx/treebase.h>
#include <wx/generic/treectlg.h>
#include <wx/bitmap.h>
#include <wx/brush.h>

#if (wxMINOR_VERSION < 5)

// -----------------------------------------------------------------------------
// forward declaration
// -----------------------------------------------------------------------------

class wxGenericCTreeItem;

// -----------------------------------------------------------------------------
// wxGenericCTreeCtrl - tree control with aMule extension.
// -----------------------------------------------------------------------------

class wxGenericCTreeCtrl : public wxGenericTreeCtrl {
public:
  // Constructors
  wxGenericCTreeCtrl(void) { Init(); }
  wxGenericCTreeCtrl(wxWindow* parent, wxWindowID id = -1,
    wxPoint const& pos = wxDefaultPosition, wxSize const& size = wxDefaultSize,
    long style = wxTR_DEFAULT_STYLE, wxValidator const& validator = wxDefaultValidator,
    wxString const& name = wxTreeCtrlNameStr) :
    wxGenericTreeCtrl(parent, id, pos, size, style, validator, name) { Init(); }
  virtual ~wxGenericCTreeCtrl() { delete m_check1; delete m_check2; }

  void SetChecked(wxTreeItemId const& item, bool mode);
  bool IsChecked(wxTreeItemId const& item) const;

  wxTreeItemId HitTest(wxPoint const& point)
      { int dummy; return HitTest(point, dummy); }
  wxTreeItemId HitTest(wxPoint const& point, int& flags);

  // Overridden methods.
  virtual bool SearchEventTable(wxEventTable& table, wxEvent& event);
  void OnPaint(wxPaintEvent&);
  void PaintLevel(wxGenericCTreeItem*, wxDC&, int, int&);
  void PaintItem(wxGenericCTreeItem* item, wxDC& dc);
  void OnMouse(wxMouseEvent& event);
  wxTreeItemId AppendItem(wxTreeItemId const&, wxString const&, int image = -1, int selImage = -1, wxTreeItemData* data = NULL);
  wxTreeItemId DoInsertItem(wxTreeItemId const&, size_t previous, wxString const&, int, int, wxTreeItemData*);
  wxTreeItemId AddRoot(wxString const& text, int image = -1, int selImage = -1, wxTreeItemData* data = NULL);

protected:
  wxBitmap* m_check1;
  wxBitmap* m_check2;

  void Init(void);

public:
  // These are called by wxGenericCTreeItem::HitTest.
  using wxGenericTreeCtrl::GetLineHeight;
  using wxGenericTreeCtrl::HasButtons;
  // Used by wxGenericCTreeItem::HitTest.
  using wxGenericTreeCtrl::m_imageListNormal;

private:
  DECLARE_EVENT_TABLE()
  DECLARE_DYNAMIC_CLASS(wxGenericCTreeCtrl)
};

class WXDLLEXPORT wxCTreeCtrl: public wxGenericCTreeCtrl {
public:
  wxCTreeCtrl(void) { }
  wxCTreeCtrl(wxWindow* parent, wxWindowID id = -1, wxPoint const& pos = wxDefaultPosition,
      wxSize const& size = wxDefaultSize, long style = wxTR_DEFAULT_STYLE,
      wxValidator const& validator = wxDefaultValidator, wxString const& name = wxTreeCtrlNameStr) :
      wxGenericCTreeCtrl(parent, id, pos, size, style, validator, name) { }

private:
  DECLARE_DYNAMIC_CLASS(wxCTreeCtrl)
};

#endif // wx 2.5

#endif // TREECTLC_H
