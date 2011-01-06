#include <wx/arrstr.h>

#include <muleunit/test.h>
#include <common/TextFile.h>
#include <common/Path.h>

using namespace muleunit;

DECLARE_SIMPLE(TextFile)

const wxChar* g_filesDefault[] = {
	wxT("TextFileTest_dos.txt"),
	wxT("TextFileTest_unix.txt")
};


wxString ArrToStr(const wxArrayString& arr)
{
	wxString str = wxT("[");

	for (size_t i = 0; i < arr.Count(); ++i) {
		if (str != wxT("[")) {
			str << wxT(", \"") << arr[i] << wxT('"');
		} else {
			str << wxT('"') << arr[i] << wxT('"');
		}
	}

	str << wxT("]");

	return str;
}


wxString ArrToStr(size_t count, const wxChar* arr[])
{
	return ArrToStr(wxArrayString(count, arr));
}



void CompareReadLines(size_t count, const wxChar* expected[], EReadTextFile criteria)
{
	CTextFile file;
	ASSERT_FALSE(file.IsOpened());
	ASSERT_TRUE(file.Eof());
	for (size_t j = 0; j < ArraySize(g_filesDefault); ++j) {
		CONTEXT(wxString(wxT("Checking file: ")) + g_filesDefault[j]);

		ASSERT_TRUE(file.Open(CPath(wxSTRINGIZE_T(SRCDIR)).JoinPaths(CPath(g_filesDefault[j])).GetRaw(), CTextFile::read));
		
		wxArrayString lines = file.ReadLines(criteria);
		
		ASSERT_EQUALS(ArrToStr(count, expected), ArrToStr(lines));
		ASSERT_EQUALS(count, lines.GetCount());
	}
	ASSERT_TRUE(file.IsOpened());
	ASSERT_TRUE(file.Eof());
};



TEST(TextFile, ReadLines)
{
	{
		CONTEXT(wxT("Checking default parameters"));

		const wxChar* lines[] = {
			wxT("abc"),
			wxT("def ghi"),
			wxT("xyz"),
		};

		CompareReadLines(ArraySize(lines), lines, txtReadDefault);
	}

	{
		CONTEXT(wxT("Checking without criteria"));

		const wxChar* lines[] = {
			wxT(" # comment"),
			wxT("abc"),
			wxT("# foo bar"),
			wxT(" "),
			wxT("def ghi "),
			wxT(""),
			wxT("# xyz"),
			wxT(" xyz"),
			wxT(" "),
			wxT("")
		};
		
		CompareReadLines(ArraySize(lines), lines, (EReadTextFile)0);
	}

	{
		CONTEXT(wxT("Checking txtIgnoreEmptyLines"));

		const wxChar* lines[] = {
			wxT(" # comment"),
			wxT("abc"),
			wxT("# foo bar"),
			wxT(" "),
			wxT("def ghi "),
			wxT("# xyz"),
			wxT(" xyz"),
			wxT(" "),
		};

		CompareReadLines(ArraySize(lines), lines, txtIgnoreEmptyLines);		
	}

	{
		CONTEXT(wxT("Checking txtIgnoreComments"));

		const wxChar* lines[] = {
			wxT("abc"),
			wxT(" "),
			wxT("def ghi "),
			wxT(""),
			wxT(" xyz"),
			wxT(" "),
			wxT("")
		};

		CompareReadLines(ArraySize(lines), lines, txtIgnoreComments);
	}

	{
		CONTEXT(wxT("Checking txtStripWhitespace"));

		const wxChar* lines[] = {
			wxT("# comment"),
			wxT("abc"),
			wxT("# foo bar"),
			wxT(""),
			wxT("def ghi"),
			wxT(""),
			wxT("# xyz"),
			wxT("xyz"),
			wxT(""),
			wxT("")
		};

		CompareReadLines(ArraySize(lines), lines, txtStripWhitespace);
	}
}


class TextFileTest : public Test
{
public:
	TextFileTest()
		: Test(wxT("TextFile"), wxT("WriteLines"))
	{
	}

	virtual void setUp()
	{
		const CPath path = CPath(wxT("testfile.txt"));
		if (path.FileExists()) {
			ASSERT_TRUE(CPath::RemoveFile(path));
		}

	}

	virtual void tearDown()
	{
		setUp();
	}

	virtual void run()
	{
		const wxChar* lines[] = {
			wxT(" # comment"),
			wxT("abc"),
			wxT("# foo bar"),
			wxT(" "),
			wxT("def ghi "),
			wxT(""),
			wxT("# xyz"),
			wxT(" xyz"),
			wxT(" "),
			wxT("")
		};

		{
			CONTEXT(wxT("Writing lines manually"));
				
			CTextFile file;
			ASSERT_TRUE(file.Open(wxT("testfile.txt"), CTextFile::write));

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format(wxT("Writing the %i'th line."), i));

				ASSERT_TRUE(file.WriteLine(lines[i]));
			}
		}

		{
			CONTEXT(wxT("Reading manually written lines"));

			CTextFile file;
			ASSERT_TRUE(file.Open(wxT("testfile.txt"), CTextFile::read));
			ASSERT_FALSE(file.Eof());

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format(wxT("Reading the %i'th line."), i));

				ASSERT_EQUALS(lines[i], file.GetNextLine());
			}
			ASSERT_TRUE(file.Eof());
		}

		{
			CONTEXT(wxT("Writing lines automatically"));
				
			CTextFile file;
			ASSERT_FALSE(file.IsOpened());
			ASSERT_TRUE(file.Open(wxT("testfile.txt"), CTextFile::write));
			ASSERT_TRUE(file.WriteLines(wxArrayString(ArraySize(lines), lines)));
			ASSERT_TRUE(file.IsOpened());
		}

		{
			CONTEXT(wxT("Reading automatically written lines"));

			CTextFile file;
			ASSERT_FALSE(file.IsOpened());
			ASSERT_TRUE(file.Open(wxT("testfile.txt"), CTextFile::read));
			ASSERT_TRUE(file.IsOpened());
			ASSERT_FALSE(file.Eof());

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format(wxT("Reading the %i'th line."), i));

				ASSERT_EQUALS(lines[i], file.GetNextLine());
			}

			ASSERT_TRUE(file.Eof());
		}
	}
} testInstance;

