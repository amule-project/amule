//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         AlcHash Class
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
///  it under the terms of the GNU General Public License as published by
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
#pragma implementation "alchash.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/ffile.h>
#include <wx/regex.h>

#include "alchash.h"
#include "md4.h"

/// Constructor
AlcHash::AlcHash()
{}

/// Destructor
AlcHash::~AlcHash()
{}

/// Get Md4 hash from a file
wxString AlcHash::GetMD4HashFromFile (const wxFileName& filename)
{
  return (GetMD4HashFromFile(filename.GetFullPath()));
}

/// Get Md4 hash from a file
wxString AlcHash::GetMD4HashFromFile(const wxString& filename)
{
  MD4 md4;
  return (md4.calcMd4FromFile(filename));
}

/// Get Ed2k hash from a file
wxString AlcHash::GetED2KHashFromFile(const wxFileName& filename)
{
  return (GetED2KHashFromFile(filename.GetFullPath()));
}

/// Get Ed2k hash from a file
wxString AlcHash::GetED2KHashFromFile(const wxString& filename)
{
  MD4 md4;
  return (md4.calcEd2kFromFile(filename));
}

/// Get Ed2k link from a file
wxString AlcHash::GetED2KLinkFromFile(const wxFileName& filename, const wxString& ed2kHash)
{
  // Check file
  wxFFile file(filename.GetFullPath());
  if (! file.IsOpened())
    {
      return (_("Unable to open ")+filename.GetFullPath());
    }

  size_t fileSize = file.Length();
  file.Close();

  // Check for file size
  if (fileSize > (size_t)-1)
    {
      return (_("The file ") + filename.GetFullPath() +
              _(" is to big for the Donkey: maximum allowed is 4 GB."));
    }

  // Compute ed2k hash if not provided
  wxString hash(ed2kHash);
  if (hash.IsEmpty())
    {
      hash = GetED2KHashFromFile(filename);
    }

  // Constructing ed2k link
  wxString ed2kLink = wxT("ed2k://|file|")+CleanFilename(filename.GetFullName())
                      +wxT("|")+ wxString::Format(wxT("%u"),fileSize) +wxT("|")+ hash + wxT("|");

  return ed2kLink;
}

/// Get Ed2k link from a file
wxString AlcHash::GetED2KLinkFromFile(const wxString& filename, const wxString& ed2kHash)
{
  return (GetED2KLinkFromFile(wxFileName(filename), ed2kHash));
}

/// Strip all non-alphanumeric characters of a filename string
wxString AlcHash::CleanFilename(const wxString& filename)
{
  wxString name(filename);

  wxRegEx toStrip(wxT("[^[:alnum:]_.-]"));
  toStrip.Replace(&name, wxT("_"));

  return (name);
}
