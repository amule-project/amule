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
#pragma implementation "alchash.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/file.h>
#include <wx/regex.h>

#include "alchash.h"
#include "md4.h"

/// Constructor
AlcHash::AlcHash()
{
  m_ed2kArrayOfHashes = 0;
  m_filename=wxEmptyString;
  m_fileSize=0;
}

/// Destructor
AlcHash::~AlcHash()
{}

/// Set Ed2k hash from a file
void AlcHash::SetED2KHashFromFile(const wxFileName& filename)
{
  // Check file (wxFile destructor will close the file)
  wxFile file(filename.GetFullPath());
  if (! file.IsOpened())
    {
      wxLogError (_("Unable to open %$"),filename.GetFullPath().c_str());
    }
  // Check for file size
  else if (file.Length() > (size_t)-1)
    {
      wxLogError (_("The file %s is to big for the Donkey: maximum allowed is 4 GB."), filename.GetFullPath().c_str());
    }
  else
    {
      MD4 md4;

      // Set members
      m_fileSize = file.Length();
      m_ed2kArrayOfHashes=md4.calcEd2kFromFile(filename.GetFullPath());
      m_filename = filename.GetFullName();
    }
}

/// Set Ed2k hash from a file
void AlcHash::SetED2KHashFromFile(const wxString& filename)
{
  SetED2KHashFromFile(wxFileName(filename));
}

/// Get Ed2k link
wxString AlcHash::GetED2KLink(const wxArrayString& arrayOfUrls, const bool addPartHashes)
{
  // Constructing ed2k basic link
  wxString ed2kLink = wxT("ed2k://|file|")+CleanFilename(m_filename)
                      +wxT("|")+ wxString::Format(wxT("%u"),m_fileSize) +wxT("|")+ m_ed2kArrayOfHashes.Last() + wxT("|");


  // Add optional URLs
  if (!arrayOfUrls.IsEmpty())
    {
      size_t i;
      for (i=0;i<arrayOfUrls.GetCount();i++)
        {
          ed2kLink += wxT("s=") + arrayOfUrls[i] + wxT("|");
        }
    }

  // Add Optional part-hashes
  if (addPartHashes && m_ed2kArrayOfHashes.GetCount()>1)
    {
      ed2kLink += wxT("p=");
      size_t i;
      for (i=0;i<(m_ed2kArrayOfHashes.GetCount()-1);i++)
        {
          ed2kLink += m_ed2kArrayOfHashes[i] + wxT(":");
        }
      ed2kLink.RemoveLast(); // Remove last :
      ed2kLink += wxT("|");
    }

  // Add last slash
  ed2kLink += wxT("/");

  return ed2kLink;
}


/// Strip all non-alphanumeric characters of a filename string
wxString AlcHash::CleanFilename(const wxString& filename)
{
  wxString name(filename);

  wxRegEx toStrip(wxT("[^[:alnum:]_.-]"));
  toStrip.Replace(&name, wxT("_"));

  return (name);
}

/// Get Ed2k Array of hashes
wxArrayString AlcHash::GetED2KHash()
{
  return (m_ed2kArrayOfHashes);
}
