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
		bool debugText = false;
		bool shouldComputePath = false;
		std::vector<PointPathDebugInfo> path;
		std::vector<std::vector<PointPathDebugInfo>> visibilityPath;

		/*
			If this ray is occluded, returns (0,0,0).
			If it intersects a participating media, returns the Transmittance of light's source radiance.
		*/
		glm::vec3 visibility(Ray scatteredToLight, Scene *s, RayIntersection currentIntersection, Light *light) {
			glm::vec3 Tr(1);

			bool insideVol = false;
			RayIntersection intersectionInsideVol;

			//if current intersection happened within a volume
			if (currentIntersection.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
				bool intersecScene = s->intersectScene(scatteredToLight, intersectionInsideVol, -0.001f, INFINITY); //TODO should test only inside THIS currentIntersection primitive

				if (intersecScene && intersectionInsideVol.primitive->material->bsdf != nullptr ) {
					insideVol = true;
					//float pathLength = intersectionInsideVol.tFar - intersectionInsideVol.tNear;
					//Tr = currentIntersection.primitive->material->medium->Tr(pathLength);
					Ray scatteredToLightOCS = transformRay(scatteredToLight, currentIntersection.instancedModel->invTransformToWCS);
					Tr = currentIntersection.primitive->material->medium->Tr(scatteredToLightOCS, intersectionInsideVol);
					if(debugText)
						printVec3(Tr, "Vis Tr: ");
					scatteredToLight.o = scatteredToLight.getPointAt(intersectionInsideVol.tFar + 0.001f);

				//if it hits light directly, return full transmission
				} else if (intersecScene && intersectionInsideVol.primitive->material->light != nullptr) {
					return glm::vec3(0, 0, 0); //Basically when exactly at the border
				}
			}
			//testing
			//return Tr;

			RayIntersection ri;
			bool didIntersect = s->intersectScene(scatteredToLight, ri, 0.001f, INFINITY);
			//if it hits a volume from a ray origin that is outside it
			if (didIntersect && !insideVol && ri.primitive->material->light == nullptr && (ri.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION)) {
				//float pathLength = ri.tFar - ri.tNear;

				//Tr = ri.primitive->material->medium->Tr(pathLength);
				Ray scatteredToLightOCS = transformRay(scatteredToLight, ri.instancedModel->invTransformToWCS);
				Tr = ri.primitive->material->medium->Tr(scatteredToLightOCS, ri);

				return Tr;
			}

			//if scatteredToLight is not pointing towards light, return a debug red only transmission.
			if (!didIntersect && insideVol && isBlack(light->Le()))
				return glm::vec3(1, 0, 0); // doesnt seem to reach this point

			if (didIntersect && ri.primitive->material->light != nullptr)
				return Tr;
			//If it is an inifite area light and the ray did not intersect any object along the way
			else if (!isBlack(light->Le()) && !didIntersect) {
				return Tr;
			}else
				return Tr * 0.0f;
		}

		bool intersectTr(Ray ray, RayIntersection *ri, glm::vec3 *Tr, Scene* s) {
			*Tr = glm::vec3(1.0f);

			while (true) {
				RayIntersection intersec;
				bool hitSurface = s->intersectScene(ray, intersec, 0.001, 9999);
				*ri = intersec;

				/*std::cout << "---------------------" << std::endl;
				std::cout << "intersectTr " << std::endl;
				printVec3(ray.o, "ray.origin ");
				printVec3(ray.d, "ray.dir ");
				std::cout << "hitScene " << hitSurface << std::endl;
				if (hitSurface) {
					std::cout << "tNear " << intersec.tNear << std::endl;
					std::cout << "tFar " << intersec.tFar << std::endl;
					std::cout << "isVolumetric " << ((intersec.primitive->material->medium == nullptr) ? "no" : "yes") << std::endl;
					std::cout << "isLight " << ((intersec.primitive->material->light == nullptr) ? "no" : "yes") << std::endl;
					printVec3(intersec.hitPoint, "hitPoint ");
				}*/

				// Accumulate beam transmittance for ray segment
				Medium* m = nullptr;
				if(hitSurface)
					m = ri->primitive->material->medium;
				if (m != nullptr) {
					Ray scatteredToLightOCS = transformRay(ray, intersec.instancedModel->invTransformToWCS);
					*Tr *= m->Tr(scatteredToLightOCS, intersec);
				}

				// Initialize next ray segment or terminate transmittance computation
				if (!hitSurface) return false;
				if (ri->primitive->material->medium != nullptr) return true;
				/*If nullptr is returned, ray intersections with the primitive should be ignored;
				the primitive only serves to delineate a volume of space for participating media.
				This method is also used to check if two rays have intersected the same object by comparing their Material pointers. */

				ray.o = intersec.hitPoint;
				ray.d = ray.d;
			}
			//std::cout << "reached end of intersectTr" << std::endl;
		}

		//p0 Intersection Point and p1 point on Light
		glm::vec3 visibilityTr(glm::vec3 intersecPoint, glm::vec3 lightPoint, Scene *scene){
			Ray ray;
			ray.o = intersecPoint;
			ray.d = lightPoint - intersecPoint;

			glm::vec3 Tr(1.f);
			while (true) {
				RayIntersection intersec;
				bool hitSurface = scene->intersectScene(ray, intersec, 0.001, 9999);

				/*std::cout << "---------------------" << std::endl;
				std::cout << "VisibilityTr " << std::endl;
				std::cout << "hitScene " << hitSurface << std::endl;
				if (hitSurface) {
					std::cout << "tNear " << intersec.tNear << std::endl;
					std::cout << "tFar " << intersec.tFar << std::endl;
					std::cout << "isVolumetric " << ((intersec.primitive->material->medium == nullptr) ? "no" : "yes") << std::endl;
					std::cout << "isLight " << ((intersec.primitive->material->light == nullptr) ? "no" : "yes") << std::endl;
					printVec3(intersec.hitPoint, "hitPoint ");
				}*/

				if (hitSurface && intersec.primitive->material->light != nullptr)
					return Tr;

				// Handle opaque surface along ray's path
				//if (hitSurface && isect.primitive->GetMaterial() != nullptr)
				//	return Spectrum(0.0f);
				if (hitSurface && intersec.primitive->material->medium == nullptr) {
					//std::cout << "visTr returned 0" << std::endl;
					return glm::vec3(0.0f);
				}
				/*If nullptr is returned, ray intersections with the primitive should be ignored;
				the primitive only serves to delineate a volume of space for participating media.
				This method is also used to check if two rays have intersected the same object by comparing their Material pointers. */

				//moved up
				// Generate next ray segment or return final transmittance
				if (!hitSurface) 
					break;

				// Update transmittance for current ray segment
				Medium* m = intersec.primitive->material->medium;
				if (m != nullptr) {
					Ray scatteredToLightOCS = transformRay(ray, intersec.instancedModel->invTransformToWCS);
					Tr *= m->Tr(scatteredToLightOCS, intersec);
				}

				// Generate next ray segment or return final transmittance
				//if (!hitSurface) break;

				//ray.o = intersec.hitPoint;
				ray.o = ray.getPointAt(intersec.tFar + 0.001);
				//ray.d = lightPoint - ray.o;
			}
			//std::cout << "reached end of visibilityTr" << std::endl;
			return Tr;
		}

		glm::vec3 estimateDirect(Ray incoming, RayIntersection intersecOnASurface, Light *light, InstancedModel* lightIm, Scene *scene) {
			glm::vec3 Ld(0.f);

			// Sample light source with multiple importance sampling
			Ray scatteredWo;
			float lightPdf = 0, scatteringPdf = 0;
			//VisibilityTester visibility;
			glm::vec3 Li = light->Li();

			//scatteredWo points from the surface to the light source
			light->sampleLi(intersecOnASurface, lightIm->transformToWCS, scatteredWo, lightPdf);

			bool isSurfaceInteraction = false;
			if (intersecOnASurface.primitive->material->medium == nullptr)
				isSurfaceInteraction = true;
			
			if (lightPdf > 0 && !isBlack(Li)) {
				// Compute BSDF or phase function's value for light sample
				glm::vec3 f;

				/* //moved up outside the if
				bool isSurfaceInteraction = false;
				if (ri.primitive->material->medium == nullptr)
					isSurfaceInteraction = true;*/

				if (isSurfaceInteraction) {
					// Evaluate BSDF for light sampling strategy
					f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface) * glm::abs(glm::dot(scatteredWo.d, intersecOnASurface.normal));
					scatteringPdf = intersecOnASurface.primitive->material->bsdf->pdf(incoming.d, scatteredWo.d, intersecOnASurface.normal);
				}else {
					// Evaluate phase function for light sampling strategy
					f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
					scatteringPdf = f.x;
				}

				if (!isBlack(f)) {
					//handleMedia removed, we will always handle it.
					//TODO not quite correct hm, transform should be done inside samplePointOnSurface
					glm::vec3 pointOnLight = light->primitive->samplePointOnSurface(intersecOnASurface, lightIm->transformToWCS);
					Li *= visibilityTr(intersecOnASurface.hitPoint, pointOnLight, scene);

					// Add light's contribution to reflected radiance
					if (!isBlack(Li)) {
						if (false /*IsDeltaLight(light.flags)*/)
							Ld += f * Li / lightPdf;
						else {
							float weight = powerHeuristic(lightPdf, scatteringPdf);
							Ld += f * Li * weight / lightPdf;
						}
					}
				}
			}

			// Sample BSDF with multiple importance sampling
			if (true /*!IsDeltaLight(light.flags)*/) {
				glm::vec3 f;
				if (isSurfaceInteraction) {
					// Sample scattered direction for surface interactions
					scatteredWo.d = intersecOnASurface.primitive->material->bsdf->sample(incoming.d, intersecOnASurface.normal);
					f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
					f *= glm::abs(glm::dot(scatteredWo.d, intersecOnASurface.normal));
					scatteringPdf = intersecOnASurface.primitive->material->bsdf->pdf(incoming.d, scatteredWo.d, intersecOnASurface.normal);
				}else {
					// Sample scattered direction for medium interactions
					f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
					scatteredWo.d = intersecOnASurface.primitive->material->bsdf->sample(incoming.d, intersecOnASurface.normal);
					scatteringPdf = f.x;
				}
				
				if (!isBlack(f) && scatteringPdf > 0) {
					// Account for light contributions along sampled direction _wi_
					float weight = 1;
					if (true /*!sampledSpecular*/) {
						lightPdf = light->primitive->pdf(intersecOnASurface, lightIm->transformToWCS);
						if (lightPdf == 0) return Ld;
						weight = powerHeuristic(scatteringPdf, lightPdf);
					}

					// Find intersection and compute transmittance
					RayIntersection lightIntersect;
					Ray ray;
					ray.o = intersecOnASurface.hitPoint;
					ray.d = scatteredWo.d;

					glm::vec3 Tr(1.0f);
					//always handle media
					bool foundSurfaceInteraction = intersectTr(ray, &lightIntersect, &Tr, scene);

					// Add light contribution from material sampling
					glm::vec3 Li(0.f);
					if (foundSurfaceInteraction) {
						if (lightIntersect.primitive->material->light == light)
							//should be      Li = lightIsect.Le(-wi);
							Li = light->Li(); //changed from Le to Li
					}else
						Li = light->Li(); //changed from Le to Li

					if (!isBlack(Li)) 
						Ld += f * Li * Tr * weight / scatteringPdf;
				}
			}
			return Ld;
		}

		glm::vec3 estimateDirectLightning(Ray incoming, RayIntersection intersec, Scene *scene, Light *light, InstancedModel *lightIm) {
			glm::vec3 radianceL(0.f);
			Ray scattered;
			float lightPdf = 0;
			float scatteringPdf = 0;

			//Sample scattered light direction and calculate BRDF with respect to it
			light->sampleLi(intersec, lightIm->transformToWCS, scattered, lightPdf);
			//glm::vec3 vis = visibility(scattered, scene, intersec, light);
			//printVec3(vis, "Visibility Tr:");
			//scattered.d = glm::normalize(scattered.d);

			glm::vec3 Li = light->Li();
			if (lightPdf > 0 && !isBlack(Li)) {
				glm::vec3 fr;
				if (intersec.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
					//phase function is coupled with the bsdf
					fr = intersec.primitive->material->bsdf->eval(incoming.d, scattered.d, intersec);
					//scatteringPdf = intersec.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersec.normal);
					scatteringPdf = fr.x;
				} else {
					//Eval BRDF with respect to light
					fr = intersec.primitive->material->bsdf->eval(incoming.d, scattered.d, intersec) * absDot(scattered.direction, intersec.normal);
					scatteringPdf = intersec.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersec.normal);
				}

				if (!isBlack(fr)) {
					//Calculate visibility term ( takes occlusion and participating media into account)
					Li *= visibility(scattered, scene, intersec, light);

					//Add light randiance contribution
					if (!isBlack(Li) && scatteringPdf > 0) {
						if (!isBlack(light->Le())) { //Lights that cant have sampled direction (point, directional light) Infinite area too??
							radianceL += fr * Li / lightPdf;
						}else {
							float weight = powerHeuristic(lightPdf, scatteringPdf);
							radianceL += fr * Li * weight / lightPdf;
						}
					}
				}
			}

			//printVec3(radianceL, "Radiance L:");
			return radianceL;
		}

		glm::vec3 uniformSampleOneLight(Ray incoming, RayIntersection intersecOnASurface, Scene *s) {
			float r = random();
			int i = (s->lights.size()) * r;

			float lightPdf = 1.0f / s->lights.size();

			Primitive *lightPrimitive = s->lights.at(i)->model->getRandomLightPrimitive();

			//TODO: is that correct?
			//Either there's no light to sample or we chose an infinite area light
			if (lightPrimitive == nullptr)
				return glm::vec3(0);

			Light *light = s->lights.at(i)->model->getRandomLightPrimitive()->material->light;

			return estimateDirect(incoming, intersecOnASurface, light, s->lights.at(i), s) / lightPdf;
			//return estimateDirectLightning(incoming, intersecOnASurface, s, light, s->lights.at(i)) / lightPdf; 
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

		std::vector<PointPathDebugInfo> getPath(Ray incoming, Scene *scene) {
			shouldComputePath = true;
			path.clear();
			glm::vec3 Radiance = Li(incoming, scene);
			shouldComputePath = false;
			return path;
		}

		glm::vec3 Li(Ray incoming, Scene *scene) {
			glm::vec3 L(0);
			glm::vec3 transmittance(1);
			RayIntersection intersection;
			
			//Store the current lightning ray path for debug purposes
			if (shouldComputePath)
				path.push_back({ incoming, transmittance, glm::vec3(-1), L});

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
								//L += transmittance * l->Le(incoming);
							}
						}
				}

				if (!didIntersect || !intersection.primitive->material->bsdf || isBlack(transmittance))
					break;

				if (intersection.primitive->material->bsdf->bsdfType & BxDF_TRANSMISSION) {
					Ray scattered;

					//Move ray origin to volume's AABB boundary
					incoming.o = incoming.getPointAt(intersection.tNear);
					intersection.tFar = intersection.tFar - intersection.tNear;
					intersection.tNear = 0;

					//Samples the Media for a scattered direction and point. Returns the transmittance from the incoming ray up to that point.
					Ray rayOCS = transformRay(incoming, intersection.instancedModel->invTransformToWCS);
					glm::vec3 sampledTransmittance = intersection.primitive->material->medium->sample(rayOCS, scattered, intersection);
					scattered = transformRay(scattered, intersection.instancedModel->transformToWCS);

					if (isAllOne(sampledTransmittance)) {
						incoming.o = incoming.getPointAt(intersection.tFar + 0.01f);
						b--;
						continue;
					}
					transmittance *= sampledTransmittance;

					if (debugText) {
						std::cout << "tNear: " << intersection.tNear << std::endl;
						std::cout << "tFar: " << intersection.tFar << std::endl;
						printVec3(incoming.o, "incoming.o: ");
						printVec3(scattered.o, "scattered.o: ");
					}

					//Evaluates the Phase Function BSDF
					glm::vec3 phaseFr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);
					//Evaluates the Phase Function BSDF PDF
					float phasePdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);

					//If the BSDF or its PDF is zero, then it is not a valid scattered ray direction
					if (isBlack(phaseFr) || phasePdf == 0.f ) {
						if (shouldComputePath)
							path.push_back({scattered, transmittance, phaseFr, L});
						break;
					}

					//Samples the Radiance arriving from a light source
					glm::vec3 lightSample = uniformSampleOneLight(scattered, intersection, scene);
					//lightSample = glm::vec3(0.11);
				
					if (debugText)
						printVec3(lightSample, "light Sample: ");

					transmittance *= phaseFr / phasePdf;

					if (isBlack(transmittance)) break;

					L += transmittance * lightSample;
					incoming = scattered;

					if (debugText)
						printVec3(transmittance, "transmittance: ");
					if (debugText)
						printVec3(L, "L: ");
					if(debugText)
						std::cout << std::endl;
					if (shouldComputePath)
						path.push_back({ incoming, transmittance, phaseFr, L });
				} else {
					//Sample BSDF for scattered direction
					float bsdfPdf;
					Ray scattered;

					L += transmittance * uniformSampleOneLight(incoming, intersection, scene);
					//L += 0.2f; //TODO debug REMOVE

					scattered.o = intersection.hitPoint; //TODO i stoped here, scattered direction was the same as incoming!
					scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, intersection.normal);
					bsdfPdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);
					//if (bsdfPdf < 0)
					//	break;
					glm::vec3 fr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);

					if (isBlack(fr) || bsdfPdf == 0.f) {
						//std::cout << "--------------\n";
						//std::cout << "BSDF\n";

						if (shouldComputePath)
							path.push_back({ scattered, transmittance, fr, L });
						break;
					}
					transmittance *= fr * absDot(incoming.direction, intersection.normal) / bsdfPdf;
					//specularBounce = (flags & BSDF_SPECULAR) != 0;

					incoming = scattered;

					if (shouldComputePath)
						path.push_back({ incoming, transmittance, fr, L });
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
				/*if (b > 3) {
					float p = glm::max(0.05f, 1 - transmittance.y);
					if (random() < p)
						break;
					transmittance = transmittance / (1 - p);
				}*/
			}

			//if (L.x < 0 || L.y < 0 || L.z < 0)
			//	std::cout << "Negative" << std::endl;
			//if (isnan(L.x) || isnan(L.y) || isnan(L.z))
			//	std::cout << "NaN" << std::endl;

			return L;
		}
	};
}