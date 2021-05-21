#pragma once
#include "lights/Light.h"
#include "materials/Texture.h"
#include "utils/Math.h"
#include "utils/Sampling.h"

namespace narvalengine {
	class InfiniteAreaLight : public Light{
        Texture* tex;
		Distribution2D *distribution;

		InfiniteAreaLight(Texture *tex) {

            float filter = 1.0f / glm::max(tex->width, tex->height);
			float *img = new float[tex->width * tex->height];

            for (int v = 0; v < tex->height; v++) {
                float vp = (float)v / (float)tex->height;
                float sinTheta = glm::sin(PI * float(v + 0.5f) / float(tex->height));
                
				for (int u = 0; u < tex->width; u++) {
                    float up = (float)u / (float)tex->width;
                    img[u + v * tex->width] = tex->sample(up, vp).y; //TODO should use trilinear sampling with mipmap or something
                    img[u + v * tex->width] *= sinTheta;
                }
            }

			distribution = new Distribution2D(img, tex->width, tex->height);
		}

		glm::vec3 Li() {
			return li;
		}

		void sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
            // Find $(u,v)$ sample coordinates in infinite light texture
            /*float mapPdf;
            glm::vec2 u = glm::vec2(random(), random());

            glm::vec2 uv = distribution->sampleContinuous(u, &mapPdf);
           // if (mapPdf == 0) return glm::vec3(0);

            // Convert infinite light sample point to direction
            float theta = uv[1] * PI;
            float phi = uv[0] * 2 * PI;
            float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
            float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
            *wi =
                LightToWorld(Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));

            // Compute PDF for sampled infinite light direction
            *pdf = mapPdf / (2 * Pi * Pi * sinTheta);
            if (sinTheta == 0) *pdf = 0;

            // Return radiance value for infinite light direction
            *vis = VisibilityTester(ref, Interaction(ref.p + *wi * (2 * worldRadius),
                ref.time, mediumInterface));
            return Spectrum(Lmap->Lookup(uv), SpectrumType::Illuminant);*/
		}

        void sampleLe() {
           /* ProfilePhase _(Prof::LightSample);
            // Compute direction for infinite light sample ray
            Point2f u = u1;

            // Find $(u,v)$ sample coordinates in infinite light texture
            Float mapPdf;
            Point2f uv = distribution->SampleContinuous(u, &mapPdf);
            if (mapPdf == 0) return Spectrum(0.f);
            Float theta = uv[1] * Pi, phi = uv[0] * 2.f * Pi;
            Float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
            Float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
            Vector3f d =
                -LightToWorld(Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));
            *nLight = (Normal3f)d;

            // Compute origin for infinite light sample ray
            Vector3f v1, v2;
            CoordinateSystem(-d, &v1, &v2);
            Point2f cd = ConcentricSampleDisk(u2);
            Point3f pDisk = worldCenter + worldRadius * (cd.x * v1 + cd.y * v2);
            *ray = Ray(pDisk + worldRadius * -d, d, Infinity, time);

            // Compute _InfiniteAreaLight_ ray PDFs
            *pdfDir = sinTheta == 0 ? 0 : mapPdf / (2 * Pi * Pi * sinTheta);
            *pdfPos = 1 / (Pi * worldRadius * worldRadius);
            return Spectrum(Lmap->Lookup(uv), SpectrumType::Illuminant);*/
        }
	};
};

