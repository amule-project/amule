#include <muleunit/test.h>

#include <common/Path.h>
#include <common/StringFunctions.h>

#include <wx/filename.h>

using namespace muleunit;

#if 0
/*
 * The validity of this test is at least questionable. The result depends on
 * - the current system locale and character encoding used, and
 * - the way wxConvFileName reports error conditions and its fallback behaviour.

struct STestStr
{
	//! Specifies the "source" of the string (user/fs)
	const bool fromFS;
	//! Input filename
	const wxString input;
	//! The expected result.
	const wxString expected;
};

const STestStr g_fromFSTests[] = {
	// From filesystem
	{ true, wxConvFileName->cMB2WC("\xe1\x62\x63"), "\xe1\x62\x63" },
	{ true, wxConvFileName->cMB2WC("\xe6\xf8\xe5"), "\xe6\xf8\xe5" },
	{ true, wxConvFileName->cMB2WC("\xd8\xa7\xd9\x84\xd8\xb9"), "\u0627\u0644\u0639" },

	// From User
	{ false, "\u0627\u0644\u0639", "\u0627\u0644\u0639" }
};

enum UsesEncoding {
	UE_Unknown,
	UE_Broken,
	UE_NonBroken
};

wxString GetExpectedString(const wxString& src)
{
	// Determine how the filenames are expected to be mangled
	static UsesEncoding encoding = UE_Unknown;

	if (encoding == UE_Unknown) {
		wxCharBuffer fn = wxConvFileName->cWC2MB("\u0627\u0644\u0639");

		if (fn) {
			encoding = UE_NonBroken;
		} else {
			encoding = UE_Broken;
		}
	}

	if ((encoding == UE_Broken) && !wxConvFileName->cWC2MB(src)) {
		// See CPath::CPath for rationale ...
		wxCharBuffer fn = wxConvUTF8.cWC2MB(src);
		return wxConvFileName->cMB2WC(fn);
	} else {
		return src;
	}
}
*/
#endif

wxString StringFrom(const CPath& prt)
{
	return prt.GetPrintable();
}


DECLARE_SIMPLE(GenericPathFunctions)

TEST(GenericPathFunctions, JoinPaths)
{
	const wxString seps = wxFileName::GetPathSeparators();
	const wxString sep = wxFileName::GetPathSeparator();

	for (size_t i = 0; i < seps.Length(); ++i) {
		const wxString cur_sep = seps.Mid(i, 1);

		ASSERT_EQUALS("a" + sep + "b", JoinPaths("a", "b"));
		ASSERT_EQUALS("a" + sep + "b", JoinPaths("a" + cur_sep, "b"));
		ASSERT_EQUALS("a" + sep + "b", JoinPaths("a", cur_sep + "b"));
		ASSERT_EQUALS("a" + sep + "b", JoinPaths("a" + cur_sep, cur_sep + "b"));
		ASSERT_EQUALS("a", JoinPaths("a", ""));
		ASSERT_EQUALS("b", JoinPaths("", "b"));
		ASSERT_EQUALS("", JoinPaths("", ""));
	}
}


DECLARE_SIMPLE(CPath)

TEST(CPath, DefaultConstructor)
{
	CPath tmp;

	ASSERT_FALSE(tmp.IsOk());
	ASSERT_EQUALS(tmp, CPath());

	ASSERT_FALSE(tmp.FileExists());
	ASSERT_FALSE(tmp.DirExists());

	ASSERT_EQUALS("", tmp.GetRaw());
	ASSERT_EQUALS("", tmp.GetPrintable());
	ASSERT_EQUALS(CPath(), tmp.GetPath());
	ASSERT_EQUALS(CPath(), tmp.GetFullName());
}


TEST(CPath, PathConstructor)
{
	{
		CPath tmp("");

		ASSERT_FALSE(tmp.IsOk());
		ASSERT_EQUALS(tmp, CPath());
	}

#if 0
/*
 * See the note above

	for (size_t i = 0; i < ArraySize(g_fromFSTests); ++i) {
		const wxString input = g_fromFSTests[i].input;
		const wxString result = g_fromFSTests[i].expected;

		ASSERT_TRUE(result.Length());
		ASSERT_TRUE(input.Length());

		CPath tmp(input);

		ASSERT_TRUE(tmp.IsOk());
		ASSERT_EQUALS(CPath(input), tmp);

		ASSERT_EQUALS(::GetExpectedString(input), tmp.GetRaw());
		ASSERT_EQUALS(result, tmp.GetPrintable());

		ASSERT_EQUALS(tmp, tmp.GetFullName());
		ASSERT_EQUALS(CPath(), tmp.GetPath());
	}
*/
#endif
}


TEST(CPath, CopyConstructor)
{
	const char* tmpPath = "foobar.tgz";

	{
		CPath a(tmpPath);

		ASSERT_TRUE(a.IsOk());
		ASSERT_EQUALS(tmpPath, a.GetRaw());
		ASSERT_EQUALS(tmpPath, a.GetPrintable());
		ASSERT_EQUALS(CPath(), a.GetPath());
		ASSERT_EQUALS(CPath(tmpPath), a.GetFullName());

		CPath b(a);
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(tmpPath, b.GetRaw());
		ASSERT_EQUALS(tmpPath, b.GetPrintable());
		ASSERT_EQUALS(CPath(), b.GetPath());
		ASSERT_EQUALS(CPath(tmpPath), b.GetFullName());

		ASSERT_EQUALS(a, b);
	}

	{
		CPath a;
		CPath b(a);

		ASSERT_EQUALS(a, b);
		ASSERT_FALSE(a.IsOk());
		ASSERT_FALSE(b.IsOk());
	}
}


TEST(CPath, Operators)
{
	const char* tmpPath1 = "foobar.tgz";
	const char* tmpPath2 = "barfoo.tar";

	{
		CPath a, b;
		ASSERT_EQUALS(a, b);
		ASSERT_FALSE(a.IsOk());
		ASSERT_FALSE(b.IsOk());

		a = CPath(tmpPath1);
		ASSERT_EQUALS(tmpPath1, a.GetRaw());
		ASSERT_EQUALS(tmpPath1, a.GetPrintable());
		ASSERT_EQUALS(CPath(), a.GetPath());
		ASSERT_EQUALS(CPath(tmpPath1), a.GetFullName());
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(a != b);

		b = a;
		ASSERT_EQUALS(tmpPath1, b.GetRaw());
		ASSERT_EQUALS(tmpPath1, b.GetPrintable());
		ASSERT_EQUALS(CPath(), b.GetPath());
		ASSERT_EQUALS(CPath(tmpPath1), b.GetFullName());
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(a, b);

		a = CPath(tmpPath2);
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_TRUE(a != b);

		ASSERT_EQUALS(tmpPath2, a.GetRaw());
		ASSERT_EQUALS(tmpPath2, a.GetPrintable());
		ASSERT_EQUALS(CPath(), a.GetPath());
		ASSERT_EQUALS(CPath(tmpPath2), a.GetFullName());
		ASSERT_EQUALS(tmpPath1, b.GetRaw());
		ASSERT_EQUALS(tmpPath1, b.GetPrintable());
		ASSERT_EQUALS(CPath(), b.GetPath());
		ASSERT_EQUALS(CPath(tmpPath1), b.GetFullName());
	}

// See note in CPath::operator==
#if 0
	{
		CPath a = CPath("/home/amule");
		CPath b = CPath("/hone/amule/");

		ASSERT_EQUALS(a, b);
	}
#endif

	// TODO: Less than
}


/** Return the path normalized for the current platform. */
CPath Norm(wxString str)
{
	str.Replace("/", wxString(wxFileName::GetPathSeparator()));

	return CPath(str);
}


TEST(CPath, JoinPaths)
{
	const CPath expected1 = Norm("/home/amule/");
	const CPath expected2 = Norm("/home/amule");

	// Note: Just a few checks, as ::JoinPaths is tested above
	ASSERT_EQUALS(expected1, Norm("/home").JoinPaths(Norm("amule/")));
	ASSERT_EQUALS(expected1, Norm("/home").JoinPaths(Norm("/amule/")));
	ASSERT_EQUALS(expected1, Norm("/home/").JoinPaths(Norm("/amule/")));
	ASSERT_EQUALS(expected1, Norm("/home/").JoinPaths(Norm("amule/")));

	ASSERT_EQUALS(expected2, Norm("/home").JoinPaths(Norm("amule")));
	ASSERT_EQUALS(expected2, Norm("/home").JoinPaths(Norm("/amule")));
	ASSERT_EQUALS(expected2, Norm("/home/").JoinPaths(Norm("/amule")));
	ASSERT_EQUALS(expected2, Norm("/home/").JoinPaths(Norm("amule")));

	ASSERT_EQUALS(expected1, expected1.JoinPaths(CPath()));
	ASSERT_EQUALS(expected1, CPath().JoinPaths(expected1));

	ASSERT_EQUALS(expected2, expected2.JoinPaths(CPath()));
	ASSERT_EQUALS(expected2, CPath().JoinPaths(expected2));
}


TEST(CPath, StartsWith)
{
	const CPath test = Norm("/home/amule/");

	ASSERT_FALSE(test.StartsWith(CPath()));
	ASSERT_FALSE(CPath().StartsWith(test));

	ASSERT_TRUE(test.StartsWith(Norm("/")));
	ASSERT_TRUE(test.StartsWith(Norm("/home")));
	ASSERT_TRUE(test.StartsWith(Norm("/home/")));
	ASSERT_TRUE(test.StartsWith(Norm("/home/amule")));
	ASSERT_TRUE(test.StartsWith(Norm("/home/amule/")));

	ASSERT_FALSE(test.StartsWith(Norm("")));
	ASSERT_FALSE(test.StartsWith(Norm("~")));
	ASSERT_FALSE(test.StartsWith(Norm("/ho")));
	ASSERT_FALSE(test.StartsWith(Norm("/home/am")));
	ASSERT_FALSE(test.StartsWith(Norm("/home/amule2")));
	ASSERT_FALSE(test.StartsWith(Norm("/home/amule/foo")));
}


TEST(CPath, IsSameDir)
{
	ASSERT_TRUE(Norm("/root").IsSameDir(Norm("/root")));
	ASSERT_TRUE(Norm("/root/").IsSameDir(Norm("/root")));
	ASSERT_TRUE(Norm("/root").IsSameDir(Norm("/root/")));
	ASSERT_TRUE(Norm("/root/").IsSameDir(Norm("/root/")));

	ASSERT_FALSE(CPath().IsSameDir(Norm("/")));
	ASSERT_FALSE(Norm("/").IsSameDir(CPath()));

	ASSERT_FALSE(Norm("/root").IsSameDir(Norm("/home")));
}


TEST(CPath, GetPath_FullName)
{
	{
		ASSERT_EQUALS(CPath(), CPath().GetPath());
		ASSERT_EQUALS(CPath(), CPath().GetFullName());
	}

	{
		const CPath path = Norm("/home/mule/");

		ASSERT_EQUALS(Norm("/home/mule"), path.GetPath());
		ASSERT_EQUALS(CPath(), path.GetFullName());
	}

	{
		const CPath path = Norm("/home/mule");

		ASSERT_EQUALS(Norm("/home"), path.GetPath());
		ASSERT_EQUALS(Norm("mule"), path.GetFullName());
	}

	{
		const CPath path = Norm("mule");

		ASSERT_EQUALS(CPath(), path.GetPath());
		ASSERT_EQUALS(Norm("mule"), path.GetFullName());
	}

	{
		const CPath path = Norm("mule.ext");

		ASSERT_EQUALS(CPath(), path.GetPath());
		ASSERT_EQUALS(Norm("mule.ext"), path.GetFullName());
	}
}


TEST(CPath, Cleanup)
{
	const CPath initial = CPath(" /a\"b*c* <d>?e|\\:f ");

	ASSERT_EQUALS(Norm("\%20a\"b*c*\%20<d>?e|\\:f\%20"), initial.Cleanup(false, false));
	ASSERT_EQUALS(Norm("\%20abc\%20def\%20"), initial.Cleanup(false, true));
	ASSERT_EQUALS(Norm(" a\"b*c* <d>?e|\\:f "), initial.Cleanup(true, false));
	ASSERT_EQUALS(Norm(" abc def "), initial.Cleanup(true, true));
}


TEST(CPath, AddPostFix)
{
	ASSERT_EQUALS(Norm("/foo_1.bar"), Norm("/foo.bar").AddPostfix("_1"));
	ASSERT_EQUALS(Norm("/foo.bar"), Norm("/foo.bar").AddPostfix(""));
	ASSERT_EQUALS(Norm("/.bar_1"), Norm("/.bar").AddPostfix("_1"));
	ASSERT_EQUALS(Norm("/_1"), Norm("/").AddPostfix("_1"));
}


TEST(CPath, Extensions)
{
	const CPath initial = Norm("/home/amule.foo.bar");

	ASSERT_EQUALS(Norm("/home/amule.foo"), initial.RemoveExt());
	ASSERT_EQUALS(Norm("/home/amule"), initial.RemoveExt().RemoveExt());
	ASSERT_EQUALS(Norm("/home/amule"), initial.RemoveExt().RemoveExt().RemoveExt());
	ASSERT_EQUALS(Norm("/home/amule"), initial.RemoveAllExt());

	ASSERT_EQUALS("bar", initial.GetExt());
	ASSERT_EQUALS("foo", initial.RemoveExt().GetExt());
	ASSERT_EQUALS("", initial.RemoveExt().RemoveExt().GetExt());
	ASSERT_EQUALS("", initial.RemoveAllExt().GetExt());

	ASSERT_EQUALS(Norm("/home/amule.foo.bar"), initial.AppendExt(""));
	ASSERT_EQUALS(Norm("/home/amule.foo.bar.zod"), initial.AppendExt("zod"));
	ASSERT_EQUALS(Norm("/home/amule.foo.bar.zod"), initial.AppendExt(".zod"));
	ASSERT_EQUALS(Norm("/home/amule.zod"), initial.RemoveAllExt().AppendExt("zod"));
	ASSERT_EQUALS(Norm("/home/amule.zod"), initial.RemoveAllExt().AppendExt(".zod"));
}

TEST(CPath, TruncatePath)
{
	const CPath testPath = Norm("/home/amule/truncate");

	ASSERT_EQUALS(Norm("/home/amule/truncate").GetPrintable(), testPath.TruncatePath(21));
	ASSERT_EQUALS(Norm("/home/amule/truncate").GetPrintable(), testPath.TruncatePath(20));
	ASSERT_EQUALS(Norm("/home/amule/tr[...]").GetPrintable(), testPath.TruncatePath(19));
	ASSERT_EQUALS(Norm("/h[...]").GetPrintable(), testPath.TruncatePath(7));
	ASSERT_EQUALS(Norm("/[...]").GetPrintable(), testPath.TruncatePath(6));
	ASSERT_EQUALS("", testPath.TruncatePath(5));
	ASSERT_EQUALS("", testPath.TruncatePath(4));

	ASSERT_EQUALS(Norm("/home/amule/truncate").GetPrintable(), testPath.TruncatePath(21, true));
	ASSERT_EQUALS(Norm("/home/amule/truncate").GetPrintable(), testPath.TruncatePath(20, true));
	ASSERT_EQUALS(Norm("[...]amule/truncate").GetPrintable(), testPath.TruncatePath(19, true));
	ASSERT_EQUALS(Norm("[...]e/truncate").GetPrintable(), testPath.TruncatePath(15, true));
	ASSERT_EQUALS("truncate", testPath.TruncatePath(14, true));
	ASSERT_EQUALS("truncate", testPath.TruncatePath(13, true));
	ASSERT_EQUALS("truncate", testPath.TruncatePath(9, true));
	ASSERT_EQUALS("truncate", testPath.TruncatePath(8, true));
	ASSERT_EQUALS("tr[...]", testPath.TruncatePath(7, true));
	ASSERT_EQUALS("t[...]", testPath.TruncatePath(6, true));
	ASSERT_EQUALS("", testPath.TruncatePath(5, true));
}
