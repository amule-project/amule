/////////////////////////////////////////////////////////////////////////////
// Name:        filefn.cpp
// Purpose:     Patched wxCopyFile & wxRenameFile for chmod behaviour
// Author:      Angel Vidal - Kry
// Modified by: Angel Vidal - Kry
// Created:     29/12/03
// Copyright:   (c) 2003 Angel Vidal - Kry
// Licence:     GPL
// Based on wxWindows's filefn.cpp
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------


#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <wx/utils.h>
#include <wx/intl.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/dir.h>

#if defined(__WXMAC__)
  #include  <wx/mac/private.h>  // includes mac headers
#endif

#ifndef __MWERKS__
    #include <sys/types.h>
    #include <sys/stat.h>
#else
    #include <stat.h>
    #include <unistd.h>
    #include <unix.h>
#endif

#ifdef __UNIX__
    #include <unistd.h>
    #include <dirent.h>
    #include <fcntl.h>
#endif

#ifdef __WXPM__
    #include <process.h>
    #include "wx/os2/private.h"
#endif
#if defined(__WINDOWS__) && !defined(__WXMICROWIN__) && !defined(__WXWINE__)
#if !defined( __GNUWIN32__ ) && !defined( __MWERKS__ ) && !defined(__SALFORDC__)
    #include <direct.h>
    #include <dos.h>
    #include <io.h>
#endif // __WINDOWS__
#endif // native Win compiler

#if defined(__DOS__)
    #ifdef __WATCOMC__
        #include <direct.h>
        #include <dos.h>
        #include <io.h>
    #endif
    #ifdef __DJGPP__
        #include <unistd.h>
    #endif
#endif

#ifdef __BORLANDC__ // Please someone tell me which version of Borland needs
                    // this (3.1 I believe) and how to test for it.
                    // If this works for Borland 4.0 as well, then no worries.
    #include <dir.h>
#endif

#ifdef __SALFORDC__
    #include <dir.h>
    #include <unix.h>
#endif

#include <wx/log.h>

// No, Cygwin doesn't appear to have fnmatch.h after all.
#if defined(HAVE_FNMATCH_H)
    #include <fnmatch.h>
#endif

#ifdef __WINDOWS__
    #include <windows.h>
    #include <wx/msw/mslu.h>

    // sys/cygwin.h is needed for cygwin_conv_to_full_win32_path()
    //
    // note that it must be included after <windows.h>
    #ifdef __GNUWIN32__
        #ifdef __CYGWIN__
            #include <sys/cygwin.h>
        #endif

        #ifndef __TWIN32__
            #include <sys/unistd.h>
        #endif
    #endif // __GNUWIN32__
#endif // __WINDOWS__

// TODO: Borland probably has _wgetcwd as well?
#ifdef _MSC_VER
    #define HAVE_WGETCWD
#endif

// ----------------------------------------------------------------------------
// constants
// -------------------------------------------------------------------------
#include "filefn.h"		// Interface declarations.

// we need to translate Mac filenames before passing them to OS functions
#define MY_OS_FILENAME(s) (s.fn_str())

// Copy files
bool
wxCopyFile_fat32 (const wxString& file1, const wxString& file2, bool overwrite, bool do_chmod)
{
#if defined(__WIN32__) && !defined(__WXMICROWIN__)
    // CopyFile() copies file attributes and modification time too, so use it
    // instead of our code if available
    //
    // NB: 3rd parameter is bFailIfExists i.e. the inverse of overwrite
    if ( !::CopyFile(file1, file2, !overwrite) )
    {
        wxLogSysError(_("Failed to copy the file '%s' to '%s'"),
                      file1.c_str(), file2.c_str());

        return FALSE;
    }
#elif defined(__WXPM__)
    if ( ::DosCopy(file2, file2, overwrite ? DCPY_EXISTING : 0) != 0 )
        return FALSE;
#else // !Win32

    wxStructStat fbuf;
    // get permissions of file1
    if ( wxStat( file1.c_str(), &fbuf) != 0 )
    {
        // the file probably doesn't exist or we haven't the rights to read
        // from it anyhow
        wxLogSysError(_("Impossible to get permissions for file '%s'"),
                      file1.c_str());
        return FALSE;
    }

    // open file1 for reading
    wxFile fileIn(file1, wxFile::read);
    if ( !fileIn.IsOpened() )
        return FALSE;

    // remove file2, if it exists. This is needed for creating
    // file2 with the correct permissions in the next step
    if ( wxFileExists(file2)  && (!overwrite || !wxRemoveFile(file2)))
    {
        wxLogSysError(_("Impossible to overwrite the file '%s'"),
                      file2.c_str());
        return FALSE;
    }

#ifdef __UNIX__
    // reset the umask as we want to create the file with exactly the same
    // permissions as the original one
    mode_t oldUmask = umask( 0 );
#endif // __UNIX__

    // create file2 with the same permissions than file1 and open it for
    // writing
    
    wxFile fileOut;
    if ( !fileOut.Create(file2, overwrite, fbuf.st_mode & 0777) )
        return FALSE;

#ifdef __UNIX__
    /// restore the old umask
    umask(oldUmask);
#endif // __UNIX__

    // copy contents of file1 to file2
    char buf[4096];
    size_t count;
    for ( ;; )
    {
        count = fileIn.Read(buf, WXSIZEOF(buf));
        if ( fileIn.Error() )
            return FALSE;

        // end of file?
        if ( !count )
            break;

        if ( fileOut.Write(buf, count) < count )
            return FALSE;
    }

    // we can expect fileIn to be closed successfully, but we should ensure
    // that fileOut was closed as some write errors (disk full) might not be
    // detected before doing this
    if ( !fileIn.Close() || !fileOut.Close() )
        return FALSE;

#if !defined(__VISAGECPP__) && !defined(__WXMAC__) || defined(__UNIX__)
    // no chmod in VA.  Should be some permission API for HPFS386 partitions
    // however
    // Kry -- don't chmod if selected
    if (do_chmod) {
    	if ( chmod(MY_OS_FILENAME(file2), fbuf.st_mode) != 0 )
    		{
        		wxLogSysError(_("Impossible to set permissions for the file '%s'"),
                      file2.c_str());
        	return FALSE;
    		}
	}
#endif // OS/2 || Mac
#endif // __WXMSW__ && __WIN32__

    return TRUE;
}

// Kry - more aditions.
bool
wxRenameFile_fat32 (const wxString& file1, const wxString& file2, bool do_chmod)
{
  // Normal system call
  if ( wxRename (file1, file2) == 0 )
    return TRUE;

  // Try to copy
  if (wxCopyFile_fat32(file1, file2,TRUE,FALSE)) {
    wxRemoveFile(file1);
    return TRUE;
  }
  // Give up
  return FALSE;
}
