#include "utils/StringID.h"

namespace narvalengine {
	StringID genStringID(const char* str, const int len) {
		StringID id = gStrIDMurmurHasher.hash(str, len);

		//We only map the hash to a string in debug mode.
		#ifdef NE_DEBUG_MODE
			if (gStringIDTable.count(id) != 0)
				return id;

			char* ss = new char[len + 1];
			std::memcpy(ss, str, (len + 1) * sizeof(char));
			gStringIDTable.insert({ id, ss });
		#endif

		return id;
	}

	StringID genStringID(const char* str) {
		return genStringID(str, strlen(str));
	}
}