#include "utils/StringID.h"

namespace narvalengine {
	StringID genStringID(const char* str, int len) {
		StringID id = gStrIDMurmurHasher.hash(str, len);
		if (gStringIDTable.count(id) != 0)
			return id;

		char *ss = new char[len + 1];
		std::memcpy(ss, str, (len + 1)* sizeof(char));

		gStringIDTable.insert({ id, ss });

		return id;
	}

	StringID genStringID(std::string str) {
		return genStringID(str.c_str(), str.length());
	}
}