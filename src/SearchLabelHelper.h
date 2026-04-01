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

#ifndef SEARCHLABELHELPER_H
#define SEARCHLABELHELPER_H

#include <wx/string.h>

class CSearchListCtrl;
class CSearchDlg;
class wxNotebook;

/**
 * Helper function to update the search tab label with state information.
 *
 * This function updates the tab label to show the search state and hit count.
 * The state can be one of:
 * - "Searching" - Search is in progress but no results yet
 * - "Populating" - Results are being populated
 * - "No Results" - Search completed with no results
 * - Empty string - Search is complete or results are shown
 *
 * The function preserves the search type prefix ([Local], [Global], [Kad]).
 *
 * @param list The search list control to update
 * @param parentDlg The parent search dialog
 * @param state The current search state
 */
void UpdateSearchState(CSearchListCtrl* list, CSearchDlg* parentDlg, const wxString& state);

/**
 * Helper function to update search tab label with state and count information.
 *
 * This function is similar to UpdateSearchState but takes explicit count parameters
 * instead of getting them from the list control. This is useful when the counts
 * are managed by SearchStateManager and may be more accurate than the list's counts.
 *
 * @param list The search list control to update
 * @param parentDlg The parent search dialog
 * @param state The current search state
 * @param shown The number of shown results
 * @param hidden The number of hidden results
 */
void UpdateSearchStateWithCount(CSearchListCtrl* list, CSearchDlg* parentDlg, const wxString& state, size_t shown, size_t hidden);

/**
 * Helper function to update the hit count display with state information.
 *
 * This function determines the search state based on result count and
 * calls UpdateSearchState to update the tab label.
 *
 * @param page The search list control to update
 * @param parentDlg The parent search dialog
 */
void UpdateHitCountWithState(CSearchListCtrl* page, CSearchDlg* parentDlg);

/**
 * Helper function to retry a search that returned no results.
 *
 * This function attempts to retry a search when no results are found.
 * It works for both ED2K and Kad searches, updating the tab state
 * appropriately.
 *
 * @param page The search list control to retry
 * @param parentDlg The parent search dialog
 * @return true if retry was initiated, false otherwise
 */
bool RetrySearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg);

/**
 * Helper function to retry a Kad search that returned no results.
 *
 * This function attempts to retry a Kad search when no results are found.
 * It uses the same search ID, keyword, and search type for the retry.
 *
 * @param page The search list control to retry
 * @param parentDlg The parent search dialog
 * @return true if retry was initiated, false otherwise
 */
bool RetryKadSearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg);

#endif // SEARCHLABELHELPER_H
