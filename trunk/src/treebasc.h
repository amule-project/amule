// Definition of classes wxCTreeCtrl base classes and types
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

#ifndef TREEBASC_H
#define TREEBASC_H

#include <wx/window.h>
#include <wx/event.h>
#include <wx/treebase.h>

// Added wxCTreeCtrl flag
#define wxTR_CHECKBOX 0x1000				// aMule extension.
static int const wxTREE_HITTEST_CHECKBOX = 0x800000;	// aMule extension.

// A tree event object that tracks control and shift key.

class wxCTreeEvent : public wxTreeEvent {
public:
  wxCTreeEvent(wxEventType commandType = wxEVT_NULL, int id = 0) :
      wxTreeEvent(commandType, id), m_ctrlDown(false), m_shiftDown(false) { }

  bool IsControlDown(void) const {return m_ctrlDown; }
  bool IsShiftDown(void) const {return m_shiftDown; }

  void SetControlDown(bool ctrlDown) { m_ctrlDown = ctrlDown; }
  void SetShiftDown(bool shiftDown) { m_shiftDown = shiftDown; }

private:
  bool m_ctrlDown;
  bool m_shiftDown;

  //friend class WXDLLEXPORT wxCTreeCtrl;
  //friend class WXDLLEXPORT wxGenericCTreeCtrl;

  DECLARE_DYNAMIC_CLASS(wxCTreeEvent);
};

// Event extensions.

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_LEFT_CLICK, 618)		// aMule extension.
END_DECLARE_EVENT_TYPES()

// aMule extension.
#define EVT_TREE_ITEM_LEFT_CLICK(id,fn) \
    DECLARE_EVENT_TABLE_ENTRY(wxEVT_COMMAND_TREE_ITEM_LEFT_CLICK, id, -1, \
        (wxObjectEventFunction)(wxEventFunction)(wxTreeEventFunction)&fn, NULL),

#endif // TREEBASC_H
