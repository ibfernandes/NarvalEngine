#include "utils/MurmurHash3.h"

namespace narvalengine {
    void MurmurHash3::init(uint32_t seed) {
        this->seed = seed;
    }

    uint32_t MurmurHash3::hash(const void* valueMem, int len) {
        const uint8_t* data = (const uint8_t*)valueMem;
        const int nblocks = len / 4;

        uint32_t h1 = seed;

        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;

        //----------
        // body

        const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

        for (int i = -nblocks; i; i++)
        {
            uint32_t k1 = getblock32(blocks, i);

            k1 *= c1;
            k1 = rotl(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1 = rotl(h1, 13);
            h1 = h1 * 5 + 0xe6546b64;
        }

        //----------
        // tail

        const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

        uint32_t k1 = 0;

        switch (len & 3)
        {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = rotl(k1, 15); k1 *= c2; h1 ^= k1;
        };

        //----------
        // finalization

        h1 ^= len;

        h1 = fmix32(h1);

        return h1;
    }

    uint32_t MurmurHash3::hash(std::string str) {
        return hash(str.c_str(), str.length());
    }

    uint32_t MurmurHash3::rotl(uint32_t value, uint32_t shift) {
        uint32_t tmp;
        tmp = value;
        value = value << shift;
        tmp = tmp >> ((sizeof(tmp) * CHAR_BIT) - shift);
        value = value | tmp;
        return(value);
    }

    uint32_t MurmurHash3::getblock32(const uint32_t* p, int i) {
        return p[i];
    }

    uint32_t MurmurHash3::fmix32(uint32_t h) {
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;

        return h;
    }
}