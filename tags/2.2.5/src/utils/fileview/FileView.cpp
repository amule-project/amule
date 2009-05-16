//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2009 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include <wx/app.h>
#include <wx/cmdline.h>
#include <list>
#include "KadFiles.h"
#include "Print.h"
#include "../../CFile.h"

#define VERSION_MAJOR	0
#define VERSION_MINOR	1
#define VERSION_MICRO	0

class CFileView : public wxApp
{
      private:
	virtual int OnRun();

	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	std::list<wxString> m_files;
};

DECLARE_APP(CFileView);

// Logging facility from aMule
enum DebugType {};

namespace CLogger {

	bool IsEnabled(DebugType)
	{
		return true;
	}

	void AddLogLine(const wxString& /*file*/, int /*line*/, bool /*critical*/, const wxString& str)
	{
		DoPrint(wxT("Log: ") + str + wxT("\n"));
	}

	void AddLogLine(const wxString& /*file*/, int /*line*/, bool /*critical*/, DebugType /*type*/, const wxString& str)
	{
		DoPrint(wxT("DebugLog: ") + str + wxT("\n"));
	}

}


IMPLEMENT_APP(CFileView);

void CFileView::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.AddSwitch(wxT("h"), wxT("help"), wxT("Show help"), wxCMD_LINE_OPTION_HELP);
	parser.AddSwitch(wxT("v"), wxT("version"), wxT("Show program version"), wxCMD_LINE_PARAM_OPTIONAL);
	parser.AddParam(wxT("input file"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool CFileView::OnCmdLineParsed(wxCmdLineParser& parser)
{
	if (parser.Found(wxT("version"))) {
		DoPrint(wxString::Format(wxT("MuleFileView version %u.%u.%u\nCopyright (c) 2008-2009 aMule Team\n"), VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO));
		return false;
	} else {
		for (size_t n = 0; n < parser.GetParamCount(); n++) {
			m_files.push_back(parser.GetParam(n));
		}
		if (m_files.size() == 0) {
			parser.Usage();
			return false;
		} else {
			return true;
		}
	}
}

int CFileView::OnRun()
{
	for (std::list<wxString>::const_iterator it = m_files.begin(); it != m_files.end(); ++it) {
		wxString basename = (*it).AfterLast(wxFileName::GetPathSeparator()).AfterLast(wxT('/'));
		try {
			CFile file(*it);
			if (file.IsOpened()) {
				if (it != m_files.begin()) {
					DoPrint(wxT("\n"));
				}
				DoPrint(wxT("Decoding file: ") + (*it) + wxT("\n"));
				if (basename == wxT("preferencesKad.dat")) {
					DecodePreferencesKadDat(file);
				} else if (basename == wxT("load_index.dat")) {
					DecodeLoadIndexDat(file);
				} else if (basename == wxT("key_index.dat")) {
					DecodeKeyIndexDat(file);
				} else if (basename == wxT("src_index.dat")) {
					DecodeSourceIndexDat(file);
				} else if (basename == wxT("nodes.dat")) {
					DecodeNodesDat(file);
				} else {
					DoPrint(wxT("ERROR: Don't know how to decode ") + (*it) + wxT("\n"));
				}
			}
		} catch (const CEOFException& e) {
			DoPrint(wxT("ERROR: A CEOFException has been raised while decoding ") + (*it) + wxT(": ") + e.what() + wxT("\n"));
			return 1;
		} catch (const CIOFailureException& e) {
			DoPrint(wxT("ERROR: A CIOFailureException has been raised while decoding ") + (*it) + wxT(": ") + e.what() + wxT("\n"));
			return 1;
		} catch (const CSafeIOException& e) {
			DoPrint(wxT("ERROR: A CSafeIOException has been raised while decoding ") + (*it) + wxT(": ") + e.what() + wxT("\n"));
			return 1;
		} catch (const CMuleException& e) {
			DoPrint(wxT("ERROR: A CMuleException has been raised while decoding ") + (*it) + wxT(": ") + e.what() + wxT("\n"));
			return 1;
		} catch (const wxString& e) {
			DoPrint(wxT("ERROR: An exception of type wxString has been raised while decoding ") + (*it) + wxT(": ") + e + wxT("\n"));
			return 1;
		}
	}

	return 0;
}
