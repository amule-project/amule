
//
// This file is part of the aMule Project.
//
// Optimized 128-bit integer implementation with SIMD support
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#include "UInt128Optimized.h"
#include "../../ArchSpecific.h"
#include <common/Format.h>  // Needed for CFormat
#include <cstdlib>

namespace Kademlia {

CUInt128Optimized::CUInt128Optimized(const CUInt128Optimized& value, unsigned numBits)
{
    // Copy whole uint32s
    unsigned numULONGs = numBits / 32;
    for (unsigned i = 0; i < numULONGs; i++) {
        Set32BitChunk(i, value.Get32BitChunk(i));
    }

    // Copy remaining bits
    for (unsigned i = numULONGs * 32; i < numBits; i++) {
        SetBitNumber(i, value.GetBitNumber(i));
    }

    // Fill remaining bits of current 32-bit chunk with random bits
    numULONGs = (numBits + 31) / 32;
    for (unsigned i = numBits; i < numULONGs * 32; i++) {
        SetBitNumber(i, rand() % 2);
    }

    // Pad with random bytes
    // NOTE: CUInt128Optimized has 4 uint32 chunks (128 bits), so we need to pad up to index 3
    for (unsigned i = numULONGs; i < 4; i++) {
        Set32BitChunk(i, rand());
    }
}

wxString CUInt128Optimized::ToHexString() const
{
    wxString str;

    for (int i = 3; i >= 0; i--) {
        str.Append(CFormat(wxT("%08X")) % m_data.u32_data[i]);
    }

    return str;
}

wxString CUInt128Optimized::ToBinaryString(bool trim) const
{
    wxString str;
    str.Alloc(128);
    int b;
    for (int i = 0; i < 128; i++) {
        b = GetBitNumber(i);
        if ((!trim) || (b != 0)) {
            str.Append(b ? wxT("1") : wxT("0"));
            trim = false;
        }
    }
    if (str.Len() == 0) {
        str = wxT("0");
    }
    return str;
}

CUInt128Optimized& CUInt128Optimized::SetValueBE(const uint8_t *valueBE) throw()
{
    m_data.u32_data[3] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE));
    m_data.u32_data[2] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 4));
    m_data.u32_data[1] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 8));
    m_data.u32_data[0] = wxUINT32_SWAP_ON_LE(RawPeekUInt32(valueBE + 12));

    return *this;
}

void CUInt128Optimized::ToByteArray(uint8_t *b) const
{
    RawPokeUInt32(b,      wxUINT32_SWAP_ON_LE(m_data.u32_data[3]));
    RawPokeUInt32(b + 4,  wxUINT32_SWAP_ON_LE(m_data.u32_data[2]));
    RawPokeUInt32(b + 8,  wxUINT32_SWAP_ON_LE(m_data.u32_data[1]));
    RawPokeUInt32(b + 12, wxUINT32_SWAP_ON_LE(m_data.u32_data[0]));
}

void CUInt128Optimized::StoreCryptValue(uint8_t *buf) const
{
    RawPokeUInt32(buf,      wxUINT32_SWAP_ON_BE(m_data.u32_data[3]));
    RawPokeUInt32(buf + 4,  wxUINT32_SWAP_ON_BE(m_data.u32_data[2]));
    RawPokeUInt32(buf + 8,  wxUINT32_SWAP_ON_BE(m_data.u32_data[1]));
    RawPokeUInt32(buf + 12, wxUINT32_SWAP_ON_BE(m_data.u32_data[0]));
}

int CUInt128Optimized::CompareTo(const CUInt128Optimized& other) const throw()
{
    // Use SIMD comparison for better performance
    __m128i cmp = _mm_cmpeq_epi32(m_data.v128, other.m_data.v128);

    // Check if all 128 bits are equal using SSE2-compatible approach
    if (_mm_movemask_epi8(cmp) == 0xFFFF) {
        return 0;
    }

    // Compare from most significant to least significant
    for (int i = 3; i >= 0; i--) {
        if (m_data.u32_data[i] < other.m_data.u32_data[i])
            return -1;
        if (m_data.u32_data[i] > other.m_data.u32_data[i])
            return 1;
    }
    return 0;
}

int CUInt128Optimized::CompareTo(uint32_t value) const throw()
{
    if ((m_data.u64_data[1] > 0) || (m_data.u32_data[1] > 0) || 
        (m_data.u32_data[0] > value))
        return 1;
    if (m_data.u32_data[0] < value)
        return -1;
    return 0;
}

CUInt128Optimized& CUInt128Optimized::Add(const CUInt128Optimized& value) throw()
{
    if (value.IsZero()) return *this;

    // Use standard 64-bit arithmetic for addition (more portable)
    uint64_t carry = 0;
    for (int i = 0; i < 2; i++) {
        uint64_t a = m_data.u64_data[i];
        uint64_t b = value.m_data.u64_data[i];
        m_data.u64_data[i] = a + b + carry;
        // Check for overflow (carry to next iteration)
        carry = (m_data.u64_data[i] < a || (m_data.u64_data[i] == a && b > 0)) ? 1 : 0;
    }

    return *this;
}

CUInt128Optimized& CUInt128Optimized::Subtract(const CUInt128Optimized& value) throw()
{
    if (value.IsZero()) return *this;

    // Use standard 64-bit arithmetic for subtraction (more portable)
    uint64_t borrow = 0;
    for (int i = 0; i < 2; i++) {
        uint64_t a = m_data.u64_data[i];
        uint64_t b = value.m_data.u64_data[i];
        m_data.u64_data[i] = a - b - borrow;
        // Check for underflow (borrow from next iteration)
        borrow = (m_data.u64_data[i] > a) ? 1 : 0;
    }

    return *this;
}

CUInt128Optimized& CUInt128Optimized::ShiftLeft(unsigned bits) throw()
{
    if ((bits == 0) || IsZero())
        return *this;

    if (bits > 127) {
        SetValue((uint32_t)0);
        return *this;
    }

    union {
        uint32_t u32_data[4];
        uint64_t u64_data[2];
        __m128i v128;
    } result = {{ 0, 0, 0, 0 }};
    int indexShift = (int)bits / 32;
    int64_t shifted = 0;

    for (int i = 3; i >= indexShift; i--) {
        shifted += ((int64_t)m_data.u32_data[3 - i]) << (bits % 32);
        result.u32_data[3 - i + indexShift] = (uint32_t)shifted;
        shifted = shifted >> 32;
    }

    m_data.v128 = result.v128;
    return *this;
}

} // End namespace
