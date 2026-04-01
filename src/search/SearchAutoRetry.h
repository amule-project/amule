//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef SEARCHAUTORETRY_H
#define SEARCHAUTORETRY_H

#include <wx/timer.h>
#include <map>
#include "SearchModel.h"

namespace search {

/**
 * SearchAutoRetry - Manages automatic retry logic for searches
 *
 * This class handles:
 * - Auto-retry on zero results
 * - Retry delays
 * - Retry count tracking
 * - Timeout handling for "More" button searches
 */
class SearchAutoRetry : public wxEvtHandler {
public:
	SearchAutoRetry();
	virtual ~SearchAutoRetry();

	// Configuration
	void SetMaxRetryCount(int maxRetries);
	int GetMaxRetryCount() const;

	void SetRetryDelay(int delayMs);
	int GetRetryDelay() const;

	void SetMoreButtonTimeout(int timeoutSeconds);
	int GetMoreButtonTimeout() const;

	// Retry management
	void StartRetry(long searchId, ModernSearchType type);
	void StopRetry(long searchId);
	void IncrementRetryCount(long searchId);
	int GetRetryCount(long searchId) const;
	bool ShouldRetry(long searchId) const;

	// "More" button timeout handling
	void StartMoreButtonTimeout(long searchId);
	void StopMoreButtonTimeout(long searchId);
	bool IsMoreButtonTimedOut(long searchId) const;

	// Callbacks
	using RetryCallback = std::function<void(long searchId, ModernSearchType type, int retryNum)>;
	void SetOnRetry(RetryCallback callback);

private:
	void OnRetryTimer(wxTimerEvent& event);
	void OnMoreButtonTimeoutTimer(wxTimerEvent& event);

	// Configuration
	int m_maxRetryCount;
	int m_retryDelay;
	int m_moreButtonTimeout;

	// Retry tracking
	struct RetryState {
		int retryCount;
		ModernSearchType searchType;
		wxDateTime lastRetryTime;
	};
	std::map<long, RetryState> m_retryStates;

	// "More" button timeout tracking
	struct MoreButtonState {
		long searchId;
		wxDateTime startTime;
		bool isActive;
	};
	std::map<long, MoreButtonState> m_moreButtonStates;

	// Timers
	wxTimer m_retryTimer;
	wxTimer m_moreButtonTimeoutTimer;

	// Callbacks
	RetryCallback m_onRetry;

	DECLARE_EVENT_TABLE()
};

} // namespace search

#endif // SEARCHAUTORETRY_H
