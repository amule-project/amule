
//
// This file is part of the aMule Project.
//
// Selector header for UInt128 implementation
// Allows switching between original and optimized implementations
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//

#ifndef __UINT128_SELECTOR_H__
#define __UINT128_SELECTOR_H__

#include "config.h"

// Check if optimized implementation is available and enabled
#if defined(USE_OPTIMIZED_UINT128) && USE_OPTIMIZED_UINT128
    #include "UInt128Optimized.h"
    namespace Kademlia {
        using CUInt128 = CUInt128Optimized;
    }
#else
    #include "UInt128.h"
#endif

#endif // __UINT128_SELECTOR_H__
