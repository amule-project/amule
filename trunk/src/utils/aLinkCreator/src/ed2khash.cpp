//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Ed2kHash Class
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
#pragma implementation "ed2khash.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/ffile.h>
#include <wx/regex.h>
#endif

#include "ed2khash.h"
#include "md4.h"

/// Constructor
Ed2kHash::Ed2kHash():MD4()
{
  m_ed2kArrayOfHashes = 0;
  m_filename=wxEmptyString;
  m_fileSize=0;
}

/// Destructor
Ed2kHash::~Ed2kHash()
{}

/// Set Ed2k hash from a file
// returns false if aborted
bool Ed2kHash::SetED2KHashFromFile(const wxFileName& filename, MD4Hook hook)
{
  size_t read;
  unsigned char ret[MD4_HASHLEN_BYTE];
  MD4Context hdc;

  size_t partcount;
  size_t dataread;

  char *buf = new char[BUFSIZE];
  int n;
  bool keep_going;

#if WANT_STRING_IMPLEMENTATION

  wxString tmpHash(wxEmptyString);
#else

  unsigned char* tmpCharHash = NULL;
#endif

  // Open file and let wxFFile destructor close the file
  // Closing it explicitly may crash on Win32 ...
  wxFFile file(filename.GetFullPath(), wxT("rbS"));
  if (! file.IsOpened())
    {
      wxLogError (_("Unable to open %$"),filename.GetFullPath().c_str());
    }
  else if (file.Length() > (size_t)-1)
    {
      wxLogError (_("The file %s is to big for the Donkey: maximum allowed is 4 GB."),
                  filename.GetFullPath().c_str());
    }
  else
    {
      // Clear Ed2k Hash
      m_ed2kArrayOfHashes.Clear();

      // Processing each block
      n = 0;
      keep_going = true;
      partcount = 0;
      while (!file.Eof() && keep_going)
        {
          dataread = 0;
          MD4Init(&hdc);
          while (dataread < PARTSIZE && !file.Eof() && keep_going)
            {
              if (hook) {
                  keep_going = hook( (n * BUFSIZE)/file.Length() );
              }
	      if (keep_going) {
                if ((dataread + BUFSIZE) > PARTSIZE)
                  {
                    read = file.Read(buf, PARTSIZE - dataread);
                  }
                else
                  {
                    read = file.Read(buf, BUFSIZE);
                  }
                dataread += read;
                MD4Update(&hdc, reinterpret_cast<unsigned char const *>(buf),
                          read);
              }
            }
          MD4Final(&hdc, ret);

          // Add part-hash
          m_ed2kArrayOfHashes.Add(charToHex(reinterpret_cast<const char *>(ret),
                                            MD4_HASHLEN_BYTE));

          partcount++;

#if WANT_STRING_IMPLEMENTATION
          // MD4_HASHLEN_BYTE is ABSOLUTLY needed as we dont want NULL
          // character to be interpreted as the end of the parthash string
#if wxUSE_UNICODE

          tmpHash += wxString(reinterpret_cast<const wchar_t *>(ret),MD4_HASHLEN_BYTE);
#else

          tmpHash += wxString(reinterpret_cast<const char *>(ret),MD4_HASHLEN_BYTE);
#endif
#else

          tmpCharHash = (unsigned char*)realloc(tmpCharHash,
                                                sizeof(unsigned char) * (MD4_HASHLEN_BYTE * partcount));
          memcpy ( tmpCharHash + MD4_HASHLEN_BYTE * (partcount - 1), ret, MD4_HASHLEN_BYTE );
#endif

        }

      delete [] buf;

      // hash == hash of concatenned parthashes
      if (partcount > 1)
        {
          wxString finalHash;

#if WANT_STRING_IMPLEMENTATION

          finalHash=calcMd4FromString(tmpHash);
#else

          MD4Init(&hdc);
          MD4Update(&hdc, tmpCharHash, MD4_HASHLEN_BYTE * partcount);
          MD4Final(&hdc, ret);

          finalHash = charToHex(reinterpret_cast<const char *>(ret),
                                MD4_HASHLEN_BYTE);
#endif

          m_ed2kArrayOfHashes.Add(finalHash);
        }

#if !WANT_STRING_IMPLEMENTATION
      free(tmpCharHash);
      tmpCharHash=NULL;
#endif

      m_ed2kArrayOfHashes.Shrink();

      // Set members
      m_fileSize = file.Length();
      m_filename = filename.GetFullName();
    }

  return keep_going;
}

/// Set Ed2k hash from a file
bool Ed2kHash::SetED2KHashFromFile(const wxString& filename, MD4Hook hook)
{
  return SetED2KHashFromFile(wxFileName(filename), hook);
}

/// Get Ed2k link
wxString Ed2kHash::GetED2KLink(const wxArrayString& arrayOfUrls, const bool addPartHashes)
{
  // Constructing ed2k basic link
  wxString ed2kLink = wxT("ed2k://|file|")+CleanFilename(m_filename)
                      +wxT("|")+ wxString::Format(wxT("%u"),m_fileSize) +wxT("|")
                      + m_ed2kArrayOfHashes.Last() + wxT("|");


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
wxString Ed2kHash::CleanFilename(const wxString& filename)
{
  wxString name(filename);

  wxRegEx toStrip(wxT("[^[:alnum:]_.-]"));
  toStrip.Replace(&name, wxT("_"));

  return (name);
}

/// Get Ed2k Array of hashes
wxArrayString Ed2kHash::GetED2KHash()
{
  return (m_ed2kArrayOfHashes);
}
