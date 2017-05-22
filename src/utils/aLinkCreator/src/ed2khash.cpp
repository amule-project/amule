////////////////////////////////////////////////////////////////////////////////
/// Name:         Ed2kHash Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
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


#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/regex.h>

#include "ed2khash.h"


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
	  // This doesn't make much sense to me, but it is what it was before and actually works.
	  wxLogError(_("Unable to open %s"), ((const char*)filename.GetFullPath().mb_str(wxConvISO8859_1)));
      return (false);
    }
  else
    {
      unsigned char ret[MD4_HASHLEN_BYTE];
      MD4Context hdc;

      size_t read;
      size_t partcount;
      size_t dataread;
      wxFileOffset totalread;

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
                  goAhead = hook((int)((double)(100.0 * totalread) / file.Length()));
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
		  delete [] buf;
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

          unsigned char *tmpPtr = (unsigned char*)realloc(tmpCharHash,
                                                sizeof(unsigned char) * (MD4_HASHLEN_BYTE * partcount));
	  if (tmpPtr) {
		  tmpCharHash = tmpPtr;
	  } else {
		  delete [] buf;
		  free(tmpCharHash);
		  wxLogError(_("Out of memory while calculating ed2k hash!"));
		  return (false);
	  }
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

#if wxCHECK_VERSION(2, 9, 0)
#define WXLONGLONGFMTSPEC wxT(wxLongLongFmtSpec)
#else
#define WXLONGLONGFMTSPEC wxLongLongFmtSpec
#endif

/// Get Ed2k link
wxString Ed2kHash::GetED2KLink(const bool addPartHashes, const wxArrayString* arrayOfUrls)
{
  // Constructing ed2k basic link
  wxString ed2kLink = wxT("ed2k://|file|") + CleanFilename(m_filename)
                      + wxString::Format(wxT("|%") WXLONGLONGFMTSPEC wxT("u|"), m_fileSize)
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
