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
#pragma implementation "onlinesig.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "onlinesig.h"
#include "wxcascte.h"

#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/config.h>

// Constructors
OnLineSig::OnLineSig ()
{

  m_isRunning = _("0");
  m_serverName = _("Unknown");
  m_serverIP = _("0.0.0.0");
  m_serverPort = _("00");
  m_connexionID = _("?");
  m_DLRate = _("0.00");
  m_ULRate = _("0.00");
  m_queue = _("0");
  m_sharedFiles = _("0");
  m_user = _("Unknown");
  m_totalDL = _("0");
  m_totalUL = _("0");
  m_version = _("0");
  m_sessionDL = _("0");
  m_sessionUL = _("0");
  m_runTime = _("0");

  m_maxDL = 0.0;

  m_amulesig =
    new wxFileName (wxConfigBase::Get()->
                    Read (WxCasCte::AMULESIG_DIR_KEY,
                          WxCasCte::DEFAULT_AMULESIG_PATH),
                    WxCasCte::AMULESIG_FILENAME);
}

// Constructor 2
OnLineSig::OnLineSig (wxFileName * file)
{
  m_amulesig = file;
  Refresh ();
}

// Destructor
OnLineSig::~OnLineSig ()
{
  delete m_amulesig;
}

// Accessors
void
OnLineSig::SetAmuleSig (wxFileName * file)
{
  m_amulesig = file;
  Refresh ();
}

void
OnLineSig::Refresh ()
{
  wxFileInputStream input (m_amulesig->GetFullPath ());

  wxTextInputStream text (input);
  text.SetStringSeparators (wxT("\n"));

  text >> m_isRunning;
  text >> m_serverName;
  text >> m_serverIP;
  text >> m_serverPort;
  text >> m_connexionID;
  text >> m_DLRate;
  text >> m_ULRate;
  text >> m_queue;
  text >> m_sharedFiles;
  text >> m_user;
  text >> m_totalDL;
  text >> m_totalUL;
  text >> m_version;
  text >> m_sessionDL;
  text >> m_sessionUL;
  text >> m_runTime;

  double dl;
  m_DLRate.ToDouble (&dl);

#ifdef __GNUG__

  m_maxDL = m_maxDL >? dl;
#else

  if (dl > m_maxDL)
    {
      m_maxDL = dl;
    }
#endif
}

bool OnLineSig::IsRunning () const
  {
    if (m_isRunning == wxT("1"))
      {
        return TRUE;
      }
    else
      {
        return FALSE;
      }
  }

wxString OnLineSig::GetServerName () const
  {
    return m_serverName;
  }

wxString OnLineSig::GetServerIP () const
  {
    return m_serverIP;
  }

wxString OnLineSig::GetServerPort () const
  {
    return m_serverPort;
  }

wxString OnLineSig::GetConnexionID () const
  {
    return m_connexionID;
  }

wxString OnLineSig::GetULRate () const
  {
    return m_ULRate;
  }

wxString OnLineSig::GetDLRate () const
  {
    return m_DLRate;
  }

wxString OnLineSig::GetQueue () const
  {
    return m_queue;
  }

wxString OnLineSig::GetSharedFiles () const
  {
    return m_sharedFiles;
  }

wxString OnLineSig::GetUser () const
  {
    return m_user;
  }

wxString OnLineSig::GetTotalUL () const
  {
    return m_totalUL;
  }


wxString OnLineSig::GetTotalDL () const
  {
    return m_totalDL;
  }

wxString OnLineSig::GetVersion () const
  {
    return m_version;
  }

wxString OnLineSig::GetSessionUL () const
  {
    return m_sessionUL;
  }

wxString OnLineSig::GetSessionDL () const
  {
    return m_sessionDL;
  }

wxString OnLineSig::GetRunTime () const
  {
    return m_runTime;
  }

wxString OnLineSig::GetConvertedTotalUL ()
{
  return (BytesConvertion (m_totalUL));
}

wxString OnLineSig::GetConvertedTotalDL ()
{
  return (BytesConvertion (m_totalDL));
}

wxString OnLineSig::GetConvertedSessionUL ()
{
  return (BytesConvertion (m_sessionUL));
}

wxString OnLineSig::GetConvertedSessionDL ()
{
  return (BytesConvertion (m_sessionDL));
}

wxString OnLineSig::GetConnexionIDType () const
  {
    if (m_connexionID == wxT("H"))
      {
        return (wxString (wxT("HighID")));
      }
    else
      {
        return (wxString (wxT("LowID")));
      }
  }

wxString OnLineSig::GetMaxDL () const
  {
    return (wxString::Format (_("%.2f kB/s"), m_maxDL));
  }

// Private use
wxString OnLineSig::BytesConvertion (const wxString & bytes)
{
  double
  d_bytes;
  wxString
  c_bytes;

  bytes.ToDouble (&d_bytes);

  wxInt32
  i = 0;
  while (d_bytes > 1024)
    {
      d_bytes /= 1024;
      i++;
    }

  switch (i)
    {
    case 0:
        c_bytes = wxString::Format (wxT("%.0f B"), d_bytes);
      break;
    case 1:
      c_bytes = wxString::Format (wxT("%.2f KB"), d_bytes);
      break;
    case 2:
      c_bytes = wxString::Format (wxT("%.2f MB"), d_bytes);
      break;
    case 3:
      c_bytes = wxString::Format (wxT("%.2f GB"), d_bytes);
      break;
    default:
      c_bytes = wxString::Format (wxT("%.2f TB"), d_bytes);
      break;
    }
  return c_bytes;
}
