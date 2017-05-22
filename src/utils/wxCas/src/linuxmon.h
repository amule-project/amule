//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         LinuxMon Class
///
/// Purpose:      Monitor Linux system by reading /proc file system
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmaps from aMule http://www.amule.org
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
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _LINUXMON_H
#define _LINUXMON_H


#include <wx/filename.h>

/// Linux Monitoring
class LinuxMon
{
private:
	wxString m_uptime;

	wxString m_sysLoad_1;
	wxString m_sysLoad_5;
	wxString m_sysLoad_15;

	static const wxFileName UPTIME_FILE;
	static const wxFileName LOADAVG_FILE;

public:
	/// Constructor
	LinuxMon ();

	/// Destructor
	~LinuxMon ();

	/// Refresh stored informations
	void Refresh ();

	/// Get system uptime
	wxString GetUptime () const;

	/// Get 1min average CPU load
	wxString GetSysLoad_1 () const;

	/// Get 5min averag CPU load
	wxString GetSysLoad_5 () const;

	/// Get 15min averag CPU load
	wxString GetSysLoad_15 () const;
};

#endif /* _LINUXMON_H */
// File_checked_for_headers
