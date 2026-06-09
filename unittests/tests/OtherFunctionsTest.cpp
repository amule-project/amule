#include <muleunit/test.h>
#include <OtherFunctions.h>

#include <cstring>

using namespace muleunit;

DECLARE_SIMPLE(Base16)

// Valid hex string round-trips correctly
TEST(Base16, RoundTrip)
{
	const unsigned char input[] = { 0xDE, 0xAD, 0xBE, 0xEF };
	wxString encoded = EncodeBase16(input, 4);
	ASSERT_EQUALS(wxString(wxT("DEADBEEF")), encoded);

	unsigned char decoded[4];
	unsigned int len = DecodeBase16(encoded, encoded.Length(), decoded);
	ASSERT_EQUALS(4u, len);
	ASSERT_TRUE(memcmp(input, decoded, 4) == 0);
}

// Characters ':' through '@' (ASCII 58-64) are not valid hex digits;
// DecodeBase16 must return 0 instead of silently mapping them to 0x9.
TEST(Base16, RejectsNonHexChars)
{
	unsigned char buf[4] = {};
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE:D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE;D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE<D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE=D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE>D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE?D"), 4, buf));
	ASSERT_EQUALS(0u, DecodeBase16(wxT("DE@D"), 4, buf));
}

// Odd-length input is not valid Base16
TEST(Base16, RejectsOddLength)
{
	unsigned char buf[4] = {};
	ASSERT_EQUALS(0u, DecodeBase16(wxT("ABC"), 3, buf));
}


DECLARE_SIMPLE(Base64)

// EncodeBase64 with a header must include both the encoded content and the
// footer — regression for the pbBufferOut = vs += bug.
TEST(Base64, HeaderAndFooterBothPresent)
{
	SetBase64Header(wxT("TEST"));
	wxString result = EncodeBase64("Man", 3);
	SetBase64Header(wxEmptyString);  // restore global state

	// "Man" encodes to "TWFu" in standard Base64
	ASSERT_TRUE(result.Contains(wxT("-----BEGIN TEST-----")));
	ASSERT_TRUE(result.Contains(wxT("TWFu")));
	ASSERT_TRUE(result.Contains(wxT("-----END TEST-----")));

	// Encoded content must appear before the footer
	ASSERT_TRUE(result.Find(wxT("TWFu")) < result.Find(wxT("-----END TEST-----")));
}

// Without a header, EncodeBase64 produces plain Base64 with no PEM framing
TEST(Base64, NoHeaderProducesPlainBase64)
{
	SetBase64Header(wxEmptyString);
	wxString result = EncodeBase64("Man", 3);

	ASSERT_FALSE(result.Contains(wxT("-----")));
	ASSERT_TRUE(result.Contains(wxT("TWFu")));
}
