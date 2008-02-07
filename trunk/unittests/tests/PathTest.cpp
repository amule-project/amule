#include <muleunit/test.h>

#include <common/Path.h>
#include <common/StringFunctions.h>

using namespace muleunit;

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


wxString StringFrom(const CPath& prt)
{
	return prt.GetPrintableString();
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

	ASSERT_EQUALS(wxEmptyString, tmp.GetPrintableString());
}


TEST(CPath, PathConstructor)
{
	{
		CPath tmp(wxT(""));

		ASSERT_FALSE(tmp.IsOk());
		ASSERT_EQUALS(tmp, CPath());
	}
	

	for (size_t i = 0; i < ArraySize(g_fromFSTests); ++i) {
		const wxString input = g_fromFSTests[i].input;
		const wxString result = g_fromFSTests[i].expected;
		
		ASSERT_TRUE(result.Length());
		ASSERT_TRUE(input.Length());

		CPath tmp(input);

		ASSERT_TRUE(tmp.IsOk());
		ASSERT_EQUALS(tmp, CPath(input));

		ASSERT_EQUALS(tmp.GetRaw(), ::GetExpectedString(input));
		ASSERT_EQUALS(tmp.GetPrintable(), result);

		ASSERT_EQUALS(tmp.GetFullName(), tmp);
		ASSERT_EQUALS(tmp.GetPath(), CPath());
	}
}


TEST(CPath, CopyConstructor)
{
	const wxChar* tmpPath = wxT("foobar.tgz");

	{
		CPath a(tmpPath);

		ASSERT_TRUE(a.IsOk());
		ASSERT_EQUALS(a.GetRaw(), tmpPath);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath));

		CPath b(a);
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(b.GetRaw(), tmpPath);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath));

		ASSERT_EQUALS(a, b);
	}

	{
		CPath a(tmpPath);

		ASSERT_TRUE(a.IsOk());
		ASSERT_EQUALS(a.GetRaw(), tmpPath);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath));

		CPath b(a);
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(b.GetRaw(), tmpPath);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath));

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
		ASSERT_EQUALS(a.GetRaw(), tmpPath1);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath1));
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(a != b);

		b = a;
		ASSERT_EQUALS(b.GetRaw(), tmpPath1);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath1));
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(a, b);

		a = CPath(tmpPath2);
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_TRUE(a != b);

		ASSERT_EQUALS(a.GetRaw(), tmpPath2);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath2);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath2));
		ASSERT_EQUALS(b.GetRaw(), tmpPath1);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath1));
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


TEST(CPath, JoinPaths)
{
	const CPath expected1(wxT("/home/amule/"));
	const CPath expected2(wxT("/home/amule"));

	// Note: Just a few checks, as ::JoinPaths is tested in StringFunctionTests.cpp
	ASSERT_EQUALS(expected1, CPath(wxT("/home")).JoinPaths(CPath(wxT("amule/"))));
	ASSERT_EQUALS(expected1, CPath(wxT("/home")).JoinPaths(CPath(wxT("/amule/"))));
	ASSERT_EQUALS(expected1, CPath(wxT("/home/")).JoinPaths(CPath(wxT("/amule/"))));
	ASSERT_EQUALS(expected1, CPath(wxT("/home/")).JoinPaths(CPath(wxT("amule/"))));
	
	ASSERT_EQUALS(expected2, CPath(wxT("/home")).JoinPaths(CPath(wxT("amule"))));
	ASSERT_EQUALS(expected2, CPath(wxT("/home")).JoinPaths(CPath(wxT("/amule"))));
	ASSERT_EQUALS(expected2, CPath(wxT("/home/")).JoinPaths(CPath(wxT("/amule"))));
	ASSERT_EQUALS(expected2, CPath(wxT("/home/")).JoinPaths(CPath(wxT("amule"))));

	ASSERT_EQUALS(expected1, expected1.JoinPaths(CPath()));
	ASSERT_EQUALS(expected1, CPath().JoinPaths(expected1));
	
	ASSERT_EQUALS(expected2, expected2.JoinPaths(CPath()));
	ASSERT_EQUALS(expected2, CPath().JoinPaths(expected2));
}


TEST(CPath, StartsWith)
{
	const CPath test(wxT("/home/amule/"));

	ASSERT_FALSE(test.StartsWith(CPath()));
	ASSERT_FALSE(CPath().StartsWith(test));

	ASSERT_TRUE(test.StartsWith(CPath(wxT("/"))));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home"))));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/"))));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/amule"))));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/amule/"))));

	ASSERT_FALSE(test.StartsWith(CPath(wxT(""))));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("~"))));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/ho"))));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/am"))));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/amule2"))));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/amule/foo"))));
}


TEST(CPath, IsSameDir)
{
	ASSERT_TRUE(CPath(wxT("/root")).IsSameDir(CPath(wxT("/root"))));
	ASSERT_TRUE(CPath(wxT("/root/")).IsSameDir(CPath(wxT("/root"))));
	ASSERT_TRUE(CPath(wxT("/root")).IsSameDir(CPath(wxT("/root/"))));
	ASSERT_TRUE(CPath(wxT("/root/")).IsSameDir(CPath(wxT("/root/"))));
	
	ASSERT_FALSE(CPath().IsSameDir(CPath(wxT("/"))));
	ASSERT_FALSE(CPath(wxT("/")).IsSameDir(CPath()));

	ASSERT_FALSE(CPath(wxT("/root")).IsSameDir(CPath(wxT("/home"))));
}


TEST(CPath, GetPath_FullName)
{
	{
		ASSERT_EQUALS(CPath().GetPath(), CPath());
		ASSERT_EQUALS(CPath().GetFullName(), CPath());
	}

	{
		const CPath path = CPath(wxT("/home/mule/"));

		ASSERT_EQUALS(path.GetPath(), CPath(wxT("/home/mule")));
		ASSERT_EQUALS(path.GetFullName(), CPath());
	}

	{
		const CPath path = CPath(wxT("/home/mule"));

		ASSERT_EQUALS(path.GetPath(), CPath(wxT("/home")));
		ASSERT_EQUALS(path.GetFullName(), CPath(wxT("mule")));
	}

	{
		const CPath path = CPath(wxT("mule"));
		
		ASSERT_EQUALS(path.GetPath(), CPath());
		ASSERT_EQUALS(path.GetFullName(), CPath(wxT("mule")));
	}

	{
		const CPath path = CPath(wxT("mule.ext"));
		
		ASSERT_EQUALS(path.GetPath(), CPath());
		ASSERT_EQUALS(path.GetFullName(), CPath(wxT("mule.ext")));
	}
}


TEST(CPath, Cleanup)
{
	const CPath initial = CPath(wxT(" /a\"b*c* <d>?e|\\:f "));

	ASSERT_EQUALS(initial.Cleanup(false, false), CPath(wxT("a\"b*c*<d>?e|\\:f")));
	ASSERT_EQUALS(initial.Cleanup(false, true), CPath(wxT("abcdef")));
	ASSERT_EQUALS(initial.Cleanup(true, false), CPath(wxT(" a\"b*c* <d>?e|\\:f ")));
	ASSERT_EQUALS(initial.Cleanup(true, true), CPath(wxT(" abc def ")));
}


TEST(CPath, AddPostFix)
{
	ASSERT_EQUALS(CPath(wxT("/foo.bar")).AddPostfix(wxT("_1")), CPath(wxT("/foo_1.bar")));
	ASSERT_EQUALS(CPath(wxT("/foo.bar")).AddPostfix(wxT("")), CPath(wxT("/foo.bar")));
	ASSERT_EQUALS(CPath(wxT("/.bar")).AddPostfix(wxT("_1")), CPath(wxT("/.bar_1")));
	ASSERT_EQUALS(CPath(wxT("/")).AddPostfix(wxT("_1")), CPath(wxT("/_1")));
}


TEST(CPath, Extensions)
{
	const CPath initial = CPath(wxT("/home/amule.foo.bar"));

	ASSERT_EQUALS(CPath(wxT("/home/amule.foo")), initial.RemoveExt());
	ASSERT_EQUALS(CPath(wxT("/home/amule")), initial.RemoveExt().RemoveExt());
	ASSERT_EQUALS(CPath(wxT("/home/amule")), initial.RemoveExt().RemoveExt().RemoveExt());
	ASSERT_EQUALS(CPath(wxT("/home/amule")), initial.RemoveAllExt());

	ASSERT_EQUALS(wxT("bar"), initial.GetExt());
	ASSERT_EQUALS(wxT("foo"), initial.RemoveExt().GetExt());
	ASSERT_EQUALS(wxEmptyString, initial.RemoveExt().RemoveExt().GetExt());
	ASSERT_EQUALS(wxEmptyString, initial.RemoveAllExt().GetExt());

	ASSERT_EQUALS(CPath(wxT("/home/amule.foo.bar")), initial.AppendExt(wxT("")));
	ASSERT_EQUALS(CPath(wxT("/home/amule.foo.bar.zod")), initial.AppendExt(wxT("zod")));
	ASSERT_EQUALS(CPath(wxT("/home/amule.foo.bar.zod")), initial.AppendExt(wxT(".zod")));
	ASSERT_EQUALS(CPath(wxT("/home/amule.zod")), initial.RemoveAllExt().AppendExt(wxT("zod")));
	ASSERT_EQUALS(CPath(wxT("/home/amule.zod")), initial.RemoveAllExt().AppendExt(wxT(".zod")));
}

