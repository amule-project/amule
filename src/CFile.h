/////////////////////////////////////////////////////////////////////////////
// Name:        file.h
// Purpose:     CFile - encapsulates low-level "file descriptor"
//              wxTempFile - safely replace the old file
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

#ifndef CFILE_H
#define CFILE_H

#include <wx/string.h>		// Needed for wxString
#include <wx/filefn.h>		// Needed for wxSeekMode and seek related stuff.

#include "CString.h"		// Needed for CString


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// we redefine these constants here because S_IREAD &c are _not_ standard
// however, we do assume that the values correspond to the Unix umask bits
#define wxS_IRUSR 00400
#define wxS_IWUSR 00200
#define wxS_IXUSR 00100

#define wxS_IRGRP 00040
#define wxS_IWGRP 00020
#define wxS_IXGRP 00010

#define wxS_IROTH 00004
#define wxS_IWOTH 00002
#define wxS_IXOTH 00001

// default mode for the new files: corresponds to umask 022
#define wxS_DEFAULT   (wxS_IRUSR | wxS_IWUSR | wxS_IRGRP | wxS_IWGRP |\
                       wxS_IROTH | wxS_IWOTH)

// ----------------------------------------------------------------------------
// class CFile: raw file IO
//
// NB: for space efficiency this class has no virtual functions, including
//     dtor which is _not_ virtual, so it shouldn't be used as a base class.
// ----------------------------------------------------------------------------
class CFile {
public:
  // more file constants
  // -------------------
    // opening mode
  enum OpenMode { read, write, read_write, write_append, write_excl };
    // standard values for file descriptor
  enum { fd_invalid = -1, fd_stdin, fd_stdout, fd_stderr };

  // static functions
  // ----------------
    // check whether a regular file by this name exists
  static bool Exists(const wxChar *name);
    // check whetther we can access the given file in given mode
    // (only read and write make sense here)
  static bool Access(const wxChar *name, OpenMode mode);

  // ctors
  // -----
    // def ctor
  CFile() { m_fd = fd_invalid; }
    // open specified file (may fail, use IsOpened())
  CFile(const wxChar *szFileName, OpenMode mode = read);
    // attach to (already opened) file
  CFile(int fd) { m_fd = fd; }

  wxString GetFilePath() {return fFilePath;}; 

  // open/close
    // create a new file (with the default value of bOverwrite, it will fail if
    // the file already exists, otherwise it will overwrite it and succeed)
  virtual bool Create(const wxChar *szFileName, bool bOverwrite = FALSE,
              int access = wxS_DEFAULT);
  virtual bool Open(const wxChar *szFileName, OpenMode mode = read,
            int access = wxS_DEFAULT);
  // Kry -Added for windoze compatibility.
  off_t GetLength( ) { return Length(); }
  
  bool Open(CString szFileName, OpenMode mode = read, int access = wxS_DEFAULT) {
    return Open(szFileName.GetData(),mode,access);
  };
  virtual bool Close();  // Close is a NOP if not opened

  // assign an existing file descriptor and get it back from CFile object
  void Attach(int fd) { Close(); m_fd = fd; }
  void Detach()       { m_fd = fd_invalid;  }
  int  fd() const { return m_fd; }

  // read/write (unbuffered)
    // returns number of bytes read or ofsInvalid on error
  virtual off_t Read(void *pBuf, off_t nCount);
    // returns the number of bytes written
  virtual size_t Write(const void *pBuf, size_t nCount);
    // returns true on success
  virtual bool Write(const wxString& s, wxMBConv& conv = wxConvLocal)
  {
      const wxWX2MBbuf buf = s.mb_str(conv);
      size_t size = strlen(buf);
      return Write((const char *) buf, size) == size;
  }
    // flush data not yet written
  virtual bool Flush();

  // file pointer operations (return ofsInvalid on failure)
    // move ptr ofs bytes related to start/current off_t/end of file
  virtual off_t Seek(off_t ofs, wxSeekMode mode = wxFromStart);
    // move ptr to ofs bytes before the end
  virtual off_t SeekEnd(off_t ofs = 0) { return Seek(ofs, wxFromEnd); }
    // get current off_t
  virtual off_t Tell() const;
    // get current file length
  virtual off_t Length() const;
    //Truncate/grow file
  bool SetLength(off_t new_len);

  // simple accessors
    // is file opened?
  bool IsOpened() const { return m_fd != fd_invalid; }
    // is end of file reached?
  bool Eof() const;
    // has an error occured?
  bool Error() const { return m_error; }

  // dtor closes the file if opened
  virtual ~CFile() { Close(); }

private:
  // copy ctor and assignment operator are private because
  // it doesn't make sense to copy files this way:
  // attempt to do it will provoke a compile-time error.
  CFile(const CFile&);
  CFile& operator=(const CFile&);

  int m_fd; // file descriptor or INVALID_FD if not opened
  bool m_error; // error memory
  wxString fFilePath;
};

#endif // CFILE_H
