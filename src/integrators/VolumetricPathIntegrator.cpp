#include "integrators/VolumetricPathIntegrator.h" 

namespace narvalengine {
	uint32_t VolumetricPathIntegrator::debugCount = 0;

	Integrator* VolumetricPathIntegrator::clone() {
		return new VolumetricPathIntegrator();
	}

	bool VolumetricPathIntegrator::intersectTr(Ray ray, RayIntersection &intersection, glm::vec3* Tr, Scene* s) {
		*Tr = glm::vec3(1.0f);

		while (true) {
			RayIntersection intersec;
			bool hitSurface = s->intersectScene(ray, intersec, EPSILON3, INFINITY);
			intersection = intersec;

			// Accumulate beam transmittance for ray segment
			Medium* m = nullptr;
			if (hitSurface && intersection.primitive->hasMaterial())
				m = intersection.primitive->material->medium;
			if (m != nullptr) 
				*Tr *= m->Tr(ray, intersec);

			// Initialize next ray segment or terminate transmittance computation
			if (!hitSurface) return false;
			if (m != nullptr) return true;

			ray.o = intersec.hitPoint;
			ray.d = ray.d;
		}
	}

	glm::vec3 VolumetricPathIntegrator::visibilityTr(glm::vec3 intersecPoint, glm::vec3 lightPoint, Scene* scene) {
		Ray ray;
		ray.o = intersecPoint;
		ray.d = lightPoint - intersecPoint;

		glm::vec3 Tr(1.f);
		while (true) {
			RayIntersection intersec;
			bool hitSurface = scene->intersectScene(ray, intersec, EPSILON3, INFINITY);

			// Generate next ray segment or return final transmittance.
			if (!hitSurface)
				break;

			if (hitSurface && intersec.primitive->hasMaterial() && intersec.primitive->material->hasLight())
				return Tr;

			// Handle opaque surface along ray's path.
			if (hitSurface && intersec.primitive->hasMaterial() && intersec.primitive->material->hasMedium())
				return glm::vec3(0.0f);

			// Update transmittance for current ray segment.
			Medium* m = nullptr;
			if (intersec.primitive->hasMaterial())
				m = intersec.primitive->material->medium;
			if (m != nullptr) 
				Tr *= m->Tr(ray, intersec);

			ray.o = ray.getPointAt(intersec.tFar + EPSILON3);
			ray.d = lightPoint - ray.o;
		}
		return Tr;
	}

	glm::vec3 VolumetricPathIntegrator::estimateDirect(Ray incoming, RayIntersection intersecOnASurface, Light* light, InstancedModel* lightIm, Scene* scene) {
		glm::vec3 Ld(0.f);

		// Sample light source with multiple importance sampling.
		Ray scatteredWo;
		float lightPdf = 0, scatteringPdf = 0;;

		glm::vec3 Li = light->sampleLi(intersecOnASurface, lightIm->transformToWCS, scatteredWo, lightPdf);

		bool isSurfaceInteraction = false;
		if (intersecOnASurface.primitive->material->hasMedium())
			isSurfaceInteraction = true;

		if (lightPdf > 0 && !isBlack(Li)) {
			// Compute BSDF or phase function's value for light sample.
			glm::vec3 f;

			if (isSurfaceInteraction) {
				// Evaluate BSDF for light sampling strategy.
				f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface) * glm::abs(glm::dot(scatteredWo.d, intersecOnASurface.normal));
				scatteringPdf = intersecOnASurface.primitive->material->bsdf->pdf(incoming.d, scatteredWo.d, intersecOnASurface.normal);
			}
			else {
				// Evaluate phase function for light sampling strategy.
				f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
				scatteringPdf = f.x;
			}

			if (!isBlack(f)) {
				glm::vec3 pointOnLight = light->primitive->samplePointOnSurface(intersecOnASurface, lightIm->transformToWCS);
				Li *= visibilityTr(intersecOnASurface.hitPoint, pointOnLight, scene);

				// Add light's contribution to reflected radiance.
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

		// Sample BSDF with multiple importance sampling.
		if (true /*!IsDeltaLight(light.flags)*/) {
			glm::vec3 f;
			if (isSurfaceInteraction) {
				// Sample scattered direction for surface interactions.
				scatteredWo.d = intersecOnASurface.primitive->material->bsdf->sample(incoming.d, intersecOnASurface.normal);
				f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
				f *= glm::abs(glm::dot(scatteredWo.d, intersecOnASurface.normal));
				scatteringPdf = intersecOnASurface.primitive->material->bsdf->pdf(incoming.d, scatteredWo.d, intersecOnASurface.normal);
			}
			else {
				// Sample scattered direction for medium interactions.
				f = intersecOnASurface.primitive->material->bsdf->eval(incoming.d, scatteredWo.d, intersecOnASurface);
				//We define the normal of a scattering event as the z-axis up.
				scatteredWo.d = intersecOnASurface.primitive->material->bsdf->sample(incoming.d, glm::vec3(0, 0, 1));
				scatteringPdf = f.x;
			}

			if (!isBlack(f) && scatteringPdf > 0) {
				// Account for light contributions along sampled incoming direction.
				float weight = 1;
				if (true /*!sampledSpecular*/) {
					lightPdf = light->primitive->pdf(intersecOnASurface, lightIm->transformToWCS);
					if (lightPdf == 0) return Ld;
					weight = powerHeuristic(scatteringPdf, lightPdf);
				}

				// Find intersection and compute transmittance.
				RayIntersection lightIntersect;
				Ray ray;
				ray.o = intersecOnASurface.hitPoint;
				ray.d = scatteredWo.d;

				glm::vec3 Tr(1.0f);
				//Always handle media.
				bool foundSurfaceInteraction = intersectTr(ray, lightIntersect, &Tr, scene);

				// Add light contribution from material sampling.
				glm::vec3 Li(0.f);
				if (foundSurfaceInteraction) 
					if (lightIntersect.primitive->material->light == light)
						//should be      Li = lightIsect.Le(-wi);
						Li = light->Li(); //changed from Le to Li
				else
					Li = light->Li(); //changed from Le to Li

				if (!isBlack(Li))
					Ld += f * Li * Tr * weight / scatteringPdf;
			}
		}
		return Ld;
	}

	glm::vec3 VolumetricPathIntegrator::uniformSampleOneLight(Ray incoming, RayIntersection intersection, Scene* scene) {
		float r = random();
		int i = scene->lights.size() * r;

		//The PDF of choosing one light.
		float lightPdf = 1.0f / scene->lights.size();

		Primitive* lightPrimitive = scene->lights.at(i)->model->getRandomLightPrimitive();

		if (lightPrimitive == nullptr)
			return glm::vec3(0);

		Light* light = scene->lights.at(i)->model->getRandomLightPrimitive()->material->light;

		return estimateDirect(incoming, intersection, light, scene->lights.at(i), scene) / lightPdf;
	}

	glm::vec3 VolumetricPathIntegrator::Li(Ray incoming, Scene* scene) {
		glm::vec3 L(0);
		glm::vec3 transmittance(1);
		RayIntersection intersection;

		//Store the current lightning ray path for debug purposes.
		if (shouldDebug())
			path.push_back({ incoming, transmittance, glm::vec3(-1), L });

		for (int b = 0; b < scene->settings.bounces; b++) {

			bool didIntersect = scene->intersectScene(incoming, intersection, EPSILON3, INFINITY);

			//If the transmittance reaches 0 there is no more light contribution possible in this path.
			if (isBlack(transmittance) /*|| !didIntersect || !intersection.primitive->material->bsdf*/)
				break;

			//If intersected a volume, which has BxDF type BxDF_TRANSMISSION.
			if (didIntersect && intersection.primitive->hasMaterial() && intersection.primitive->material->hasBSDF() && intersection.primitive->material->bsdf->hasType(BxDF_TRANSMISSION)) {
				Ray scattered;

				//Move ray origin to volume's AABB boundary.
				incoming.o = incoming.getPointAt(intersection.tNear);
				intersection.tFar = intersection.tFar - intersection.tNear;
				intersection.tNear = 0;

				//Samples the Media for a scattered direction and point. 
				//Returns the transmittance from the incoming ray up to the sampled point where the scattering event occured.
				glm::vec3 sampledTransmittance = intersection.primitive->material->medium->sample(incoming, scattered, intersection);

				//If the transmittance sampled along the ray was (1, 1, 1) it means that we did not hit any voxel with density > 1.
				//Hence, it is not a valid bounce count.
				if (areAllOne(sampledTransmittance)) {
					incoming.o = incoming.getPointAt(intersection.tFar + 0.01f);
					b--;
					continue;
				}

				transmittance *= sampledTransmittance;

				//Evaluates the Phase Function BSDF.
				glm::vec3 phaseFr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);
				//Evaluates the Phase Function BSDF PDF.
				float phasePdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);

				//If the BSDF or its PDF is zero, then it is not a valid scattered ray direction
				if (isBlack(phaseFr) || phasePdf == 0.f) {
					if (shouldDebug())
						path.push_back({ scattered, transmittance, phaseFr, L });
					break;
				}

				//Samples the Radiance coming from a light source into this scattered event.
				glm::vec3 lightSample = uniformSampleOneLight(scattered, intersection, scene);

				transmittance *= phaseFr / phasePdf;

				//if (isBlack(transmittance)) break;

				L += transmittance * lightSample;
				incoming = scattered;

				if (shouldDebug())
					path.push_back({ incoming, transmittance, phaseFr, L });
			}
			//If we intersected a surface.
			else {

				if (b == 0 /*|| specularBounce*/) {
					//If ray intersected a primitive and it is emissive, get its Light Emission Radiance (Le).
					if (didIntersect && intersection.primitive->hasMaterial() && intersection.primitive->material->hasLight())
						L += transmittance * intersection.primitive->material->light->Li();
					else
						//For each scene's light, get its Le (which is different from 0 only for infinite area lights)
						for (InstancedModel* light : scene->lights) {
							for (Material* m : light->model->materials) {
								Light* l = m->light;
								L += transmittance * l->Le(incoming, light->invTransformToWCS);
							}
						}
				}

				if (!didIntersect || !intersection.primitive->hasMaterial() || !intersection.primitive->material->hasBSDF())
					break;

				//Sample BSDF for scattered direction
				float bsdfPdf;
				Ray scattered;

				L += transmittance * uniformSampleOneLight(incoming, intersection, scene);

				scattered.o = intersection.hitPoint;
				scattered.d = intersection.primitive->material->bsdf->sample(incoming.d, intersection.normal);
				bsdfPdf = intersection.primitive->material->bsdf->pdf(incoming.d, scattered.d, intersection.normal);
				glm::vec3 fr = intersection.primitive->material->bsdf->eval(incoming.d, scattered.d, intersection);

				if (isBlack(fr) || bsdfPdf == 0.f) {
					if (shouldDebug())
						path.push_back({ scattered, transmittance, fr, L });
					break;
				}
				transmittance *= fr * absDot(incoming.direction, intersection.normal) / bsdfPdf;

				incoming = scattered;

				if (shouldDebug())
					path.push_back({ incoming, transmittance, fr, L });
			}
		}

		#ifdef NE_DEBUG_MODE
		//Resets the counter in order to avoid any possible integer overflow.
		//Also resets de vectors.
		if (debugCount > 0 && shouldDebug()) {
			path.clear();
			visibilityPath.clear();
			debugCount = 0;
		}
		debugCount++;
		DLOG_IF(WARNING, glm::any(glm::isnan(L))) << "Radiance was NaN. Value was: " << toString(L) << ".";
		DLOG_IF(WARNING, glm::any(glm::lessThan(L, glm::vec3(0)))) << "Radiance was negative. Value was: " << toString(L) << ".";
		DLOG_IF(WARNING, glm::any(glm::isinf(L))) << "Radiance was infinite. Value was: " << toString(L) << ".";
		#endif

		return L;
	}
};
