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

#ifdef __GNUG__
#pragma implementation "alcc.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP

#endif
#include <wx/log.h>
#include "alcc.h"

// Application implementation
IMPLEMENT_APP (alcc)

int alcc::OnRun ()
{
  static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
      {
        wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP
      },
      { wxCMD_LINE_SWITCH, _T("v"), _T("verbose"), _T("be verbose") },

      { wxCMD_LINE_SWITCH, wxT("p"), wxT("parthashes"), wxT("add part-hashes to ed2k link") },

      { wxCMD_LINE_PARAM,  NULL, NULL, wxT("input files"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE },

      { wxCMD_LINE_NONE }
    };

#if wxUSE_UNICODE

  wxChar **wargv = new wxChar *[argc + 1];


  for ( int n = 0; n < argc; n++ )
    {
      wxMB2WXbuf warg = wxConvertMB2WX(argv[n]);
      wargv[n] = wxStrdup(warg);
    }

  wargv[n] = NULL;

  wxCmdLineParser parser(cmdLineDesc, argc, wargv);

  for ( int n = 0; n < argc; n++ )
    free(wargv[n]);

  delete [] wargv;

#else

  wxCmdLineParser parser(cmdLineDesc, argc, argv);
#endif // wxUSE_UNICODE

  switch (parser.Parse())
    {
    case -1: // Exit after giving usage msg
      return 0;
      break;

    case 0: // Run
      return (computeEd2kLinks(parser));
      break;

    default: // Syntax error, exit after giving usage msg
      return 1;
      break;
    }
}


int alcc::computeEd2kLinks(const wxCmdLineParser& cmdline)
{
  wxLogError(wxT("Let me some time to implement it !"));
  return 0;
}
