//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Main wxBase App
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

#ifndef _ALCC_H
#define _ALCC_H

#ifdef __GNUG__
#pragma interface "alcc.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/cmdline.h>
#endif

/// Command line parameters
static const wxCmdLineEntryDesc cmdLineDesc[] =
  {
    {
      wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP
    },
    { wxCMD_LINE_SWITCH, wxT("v"), wxT("verbose"), wxT("be verbose"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL},

    { wxCMD_LINE_SWITCH, wxT("p"), wxT("parthashes"), wxT("add part-hashes to ed2k link"), wxCMD_LINE_VAL_NONE,wxCMD_LINE_PARAM_OPTIONAL },

    { wxCMD_LINE_PARAM,  NULL, NULL, wxT("input files"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE },

    { wxCMD_LINE_NONE }
  };


// Application
#if wxCHECK_VERSION(2,5,0)
class alcc : public wxAppConsole
#else
class alcc : public wxApp
#endif

  {
  private:
    bool m_flagVerbose ;
    bool m_flagPartHashes;
    wxArrayString m_filesToHash;

    /// Parse command line
    virtual void OnInitCmdLine(wxCmdLineParser& cmdline);

    /// Command line preocessing
    virtual bool OnCmdLineParsed(wxCmdLineParser& cmdline);

  public:
    virtual int OnRun ();
  };

DECLARE_APP(alcc);

#endif /* _ALCC_H */

