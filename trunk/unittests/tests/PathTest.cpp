#include <muleunit/test.h>

#include <common/Path.h>
#include <common/StringFunctions.h>

using namespace muleunit;

struct STestStr
{
	//! Input filename
	const wxChar* input;
	//! The expected result.
	const wxChar* expected;
};

const STestStr g_fromFSTests[] = {
	{ wxT("\xe1\x62\x63"), wxT("\xe1\x62\x63") },
	{ wxT("\xe6\xf8\xe5"), wxT("\xe6\xf8\xe5") }
};


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
		CPath tmp(wxT(""), CPath::FromFS);

		ASSERT_FALSE(tmp.IsOk());
		ASSERT_EQUALS(tmp, CPath());
	}
	

	for (size_t i = 0; i < ArraySize(g_fromFSTests); ++i) {
		const wxChar* input = g_fromFSTests[i].input;
		const wxChar* printable = g_fromFSTests[i].expected;

		CPath tmp(input, CPath::FromFS);

		ASSERT_TRUE(tmp.IsOk());
		ASSERT_EQUALS(tmp, CPath(input, CPath::FromFS));

		ASSERT_EQUALS(tmp.GetRaw(), input);
		ASSERT_EQUALS(tmp.GetPrintable(), printable);

		ASSERT_EQUALS(tmp.GetFullName(), tmp);
		ASSERT_EQUALS(tmp.GetPath(), CPath());
	}
}


TEST(CPath, CopyConstructor)
{
	const wxChar* tmpPath = wxT("foobar.tgz");

	{
		CPath a(tmpPath, CPath::FromFS);

		ASSERT_TRUE(a.IsOk());
		ASSERT_EQUALS(a.GetRaw(), tmpPath);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath, CPath::FromFS));

		CPath b(a);
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(b.GetRaw(), tmpPath);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath, CPath::FromFS));

		ASSERT_EQUALS(a, b);
	}

	{
		CPath a(tmpPath, CPath::FromUser);

		ASSERT_TRUE(a.IsOk());
		ASSERT_EQUALS(a.GetRaw(), tmpPath);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath, CPath::FromUser));

		CPath b(a);
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(b.GetRaw(), tmpPath);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath, CPath::FromUser));

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

		a = CPath(tmpPath1, CPath::FromFS);
		ASSERT_EQUALS(a.GetRaw(), tmpPath1);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath1, CPath::FromUser));
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(a != b);

		b = a;
		ASSERT_EQUALS(b.GetRaw(), tmpPath1);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath1, CPath::FromUser));
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_EQUALS(a, b);

		a = CPath(tmpPath2, CPath::FromFS);
		ASSERT_TRUE(a.IsOk());
		ASSERT_TRUE(b.IsOk());
		ASSERT_TRUE(a != b);

		ASSERT_EQUALS(a.GetRaw(), tmpPath2);
		ASSERT_EQUALS(a.GetPrintable(), tmpPath2);
		ASSERT_EQUALS(a.GetPath(), CPath());
		ASSERT_EQUALS(a.GetFullName(), CPath(tmpPath2, CPath::FromUser));
		ASSERT_EQUALS(b.GetRaw(), tmpPath1);
		ASSERT_EQUALS(b.GetPrintable(), tmpPath1);
		ASSERT_EQUALS(b.GetPath(), CPath());
		ASSERT_EQUALS(b.GetFullName(), CPath(tmpPath1, CPath::FromUser));
	}

// See note in CPath::operator==
#if 0
	{
		CPath a = CPath(wxT("/home/amule"), CPath::FromFS);
		CPath b = CPath(wxT("/hone/amule/"), CPath::FromFS);

		ASSERT_EQUALS(a, b);
	}
#endif 

	// TODO: Less than
}


TEST(CPath, JoinPaths)
{
	const CPath expected1(wxT("/home/amule/"), CPath::FromFS);
	const CPath expected2(wxT("/home/amule"), CPath::FromFS);

	// Note: Just a few checks, as ::JoinPaths is tested in StringFunctionTests.cpp
	ASSERT_EQUALS(expected1, CPath(wxT("/home"), CPath::FromFS).JoinPaths(CPath(wxT("amule/"), CPath::FromFS)));
	ASSERT_EQUALS(expected1, CPath(wxT("/home"), CPath::FromFS).JoinPaths(CPath(wxT("/amule/"), CPath::FromFS)));
	ASSERT_EQUALS(expected1, CPath(wxT("/home/"), CPath::FromFS).JoinPaths(CPath(wxT("/amule/"), CPath::FromFS)));
	ASSERT_EQUALS(expected1, CPath(wxT("/home/"), CPath::FromFS).JoinPaths(CPath(wxT("amule/"), CPath::FromFS)));
	
	ASSERT_EQUALS(expected2, CPath(wxT("/home"), CPath::FromFS).JoinPaths(CPath(wxT("amule"), CPath::FromFS)));
	ASSERT_EQUALS(expected2, CPath(wxT("/home"), CPath::FromFS).JoinPaths(CPath(wxT("/amule"), CPath::FromFS)));
	ASSERT_EQUALS(expected2, CPath(wxT("/home/"), CPath::FromFS).JoinPaths(CPath(wxT("/amule"), CPath::FromFS)));
	ASSERT_EQUALS(expected2, CPath(wxT("/home/"), CPath::FromFS).JoinPaths(CPath(wxT("amule"), CPath::FromFS)));

	ASSERT_EQUALS(expected1, expected1.JoinPaths(CPath()));
	ASSERT_EQUALS(expected1, CPath().JoinPaths(expected1));
	
	ASSERT_EQUALS(expected2, expected2.JoinPaths(CPath()));
	ASSERT_EQUALS(expected2, CPath().JoinPaths(expected2));
}


TEST(CPath, StartsWith)
{
	const CPath test(wxT("/home/amule/"), CPath::FromFS);

	ASSERT_FALSE(test.StartsWith(CPath()));
	ASSERT_FALSE(CPath().StartsWith(test));

	ASSERT_TRUE(test.StartsWith(CPath(wxT("/"), CPath::FromFS)));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home"), CPath::FromFS)));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/"), CPath::FromFS)));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/amule"), CPath::FromFS)));
	ASSERT_TRUE(test.StartsWith(CPath(wxT("/home/amule/"), CPath::FromFS)));

	ASSERT_FALSE(test.StartsWith(CPath(wxT(""), CPath::FromFS)));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("~"), CPath::FromFS)));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/ho"), CPath::FromFS)));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/am"), CPath::FromFS)));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/amule2"), CPath::FromFS)));
	ASSERT_FALSE(test.StartsWith(CPath(wxT("/home/amule/foo"), CPath::FromFS)));
}

