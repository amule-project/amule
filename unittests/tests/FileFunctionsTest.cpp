#include <muleunit/test.h>
#include <FileFunctions.h>

#include <wx/filename.h>

using namespace muleunit;

DECLARE_SIMPLE(FileFunctions)

TEST(FileFunctions, JoinPaths)
{
	const wxString sep = wxFileName::GetPathSeparators();

	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + sep, wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), sep + wxT("b")));
	ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + sep, sep + wxT("b")));
	ASSERT_EQUALS(wxT("a"), JoinPaths(wxT("a"), wxEmptyString));
	ASSERT_EQUALS(wxT("b"), JoinPaths(wxEmptyString, wxT("b")));
	ASSERT_EQUALS(wxEmptyString, JoinPaths(wxEmptyString, wxEmptyString));
}

