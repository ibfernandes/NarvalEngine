#pragma once
#include "utils/Math.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "materials/Material.h"
#include <glog/logging.h>

namespace narvalengine {

	/**
	 *	All phase functions operate using the Spherical Coordinate System (SCS) where the z-axis is up.
	 */
	class PhaseFunction {
	public:
		/**
		 * Given the {@code incomingDir} in Spherical Coordinate System (SCS) samples a scattered direction.
		 * 
		 * @param incomingDir in Spherical Coordinate System (SCS).
		 * @return sampled scattered direction in Spherical Coordinate System (SCS).
		 */
		virtual glm::vec3 sample(glm::vec3 incomingDir) = 0;
		/**
		 * Calculates the PDF between the {@code incoming} and {@code scattered} directions.
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param scattered in Spherical Coordinate System (SCS).
		 * @return calculated PDF.
		 */
		virtual float pdf(glm::vec3 incoming, glm::vec3 scattered) = 0;
		/**
		 * Evalutes the phase function between the {@code incoming} and {@code scattered} directions.
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param scattered in Spherical Coordinate System (SCS).
		 * @return eval.
		 */
		virtual float eval(glm::vec3 incoming, glm::vec3 scattered) = 0;
	};

	/**
	 * An isotropic phase function samples uniformly over the unit sphere.
	 */
	class IsotropicPhaseFunction : public PhaseFunction {
	public:
		glm::vec3 sample(glm::vec3 incomingDir) {
			return sampleUnitSphere(random(), random());
		}

		/**
		 * The PDF of choosing uniformly over the sphere is 1.0f / Area, where the area of an unit sphere is 1 / 4 * PI * 1^2.
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param scattered in Spherical Coordinate System (SCS).
		 * @return pdf.
		 */
		float pdf(glm::vec3 incoming, glm::vec3 scattered) {
			return 1.0f / FOUR_PI;
		}

		float eval(glm::vec3 incoming, glm::vec3 scattered) {
			return 1.0f / FOUR_PI;
		}
	};

	/**
	 * Henyey-Greenstein phase function uses an asymmetry factor g to determine how forward scattering, i.e. g > 0,
	 *  or backward scattering, i.e. g < 0, the sampling behaves.
	 */
	class HG : public PhaseFunction {
	public:
		/**
		 * The asymmetry parameter g defines the behavior of the HG phase function. It must be in the interval [-1, 1].
		 * If g > 0 : Forward scattering.
		 * If g = 0 : Isotropic.
		 * If g < 0 : Backward scattering.
		 */
		float g = 0;

		/**
		 * Instantiates this HG Phase function with param {@code g}.
		 * 
		 * @param g in the interval [-1, 1].
		 */
		HG(float g = 0) {
			if (g < -1 || g > 1)
				LOG(WARNING) << "Param g must be in the interval [-1, 1].";
			this->g = g;
		}

		/**
		 * Samples a scattered direction given the incoming direction {@code incomingDir}, interpreted as the forward direction.
		 * 
		 * @param incomingDir in Spherical Coordinate System (SCS).
		 * @return sampled scattered direction.
		 */
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

			return scattered;
		}

		float pdf(glm::vec3 incoming, glm::vec3 scattered) {
			float cosTheta = glm::dot(glm::normalize(incoming), glm::normalize(scattered));
			float denom = 1.0f + g * g - 2.0f * g * cosTheta;
			assert(denom > 0);
			return INV4PI * (1.0f - g * g) / (denom * glm::sqrt(denom));
		}

		float eval(glm::vec3 incoming, glm::vec3 scattered) {
			float cosTheta = glm::dot(glm::normalize(incoming), glm::normalize(scattered));
			float denom = 1.0f + g * g - 2.0f * g * cosTheta;
			assert(denom > 0);
			return INV4PI * (1.0f - g * g) / (denom * glm::sqrt(denom));
		}
	};

	class Medium {
	public:
		/**
		 * Receives ray in World Coordinate System (WCS) and calculates the transmittance.
		 * 
		 * @param incoming in World Coordinate System (WCS).
		 * @param intersection where this incoming ray hit the object.
		 * @return 
		 */
		virtual glm::vec3 Tr(Ray incoming, RayIntersection intersection) = 0;
		/**
		 * Receives ray in World Coordinate System (WCS).
		 * 
		 * @param incoming in World Coordinate System (WCS).
		 * @param scattered returned in World Coordinate System (WCS).
		 * @param intersection where this incoming ray hit the object. Must be inside or at this volume's boundary.
		 * @return sampled transmittance up until the scattering event occured, i.e., {@code scattered.origin}.
		 */
		virtual glm::vec3 sample(Ray incoming, Ray &scattered, RayIntersection intersection) = 0;
	};
}

