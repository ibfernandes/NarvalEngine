#pragma once
#include <stdint.h> 
#include <cstring>
#include "utils/MurmurHash3.h"
#include <map>
#include <unordered_map>
#include "defines.h"
#define NE_INVALID_STRING_ID uint32_t(4294967295)

namespace narvalengine {
	typedef uint32_t StringID;
	inline MurmurHash3 gStrIDMurmurHasher;
	inline std::unordered_map <StringID, const char*> gStringIDTable = {};

	StringID genStringID(const char* str, int len);
	StringID genStringID(const char* str);
}