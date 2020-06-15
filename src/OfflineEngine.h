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
#include "DielectricMaterial.h"
#include "DiffuseLight.h"
#include "MetalMaterial.h"
#include "GridMaterial.h"
#include "LBVH2.h"
#include "LBVH3.h"
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
	const int bitsPerChannel = 8;
	const int maxValueChannel = std::pow(2, bitsPerChannel) - 1;
	std::thread *threads;
	std::atomic<bool> *isThreadDone;
	int numOfThreads;
	int spp = 100;
	int maxDepth = 3;
	glm::ivec2 numOfTiles;
	glm::ivec2 tileSize;
	 int windowWidth = 400, windowHeight = 200, totalPixels = windowWidth * windowHeight;
	glm::ivec3 *pixels;

	glm::vec3 lightPos = glm::vec3(2.8f, 1.2f, 0.f);
	glm::vec3 upColor = glm::vec3(0.0f);
	glm::vec3 bottomColor = glm::vec3(0.0f);

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
			}
		}

		return hitAnything;
	}

	glm::vec3 calculateColor(Ray r, int depth) {
		Hit hit;

		//if (depth >= maxDepth)
		//	return glm::vec3(0, 0, 0);
		if (depth == maxDepth)
			r.direction = glm::normalize(lightPos - r.o);

		if (depth > maxDepth)
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

		return emitted + attenuation * calculateColor(scattered, depth + 1) ;
		//emitted + att (emitted + att( ... ))
	}

	glm::vec3 calculateColorNaive(Ray r, int depth) {
		Hit hit;
		glm::vec3 finalColor(0.0f);
		glm::vec3 throughput(1.0f);

		for (int i = 0; i < maxDepth; i++) {

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



	//evaluates BRDF * Li
	glm::vec3 estimateDirect(Ray &incoming, Ray &scattered, Hit hit, Model *emitter, glm::vec3 attenuation) {
		glm::vec3 directLight(0);
		Sphere *s = (Sphere*)emitter->geometry;
		if (hit.material->hasSpecularLobe)
			return directLight;

		//glm::vec3 Li = hit.material->emitted(0,0, glm::vec3(0));
		glm::vec3 Li = s->material->emitted(0,0,glm::vec3(0));

		
		//trocar essas nomral
		glm::vec4 pdf = spherePDF(hit.p, s->center, s->radius);
		//incoming.direction = glm::vec3(pdf);

		if (pdf.w != 0 && !isAllZero(Li)) {
			
			float dotNL = glm::clamp(glm::dot(glm::vec3(pdf), glm::normalize(hit.normal)), 0.0f, 1.0f);
			scattered = Ray(incoming.o, glm::normalize(glm::vec3(pdf)));
			glm::vec3 bsdf = hit.material->brdf->eval(incoming, scattered, hit, attenuation);

			Hit d;
		
			if (dotNL > 0 && !checkHits(Ray(hit.p, pdf), 0.001, glm::length(glm::vec3(pdf)), d))
				directLight += bsdf * /*dotNL **/ Li * pdf.w;
		
		}

		return directLight;
	}

	glm::vec3 sampleLights(Ray &incoming, Ray &scattered, Hit t, glm::vec3 attenuation) {

		glm::vec3 l(0);
		//iterate over all lights
		for (int i = 0; i < emitters.size(); i++) {
			//if current hit is light, continue
			//if (t.material->emitted() == 0)
				//continue;
			if (t.material == emitters.at(i)->geometry->material)
				continue;
			l = l + estimateDirect(incoming, scattered, t, emitters.at(i), attenuation);
		}

		return l;
	}

	glm::vec3 calculateColorWithExplicitLight(Ray r, int depth) {
		Hit hit;
		glm::vec3 finalColor(0.0f);
		glm::vec3 throughput(1.0f);

		for (int i = 0; i < maxDepth; i++) {

			if (!checkHits(r, 0.001, std::numeric_limits<float>::max(), hit)) {
				glm::vec3 dir = glm::normalize(r.direction);
				float t = 0.5f * (dir.y + 1.0);
				finalColor += throughput * blend(bottomColor, upColor, t);
				break;
			}


			Ray scattered;
			float pdf;
			glm::vec3 attenuation(1, 1, 1);
			glm::vec3 emitted = hit.material->emitted(0, 0, glm::vec3(0));

			if ((/*hit.material->hasSpecularLobe ||*/ !hit.material->scatter(r, hit, attenuation, scattered, pdf)) && i == 0) {
				finalColor += emitted * throughput;
				//break;
			}


			finalColor += throughput * sampleLights(r, scattered, hit, attenuation);

			//r = scattered;

			throughput *= attenuation;
		}

		return finalColor;
	}

	void measurementTests() {
		glm::vec3 size = glm::vec3(256, 256, 256);;
		ResourceManager::getSelf()->loadVDBasTexture3D("cloud", "vdb/dragonHavard.vdb");
		glm::vec3 res = ResourceManager::getSelf()->getTexture3D("cloud")->getResolution();
		Timer *t = new Timer();

		std::cout << "Grid resolution: ";
		printVec3(res);

		LBVH *lbvh = new LBVH(size);

		float constructionTime1, constructionTime2;

		t->startTimer();
		LBVH2 *lbvh2 = new LBVH2(ResourceManager::getSelf()->getTexture3D("cloud")->floatData, res);
		t->endTimer();
		constructionTime1 = t->getMicroseconds();

		t->startTimer();
		LBVH3 *lbvh3 = new LBVH3(ResourceManager::getSelf()->getTexture3D("cloud")->floatData, res);
		t->endTimer();
		constructionTime2 = t->getMicroseconds();

		glm::vec3 origin = glm::vec3((res.x / 2.0f) + 0.2f, (res.y / 2.0f) + 0.2f, 0.5);
		glm::vec3 direction= glm::vec3(0, 0, -1);

		Ray *r = new Ray(lbvh2->getWCS(origin), direction);

		std::cout << "------------------------" << std::endl;
	/*	t->startTimer();
		res = lbvh->traverseTree(r->o, r->d);
		t->endTimer();
		std::cout << "Naive binary tree approach" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		t->printlnNanoSeconds();
		std::cout << std::endl;*/

		t->startTimer();
		res = lbvh2->traverseTree(*r);
		t->endTimer();
		std::cout << "Bucket binary tree approach" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		t->printlnNanoSeconds();
		std::cout << std::endl;

		t->startTimer();
		res = lbvh3->traverse(Ray(origin, direction));
		t->endTimer();
		std::cout << "Linear BVH paper approach" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		t->printlnNanoSeconds();
		std::cout << "------------------------" << std::endl;

		std::cout << "------------------------" << std::endl;
		std::cout << "Construction time" << std::endl;
		std::cout << "Bucket time: " << constructionTime1 << std::endl;
		std::cout << "Paper time: " << constructionTime2 << std::endl;
		std::cout << "------------------------" << std::endl;

		res = ResourceManager::getSelf()->getTexture3D("cloud")->getResolution();
		float avgBucket = 0;
		float avgPaper = 0;
		for(int x = 0; x < res.x; x++)
			for (int y = 0; y < res.y; y++) {
				origin = glm::vec3(x + 0.2f, y + 0.2f, 0.5);
				r = new Ray(lbvh2->getWCS(origin), direction);
				glm::vec3 val1, val2;
				float t1, t2;

				//Bucket
				t->startTimer();
				val1 = lbvh2->traverseTree(*r);
				t->endTimer();
				t1 = t->getMicroseconds();
				avgBucket += t1;

				//Paper
				t->startTimer();
				val2 = lbvh3->traverse(Ray(origin, direction));
				t->endTimer();
				t2 = t->getMicroseconds();
				avgPaper += t2;

				if (t1 > t2) {
					/*std::cout << "Bucket took more time" << std::endl;
					std::cout << "bucket time: " << t1 << std::endl;
					std::cout << "paper time: " << t2 << std::endl;
					std::cout << "bucket acc: " << val1.z << std::endl;
					std::cout << "paper acc: " << val2.z << std::endl;
					printVec3(origin);*/
				}

				if (val1.z != val2.z) {
					/*std::cout << "bucket: " << val1.z << std::endl;
					std::cout << "paper:  " << val2.z << std::endl;
					std::cout << "ERROR" << std::endl;*/
				}
			}

		std::cout << "------------------------" << std::endl;
		std::cout << "whole grid testing" << std::endl;
		std::cout << "Bucket approach" << std::endl;
		std::cout << "Avg time: " << avgBucket / (res.x * res.y) << std::endl;
		std::cout << "\t ~" << std::endl;

		std::cout << "Paper approach" << std::endl;
		std::cout << "Avg time: " << avgPaper / (res.x * res.y) << std::endl;
		std::cout << "------------------------" << std::endl;
		std::cout << '\a';
		while (true)
			continue;
	}

	void init(Camera c, Settings s) {
	
		measurementTests();
		while (true) continue;
		//glm::vec3 lookFrom(0.0f, 0, 4.5f);
		//glm::vec3 lookAt(0, 0, -1);
		//Camera cam(lookFrom, lookAt, glm::vec3(0, 1, 0), 45.0f, float(windowWidth) / float(windowHeight), 0.0001f, (lookFrom - lookAt).length());
		cam = c;
		spp = s.spp;
		windowWidth = s.resolution.x;
		windowHeight = s.resolution.y;
		totalPixels = windowWidth * windowHeight;
		maxDepth = s.bounces;

		//Transfers all data from ResourceManager to local vector for faster processing
		std::map<std::string, Model*> m = ResourceManager::getSelf()->getModels();
		std::map<std::string, Model*>::iterator it;
		for (it = m.begin(); it != m.end(); ++it) {
			models.push_back(it->second);

			Material *m = (it->second)->geometry->material;
			
			if(!isAllZero(m->emitted(0,0,glm::vec3(0))))
				emitters.push_back(it->second);
		}

		//LBVH2 *lbvh = new LBVH2(ResourceManager::getSelf()->getTexture3D("cloud")->floatData, ResourceManager::getSelf()->getTexture3D("cloud")->getResolution());
		//models.push_back(new Volume(new GridMaterial(lbvh), lbvh));
		//models.push_back(new Sphere(lightPos, 0.2f, new DiffuseLight(glm::vec3(20.5f))));
		//models.push_back(new Sphere(glm::vec3(0, 1.0, 1.0f), 0.5f, new DiffuseMaterial(glm::vec3(1.0, 0.1, 0.1))));
		//models.push_back(new Sphere(glm::vec3(1.0f, 1.0, -1.0f), 0.5f, new DielectricMaterial(1.5f)));
		//models.push_back(new Sphere(glm::vec3(0, -300.5f, -1), 300.0f, new DiffuseMaterial(glm::vec3(0.5, 0.5, 0.5))));

		/*for (int x = 0; x < 0; x++) {
			glm::vec3 center((2 * random() - 1) * 3, 0.0f, 3.0f * (2 * random() - 1));
			float materialChance = random();

			if (materialChance < 0.8)
				models.push_back(&*(new Sphere(center, 0.35f * random(), new DiffuseMaterial(glm::vec3(random()*random(), random()*random(), random()*random())))));
			else if (materialChance < 0.95)
				models.push_back(&*(new Sphere(center, 0.35f * random(), new MetalMaterial(glm::vec3(0.5 * (1 + random()), 0.5 * (1 + random()), 0.5 * (1 + random()))))));
			else
				models.push_back(&*(new Sphere(center, 0.35f * random(), new DielectricMaterial(1.5f))));
		}*/

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

	//TODO: gamma correction and tone mapping
	glm::ivec3 postProcessing(glm::vec3 v) {
		return glm::max( glm::vec3(0,0,0), glm::min(v, glm::vec3(maxValueChannel)));
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
					//color += calculateColorNaive(r, 0);
				}
				color = color / float(spp);
				color = float(maxValueChannel) * glm::vec3(sqrt(color.x), sqrt(color.y), sqrt(color.z));
				pixels[to1D(windowWidth, windowHeight, x, y, 0)] = postProcessing(color); 
			}

		p = true;
	}

	void mainLoop() {
		Timer t;
		std::ofstream file;
		file.open("output.ppm");
		file << "P3\n" << windowWidth << " " << windowHeight << "\n" << maxValueChannel << "\n";
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

			for(int i = totalPixels - 1; i >= 0 ; i--)
				file << (int)pixels[i].x << " " << (int)pixels[i].y << " " << (int)pixels[i].z << "\n";

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
						color += calculateColor(r, 0);
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

