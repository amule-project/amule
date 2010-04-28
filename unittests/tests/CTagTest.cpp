#include <muleunit/test.h>

#include <wx/filename.h>
#include <MemFile.h>
#include <tags/FileTags.h>
#include <math.h>

#include "MD4Hash.h"
#include "amule.h"
#include "Packet.h"
#include <vector>

using namespace muleunit;

DECLARE_SIMPLE(CTag)

void test_taglist_serialization(TagPtrList & taglist, byte* packet, uint64 packet_len);

template <class T1, class T2>
void AssertEquals(const T1& a, const T2& b)
{
	ASSERT_EQUALS(a, b);
}

struct BLOBValue : std::vector<byte>
{
	BLOBValue(uint32 _length, const byte* _ptr)
		: std::vector<byte > (_ptr, _ptr + _length)
	{}
};

struct BSOBValue : std::vector<byte>
{
	BSOBValue(uint8 _length, const byte* _ptr)
		: std::vector<byte > (_ptr, _ptr + _length)
	{}
};

BLOBValue valid_tag_value(const BLOBValue& x)	{ return x; }
BSOBValue valid_tag_value(const BSOBValue& x)	{ return x; }
CMD4Hash valid_tag_value(const CMD4Hash& x)	{ return x; }
float valid_tag_value(float x)			{ return x; }
wxString valid_tag_value(const wxString& x)	{ return x; }
uint64 valid_tag_value(int x)			{ return x; }
uint64 valid_tag_value(long long x)		{ return x; }
uint64 valid_tag_value(uint64 x)		{ return x; }

template<class T>
wxString toString(const T& value)
{
	wxString buf;

	return buf << value;
}

template<class T>
wxString toString(T& value)
{
	wxString buf;

	return buf << value;
}

template<>
wxString toString(const CMD4Hash& value)
{
	return value.Encode();
}

template<>
wxString toString(CMemFile& buf)
{
	uint64 curpos = buf.GetPosition();
	wxString result;
	buf.Seek(0, wxFromStart);
	for (uint64 i = 0; i < buf.GetLength(); i++) {
		result += wxString::Format(wxT("0x%02x,"), buf.ReadUInt8());
	}
	buf.Seek(curpos, wxFromStart);

	return result;
}

template<>
wxString toString(const BSOBValue& buf)
{
	wxString result;
	for (uint64 i = 0; i < buf.size(); i++) {
		result += wxString::Format(wxT("0x%02x,"), buf[i]);
	}
	return result;
}

template<>
wxString toString(const BLOBValue& buf)
{
	wxString result;
	for (uint64 i = 0; i < buf.size(); i++) {
		result += wxString::Format(wxT("0x%02x,"), buf[i]);
	}
	return result;
}

template<>
void AssertEquals(const CMD4Hash& a, const CMD4Hash& b)
{
	CONTEXT(wxT("Compare CMD4Hashes"));
	ASSERT_EQUALS(a.Encode(), b.Encode());
}

template<>
void AssertEquals(const BSOBValue& a, const BSOBValue& b)
{
	CONTEXT(wxT("Compare BSOBValue"));
	ASSERT_EQUALS(toString(a), toString(b));
}

template<>
void AssertEquals(const BLOBValue& a, const BLOBValue& b)
{
	CONTEXT(wxT("Compare BLOBValue"));
	ASSERT_EQUALS(toString(a), toString(b));
}

void CheckTagName(const wxString& tagName, CTag* tag)
{
	CONTEXT(wxT("Checking string tagname"));
	ASSERT_EQUALS(tagName, tag->GetName());
}

void CheckTagName(uint8 tagName, CTag* tag)
{
	CONTEXT(wxT("Checking int tagname"));
	ASSERT_EQUALS(tagName, tag->GetNameID());
}


typedef bool (CTag::*CTagTypeChecker)() const;

template<class T>
struct CTagAccess {};

template<>
struct CTagAccess<wxString>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsStr();
	}

	static const wxString & GetValue(CTag* tag)
	{
		return tag->GetStr();
	}
};

template<>
struct CTagAccess<CMD4Hash>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsHash();
	}

	static const CMD4Hash & GetValue(CTag* tag)
	{
		return tag->GetHash();
	}
};

template<>
struct CTagAccess<float>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsFloat();
	}

	static float GetValue(CTag* tag)
	{
		return tag->GetFloat();
	}
};

template<>
struct CTagAccess<uint64>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsInt();
	}

	static uint64 GetValue(CTag* tag)
	{
		return tag->GetInt();
	}
};

template<>
struct CTagAccess<BLOBValue>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsBlob();
	}

	static BLOBValue GetValue(CTag* tag)
	{
		return BLOBValue(tag->GetBlobSize(), tag->GetBlob());
	}
};

template<>
struct CTagAccess<BSOBValue>
{
	static bool IsRightType(CTag* tag)
	{
		return tag->IsBsob();
	}

	static BSOBValue GetValue(CTag* tag) {
		return BSOBValue(tag->GetBsobSize(), tag->GetBsob());
	}
};

template<class V>
void CheckTagValue(V tagValue, CTag* tag)
{
	CONTEXT(wxT("Check tag value"));

	AssertEquals(tagValue, CTagAccess< V >::GetValue(tag));
}

template<class V>
void CheckTagType(V, CTag* tag)
{
	CONTEXT(wxT("Check tag type"));
	ASSERT_EQUALS(true, CTagAccess<V>::IsRightType(tag));
}

template<class V, class TName>
void CheckTagData(CTag* tag, TName tagName, const V& tagValue)
{
	CONTEXT(wxT("Expected tag value:") + toString(tagValue));
	CONTEXT(wxT("Parsed tag info:") + tag->GetFullInfo());

	CheckTagType(tagValue, tag);
	CheckTagName(tagName, tag);
	CheckTagValue(valid_tag_value(tagValue), tag);
}

void test_taglist_serialization(TagPtrList& taglist, byte* packet, uint64 packet_len)
{
	CMemFile fout;

	{
		CONTEXT(wxT("Writing taglist to CMemFile"));

		fout.WriteTagPtrList(taglist);
	}

	// Rewind file
	fout.Seek(0, wxFromStart);

	{
		CONTEXT(wxT("Check taglist serialization length"));
		ASSERT_EQUALS(packet_len, fout.GetLength());
	}

	std::vector<byte> buf(packet_len);

	{
		CONTEXT(wxT("Reading back serialized taglist bytes from CMemFile"));
		fout.Read(&buf[0], packet_len);
	}

	for (uint64 i = 0; i < packet_len; i++) {
		CONTEXT(wxString::Format(wxT("Comparing serialized byte #%d"), i));

		ASSERT_EQUALS(packet[i], buf[i]);
	}
}

void ReadTagPtrList(TagPtrList& taglist, byte* packet, uint64 packet_len)
{

	CONTEXT(wxT("Reading taglist from buffer"));
	CMemFile fin(packet, packet_len);
	fin.ReadTagPtrList(&taglist, true);

	{
		CONTEXT(wxT("Verify position is at end of packet"));
		ASSERT_EQUALS(packet_len, fin.GetPosition());
	}
}

TEST_M(CTag, ReadTagList1, wxT("Kad: Parse taglist from Kad packet with UTF8 string #1"))
{
	byte packet[] = {
		0x07,
		/*Tag1*/ 0x02, 0x01, 0x00, 0x01, 0x22, 0x00, 0x47, 0x65, 0x6d, 0x20, 0x42, 0x6f, 0x79, 0x20, 0x2d,
		0x20, 0x53, 0x61, 0x72, 0xc3, 0xa0, 0x20, 0x70, 0x65, 0x72, 0x63, 0x68, 0xc3, 0xa8, 0x20, 0x74,
		0x69, 0x20, 0x61, 0x6d, 0x6f, 0x2e, 0x6d, 0x70, 0x33,
		/*Tag2*/ 0x03, 0x01, 0x00, 0x02, 0x1d, 0x6f, 0x1f, 0x00,
		/*Tag3*/ 0x09, 0x01, 0x00, 0x15, 0x01,
		/*Tag4*/ 0x02, 0x01, 0x00, 0x03, 0x05, 0x00, 0x41, 0x75, 0x64, 0x69, 0x6f,
		/*Tag5*/ 0x09, 0x01, 0x00, 0xd3, 0x6b,
		/*Tag6*/ 0x09, 0x01, 0x00, 0xd4, 0x9a,
		/*Tag7*/ 0x03, 0x01, 0x00, 0x33, 0x2f, 0x00, 0x01, 0x01
	};

	TagPtrList taglist;
	ReadTagPtrList(taglist, packet, sizeof (packet));
	TagPtrList::iterator it = taglist.begin();

	CheckTagData(*it++, TAG_FILENAME, valid_tag_value(wxT("Gem Boy - Sarà perchè ti amo.mp3")));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(2060061));
	CheckTagData(*it++, TAG_SOURCES, valid_tag_value(1));
	CheckTagData(*it++, TAG_FILETYPE, valid_tag_value(ED2KFTSTR_AUDIO));
	CheckTagData(*it++, TAG_MEDIA_LENGTH, valid_tag_value(107));
	CheckTagData(*it++, TAG_MEDIA_BITRATE, valid_tag_value(154));
	CheckTagData(*it++, TAG_PUBLISHINFO, valid_tag_value(16842799));

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, packet, sizeof (packet));
	deleteTagPtrListEntries(&taglist);
}

TEST_M(CTag, ReadTagList2, wxT("Kad: Parse taglist from Kad packet with UTF8 string #2"))
{
	byte packet[] = {
		0x05,
		/*Tag1*/0x02, 0x01, 0x00, 0x01, 0x33, 0x00, 0x53, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x69, 0x7a, 0x61,
		0x74, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x65, 0x73, 0x74, 0x20, 0x46, 0x69, 0x6c, 0x65, 0x20, 0x31,
		0x38, 0x39, 0x33, 0x2d, 0xe2, 0x82, 0xac, 0xe2, 0x82, 0xac, 0xc3, 0xa8, 0xc3, 0xa9, 0xc3, 0xa7,
		0xc3, 0xa0, 0xc3, 0xb9, 0xc2, 0xa7, 0x2e, 0x74, 0x78, 0x74,
		/*Tag2*/0x09, 0x01, 0x00, 0x02, 0x0d,
		/*Tag3*/0x09, 0x01, 0x00, 0x15, 0x00,
		/*Tag4*/0x02, 0x01, 0x00, 0x03, 0x03, 0x00, 0x44, 0x6f, 0x63, 0x03, 0x01, 0x00,
		0x33, 0xe8, 0x03, 0x01, 0x01
	};

	TagPtrList taglist;
	ReadTagPtrList(taglist, packet, sizeof (packet));
	TagPtrList::iterator it = taglist.begin();

	CheckTagData(*it++, TAG_FILENAME, valid_tag_value(wxT("Serialization Test File 1893-€€èéçàù§.txt")));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(13));
	CheckTagData(*it++, TAG_SOURCES, valid_tag_value(0));
	CheckTagData(*it++, TAG_FILETYPE, valid_tag_value(ED2KFTSTR_DOCUMENT));
	CheckTagData(*it++, TAG_PUBLISHINFO, valid_tag_value(16843752));

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, packet, sizeof (packet));
	deleteTagPtrListEntries(&taglist);

}

TEST_M(CTag, Float, wxT("Kad: Read/Write floats"))
{
	byte packet[] = {
		0x02,
		/*Tag1*/0x04, 0x01, 0x00, 0xFF, 0x79, 0xe9, 0xf6, 0x42,
		/*Tag2*/0x04, 0x01, 0x00, 0xFF, 0x79, 0xd9, 0xd6, 0x42,
	};

	TagPtrList taglist;
	ReadTagPtrList(taglist, packet, sizeof (packet));
	TagPtrList::iterator it = taglist.begin();

	CheckTagData(*it++, TAG_SOURCETYPE, valid_tag_value((float) 123.456));
	CheckTagData(*it++, TAG_SOURCETYPE, valid_tag_value((float) 107.424751));

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, packet, sizeof (packet));
	deleteTagPtrListEntries(&taglist);
}

TEST_M(CTag, CMD4Hash, wxT("Kad: Read/Write CMD4Hash"))
{
	byte packet[] = {
		0x01,
		/*Tag1*/0x01,
		0x01, 0x00, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	};

	TagPtrList taglist;
	ReadTagPtrList(taglist, packet, sizeof (packet));
	TagPtrList::iterator it = taglist.begin();

	CMD4Hash hash;
	ASSERT_TRUE(hash.Decode("000102030405060708090A0B0C0D0E0F"));

	CheckTagData(*it++, TAG_SOURCETYPE, valid_tag_value(hash));

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, packet, sizeof (packet));
	deleteTagPtrListEntries(&taglist);
}

template<class T, class V>
void check_single_kad_tag(byte* packet, size_t packet_len, T tagName, V tagValue)
{
	CMemFile buf(packet, packet_len);
	CONTEXT(wxT("Starting buffer: ") + toString(buf));
	CTag* tag = buf.ReadTag(true);

	CheckTagData(tag, tagName, valid_tag_value(tagValue));
	{
		CONTEXT(wxT("Check end of buffer"));
		ASSERT_EQUALS(packet_len, buf.GetPosition());
	}

	{
		CMemFile newbuf;
		newbuf.WriteTag(*tag);

		CONTEXT(wxT("Serialized    : ") + toString(newbuf));

		newbuf.Seek(0, wxFromStart);
		for (size_t i = 0; i < packet_len; i++) {
			CONTEXT(wxString::Format(wxT("Comparing byte #%d"), i));

			ASSERT_EQUALS(packet[i], newbuf.ReadUInt8());
		}

		ASSERT_EQUALS(packet_len, buf.GetPosition());
	}
	delete tag;
}

TEST_M(CTag, KadBsob, wxT("Kad: Read/Write BSOB"))
{
	byte packet[] = {
		/*Tag1*/ 0x0A, 0x01, 0x00, 0x02, 0x04, 0x01, 0x02, 0x03, 0x04,
	};
	byte raw_data[] = {0x01, 0x02, 0x03, 0x04};
	{
		CONTEXT(wxT("Create BSOBValue"));
		BSOBValue bsob(sizeof (raw_data), raw_data);

		CONTEXT(wxT("check_single_kad_tag BSOBValue"));
		check_single_kad_tag(packet, sizeof (packet), TAG_FILESIZE, bsob);
	}
}

TEST_M(CTag, KadInt64, wxT("Kad: Read/Write integer 64bit"))
{
	byte packet[] = {
		/*Tag1*/ 0x0b, 0x01, 0x00, 0x02, 0x10, 0x11, 0x12, 0x13, 0x20, 0x21, 0x22, 0x23, // 64 bit int
	};
	check_single_kad_tag(packet, sizeof (packet), TAG_FILESIZE, 0x2322212013121110LL);
}

TEST_M(CTag, KadInt32, wxT("Kad: Read/Write integer 32bit"))
{
	byte packet[] = {
		/*Tag1*/ 0x03, 0x01, 0x00, 0x02, 0x12, 0x34, 0x56, 0x78, // 32 bit int
	};
	check_single_kad_tag(packet, sizeof (packet), TAG_FILESIZE, 0x78563412);
}

TEST_M(CTag, KadInt16, wxT("Kad: Read/Write integer 16bit"))
{
	byte packet[] = {
		/*Tag1*/ 0x08, 0x01, 0x00, 0x02, 0x12, 0x34, // 16 bit int
	};
	check_single_kad_tag(packet, sizeof (packet), TAG_FILESIZE, 0x3412);
}

TEST_M(CTag, KadInt8, wxT("Kad: Read/Write integer  8bit"))
{
	byte packet[] = {
		/*Tag1*/ 0x09, 0x01, 0x00, 0x02, 0x12, //  8 bit int
	};
	check_single_kad_tag(packet, sizeof (packet), TAG_FILESIZE, 0x12);
}

TEST_M(CTag, ReadIntegers, wxT("Kad: Read/Write multiple integers"))
{
	byte packet[] = {
		0x08,
		/*Tag1*/ 0x03, 0x01, 0x00, 0x02, 0x12, 0x34, 0x56, 0x78, // 32 bit int
		/*Tag2*/ 0x08, 0x01, 0x00, 0x02, 0x12, 0x34, // 16 bit int
		/*Tag3*/ 0x09, 0x01, 0x00, 0x02, 0x12, //  8 bit int
		/*Tag4*/ 0x0b, 0x01, 0x00, 0x02, 0x10, 0x11, 0x12, 0x13, 0x20, 0x21, 0x22, 0x23, // 64 bit int

		/*Tag5*/ 0x03, 0x01, 0x00, 0x02, 0x12, 0x34, 0x56, 0x78, // 32 bit int
		/*Tag6*/ 0x08, 0x01, 0x00, 0x02, 0x12, 0x34, // 16 bit int
		/*Tag7*/ 0x09, 0x01, 0x00, 0x02, 0x12, //  8 bit int
		/*Tag8*/ 0x0b, 0x01, 0x00, 0x02, 0x10, 0x11, 0x12, 0x13, 0x20, 0x21, 0x22, 0x23, // 64 bit int
	};

	TagPtrList taglist;
	ReadTagPtrList(taglist, packet, sizeof (packet));
	TagPtrList::iterator it = taglist.begin();

	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x78563412));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x3412));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x12));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x2322212013121110LL));

	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x78563412));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x3412));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x12));
	CheckTagData(*it++, TAG_FILESIZE, valid_tag_value(0x2322212013121110LL));

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, packet, sizeof (packet));
	deleteTagPtrListEntries(&taglist);
}


#include <map>

typedef std::map<wxString, wxString> TagNamesByString;

TEST_M(CTag, KadTagNames, wxT("Kad: Test Kad tags (name=string) - write/read every valid tag name"))
{
	TagNamesByString tagNames;

	tagNames[TAG_FILENAME] = wxT("TAG_FILENAME");
	tagNames[TAG_FILESIZE] = wxT("TAG_FILESIZE");
	tagNames[TAG_FILESIZE_HI] = wxT("TAG_FILESIZE_HI");
	tagNames[TAG_FILETYPE] = wxT("TAG_FILETYPE");
	tagNames[TAG_FILEFORMAT] = wxT("TAG_FILEFORMAT");
	tagNames[TAG_COLLECTION] = wxT("TAG_COLLECTION");
	tagNames[TAG_PART_PATH] = wxT("TAG_PART_PATH");
	tagNames[TAG_PART_HASH] = wxT("TAG_PART_HASH");
	tagNames[TAG_COPIED] = wxT("TAG_COPIED");
	tagNames[TAG_GAP_START] = wxT("TAG_GAP_START");
	tagNames[TAG_GAP_END] = wxT("TAG_GAP_END");
	tagNames[TAG_DESCRIPTION] = wxT("TAG_DESCRIPTION");
	tagNames[TAG_PING] = wxT("TAG_PING");
	tagNames[TAG_FAIL] = wxT("TAG_FAIL");
	tagNames[TAG_PREFERENCE] = wxT("TAG_PREFERENCE");
	tagNames[TAG_PORT] = wxT("TAG_PORT");
	tagNames[TAG_IP_ADDRESS] = wxT("TAG_IP_ADDRESS");
	tagNames[TAG_VERSION] = wxT("TAG_VERSION");
	tagNames[TAG_TEMPFILE] = wxT("TAG_TEMPFILE");
	tagNames[TAG_PRIORITY] = wxT("TAG_PRIORITY");
	tagNames[TAG_STATUS] = wxT("TAG_STATUS");
	tagNames[TAG_SOURCES] = wxT("TAG_SOURCES");
	tagNames[TAG_AVAILABILITY] = wxT("TAG_AVAILABILITY");
	tagNames[TAG_PERMISSIONS] = wxT("TAG_PERMISSIONS");
	tagNames[TAG_QTIME] = wxT("TAG_QTIME");
	tagNames[TAG_PARTS] = wxT("TAG_PARTS");
	tagNames[TAG_PUBLISHINFO] = wxT("TAG_PUBLISHINFO");
	tagNames[TAG_MEDIA_ARTIST] = wxT("TAG_MEDIA_ARTIST");
	tagNames[TAG_MEDIA_ALBUM] = wxT("TAG_MEDIA_ALBUM");
	tagNames[TAG_MEDIA_TITLE] = wxT("TAG_MEDIA_TITLE");
	tagNames[TAG_MEDIA_LENGTH] = wxT("TAG_MEDIA_LENGTH");
	tagNames[TAG_MEDIA_BITRATE] = wxT("TAG_MEDIA_BITRATE");
	tagNames[TAG_MEDIA_CODEC] = wxT("TAG_MEDIA_CODEC");
	tagNames[TAG_KADMISCOPTIONS] = wxT("TAG_KADMISCOPTIONS");
	tagNames[TAG_ENCRYPTION] = wxT("TAG_ENCRYPTION");
	tagNames[TAG_FILERATING] = wxT("TAG_FILERATING");
	tagNames[TAG_BUDDYHASH] = wxT("TAG_BUDDYHASH");
	tagNames[TAG_CLIENTLOWID] = wxT("TAG_CLIENTLOWID");
	tagNames[TAG_SERVERPORT] = wxT("TAG_SERVERPORT");
	tagNames[TAG_SERVERIP] = wxT("TAG_SERVERIP");
	tagNames[TAG_SOURCEUPORT] = wxT("TAG_SOURCEUPORT");
	tagNames[TAG_SOURCEPORT] = wxT("TAG_SOURCEPORT");
	tagNames[TAG_SOURCEIP] = wxT("TAG_SOURCEIP");
	tagNames[TAG_SOURCETYPE] = wxT("TAG_SOURCETYPE");

	CMemFile buf;
	buf.WriteUInt8(tagNames.size());
	int counter = 0;
	// For each tagNames entry write an 8bit int tag (type:0x9)
	for (TagNamesByString::iterator it_name = tagNames.begin(); it_name != tagNames.end(); it_name++) {
		buf.WriteUInt8(0x09); // 8 bit int tag type

		buf.WriteUInt8(0x01); // single char string
		buf.WriteUInt8(0x00); //

		buf.WriteUInt8(it_name->first.GetChar(0)); // Write string first char
		buf.WriteUInt8(counter++); // write tag value
	}

	TagPtrList taglist;
	buf.Seek(0, wxFromStart);
	size_t packet_len = buf.GetLength();
	std::vector<byte> packet(packet_len);
	buf.Read(&packet[0], packet_len);

	ReadTagPtrList(taglist, &packet[0], packet_len);

	TagPtrList::iterator it = taglist.begin();

	counter = 0;
	for (TagNamesByString::iterator it_name = tagNames.begin(); it_name != tagNames.end(); it_name++) {
		CONTEXT(wxT("Testing tag name: ") + it_name->second);
		CheckTagData(*it++, it_name->first, valid_tag_value(counter++));
	}

	ASSERT_TRUE(it == taglist.end());
	test_taglist_serialization(taglist, &packet[0], packet_len);
	deleteTagPtrListEntries(&taglist);
}

typedef std::map<int, wxString> TagNamesByInt;

TEST_M(CTag, ED2kTagNames, wxT("Ed2k: Test ed2k tags (name=id) - write/read every valid tag name"))
{
	TagNamesByInt tagNames;

	tagNames[FT_FILENAME] = wxT("FT_FILENAME");
	tagNames[FT_FILESIZE] = wxT("FT_FILESIZE");
	tagNames[FT_FILESIZE_HI] = wxT("FT_FILESIZE_HI");
	tagNames[FT_FILETYPE] = wxT("FT_FILETYPE");
	tagNames[FT_FILEFORMAT] = wxT("FT_FILEFORMAT");
	tagNames[FT_LASTSEENCOMPLETE] = wxT("FT_LASTSEENCOMPLETE");
	tagNames[FT_TRANSFERRED] = wxT("FT_TRANSFERRED");
	tagNames[FT_GAPSTART] = wxT("FT_GAPSTART");
	tagNames[FT_GAPEND] = wxT("FT_GAPEND");
	tagNames[FT_PARTFILENAME] = wxT("FT_PARTFILENAME");
	tagNames[FT_OLDDLPRIORITY] = wxT("FT_OLDDLPRIORITY");
	tagNames[FT_STATUS] = wxT("FT_STATUS");
	tagNames[FT_SOURCES] = wxT("FT_SOURCES");
	tagNames[FT_PERMISSIONS] = wxT("FT_PERMISSIONS");
	tagNames[FT_OLDULPRIORITY] = wxT("FT_OLDULPRIORITY");
	tagNames[FT_DLPRIORITY] = wxT("FT_DLPRIORITY");
	tagNames[FT_ULPRIORITY] = wxT("FT_ULPRIORITY");
	tagNames[FT_KADLASTPUBLISHKEY] = wxT("FT_KADLASTPUBLISHKEY");
	tagNames[FT_KADLASTPUBLISHSRC] = wxT("FT_KADLASTPUBLISHSRC");
	tagNames[FT_FLAGS] = wxT("FT_FLAGS");
	tagNames[FT_DL_ACTIVE_TIME] = wxT("FT_DL_ACTIVE_TIME");
	tagNames[FT_CORRUPTEDPARTS] = wxT("FT_CORRUPTEDPARTS");
	tagNames[FT_DL_PREVIEW] = wxT("FT_DL_PREVIEW");
	tagNames[FT_KADLASTPUBLISHNOTES] = wxT("FT_KADLASTPUBLISHNOTES");
	tagNames[FT_AICH_HASH] = wxT("FT_AICH_HASH");
	tagNames[FT_COMPLETE_SOURCES] = wxT("FT_COMPLETE_SOURCES");
	tagNames[FT_PUBLISHINFO] = wxT("FT_PUBLISHINFO");
	tagNames[FT_ATTRANSFERRED] = wxT("FT_ATTRANSFERRED");
	tagNames[FT_ATREQUESTED] = wxT("FT_ATREQUESTED");
	tagNames[FT_ATACCEPTED] = wxT("FT_ATACCEPTED");
	tagNames[FT_CATEGORY] = wxT("FT_CATEGORY");
	tagNames[FT_ATTRANSFERREDHI] = wxT("FT_ATTRANSFERREDHI");
	tagNames[FT_MEDIA_ARTIST] = wxT("FT_MEDIA_ARTIST");
	tagNames[FT_MEDIA_ALBUM] = wxT("FT_MEDIA_ALBUM");
	tagNames[FT_MEDIA_TITLE] = wxT("FT_MEDIA_TITLE");
	tagNames[FT_MEDIA_LENGTH] = wxT("FT_MEDIA_LENGTH");
	tagNames[FT_MEDIA_BITRATE] = wxT("FT_MEDIA_BITRATE");
	tagNames[FT_MEDIA_CODEC] = wxT("FT_MEDIA_CODEC");
	tagNames[FT_FILERATING] = wxT("FT_FILERATING");


	CMemFile buf;

	uint64 counter = 0;
	for (TagNamesByInt::iterator it_name = tagNames.begin(); it_name != tagNames.end(); it_name++) {
		// m_uType
		buf.WriteUInt8(0x09 + 0x80);
		// m_uName
		buf.WriteUInt8(it_name->first);
		// 8 bit
		buf.WriteUInt8(counter++);
	}

	buf.Seek(0, wxFromStart);

	counter = 0;
	for (TagNamesByInt::iterator it_name = tagNames.begin(); it_name != tagNames.end(); it_name++) {
		CONTEXT(wxString::Format(wxT("Reading tag#%d"), counter));
		CTag* newtag = new CTag(buf, true);
		CheckTagName(it_name->first, newtag);
		CheckTagValue( valid_tag_value( counter ), newtag);
		delete newtag;
		counter++;
	}
}

TEST_M(CTag, Ed2kBlob1, wxT("Ed2k: Read/Write BLOB - numeric tagname"))
{
	byte packet[] = {
		/*Tag1*/ 0x87, 0xFF, 0x04, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03, 0x04,
	};

	CMemFile buf(packet, sizeof (packet));
	buf.Seek(0, wxFromStart);
	byte raw_data[] = {0x01, 0x02, 0x03, 0x04};
	{
		CONTEXT(wxT("Create BLOBValue"));
		BLOBValue blob(sizeof (raw_data), raw_data);

		CTag tag(buf, true);
		CheckTagName(0xFF, &tag);
		CheckTagValue( valid_tag_value( blob ), &tag);
	}
}

TEST_M(CTag, Ed2kBlob2, wxT("Ed2k: Read/Write BLOB - string tagname"))
{
	byte packet[] = {
		/*Tag1*/ 0x07, 0x02, 0x00, 'A', 'A', 0x04, 0x00, 0x00, 0x00,
		0x01, 0x02, 0x03, 0x04,
	};

	CMemFile buf(packet, sizeof (packet));
	buf.Seek(0, wxFromStart);
	byte raw_data[] = {0x01, 0x02, 0x03, 0x04};
	{
		CONTEXT(wxT("Create BLOBValue"));
		BLOBValue blob(sizeof (raw_data), raw_data);

		CTag tag(buf, true);
		CheckTagName(wxT("AA"), &tag);
		CheckTagValue( valid_tag_value( blob ), &tag);
	}
}
