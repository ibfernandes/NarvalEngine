#pragma once
#include <iostream>
#include <fstream>
#include "Model.h"
#include "Sphere.h"
#include "Ray.h"
#include "Camera.h"
#include "Settings.h"
#include "Volume.h"
#include "Math.h"
#include "DiffuseMaterial.h"
#include "RoughConductorBRDF.h"
#include "DielectricMaterial.h"
#include "DiffuseLight.h"
#include "MetalMaterial.h"
#include "GridMaterial.h"
#include "BucketLBVH.h"
#include "MaterialBRDF.h"
#include "OBB.h"
#include <assert.h>
#include <Vector>
#include <limits>
#include <cstddef>
#include <glm/glm.hpp>
#include <bitset>
#include "Timer.h"
#include <thread>
#include <atomic>
#define MULTI_THREAD_MODE true

class OfflineEngine
{
public:
	std::vector<Model*> models;
	std::vector<Model*> emitters;
	Camera cam;
	const int bitsPerChannel = 16;
	const int maxValueChannel = std::pow(2, bitsPerChannel) - 1;
	std::thread *threads;
	std::atomic<bool> *isThreadDone;
	int numOfThreads;
	int spp = 100;
	int maxBounces = 3;
	glm::ivec2 numOfTiles;
	glm::ivec2 tileSize;
	 int windowWidth = 400, windowHeight = 200, totalPixels = windowWidth * windowHeight;
	glm::ivec3 *pixels;

	glm::vec3 lightPos = glm::vec3(2.8f, 1.2f, 0.f);
	glm::vec3 upColor = glm::vec3(0.00f);
	glm::vec3 bottomColor = glm::vec3(0.00f);

	glm::vec3 blend(glm::vec3 v1, glm::vec3 v2, float t) {
		return (1.0f - t) * v1 + t * v2;
	}
	
	bool checkHits(Ray &r, float tMin, float tMax, Hit &hit) {
		Hit tempHit;
		bool hitAnything = false;
		float closestHit = tMax;

		for (int i = 0; i < models.size(); i++) {

			if ((*models.at(i)).geometry->hit(r, tMin, closestHit, tempHit)) {
				hitAnything = true;
				closestHit = tempHit.t;
				hit = tempHit;
				hit.modelId = (*models.at(i)).modelId;
			}
		}

		return hitAnything;
	}

	glm::vec3 calculateColorRecursive(Ray r, int depth) {
		Hit hit;

		//if (depth >= maxBounces)
		//	return glm::vec3(0, 0, 0);
		if (depth == maxBounces)
			r.direction = glm::normalize(lightPos - r.o);

		if (depth > maxBounces)
			return glm::vec3(0, 0, 0);

		if (!checkHits(r, 0.001, std::numeric_limits<float>::max(), hit)) {
			glm::vec3 dir = glm::normalize(r.direction);
			float t = 0.5f * (dir.y + 1.0);
			return blend(bottomColor, upColor, t);
		}

		Ray scattered;
		glm::vec3 attenuation(1, 1, 1);
		glm::vec3 emitted = hit.material->emitted(0, 0, glm::vec3(0));
		float pdf;

		if (!hit.material->scatter(r, hit, attenuation, scattered, pdf))
			return emitted;

		//float pdf =  1.0f / spherePDF(hit.p, lightPos, hit.normal, 0.2f);

		return emitted + attenuation * calculateColorRecursive(scattered, depth + 1) ;
		//emitted + att (emitted + att( ... ))
	}

	glm::vec3 calculateColorNaive(Ray r, int depth) {
		Hit hit;
		glm::vec3 finalColor(0.0f);
		glm::vec3 throughput(1.0f);

		for (int i = 0; i < maxBounces; i++) {

			if (!checkHits(r, 0.001, std::numeric_limits<float>::max(), hit)) {
				glm::vec3 dir = glm::normalize(r.direction);
				float t = 0.5f * (dir.y + 1.0);
				finalColor += throughput * blend(bottomColor, upColor, t);
				break;
			}

			Ray scattered;
			glm::vec3 attenuation(1, 1, 1);
			glm::vec3 emitted = hit.material->emitted(0, 0, glm::vec3(0));
			float pdf;

			if (!hit.material->scatter(r, hit, attenuation, scattered, pdf)) {
				finalColor += emitted * throughput;
			}

			r = scattered;
			throughput *= attenuation;
		}

		return finalColor;
	}

	glm::vec3 calculateColorBRDF(Ray r, int depth) {
		Hit hit;
		glm::vec3 finalColor(0.0f);
		glm::vec3 throughput(1.0f);
		Ray incoming = r;

		for (int i = 0; i < maxBounces; i++) {

			if (!checkHits(r, 0.001, std::numeric_limits<float>::max(), hit)) {
				glm::vec3 dir = glm::normalize(r.direction);
				float t = 0.5f * (dir.y + 1.0);
				finalColor += throughput * blend(bottomColor, upColor, t);
				break;
			}

			Ray scattered;
			glm::vec3 emitted = hit.material->emitted(0, 0, glm::vec3(0));
			float pdf;
			bool isEmissive = !isAllZero(emitted);

			if (isEmissive) 
				finalColor += emitted * throughput;

			if (isAllZero(emitted)) {
				float pdf = 1;
				hit.material->brdf->sample(incoming, scattered, hit);
				glm::vec3 brdf = hit.material->brdf->eval(incoming, scattered, hit, pdf);
				brdf = brdf * ( 1.0f / pdf);
				throughput *= brdf;
			}

			r = scattered;
		}

		return finalColor;
	}

	//evaluates BRDF * Li
	glm::vec3 estimateDirect(Model *emitter, Ray incoming, Hit hit) {
		glm::vec3 directLight(0);
		Sphere *s = (Sphere*)emitter->geometry;

		glm::vec3 Li = s->material->emitted(0, 0, glm::vec3(0));
		bool isEmissive = !isAllZero(Li);

		//dir.xyz = direction towards sphere
		//dir.w = pdf of direction
		glm::vec4 dir = sampleSphere(hit.p, s->center, s->radius);
		Ray scattered;
		scattered.o = hit.p;
		scattered.d = dir;

		Ray occlusionRay{ hit.p, glm::vec3(dir) };
		Hit occlusionHit;
		if (checkHits(occlusionRay, 0.001, std::numeric_limits<float>::max(), occlusionHit)) {
			//If it is occluded and this object is not emissive, no contribution is computed
			if (isAllZero(occlusionHit.material->emitted(0, 0, glm::vec3(0, 0, 0))))
				return directLight;
		}

		if (dir.w != 0 && isAllZero(hit.material->emitted(0,0,glm::vec3(0)))) {
			float pdf;
			glm::vec3 brdf = hit.material->brdf->eval(incoming, scattered, hit, pdf);
			directLight += brdf * Li *  (1.0f / dir.w);
		}

		return directLight;
	}

	Model *getModelById(int modelId) {
		for (Model *m : models)
			if (m->modelId == modelId)
				return m;
		return nullptr;
	}

	//TODO: measure min and max pdf
	float brdfmin = 9999999999;
	float brdfmax = 0;
	float lightmin = 9999999999;
	float lightmax = 0;

	glm::vec3 estimateDirectMIS(Model *emitter, Ray incoming, Hit hit) {
		glm::vec3 directLight(0);
		Sphere *s = (Sphere*)emitter->geometry;

		glm::vec3 Li = s->material->emitted(0, 0, glm::vec3(0));
		bool isEmissive = !isAllZero(Li);

		//dir.xyz = direction towards sphere
		//dir.w = pdf of direction
		glm::vec4 dir = sampleSphere(hit.p, s->center, s->radius);
		float lightPDF = dir.w;
		Ray scattered;
		scattered.o = hit.p;
		scattered.d = dir;

		Ray occlusionRay{ hit.p, glm::vec3(dir) };
		Hit occlusionHit;
		if (checkHits(occlusionRay, 0.001, std::numeric_limits<float>::max(), occlusionHit)) {
			//If it is occluded and this object is not emissive, no contribution is computed
			if (isAllZero(occlusionHit.material->emitted(0, 0, glm::vec3(0, 0, 0))))
				return directLight;
		}

		//Eval light
		float brdfPDF = 0;
		float NdotWo = glm::max(0.0f, glm::dot(hit.normal, scattered.d));
		if (NdotWo > 0 && lightPDF != 0 && isAllZero(hit.material->emitted(0, 0, glm::vec3(0)))) {
			glm::vec3 brdf = hit.material->brdf->eval(incoming, scattered, hit, brdfPDF);

			brdfmin = glm::min(brdfmin, brdfPDF);
			brdfmax = glm::max(brdfmax, brdfPDF);

			lightmin = glm::min(lightmin, lightPDF);
			lightmax = glm::max(lightmax, lightPDF);

			if (brdfPDF != 0) {
				float weight = powerHeuristic(lightPDF, brdfPDF);
				directLight += brdf * Li *  weight / lightPDF;
			}
		}
		
		//Eval BRDF
		hit.material->brdf->sample(incoming, scattered, hit);
		glm::vec3 brdf = hit.material->brdf->eval(incoming, scattered, hit, brdfPDF);
		occlusionRay.o = hit.p;
		occlusionRay.d = scattered.d;
		Hit brdfHit;
		brdfHit.modelId = -1;
		NdotWo = glm::max(0.0f, glm::dot(hit.normal, scattered.d));

		if (hit.modelId == 4) // first plate
			float kk = 0;

		if (NdotWo > 0 && brdfPDF != 0.0f && !isAllZero(brdf)) {
			if (checkHits(occlusionRay, 0.01, std::numeric_limits<float>::max(), brdfHit)) {
				//If it is occluded and this object is not emissive, no contribution is computed
				if (isAllZero(brdfHit.material->emitted(0, 0, glm::vec3(0, 0, 0))))
					return directLight;
			}
			else {
				return directLight;
			}

			if(brdfHit.modelId != emitter->modelId)
				return directLight;

			/*if (brdfHit.modelId!=-1 && !isAllZero(brdfHit.material->emitted(0, 0, glm::vec3(0, 0, 0))))
				s = (Sphere*)getModelById(brdfHit.modelId);
			else
				return directLight;*/

			lightPDF = spherePDF(hit.p, s->center, s->radius);

			if (lightPDF == 0.0f) {
				// We didn't hit anything, so ignore the brdf sample	
				return directLight;
			}

			//float weight = powerHeuristic(brdfPDF, lightPDF/1000.0f);
			float weight = powerHeuristic(brdfPDF, lightPDF);
			directLight += brdf * Li * weight / brdfPDF;
		}

		return directLight;
	}

	glm::vec3 sampleLights(Ray &incoming, Hit hit) {
		if (!isAllZero(hit.material->emitted(0, 0, glm::vec3(0))))
			return glm::vec3(0);

		glm::vec3 l(0);
		for (int i = 0; i < emitters.size(); i++) {
			if (hit.modelId == emitters.at(i)->modelId)
				continue;
			//l = l + estimateDirect(emitters.at(i), incoming, hit);
			l = l + estimateDirectMIS(emitters.at(i), incoming, hit);
		}

		return l;
	}

	glm::vec3 integrateToLight(Ray dirToLight, GridMaterial *gm, Geometry *collider) {

		Hit h;
		bool didHit = collider->hit(dirToLight, -0.001f, 99999, h);
		//happens when we just hit the boundary
		if (!didHit)
			return glm::vec3(1);

		glm::vec3 thit = gm->lbvh->traverseTreeUntil(dirToLight, 999999);
		float density = thit.z;
		glm::vec3 Tr = exp(-gm->extinction * density);

		return Tr;
	}

	//"samplelights"
	glm::vec3 volumetricShadowRay(glm::vec3 volumePoint, GridMaterial *gm, Geometry *collider) {

		glm::vec3 l(0);
		for (int i = 0; i < emitters.size(); i++) {
			Sphere *s = (Sphere*)emitters.at(i)->geometry;
			glm::vec3 Li = s->material->emitted(0, 0, glm::vec3(0));
			//TODO: instead of center should cone sample
			glm::vec3 dirToLight = glm::normalize(s->center - volumePoint);

			l = l + Li * integrateToLight(Ray(volumePoint, dirToLight), gm, collider);
		}

		return l;
	}

	//kind of a lambertian brdf?
	float isotropicPhaseFunction() {
		return 1.0f / (4.0f * PI);
	}

	bool integrateVolume(Ray incoming, Ray &scattered, Hit &hit, glm::vec3 &tr, glm::vec3 &inScattering, GridMaterial *gm, glm::vec3 throughtput) {
		glm::vec3 totalTr = glm::vec3(1.0f);
		glm::vec3 currentTr = glm::vec3(0);
		glm::vec3 inScatter = glm::vec3(0.0f);
		//move ray origin to hit point on the volume boundary
		incoming.o = incoming.o + hit.t * incoming.d;

		float t = -std::log(1 - random()) / gm->extinctionAvg;
		glm::vec3 thit = gm->lbvh->traverseTreeUntil(incoming, t);
		thit.x = glm::max(0.0f, thit.x);

		//point is outside volume bounding box
		if (t > hit.tFar) {
			t = hit.tFar + 0.01f;
		}

		//missed all lbvh nodes inside this bounding box
		//glm::vec3 thitThrought = gm->lbvh->traverseTreeUntil(incoming, 999999);
		if (thit.x > thit.y || thit.z == 0) {
			tr = totalTr;
			inScattering = inScatter;
			scattered.d = incoming.d;
			t = (hit.tFar - hit.t) + 0.01f;
			scattered.o = incoming.o + t * incoming.d;
			return false;
		}

		//only change direction if hit anything
		scattered.d = sampleUnitSphere();
		scattered.o = incoming.o + thit.y * incoming.d;

		float density = thit.z; //TODO should not have thit.y

		currentTr = exp(-gm->extinction * density);
		totalTr *= currentTr;
		//throughtput *= currentTr;

		glm::vec3 Ls = volumetricShadowRay(scattered.o, gm, hit.collider) * isotropicPhaseFunction();
		//Integrate Ls from 0 to d
		Ls = (Ls - Ls * currentTr) / gm->extinction;

		inScatter += throughtput * gm->scattering * Ls;

		tr = totalTr;
		inScattering = inScatter;
		return true;
	}

	glm::vec3 integrateToLightHomogeneous(Ray dirToLight, GridMaterial *gm, Geometry *collider) {

		Hit h;
		bool didHit = collider->hit(dirToLight, -0.001f, 99999, h);
		//happens when we just hit the boundary
		if (!didHit)
			return glm::vec3(1);
		float volumeDensityAcross = 3.1f;
		float density = volumeDensityAcross * h.tFar;
		glm::vec3 Tr = exp(-gm->extinction * density);

		return Tr;
	}

	//"samplelights"
	glm::vec3 volumetricShadowRayHomogeneous(glm::vec3 volumePoint, GridMaterial *gm, Geometry *collider) {

		glm::vec3 l(0);
		for (int i = 0; i < emitters.size(); i++) {
			Sphere *s = (Sphere*)emitters.at(i)->geometry;
			glm::vec3 Li = s->material->emitted(0, 0, glm::vec3(0));
			//TODO: instead of center should cone sample
			glm::vec3 dirToLight = glm::normalize(s->center - volumePoint);

			l = l + Li * integrateToLightHomogeneous(Ray(volumePoint, dirToLight), gm, collider);
		}

		return l;
	}

	bool integrateVolumeHomogeneous(Ray incoming, Ray &scattered, Hit &hit, glm::vec3 &tr, glm::vec3 &inScattering, GridMaterial *gm) {
		glm::vec3 totalTr = glm::vec3(1.0f);
		glm::vec3 currentTr = glm::vec3(0);
		glm::vec3 inScatter = glm::vec3(0.0f);
		//move ray origin to hit point on the volume boundary
		incoming.o = incoming.o + hit.t * incoming.d;
		float volumeDensityAcross = 3.1f;

		float t = -std::log(1 - random()) / gm->extinctionAvg;

		//point is outside volume
		if (t > hit.tFar) {
			t = hit.tFar + 0.01f;
		}

		scattered.d = sampleUnitSphere();
		scattered.o = incoming.o + t * incoming.d; //TODO: THIS HERE IS WRONG, I ALREADY MESSED WITH INCOMING.O UP THERE

		float density = volumeDensityAcross * t;

		currentTr = exp(-gm->extinction * density);
		totalTr *= currentTr;

		glm::vec3 Ls = volumetricShadowRayHomogeneous(scattered.o, gm, hit.collider) * isotropicPhaseFunction();
		//Integrate Ls from 0 to d
		Ls = (Ls - Ls * currentTr) / gm->extinction;

		inScatter += totalTr * gm->scattering * Ls;

		tr = totalTr;
		inScattering = inScatter;
		return true;
	}

	glm::vec3 calculateColorWithExplicitLight(Ray r, int depth) {
		Hit hit;
		glm::vec3 finalColor(0.0f);
		glm::vec3 throughput(1.0f);
		Ray incoming = r;

		for (int bounce = 0; bounce < maxBounces; bounce++) {

			if (!checkHits(incoming, -0.001, std::numeric_limits<float>::max(), hit)) {
				glm::vec3 dir = glm::normalize(incoming.direction);
				float t = 0.5f * (dir.y + 1.0);
				finalColor += throughput * blend(bottomColor, upColor, t);
				break;
			}

			Ray scattered;
			glm::vec3 attenuation(1, 1, 1);
			glm::vec3 emitted = hit.material->emitted(0, 0, glm::vec3(0));
			float pdf;
			bool isEmissive = !isAllZero(emitted);

			if (isEmissive && bounce == 0) {
				finalColor += throughput * emitted;
			}

			// Calculate the direct lighting
			if (GridMaterial *gm = dynamic_cast<GridMaterial*>(hit.material)) {
				glm::vec3 tr(1);
				glm::vec3 inScattering(0);
				//bool sampled = integrateVolumeHomogeneous(incoming, scattered, hit, tr, inScattering, gm);
				bool sampled = integrateVolume(incoming, scattered, hit, tr, inScattering, gm, throughput);
				
				incoming = scattered;
				finalColor += throughput * inScattering;
				throughput *=  tr ;

				continue;
			}

			//direct light contribution
			finalColor += throughput * sampleLights(incoming, hit);

			if (isAllZero(emitted)) {
				float pdf = 0;
				hit.material->brdf->sample(incoming, scattered, hit);
				glm::vec3 brdf = hit.material->brdf->eval(incoming, scattered, hit, pdf);
				if (pdf != 0)
					brdf = brdf * (1.0f / pdf);
				else
					brdf = glm::vec3(0);
				throughput *= brdf;
			}

			incoming = scattered;
		}

		return finalColor;
	}

	float calculateEmptyRatio(float *grid, int size) {
		int count = 0;
		for (int i = 0; i < size; i++)
			if (grid[i] == 0.0f)
				count++;

		return float(count) / float(size);
	}

	void init(Camera c, Settings s) {
		cam = c;
		spp = s.spp;
		windowWidth = s.resolution.x;
		windowHeight = s.resolution.y;
		totalPixels = windowWidth * windowHeight;
		maxBounces = s.bounces;

		//Transfers all data from ResourceManager to local vector for faster processing
		std::map<std::string, Model*> m = ResourceManager::getSelf()->getModels();
		std::map<std::string, Model*>::iterator it;
		for (it = m.begin(); it != m.end(); ++it) {
			models.push_back(it->second);

			Material *m = (it->second)->geometry->material;
			
			if(!isAllZero(m->emitted(0,0,glm::vec3(0))))
				emitters.push_back(it->second);
		}

		if (MULTI_THREAD_MODE) {
			numOfThreads = 8;
			numOfTiles.x = 40;
			numOfTiles.y = 10;
			tileSize.x = windowWidth / numOfTiles.x;
			tileSize.y = windowHeight/ numOfTiles.y;
			threads = new std::thread[numOfThreads];
			isThreadDone = new std::atomic<bool>[numOfThreads];
			pixels = new glm::ivec3[totalPixels];
		}
	}

	glm::ivec3 postProcessing(glm::vec3 hdrColor) {
		const float gamma = 2.2;
		const float exposure = 0.5;

		// reinhard tone mapping
		//glm::vec3 mapped = hdrColor / (hdrColor + glm::vec3(1.0f));

		//exposure tone mapping
		glm::vec3 mapped = glm::vec3(1.0) - glm::exp(-hdrColor * exposure);
		// gamma correction 
		mapped = glm::pow(mapped, glm::vec3(1.0f / gamma));

		mapped *= float(maxValueChannel);

		return glm::max( glm::vec3(0,0,0), glm::min(mapped, glm::vec3(maxValueChannel)));
	}

	void tiledRendering(Camera cam, int index, std::atomic<bool> &p) {
		int mx = index % numOfTiles.x;
		int my = index / numOfTiles.x;
		mx = mx * tileSize.x;
		my = my * tileSize.y;

		for(int y = my; y < my + tileSize.y; y++)
			for (int x = mx; x < mx + tileSize.x; x++) {
				glm::vec3 color(0, 0, 0);
				for (int s = 0; s < spp; s++) {
					float u = float(x + random()) / windowWidth;
					float v = float(y + random()) / windowHeight;
					Ray r = cam.getRayPassingThrough(u, v);
					//color += calculateColor(r, 0);
					color += calculateColorWithExplicitLight(r, 0);
					//color += calculateColorBRDF(r, 0);
					//color += calculateColorNaive(r, 0);
				}
				color = color / float(spp);
				pixels[to1D(windowWidth, windowHeight, x, y, 0)] = postProcessing(color); 
			}

		p = true;
	}

	void mainLoop() {
		Timer t;
		std::ofstream file;
		file.open("output.ppm", std::ios::binary);
		file << "P6\n" << windowWidth << " " << windowHeight << "\n" << maxValueChannel << "\n";
		std::cout << "Processing..." << std::endl;
		t.startTimer();

		if (MULTI_THREAD_MODE) {
			int count = 0;

			for (int i = 0; i < numOfThreads; i++) {
				threads[i] = std::thread(&OfflineEngine::tiledRendering, this, std::ref(cam), count, std::ref(isThreadDone[i]));
				count++;
			}

			while (count < numOfTiles.x * numOfTiles.y) {
				for (int i = 0; i < numOfThreads; i++) {
					if (count == numOfTiles.x * numOfTiles.y)
						break;

					if (isThreadDone[i]) {
						threads[i].join();
						isThreadDone[i] = false;
						threads[i] = std::thread(&OfflineEngine::tiledRendering, this, std::ref(cam), count, std::ref(isThreadDone[i]));
						count++;
					}
				}
			}

			for (int i = 0; i < numOfThreads; i++)
				threads[i].join();

			for (int i = totalPixels - 1; i >= 0; i--) {
				//file << (int)pixels[i].x << " " << (int)pixels[i].y << " " << (int)pixels[i].z << "\n";
				uint16_t x = pixels[i].x;
				uint16_t y = pixels[i].y;
				uint16_t z = pixels[i].z;
				//std::cout << x << " " << y << " " << z << std::endl;
				
				uint8_t out[2];
				out[0] = (x >> 8) & 0xFF;
				out[1] = x & 0xFF;
				file.write(reinterpret_cast<const char*>(&out), sizeof(uint16_t));

				out[0] = (y >> 8) & 0xFF;
				out[1] = y & 0xFF;
				file.write(reinterpret_cast<const char*>(&out), sizeof(uint16_t));

				out[0] = (z >> 8) & 0xFF;
				out[1] = z & 0xFF;
				file.write(reinterpret_cast<const char*>(&out), sizeof(uint16_t));

			}

		}else {
			//writes from left to right, from top to bottom.
			for (int y = windowHeight - 1; y >= 0; y--) {
				for (int x = 0; x < windowWidth; x++) {
					int pos = x + (windowHeight - y) * windowWidth;

					if (pos == int(totalPixels * 0.05f))
						std::cout << "5% done..." << std::endl;
					else if (pos == int(totalPixels * 0.10f))
						std::cout << "10% done..." << std::endl;
					else if (pos == int(totalPixels * 0.15f))
						std::cout << "15% done..." << std::endl;
					else if (pos == int(totalPixels * 0.20f))
						std::cout << "20% done..." << std::endl;
					else if (pos == int(totalPixels * 0.25f))
						std::cout << "25% done..." << std::endl;
					else if (pos == int(totalPixels * 0.50f))
						std::cout << "50% done..." << std::endl;
					else if (pos == int(totalPixels * 0.75f))
						std::cout << "75% done..." << std::endl;

					glm::vec3 color(0, 0, 0);
					for (int s = 0; s < spp; s++) {
						float u = float(x + random()) / windowWidth;
						float v = float(y + random()) / windowHeight;
						Ray r = cam.getRayPassingThrough(u, v);
						color += calculateColorRecursive(r, 0);
					}
					color = color / float(spp);
					color = float(maxValueChannel) * glm::vec3(sqrt(color.x), sqrt(color.y), sqrt(color.z));

					file << (int)color.x << " " << (int)color.y << " " << (int)color.z << "\n";
				}
			}
		}

		file.close();
		t.endTimer();
		std::cout << "File output.ppm written with successs" << std::endl;
		t.printlnSeconds();
		t.printlnMinutes();
		std::cout << '\a';
	}
	
	OfflineEngine();
	~OfflineEngine();
};

