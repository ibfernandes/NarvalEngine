#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"
#include <glm/glm.hpp>
#define MAX_BXDF 3

namespace narvalengine {

	/**
	* Enum flag responsible for enumerating supported types of Bidirectional Distribution functions (BxDF), where x 
	* represents the distribution model.
	*/
	enum BxDFType {
		BxDF_REFLECTION = 1 << 0,
		BxDF_TRANSMISSION = 1 << 1,
		BxDF_DIFFUSE = 1 << 2,
		BxDF_GLOSSY = 1 << 3,
		BxDF_SPECULAR = 1 << 4,
		BxDF_ALL = BxDF_DIFFUSE | BxDF_GLOSSY | BxDF_SPECULAR | BxDF_REFLECTION | BxDF_TRANSMISSION
	};

	class BxDF {
	public:
		BxDFType bxdftype;

		/**
		 * Sample scattered direction given incoming direction in Spherical Coordinate System (SCS).
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param normal in Spherical Coordinate System (SCS).
		 * @param ri in Spherical Coordinate System (SCS).
		 * @return sampled direction in Spherical Coordinate System (SCS).
		 */
		virtual glm::vec3 sample(const glm::vec3 &incoming, const glm::vec3 &normal, const RayIntersection &ri) = 0;

		/**
		 * Calculates the Probability Density Function (PDF) of sampling this scattered direction in Spherical Coordinate System (SCS).
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param scattered in Spherical Coordinate System (SCS).
		 * @param ri in Spherical Coordinate System (SCS).
		 * @return pdf.
		 */
		virtual float pdf(const glm::vec3 &incoming, const glm::vec3 &scattered, const RayIntersection &ri) = 0;

		/**
		 * Evals the BSDF function fr(x, w_i, w_o) in Spherical Coordinate System (SCS).
		 * 
		 * @param incoming in Spherical Coordinate System (SCS).
		 * @param scattered in Spherical Coordinate System (SCS).
		 * @param ri in Spherical Coordinate System (SCS).
		 * @return eval.
		 */
		virtual glm::vec3 eval(const glm::vec3 &incoming, const glm::vec3 &scattered, const RayIntersection &ri) = 0;
	};

	/**
	* All BSDF functions receive Rays in World Coordinate System (WCS) and converts them to Spherical Coordinate System (SCS), where the z-axis is up, before invoking any BxDF functions.
	* It is also necessary to negate the incoming ray before converting it to SCS, as we are pointing from the camera TO the hit point.
	* All results are returned in World Coordinate System (WCS).
	*/
	class BSDF {
	public:
		BxDFType bsdfType;
		int nBxdf = 0;
		BxDF* bxdf[MAX_BXDF];

		/**
		 * Adds {@param BxDF} to this BSDF.
		 * 
		 * @param bxdfArg
		 */
		void addBxdf(BxDF* bxdfArg) {
			bxdf[nBxdf++] = bxdfArg;
			bsdfType = BxDFType(bsdfType | bxdfArg->bxdftype);
		}

		/**
		 * Tests if this BxDF contains the flag {@code testType} set.
		 * 
		 * @param testType to test.
		 * @return true if testType is set. False otherwise.
		 */
		bool hasType(BxDFType testType) {
			if (bsdfType & testType)
				return true;
			else
				return false;
		}

		/**
		 * Sample scattered direction given incoming direction and normal.
		 * 
		 * @param incoming in world coordinates space (WCS).
		 * @param normal in world coordinates space (WCS).
		 * @param ri RayIntersection where this incomingRay hit and has its origin set to.
		 * @return sampled direction in world coordinates space (WCS).
		 */
		glm::vec3 sample(const glm::vec3 &incoming, const glm::vec3 &normal, const RayIntersection &ri) {
			glm::vec3 ss, ts;
			generateOrthonormalCS(normal, ss, ts);
			glm::vec3 scattered = bxdf[0]->sample(toLCS(-glm::normalize(incoming), normal, ss, ts), normal, ri);
			scattered = toWorld(scattered, normal, ss, ts);
			return scattered;
		}

		/**
		 * Calculates the Probability Density Function (PDF) of sampling this scattered direction.
		 * 
		 * @param incoming in world coordinates space (WCS).
		 * @param scattered in world coordinates space (WCS).
		 * @param normal in world coordinates space (WCS).
		 * @param ri RayIntersection where this incomingRay hit and has its origin set to.
		 * @return pdf.
		 */
		float pdf(const glm::vec3 &incoming, const glm::vec3 &scattered, const glm::vec3 &normal, const RayIntersection &ri) {
			if (!(bsdfType & BxDF_TRANSMISSION) && (!sameHemisphere(-incoming, normal) || !sameHemisphere(scattered, normal)))
				return 0;

			glm::vec3 ss, ts;
			generateOrthonormalCS(normal, ss, ts);

			return bxdf[0]->pdf(toLCS(-glm::normalize(incoming), normal, ss, ts), toLCS(scattered, normal, ss, ts), ri);
		}

		/**
		 * Evals the BxDF function Fr(x, w_i, w_o).
		 * 
		 * @param incoming in world coordinates space (WCS).
		 * @param scattered in world coordinates space (WCS).
		 * @param ri RayIntersection where this incomingRay hit and has its origin set to.
		 * @return eval.
		 */
		glm::vec3 eval(const glm::vec3 &incoming, const glm::vec3 &scattered, const RayIntersection &ri) {
			if (!(bsdfType & BxDF_TRANSMISSION) && (!sameHemisphere(-incoming, ri.normal) || !sameHemisphere(scattered, ri.normal)))
				return glm::vec3(0);
			glm::vec3 ss, ts;
			generateOrthonormalCS(ri.normal, ss, ts);

			return bxdf[0]->eval(toLCS(-glm::normalize(incoming), ri.normal, ss, ts), toLCS(scattered, ri.normal, ss, ts), ri);
		}
	};

	class Fresnel {
	public:
		virtual glm::vec3 eval(float cosTheta) = 0;
	};

	class FresnelSchilck : public Fresnel {
	public:
		glm::vec3 f0 = glm::vec3(0.04f);

		glm::vec3 eval(float cosTheta) {
			return f0 + (1.0f - f0) * std::pow(1.0f - cosTheta, 5.0f);
		}
	};
}