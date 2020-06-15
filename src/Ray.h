#pragma once
#include <glm/glm.hpp>

class Ray {
public:
	union { glm::vec3 o, origin;  };
	union { glm::vec3 d, direction;  };

	Ray() {};

	glm::vec3 getPointAt(float t) {
		return o + t * d;
	}

	Ray(glm::vec3 origin, glm::vec3 direction){
		this->o = origin;
		this->d = direction;
	}
};