// This file is part of the aMule project.
// 
// Copyright (C) 2004 aMule Team ( http://www.amule-project.net )
// Copyright (c) 1998, Robert Roebling, Julian Smart et al
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

// -----------------------------------------------------------------------------
// headers
// -----------------------------------------------------------------------------


// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_TREECTRL

#include "treebasc.h"
#include "wx/settings.h"
#include "wx/log.h"
#include "wx/intl.h"
#include "wx/dynarray.h"
#include "wx/arrimpl.cpp"
#include "wx/dcclient.h"
#include "wx/msgdlg.h"

// ----------------------------------------------------------------------------
// events
// ----------------------------------------------------------------------------

DEFINE_EVENT_TYPE(wxEVT_COMMAND_TREE_ITEM_LEFT_CLICK)	// aMule extension.

// ----------------------------------------------------------------------------
// Tree event
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxCTreeEvent, wxNotifyEvent)

#endif // wxUSE_TREE_CTRL

