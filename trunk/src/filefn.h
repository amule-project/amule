// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
////////////////////////////////////////////////////////////////////////////
// Name:        filefn.h
// Purpose:     Patched wxCopyFile & wxRenameFile for chmod behaviour
// Author:      Angel Vidal - Kry
// Modified by: Angel Vidal - Kry
// Created:     29/12/03
// Copyright:   (c) 2003 Angel Vidal - Kry
// Licence:     GPL
// Based on wxWindows's filefn.cpp
/////////////////////////////////////////////////////////////////////////////

#ifndef FILEFN_H
#define FILEFN_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString

// Kry - vfat versions
bool wxCopyFile_fat32(const wxString& file1, const wxString& file2, bool overwrite = true, bool do_chmod = false);
bool wxRenameFile_fat32(const wxString& file1, const wxString& file2, bool do_chmod = false);

#endif // FILEFN_H
