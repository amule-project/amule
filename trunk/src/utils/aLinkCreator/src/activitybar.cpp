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

#ifdef __GNUG__
#pragma implementation "activitybar.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "activitybar.h"

/// Constructor
ActivityBar::ActivityBar (wxWindow* parent, wxWindowID id, int range, int speed,
                          const wxPoint&  pos, const wxSize& size, long style, const wxValidator& validator,
                          const wxString& name):wxGauge(parent, id, range, pos, size, style, validator, name)
{
  m_timer = new wxTimer (this, ID_TIMER);
  m_speed = speed;
  m_range = range;
  m_goUp = TRUE;
  m_step = 0;
}

/// Destructor
ActivityBar::~ActivityBar ()
{}

/// Events table
BEGIN_EVENT_TABLE (ActivityBar, wxGauge)
EVT_TIMER (ID_TIMER, ActivityBar::OnTimer)
END_EVENT_TABLE ()

/// Update gauge on Timer event and switch direction at begining or end
void ActivityBar::OnTimer (wxTimerEvent & event)
{
  if (m_goUp)
    {
      m_step ++;
      if (m_step == m_range)
        {
          m_goUp=FALSE;
        }

      this->SetValue(m_step);
    }
  else
    {
      m_step --;
      if (m_step == 0)
        {
          m_goUp=TRUE;
        }

      this->SetValue(m_step);
    }
}

/// Start the activity bar
void ActivityBar::Start()
{
  m_goUp = TRUE;
  m_step = 0;
  this->SetValue(m_step);
  m_timer->Start(m_speed);
}

/// Stop the activity bar
void ActivityBar::Stop()
{
  m_timer->Stop();
  m_goUp = TRUE;
  m_step = 0;
  this->SetValue(m_step);
}
