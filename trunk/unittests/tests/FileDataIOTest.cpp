#include <muleunit/test.h>
#include <CFile.h>
#include <MemFile.h>
#include <CMD4Hash.h>
#include <limits>
#include <kademlia/utils/UInt128.h>

using Kademlia::CUInt128;
using namespace muleunit;


//! The max file-size of auto-generated files to test.
const size_t TEST_LENGTH = 512;

namespace muleunit {
	//! Needed for ASSERT_EQUALS with CMD4Hash values
	template <>
	wxString StringFrom<CMD4Hash>(const CMD4Hash& hash) {
		return hash.Encode();
	}

	//! Needed for ASSERT_EQUALS with CUInt128 values
	template <>
	wxString StringFrom<CUInt128>(const CUInt128& value) {
		wxString str;
		value.toHexString(&str);
		return str;
	}
}


void writePredefData(CFileDataIO* file)
{
	char data[TEST_LENGTH];

	for (size_t j = 0; j < TEST_LENGTH; ++j) {
		data[j] = j & 0xff;
	}

	file->Write(data, TEST_LENGTH);
	file->Seek(0, wxFromStart);
}



/////////////////////////////////////////////////////////////////////
// Specialize this template for each implemention
// of the CFileDataIO interface you wish to test.
//
// This struct must be a subclass of Test.
// 
// Two pointers are to be defined:
//  m_emptyFile, which must be an empty, zero-length file
//  m_predefFile, which must be TEST_LENGTH in size and
//                and contain the sequence 0..255 repeated
//                as needed.
// 
// The following functions should be overridden:
//  - setUp()
//  - tearDown()
//
template <typename TYPE>
struct FileDataIOFixture;


template <>
class FileDataIOFixture<CFile> : public Test
{
public:
	FileDataIOFixture(const wxString& testName)
		: Test(wxT("FileDataIO"), wxT("CFile - ") + testName) {}

	
	CFile* m_emptyFile;
	CFile* m_predefFile;
	
	void setUp() {
		m_emptyFile = new CFile();
		m_emptyFile->Create(wxT("FileDataIOTest.empty"), true);
		ASSERT_TRUE(m_emptyFile->IsOpened());
		m_emptyFile->Close();
		m_emptyFile->Open(wxT("FileDataIOTest.empty"), CFile::read_write);
		ASSERT_TRUE(m_emptyFile->IsOpened());
		
		m_predefFile = new CFile();
		m_predefFile->Create(wxT("FileDataIOTest.dat"), true);
		ASSERT_TRUE(m_predefFile->IsOpened());
		m_predefFile->Close();
		m_predefFile->Open(wxT("FileDataIOTest.dat"), CFile::read_write);
		ASSERT_TRUE(m_predefFile->IsOpened());

		writePredefData(m_predefFile);
		ASSERT_EQUALS(0, m_predefFile->GetPosition());
		ASSERT_EQUALS(TEST_LENGTH, m_predefFile->GetLength());		
	}
	
	void tearDown() {
		delete m_emptyFile;
		delete m_predefFile;
		
		wxRemoveFile(wxT("FileDataIOTest.dat"));
		wxRemoveFile(wxT("FileDataIOTest.empty"));
	}
};


template <>
class FileDataIOFixture<CMemFile> : public Test
{
public:
	FileDataIOFixture(const wxString& testName)
		: Test(wxT("FileDataIO"), wxT("CMemFile - ") + testName) {}

	
	CMemFile* m_emptyFile;
	CMemFile* m_predefFile;
	
	void setUp() {
		m_emptyFile = new CMemFile();
		m_predefFile = new CMemFile();
		
		writePredefData(m_predefFile);		
		ASSERT_EQUALS(0, m_predefFile->GetPosition());
		ASSERT_EQUALS(TEST_LENGTH, m_predefFile->GetLength());
	}
	
	void tearDown() {
		delete m_emptyFile;
		delete m_predefFile;
	}
};


/////////////////////////////////////////////////////////////////////
// A writeWrite interface should be implemented for each set of 
// read/write functions that is to be tested. The following 3
// static functions must be implemented in each specialization of the
// template:
//
//  - TYPE genValue(size_t j), which returns the expected value at 
//    position j in the files with predefined data.
//  - TYPE readValue(CFileDataIO*), which returns and returns the
//    value at the current position in the file.
//  - void writeValue(CFileDataIO*, TYPE), which writes the given
//    value at the current position in the file.
//  - wxString name(), which returns the human-readble name of the type
//
template <typename TYPE>
struct RWInterface;

template <>
struct RWInterface<uint8>
{
	static uint8 genValue(size_t j) {
		return j & 0xff;
	}

	static uint8 readValue(CFileDataIO* file) {
		return file->ReadUInt8();
	}
	
	static void writeValue(CFileDataIO* file, uint8 value) {
		file->WriteUInt8(value);
	}

	static wxString name() { return wxT("UInt8"); }
};


template <>
struct RWInterface<uint16>
{
	static uint16 genValue(size_t j) {
		return (((j + 1) & 0xff) << 8) | (j & 0xff);
	}

	static uint16 readValue(CFileDataIO* file) {
		return file->ReadUInt16();
	}
	
	static void writeValue(CFileDataIO* file, uint16 value) {
		file->WriteUInt16(value);
	}
	
	static wxString name() { return wxT("UInt16"); }
};


template <>
struct RWInterface<uint32>
{
	static uint32 genValue(size_t j) {
		return (((j + 3) & 0xff) << 24) | (((j + 2) & 0xff) << 16) | (((j + 1) & 0xff) << 8) | (j & 0xff);
	}

	static uint32 readValue(CFileDataIO* file) {
		return file->ReadUInt32();
	}
	
	static void writeValue(CFileDataIO* file, uint32 value) {
		file->WriteUInt32(value);
	}

	static wxString name() { return wxT("UInt32"); }
};


template <>
struct RWInterface<CMD4Hash>
{
	static CMD4Hash genValue(size_t j) {
		CMD4Hash value;
		for (size_t y = j; y < j + 16; y++) {
			value[y - j] = y & 0xff;
		}
		
		return value;
	}

	static CMD4Hash readValue(CFileDataIO* file) {
		return file->ReadHash();
	}
	
	static void writeValue(CFileDataIO* file, CMD4Hash value) {
		file->WriteHash(value);
	}

	static wxString name() { return wxT("CMD4Hash"); }
};


template <>
struct RWInterface<CUInt128>
{
	static CUInt128 genValue(size_t j) {
		CUInt128 value;
		uint32* data = (uint32*)value.getDataPtr();
		for (size_t y = 0; y < 4; y++) {
			data[y] = (j + 3 + y * 4) & 0xff;
			data[y] = (data[y] << 8) | (j + 2 + y * 4) & 0xff;
			data[y] = (data[y] << 8) | (j + 1 + y * 4) & 0xff;
			data[y] = (data[y] << 8) | (j + 0 + y * 4) & 0xff;
		}

		return value;
	}

	static CUInt128 readValue(CFileDataIO* file) {
		return file->ReadUInt128();
	}
	
	static void writeValue(CFileDataIO* file, CUInt128 value) {
		file->WriteUInt128(value);
	}

	static wxString name() { return wxT("CUInt128"); }
};


/////////////////////////////////////////////////////////////////////
// The following tests ensure that the given implementations
// of the CFileDataIO interface properly does so.

template <typename IMPL, typename TYPE, size_t SIZE>
class ReadTest : public FileDataIOFixture<IMPL>
{
	typedef RWInterface<TYPE> RW;
	
public:
	ReadTest()
		: FileDataIOFixture<IMPL>(wxT("Read ") + RW::name()) {}
	
	void run() {
		CFileDataIO* file = this->m_predefFile;
		
		for (size_t j = 0; j < TEST_LENGTH + 1 - SIZE; ++j) {
			ASSERT_EQUALS(j, file->Seek(j, wxFromStart));
			ASSERT_EQUALS(j, file->GetPosition());
			ASSERT_EQUALS(RW::genValue(j), RW::readValue(file));
			ASSERT_EQUALS(j + SIZE, file->GetPosition());
		}

		ASSERT_EQUALS(TEST_LENGTH, file->GetLength());

		// Check reads past EOF
		for (off_t i = 0; i < SIZE; ++i) {
			ASSERT_EQUALS(TEST_LENGTH - i, file->Seek(-i, wxFromEnd));
			ASSERT_RAISES(CEOFException, RW::readValue(file));
		}

		// Check that only the given length is written to the target buffer
		char testBuffer[32];
		memset(testBuffer, 127, 32);
		char* buf = testBuffer + 8;
		
		for (int i = 0; i < 16; ++i) {
			ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
			ASSERT_EQUALS(0, file->GetPosition());

			file->Read(buf, i + 1);

			for (int j = 0; j < 32; ++j) {
				if (j < 8 || j > i + 8) {
					ASSERT_EQUALS(127, (int)testBuffer[j]);
				} else {
					ASSERT_EQUALS(j - 8, (int)testBuffer[j]);
				}
			}
		}		
	}
};


template <typename IMPL, typename TYPE, size_t SIZE>
class WriteTest : public FileDataIOFixture<IMPL>
{
	typedef RWInterface<TYPE> RW;

public:
	WriteTest()
		: FileDataIOFixture<IMPL>(wxT("Write ") + RW::name()) {}

	void run() {
		const unsigned char CanaryData = 170;
		const char canaryBlock[] = { CanaryData };

		CFileDataIO* file = this->m_predefFile;
		
		for (size_t j = 0; j < TEST_LENGTH + 1 - SIZE; ++j) {
			// Clear before, after and at the target byte(s)
			for (int t = -SIZE; t < (int)(2*SIZE); ++t) {
				if ((j + t) < TEST_LENGTH && ((int)j + t) >= 0) {
					file->Seek(j + t, wxFromStart);
					ASSERT_EQUALS(j + t, file->GetPosition());
					file->Write(canaryBlock, 1);
					ASSERT_EQUALS(j + t + 1, file->GetPosition());

					// Check that canary was written
					file->Seek(j + t, wxFromStart);
					ASSERT_EQUALS(CanaryData, file->ReadUInt8());
					ASSERT_EQUALS(j + t + 1, file->GetPosition());					
				}
			}
				
			file->Seek(j, wxFromStart);

			ASSERT_EQUALS(j, file->GetPosition());
			RW::writeValue(file, RW::genValue(j));
			ASSERT_EQUALS(j + SIZE, file->GetPosition());
				
			// Check before, after and at the target byte
			for (int t = -SIZE; t < (int)(2*SIZE); ++t) {
				if ((j + t) < TEST_LENGTH && ((int)j + t) >= 0) {
					if (t) {
						if (t < 0 || t >= (int)SIZE) {
							file->Seek(j + t, wxFromStart);
							ASSERT_EQUALS(CanaryData, file->ReadUInt8());
							ASSERT_EQUALS(j + t + 1, file->GetPosition());
						}
					} else {
						file->Seek(j + t, wxFromStart);
						ASSERT_EQUALS(RW::genValue(j), RW::readValue(file));
						ASSERT_EQUALS(j + t + SIZE, file->GetPosition());
					}
				}
			}
		}

		ASSERT_EQUALS(TEST_LENGTH, file->GetLength());
	}
};


template <typename IMPL>
class SeekTest : public FileDataIOFixture<IMPL>
{
public:
	SeekTest()
		: FileDataIOFixture<IMPL>(wxT("Seek")) {}
	
	void run() {
		CFileDataIO* file = this->m_predefFile;
		
		ASSERT_EQUALS(0, file->GetPosition());
		for (off_t pos = 0; pos < TEST_LENGTH * 2; pos += pos + 1) {
			ASSERT_EQUALS(pos, file->Seek(pos, wxFromStart));
			ASSERT_EQUALS(pos, file->GetPosition());
		}
		
		ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
		ASSERT_EQUALS(0, file->GetPosition());
		
		for (off_t pos = 0, cur = 0; pos < TEST_LENGTH * 2; pos += ++cur) {
			ASSERT_EQUALS(pos, file->Seek(cur, wxFromCurrent));
			ASSERT_EQUALS(pos, file->GetPosition());
		}
	
		ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
		ASSERT_EQUALS(0, file->GetPosition());
		
		for (off_t pos = 0; pos < TEST_LENGTH; pos += pos + 1) {
			ASSERT_EQUALS(TEST_LENGTH - pos, file->Seek(-1 * pos, wxFromEnd));
			ASSERT_EQUALS(TEST_LENGTH - pos, file->GetPosition());
		}

		ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
		ASSERT_EQUALS(0, file->GetPosition());

		// Seek to negative is invalid
		for (off_t pos = 1; pos < 10; ++pos) {
			ASSERT_RAISES(CInvalidParamsEx, file->Seek(-1 * pos));
			ASSERT_EQUALS(0, file->GetPosition());
		}

		// Corner-case
		ASSERT_RAISES(CInvalidParamsEx, file->Seek(std::numeric_limits<off_t>::min()));
		ASSERT_EQUALS(0, file->GetPosition());
	}
};


template <typename IMPL>
class WritePastEndTest : public FileDataIOFixture<IMPL>
{
public:
	WritePastEndTest()
		: FileDataIOFixture<IMPL>(wxT("Write Past End")) {}
	
	void run() {
		CFileDataIO* file = this->m_emptyFile;

		ASSERT_EQUALS(0, file->GetLength());
		ASSERT_EQUALS(0, file->GetPosition());

		file->WriteUInt8(0);

		ASSERT_EQUALS(1, file->GetLength());
		ASSERT_EQUALS(1, file->GetPosition());
		
		file->WriteUInt16(0);

		ASSERT_EQUALS(3, file->GetLength());
		ASSERT_EQUALS(3, file->GetPosition());
		
		file->WriteUInt32(0);

		ASSERT_EQUALS(7, file->GetLength());
		ASSERT_EQUALS(7, file->GetPosition());

		file->WriteHash(CMD4Hash());

		ASSERT_EQUALS(23, file->GetLength());
		ASSERT_EQUALS(23, file->GetPosition());
		
		// TODO: ReadUInt128
		

		char tmp[42];
		bzero(tmp, 42);
		file->Write(tmp, 42);

		ASSERT_EQUALS(65, file->GetLength());
		ASSERT_EQUALS(65, file->GetPosition());

		// Check that the length is always increased, regardless of starting pos
		off_t length = file->GetLength();
		for (size_t j = 0; j < 16; ++j) {
			ASSERT_EQUALS(length + j - 15, file->Seek(-15, wxFromEnd));
			ASSERT_EQUALS(length + j - 15, file->GetPosition());
			file->WriteHash(CMD4Hash());
			ASSERT_EQUALS(length + j + 1, file->GetLength());
			ASSERT_EQUALS(length + j + 1, file->GetPosition());
		}
	}
};


template <typename IMPL>
class StringTest : public FileDataIOFixture<IMPL>
{
public:
	StringTest()
		: FileDataIOFixture<IMPL>(wxT("String")) {}
	
	struct Encoding
	{
		EUtf8Str		id;
		const char*		header;
		size_t			headLen;
	};
	
	void run() {
		CFileDataIO* file = this->m_emptyFile;
	
		// TODO: Need to test non-ascii values when using unicode/etc
		Encoding encodings[] = 
		{
			{utf8strNone,	NULL,			0},
			{utf8strOptBOM,	"\xEF\xBB\xBF",	3},
			{utf8strRaw,	NULL,			0}
		};
		
		
		const wxChar* testData[] = 
		{
			wxT("0123456789abcdef"),
			wxT("")
		};
		
	
		for (size_t str = 0; str < 2; ++str) {
			for (size_t enc = 0; enc < 3; ++enc) {
				const wxChar* curStr = testData[str];
				size_t strLen = wxStrlen(curStr);
				size_t headLen = encodings[enc].headLen;
				
				file->WriteString(curStr, encodings[enc].id, 2);
				ASSERT_EQUALS(strLen + 2 + headLen, file->GetPosition());
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
				ASSERT_EQUALS(strLen + headLen, file->ReadUInt16());

				// Check header (if any)
				if (encodings[enc].header) {
					char head[headLen];
					file->Read(head, headLen);
					ASSERT_EQUALS(0, memcmp(head, encodings[enc].header, headLen));
				}
				
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
				ASSERT_EQUALS(curStr, file->ReadString(encodings[enc].id, 2));
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
				
				
				file->WriteString(curStr, encodings[enc].id, 4);
				ASSERT_EQUALS(strLen + 4 + headLen, file->GetPosition());
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
				ASSERT_EQUALS(strLen + headLen, file->ReadUInt32());

				// Check header (if any)
				if (encodings[enc].header) {
					char head[headLen];
					file->Read(head, headLen);
					ASSERT_EQUALS(0, memcmp(head, encodings[enc].header, headLen));
				}
				
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
				ASSERT_EQUALS(curStr, file->ReadString(encodings[enc].id, 4));
				ASSERT_EQUALS(0, file->Seek(0, wxFromStart));
			}
		}
	}
};


/////////////////////////////////////////////////////////////////////
// Registration of all tests

ReadTest<CFile, uint8, 1>			CFileReadUInt8Test;
ReadTest<CFile, uint16, 2>			CFileReadUInt16Test;
ReadTest<CFile, uint32, 4>			CFileReadUInt32Test;
ReadTest<CFile, CMD4Hash, 16>		CFileReadCMD4HashTest;
ReadTest<CFile, CUInt128, 16>		CFileReadCUInt128Test;

WriteTest<CFile, uint8, 1>			CFileWriteUInt8Test;
WriteTest<CFile, uint16, 2>			CFileWriteUInt16Test;
WriteTest<CFile, uint32, 4>			CFileWriteUInt32Test;
WriteTest<CFile, CMD4Hash, 16>		CFileWriteCMD4HashTest;
WriteTest<CFile, CUInt128, 16>		CFileWriteCUInt128Test;

SeekTest<CFile>						CFileSeekTest;
WritePastEndTest<CFile>				CFileWritePastEnd;
StringTest<CFile>					CFileStringTest;


ReadTest<CMemFile, uint8, 1>		CMemFileReadUInt8Test;
ReadTest<CMemFile, uint16, 2>		CMemFileReadUInt16Test;
ReadTest<CMemFile, uint32, 4>		CMemFileReadUInt32Test;
ReadTest<CMemFile, CMD4Hash, 16>	CMemFileReadCMD4HashTest;
ReadTest<CMemFile, CUInt128, 16>	CMemFileReadCUInt128Test;

WriteTest<CMemFile, uint8, 1>		CMemFileWriteUInt8Test;
WriteTest<CMemFile, uint16, 2>		CMemFileWriteUInt16Test;
WriteTest<CMemFile, uint32, 4>		CMemFileWriteUInt32Test;
WriteTest<CMemFile, CMD4Hash, 16>	CMemFileWriteCMD4HashTest;
WriteTest<CMemFile, CUInt128, 16>	CMemFileWriteCUInt128Test;

SeekTest<CMemFile>					CMemFileSeekTest;
WritePastEndTest<CMemFile>			CMemFileWritePastEnd;
StringTest<CMemFile>				CMemFileStringTest;


// TODO:
#if 0
	wxString	ReadString(bool bOptUTF8, uint8 SizeLen = 2 /* bytes */, bool SafeRead = false) const;
	wxString	ReadOnlyString(bool bOptUTF8, uint16 raw_len) const;

	void WriteString(const wxString& rstr, EUtf8Str eEncode = utf8strNone, uint8 SizeLen = 2 /* bytes */);
#endif
