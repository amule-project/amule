////////////////////////////////////////////////////////////////////////////////
/// Name:         Ed2kHash Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Copyright (C) 2004 by Phoenix
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
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
////////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/ffile.h>
#include <wx/regex.h>

#include "ed2khash.h"


// efe, sorry for that, i have not enough time to do the right thing now, but 
// please, create a file called like unicodestuff.h and put this. Include in 
// alcc.c and here. And remove this stupid comment :)
//-----------------------------------------------------------------------------
// efe, this can be put in a separete include file, if you want to reuse
static wxCSConv aMuleConv(wxT("iso8859-1"));
#ifdef wxUSE_UNICODE
        #define unicode2char(x) (const char*) aMuleConv.cWX2MB(x)
        #define char2unicode(x) aMuleConv.cMB2WX(x)
#else
        #define unicode2char(x) x.c_str()
        #define char2unicode(x) x
#endif
//-----------------------------------------------------------------------------


/// Constructor
Ed2kHash::Ed2kHash():MD4()
{
  m_ed2kArrayOfHashes.Clear();
  m_filename.Clear();
  m_fileSize=0;
}

/// Destructor
Ed2kHash::~Ed2kHash()
{}

/// Set Ed2k hash from a file
// returns false if aborted
bool Ed2kHash::SetED2KHashFromFile(const wxFileName& filename, MD4Hook hook)
{
  // Open file and let wxFFile destructor close the file
  // Closing it explicitly may crash on Win32 ...
  wxFFile file(filename.GetFullPath(), wxT("rbS"));
  if (! file.IsOpened())
    {
      wxLogError (_("Unable to open %s"),unicode2char(filename.GetFullPath()));
      return (false);
    }
  else if (file.Length() > (size_t)-1)
    {
      wxLogError (_("The file %s is to big for the Donkey: maximum allowed is 4 GB."),
                  unicode2char(filename.GetFullPath()));
      return (false);
    }
  else
    {
      unsigned char ret[MD4_HASHLEN_BYTE];
      MD4Context hdc;

      size_t read;
      size_t partcount;
      size_t dataread;
      size_t totalread;

      char *buf = new char[BUFSIZE];

      bool goAhead = true;

#ifdef WANT_STRING_IMPLEMENTATION

      wxString tmpHash(wxEmptyString);
#else

      unsigned char* tmpCharHash = NULL;
#endif
      // Clear Ed2k Hash
      m_ed2kArrayOfHashes.Clear();

      // Processing each block
      totalread=0;
      partcount = 0;
      while (!file.Eof())
        {
          dataread = 0;
          MD4Init(&hdc);
          while (dataread < PARTSIZE && !file.Eof())
            {
              if (hook)
                {
                  goAhead = hook( (int)((double)(100.0 * totalread) / file.Length()));
                }
              if (goAhead)
                {
                  if ((dataread + BUFSIZE) > PARTSIZE)
                    {
                      read = file.Read(buf, PARTSIZE - dataread);
                    }
                  else
                    {
                      read = file.Read(buf, BUFSIZE);
                    }
                  dataread += read;
                  totalread += read;
                  MD4Update(&hdc, reinterpret_cast<unsigned char const *>(buf),
                            read);
                }
              else
                {
                  return (false);
                }

            }
          MD4Final(&hdc, ret);

          // Add part-hash
          m_ed2kArrayOfHashes.Add(charToHex(reinterpret_cast<const char *>(ret),
                                            MD4_HASHLEN_BYTE));

          partcount++;

#ifdef WANT_STRING_IMPLEMENTATION
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

#ifdef WANT_STRING_IMPLEMENTATION

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

#ifndef WANT_STRING_IMPLEMENTATION
      free(tmpCharHash);
      tmpCharHash=NULL;
#endif

      m_ed2kArrayOfHashes.Shrink();

      // Set members
      m_fileSize = file.Length();
      m_filename = filename.GetFullName();

      return true;
    }
}

/// Set Ed2k hash from a file
bool Ed2kHash::SetED2KHashFromFile(const wxString& filename, MD4Hook hook)
{
  return SetED2KHashFromFile(wxFileName(filename), hook);
}

/// Get Ed2k link
wxString Ed2kHash::GetED2KLink(const bool addPartHashes, const wxArrayString* arrayOfUrls)
{
  // Constructing ed2k basic link
  wxString ed2kLink = wxT("ed2k://|file|")+CleanFilename(m_filename)
                      +wxT("|")+ wxString::Format(wxT("%u"),m_fileSize) +wxT("|")
                      + m_ed2kArrayOfHashes.Last() + wxT("|");


  // Add optional URLs
  if ( arrayOfUrls && !arrayOfUrls->IsEmpty())
    {
      size_t i;
      for ( i = 0; i < arrayOfUrls->GetCount(); i++ ) 
        {
          ed2kLink += wxT("s=") + (*arrayOfUrls)[i] + wxT("|");
        }
    }

  // Add Optional part-hashes
  if (addPartHashes && m_ed2kArrayOfHashes.GetCount()>1)
    {
      ed2kLink += wxT("p=");
      size_t i;
      for (i=0;i<(m_ed2kArrayOfHashes.GetCount()-1);++i)
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
// File_checked_for_headers
