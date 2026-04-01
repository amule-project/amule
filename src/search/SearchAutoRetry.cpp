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

#include "SearchAutoRetry.h"
#include <wx/datetime.h>
#include <wx/log.h>

namespace search {

// Default configuration values
static const int DEFAULT_MAX_RETRY_COUNT = 3;
static const int DEFAULT_RETRY_DELAY = 5000; // 5 seconds
static const int DEFAULT_MORE_BUTTON_TIMEOUT = 30; // 30 seconds

BEGIN_EVENT_TABLE(SearchAutoRetry, wxEvtHandler)
	EVT_TIMER(wxID_ANY, SearchAutoRetry::OnRetryTimer)
END_EVENT_TABLE()

SearchAutoRetry::SearchAutoRetry()
	: m_maxRetryCount(DEFAULT_MAX_RETRY_COUNT)
	, m_retryDelay(DEFAULT_RETRY_DELAY)
	, m_moreButtonTimeout(DEFAULT_MORE_BUTTON_TIMEOUT)
	, m_retryTimer(this)
	, m_moreButtonTimeoutTimer(this)
{
	// Initialize timers
	m_retryTimer.Stop();
	m_moreButtonTimeoutTimer.Stop();
}

SearchAutoRetry::~SearchAutoRetry()
{
	// Clean up timers
	m_retryTimer.Stop();
	m_moreButtonTimeoutTimer.Stop();

	// Clear all states
	m_retryStates.clear();
	m_moreButtonStates.clear();
}

void SearchAutoRetry::SetMaxRetryCount(int maxRetries)
{
	wxCHECK_RET(maxRetries >= 0, wxT("Max retry count must be non-negative"));
	m_maxRetryCount = maxRetries;
}

int SearchAutoRetry::GetMaxRetryCount() const
{
	return m_maxRetryCount;
}

void SearchAutoRetry::SetRetryDelay(int delayMs)
{
	wxCHECK_RET(delayMs >= 0, wxT("Retry delay must be non-negative"));
	m_retryDelay = delayMs;
}

int SearchAutoRetry::GetRetryDelay() const
{
	return m_retryDelay;
}

void SearchAutoRetry::SetMoreButtonTimeout(int timeoutSeconds)
{
	wxCHECK_RET(timeoutSeconds >= 0, wxT("Timeout must be non-negative"));
	m_moreButtonTimeout = timeoutSeconds;
}

int SearchAutoRetry::GetMoreButtonTimeout() const
{
	return m_moreButtonTimeout;
}

void SearchAutoRetry::StartRetry(long searchId, ModernSearchType type)
{
	RetryState state;
	state.retryCount = 0;
	state.searchType = type;
	state.lastRetryTime = wxDateTime::Now();

	m_retryStates[searchId] = state;

	// Start retry timer
	m_retryTimer.StartOnce(m_retryDelay);
}

void SearchAutoRetry::StopRetry(long searchId)
{
	m_retryStates.erase(searchId);
}

void SearchAutoRetry::IncrementRetryCount(long searchId)
{
	auto it = m_retryStates.find(searchId);
	if (it != m_retryStates.end()) {
		it->second.retryCount++;
		it->second.lastRetryTime = wxDateTime::Now();
	}
}

int SearchAutoRetry::GetRetryCount(long searchId) const
{
	auto it = m_retryStates.find(searchId);
	if (it != m_retryStates.end()) {
		return it->second.retryCount;
	}
	return 0;
}

bool SearchAutoRetry::ShouldRetry(long searchId) const
{
	auto it = m_retryStates.find(searchId);
	if (it != m_retryStates.end()) {
		return it->second.retryCount < m_maxRetryCount;
	}
	return false;
}

void SearchAutoRetry::StartMoreButtonTimeout(long searchId)
{
	MoreButtonState state;
	state.searchId = searchId;
	state.startTime = wxDateTime::Now();
	state.isActive = true;

	m_moreButtonStates[searchId] = state;

	// Start timeout timer
	m_moreButtonTimeoutTimer.StartOnce(m_moreButtonTimeout * 1000);
}

void SearchAutoRetry::StopMoreButtonTimeout(long searchId)
{
	m_moreButtonStates.erase(searchId);
}

bool SearchAutoRetry::IsMoreButtonTimedOut(long searchId) const
{
	wxDateTime now = wxDateTime::Now();

	for (const auto& pair : m_moreButtonStates) {
		const MoreButtonState& state = pair.second;

		if (state.isActive) {
			wxTimeSpan elapsed = now - state.startTime;
			if (elapsed.GetSeconds().ToLong() >= m_moreButtonTimeout) {
				return true;
			}
		}
	}

	return false;
}

void SearchAutoRetry::SetOnRetry(RetryCallback callback)
{
	m_onRetry = callback;
}

void SearchAutoRetry::OnRetryTimer(wxTimerEvent& event)
{
	// Find searches that need retry
	wxDateTime now = wxDateTime::Now();

	for (auto it = m_retryStates.begin(); it != m_retryStates.end(); ) {
		long searchId = it->first;
		RetryState& state = it->second;

		// Check if enough time has passed since last retry
		wxTimeSpan elapsed = now - state.lastRetryTime;
		if (elapsed.GetMilliseconds().ToLong() >= m_retryDelay) {
			// Check if we should retry
			if (state.retryCount < m_maxRetryCount) {
				// Trigger retry callback
				if (m_onRetry) {
					m_onRetry(searchId, state.searchType, state.retryCount + 1);
				}

				// Update state
				state.retryCount++;
				state.lastRetryTime = now;
				it->second = state;

				++it;
			} else {
				// Max retries reached, remove state
				it = m_retryStates.erase(it);
			}
		} else {
			++it;
		}
	}
}

void SearchAutoRetry::OnMoreButtonTimeoutTimer(wxTimerEvent& event)
{
	// Check for timed-out "More" button searches
	wxDateTime now = wxDateTime::Now();

	for (auto it = m_moreButtonStates.begin(); it != m_moreButtonStates.end(); ) {
		const MoreButtonState& state = it->second;

		if (state.isActive) {
			wxTimeSpan elapsed = now - state.startTime;
			if (elapsed.GetSeconds().ToLong() >= m_moreButtonTimeout) {
				// Timeout occurred
				wxLogMessage(wxT("More button search timed out for search ID: %ld"),
					state.searchId);

				// Mark as timed out
				it->second.isActive = false;
			}
		}

		++it;
	}
}

} // namespace search
