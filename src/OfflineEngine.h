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
#include "LQTBVH.h"
#include "LBTBVH.h"
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

	float calculateEmptyRatio(float *grid, int size) {
		int count = 0;
		for (int i = 0; i < size; i++)
			if (grid[i] == 0.0f)
				count++;

		return float(count) / float(size);
	}

	void appendResultsToFile(std::string filename, std::string text) {
		std::ofstream file;
		file.open("tests\\" + filename + ".txt", std::ios_base::app);

		file << text << "\n";

		file.close();
	}

	std::string avgConstructionTime(std::string gridName, int bucketSize, std::string *algorithmNames, int numberOfAlgorithms) {
		ResourceManager::getSelf()->loadVDBasTexture3D(gridName, "vdb/" + gridName);
		float *grid = ResourceManager::getSelf()->getTexture3D(gridName)->floatData;
		glm::vec3 gridRes = ResourceManager::getSelf()->getTexture3D(gridName)->getResolution();
		int numSamples = 5;
		float *constructionTime = new float[numberOfAlgorithms];
		constructionTime[0] = 0;
		constructionTime[1] = 0;
		constructionTime[2] = 0;
		Timer *t = new Timer();

		for (int i = 0; i < numSamples; i++) {
			//binary
			t->startTimer();
			LBTBVH *lbtbvh = new LBTBVH(grid, gridRes, bucketSize);
			t->endTimer();
			constructionTime[0] += t->getMicroseconds();
			delete lbtbvh;

			//quad
			t->startTimer();
			LQTBVH *lqtbvh = new LQTBVH(grid, gridRes, bucketSize);
			t->endTimer();
			constructionTime[1] += t->getMicroseconds();
			delete lqtbvh;

			//paper
			t->startTimer();
			LBVH3 *lbvh3 = new LBVH3(grid, gridRes);
			t->endTimer();
			constructionTime[2] += t->getMicroseconds();
			delete lbvh3;
		}

		constructionTime[0] = constructionTime[0] / numSamples;
		constructionTime[1] = constructionTime[1] / numSamples;
		constructionTime[2] = constructionTime[2] / numSamples;

		std::stringstream ss;
		for (int i = 0; i < numberOfAlgorithms; i++) {
			ss << algorithmNames[i] << " avg. construction time: " << constructionTime[i] << std::endl;
		}
		ss << std::endl;

		return ss.str();
	}

	void fileTests() {
		int const numberOfGrids = 4;
		int const numberOfBucketSizes = 10;
		int const numberOfAlgorithms = 3;
		std::string gridNames[numberOfGrids] = {"dragonHavard.vdb", "wdas_cloud_sixteenth.vdb", "wdas_cloud_eighth.vdb", "wdas_cloud_quarter.vdb" };
		int bucketSizes[numberOfBucketSizes] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
		//int bucketSizes[numberOfBucketSizes] = { 512 };
		std::string algorithmNames[numberOfAlgorithms] = { "Bucket bin. tree", "Bucket quad tree", "Paper point tree" };

		float constructionTime[numberOfAlgorithms];
		Timer *t = new Timer();
		glm::vec3 origin = glm::vec3(1.0 + 0.2f, 84.0f + 0.2f, -1.0);
		glm::vec3 direction = glm::vec3(0, 0, 1);
		Ray *r = new Ray(origin, direction);

		for (int g = 0; g < numberOfGrids; g++) {
			ResourceManager::getSelf()->loadVDBasTexture3D(gridNames[g], "vdb/" + gridNames[g]);
			float *grid = ResourceManager::getSelf()->getTexture3D(gridNames[g])->floatData;
			glm::vec3 gridRes = ResourceManager::getSelf()->getTexture3D(gridNames[g])->getResolution();
			std::stringstream ss;
			ss << "==========================================" << std::endl;
			ss << "Grid name: " << gridNames[g] << std::endl;
			ss << "Grid resolution: " << "[" << gridRes.x << ", " << gridRes.y << ", " << gridRes.z << "]" << std::endl;
			ss << "Emptiness: " << calculateEmptyRatio(grid, gridRes.x * gridRes.y * gridRes.z) * 100 << "% " << std::endl;
			ss << "==========================================" << std::endl;
			appendResultsToFile(gridNames[g], ss.str());
			ss.str("");

			for (int b = 0; b < numberOfBucketSizes; b++) {
				if (bucketSizes[b] > (gridRes.x * gridRes.y * gridRes.z) / 4.0f)
					continue;

				//binary
				t->startTimer();
				LBTBVH *lbtbvh = new LBTBVH(grid, gridRes, bucketSizes[b]);
				t->endTimer();
				constructionTime[0] = t->getMicroseconds();

				//quad
				t->startTimer();
				LQTBVH *lqtbvh = new LQTBVH(grid, gridRes, bucketSizes[b]);
				t->endTimer();
				constructionTime[1] = t->getMicroseconds();

				//paper
				t->startTimer();
				LBVH3 *lbvh3 = new LBVH3(grid, gridRes);
				t->endTimer();
				constructionTime[2] = t->getMicroseconds();

				float avgTraverseTime[numberOfAlgorithms] = { 0,0,0 };
				float avgIntersectionTests[numberOfAlgorithms] = { 0,0,0 };
				float avgDensity[numberOfAlgorithms] = { 0,0,0 };

				for (int x = 0; x < gridRes.x; x++) {
					for (int y = 0; y < gridRes.y; y++) {
						origin = glm::vec3(x + 0.2f, y + 0.2f, 0.5);
						r = new Ray(origin, direction);
						glm::vec3 val[numberOfAlgorithms];
						float time[numberOfAlgorithms];

						//Bucket bin
						t->startTimer();
						val[0] = lbtbvh->traverse(*r);
						t->endTimer();
						time[0] = t->getMicroseconds();
						avgTraverseTime[0] += time[0];
						avgIntersectionTests[0] += lbtbvh->intersectionsCount;
						avgDensity[0] += val[0].z;

						//quadtree
						t->startTimer();
						val[1] = lqtbvh->traverseTree(*r);
						t->endTimer();
						time[1] = t->getMicroseconds();
						avgTraverseTime[1] += time[1];
						avgIntersectionTests[1] += lqtbvh->intersectionsCount;
						avgDensity[1] += val[1].z;

						//Paper
						t->startTimer();
						val[2] = lbvh3->traverse(Ray(origin, direction));
						t->endTimer();
						time[2] = t->getMicroseconds();
						avgTraverseTime[2] += time[2];
						avgIntersectionTests[2] += lbvh3->intersectionsCount;
						avgDensity[2] += val[2].z;
					}
				}

				ss << "-----------------------------------------" << std::endl;
				ss << "Current bucket Size: " << bucketSizes[b] << std::endl;
				ss << std::endl << std::endl;

				//TODO: estimated memory usage
				ss << algorithmNames[0] << std::endl;
				ss << lbtbvh->getStatus() << std::endl;

				ss << algorithmNames[1] << std::endl;
				ss << lqtbvh->getStatus() << std::endl;

				ss << algorithmNames[2] << std::endl;
				ss << lbvh3->getStatus() << std::endl;

				ss << std::endl;
				int winnerId = 0;
				float bestimer = 99999999;
				for (int a = 0; a < numberOfAlgorithms; a++) {
					ss << algorithmNames[a] << " construction time: " << constructionTime[a] << std::endl;
					if (constructionTime[a] < bestimer) {
						bestimer = constructionTime[a];
						winnerId = a;
					}
				}
				ss << "Winner: " << algorithmNames[winnerId] << std::endl;
				ss << "Ratio (Winner/paper): " << constructionTime[winnerId] / constructionTime[2] << std::endl;

				//ss << std::endl;
				//ss << avgConstructionTime(gridNames[g], bucketSizes[b], algorithmNames, numberOfAlgorithms);
				//std::cout << "Avg. construction time done." << std::endl;

				ss << std::endl;
				winnerId = 0;
				bestimer = 99999999;
				for (int a = 0; a < numberOfAlgorithms; a++) {
					float avgTime = avgTraverseTime[a] / (gridRes.x * gridRes.y);
					ss << algorithmNames[a] << " avg. traverse time: " << avgTime << std::endl;
					if (avgTime < bestimer) {
						bestimer = avgTime;
						winnerId = a;
					}
				}
				ss << "Winner: " << algorithmNames[winnerId] << std::endl;
				ss << "Ratio (Winner/paper): " << avgTraverseTime[winnerId] / avgTraverseTime[2] << std::endl;

				ss << std::endl;
				winnerId = 0;
				bestimer = 999999999;
				for (int a = 0; a < numberOfAlgorithms; a++) {
					float avgIntersec = avgIntersectionTests[a] / (gridRes.x * gridRes.y);
					ss << algorithmNames[a] << " avg. intersections tests: " << formatWithCommas(avgIntersec) << std::endl;
					if (avgIntersec < bestimer) {
						bestimer = avgIntersec;
						winnerId = a;
					}
				}
				ss << "Winner: " << algorithmNames[winnerId] << std::endl;
				ss << "Ratio (Winner/paper): " << avgIntersectionTests[winnerId] / avgIntersectionTests[2] << std::endl;

				ss << std::endl;
				for (int a = 0; a < numberOfAlgorithms; a++) {
					ss << algorithmNames[a] << " avg. density: " << avgDensity[a] << std::endl;
				}

				ss << "-----------------------------------------" << std::endl;
				appendResultsToFile(gridNames[g], ss.str());
				ss.str("");

				std::cout << "Grid: \"" << gridNames[g] << "\" with bucket size " << bucketSizes[b] << " finished." << std::endl;
				delete lbtbvh;
				delete lqtbvh;
				delete lbvh3;
			}
		}
	}

	void measurementTests() {
		fileTests();

		int const numberOfGrids = 1;
		//std::string gridNames[numberOfGrids] = {"dragonHavard.vdb", "wdas_cloud_sixteenth.vdb", "wdas_cloud_eighth.vdb", "wdas_cloud_quarter.vdb"};
		std::string gridNames[numberOfGrids] = {"wdas_cloud_eighth.vdb"};
		int currentGrid = 0;
		int const numberOfBucketSizes = 1;
		//int bucketSizes[] = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
		int bucketSizes[numberOfBucketSizes] = {4};
		int currentBucketSize = 0;
		int const numberOfAlgorithms = 3;
		float constructionTime[numberOfAlgorithms];
		std::string algorithmNames[numberOfAlgorithms] = { "Bucket bin. tree", "Bucket quad tree", "Paper point tree"};

		ResourceManager::getSelf()->loadVDBasTexture3D("cloud", "vdb/" + gridNames[currentGrid]);
		float *grid = ResourceManager::getSelf()->getTexture3D("cloud")->floatData;
		glm::vec3 gridRes = ResourceManager::getSelf()->getTexture3D("cloud")->getResolution();
		bool testGrid = false;
		if (testGrid) {
			gridRes = glm::vec3(3, 3, 3);
			grid = new float[gridRes.x * gridRes.y * gridRes.z];
			for (int i = 0; i < gridRes.x * gridRes.y * gridRes.z; i++)
				grid[i] = 1.0f;
		}
		Timer *t = new Timer();
		//glm::vec3 origin = glm::vec3((res.x / 2.0f) + 0.2f, (res.y / 2.0f) + 0.2f, 0.5);
		glm::vec3 origin = glm::vec3(1.0 + 0.2f, 84.0f + 0.2f, -1.0);
		//glm::vec3 origin = glm::vec3(0.5f, 0.5f, -1.0);
		glm::vec3 direction = glm::vec3(0, 0, 1);

		Ray *r = new Ray(origin, direction);

		std::cout << "Grid resolution: ";
		printVec3(gridRes);
		float empt = calculateEmptyRatio(grid, gridRes.x * gridRes.y * gridRes.z);
		std::cout << "Emptiness: " <<  empt << std::endl;

		//-------------------------------------------------
		//Construction

		t->startTimer();
		LQTBVH *lqtbvh = new LQTBVH(grid, gridRes, bucketSizes[currentBucketSize]);
		t->endTimer();
		constructionTime[0] = t->getMicroseconds();

		t->startTimer();
		LBVH3 *lbvh3 = new LBVH3(grid, gridRes);
		t->endTimer();
		constructionTime[1] = t->getMicroseconds();

		t->startTimer();
		LBTBVH *lbtbvh = new LBTBVH(grid, gridRes, bucketSizes[currentBucketSize]);
		t->endTimer();
		constructionTime[2] = t->getMicroseconds();

		//-------------------------------------------------
		//Tree status
		std::cout << lqtbvh->getStatus();
		lbvh3->printStatus();
		std::cout << lbtbvh->getStatus();

		//-------------------------------------------------
		//Single ray testing
		std::cout << "------------------------" << std::endl;
		float time[numberOfAlgorithms];
		glm::vec3 res;

		t->startTimer();
		res = lqtbvh->traverseTree(*r);
		t->endTimer();
		std::cout << "Bucket quad tree approach" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		std::cout << "hit [" << res.x << ", " << res.y << "]" << std::endl;
		time[0] = t->getMicroseconds();
		t->printlnNanoSeconds();
		std::cout << std::endl;

		t->startTimer();
		res = lbvh3->traverse(Ray(origin, direction));
		t->endTimer();
		std::cout << "Paper linear time bvh approach" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		std::cout << "hit [" << res.x << ", " << res.y << "]" << std::endl;
		time[1] = t->getMicroseconds();
		t->printlnNanoSeconds();
		std::cout << std::endl;

		t->startTimer();
		res = lbtbvh->traverse(Ray(origin, direction));
		t->endTimer();
		std::cout << "Bucket binary tree refactored" << std::endl;
		std::cout << "Accumulated " << res.z << std::endl;
		std::cout << "hit [" << res.x << ", " << res.y << "]" << std::endl;
		time[2] = t->getMicroseconds();
		t->printlnNanoSeconds();
		//std::cout << "------------------------" << std::endl;
		//std::cout << "Performance ratio: " << time[0] / time[2] << std::endl;

		std::cout << "------------------------" << std::endl;
		std::cout << "Single ray traversing time" << std::endl << std::endl;
		std::cout << "Bucket time: \t\t" << time[0] << std::endl;
		std::cout << "Bucket quadtree time: \t" << time[1] << std::endl;
		std::cout << "Paper time: \t\t" << time[2] << std::endl;
		std::cout << "------------------------" << std::endl;

		std::cout << "------------------------" << std::endl;
		std::cout << "Single ray intersections done" << std::endl << std::endl;
		std::cout << "Bucket: \t\t" << formatWithCommas(lbtbvh->intersectionsCount) << std::endl;
		std::cout << "Bucket quadtree: \t" << formatWithCommas(lqtbvh->intersectionsCount) << std::endl;
		std::cout << "Paper: \t\t\t" << formatWithCommas(lbvh3->intersectionsCount) << std::endl;
		std::cout << "------------------------" << std::endl;

		std::cout << "------------------------" << std::endl;
		std::cout << "Construction time" << std::endl << std::endl;
		std::cout << "Bucket time: \t\t" << constructionTime[0] << std::endl;
		std::cout << "Bucket quadtree time: \t" << constructionTime[1] << std::endl;
		std::cout << "Paper time: \t\t" << constructionTime[2] << std::endl;
		std::cout << "------------------------" << std::endl;

		//while (true)
		//	continue;
		res = ResourceManager::getSelf()->getTexture3D("cloud")->getResolution();
		float avgTraverseTime[numberOfAlgorithms] = {0,0,0};
		float avgIntersectionTests[numberOfAlgorithms] = {0,0,0};
		float avgDensity[numberOfAlgorithms] = {0,0,0};
		int numberOfCasesWhereILost = 0;
		int numberOfCasesItWasAccZero = 0;
		for (int x = 0; x < res.x; x++) {
			//std::cout << x << " of " << res.x << std::endl;
			for (int y = 0; y < res.y; y++) {
				origin = glm::vec3(x + 0.2f, y + 0.2f, 0.5);
				r = new Ray(origin, direction);
				glm::vec3 val[numberOfAlgorithms];
				float time[numberOfAlgorithms];

				//Bucket bin
				t->startTimer();
				val[0] = lbtbvh->traverse(*r);
				t->endTimer();
				time[0] = t->getMicroseconds();
				avgTraverseTime[0] += time[0];
				avgIntersectionTests[0] += lbtbvh->intersectionsCount;
				avgDensity[0] += val[0].z;

				//quadtree
				t->startTimer();
				val[1] = lqtbvh->traverseTree(*r);
				t->endTimer();
				time[1] = t->getMicroseconds();
				avgTraverseTime[1] += time[1];
				avgIntersectionTests[1] += lqtbvh->intersectionsCount;
				avgDensity[1] += val[1].z;

				//Paper
				t->startTimer();
				val[2] = lbvh3->traverse(Ray(origin, direction));
				t->endTimer();
				time[2] = t->getMicroseconds();
				avgTraverseTime[2] += time[2];
				avgIntersectionTests[2] += lbvh3->intersectionsCount;
				avgDensity[2] += val[2].z;

				if (time[0] > time[2]) {
					/*std::cout << "Bucket took more time" << std::endl;
					std::cout << "bucket time: " << t1 << std::endl;
					std::cout << "paper time: " << t2 << std::endl;
					std::cout << "bucket acc: " << val[0].z << std::endl;
					std::cout << "paper acc: " << val[2].z << std::endl;
					printVec3(origin);*/
					numberOfCasesWhereILost++;
					if (val[0].z == 0)
						numberOfCasesItWasAccZero++;
				}

				if (val[0].z != val[2].z) {
					/*std::cout << "bucket refactored: \t" << val[3].z << std::endl;
					std::cout << "paper: \t\t\t" << val[2].z << std::endl;
					std::cout << "ERROR" << std::endl;*/
				}
			}
		}

		std::cout << "------------------------" << std::endl;
		std::cout << "Whole grid testing" << std::endl << std::endl;
		std::cout << "Bucket binary approach" << std::endl;
		std::cout << "Avg time: " << avgTraverseTime[0] / (res.x * res.y) << std::endl;
		std::cout << "\t ~" << std::endl;

		std::cout << "Bucket quadtree approach" << std::endl;
		std::cout << "Avg time: " << avgTraverseTime[1] / (res.x * res.y) << std::endl;
		std::cout << "\t ~" << std::endl;

		std::cout << "Paper approach" << std::endl;
		std::cout << "Avg time: " << avgTraverseTime[2] / (res.x * res.y) << std::endl;
		std::cout << "\t ~" << std::endl;

		std::cout << "Bucket binary refactored approach" << std::endl;
		std::cout << "Avg time: " << avgTraverseTime[3] / (res.x * res.y) << std::endl;
		std::cout << "------------------------" << std::endl;

		std::cout << "Cases lost: " << numberOfCasesWhereILost << "/" << res.x*res.y <<std::endl;
		std::cout << "Cases lost with acc 0: " << numberOfCasesItWasAccZero << "/" << res.x*res.y <<std::endl;
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

