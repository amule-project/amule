
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

#ifndef __UINT128_OPTIMIZED_H__
#define __UINT128_OPTIMIZED_H__

#include "../../Types.h"
#include <cstring>
#include <immintrin.h>

namespace Kademlia {

/**
 * Optimized 128-bit integer class with SIMD support
 * 
 * This implementation provides:
 * - SIMD-optimized XOR operations (critical for Kademlia distance calculations)
 * - Efficient bit operations
 * - Optimized comparison and conversion functions
 * - Maintain API compatibility with original CUInt128
 */
class CUInt128Optimized
{
public:
    // Constructors maintaining API compatibility
    CUInt128Optimized(const CUInt128Optimized& value) throw()
    {
        SetValue(value);
    }

    explicit CUInt128Optimized(bool fill = false) throw()
    {
        if (fill) {
            m_data.v128 = _mm_set1_epi32(0xFFFFFFFF);
        } else {
            m_data.v128 = _mm_setzero_si128();
        }
    }

    explicit CUInt128Optimized(uint32_t value) throw()
    {
        SetValue(value);
    }

    explicit CUInt128Optimized(const uint8_t *valueBE) throw()
    {
        SetValueBE(valueBE);
    }

    /**
     * Generates a new number, copying of most significant 'numBits' bits from 'value'.
     * The remaining bits are randomly generated.
     */
    CUInt128Optimized(const CUInt128Optimized& value, unsigned numBits);

    /* Bit at level 0 being most significant. */
    unsigned GetBitNumber(unsigned bit) const throw()
    {
        if (bit > 127) return 0;
        return (m_data.u32_data[(127 - bit) / 32] >> ((127 - bit) % 32)) & 1;
    }

    /* Bit at level 0 being most significant. */
    CUInt128Optimized& SetBitNumber(unsigned bit, unsigned value)
    {
        if (bit > 127) return *this;

        if (value) {
            m_data.u32_data[(127 - bit) / 32] |= 1 << ((127 - bit) % 32);
        } else {
            m_data.u32_data[(127 - bit) / 32] &= ~(1 << ((127 - bit) % 32));
        }

        return *this;
    }

    /* Chunk 0 being the most significant */
    uint32_t Get32BitChunk(unsigned val) const throw()
    {
        return val < 4 ? m_data.u32_data[3 - val] : 0;
    }

    /* Chunk 0 being the most significant */
    void Set32BitChunk(unsigned chunk, uint32_t value)
    {
        if (chunk < 4) {
            m_data.u32_data[3 - chunk] = value;
        }
    }

    CUInt128Optimized& SetValueBE(const uint8_t *valueBE) throw();

    wxString ToHexString() const;
    wxString ToBinaryString(bool trim = false) const;
    void ToByteArray(uint8_t *b) const;
    void StoreCryptValue(uint8_t *buf) const;

private:
    int CompareTo(const CUInt128Optimized& other) const throw();
    int CompareTo(uint32_t value) const throw();
    CUInt128Optimized& Add(const CUInt128Optimized& value) throw();
    CUInt128Optimized& Add(uint32_t value) throw() 
    { 
        return value ? Add(CUInt128Optimized(value)) : *this; 
    }

    CUInt128Optimized& Subtract(const CUInt128Optimized& value) throw();
    CUInt128Optimized& Subtract(uint32_t value) throw() 
    { 
        return value ? Subtract(CUInt128Optimized(value)) : *this; 
    }

    CUInt128Optimized& ShiftLeft(unsigned bits) throw();

    /**
     * SIMD-optimized XOR operation
     * This is the most critical operation for Kademlia distance calculations
     */
    CUInt128Optimized& XOR(const CUInt128Optimized& value) throw()
    {
        m_data.v128 = _mm_xor_si128(m_data.v128, value.m_data.v128);
        return *this;
    }

    bool IsZero() const throw() 
    { 
        // Use SSE2-compatible comparison instead of SSE4.1 _mm_test_all_zeros
        __m128i zero = _mm_setzero_si128();
        __m128i cmp = _mm_cmpeq_epi32(m_data.v128, zero);
        return _mm_movemask_epi8(cmp) == 0xFFFF;
    }

public:
    // Comparison operators
    bool operator< (const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) < 0); 
    }

    bool operator> (const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) > 0); 
    }

    bool operator<=(const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) <= 0); 
    }

    bool operator>=(const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) >= 0); 
    }

    bool operator==(const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) == 0); 
    }

    bool operator!=(const CUInt128Optimized& value) const throw() 
    { 
        return (CompareTo(value) != 0); 
    }

    // Comparison with uint32_t
    bool operator< (uint32_t value) const throw() 
    { 
        return (CompareTo(value) < 0); 
    }

    bool operator> (uint32_t value) const throw() 
    { 
        return (CompareTo(value) > 0); 
    }

    bool operator<=(uint32_t value) const throw() 
    { 
        return (CompareTo(value) <= 0); 
    }

    bool operator>=(uint32_t value) const throw() 
    { 
        return (CompareTo(value) >= 0); 
    }

    bool operator==(uint32_t value) const throw() 
    { 
        return (CompareTo(value) == 0); 
    }

    bool operator!=(uint32_t value) const throw() 
    { 
        return (CompareTo(value) != 0); 
    }

    // Assignment operators
    CUInt128Optimized& operator= (const CUInt128Optimized& value) throw() 
    { 
        SetValue(value); 
        return *this; 
    }

    CUInt128Optimized& operator= (uint32_t value) throw() 
    { 
        SetValue(value); 
        return *this; 
    }

    // Arithmetic operators
    CUInt128Optimized& operator+=(const CUInt128Optimized& value) throw() 
    { 
        return Add(value); 
    }

    CUInt128Optimized& operator-=(const CUInt128Optimized& value) throw() 
    { 
        return Subtract(value); 
    }

    CUInt128Optimized& operator^=(const CUInt128Optimized& value) throw() 
    { 
        return XOR(value); 
    }

    CUInt128Optimized& operator+=(uint32_t value) throw() 
    { 
        return Add(value); 
    }

    CUInt128Optimized& operator-=(uint32_t value) throw() 
    { 
        return Subtract(value); 
    }

    CUInt128Optimized& operator^=(uint32_t value) throw() 
    { 
        return value ? XOR(CUInt128Optimized(value)) : *this; 
    }

    CUInt128Optimized& operator<<=(unsigned bits) throw() 
    { 
        return ShiftLeft(bits); 
    }

    // Binary operators
    CUInt128Optimized  operator+(const CUInt128Optimized& value) const throw() 
    { 
        return CUInt128Optimized(*this).operator+=(value); 
    }

    CUInt128Optimized  operator-(const CUInt128Optimized& value) const throw() 
    { 
        return CUInt128Optimized(*this).operator-=(value); 
    }

    CUInt128Optimized  operator^(const CUInt128Optimized& value) const throw() 
    { 
        return CUInt128Optimized(*this).operator^=(value); 
    }

    CUInt128Optimized  operator+(uint32_t value) const throw() 
    { 
        return CUInt128Optimized(*this).operator+=(value); 
    }

    CUInt128Optimized  operator-(uint32_t value) const throw() 
    { 
        return CUInt128Optimized(*this).operator-=(value); 
    }

    CUInt128Optimized  operator^(uint32_t value) const throw() 
    { 
        return CUInt128Optimized(*this).operator^=(value); 
    }

    CUInt128Optimized  operator<<(unsigned bits) const throw() 
    { 
        return CUInt128Optimized(*this).operator<<=(bits); 
    }

private:
    void SetValue(const CUInt128Optimized& other) throw()
    {
        m_data.v128 = other.m_data.v128;
    }

    void SetValue(uint32_t value) throw()
    {
        m_data.u32_data[0] = value;
        m_data.u32_data[1] = 0;
        m_data.u64_data[1] = 0;
    }

    union {
        uint32_t u32_data[4];
        uint64_t u64_data[2];
        __m128i v128;  // SIMD register for optimized operations
    } m_data;
};

// Comparison operators with uint32_t as left operand
inline bool operator==(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator==(x); 
}

inline bool operator!=(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator!=(x); 
}

inline bool operator<(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator>(x); 
}

inline bool operator>(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator<(x); 
}

inline bool operator<=(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator>=(x); 
}

inline bool operator>=(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator<=(x); 
}

inline CUInt128Optimized operator+(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator+(x); 
}

inline CUInt128Optimized operator-(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return CUInt128Optimized(x).operator-(y); 
}

inline CUInt128Optimized operator^(uint32_t x, const CUInt128Optimized& y) throw() 
{ 
    return y.operator^(x); 
}

} // End namespace

#endif // __UINT128_OPTIMIZED_H__
