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

#ifdef __GNUG__
#pragma implementation "linuxmon.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "linuxmon.h"

#include <wx/txtstrm.h>
#include <wx/wfstream.h>

// Constants
const wxFileName
LinuxMon::UPTIME_FILE ("/proc/uptime");

const wxFileName
LinuxMon::LOADAVG_FILE ("/proc/loadavg");

// Constructors
LinuxMon::LinuxMon ()
{
  m_uptime = _("Unknown");
  m_sysLoad_1 = 0.0;
  m_sysLoad_5 = 0.0;
  m_sysLoad_15 = 0.0;
}

// Destructor
LinuxMon::~LinuxMon ()
{
}

// Refresh
void
LinuxMon::Refresh ()
{
  wxFileInputStream upInput (UPTIME_FILE.GetFullPath ());
  wxFileInputStream loadInput (LOADAVG_FILE.GetFullPath ());

  wxTextInputStream upText (upInput);
  wxTextInputStream loadText (loadInput);

  loadText >> m_sysLoad_1;
  loadText >> m_sysLoad_5;
  loadText >> m_sysLoad_15;

  double uptime;
  upText >> uptime;

  wxInt32 day, hour, min, sec;
  day = (wxInt32) (uptime / 86400);
  hour = (wxInt32) ((uptime - day * 86400) / 3600);
  min = (wxInt32) ((uptime - day * 86400 - hour * 3600) / 60);
  sec = (wxInt32) (uptime - day * 86400 - hour * 3600 - min * 60);

  m_uptime =
    m_uptime.Format (_("%i day(s) %i hour(s) %i min %i s"), day, hour, min,
		     sec);
}

// Accessors
wxString
LinuxMon::GetUptime () const
{
  return m_uptime;
}

float
LinuxMon::GetSysLoad_1 () const
{
  return m_sysLoad_1;
}

float
LinuxMon::GetSysLoad_5 () const
{
  return m_sysLoad_5;
}

float
LinuxMon::GetSysLoad_15 () const
{
  return m_sysLoad_15;
}
