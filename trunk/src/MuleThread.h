//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef MULETHREAD_H
#define MULETHREAD_H

#include <wx/thread.h>


class CMuleThread : public wxThread
{
public:
	//! @see wxThread::wxThread
	CMuleThread(wxThreadKind kind = wxTHREAD_DETACHED)
		: wxThread(kind),
		m_stop(false) {}

	/**
	 * Stops the thread.
	 *
	 * For detached threads, this function is equivalent
	 * to Delete, but is also useable for joinable threads,
	 * where Delete should not be used, due to crashes
	 * experienced in that case. In the case of joinable
	 * threads, Wait is called rather than Delete.
	 *
	 * @see wxThread::Delete
	 */
	void Stop()
	{
		m_stop = true;
		if (IsDetached()) {
			Delete();
		} else {
			Wait();
		}
	}

	//! Returns true if Delete or Stop has been called.
	virtual bool TestDestroy()
	{
		// m_stop is checked last, because some functionality is
		// dependant upon wxThread::TestDestroy() being called,
		// for instance Pause().
		return wxThread::TestDestroy() || m_stop;
	}
private:
	//! Is set if Stop is called.
	bool	m_stop;
};

#endif
