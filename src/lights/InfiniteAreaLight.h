#pragma once
#include "lights/Light.h"
#include "materials/Texture.h"
#include "utils/Math.h"
#include "utils/Sampling.h"

namespace narvalengine {
	class InfiniteAreaLight : public Light{
        public:
        Texture* tex;
		Distribution2D *distribution;
        //glm::vec3 worldCenter;
        //float worldRadius;
        float scale = 1;

        InfiniteAreaLight() {

        }

		InfiniteAreaLight(Texture *tex) {
            //this->worldCenter = worldCenter;
            //this->worldRadius = worldRadius;
            this->tex = tex;

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

        glm::vec3 Le(Ray ray, glm::mat4 invTransformToWCS) {
            glm::vec3 w = glm::normalize(invTransformToWCS * glm::vec4(ray.d, 0));
            glm::vec3 normal = glm::vec3(0, 0, 1);
            glm::vec3 ss, ts;
            generateOrthonormalCS(normal, ss, ts);
            glm::vec3 incoming = toLCS(w, normal, ss, ts);

            glm::vec2 st = glm::vec2(getSphericalPhi(incoming) * INV2PI, getSphericalTheta(incoming) * INVPI);
            return tex->sample(st.x, st.y);
        }

		glm::vec3 sampleLi(RayIntersection intersec, glm::mat4 transformToWCS, Ray& wo, float& lightPdf) {
            // Find $(u,v)$ sample coordinates in infinite light texture
            float mapPdf;
            glm::vec2 u = glm::vec2(random(), random());

            glm::vec2 uv = distribution->sampleContinuous(u, &mapPdf);
            if (mapPdf == 0) 
                return glm::vec3(0);

            // Convert infinite light sample point to direction
            float theta = uv[1] * PI;
            float phi = uv[0] * 2 * PI;
            float cosTheta = std::cos(theta);
            float sinTheta = std::sin(theta);
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            wo.o = intersec.hitPoint;
            glm::vec3 polarCoord = glm::vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 ss, ts;
            generateOrthonormalCS(glm::vec3(0,1,0), ss, ts);
            wo.d = toLCS(polarCoord, glm::vec3(0,1,0), ss, ts);


            // Compute PDF for sampled infinite light direction
            lightPdf = mapPdf / (2 * PI * PI * sinTheta);
            if (sinTheta == 0) 
                lightPdf = 0;

            // Return radiance value for infinite light direction
            return tex->sample(uv.x, uv.y); //SpectrumType::Illuminant
		}

        //TODO pass the matrix by reference instead of copy
        glm::vec3 sampleLe(Ray &fromLight, glm::mat4 transformToWCS, float &lightDirPdf, float &lightPosPdf) {
            Sphere* s = (Sphere*) this->primitive;
            glm::vec3 worldCenter = transformToWCS * glm::vec4(s->getCenter(), 1.0f);

            // Compute direction for infinite light sample ray
            glm::vec2 u = glm::vec2(random(), random());

            // Find $(u,v)$ sample coordinates in infinite light texture
            float mapPdf;
            glm::vec2 uv = distribution->sampleContinuous(u, &mapPdf);
            if (mapPdf == 0) 
                return glm::vec3(0);

            float theta = uv[1] * PI;
            float phi = uv[0] * 2.f * PI;

            float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            glm::vec3 polarCoord = glm::vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
            glm::vec3 ss, ts;

            generateOrthonormalCS(glm::vec3(0, 1, 0), ss, ts);
            fromLight.d = toLCS(polarCoord, glm::vec3(0, 1, 0), ss, ts);

            // Compute origin for infinite light sample ray
            glm::vec3 v1, v2;
            generateOrthonormalCS(fromLight.d, v1, v2);
            glm::vec2 cd = sampleConcentricDisk();
            glm::vec3 pDisk = worldCenter + s->radius * (cd.x * v1 + cd.y * v2);

            fromLight.o = pDisk + s->radius * -fromLight.d;

            // Compute _InfiniteAreaLight_ ray PDFs
            lightDirPdf = sinTheta == 0 ? 0 : mapPdf / (2 * PI * PI * sinTheta);
            lightPosPdf = 1 / (PI * s->radius * s->radius);
            return tex->sample(uv.x, uv.y); //SpectrumType::Illuminant
            return le;
        }
	};
};

