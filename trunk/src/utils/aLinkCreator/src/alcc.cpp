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
#include <wx/log.h>
#include <wx/filename.h>
#endif

#include "alcc.h"
#include "ed2khash.h"

// Application implementation
IMPLEMENT_APP (alcc)

int alcc::OnRun ()
{
  static const wxCmdLineEntryDesc cmdLineDesc[] =
    {
      {
        wxCMD_LINE_SWITCH, wxT("h"), wxT("help"), wxT("show this help message"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP
      },
      
      { wxCMD_LINE_SWITCH, wxT("p"), wxT("parthashes"), wxT("add part-hashes to ed2k link"), wxCMD_LINE_VAL_NONE,wxCMD_LINE_PARAM_OPTIONAL },

      { wxCMD_LINE_PARAM,  NULL, NULL, wxT("input files"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE },

      { wxCMD_LINE_NONE }
    };

  wxCmdLineParser parser(cmdLineDesc, argc, argv);

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
  wxFileName filename;
  size_t i;

  bool flagPartHashes = cmdline.Found(_T("p"));

  for (i = 0; i < cmdline.GetParamCount(); i++)
    {
      filename.Assign(cmdline.GetParam(i));
      Ed2kHash hash;

      if (hash.SetED2KHashFromFile(filename, NULL))
        {
          printf (wxT("%s ---> %s\n\n"),filename.GetFullName().c_str(),
                  hash.GetED2KLink(flagPartHashes).c_str());
        }
    }
  return 0;
}
