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
	{ true, wxConvFileName->cMB2WC("\xe1\x62\x63"), wxT("\xe1\x62\x63") },
	{ true, wxConvFileName->cMB2WC("\xe6\xf8\xe5"), wxT("\xe6\xf8\xe5") },
	{ true, wxConvFileName->cMB2WC("\xd8\xa7\xd9\x84\xd8\xb9"), wxT("\u0627\u0644\u0639") },

	// From User
	{ false, wxT("\u0627\u0644\u0639"), wxT("\u0627\u0644\u0639") }
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
		wxCharBuffer fn = wxConvFile.cWC2MB(wxT("\u0627\u0644\u0639"));

		if (fn) {
			encoding = UE_NonBroken;
		} else {
			encoding = UE_Broken;
		}
	}

	if ((encoding == UE_Broken) && !wxConvFile.cWC2MB(src)) {
		// See CPath::CPath for rationale ...
		wxCharBuffer fn = wxConvUTF8.cWC2MB(src);
		return wxConvFile.cMB2WC(fn);
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

		ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), wxT("b")));
		ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + cur_sep, wxT("b")));
		ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a"), cur_sep + wxT("b")));
		ASSERT_EQUALS(wxT("a") + sep + wxT("b"), JoinPaths(wxT("a") + cur_sep, cur_sep + wxT("b")));
		ASSERT_EQUALS(wxT("a"), JoinPaths(wxT("a"), wxEmptyString));
		ASSERT_EQUALS(wxT("b"), JoinPaths(wxEmptyString, wxT("b")));
		ASSERT_EQUALS(wxEmptyString, JoinPaths(wxEmptyString, wxEmptyString));
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

	ASSERT_EQUALS(wxEmptyString, tmp.GetRaw());
	ASSERT_EQUALS(wxEmptyString, tmp.GetPrintable());
	ASSERT_EQUALS(CPath(), tmp.GetPath());
	ASSERT_EQUALS(CPath(), tmp.GetFullName());
}


TEST(CPath, PathConstructor)
{
	{
		CPath tmp(wxT(""));

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
	const wxChar* tmpPath = wxT("foobar.tgz");

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
	const wxChar* tmpPath1 = wxT("foobar.tgz");
	const wxChar* tmpPath2 = wxT("barfoo.tar");

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
		CPath a = CPath(wxT("/home/amule"));
		CPath b = CPath(wxT("/hone/amule/"));

		ASSERT_EQUALS(a, b);
	}
#endif

	// TODO: Less than
}


/** Return the path normalized for the current platform. */
CPath Norm(wxString str)
{
	str.Replace(wxT("/"), wxString(wxFileName::GetPathSeparator()));

	return CPath(str);
}


TEST(CPath, JoinPaths)
{
	const CPath expected1 = Norm(wxT("/home/amule/"));
	const CPath expected2 = Norm(wxT("/home/amule"));

	// Note: Just a few checks, as ::JoinPaths is tested above
	ASSERT_EQUALS(expected1, Norm(wxT("/home")).JoinPaths(Norm(wxT("amule/"))));
	ASSERT_EQUALS(expected1, Norm(wxT("/home")).JoinPaths(Norm(wxT("/amule/"))));
	ASSERT_EQUALS(expected1, Norm(wxT("/home/")).JoinPaths(Norm(wxT("/amule/"))));
	ASSERT_EQUALS(expected1, Norm(wxT("/home/")).JoinPaths(Norm(wxT("amule/"))));

	ASSERT_EQUALS(expected2, Norm(wxT("/home")).JoinPaths(Norm(wxT("amule"))));
	ASSERT_EQUALS(expected2, Norm(wxT("/home")).JoinPaths(Norm(wxT("/amule"))));
	ASSERT_EQUALS(expected2, Norm(wxT("/home/")).JoinPaths(Norm(wxT("/amule"))));
	ASSERT_EQUALS(expected2, Norm(wxT("/home/")).JoinPaths(Norm(wxT("amule"))));

	ASSERT_EQUALS(expected1, expected1.JoinPaths(CPath()));
	ASSERT_EQUALS(expected1, CPath().JoinPaths(expected1));

	ASSERT_EQUALS(expected2, expected2.JoinPaths(CPath()));
	ASSERT_EQUALS(expected2, CPath().JoinPaths(expected2));
}


TEST(CPath, StartsWith)
{
	const CPath test = Norm(wxT("/home/amule/"));

	ASSERT_FALSE(test.StartsWith(CPath()));
	ASSERT_FALSE(CPath().StartsWith(test));

	ASSERT_TRUE(test.StartsWith(Norm(wxT("/"))));
	ASSERT_TRUE(test.StartsWith(Norm(wxT("/home"))));
	ASSERT_TRUE(test.StartsWith(Norm(wxT("/home/"))));
	ASSERT_TRUE(test.StartsWith(Norm(wxT("/home/amule"))));
	ASSERT_TRUE(test.StartsWith(Norm(wxT("/home/amule/"))));

	ASSERT_FALSE(test.StartsWith(Norm(wxT(""))));
	ASSERT_FALSE(test.StartsWith(Norm(wxT("~"))));
	ASSERT_FALSE(test.StartsWith(Norm(wxT("/ho"))));
	ASSERT_FALSE(test.StartsWith(Norm(wxT("/home/am"))));
	ASSERT_FALSE(test.StartsWith(Norm(wxT("/home/amule2"))));
	ASSERT_FALSE(test.StartsWith(Norm(wxT("/home/amule/foo"))));
}


TEST(CPath, IsSameDir)
{
	ASSERT_TRUE(Norm(wxT("/root")).IsSameDir(Norm(wxT("/root"))));
	ASSERT_TRUE(Norm(wxT("/root/")).IsSameDir(Norm(wxT("/root"))));
	ASSERT_TRUE(Norm(wxT("/root")).IsSameDir(Norm(wxT("/root/"))));
	ASSERT_TRUE(Norm(wxT("/root/")).IsSameDir(Norm(wxT("/root/"))));

	ASSERT_FALSE(CPath().IsSameDir(Norm(wxT("/"))));
	ASSERT_FALSE(Norm(wxT("/")).IsSameDir(CPath()));

	ASSERT_FALSE(Norm(wxT("/root")).IsSameDir(Norm(wxT("/home"))));
}


TEST(CPath, GetPath_FullName)
{
	{
		ASSERT_EQUALS(CPath(), CPath().GetPath());
		ASSERT_EQUALS(CPath(), CPath().GetFullName());
	}

	{
		const CPath path = Norm(wxT("/home/mule/"));

		ASSERT_EQUALS(Norm(wxT("/home/mule")), path.GetPath());
		ASSERT_EQUALS(CPath(), path.GetFullName());
	}

	{
		const CPath path = Norm(wxT("/home/mule"));

		ASSERT_EQUALS(Norm(wxT("/home")), path.GetPath());
		ASSERT_EQUALS(Norm(wxT("mule")), path.GetFullName());
	}

	{
		const CPath path = Norm(wxT("mule"));

		ASSERT_EQUALS(CPath(), path.GetPath());
		ASSERT_EQUALS(Norm(wxT("mule")), path.GetFullName());
	}

	{
		const CPath path = Norm(wxT("mule.ext"));

		ASSERT_EQUALS(CPath(), path.GetPath());
		ASSERT_EQUALS(Norm(wxT("mule.ext")), path.GetFullName());
	}
}


TEST(CPath, Cleanup)
{
	const CPath initial = CPath(wxT(" /a\"b*c* <d>?e|\\:f "));

	ASSERT_EQUALS(Norm(wxT("\%20a\"b*c*\%20<d>?e|\\:f\%20")), initial.Cleanup(false, false));
	ASSERT_EQUALS(Norm(wxT("\%20abc\%20def\%20")), initial.Cleanup(false, true));
	ASSERT_EQUALS(Norm(wxT(" a\"b*c* <d>?e|\\:f ")), initial.Cleanup(true, false));
	ASSERT_EQUALS(Norm(wxT(" abc def ")), initial.Cleanup(true, true));
}


TEST(CPath, AddPostFix)
{
	ASSERT_EQUALS(Norm(wxT("/foo_1.bar")), Norm(wxT("/foo.bar")).AddPostfix(wxT("_1")));
	ASSERT_EQUALS(Norm(wxT("/foo.bar")), Norm(wxT("/foo.bar")).AddPostfix(wxT("")));
	ASSERT_EQUALS(Norm(wxT("/.bar_1")), Norm(wxT("/.bar")).AddPostfix(wxT("_1")));
	ASSERT_EQUALS(Norm(wxT("/_1")), Norm(wxT("/")).AddPostfix(wxT("_1")));
}


TEST(CPath, Extensions)
{
	const CPath initial = Norm(wxT("/home/amule.foo.bar"));

	ASSERT_EQUALS(Norm(wxT("/home/amule.foo")), initial.RemoveExt());
	ASSERT_EQUALS(Norm(wxT("/home/amule")), initial.RemoveExt().RemoveExt());
	ASSERT_EQUALS(Norm(wxT("/home/amule")), initial.RemoveExt().RemoveExt().RemoveExt());
	ASSERT_EQUALS(Norm(wxT("/home/amule")), initial.RemoveAllExt());

	ASSERT_EQUALS(wxT("bar"), initial.GetExt());
	ASSERT_EQUALS(wxT("foo"), initial.RemoveExt().GetExt());
	ASSERT_EQUALS(wxEmptyString, initial.RemoveExt().RemoveExt().GetExt());
	ASSERT_EQUALS(wxEmptyString, initial.RemoveAllExt().GetExt());

	ASSERT_EQUALS(Norm(wxT("/home/amule.foo.bar")), initial.AppendExt(wxT("")));
	ASSERT_EQUALS(Norm(wxT("/home/amule.foo.bar.zod")), initial.AppendExt(wxT("zod")));
	ASSERT_EQUALS(Norm(wxT("/home/amule.foo.bar.zod")), initial.AppendExt(wxT(".zod")));
	ASSERT_EQUALS(Norm(wxT("/home/amule.zod")), initial.RemoveAllExt().AppendExt(wxT("zod")));
	ASSERT_EQUALS(Norm(wxT("/home/amule.zod")), initial.RemoveAllExt().AppendExt(wxT(".zod")));
}

TEST(CPath, TruncatePath)
{
	const CPath testPath = Norm(wxT("/home/amule/truncate"));

	ASSERT_EQUALS(Norm(wxT("/home/amule/truncate")).GetPrintable(), testPath.TruncatePath(21));
	ASSERT_EQUALS(Norm(wxT("/home/amule/truncate")).GetPrintable(), testPath.TruncatePath(20));
	ASSERT_EQUALS(Norm(wxT("/home/amule/tr[...]")).GetPrintable(), testPath.TruncatePath(19));
	ASSERT_EQUALS(Norm(wxT("/h[...]")).GetPrintable(), testPath.TruncatePath(7));
	ASSERT_EQUALS(Norm(wxT("/[...]")).GetPrintable(), testPath.TruncatePath(6));
	ASSERT_EQUALS(wxEmptyString, testPath.TruncatePath(5));
	ASSERT_EQUALS(wxEmptyString, testPath.TruncatePath(4));

	ASSERT_EQUALS(Norm(wxT("/home/amule/truncate")).GetPrintable(), testPath.TruncatePath(21, true));
	ASSERT_EQUALS(Norm(wxT("/home/amule/truncate")).GetPrintable(), testPath.TruncatePath(20, true));
	ASSERT_EQUALS(Norm(wxT("[...]amule/truncate")).GetPrintable(), testPath.TruncatePath(19, true));
	ASSERT_EQUALS(Norm(wxT("[...]e/truncate")).GetPrintable(), testPath.TruncatePath(15, true));
	ASSERT_EQUALS(wxT("truncate"), testPath.TruncatePath(14, true));
	ASSERT_EQUALS(wxT("truncate"), testPath.TruncatePath(13, true));
	ASSERT_EQUALS(wxT("truncate"), testPath.TruncatePath(9, true));
	ASSERT_EQUALS(wxT("truncate"), testPath.TruncatePath(8, true));
	ASSERT_EQUALS(wxT("tr[...]"), testPath.TruncatePath(7, true));
	ASSERT_EQUALS(wxT("t[...]"), testPath.TruncatePath(6, true));
	ASSERT_EQUALS(wxEmptyString, testPath.TruncatePath(5, true));
}
