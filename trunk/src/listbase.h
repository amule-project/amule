//
// This file is part of the aMule Project.
//
// Copyright (c) 2003 aMule Team ( http://www.amule-project.net )
// Copyright (c) 1998 wxWindows team
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef LISTBASE_H
#define LISTBASE_H

#include <wx/defs.h>
#include <wx/listbase.h>

#define wxLC_OWNERDRAW 0x10000

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_MIDDLE_CLICK, 722)
END_DECLARE_EVENT_TYPES()

#define EVT_LIST_COL_MIDDLE_CLICK(id, fn) DECLARE_EVENT_TABLE_ENTRY( wxEVT_COMMAND_LIST_COL_MIDDLE_CLICK, id, -1, (wxObjectEventFunction) (wxEventFunction) (wxListEventFunction) & fn, NULL ),

#endif // LISTBASE_H
