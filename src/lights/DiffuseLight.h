#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DiffuseLight : public Light {
	public:
		glm::vec3 li;

		glm::vec3 Li() {
			return li;
		}
	};
}