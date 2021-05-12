#pragma once
#include "lights/Light.h"

namespace narvalengine {
	class DiffuseLight : public Light {
	public:
		glm::vec3 Li() {
			return li;
		}

		void sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
			wo.o = intersec.hitPoint;

			glm::vec3 pointOnLightPrimitive = primitive->samplePointOnSurface(intersec, transformToWCS); //sample point and pdf needs intersec, as its sample is "view" dependent
			//pointOnLightPrimitive = transformToWCS * glm::vec4(pointOnLightPrimitive, 1.0f);

			glm::vec3 d = pointOnLightPrimitive - wo.o;
			float distanceSquared = glm::length(d);

			wo.d = glm::normalize(d);

			//if (!sameHemisphere(wo.d, intersec.normal)) //TODO does not work for volumetrics
			///	lightPdf = 0;
			//else
				//lightPdf = (distanceSquared / absDot(intersec.normal, wo.d) )* primitive->pdf(intersec, transformToWCS);

			lightPdf = primitive->pdf(intersec, transformToWCS);
		}
	};
}