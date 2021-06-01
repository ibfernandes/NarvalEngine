#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "materials/Material.h"

namespace narvalengine {

	class PhaseFunction {
	public:
		/*
			Returns scattered direction
		*/
		virtual glm::vec3 sample(glm::vec3 incomingDir) = 0;
		virtual float pdf(glm::vec3 incoming, glm::vec3 scattered) = 0;
		/*
			Evals the phase function
		*/
		virtual float eval(glm::vec3 incoming, glm::vec3 scattered) = 0;
	};

	class IsotropicPhaseFunction : public PhaseFunction {
	public:
		glm::vec3 sample(glm::vec3 incomingDir) {
			return sampleUnitSphere(random(), random());
		}

		float pdf(glm::vec3 incoming, glm::vec3 scattered) {
			return 1.0f / (4.0f * PI);
		}

		float eval(glm::vec3 incoming, glm::vec3 scattered) {
			return 1.0f / (4.0f * PI);
		}
	};

	//Henyey-Greenstein Phase Function
	class HG : public PhaseFunction {
	public:
		float g = 0;
		/*
			TODO: for debug purposes only
		*/
		float theta = 0;
		float phi = 0;

		HG(float g = 0) {
			this->g = g;
		}

		glm::vec3 sample(glm::vec3 incomingDir) {
			float cosTheta;
			glm::vec2 u = glm::vec2(random(), random());

			if (std::abs(g) < 1e-3f)
				cosTheta = 1.0f - 2.0f * u[0];
			else {
				float sqr = (1.0f - g * g) / (1.0f + g - 2.0f * g * u[0]);
				sqr = sqr * sqr;
				cosTheta = -1.0f / (2.0f * g) * (1.0f + g * g - sqr);
			}

			// Compute direction _wi_ for Henyey--Greenstein sample
			float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);
			float phi = 2.0f * PI * u[1];
			glm::vec3 scattered = sphericalToCartesianPre(sinTheta, cosTheta, phi);

			//TODO: for debug only
			theta = glm::acos(cosTheta);
			this->phi = phi;

			return scattered;
		}

		float pdf(glm::vec3 incoming, glm::vec3 scattered) {
			float cosTheta = glm::dot(glm::normalize(incoming), glm::normalize(scattered));
			float denom = 1.0f + g * g + 2.0f * g * cosTheta;
			return INV4PI * (1.0f - g * g) / (denom * glm::sqrt(denom));
		}

		float eval(glm::vec3 incoming, glm::vec3 scattered) {
			float cosTheta = glm::dot(glm::normalize(incoming), glm::normalize(scattered));
			float denom = 1.0f + g * g + 2.0f * g * cosTheta;
			return INV4PI * (1.0f - g * g) / (denom * glm::sqrt(denom));
		}
	};

	class Medium {
	public:
		virtual glm::vec3 Tr(Ray incoming, RayIntersection ri) = 0;
		virtual glm::vec3 Tr(float distance) = 0;
		virtual glm::vec3 sample(Ray incoming, Ray &scattered, RayIntersection intersection) = 0;
	};
}

