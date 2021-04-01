//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <string>

namespace narvalengine {

    class MurmurHash3 {
    public:
        void init(uint32_t seed);

        uint32_t hash(std::string str);
        uint32_t hash(const void* valueMem, int len);

    private:
        uint32_t seed = 0;

        uint32_t rotl(uint32_t value, uint32_t shift);
        uint32_t getblock32(const uint32_t* p, int i);
        uint32_t fmix32(uint32_t h);
    };
}