#pragma once
#include <glm/glm.hpp>
#include "integrators/Integrator.h"
#include "lights/Light.h"
#include "primitives/Ray.h"
#include "primitives/Primitive.h"
#include "utils/Math.h"
#include "core/BSDF.h"
#include "materials/Medium.h"

namespace narvalengine {
	class VolumetricPathIntegrator : public Integrator {
	public:

		/*
			If this ray is occluded, returns (0,0,0).
			If it intersects a participating media, returns the Transmittance of light's source radiance.
		*/
		glm::vec3 visibility(Ray scatteredToLight, Scene *s, RayIntersection currentIntersection) {
			glm::vec3 Tr(1);


			bool insideVol = false;
			RayIntersection intersectionInsideVol;

			//if current intersection happened within a volume
			if (currentIntersection.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
				bool intersecScene = s->intersectScene(scatteredToLight, intersectionInsideVol, -0.001f, INFINITY); //TODO should test only inside THIS currentIntersection primitive

				if (intersecScene && intersectionInsideVol.primitive->material->bsdf != nullptr ) {
					insideVol = true;
					float pathLength = intersectionInsideVol.tFar - intersectionInsideVol.tNear;
					//Tr = currentIntersection.primitive->material->medium->Tr(pathLength);
					Tr = currentIntersection.primitive->material->medium->Tr(scatteredToLight, intersectionInsideVol);
					scatteredToLight.o = scatteredToLight.getPointAt(intersectionInsideVol.tFar + 0.001f);

				//if it hits light directly, return full transmission
				} else if (intersecScene && intersectionInsideVol.primitive->material->light != nullptr) {
					return glm::vec3(1, 1, 1);
				}

			}

			RayIntersection ri;
			bool didIntersect = s->intersectScene(scatteredToLight, ri, 0.001f, INFINITY);
			if (didIntersect && !insideVol && ri.primitive->material->light == nullptr && (ri.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION)) {
				float pathLength = ri.tFar - ri.tNear;

				//Tr = ri.primitive->material->medium->Tr(pathLength);
				Tr = ri.primitive->material->medium->Tr(scatteredToLight, ri);
				return Tr;
			}

			//scatteredToLight is not pointing towards light, why?!
			if (!didIntersect && insideVol)
				return glm::vec3(0, 1, 0);

			if (didIntersect && ri.primitive->material->light != nullptr)
				return Tr;
			else
				return Tr * 0.0f;
		}

		glm::vec3 estimateDirectLightning(Ray incoming, RayIntersection intersec, Scene *scene, Light *light, InstancedModel *im) {
			glm::vec3 radianceL(0.f);
			Ray scattered;
			float lightPdf = 0;
			float scatteringPdf = 0;

			//Sample scattered light direction and calculate BRDF with respect to it
			light->sampleLi(intersec, im->transformToWCS, scattered, lightPdf);
			glm::vec3 vis = visibility(scattered, scene, intersec);
			//scattered.d = glm::normalize(scattered.d);

			glm::vec3 Li = light->Li();
			if (lightPdf > 0 && !isBlack(Li)) {
				glm::vec3 fr;
				if (intersec.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
					//phase function is coupled with the bsdf
					fr = intersec.primitive->material->bsdf->eval(incoming.d, scattered.d, intersec);
					scatteringPdf = intersec.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersec.normal);
				} else {
					//Eval BRDF with respect to light
					fr = intersec.primitive->material->bsdf->eval(incoming.d, scattered.d, intersec) * absDot(scattered.direction, intersec.normal);
					scatteringPdf = intersec.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersec.normal);
				}

				if (!isBlack(fr)) {
					//Calculate visibility term ( takes occlusion and participating media into account)
					Li *= vis;

					//Add light randiance contribution
					if (!isBlack(Li) && lightPdf > 0 && scatteringPdf > 0) {
						//if (IsDeltaLight(light.flags)) Lights that cant have sampled direction (point, directional light) Infinite area too??
						//	Ld += f * Li / lightPdf;
						//else 
						float weight = powerHeuristic(lightPdf, scatteringPdf);
						radianceL += fr * Li * weight / lightPdf;
					}
				}
			}

			return radianceL;
		}

		glm::vec3 uniformSampleOneLight(Ray incoming, RayIntersection intersec, Scene *s) {
			int i = (s->lights.size()) * random();

			Primitive *lightPrimitive = s->lights.at(i)->model->getRandomLightPrimitive();

			//Either there's no light to sample or we chose a infinite area light
			if (lightPrimitive == nullptr)
				return glm::vec3(0);

			Light *light = s->lights.at(i)->model->getRandomLightPrimitive()->material->light;
			return estimateDirectLightning(incoming, intersec, s, light, s->lights.at(i)); //TODO / lightPdf ( 1/ Nlights)
		}

		void testIntersectionInsideMedia(Scene *s) {
			Ray ray;
			ray.o = glm::vec3(0.4, 0, 0.6);
			ray.d = glm::vec3(0, 0, 1);

			RayIntersection rayIntersection;

			s->intersectScene(ray, rayIntersection, -0.001f, INFINITY);

			while (true)
				continue;
		}

		glm::vec3 Li(Ray incoming, Scene *scene) {
			glm::vec3 L(0);
			glm::vec3 transmittance(1);
			RayIntersection intersection;
			//testIntersectionInsideMedia(scene);

			for (int b = 0; b < scene->settings.bounces; b++) {

				bool didIntersect = scene->intersectScene(incoming, intersection, 0.001f, 9999);

				if (b == 0 /*|| specularBounce*/) {
					//If ray intersected a primitive and it is emissive, get its Light Emission Radiance (Le).
					if (didIntersect && (intersection.primitive->material->light != nullptr))
						L += transmittance * intersection.primitive->material->light->Li();
					else
						//For each scene's light, get its Le (which is different from 0 only for infinite area lights)
						for (InstancedModel *light : scene->lights) {
							//TODO: watch for nulls? materials will be restricted to one light only?
							for (Material *m : light->model->materials) {
								Light *l = m->light;
								L += transmittance * l->Le(incoming);
							}
						}
				}

				//TODO:  i did some shortcuts in here
				if (!didIntersect || !intersection.primitive->material->bsdf)
					break;

				if (intersection.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
					float phasePdf;
					Ray scattered;

					//Move ray origin to volume's boundary
					incoming.o = incoming.getPointAt(intersection.tNear);
					glm::vec3 tr = intersection.primitive->material->medium->sample(incoming, scattered, intersection);
					
					if (allEqualTo(tr, -1.0f)) {
						incoming = scattered;
						b--;
						continue;
					}

					glm::vec3 phaseFr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);

					phasePdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);

					transmittance *=  tr * phaseFr / phasePdf;
					if (isBlack(phaseFr) || phasePdf == 0.f || isBlack(tr))
						break;

					glm::vec3 lightSample = uniformSampleOneLight(scattered, intersection, scene);
					//lightSample = lightSample - lightSample * tr;

					L += transmittance * lightSample;

					//transmittance *= tr * phaseFr / phasePdf;
					incoming = scattered;
				} else {
					//Sample BSDF for scattered direction
					float bsdfPdf;
					Ray scattered;

					L += transmittance * uniformSampleOneLight(incoming, intersection, scene);
					L += 0.2f; //TODO remove, DEBUG

					scattered.o = intersection.hitPoint; //TODO i stoped here, scattered direction was the same as incoming!
					scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, intersection.normal);
					bsdfPdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);
					if (bsdfPdf < 0)
						break;
					glm::vec3 fr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);

					if (isBlack(fr) || bsdfPdf == 0.f)
						break;
					transmittance *= fr * absDot(incoming.direction, intersection.normal) / bsdfPdf;
					//specularBounce = (flags & BSDF_SPECULAR) != 0;

					incoming = scattered;
				}

				//L += transmittance * uniformSampleOneLight(incoming, intersection, scene);

				//<< Account for subsurface scattering, if applicable >>
				/*if (isect.bssrdf && (flags & BSDF_TRANSMISSION)) {
					<< Importance sample the BSSRDF >>
						SurfaceInteraction pi;
					Spectrum S = isect.bssrdf->Sample_S(scene, sampler.Get1D(),
						sampler.Get2D(), arena, &pi, &pdf);
					if (S.IsBlack() || pdf == 0)
						break;
					beta *= S / pdf;

					<< Account for the direct subsurface scattering component >>
						L += beta * UniformSampleOneLight(pi, scene, arena, sampler);

					<< Account for the indirect subsurface scattering component >>
						Spectrum f = pi.bsdf->Sample_f(pi.wo, &wi, sampler.Get2D(), &pdf,
							BSDF_ALL, &flags);
					if (f.IsBlack() || pdf == 0)
						break;
					beta *= f * AbsDot(wi, pi.shading.n) / pdf;
					specularBounce = (flags & BSDF_SPECULAR) != 0;
					ray = pi.SpawnRay(wi);

				}*/

				//Decide wether or not to terminate path using Russian Roulette
				if (b > 3) {
					float p = glm::max(0.05f, 1 - transmittance.y);
					if (random() < p)
						break;
					transmittance /= 1 - p;
				}
			}

			return L;
		}
	};
}