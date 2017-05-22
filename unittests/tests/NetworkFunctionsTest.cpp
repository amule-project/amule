#include <muleunit/test.h>
#include <NetworkFunctions.h>

#define itemsof(x) (sizeof(x)/sizeof(x[0]))

using namespace muleunit;

// Needed for Boost-enabled build
namespace MuleNotify {
	void HandleNotificationAlways(const class CMuleNotiferBase&) {}
};

DECLARE_SIMPLE(NetworkFunctions)

TEST(NetworkFunctions, StringIPtoUint32)
{
	unsigned int values[] = { 0, 1, 127, 254, 255 };
	const wxChar whitespace[] = { wxT(' '), wxT('\t'), wxT('\n') };
	int items = itemsof(values);
	int whites = 2;
	int zeros = 2;

	// Test a few standard IP combinations
	for (int wl = 0; wl < whites; ++wl) {
		for (int wr = 0; wr < whites; ++wr) {
			for (int a = 0; a < items; ++a) {
				for (int za = 0; za < zeros; ++za) {
					for (int b = 0; b < items; ++b) {
						for (int zb = 0; zb < zeros; ++zb) {
							for (int c = 0; c < items; ++c) {
								for (int zc = 0; zc < zeros; ++zc) {
									for (int d = 0; d < items; ++d) {
										for (int zd = 0; zd < zeros; ++zd) {
											wxString IP;

											IP << wxString(wxT(' '), wl)
											   << wxString(wxT('0'), za)
											   << values[a]
											   << wxT('.')
											   << wxString(wxT('0'), zb)
											   << values[b]
											   << wxT('.')
											   << wxString(wxT('0'), zc)
											   << values[c]
											   << wxT('.')
											   << wxString(wxT('0'), zd)
											   << values[d]
											   << wxString(wxT(' '), wr);

											uint32 resultIP = 17;

											ASSERT_TRUE(StringIPtoUint32(IP, resultIP));

											uint32 expected = (values[d] << 24) | (values[c] << 16) | (values[b] << 8) | values[a];

											ASSERT_EQUALS(expected, resultIP);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}


	// Test invalid IPs
	uint32 dummyIP = 27;

	// Missing fields
	ASSERT_FALSE(StringIPtoUint32(wxT(".2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1..3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2..4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3."), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT(".2.3."), dummyIP));

	// Extra dots
	ASSERT_FALSE(StringIPtoUint32(wxT(".1.2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1..2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2..3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3..4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.4."), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT(".1.2.3.4."), dummyIP));

	// Garbage
	ASSERT_FALSE(StringIPtoUint32(wxT("abc"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("a1.1.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.1.3.4b"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("a.1.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.b.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.c.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.d"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.b.3.d"), dummyIP));

	// Invalid fields
	ASSERT_FALSE(StringIPtoUint32(wxT("256.2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.256.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.256.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.256"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("256.2.3.256"), dummyIP));

	// Negative fields
	ASSERT_FALSE(StringIPtoUint32(wxT("-1.2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.-2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.-3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.-4"), dummyIP));

	// Whitespace between fields
	for (unsigned i = 0; i < itemsof(whitespace); ++i) {
		wxChar c = whitespace[i];

		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1%c.2.3.4"), c), dummyIP));
		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1.%c2.3.4"), c), dummyIP));
		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1.2%c.3.4"), c), dummyIP));
		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1.2.%c3.4"), c), dummyIP));
		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1.2.3%c.4"), c), dummyIP));
		ASSERT_FALSE(StringIPtoUint32(wxString::Format(wxT("1.2.3.%c4"), c), dummyIP));
	}


	// Faar too large values (triggered overflow and became negative)
	ASSERT_FALSE(StringIPtoUint32(wxT("2147483648.2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2147483648.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.2147483648.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.2147483648"), dummyIP));

	// Values greater than 2 ** 32 - 1 (triggered overflow and becames x - (2 ** 32 - 1))
	ASSERT_FALSE(StringIPtoUint32(wxT("4294967296.2.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.4294967296.3.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.4294967296.4"), dummyIP));
	ASSERT_FALSE(StringIPtoUint32(wxT("1.2.3.4294967296"), dummyIP));


	// The dummyIP value shouldn't have been changed by any of these calls
	ASSERT_EQUALS(27u, dummyIP);
}


// Testing the IsGoodIP() and IsLanIP() functions
TEST(NetworkFunctions, IsGoodIP)
{
	struct {
		wxString	ip;
		bool		isgood;
		bool		islan;
	} ipList[] = {
		{ wxT("0.0.0.0"),	false,	false	},
		{ wxT("0.0.0.1"),	false,	false	},
		{ wxT("0.0.1.0"),	false,	false	},
		{ wxT("0.1.0.0"),	false,	false	},
		{ wxT("1.0.0.0"),	true,	false	},
		{ wxT("10.0.0.1"),	true,	true	},
		{ wxT("10.0.1.0"),	true,	true	},
		{ wxT("10.1.0.0"),	true,	true	},
		{ wxT("14.156.39.4"),	true,	false	},
		{ wxT("24.93.63.177"),	true,	false	},
		{ wxT("172.15.0.0"),	true,	false	},
		{ wxT("172.16.0.0"),	true,	true	},
		{ wxT("172.17.0.0"),	true,	true	},
		{ wxT("172.31.0.0"),	true,	true	},
		{ wxT("172.32.0.0"),	true,	false	},
		{ wxT("192.88.98.176"),	true,	false	},
		{ wxT("192.88.99.175"),	false,	false	},
		{ wxT("192.88.100.17"),	true,	false	},
		{ wxT("192.167.0.0"),	true,	false	},
		{ wxT("192.168.0.0"),	true,	true	},
		{ wxT("192.168.255.255"), true, true	},
		{ wxT("192.169.0.0"),	true,	false	},
		{ wxT("198.17.0.0"),	true,	false	},
		{ wxT("198.18.0.0"),	false,	false	},
		{ wxT("198.19.0.0"),	false,	false	},
		{ wxT("198.20.0.0"),	true,	false	}
	};

	unsigned int ip;
	for (unsigned int i = 0; i < itemsof(ipList); i++) {
		ASSERT_TRUE(StringIPtoUint32(ipList[i].ip, ip));
		ASSERT_EQUALS(ipList[i].isgood, IsGoodIP(ip, false));
		ASSERT_EQUALS(ipList[i].islan, IsLanIP(ip));
		ASSERT_EQUALS(ipList[i].isgood && !ipList[i].islan, IsGoodIP(ip, true));
	}
}
