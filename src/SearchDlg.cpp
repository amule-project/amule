//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net /
// http://www.emule-project.net )
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

#include "SearchDlg.h" // Interface declarations.

#include <cassert>
#include <common/Format.h>
#include <tags/FileTags.h>
#include <wx/app.h>
#include <wx/gauge.h> // Do_not_auto_remove (win32)
#include "search/SearchModel.h" // Needed for search::ModernSearchType

#include "GetTickCount.h"
#include "Logger.h"
#include "MuleNotebook.h"
#include "OtherFunctions.h" // Needed for GetTypeSize
#include "Preferences.h"
#include "SearchLabelHelper.h"
#include "SearchList.h"     // Needed for CSearchList
#include "SearchListCtrl.h" // Needed for CSearchListCtrl
#include "amule.h"          // Needed for theApp
#include "amuleDlg.h"       // Needed for CamuleDlg
#include "muuli_wdr.h"      // Needed for IDC_STARTS
#include "search/SearchLogging.h"
#include "search/SearchIdGenerator.h"  // Needed for search ID generation
#include "kademlia/kademlia/SearchManager.h"  // Needed for Kademlia::CSearchManager::IsSearching
#include "kademlia/kademlia/Kademlia.h"  // Needed for Kademlia::WordList

#define ID_SEARCHLISTCTRL wxID_HIGHEST + 667

// just to keep compiler happy
static wxCommandEvent nullEvent;

BEGIN_EVENT_TABLE(CSearchDlg, wxPanel)
EVT_BUTTON(IDC_STARTS, CSearchDlg::OnBnClickedStart)
EVT_TEXT_ENTER(IDC_SEARCHNAME, CSearchDlg::OnBnClickedStart)

EVT_BUTTON(IDC_CANCELS, CSearchDlg::OnBnClickedStop)

EVT_LIST_ITEM_SELECTED(ID_SEARCHLISTCTRL, CSearchDlg::OnListItemSelected)

EVT_BUTTON(IDC_SDOWNLOAD, CSearchDlg::OnBnClickedDownload)
EVT_BUTTON(IDC_SEARCH_RESET, CSearchDlg::OnBnClickedReset)
EVT_BUTTON(IDC_CLEAR_RESULTS, CSearchDlg::OnBnClickedClear)
EVT_BUTTON(IDC_SEARCHMORE, CSearchDlg::OnBnClickedMore)

EVT_CHECKBOX(IDC_EXTENDEDSEARCHCHECK, CSearchDlg::OnExtendedSearchChange)
EVT_CHECKBOX(IDC_FILTERCHECK, CSearchDlg::OnFilterCheckChange)

// Event handler for search type change
EVT_CHOICE(ID_SEARCHTYPE, CSearchDlg::OnSearchTypeChanged)

EVT_MULENOTEBOOK_PAGE_CLOSING(ID_NOTEBOOK, CSearchDlg::OnSearchClosing)
EVT_NOTEBOOK_PAGE_CHANGED(ID_NOTEBOOK, CSearchDlg::OnSearchPageChanged)

// Event handlers for the parameter fields getting changed
EVT_CUSTOM(wxEVT_COMMAND_TEXT_UPDATED, IDC_SEARCHNAME,
           CSearchDlg::OnFieldChanged)
EVT_CUSTOM(wxEVT_COMMAND_TEXT_UPDATED, IDC_EDITSEARCHEXTENSION,
           CSearchDlg::OnFieldChanged)
EVT_CUSTOM(wxEVT_COMMAND_SPINCTRL_UPDATED, wxID_ANY, CSearchDlg::OnFieldChanged)
EVT_CUSTOM(wxEVT_COMMAND_CHOICE_SELECTED, wxID_ANY, CSearchDlg::OnFieldChanged)

// Event handlers for the filter fields getting changed.
EVT_TEXT_ENTER(ID_FILTER_TEXT, CSearchDlg::OnFilteringChange)
EVT_CHECKBOX(ID_FILTER_INVERT, CSearchDlg::OnFilteringChange)
EVT_CHECKBOX(ID_FILTER_KNOWN, CSearchDlg::OnFilteringChange)
EVT_BUTTON(ID_FILTER, CSearchDlg::OnFilteringChange)

// Timer event for timeout checking
EVT_TIMER(wxID_ANY, CSearchDlg::OnTimeoutCheck)
END_EVENT_TABLE()

CSearchDlg::CSearchDlg(wxWindow *pParent) : wxPanel(pParent, -1) {
  m_last_search_time = 0;

  wxSizer *content = searchDlg(this, true);
  content->Show(this, true);

  m_progressbar = CastChild(ID_SEARCHPROGRESS, wxGauge);
  m_progressbar->SetRange(100);

  m_notebook = CastChild(ID_NOTEBOOK, CMuleNotebook);

#ifdef __WXMAC__
  // #warning TODO: restore the image list if/when wxMac supports locating the
  // image
#else
  // Initialise the image list
  wxImageList *m_ImageList = new wxImageList(16, 16);
  m_ImageList->Add(amuleSpecial(3));
  m_ImageList->Add(amuleSpecial(4));
  m_notebook->AssignImageList(m_ImageList);
#endif

  // Sanity sanity
  wxChoice *searchchoice = CastChild(ID_SEARCHTYPE, wxChoice);
  wxASSERT(searchchoice);
  wxASSERT(searchchoice->GetString(0) == _("Local"));
  wxASSERT(searchchoice->GetString(2) == _("Kad"));
  wxASSERT(searchchoice->GetCount() == 3);

  m_searchchoices = searchchoice->GetStrings();

  // Register as observer for search state changes
  m_stateManager.RegisterObserver(this);

  // Initialize timeout check timer (check every 5 seconds)
  m_timeoutCheckTimer.SetOwner(this);
  m_timeoutCheckTimer.Start(5000);

  // Register as observer with search state manager
  m_stateManager.RegisterObserver(this);

  // Set up search completion callback for UnifiedSearchManager
  m_unifiedSearchManager.setSearchCompletedCallback(
    [this](uint32_t searchId, bool hasResults) {
      // Notify the search state manager that the search completed
      if (hasResults) {
        m_stateManager.UpdateState(searchId, STATE_HAS_RESULTS);
      } else {
        m_stateManager.UpdateState(searchId, STATE_NO_RESULTS);
      }
    });

  // Let's break it now.

  FixSearchTypes();

  CastChild(IDC_TypeSearch, wxChoice)->SetSelection(0);
  CastChild(IDC_SEARCHMINSIZE, wxChoice)->SetSelection(2);
  CastChild(IDC_SEARCHMAXSIZE, wxChoice)->SetSelection(2);

  // Not there initially.
  s_searchsizer->Show(s_extendedsizer, false);
  s_searchsizer->Show(s_filtersizer, false);

  Layout();
}

CSearchDlg::~CSearchDlg() {
  // Unregister as observer for search state changes
  m_stateManager.UnregisterObserver(this);
}

void CSearchDlg::FixSearchTypes() {
  wxChoice *searchchoice = CastChild(ID_SEARCHTYPE, wxChoice);

  searchchoice->Clear();

  int pos = 0;

  // ED2K search options
  if (thePrefs::GetNetworkED2K()) {
    searchchoice->Insert(m_searchchoices[0], pos++); // Local
    searchchoice->Insert(m_searchchoices[1], pos++); // Global
  }

  // Kademlia search option
  if (thePrefs::GetNetworkKademlia()) {
    searchchoice->Insert(m_searchchoices[2], pos++); // Kad
  }

  searchchoice->SetSelection(0);
}

CSearchListCtrl *CSearchDlg::GetSearchList(wxUIntPtr id) {
  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; i++) {
    CSearchListCtrl *page =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));

    if (page->GetSearchId() == id) {
      return page;
    }
  }

  return NULL;
}

void CSearchDlg::AddResult(CSearchFile *toadd) {
  CSearchListCtrl *outputwnd = GetSearchList(toadd->GetSearchID());

  if (outputwnd) {
    // Check if the tab is being closed before adding results
    int pageIndex = m_notebook->FindPage(outputwnd);
    if (pageIndex == wxNOT_FOUND) {
      // Tab has been closed, discard this result
      return;
    }

    outputwnd->AddResult(toadd);

    // Update the result count in the state manager
    size_t shown = outputwnd->GetItemCount();
    size_t hidden = outputwnd->GetHiddenItemCount();
    m_stateManager.UpdateResultCount(toadd->GetSearchID(), shown, hidden);

    // Update the hit count in the tab label
    UpdateHitCount(outputwnd);
  }
}

void CSearchDlg::UpdateResult(CSearchFile *toupdate) {
  CSearchListCtrl *outputwnd = GetSearchList(toupdate->GetSearchID());

  if (outputwnd) {
    // Check if the tab is being closed before updating results
    int pageIndex = m_notebook->FindPage(outputwnd);
    if (pageIndex == wxNOT_FOUND) {
      // Tab has been closed, discard this update
      return;
    }

    outputwnd->UpdateResult(toupdate);

    // Update the result count in the state manager
    size_t shown = outputwnd->GetItemCount();
    size_t hidden = outputwnd->GetHiddenItemCount();
    m_stateManager.UpdateResultCount(toupdate->GetSearchID(), shown, hidden);

    // Update the hit count in the tab label
    UpdateHitCount(outputwnd);
  }
}

void CSearchDlg::OnListItemSelected(wxListEvent &event) {
  FindWindow(IDC_SDOWNLOAD)->Enable(true);

  event.Skip();
}

void CSearchDlg::OnExtendedSearchChange(wxCommandEvent &event) {
  s_searchsizer->Show(s_extendedsizer, event.IsChecked());

  Layout();
}

void CSearchDlg::OnFilterCheckChange(wxCommandEvent &event) {
  s_searchsizer->Show(s_filtersizer, event.IsChecked());
  Layout();

  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; i++) {
    CSearchListCtrl *page =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));

    page->EnableFiltering(event.IsChecked());

    UpdateHitCount(page);
  }
}

void CSearchDlg::OnSearchClosing(wxBookCtrlEvent &evt) {
  // Abort global search if it was last tab that was closed.
  if (evt.GetSelection() == ((int)m_notebook->GetPageCount() - 1)) {
    OnBnClickedStop(nullEvent);
  }

  CSearchListCtrl *ctrl =
      dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(evt.GetSelection()));
  wxASSERT(ctrl);

  // Clean up the search
  long searchId = ctrl->GetSearchId();

  // Stop the search using UnifiedSearchManager
  m_unifiedSearchManager.stopSearch(searchId);

  // Zero to avoid results added while destructing.
  ctrl->ShowResults(0);

  // Remove from SearchStateManager
  m_stateManager.RemoveSearch(searchId);

  // Remove from search cache (allows future duplicate searches to create new tabs)
  if (m_searchCache.IsEnabled()) {
    m_searchCache.RemoveSearch(searchId);
  }

  // Do cleanups if this was the last tab
  if (m_notebook->GetPageCount() == 1) {
    FindWindow(IDC_SDOWNLOAD)->Enable(FALSE);
    FindWindow(IDC_CLEAR_RESULTS)->Enable(FALSE);
  }
}

void CSearchDlg::OnSearchPageChanged(wxBookCtrlEvent &WXUNUSED(evt)) {
  int selection = m_notebook->GetSelection();

  // Workaround for a bug in wxWidgets, where deletions of pages
  // can result in an invalid selection. This has been reported as
  // http://sourceforge.net/tracker/index.php?func=detail&aid=1865141&group_id=9863&atid=109863
  if (selection >= (int)m_notebook->GetPageCount()) {
    selection = m_notebook->GetPageCount() - 1;
  }

  // Only enable the Download button for pages where files have been selected
  if (selection != -1) {
    CSearchListCtrl *ctrl =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(selection));

    bool enable = (ctrl->GetSelectedItemCount() > 0);
    FindWindow(IDC_SDOWNLOAD)->Enable(enable);

    // Enable the More button for all search types (Local, Global, Kad)
    // Kad searches now support requesting more results using the reaskMore mechanism
    // Use SearchStateManager to get the search type instead of parsing tab text
    long searchId = ctrl->GetSearchId();
    wxString searchType = m_stateManager.GetSearchType(searchId);
    bool isValidSearchType =
        (searchType == wxT("Local") || searchType == wxT("Global") || searchType == wxT("Kad"));
    FindWindow(IDC_SEARCHMORE)->Enable(isValidSearchType);
  }
}

void CSearchDlg::OnBnClickedStart(wxCommandEvent &WXUNUSED(evt)) {
  if (!thePrefs::GetNetworkED2K() && !thePrefs::GetNetworkKademlia()) {
    wxMessageBox(_("It's impossible to search when both eD2k and Kademlia are "
                   "disabled."),
                 _("Search error"), wxOK | wxCENTRE | wxICON_ERROR);
    return;
  }

  // Check if the selected search type is connected to its respective network
  int selection = CastChild(ID_SEARCHTYPE, wxChoice)->GetSelection();
  if (selection == wxNOT_FOUND) {
    wxMessageBox(_("Please select a search type."), _("Search error"),
                 wxOK | wxCENTRE | wxICON_WARNING);
    return;
  }

  // Determine which network corresponds to the selected search type
  bool isSearchTypeConnected = false;

  if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
    // Full network support - 3 options (Local, Global, Kad)
    switch (selection) {
    case 0: // Local - needs ED2K connection
      isSearchTypeConnected = theApp->IsConnectedED2K();
      break;
    case 1: // Global - needs ED2K connection
      isSearchTypeConnected = theApp->IsConnectedED2K();
      break;
    case 2: // Kad - needs Kad connection
      isSearchTypeConnected = theApp->IsConnectedKad();
      break;
    }
  } else if (thePrefs::GetNetworkED2K()) {
    // Only ED2K support - 2 options (Local, Global)
    switch (selection) {
    case 0: // Local - needs ED2K connection
      isSearchTypeConnected = theApp->IsConnectedED2K();
      break;
    case 1: // Global - needs ED2K connection
      isSearchTypeConnected = theApp->IsConnectedED2K();
      break;
    }
  } else if (thePrefs::GetNetworkKademlia()) {
    // Only Kad support - 1 option (Kad)
    switch (selection) {
    case 0: // Kad - needs Kad connection
      isSearchTypeConnected = theApp->IsConnectedKad();
      break;
    }
  }

  if (!isSearchTypeConnected) {
    wxString searchTypeName;
    if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
      switch (selection) {
      case 0:
        searchTypeName = _("Local (eD2k)");
        break;
      case 1:
        searchTypeName = _("Global (eD2k)");
        break;
      case 2:
        searchTypeName = _("Kad");
        break;
      }
    } else if (thePrefs::GetNetworkED2K()) {
      switch (selection) {
      case 0:
        searchTypeName = _("Local (eD2k)");
        break;
      case 1:
        searchTypeName = _("Global (eD2k)");
        break;
      }
    } else if (thePrefs::GetNetworkKademlia()) {
      searchTypeName = _("Kad");
    }

    wxMessageBox(_("The selected search type (" + searchTypeName +
                   ") is not connected to its network. Please connect first."),
                 _("Search error"), wxOK | wxCENTRE | wxICON_WARNING);
    return;
  }

  // We mustn't search more often than once every 2 secs
  if ((GetTickCount() - m_last_search_time) >= 2000) {
    m_last_search_time = GetTickCount();
    StartNewSearch();
  } else {
    // Provide feedback to the user that they need to wait
    uint32_t remainingTime = 2000 - (GetTickCount() - m_last_search_time);
    AddDebugLogLineN(logSearch, CFormat(wxT("Please wait %u ms before starting another search"))
        % remainingTime);
  }
}

void CSearchDlg::UpdateStartButtonState() {
  wxButton *startBtn = CastChild(IDC_STARTS, wxButton);
  if (startBtn) {
    // Check if networks are enabled
    bool networksEnabled =
        thePrefs::GetNetworkED2K() || thePrefs::GetNetworkKademlia();
    if (!networksEnabled) {
      startBtn->Enable(false);
      return;
    }

    // Check if there's search text
    bool hasSearchText =
        !CastChild(IDC_SEARCHNAME, wxTextCtrl)->GetValue().IsEmpty();
    if (!hasSearchText) {
      startBtn->Enable(false);
      return;
    }

    // Get the currently selected search type
    int selection = CastChild(ID_SEARCHTYPE, wxChoice)->GetSelection();
    if (selection == wxNOT_FOUND) {
      startBtn->Enable(false);
      return;
    }

    // Determine which network corresponds to the selected search type
    bool isSearchTypeConnected = false;

    // Recreate the same logic as in StartNewSearch to map selection to search
    // type
    if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
      // Full network support - 3 options (Local, Global, Kad)
      switch (selection) {
      case 0: // Local - needs ED2K connection
        isSearchTypeConnected = theApp->IsConnectedED2K();
        break;
      case 1: // Global - needs ED2K connection
        isSearchTypeConnected = theApp->IsConnectedED2K();
        break;
      case 2: // Kad - needs Kad connection
        isSearchTypeConnected = theApp->IsConnectedKad();
        break;
      }
    } else if (thePrefs::GetNetworkED2K()) {
      // Only ED2K support - 2 options (Local, Global)
      switch (selection) {
      case 0: // Local - needs ED2K connection
        isSearchTypeConnected = theApp->IsConnectedED2K();
        break;
      case 1: // Global - needs ED2K connection
        isSearchTypeConnected = theApp->IsConnectedED2K();
        break;
      }
    } else if (thePrefs::GetNetworkKademlia()) {
      // Only Kad support - 1 option (Kad)
      switch (selection) {
      case 0: // Kad - needs Kad connection
        isSearchTypeConnected = theApp->IsConnectedKad();
        break;
      }
    }

    startBtn->Enable(hasSearchText && isSearchTypeConnected);
  }
}

void CSearchDlg::OnFieldChanged(wxEvent &WXUNUSED(evt)) {
  bool enable = false;

  // These are the IDs of the search-fields
  int textfields[] = {IDC_SEARCHNAME, IDC_EDITSEARCHEXTENSION};

  for (uint16 i = 0; i < itemsof(textfields); i++) {
    enable |= !CastChild(textfields[i], wxTextCtrl)->GetValue().IsEmpty();
  }

  // Check if either of the dropdowns have been changed
  enable |= (CastChild(IDC_SEARCHMINSIZE, wxChoice)->GetSelection() != 2);
  enable |= (CastChild(IDC_SEARCHMAXSIZE, wxChoice)->GetSelection() != 2);
  enable |= (CastChild(IDC_TypeSearch, wxChoice)->GetSelection() > 0);
  enable |= (CastChild(ID_AUTOCATASSIGN, wxChoice)->GetSelection() > 0);

  // These are the IDs of the search-fields
  int spinfields[] = {IDC_SPINSEARCHMIN, IDC_SPINSEARCHMAX,
                      IDC_SPINSEARCHAVAIBILITY};
  for (uint16 i = 0; i < itemsof(spinfields); i++) {
    enable |= (CastChild(spinfields[i], wxSpinCtrl)->GetValue() > 0);
  }

  // Enable the "Reset" button if any fields contain text
  FindWindow(IDC_SEARCH_RESET)->Enable(enable);

  // Update start button state based on field changes and connection status
  UpdateStartButtonState();
}

void CSearchDlg::OnFilteringChange(wxCommandEvent &WXUNUSED(evt)) {
  wxString filter = CastChild(ID_FILTER_TEXT, wxTextCtrl)->GetValue();
  bool invert = CastChild(ID_FILTER_INVERT, wxCheckBox)->GetValue();
  bool known = CastChild(ID_FILTER_KNOWN, wxCheckBox)->GetValue();

  // Check that the expression compiles before we try to assign it
  // Otherwise we will get an error-dialog for each result-list.
  if (wxRegEx(filter, wxRE_DEFAULT | wxRE_ICASE).IsValid()) {
    int nPages = m_notebook->GetPageCount();
    for (int i = 0; i < nPages; i++) {
      CSearchListCtrl *page =
          dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));

      page->SetFilter(filter, invert, known);

      UpdateHitCount(page);
    }
  }
}

bool CSearchDlg::CheckTabNameExists(SearchType searchType,
                                    const wxString &searchString) {
  wxMutexLocker lock(m_searchCreationMutex);
  
  // Convert SearchType to string for comparison with SearchStateManager
  wxString searchTypeStr;
  switch (searchType) {
  case LocalSearch:
    searchTypeStr = wxT("Local");
    break;
  case GlobalSearch:
    searchTypeStr = wxT("Global");
    break;
  case KadSearch:
    searchTypeStr = wxT("Kad");
    break;
  default:
    return false;
  }

  // Check all tabs using SearchStateManager for reliable identification
  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; i++) {
    CSearchListCtrl *page = dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));
    if (page) {
      long searchId = page->GetSearchId();
      if (searchId != 0 && m_stateManager.HasSearch(searchId)) {
        // Get search information from SearchStateManager
        wxString tabSearchType = m_stateManager.GetSearchType(searchId);
        wxString tabKeyword = m_stateManager.GetKeyword(searchId);
        
        // Check if type and keyword match
        if (tabSearchType == searchTypeStr && tabKeyword == searchString) {
          return true;
        }
      }
    }
  }

  return false;
}


void CSearchDlg::CreateNewTab(const wxString &searchString,
                              wxUIntPtr nSearchID) {
  wxMutexLocker lock(m_searchCreationMutex);
  
  CSearchListCtrl *list =
      new CSearchListCtrl(m_notebook, ID_SEARCHLISTCTRL, wxDefaultPosition,
                          wxDefaultSize, wxLC_REPORT | wxNO_BORDER);
  m_notebook->AddPage(list, searchString, true, 0);

  // Parse search type from search string (e.g., "[Local] ", "[Global] ", "[Kad]
  // ")
  wxString searchType;
  if (searchString.StartsWith(wxT("[Local] "))) {
    searchType = wxT("Local");
  } else if (searchString.StartsWith(wxT("[Global] "))) {
    searchType = wxT("Global");
  } else if (searchString.StartsWith(wxT("[Kad] "))) {
    searchType = wxT("Kad");
  }

  // Store search type in the list control for validation
  list->SetSearchType(searchType);

  // Ensure that new results are filtered
  bool enable = CastChild(IDC_FILTERCHECK, wxCheckBox)->GetValue();
  wxString filter = CastChild(ID_FILTER_TEXT, wxTextCtrl)->GetValue();
  bool invert = CastChild(ID_FILTER_INVERT, wxCheckBox)->GetValue();
  bool known = CastChild(ID_FILTER_KNOWN, wxCheckBox)->GetValue();

  list->SetFilter(filter, invert, known);
  list->EnableFiltering(enable);
  list->ShowResults(nSearchID);

  // Update the tab label with initial state and hit count
  // The search should already be initialized in SearchStateManager from
  // StartNewSearch
  UpdateHitCount(list);

  Layout();
  FindWindow(IDC_CLEAR_RESULTS)->Enable(true);

  // Enable the More button for all search types (Local, Global, Kad)
  // Kad searches now support requesting more results using the reaskMore mechanism
  bool isEd2kSearch = (searchString.StartsWith(wxT("[Local] ")) ||
                       searchString.StartsWith(wxT("[Global] ")));
  bool isKadSearch = searchString.StartsWith(wxT("[Kad] "));
  FindWindow(IDC_SEARCHMORE)->Enable(isEd2kSearch || isKadSearch);
}

void CSearchDlg::OnBnClickedStop(wxCommandEvent &WXUNUSED(evt)) {
  // Stop all active searches using UnifiedSearchManager
  m_unifiedSearchManager.stopAllSearches();
  ResetControls();
}

void CSearchDlg::ResetControls() {
  m_progressbar->SetValue(0);

  FindWindow(IDC_CANCELS)->Disable();
  FindWindow(IDC_STARTS)
      ->Enable(!CastChild(IDC_SEARCHNAME, wxTextCtrl)->GetValue().IsEmpty());
  FindWindow(IDC_SEARCHMORE)->Disable();
}

void CSearchDlg::GlobalSearchEnd() {
  // Update all search tabs to show proper state when global search ends
  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; ++i) {
    CSearchListCtrl *page =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));
    if (page) {
      long searchId = page->GetSearchId();
      // Check if this is a Global search tab
      wxString searchType = m_stateManager.GetSearchType(searchId);
      if (searchType == wxT("Global")) {
        // Update result count in state manager
        size_t shown = page->GetItemCount();
        size_t hidden = page->GetHiddenItemCount();
        m_stateManager.UpdateResultCount(searchId, shown, hidden);

        // Check if we need to retry (no results and retry count not exceeded)
        if (shown == 0 && hidden == 0) {
          // Request retry through state manager
          if (m_stateManager.RequestRetry(searchId)) {
            // Trigger the actual retry
            OnRetryRequested(searchId);
            // Retry initiated, don't mark as finished yet
            continue;
          }
        }

        // End the search in the state manager (only if not retrying)
        m_stateManager.EndSearch(searchId);

        // Mark search as inactive in cache (allows future duplicate searches)
        if (m_searchCache.IsEnabled()) {
          m_searchCache.UpdateSearch(searchId, false);
        }
      }
    }
  }
  ResetControls();
}

void CSearchDlg::LocalSearchEnd() {
  // Update all search tabs to show proper state when local search ends
  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; ++i) {
    CSearchListCtrl *page =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));
    if (page) {
      long searchId = page->GetSearchId();
      // Check if this is an ED2K search tab (Local or Global)
      // Get the search type from state manager instead of parsing tab text
      wxString searchType = m_stateManager.GetSearchType(searchId);
      if (searchType == wxT("Local") || searchType == wxT("ED2K") ||
          searchType == wxT("Global")) {
        // Update result count in state manager
        size_t shown = page->GetItemCount();
        size_t hidden = page->GetHiddenItemCount();
        m_stateManager.UpdateResultCount(searchId, shown, hidden);

        // Check if we need to retry (no results and retry count not exceeded)
        if (shown == 0 && hidden == 0) {
          // Request retry through state manager
          if (m_stateManager.RequestRetry(searchId)) {
            // Trigger the actual retry
            OnRetryRequested(searchId);
            // Retry initiated, don't mark as finished yet
            continue;
          }
        }

        // End the search in the state manager (only if not retrying)
        m_stateManager.EndSearch(searchId);

        // Mark search as inactive in cache (allows future duplicate searches)
        if (m_searchCache.IsEnabled()) {
          m_searchCache.UpdateSearch(searchId, false);
        }
      }
    }
  }
  ResetControls();
}

void CSearchDlg::KadSearchEnd(uint32 id) {
  int nPages = m_notebook->GetPageCount();
  for (int i = 0; i < nPages; ++i) {
    CSearchListCtrl *page =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(i));
    if (!page) {
      continue;
    }

    long searchId = page->GetSearchId();
    if (searchId == id || id == 0) { // 0: just update all pages (there is only
                                     // one KAD search running at a time anyway)
      // Check if this is a Kad search
      wxString searchType = m_stateManager.GetSearchType(searchId);
      if (searchType == wxT("Kad")) {
        // Check if the search is still in "Searching" state
        // If so, don't mark it as complete to avoid race conditions
        SearchState currentState = STATE_IDLE;
        if (m_stateManager.HasSearch(searchId)) {
          currentState = m_stateManager.GetSearchState(searchId);
        }

        if (currentState == STATE_SEARCHING) {
          // Search is still active, don't mark it as complete
          AddDebugLogLineN(logSearch, CFormat(wxT("KadSearchEnd: Search %u is still searching, skipping completion"))
              % searchId);
          continue;
        }

        // Update result count in state manager
        size_t shown = page->GetItemCount();
        size_t hidden = page->GetHiddenItemCount();
        m_stateManager.UpdateResultCount(searchId, shown, hidden);

        // Check if we need to retry (no results and retry count not exceeded)
        if (shown == 0 && hidden == 0) {
          // Request retry through state manager
          if (m_stateManager.RequestRetry(searchId)) {
            // Trigger the actual retry
            OnRetryRequested(searchId);
            // Retry initiated, don't mark as finished yet
            continue;
          }
        }

        // End the search in the state manager (only if not retrying)
        m_stateManager.EndSearch(searchId);

        // Mark search as inactive in cache (allows future duplicate searches)
        if (m_searchCache.IsEnabled()) {
          m_searchCache.UpdateSearch(searchId, false);
        }
      }
    }
  }
}

void CSearchDlg::OnBnClickedDownload(wxCommandEvent &WXUNUSED(evt)) {
  int sel = m_notebook->GetSelection();
  if (sel != -1) {
    CSearchListCtrl *list =
        dynamic_cast<CSearchListCtrl *>(m_notebook->GetPage(sel));

    // Download with items added to category specified in the drop-down menu
    list->DownloadSelected();
  }
}

void CSearchDlg::OnBnClickedClear(wxCommandEvent &WXUNUSED(event)) {
  if (m_notebook->GetPageCount() > 0) {
    CSearchListCtrl *list = static_cast<CSearchListCtrl *>(
        m_notebook->GetPage(m_notebook->GetSelection()));
    list->DeleteAllItems();
    UpdateHitCount(list);
  }
}

void CSearchDlg::OnBnClickedMore(wxCommandEvent &WXUNUSED(event)) {
  // Get the currently selected search tab
  if (m_notebook->GetPageCount() == 0) {
    wxMessageBox(_("No search tabs available."), _("Search Error"), wxOK | wxICON_ERROR);
    return;
  }

  CSearchListCtrl *list = static_cast<CSearchListCtrl *>(
      m_notebook->GetPage(m_notebook->GetSelection()));

  // Get all information directly from the active tab
  long searchId = list->GetSearchId();
  wxString searchType = list->GetSearchType();

  AddDebugLogLineN(logSearch, CFormat(wxT("OnBnClickedMore: searchId=%ld, searchType='%s', list=%p"))
      % searchId % searchType % (void*)list);

  // Debug logging with detailed information
  AddDebugLogLineN(logSearch,
                   CFormat(wxT("More button clicked: searchId=%ld, searchType='%s'"))
                       % searchId % searchType);
  AddDebugLogLineN(logSearch,
                   CFormat(wxT("SearchManager has search: %s"))
                       % (m_stateManager.HasSearch(searchId) ? wxT("yes") : wxT("no")));

  // Check if we have a valid search ID
  if (searchId == 0) {
    wxMessageBox(_("Invalid search ID. The selected tab may not be a valid search."),
                 _("Search Error"), wxOK | wxICON_ERROR);
    return;
  }

  // Determine the search type
  bool isKadSearch = (searchType == wxT("Kad"));
  bool isLocalSearch = (searchType == wxT("Local"));
  bool isGlobalSearch = (searchType == wxT("Global"));

  // More button now works for all search types (Local, Global, Kad)
  if (!isLocalSearch && !isGlobalSearch && !isKadSearch) {
    wxMessageBox(CFormat(wxT("Unknown search type: '%s'.\n\n"
                          "The 'More' button only works for Local, Global, and Kad searches."))
                % searchType,
                _("Search Error"), wxOK | wxICON_ERROR);
    return;
  }

  // Get search parameters from SearchStateManager
  CSearchList::CSearchParams params;
  AddDebugLogLineN(logSearch, CFormat(wxT("Attempting to get search parameters for search ID %ld..."))
      % searchId);

  // Check if search exists in state manager
  bool hasSearchInStateManager = m_stateManager.HasSearch(searchId);
  AddDebugLogLineN(logSearch, CFormat(wxT("Search exists in StateManager: %s"))
      % (hasSearchInStateManager ? wxT("yes") : wxT("no")));

  // Try to get parameters from StateManager
  bool gotParamsFromStateManager = m_stateManager.GetSearchParams(searchId, params);
  AddDebugLogLineN(logSearch, CFormat(wxT("Got parameters from StateManager: %s"))
      % (gotParamsFromStateManager ? wxT("yes") : wxT("no")));

  if (!gotParamsFromStateManager) {
    // Build detailed diagnostic information
    wxString diagnosticInfo = CFormat(wxT(
        "=== DIAGNOSTIC INFORMATION ===\n\n"
        "Search ID: %ld\n"
        "Search Type: %s\n"
        "Search exists in StateManager: %s\n\n"
        "=== POSSIBLE CAUSES ===\n\n"
        "1. The search was not properly initialized\n"
        "2. The search parameters were cleared when the search ended\n"
        "3. The search was removed from the search manager\n"
        "4. The tab may have been closed and reopened\n\n"
        "=== RECOMMENDED ACTIONS ===\n\n"
        "- Try starting a new search with the same parameters\n"
        "- If this happens repeatedly, please report this bug\n"
        "- Check the debug log for more details\n\n"
        "=== DEBUG DETAILS ===\n\n"))
        % searchId
        % searchType
        % (hasSearchInStateManager ? wxT("Yes") : wxT("No"));

    AddDebugLogLineN(logSearch, wxT("Failed to get search parameters from StateManager!"));
    wxMessageBox(diagnosticInfo, _("Search Error - No Parameters Available"), wxOK | wxICON_ERROR);
    return;
  }

  if (params.searchString.IsEmpty()) {
    // Build detailed diagnostic information for empty search string
    wxString diagnosticInfo = CFormat(wxT(
        "=== DIAGNOSTIC INFORMATION ===\n\n"
        "Search ID: %ld\n"
        "Search Type: %s\n"
        "Search String: [EMPTY]\n\n"
        "=== POSSIBLE CAUSES ===\n\n"
        "1. The search was initialized with an empty search string\n"
        "2. The search string was cleared after initialization\n"
        "3. There is a bug in parameter storage/retrieval\n\n"
        "=== RECOMMENDED ACTIONS ===\n\n"
        "- Try starting a new search with valid parameters\n"
        "- Check the debug log for more details\n\n"
        "=== DEBUG DETAILS ===\n\n"))
        % searchId
        % searchType;

    diagnosticInfo += CFormat(wxT(
        "Retrieved parameters from StateManager:\n"
        "  searchString: '%s'\n"
        "  strKeyword: '%s'\n"
        "  typeText: '%s'\n"
        "  extension: '%s'\n"
        "  minSize: %llu\n"
        "  maxSize: %llu\n"
        "  availability: %d\n"
        "  searchType: %d\n\n"))
        % params.searchString
        % params.strKeyword
        % params.typeText
        % params.extension
        % params.minSize
        % params.maxSize
        % params.availability
        % (int)params.searchType;

    AddDebugLogLineN(logSearch, CFormat(wxT("Search string is empty for search ID %ld"))
        % searchId);
    wxMessageBox(diagnosticInfo, _("Search Error - Empty Search String"), wxOK | wxICON_ERROR);
    return;
  }

  // Store the original search ID before making any changes
  long originalSearchId = searchId;

  // Store the search parameters in SearchList's m_searchParams before requesting more results
  // This ensures that RequestMoreResults can find the parameters
  AddDebugLogLineN(logSearch, CFormat(wxT("Storing search parameters in SearchList for search ID %ld"))
      % searchId);
  // Use UnifiedSearchManager for all search types (Local, Global, Kad)
  // This provides a consistent API for requesting more results
  AddDebugLogLineN(logSearch, CFormat(wxT("Requesting more results via UnifiedSearchManager for search ID %ld, type='%s'"))
      % searchId % searchType);

  wxString error;
  bool success = m_unifiedSearchManager.requestMoreResults(searchId, error);

  if (!success) {
    wxMessageBox(CFormat(wxT("Failed to request more results:\n\n%s")) % error,
                 _("Search Error"), wxOK | wxICON_ERROR);
    return;
  }

  AddDebugLogLineN(logSearch, CFormat(wxT("Successfully requested more results for search ID %ld"))
      % searchId);

  // Disable buttons during the new search
  FindWindow(IDC_STARTS)->Disable();
  FindWindow(IDC_SDOWNLOAD)->Disable();
  FindWindow(IDC_CANCELS)->Enable();

  // Get the current tab index for text manipulation
  int currentTab = m_notebook->GetSelection();
  wxString originalTabText = m_notebook->GetPageText(currentTab);

  // Save the original tab text before modifying it - use search ID as key
  m_originalTabTexts[searchId] = originalTabText;

  // Track this "More" button search for timeout detection - use search ID as key
  m_moreButtonSearches[searchId] = wxDateTime::Now();

  // Update the tab text to reflect that we're requesting more results
  // Include the current hit count
  size_t shown = list->GetItemCount();
  size_t hidden = list->GetHiddenItemCount();

  // Build the new tab text with hit count and "updating" status
  wxString newText = originalTabText.BeforeLast(wxT('('));
  if (hidden > 0) {
    newText += wxString::Format(wxT(" (%zu + %zu hidden) (updating...)"),
                                shown, hidden);
  } else {
    newText += wxString::Format(wxT(" (%zu) (updating...)"), shown);
  }
  m_notebook->SetPageText(currentTab, newText);
}

void CSearchDlg::StartNewSearch() {
  // Use mutex to prevent race conditions in search creation
  wxMutexLocker creationLock(m_searchCreationMutex);

  FindWindow(IDC_STARTS)->Disable();
  FindWindow(IDC_SDOWNLOAD)->Disable();
  FindWindow(IDC_CANCELS)->Enable();

  CSearchList::CSearchParams params;

  params.searchString = CastChild(IDC_SEARCHNAME, wxTextCtrl)->GetValue();
  params.searchString.Trim(true);
  params.searchString.Trim(false);

  if (params.searchString.IsEmpty()) {
    return;
  }

  if (CastChild(IDC_EXTENDEDSEARCHCHECK, wxCheckBox)->GetValue()) {
    params.extension =
        CastChild(IDC_EDITSEARCHEXTENSION, wxTextCtrl)->GetValue();

    uint32 sizemin = GetTypeSize(
        (uint8)CastChild(IDC_SEARCHMINSIZE, wxChoice)->GetSelection());
    uint32 sizemax = GetTypeSize(
        (uint8)CastChild(IDC_SEARCHMAXSIZE, wxChoice)->GetSelection());

    // Parameter Minimum Size
    params.minSize =
        (uint64_t)(CastChild(IDC_SPINSEARCHMIN, wxSpinCtrl)->GetValue()) *
        (uint64_t)sizemin;

    // Parameter Maximum Size
    params.maxSize =
        (uint64_t)(CastChild(IDC_SPINSEARCHMAX, wxSpinCtrl)->GetValue()) *
        (uint64_t)sizemax;

    if ((params.maxSize < params.minSize) && (params.maxSize)) {
      wxMessageDialog dlg(
          this, _("Min size must be smaller than max size. Max size ignored."),
          _("Search warning"), wxOK | wxCENTRE | wxICON_INFORMATION);
      dlg.ShowModal();

      params.maxSize = 0;
    }

    // Parameter Availability
    params.availability =
        CastChild(IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl)->GetValue();

    switch (CastChild(IDC_TypeSearch, wxChoice)->GetSelection()) {
    case 0:
      params.typeText.Clear();
      break;
    case 1:
      params.typeText = ED2KFTSTR_ARCHIVE;
      break;
    case 2:
      params.typeText = ED2KFTSTR_AUDIO;
      break;
    case 3:
      params.typeText = ED2KFTSTR_CDIMAGE;
      break;
    case 4:
      params.typeText = ED2KFTSTR_IMAGE;
      break;
    case 5:
      params.typeText = ED2KFTSTR_PROGRAM;
      break;
    case 6:
      params.typeText = ED2KFTSTR_DOCUMENT;
      break;
    case 7:
      params.typeText = ED2KFTSTR_VIDEO;
      break;
    default:
      AddDebugLogLineC(
          logGeneral,
          CFormat(wxT("Warning! Unknown search-category (%s) selected!")) %
              params.typeText);
      break;
    }
  }

  SearchType search_type = KadSearch;

  int selection = CastChild(ID_SEARCHTYPE, wxChoice)->GetSelection();

  // Update selection accounting for removed BitTorrent and Hybrid search
  // options
  if (thePrefs::GetNetworkED2K() && thePrefs::GetNetworkKademlia()) {
    // Full network support - only 3 options available now (Local, Global, Kad)
    switch (selection) {
    case 0:
      search_type = LocalSearch;
      break;
    case 1:
      search_type = GlobalSearch;
      break;
    case 2:
      search_type = KadSearch;
      break;
    default:
      wxFAIL;
      break;
    }
  } else if (thePrefs::GetNetworkED2K()) {
    // Only ED2K support - 2 options (Local, Global)
    switch (selection) {
    case 0:
      search_type = LocalSearch;
      break;
    case 1:
      search_type = GlobalSearch;
      break;
    default:
      wxFAIL;
      break;
    }
  } else if (thePrefs::GetNetworkKademlia()) {
    // Only Kad support - 1 option (Kad)
    switch (selection) {
    case 0:
      search_type = KadSearch;
      break;
    default:
      wxFAIL;
      break;
    }
  } else {
    // No network support
    AddLogLineC(_("No networks are enabled."));
    return;
  }

  // Check if an identical search already exists
  uint32_t existingSearchId = 0;
  if (m_searchCache.IsEnabled() &&
      m_searchCache.FindExistingSearch(search_type, params.searchString, params, existingSearchId)) {
    // Duplicate search found - reuse existing tab and request more results
    AddDebugLogLineN(logSearch, CFormat(wxT("Duplicate search detected: type=%d, query='%s', existingId=%u"))
        % (int)search_type % params.searchString % existingSearchId);

    // Find the existing tab
    CSearchListCtrl* existingTab = GetSearchList(existingSearchId);
    if (existingTab) {
      // Switch to the existing tab
      int tabIndex = m_notebook->FindPage(existingTab);
      if (tabIndex != wxNOT_FOUND) {
        m_notebook->SetSelection(tabIndex);
      }

      // Check if the search is still in "Searching" state
      // If so, don't request more results to avoid race conditions
      SearchState currentState = STATE_IDLE;
      if (m_stateManager.HasSearch(existingSearchId)) {
        currentState = m_stateManager.GetSearchState(existingSearchId);
      }

      if (currentState == STATE_SEARCHING) {
        // Search is still active, just switch to the tab without requesting more results
        AddDebugLogLineN(logSearch, CFormat(wxT("Duplicate search detected but search is still active (ID=%u), just switching tab"))
            % existingSearchId);
      } else {
        // Search has completed, request more results
        wxString error;
        m_unifiedSearchManager.requestMoreResults(existingSearchId, error);
      }

      // Re-enable the start button (since we're not creating a new search)
      FindWindow(IDC_STARTS)->Enable();
      FindWindow(IDC_SDOWNLOAD)->Disable();
      FindWindow(IDC_CANCELS)->Disable();

      return;  // Don't create a new search
    } else {
      // Tab not found (may have been closed), remove from cache and continue
      m_searchCache.RemoveSearch(existingSearchId);
    }
  }
  // Use UnifiedSearchManager for all search types
  search::ModernSearchType modernSearchType;
  wxString searchTypeStr;
  wxString prefix;

  switch (search_type) {
  case LocalSearch:
    modernSearchType = search::ModernSearchType::LocalSearch;
    searchTypeStr = wxT("Local");
    prefix = wxT("Local: ");
    break;
  case GlobalSearch:
    modernSearchType = search::ModernSearchType::GlobalSearch;
    searchTypeStr = wxT("Global");
    prefix = wxT("Global: ");
    break;
  case KadSearch:
    modernSearchType = search::ModernSearchType::KadSearch;
    searchTypeStr = wxT("Kad");
    prefix = wxT("Kad: ");
    break;
  default:
    modernSearchType = search::ModernSearchType::LocalSearch;
    searchTypeStr = wxT("Local");
    prefix = wxT("Local: ");
    break;
  }

  // Create SearchParams for the new architecture
  search::SearchParams searchParams;
  searchParams.searchString = params.searchString;
  searchParams.typeText = params.typeText;
  searchParams.extension = params.extension;
  searchParams.minSize = params.minSize;
  searchParams.maxSize = params.maxSize;
  searchParams.availability = params.availability;
  searchParams.searchType = modernSearchType;

  // For Kad searches, extract the keyword from the search string
  if (search_type == KadSearch) {
    Kademlia::WordList words;
    Kademlia::CSearchManager::GetWords(params.searchString, &words);
    if (!words.empty()) {
      searchParams.strKeyword = words.front();
      AddDebugLogLineC(logSearch, CFormat(wxT("SearchDlg::StartNewSearch: Kad keyword extracted: '%s' from search string: '%s'"))
          % searchParams.strKeyword % params.searchString);
    } else {
      AddDebugLogLineC(logSearch, CFormat(wxT("SearchDlg::StartNewSearch: No keyword extracted from search string: '%s'"))
          % params.searchString);
      wxMessageBox(_("No keyword for Kad search - aborting"),
                   _("Search error"), wxOK | wxCENTRE | wxICON_ERROR, this);
      FindWindow(IDC_STARTS)->Enable();
      FindWindow(IDC_SDOWNLOAD)->Disable();
      FindWindow(IDC_CANCELS)->Disable();
      return;
    }
  } else {
    // For non-Kad searches, just use the search string as keyword
    searchParams.strKeyword = params.searchString;
  }

  // Start the search using UnifiedSearchManager
  wxString error;
  uint32 real_id = m_unifiedSearchManager.startSearch(searchParams, error);

  if (!error.IsEmpty() || real_id == 0) {
    wxMessageBox(error.IsEmpty() ? _("Failed to start search") : error,
                 _("Search error"), wxOK | wxCENTRE | wxICON_ERROR, this);
    FindWindow(IDC_STARTS)->Enable();
    FindWindow(IDC_SDOWNLOAD)->Disable();
    FindWindow(IDC_CANCELS)->Disable();
    return;
  }

  // Create a new tab for this search
  CreateNewTab(prefix + params.searchString, real_id);

  // Initialize the search in SearchStateManager
  m_stateManager.InitializeSearch(real_id, searchTypeStr, params.searchString,
                                  params);

  // Register the search in the cache for duplicate detection
  if (m_searchCache.IsEnabled()) {
    m_searchCache.RegisterSearch(real_id, search_type, params.searchString, params);
  }
}

void CSearchDlg::UpdateHitCount(CSearchListCtrl *page) {
  if (!page) {
    return;
  }

  // Get the search ID
  long searchId = page->GetSearchId();
  if (searchId == 0) {
    return;
  }

  // Update result count in SearchStateManager
  size_t shown = page->GetItemCount();
  size_t hidden = page->GetHiddenItemCount();

  // Log the hit count values for debugging
  SEARCH_DEBUG_COUNT(
      CFormat(wxT("UpdateHitCount: searchId=%ld, shown=%u, hidden=%u")) %
      searchId % shown % hidden);

  // Ensure the search exists in state manager before updating
  if (!m_stateManager.HasSearch(searchId)) {
    // Search not initialized yet - this shouldn't happen but handle it
    // gracefully
    SEARCH_DEBUG_COUNT(CFormat(wxT("UpdateHitCount: Search ID %ld not found in "
                                   "state manager, skipping update")) %
                       searchId);
    return;
  }

  m_stateManager.UpdateResultCount(searchId, shown, hidden);

  // Update the tab label with current state from SearchStateManager
  SearchState state = m_stateManager.GetSearchState(searchId);
  int retryCount = m_stateManager.GetRetryCount(searchId);

  wxString stateStr;
  switch (state) {
  case STATE_SEARCHING:
    stateStr = wxT("Searching");
    break;
  case STATE_RETRYING:
    stateStr = (CFormat(wxT("Retrying %d")) % retryCount).GetString();
    break;
  case STATE_NO_RESULTS:
    stateStr = wxT("No Results");
    break;
  case STATE_HAS_RESULTS:
  case STATE_POPULATING:
  case STATE_IDLE:
    stateStr = wxEmptyString;
    break;
  }

  // Update the tab label with state information using counts from
  // SearchStateManager
  UpdateSearchStateWithCount(page, this, stateStr, shown, hidden);
}

void CSearchDlg::OnSearchStateChanged(uint32_t searchId, SearchState state,
                                      int retryCount) {
  // Find the search list control for this search ID
  CSearchListCtrl *list = GetSearchList(searchId);
  if (!list) {
    return;
  }

  // Convert state to string
  wxString stateStr;
  switch (state) {
  case STATE_SEARCHING:
    stateStr = wxT("Searching");
    break;
  case STATE_RETRYING:
    stateStr = (CFormat(wxT("Retrying %d")) % retryCount).GetString();
    break;
  case STATE_NO_RESULTS:
    stateStr = wxT("No Results");
    break;
  case STATE_HAS_RESULTS:
  case STATE_POPULATING:
  case STATE_IDLE:
    stateStr = wxEmptyString;
    break;
  }

  // Get the result counts from SearchStateManager
  size_t shown, hidden;
  m_stateManager.GetResultCount(searchId, shown, hidden);

  // Update the tab label with state information and correct counts
  UpdateSearchStateWithCount(list, this, stateStr, shown, hidden);
}

bool CSearchDlg::OnRetryRequested(uint32_t searchId) {
  // Find the search list control for this search ID
  CSearchListCtrl *list = GetSearchList(searchId);
  if (!list) {
    return false;
  }

  // Get the search type from SearchStateManager
  wxString searchType = m_stateManager.GetSearchType(searchId);

  // Reset state to Searching before triggering retry
  // This ensures the UI shows "Searching" instead of jumping to "No Results"
  m_stateManager.UpdateState(searchId, STATE_SEARCHING);

  // Retry based on search type
  if (searchType == wxT("Kad")) {
    return RetryKadSearchWithState(list, this);
  } else if (searchType == wxT("Local") || searchType == wxT("ED2K") ||
             searchType == wxT("Global")) {
    return RetrySearchWithState(list, this);
  }

  return false;
}

void CSearchDlg::UpdateTabLabelWithState(CSearchListCtrl *list,
                                         const wxString &state) {
  // Validate inputs
  if (!list || !m_notebook) {
    return;
  }

  // Check if the dialog is being destroyed
  if (IsBeingDeleted()) {
    return;
  }

  // Find the tab index for this list control
  int tabIndex = m_notebook->FindPage(list);
  if (tabIndex == wxNOT_FOUND) {
    // Tab no longer exists, skip update
    return;
  }

  // Get the search type from the list control (stored variable)
  wxString searchType = list->GetSearchType();

  // If search type is not set, parse it from current tab text for backward
  // compatibility
  if (searchType.IsEmpty()) {
    wxString tabText = m_notebook->GetPageText(tabIndex);
    assert(!tabText.IsEmpty());

    // Parse search type from tab text
    if (tabText.StartsWith(wxT("[Local] "))) {
      searchType = wxT("Local");
    } else if (tabText.StartsWith(wxT("[Global] "))) {
      searchType = wxT("Global");
    } else if (tabText.StartsWith(wxT("[Kad] "))) {
      searchType = wxT("Kad");
    }

    // Store the parsed search type for future use
    list->SetSearchType(searchType);
  }

  // Get the keyword from SearchStateManager
  long searchId = list->GetSearchId();
  wxString keyword = m_stateManager.GetKeyword(searchId);
  if (keyword.IsEmpty()) {
    // Fallback: get keyword from current tab text
    wxString tabText = m_notebook->GetPageText(tabIndex);
    // Remove type prefix
    if (tabText.StartsWith(wxT("[Local] "))) {
      tabText = tabText.Mid(8);
    } else if (tabText.StartsWith(wxT("[Global] "))) {
      tabText = tabText.Mid(8);
    } else if (tabText.StartsWith(wxT("[Kad] "))) {
      tabText = tabText.Mid(6);
    }
    // Remove state prefix
    if (tabText.StartsWith(wxT("["))) {
      size_t stateEnd = tabText.Find(wxT("]"));
      if (stateEnd != wxString::npos) {
        tabText = tabText.Mid(stateEnd + 2);
      }
    }
    // Remove count suffix
    int parenPos = tabText.Find(wxT(" ("));
    if (parenPos != wxNOT_FOUND) {
      tabText = tabText.Left(parenPos);
    }
    keyword = tabText.Trim();
  }

  // Log the values for debugging
  theLogger.AddLogLine(wxT("SearchDlg.cpp"), __LINE__, false, logStandard,
                       CFormat(wxT("UpdateTabLabelWithState: state='%s', "
                                   "searchType='%s', keyword='%s'")) %
                           state % searchType % keyword);

  // Build the new tab text using stored search type
  wxString newText;

  // Add search type prefix
  if (searchType == wxT("Local")) {
    newText = wxT("[Local] ");
  } else if (searchType == wxT("Global")) {
    newText = wxT("[Global] ");
  } else if (searchType == wxT("Kad")) {
    newText = wxT("[Kad] ");
  }

  // Add state if provided
  if (!state.IsEmpty()) {
    newText += wxT("[") + state + wxT("] ");
  }

  // Add the keyword
  newText += keyword;

  // Get the result counts
  size_t shown = list->GetItemCount();
  size_t hidden = list->GetHiddenItemCount();

  // Validate counts - hidden should not exceed shown
  assert(shown >= hidden);

  // Add count information
  // Always show count when there is a state (e.g., "No Results", "Retrying
  // 1") or when there are actual results
  if (!state.IsEmpty() || shown > 0 || hidden > 0) {
    if (hidden) {
      newText +=
          (CFormat(wxT(" (%u/%u)")) % shown % (shown + hidden)).GetString();
    } else {
      newText += (CFormat(wxT(" (%u)")) % shown).GetString();
    }
  }

  // Log the final tab text for debugging
  theLogger.AddLogLine(
      wxT("SearchDlg.cpp"), __LINE__, false, logStandard,
      CFormat(wxT("UpdateTabLabelWithState: Setting tab text to '%s'")) %
          newText);

  m_notebook->SetPageText(tabIndex, newText);
}

// UpdateSearchState is now implemented as an external helper function in
// SearchLabelHelper.cpp

void CSearchDlg::OnBnClickedReset(wxCommandEvent &WXUNUSED(evt)) {
  CastChild(IDC_SEARCHNAME, wxTextCtrl)->Clear();
  CastChild(IDC_EDITSEARCHEXTENSION, wxTextCtrl)->Clear();
  CastChild(IDC_SPINSEARCHMIN, wxSpinCtrl)->SetValue(0);
  CastChild(IDC_SEARCHMINSIZE, wxChoice)->SetSelection(2);
  CastChild(IDC_SPINSEARCHMAX, wxSpinCtrl)->SetValue(0);
  CastChild(IDC_SEARCHMAXSIZE, wxChoice)->SetSelection(2);
  CastChild(IDC_SPINSEARCHAVAIBILITY, wxSpinCtrl)->SetValue(0);
  CastChild(IDC_TypeSearch, wxChoice)->SetSelection(0);
  CastChild(ID_AUTOCATASSIGN, wxChoice)->SetSelection(0);

  FindWindow(IDC_SEARCH_RESET)->Enable(FALSE);
}

void CSearchDlg::UpdateCatChoice() {
  wxChoice *c_cat = CastChild(ID_AUTOCATASSIGN, wxChoice);
  c_cat->Clear();

  c_cat->Append(_("Main"));

  for (unsigned i = 1; i < theApp->glob_prefs->GetCatCount(); i++) {
    c_cat->Append(theApp->glob_prefs->GetCategory(i)->title);
  }

  c_cat->SetSelection(0);
}

void CSearchDlg::UpdateProgress(uint32 new_value) {
  m_progressbar->SetValue(new_value);
}

void CSearchDlg::OnTimeoutCheck(wxTimerEvent &event) {
  wxDateTime now = wxDateTime::Now();
  const int TIMEOUT_SECONDS = 30; // 30 second timeout

  // Check for timed-out "More" button searches
  for (auto it = m_moreButtonSearches.begin();
       it != m_moreButtonSearches.end();) {
    uint32_t searchId = it->first;
    wxDateTime startTime = it->second;

    wxTimeSpan elapsed = now - startTime;

    if (elapsed.GetSeconds().ToLong() >= TIMEOUT_SECONDS) {
      // Timeout occurred
      HandleMoreButtonTimeout(searchId);
      it = m_moreButtonSearches.erase(it);
    } else {
      ++it;
    }
  }

  // Check for Kad searches that have finished but not yet marked as complete
  // Iterate through all notebook tabs to find active searches
  for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
    CSearchListCtrl* list = static_cast<CSearchListCtrl*>(m_notebook->GetPage(i));
    if (list) {
      long searchId = list->GetSearchId();
      if (searchId > 0) {
        SearchState state = m_stateManager.GetSearchState(searchId);
        wxString searchType = m_stateManager.GetSearchType(searchId);
        
        if (state == STATE_SEARCHING && searchType == wxT("Kad")) {
          // Convert to Kad search ID format (0xffffff??)
          uint32_t kadSearchId = 0xffffff00 | (searchId & 0xff);
          
          // Check if Kad search is still active in Kademlia subsystem
          bool isKadStillSearching = Kademlia::CSearchManager::IsSearching(kadSearchId);
          
          if (!isKadStillSearching) {
            // Kad search has finished in Kademlia subsystem
            // Check if we have results
            search::SearchModel* searchModel = m_unifiedSearchManager.getSearchModel(searchId);
            if (searchModel) {
              size_t resultCount = searchModel->getResultCount();
              AddDebugLogLineC(logSearch, CFormat(wxT("SearchDlg::OnTimeoutCheck: Kad search %u finished in Kademlia, has %zu results"))
                  % searchId % resultCount);
              
              // Update state manager with result count
              m_stateManager.UpdateResultCount(searchId, resultCount, 0);
              
              // Mark search as complete
              m_stateManager.EndSearch(searchId);
              
              AddDebugLogLineC(logSearch, CFormat(wxT("SearchDlg::OnTimeoutCheck: Kad search %u marked as complete"))
                  % searchId);
            } else {
              AddDebugLogLineC(logSearch, CFormat(wxT("SearchDlg::OnTimeoutCheck: Kad search %u finished but no model found"))
                  % searchId);
              // Still mark as complete to avoid stuck state
              m_stateManager.EndSearch(searchId);
            }
          }
        }
      }
    }
  }
}

void CSearchDlg::HandleMoreButtonTimeout(uint32 searchId) {
  // Find the tab index for this search ID
  int tabIndex = -1;
  for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
    CSearchListCtrl* list = static_cast<CSearchListCtrl*>(m_notebook->GetPage(i));
    if (list && list->GetSearchId() == (long)searchId) {
      tabIndex = i;
      break;
    }
  }

  // Check if we found the tab
  if (tabIndex < 0) {
    // Tab no longer exists, clean up
    m_originalTabTexts.erase(searchId);
    return;
  }

  // Restore the original tab text
  auto it = m_originalTabTexts.find(searchId);
  if (it != m_originalTabTexts.end()) {
    m_notebook->SetPageText(tabIndex, it->second);
    m_originalTabTexts.erase(it);
  } else {
    // Fallback: remove "updating..." from current text
    wxString tabText = m_notebook->GetPageText(tabIndex);
    if (tabText.Contains(wxT("(updating...)"))) {
      tabText.Replace(wxT("(updating...)"), wxT(""));
      m_notebook->SetPageText(tabIndex, tabText);
    }
  }

  // Re-enable buttons
  FindWindow(IDC_STARTS)->Enable();
  FindWindow(IDC_SDOWNLOAD)->Enable();
  FindWindow(IDC_CANCELS)->Disable();
}

void CSearchDlg::OnSearchTypeChanged(wxCommandEvent &evt) {
  // Call the base event handler
  UpdateStartButtonState();
  evt.Skip();
}

// File_checked_for_headers
