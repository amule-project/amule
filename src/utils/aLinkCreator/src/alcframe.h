//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         AlcFrame Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Copyright (C) 2004 by Phoenix
///
/// Pixmaps from http://jimmac.musichall.cz/ikony.php3 | http://www.everaldo.com | http://www.icomania.com
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _ALCFRAME_H
#define _ALCFRAME_H

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/statline.h>
#include <wx/progdlg.h>


// Compute and display md4sum or not
//#define WANT_MD4SUM 1


/// Main Alc Frame
class AlcFrame:public wxFrame
  {
  private:
    wxToolBar *m_toolbar;
    wxBitmap m_toolBarBitmaps[4];

    wxBoxSizer *m_frameVBox;

    wxPanel *m_mainPanel;
    wxBoxSizer *m_mainPanelVBox;

    wxStaticLine *m_staticLine;

    wxStaticBox *m_inputSBox;
    wxStaticBoxSizer* m_inputSBoxSizer;
    wxFlexGridSizer *m_inputFlexSizer;
    wxStaticText *m_inputFileStaticText;
    wxTextCtrl *m_inputFileTextCtrl;
    wxButton *m_inputFileBrowseButton ;
    wxStaticText *m_inputAddStaticText;
    wxTextCtrl *m_inputAddTextCtrl;
    wxButton *m_inputAddButton ;
    wxListBox *m_inputUrlListBox;
    wxBoxSizer *m_buttonUrlVBox;
    wxButton *m_removeButton;
    wxButton *m_clearButton;
    wxCheckBox *m_parthashesCheck;

    wxProgressDialog *m_progressBar;
    bool m_goAhead;


#ifdef WANT_MD4SUM

    wxStaticBox *m_md4HashSBox;
    wxStaticBoxSizer* m_md4HashSBoxSizer;
    wxTextCtrl *m_md4HashTextCtrl;
#endif

    wxStaticBox *m_e2kHashSBox;
    wxStaticBoxSizer* m_e2kHashSBoxSizer;
    wxTextCtrl *m_e2kHashTextCtrl;

    wxStaticBox *m_ed2kSBox;
    wxStaticBoxSizer* m_ed2kSBoxSizer;
    wxTextCtrl *m_ed2kTextCtrl;

    wxBoxSizer* m_buttonHBox;
    wxButton *m_copyButton;
    wxButton *m_startButton;
    wxButton *m_saveButton;
    wxButton *m_closeButton;

    enum
    {
      ID_BAR_OPEN = 1000,
      ID_BAR_SAVEAS,
      ID_BAR_COPY,
      ID_BAR_ABOUT,
      ID_START_BUTTON,
      ID_SAVEAS_BUTTON,
      ID_COPY_BUTTON,
      ID_EXIT_BUTTON,
      ID_BROWSE_BUTTON,
      ID_ADD_BUTTON,
      ID_REMOVE_BUTTON,
      ID_CLEAR_BUTTON,
      ID_PARTHASHES_CHECK
    };

    /// Set File to hash in wxTextCtrl
    void SetFileToHash();

    /// Save computed Ed2k link to file
    void SaveEd2kLinkToFile();

    /// Copy Ed2k link to clip board
    void CopyEd2kLinkToClipBoard();

  protected:

    /// Toolbar Open button
    void OnBarOpen (wxCommandEvent & event);

    /// Toolbar Save As button
    void OnBarSaveAs (wxCommandEvent & event);

    /// Toolbar Copy button
    void OnBarCopy(wxCommandEvent & event);

    /// Toolbar About button
    void OnBarAbout (wxCommandEvent & event);

    /// Close Button
    void OnCloseButton (wxCommandEvent & event);

    /// Save As button
    void OnSaveAsButton(wxCommandEvent & event);

    /// Copy button
    void OnCopyButton(wxCommandEvent & event);

    /// Compute Hashes on Start Button
    void OnStartButton (wxCommandEvent & event);

    /// Browse button to select file to hash
    void OnBrowseButton (wxCommandEvent & event);

    /// Add an URL to the URL list box
    void OnAddUrlButton (wxCommandEvent & event);

    /// Remove the selected URL from the URL list box
    void OnRemoveUrlButton (wxCommandEvent & event);

    /// Clear the URL list box
    void OnClearUrlButton (wxCommandEvent & event);

    DECLARE_EVENT_TABLE ()

  public:
    /// Constructor
    AlcFrame (const wxString& title);

    /// Destructor
    ~AlcFrame ();

    // Hook function for external update of the progress bar
    static bool Hook(int percent);
  };

#endif /* _ALCFRAME_H */
