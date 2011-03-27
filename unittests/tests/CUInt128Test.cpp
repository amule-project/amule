//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <muleunit/test.h>
#include <kademlia/utils/UInt128.h>

using namespace muleunit;
using Kademlia::CUInt128;

namespace muleunit {
	// Needed for ASSERT_EQUALS with CUInt128
	template<> wxString StringFrom<CUInt128>(const CUInt128& value) {
		return value.ToHexString();
	}

	typedef uint8_t ByteArray[16];
	// Needed for ASSERT_EQUALS with uint8_t[16]
	template<> wxString StringFrom<ByteArray>(const ByteArray& value) {
		wxString retval;
		for (int i = 0; i < 16; i++) {
			if (i) {
				retval.Append(wxString::Format(wxT(" %02X"), value[i]));
			} else {
				retval.Append(wxString::Format(wxT("%02X"), value[i]));
			}
		}
		return retval;
	}
}

// Put static test data into a namespace to avoid possible clash with muleunit namespace
namespace TestData {
	static uint8_t sequence[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };
	static uint8_t zero[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	static uint8_t one[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	static uint8_t minusOne[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	static uint8_t uintValue[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12, 0x34, 0x56, 0x78 };
	static uint8_t randomValue[16] = { 0xef, 0xac, 0xd6, 0x21, 0x99, 0x1b, 0x05, 0xbe, 0xfb, 0x97, 0xdf, 0xdd, 0xab, 0x4b, 0x88, 0xe3 };
}

DECLARE_SIMPLE(CUInt128);

// Each test uses only functionality previously tested.

TEST_M(CUInt128, ConstructorAndToByteArray, wxT("Reading/writing byte sequences"))
{
	uint8_t result[16];

	CUInt128 test((uint8_t *)&TestData::sequence);
	test.ToByteArray((uint8_t *)&result);
	ASSERT_TRUE_M(TestData::sequence[0] == result[0] &&
		      TestData::sequence[1] == result[1] &&
		      TestData::sequence[2] == result[2] &&
		      TestData::sequence[3] == result[3] &&
		      TestData::sequence[4] == result[4] &&
		      TestData::sequence[5] == result[5] &&
		      TestData::sequence[6] == result[6] &&
		      TestData::sequence[7] == result[7] &&
		      TestData::sequence[8] == result[8] &&
		      TestData::sequence[9] == result[9] &&
		      TestData::sequence[10] == result[10] &&
		      TestData::sequence[11] == result[11] &&
		      TestData::sequence[12] == result[12] &&
		      TestData::sequence[13] == result[13] &&
		      TestData::sequence[14] == result[14] &&
		      TestData::sequence[15] == result[15],
		      wxString(wxT("Expected '")) + StringFrom(TestData::sequence) + wxT("' but got '") + StringFrom(result) + wxT("'"));
}

TEST(CUInt128, ToHexString)
{
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_EQUALS(wxT("000102030405060708090A0B0C0D0E0F"), test.ToHexString());
}

TEST(CUInt128, ToBinaryString)
{
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_EQUALS(wxT("00000000000000010000001000000011000001000000010100000110000001110000100000001001000010100000101100001100000011010000111000001111"), test.ToBinaryString());
	ASSERT_EQUALS(wxT("10000001000000011000001000000010100000110000001110000100000001001000010100000101100001100000011010000111000001111"), test.ToBinaryString(true));
	CUInt128 testZero((uint8_t *)&TestData::zero);
	ASSERT_EQUALS(wxT("0"), testZero.ToBinaryString(true));
}

TEST(CUInt128, Get32BitChunk)
{
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_EQUALS(0x00010203u, test.Get32BitChunk(0));
	ASSERT_EQUALS(0x04050607u, test.Get32BitChunk(1));
	ASSERT_EQUALS(0x08090a0bu, test.Get32BitChunk(2));
	ASSERT_EQUALS(0x0c0d0e0fu, test.Get32BitChunk(3));
	ASSERT_EQUALS(0u, test.Get32BitChunk(4));
}

TEST_M(CUInt128, OperatorEqualsCUInt128, wxT("operator==(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b((uint8_t *)&TestData::one);

	ASSERT_TRUE(a == a);
	ASSERT_FALSE(a == b);
}

TEST_M(CUInt128, OperatorEqualsUint32, wxT("operator==(uint32_t)"))
{
	ASSERT_TRUE(CUInt128((uint8_t *)&TestData::uintValue) == 0x12345678u);
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_FALSE(test == 0x00010203u);
	ASSERT_FALSE(test == 0x04050607u);
	ASSERT_FALSE(test == 0x08090a0bu);
	ASSERT_FALSE(test == 0x0c0d0e0fu);
}

TEST_M(CUInt128, OperatorEqualsUint32CUInt128, wxT("operator==(uint32_t, const CUInt128&)"))
{
	ASSERT_TRUE(0x12345678u == CUInt128((uint8_t *)&TestData::uintValue));
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_FALSE(0x00010203u == test);
	ASSERT_FALSE(0x04050607u == test);
	ASSERT_FALSE(0x08090a0bu == test);
	ASSERT_FALSE(0x0c0d0e0fu == test);
}

TEST_M(CUInt128, EmptyContructor, wxT("CUInt128()"))
{
	CUInt128 test;
	CUInt128 ref((uint8_t *)&TestData::zero);

	ASSERT_EQUALS(ref, test);
}

TEST_M(CUInt128, CopyConstructor, wxT("CUInt128(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b(a);

	ASSERT_EQUALS(a, b);
}

TEST_M(CUInt128, ConstructFromBool, wxT("CUInt128(bool)"))
{
	CUInt128 empty(false);
	CUInt128 full(true);
	CUInt128 zero((uint8_t *)&TestData::zero);
	CUInt128 minusOne((uint8_t *)&TestData::minusOne);

	ASSERT_EQUALS(zero, empty);
	ASSERT_EQUALS(minusOne, full);
}

TEST_M(CUInt128, ConstructFromUint32, wxT("CUInt128(uint32_t)"))
{
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::uintValue), CUInt128(0x12345678u));
}

TEST_M(CUInt128, AssignCUInt128, wxT("operator=(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b = a;

	ASSERT_EQUALS(a, b);
}

TEST_M(CUInt128, AssignUint32, wxT("operator=(uint32_t)"))
{
	CUInt128 a((uint8_t *)&TestData::uintValue);
	// Note that CUInt128 b = 0x12345678u; won't work,
	// the compiler only allows assignment between the
	// same types when constructing the object.
	CUInt128 b;
	b = 0x12345678u;

	ASSERT_EQUALS(a, b);
}

TEST(CUInt128, Set32BitChunk)
{
	CUInt128 test;

	test.Set32BitChunk(3, 0x12345678u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::uintValue), test);

	test.Set32BitChunk(0, 0x00010203u);
	test.Set32BitChunk(1, 0x04050607u);
	test.Set32BitChunk(2, 0x08090a0bu);
	test.Set32BitChunk(3, 0x0c0d0e0fu);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), test);
#ifdef __WXDEBUG__
	ASSERT_RAISES(CAssertFailureException, test.Set32BitChunk(4, 0));
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), test);
#endif

	CAssertOff null;
	test.Set32BitChunk(4, 0);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), test);
}

TEST_M(CUInt128, SetValueCUInt128, wxT("SetValue(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b;

	b.SetValue(a);
	ASSERT_EQUALS(a, b);
}

TEST_M(CUInt128, SetValueUint32, wxT("SetValue(uint32_t)"))
{
	CUInt128 a((uint8_t *)&TestData::uintValue);
	CUInt128 b;

	b.SetValue(0x12345678u);
	ASSERT_EQUALS(a, b);
}

TEST(CUInt128, SetValueBE)
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b;
	uint8_t buffer[16];

	a.ToByteArray((uint8_t *)&buffer);
	b.SetValueBE((uint8_t *)&buffer);
	ASSERT_EQUALS(a, b);
}

TEST(CUInt128, StoreCryptValue)
{
	uint8_t ref[16] = { 0x3, 0x2, 0x1, 0x0, 0x7, 0x6, 0x5, 0x4, 0xb, 0xa, 0x9, 0x8, 0xf, 0xe, 0xd, 0xc };
	uint8_t tmp[16];
	CUInt128 test((uint8_t *)&TestData::sequence);

	test.StoreCryptValue((uint8_t *)&tmp);
	ASSERT_EQUALS(CUInt128((uint8_t *)&ref), CUInt128((uint8_t *)&tmp));
}

TEST(CUInt128, ShiftLeft)
{
	CUInt128 test((uint8_t *)&TestData::one);
	uint8_t r1[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
	uint8_t r2[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0 };
	uint8_t r3[16] = { 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t r4[16] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	test.ShiftLeft(0);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::one), test);
	test.ShiftLeft(1);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), test);
	test.ShiftLeft(32);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), test);
	test.ShiftLeft(58);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), test);
	test.SetValueBE((uint8_t *)&TestData::one);
	test.ShiftLeft(127);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r4), test);
	test.SetValueBE((uint8_t *)&TestData::one);
	test.ShiftLeft(128);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::zero), test);
}

TEST_M(CUInt128, AddCUInt128, wxT("Add(const CUInt128&)"))
{
	uint8_t d0[16] = { 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r3[16] = { 0xfc, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r4[16] = { 0xf8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 d((uint8_t *)&d0);

	a.Add(CUInt128(0x01234567u));
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a.Add(CUInt128(0xff000000u));
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a.Add(d);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&d0), d);
	a.Add(d);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r4), a);
	a.SetValueBE((uint8_t *)&TestData::minusOne);
	a.Add(CUInt128((uint8_t *)&TestData::one));
	ASSERT_EQUALS(CUInt128(), a);
}

TEST_M(CUInt128, AddUint32, wxT("Add(uint32_t)"))
{
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&TestData::sequence);

	a.Add(0x01234567u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a.Add(0xff000000u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a.SetValueBE((uint8_t *)&TestData::minusOne);
	a.Add(1u);
	ASSERT_EQUALS(0, a);
}

TEST_M(CUInt128, SubtractCUInt128, wxT("Subtract(const CUInt128&)"))
{
	uint8_t d0[16] = { 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r3[16] = { 0xfc, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r4[16] = { 0xf8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&r4);
	CUInt128 d((uint8_t *)&d0);

	a.Subtract(d);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&d0), d);
	a.Subtract(d);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a.Subtract(CUInt128(0xff000000u));
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a.Subtract(CUInt128(0x01234567u));
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), a);
	a.SetValue(0u);
	a.Subtract(CUInt128((uint8_t *)&TestData::one));
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::minusOne), a);
}

TEST_M(CUInt128, SubtractUint32, wxT("Subtract(uint32_t)"))
{
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&r2);

	a.Subtract(0xff000000u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a.Subtract(0x01234567u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), a);
	a.SetValue(0u);
	a.Subtract(1u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::minusOne), a);
}

TEST_M(CUInt128, XorCUInt128, wxT("XOR(const CUInt128&)"))
{
	uint8_t xd[16] = { 0xff, 0x00, 0xee, 0x11, 0xdd, 0x22, 0xcc, 0x33, 0xbb, 0x44, 0xaa, 0x55, 0x99, 0x66, 0x88, 0x77 };
	uint8_t xr[16] = { 0xff, 0x01, 0xec, 0x12, 0xd9, 0x27, 0xca, 0x34, 0xb3, 0x4d, 0xa0, 0x5e, 0x95, 0x6b, 0x86, 0x78 };
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 x((uint8_t *)&xd);

	a.XOR(x);
	ASSERT_EQUALS(CUInt128((uint8_t *)&xr), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&xd), x);
}

// Not yet implemented
#if 0
TEST_M(CUInt128, XorUint32, wxT("XOR(uint32_t)"))
{
	CUInt128 a(0x12345678u);

	a.XOR(0x76543210u);
	ASSERT_EQUALS(0x64606468, a);
}
#endif

TEST_M(CUInt128, OperatorShiftLeftAssign, wxT("operator<<=(unsigned)"))
{
	CUInt128 test((uint8_t *)&TestData::one);
	uint8_t r1[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 };
	uint8_t r2[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0 };
	uint8_t r3[16] = { 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t r4[16] = { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	test <<= 0;
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::one), test);
	test <<= 1;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), test);
	test <<= 32;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), test);
	test <<= 58;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), test);
	test.SetValueBE((uint8_t *)&TestData::one);
	test <<= 127;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r4), test);
	test.SetValueBE((uint8_t *)&TestData::one);
	test <<= 128;
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::zero), test);
}

TEST_M(CUInt128, OperatorAddAssignCUInt128, wxT("operator+=(const CUInt128&)"))
{
	uint8_t d0[16] = { 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r3[16] = { 0xfc, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r4[16] = { 0xf8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 d((uint8_t *)&d0);

	a += CUInt128(0x01234567u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a += CUInt128(0xff000000u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a += d;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&d0), d);
	a += d;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r4), a);
	a.SetValueBE((uint8_t *)&TestData::minusOne);
	a += CUInt128((uint8_t *)&TestData::one);
	ASSERT_EQUALS(CUInt128(), a);
}

TEST_M(CUInt128, OperatorAddAssignUint32, wxT("operator+=(uint32_t)"))
{
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&TestData::sequence);

	a += 0x01234567u;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a += 0xff000000u;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a.SetValueBE((uint8_t *)&TestData::minusOne);
	a += 1;
	ASSERT_EQUALS(0, a);
}

TEST_M(CUInt128, OperatorSubtractAssignCUInt128, wxT("operator-=(const CUInt128&)"))
{
	uint8_t d0[16] = { 0xfc, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r3[16] = { 0xfc, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	uint8_t r4[16] = { 0xf8, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&r4);
	CUInt128 d((uint8_t *)&d0);

	a -= d;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r3), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&d0), d);
	a -= d;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r2), a);
	a -= CUInt128(0xff000000u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a -= CUInt128(0x01234567u);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), a);
	a.SetValue(0u);
	a -= CUInt128((uint8_t *)&TestData::one);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::minusOne), a);
}

TEST_M(CUInt128, OperatorSubtractAssignUint32, wxT("operator-=(uint32_t)"))
{
	uint8_t r1[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xd, 0x30, 0x53, 0x76 };
	uint8_t r2[16] = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xc, 0xc, 0x30, 0x53, 0x76 };
	CUInt128 a((uint8_t *)&r2);

	a -= 0xff000000u;
	ASSERT_EQUALS(CUInt128((uint8_t *)&r1), a);
	a -= 0x01234567u;
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), a);
	a = 0;
	a -= 1;
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::minusOne), a);
}

TEST_M(CUInt128, OperatorXorAssignCUInt128, wxT("operator^=(const CUInt128&)"))
{
	uint8_t xd[16] = { 0xff, 0x00, 0xee, 0x11, 0xdd, 0x22, 0xcc, 0x33, 0xbb, 0x44, 0xaa, 0x55, 0x99, 0x66, 0x88, 0x77 };
	uint8_t xr[16] = { 0xff, 0x01, 0xec, 0x12, 0xd9, 0x27, 0xca, 0x34, 0xb3, 0x4d, 0xa0, 0x5e, 0x95, 0x6b, 0x86, 0x78 };
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 x((uint8_t *)&xd);

	a ^= x;
	ASSERT_EQUALS(CUInt128((uint8_t *)&xr), a);
	ASSERT_EQUALS(CUInt128((uint8_t *)&xd), x);
}

TEST_M(CUInt128, OperatorXorAssignUint32, wxT("operator^=(uint32_t)"))
{
	CUInt128 a(0x12345678u);

	a ^= 0x76543210u;
	ASSERT_EQUALS(0x64606468, a);
}

TEST(CUInt128, GetBitNumber)
{
	CUInt128 test;
	test = 1;

	ASSERT_TRUE(test.GetBitNumber(0) == 0);
	ASSERT_TRUE(test.GetBitNumber(127) == 1);
	test <<= 127;
	ASSERT_TRUE(test.GetBitNumber(0) == 1);
	ASSERT_TRUE(test.GetBitNumber(127) == 0);

	CUInt128 test2(true);
	ASSERT_EQUALS(0u, test2.GetBitNumber(128));
}

TEST(CUInt128, SetBitNumber)
{
	CUInt128 test;

	test.SetBitNumber(127, 1);
	ASSERT_EQUALS(1, test);
	test.SetBitNumber(126, 1);
	ASSERT_EQUALS(3, test);
	test.SetBitNumber(127, 0);
	ASSERT_EQUALS(2, test);
	test.SetBitNumber(0, 1);
	ASSERT_EQUALS(0x80000000u, test.Get32BitChunk(0));
#ifdef __WXDEBUG__
	ASSERT_RAISES(CAssertFailureException, test.SetBitNumber(128, 0));
#endif

	CAssertOff null;
	test.SetValueBE((uint8_t *)&TestData::sequence);
	test.SetBitNumber(128, 1);
	ASSERT_EQUALS(CUInt128((uint8_t *)&TestData::sequence), test);
}

TEST_M(CUInt128, OperatorNotEqualCUInt128, wxT("operator!=(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);
	CUInt128 b((uint8_t *)&TestData::one);

	ASSERT_FALSE(a != a);
	ASSERT_TRUE(a != b);
}

TEST_M(CUInt128, OperatorNotEqualUint32, wxT("operator!=(uint32_t)"))
{
	ASSERT_FALSE(CUInt128((uint8_t *)&TestData::uintValue) != 0x12345678u);
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_TRUE(test != 0x00010203u);
	ASSERT_TRUE(test != 0x04050607u);
	ASSERT_TRUE(test != 0x08090a0bu);
	ASSERT_TRUE(test != 0x0c0d0e0fu);
}

TEST_M(CUInt128, OperatorNotEqualUint32CUInt128, wxT("operator!=(uint32_t, const CUInt128&)"))
{
	ASSERT_FALSE(0x12345678u != CUInt128((uint8_t *)&TestData::uintValue));
	CUInt128 test((uint8_t *)&TestData::sequence);
	ASSERT_TRUE(0x00010203u != test);
	ASSERT_TRUE(0x04050607u != test);
	ASSERT_TRUE(0x08090a0bu != test);
	ASSERT_TRUE(0x0c0d0e0fu != test);
}

TEST_M(CUInt128, OperatorLessCUInt128, wxT("operator<(const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		CUInt128 b;
		a.Set32BitChunk(i, 0x0d3859feu);
		b.Set32BitChunk(i, 0xff579ec1u);
		ASSERT_TRUE(a < b);
	}
}

TEST_M(CUInt128, OperatorLessOrEqualCUInt128, wxT("operator<=(const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		CUInt128 b;
		a.Set32BitChunk(i, 0x0d3859feu);
		b.Set32BitChunk(i, 0xff579ec1u);
		ASSERT_TRUE(a <= b);
		b.Set32BitChunk(i, a.Get32BitChunk(i));
		ASSERT_TRUE(a <= b);
	}
}

TEST_M(CUInt128, OperatorGreaterCUInt128, wxT("operator>(const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		CUInt128 b;
		b.Set32BitChunk(i, 0x0d3859feu);
		a.Set32BitChunk(i, 0xff579ec1u);
		ASSERT_TRUE(a > b);
	}
}

TEST_M(CUInt128, OperatorGreaterOrEqualCUInt128, wxT("operator>=(const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		CUInt128 b;
		b.Set32BitChunk(i, 0x0d3859feu);
		a.Set32BitChunk(i, 0xff579ec1u);
		ASSERT_TRUE(a >= b);
		b.Set32BitChunk(i, a.Get32BitChunk(i));
		ASSERT_TRUE(a >= b);
	}
}

TEST_M(CUInt128, OperatorLessUint32, wxT("operator<(uint32_t)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0x0d3859feu);
		if (i == 3) {
			ASSERT_TRUE(a < 0xffed3216u);
		} else {
			ASSERT_FALSE(a < 0xffed3216u);
		}
	}
}

TEST_M(CUInt128, OperatorLessOrEqualUint32, wxT("operator<=(uint32_t)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0x0d3859feu);
		if (i == 3) {
			ASSERT_TRUE(a <= 0xffed3216u);
			ASSERT_TRUE(a <= 0x0d3859feu);
		} else {
			ASSERT_FALSE(a <= 0xffed3216u);
			ASSERT_FALSE(a <= 0x0d3859feu);
		}
	}
}

TEST_M(CUInt128, OperatorGreaterUint32, wxT("operator>(uint32_t)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0xffed3216u);
		ASSERT_TRUE(a > 0x0d3859feu);
	}
}

TEST_M(CUInt128, OperatorGreaterOrEqualUint32, wxT("operator>=(uint32_t)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0xffed3216u);
		ASSERT_TRUE(a >= 0x0d3859feu);
		ASSERT_TRUE(a >= 0xffed3216u);
	}
}

TEST_M(CUInt128, OperatorLessUint32CUInt128, wxT("operator<(uint32_t, const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0xffed3216u);
		ASSERT_TRUE(a > 0x0d3859feu);
	}
}

TEST_M(CUInt128, OperatorLessOrEqualUint32CUInt128, wxT("operator<=(uint32_t, const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0xffed3216u);
		ASSERT_TRUE(0x0d3859feu <= a);
		ASSERT_TRUE(0xffed3216u <= a);
	}
}

TEST_M(CUInt128, OperatorGreaterUint32CUInt128, wxT("operator>(uint32_t, const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0x0d3859feu);
		if (i == 3) {
			ASSERT_TRUE(0xffed3216u > a);
		} else {
			ASSERT_FALSE(0xffed3216u > a);
		}
	}
}

TEST_M(CUInt128, OperatorGreaterOrEqualUint32CUInt128, wxT("operator>=(uint32_t, const CUInt128&)"))
{
	for (unsigned i = 0; i < 4; i++) {
		CUInt128 a;
		a.Set32BitChunk(i, 0x0d3859feu);
		if (i == 3) {
			ASSERT_TRUE(0xffed3216u >= a);
			ASSERT_TRUE(0x0d3859feu >= a);
		} else {
			ASSERT_FALSE(0xffed3216u >= a);
			ASSERT_FALSE(0x0d3859feu >= a);
		}
	}
}

TEST_M(CUInt128, OperatorAddCUInt128, wxT("operator+(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::sequence);

	CUInt128 ref(a);
	CUInt128 check(a);

	CUInt128 result(a + a);
	check.Add(ref);
	ASSERT_EQUALS(check, result);
	ASSERT_EQUALS(ref, a);

	a = ref;
	a.SetBitNumber(32, 1);
	check = a;
	check += a;
	result = a + a;
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorSubtractCUInt128, wxT("operator-(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	CUInt128 b((uint8_t *)&TestData::sequence);

	CUInt128 refa(a);
	CUInt128 refb(b);
	CUInt128 check(a);

	CUInt128 result(a - b);
	ASSERT_EQUALS(refa, a);
	ASSERT_EQUALS(refb, b);
	check.Subtract(b);
	ASSERT_EQUALS(check, result);

	result = b - a;
	check = b;
	check -= a;
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorXorCUInt128, wxT("operator^(const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	CUInt128 b((uint8_t *)&TestData::sequence);

	CUInt128 refa(a);
	CUInt128 refb(b);

	CUInt128 result(a ^ b);
	CUInt128 check(refa);
	check ^= refb;

	ASSERT_EQUALS(refa, a);
	ASSERT_EQUALS(refb, b);
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorAddUint32, wxT("operator+(uint32_t)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);
	CUInt128 check(a);

	CUInt128 result(a + b);
	check.Add(b);
	ASSERT_EQUALS(check, result);
	ASSERT_EQUALS(ref, a);

	a = ref;
	a.SetBitNumber(96, 1);
	b = a.Get32BitChunk(3);
	check = a;
	check += b;
	result = a + b;
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorSubtractUint32, wxT("operator-(uint32_t)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);
	CUInt128 check(a);

	CUInt128 result(a - b);
	ASSERT_EQUALS(ref, a);
	check.Subtract(b);
	ASSERT_EQUALS(check, result);

	result = a - ~b;
	check = a;
	check -= ~b;
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorXorUint32, wxT("operator^(uint32_t)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);

	CUInt128 result(a ^ b);
	CUInt128 check(ref);
	check ^= b;

	ASSERT_EQUALS(ref, a);
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorAddUint32CUInt128, wxT("operator+(uint32_t, const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);
	CUInt128 check(b);

	CUInt128 result(b + a);
	check.Add(a);
	ASSERT_EQUALS(check, result);
	ASSERT_EQUALS(ref, a);

	a = ref;
	a.SetBitNumber(96, 1);
	b = a.Get32BitChunk(3);
	check = b;
	check += a;
	result = b + a;
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorSubtractUint32CUInt128, wxT("operator-(uint32_t, const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);
	CUInt128 check(b);

	CUInt128 result(b - a);
	ASSERT_EQUALS(ref, a);
	check.Subtract(a);
	ASSERT_EQUALS(check, result);
}

TEST_M(CUInt128, OperatorXorUint32CUInt128, wxT("operator^(uint32_t, const CUInt128&)"))
{
	CUInt128 a((uint8_t *)&TestData::randomValue);
	uint32_t b = a.Get32BitChunk(0);

	CUInt128 ref(a);
	CUInt128 check(b);

	CUInt128 result(b ^ a);
	check ^= a;

	ASSERT_EQUALS(ref, a);
	ASSERT_EQUALS(check, result);
}
