#include <wx/arrstr.h>

#include <muleunit/test.h>
#include <common/TextFile.h>
#include <common/Path.h>

using namespace muleunit;

DECLARE_SIMPLE(TextFile)

const char* g_filesDefault[] = {
	"TextFileTest_dos.txt",
	"TextFileTest_unix.txt"
};


wxString ArrToStr(const wxArrayString& arr)
{
	wxString str = "[";

	for (size_t i = 0; i < arr.Count(); ++i) {
		if (str != "[") {
			str << ", \"" << arr[i] << '"';
		} else {
			str << '"' << arr[i] << '"';
		}
	}

	str << "]";

	return str;
}


wxString ArrToStr(size_t count, const char* arr[])
{
	return ArrToStr(wxArrayString(count, arr));
}



void CompareReadLines(size_t count, const char* expected[], EReadTextFile criteria)
{
	CTextFile file;
	ASSERT_FALSE(file.IsOpened());
	ASSERT_TRUE(file.Eof());
	for (size_t j = 0; j < ArraySize(g_filesDefault); ++j) {
		CONTEXT(wxString("Checking file: ") + g_filesDefault[j]);

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
	ASSERT_TRUE(CPath::DirExists(wxSTRINGIZE_T(SRCDIR)));

	{
		CONTEXT("Checking default parameters");

		const char* lines[] = {
			"abc",
			"def ghi",
			"xyz",
		};

		CompareReadLines(ArraySize(lines), lines, txtReadDefault);
	}

	{
		CONTEXT("Checking without criteria");

		const char* lines[] = {
			" # comment",
			"abc",
			"# foo bar",
			" ",
			"def ghi ",
			"",
			"# xyz",
			" xyz",
			" ",
			""
		};

		CompareReadLines(ArraySize(lines), lines, (EReadTextFile)0);
	}

	{
		CONTEXT("Checking txtIgnoreEmptyLines");

		const char* lines[] = {
			" # comment",
			"abc",
			"# foo bar",
			" ",
			"def ghi ",
			"# xyz",
			" xyz",
			" ",
		};

		CompareReadLines(ArraySize(lines), lines, txtIgnoreEmptyLines);
	}

	{
		CONTEXT("Checking txtIgnoreComments");

		const char* lines[] = {
			"abc",
			" ",
			"def ghi ",
			"",
			" xyz",
			" ",
			""
		};

		CompareReadLines(ArraySize(lines), lines, txtIgnoreComments);
	}

	{
		CONTEXT("Checking txtStripWhitespace");

		const char* lines[] = {
			"# comment",
			"abc",
			"# foo bar",
			"",
			"def ghi",
			"",
			"# xyz",
			"xyz",
			"",
			""
		};

		CompareReadLines(ArraySize(lines), lines, txtStripWhitespace);
	}
}


class TextFileTest : public Test
{
public:
	TextFileTest()
		: Test("TextFile", "WriteLines")
	{
	}

	virtual void setUp()
	{
		const CPath path = CPath("testfile.txt");
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
		const char* lines[] = {
			" # comment",
			"abc",
			"# foo bar",
			" ",
			"def ghi ",
			"",
			"# xyz",
			" xyz",
			" ",
			""
		};

		{
			CONTEXT("Writing lines manually");

			CTextFile file;
			ASSERT_TRUE(file.Open("testfile.txt", CTextFile::write));

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format("Writing the %i'th line.", static_cast<int>(i)));

				ASSERT_TRUE(file.WriteLine(lines[i]));
			}
		}

		{
			CONTEXT("Reading manually written lines");

			CTextFile file;
			ASSERT_TRUE(file.Open("testfile.txt", CTextFile::read));
			ASSERT_FALSE(file.Eof());

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format("Reading the %i'th line.", static_cast<int>(i)));

				ASSERT_EQUALS(lines[i], file.GetNextLine());
			}
			ASSERT_TRUE(file.Eof());
		}

		{
			CONTEXT("Writing lines automatically");

			CTextFile file;
			ASSERT_FALSE(file.IsOpened());
			ASSERT_TRUE(file.Open("testfile.txt", CTextFile::write));
			ASSERT_TRUE(file.WriteLines(wxArrayString(ArraySize(lines), lines)));
			ASSERT_TRUE(file.IsOpened());
		}

		{
			CONTEXT("Reading automatically written lines");

			CTextFile file;
			ASSERT_FALSE(file.IsOpened());
			ASSERT_TRUE(file.Open("testfile.txt", CTextFile::read));
			ASSERT_TRUE(file.IsOpened());
			ASSERT_FALSE(file.Eof());

			for (size_t i = 0; i < ArraySize(lines); ++i) {
				CONTEXT(wxString::Format("Reading the %i'th line.", static_cast<int>(i)));

				ASSERT_EQUALS(lines[i], file.GetNextLine());
			}

			ASSERT_TRUE(file.Eof());
		}
	}
} testInstance;
