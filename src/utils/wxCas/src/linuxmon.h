//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wxCas
// Purpose:    Display aMule Online Statistics
// Author:       ThePolish <thepolish@vipmail.ru>
// Copyright (C) 2004 by ThePolish
//
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
//
// This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _LINUXMON_H
#define _LINUXMON_H

#ifdef __GNUG__
#pragma interface "linuxmon.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>

// LinuxMon Class
class LinuxMon
  {
  private:
    wxString m_uptime;

    float m_sysLoad_1;
    float m_sysLoad_5;
    float m_sysLoad_15;

    static const wxFileName UPTIME_FILE;
    static const wxFileName LOADAVG_FILE;

  public:
    // Constructor
    LinuxMon ();

    // Destructor
    ~LinuxMon ();

    // Accessors
    void Refresh ();
    wxString GetUptime () const;
    float GetSysLoad_1 () const;
    float GetSysLoad_5 () const;
    float GetSysLoad_15 () const;
  };

#endif /* _LINUXMON_H */
