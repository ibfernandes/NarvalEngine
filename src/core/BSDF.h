#pragma once
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"
#include <glm/glm.hpp>
#define MAX_BXDF 3

namespace narvalengine {
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
		/*
			Sample scattered direction given incoming direction in SCS
		*/
		virtual glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) = 0;
		/*
			Calculates the Probability Density Function(PDF) of sampling this scattered direction in SCS
		*/
		virtual float pdf(glm::vec3 incoming, glm::vec3 scattered) = 0;
		/*
			Evals BSDF function Fr(x, w_i, w_o) in SCS
		*/
		virtual glm::vec3 eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri) = 0;
	};

	/*
		All BSDF functions receive Rays in WCS and converts them to SCS before invoking BxDF functions. All results are returned in WCS.
	*/
	class BSDF {
	public:
		BxDFType bsdfType;
		int nBxdf = 0;
		BxDF *bxdf[MAX_BXDF];

		void addBxdf(BxDF *bxdfArg) {
			bxdf[nBxdf++] = bxdfArg;
			bsdfType = BxDFType(bsdfType | bxdfArg->bxdftype);
		}

		bool isType(BxDFType testType) {
			if (bsdfType & testType)
				return true;
			else
				return false;
		}

		/*
			Sample scattered direction given incoming direction and normal
		*/
		glm::vec3 sample(glm::vec3 incoming, glm::vec3 normal) {
			//TODO: handle multiple BxDF
			//printVec3(incoming, "incoming world: ");
			glm::vec3 ss, ts;
			generateOrthonormalCS(normal, ss, ts);
			incoming = toLCS(incoming, normal, ss, ts);
			glm::vec3 scattered = bxdf[0]->sample(incoming, normal);

			//printVec3(scattered, "scattered: ");
			scattered = toWorld(scattered, normal, ss, ts);
			//printVec3(scattered, "scattered world: ");

			//printVec3(normal, "normal: ");
			//printVec3(ss, "ss: ");
			//printVec3(ts, "ts: ");
			//printVec3(incoming, "incoming: ");
			return scattered;
		}

		/*
			Calculates the Probability Density Function(PDF) of sampling this scattered direction
		*/
		float pdf(glm::vec3 incoming, glm::vec3 scattered, glm::vec3 normal) {
			if (!(bsdfType & BxDF_TRANSMISSION) && (!sameHemisphere(-incoming, normal) || !sameHemisphere(scattered, normal)))
				return 0;

			//TODO: handle multiple BxDF
			glm::vec3 ss, ts;
			generateOrthonormalCS(normal, ss, ts);
			incoming = toLCS(incoming, normal, ss, ts);
			scattered = toLCS(scattered, normal, ss, ts);

			return bxdf[0]->pdf(incoming, scattered);
		}

		/*
			Evals BSDF function Fr(x, w_i, w_o)
		*/
		glm::vec3 eval(glm::vec3 incoming, glm::vec3 scattered, RayIntersection ri) {
			if (!(bsdfType & BxDF_TRANSMISSION) && (!sameHemisphere(-incoming, ri.normal) || !sameHemisphere(scattered, ri.normal)))
				return glm::vec3(0);
			//TODO: handle multiple BxDF
			glm::vec3 ss, ts;
			//glm::vec3 normal = glm::normalize(-incoming + scattered);
			generateOrthonormalCS(ri.normal, ss, ts);
			incoming = toLCS(incoming, ri.normal, ss, ts);
			scattered = toLCS(scattered, ri.normal, ss, ts);

			return bxdf[0]->eval(incoming, scattered, ri);
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