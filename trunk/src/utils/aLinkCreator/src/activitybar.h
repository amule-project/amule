//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         ActivityBar Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Pixmaps from http://www.everaldo.com and http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ACTIVITYBAR_H
#define _ACTIVITYBAR_H

#ifdef __GNUG__
#pragma interface "activitybar.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/gauge.h>
#endif

class ActivityBar:public wxGauge
  {
  private:

    wxTimer *m_timer;
    int m_speed;
    int m_range;
    bool m_goUp;
    int m_step;

    enum
    {
      ID_TIMER = 1000
    };

  protected:

    /// Update gauge on Timer event and switch direction at begining or end
    void OnTimer (wxTimerEvent & event);

    DECLARE_EVENT_TABLE ();

  public:

    /// Constructor
    ActivityBar(wxWindow* parent, wxWindowID id, int range, int speed,
                const wxPoint&  pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = wxGA_HORIZONTAL, const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxT("activitybar"));

    /// Destructor
    ~ActivityBar();

    /// Start the activity bar
    void Start();

    /// Stop the activity bar
    void Stop();
  };

#endif /* _ACTIVITYBAR_H */
