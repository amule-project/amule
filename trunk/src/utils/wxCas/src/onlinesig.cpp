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

#include <wx/txtstrm.h>
#include <wx/wfstream.h>

// Constructors
OnLineSig::OnLineSig ()
{
  m_isRunning = "0";
  m_serverName = "Unknown";
  m_serverIP = "0.0.0.0";
  m_serverPort = "00";
  m_connexionID = "?";
  m_DLRate = "0.00";
  m_ULRate = "0.00";
  m_queue = "0";
  m_sharedFiles = "0";
  m_user = "Unknown";
  m_totalDL = "0";
  m_totalUL = "0";
  m_version = "0";
  m_sessionDL = "0";
  m_sessionUL = "0";
  m_runTime = "0";
  
  m_maxDL = 0.0;

  m_amulesig = new wxFileName (wxFileName::GetHomeDir (), "amulesig.dat");
  m_amulesig->AppendDir (".aMule");
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
  text.SetStringSeparators ("\n");

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
  m_DLRate.ToDouble(&dl);
  
#ifdef __GNUG__
  m_maxDL = m_maxDL >? dl;
#else
  if (dl > m_maxDL)
    {
      m_maxDL = dl;
    }
#endif
}

bool OnLineSig::IsRunning ()
{
  if (m_isRunning == "1")
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

wxString OnLineSig::GetServerName ()
{
  return m_serverName;
}

wxString OnLineSig::GetServerIP ()
{
  return m_serverIP;
}

wxString OnLineSig::GetServerPort ()
{
  return m_serverPort;
}

wxString OnLineSig::GetConnexionID ()
{
  return m_connexionID;
}

wxString OnLineSig::GetULRate ()
{
  return m_ULRate;
}

wxString OnLineSig::GetDLRate ()
{
  return m_DLRate;
}

wxString OnLineSig::GetQueue ()
{
  return m_queue;
}

wxString OnLineSig::GetSharedFiles ()
{
  return m_sharedFiles;
}

wxString OnLineSig::GetUser ()
{
  return m_user;
}

wxString OnLineSig::GetTotalUL ()
{
  return m_totalUL;
}


wxString OnLineSig::GetTotalDL ()
{
  return m_totalDL;
}

wxString OnLineSig::GetVersion ()
{
  return m_version;
}

wxString OnLineSig::GetSessionUL ()
{
  return m_sessionUL;
}


wxString OnLineSig::GetSessionDL ()
{
  return m_sessionDL;
}

wxString OnLineSig::GetRunTime ()
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

wxString OnLineSig::GetConnexionIDType ()
{
  if (m_connexionID == "H")
    {
      return (wxString ("HighID"));
    }
  else
    {
      return (wxString ("LowID"));
    }
}

wxString OnLineSig::GetMaxDL ()
{
  return (wxString::Format(_("%.2f kB/s"),m_maxDL));
}

// Private use
wxString OnLineSig::BytesConvertion (wxString bytes)
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
      c_bytes = wxString::Format ("%.0f B", d_bytes);
      break;
    case 1:
      c_bytes = wxString::Format ("%.2f KB", d_bytes);
      break;
    case 2:
      c_bytes = wxString::Format ("%.2f MB", d_bytes);
      break;
    case 3:
      c_bytes = wxString::Format ("%.2f GB", d_bytes);
      break;
    default:
      c_bytes = wxString::Format ("%.2f TB", d_bytes);
      break;
    }
  return c_bytes;
}
