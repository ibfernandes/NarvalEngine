#pragma once
#include <iostream>
#include <fstream>
#include <assert.h>
#include <Vector>
#include <limits>
#include <cstddef>
#include <glm/glm.hpp>
#include <bitset>
#include <thread>
#include <atomic>
#include "utils/Timer.h"
#include "core/Camera.h"
#include "core/Settings.h"
#include "core/Scene.h"
#include "integrators/VolumetricPathIntegrator.h"
#define MULTI_THREAD_MODE true

namespace narvalengine {
	class OfflineEngine {
	public:
		Camera camera;
		Settings settings;
		Scene scene;
		VolumetricPathIntegrator pathIntegrator;
		const int bitsPerChannel = 16;
		int maxValuePerChannel;
		int numberOfThreads;
		glm::ivec2 numberOfTiles;
		glm::ivec2 tileSize;
		std::thread *threadPool;
		std::atomic<bool> *isThreadDone;
		glm::vec3 *pixels;

		OfflineEngine(Camera camera, Settings settings, Scene scene) {
			updateOfflineEngine(camera, settings, scene);
		}

		void updateOfflineEngine(Camera camera, Settings settings, Scene scene) {
			this->camera = camera;
			this->settings = settings;
			this->scene = scene;

			maxValuePerChannel = std::pow(2, bitsPerChannel) - 1;
			numberOfThreads = 8;
			numberOfTiles = glm::ivec2(40, 10);
			tileSize.x = settings.resolution.x / numberOfTiles.x;
			tileSize.y = settings.resolution.y / numberOfTiles.y;
			threadPool = new std::thread[numberOfThreads];
			isThreadDone = new std::atomic<bool>[numberOfThreads];
			pixels = new glm::vec3[settings.resolution.x * settings.resolution.y];
		}

		glm::vec3 exposureToneMapping(glm::vec3 hdrColor, float exposure) {
			return glm::vec3(1.0) - glm::exp(-hdrColor * exposure);
		}

		glm::vec3 reinhardToneMapping(glm::vec3 hdrColor) {
			return hdrColor / (hdrColor + glm::vec3(1.0f));
		}

		glm::vec3 gammaCorrection(glm::vec3 hdrColor) {
			return glm::pow(hdrColor, glm::vec3(1.0f / 2.2f));
		}

		glm::vec3 postProcessing(glm::vec3 hdrColor) {
			const float exposure = 0.5;

			glm::vec3 mapped = exposureToneMapping(hdrColor, exposure);
			mapped = gammaCorrection(mapped);

			//mapped *= float(maxValuePerChannel);
			//return glm::clamp(mapped, glm::vec3(0, 0, 0), glm::vec3(maxValuePerChannel));

			return glm::clamp(mapped, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1));
		}

		void renderTile(Camera cam, int index, std::atomic<bool> &finished) {
			int mx = index % numberOfTiles.x;
			int my = index / numberOfTiles.x;
			mx = mx * tileSize.x;
			my = my * tileSize.y;

			for (int y = my; y < my + tileSize.y; y++)
				for (int x = mx; x < mx + tileSize.x; x++) {
					glm::vec3 color(0, 0, 0);
					for (int s = 0; s < settings.spp; s++) {
						float u = float(x + random()) / settings.resolution.x;
						float v = float(y + random()) / settings.resolution.y;
						Ray r = cam.getRayPassingThrough(u, v);
						color += pathIntegrator.Li(r, &scene);
					}
					color = color / float(settings.spp);
					pixels[to1D(settings.resolution.x, settings.resolution.y, x, y, 0)] = postProcessing(color);
				}


			finished = true;
		}

		void coreLoop() {
			Timer t;
			std::ofstream file;
			file.open("output.ppm", std::ios::binary);
			file << "P6\n" << settings.resolution.x << " " << settings.resolution.y << "\n" << maxValuePerChannel << "\n";
			std::cout << "Processing..." << std::endl;
			t.startTimer();

			if (MULTI_THREAD_MODE) {
				int renderedTiles = 0;

				//Initializes first N threads
				for (int i = 0; i < numberOfThreads; i++) {
					threadPool[i] = std::thread(&OfflineEngine::renderTile, this, std::ref(camera), renderedTiles, std::ref(isThreadDone[i]));
					renderedTiles++;
				}

				//While all tiles are not rendered keep checking which threads are done and realloc them to reamining tiles to render.
				while (renderedTiles < numberOfTiles.x * numberOfTiles.y) {
					for (int i = 0; i < numberOfThreads; i++) {
						if (renderedTiles == numberOfTiles.x * numberOfTiles.y)
							break;

						if (isThreadDone[i]) {
							threadPool[i].join();
							isThreadDone[i] = false;
							threadPool[i] = std::thread(&OfflineEngine::renderTile, this, std::ref(camera), renderedTiles, std::ref(isThreadDone[i]));
							renderedTiles++;
						}
					}
				}

				//When there is no more tiles to be assigned wait for reamining threads to finish
				for (int i = 0; i < numberOfThreads; i++)
					threadPool[i].join();
			} else {
				for (int i = 0; i < numberOfTiles.x * numberOfTiles.y; i++) 
					renderTile(camera, i, isThreadDone[0]);
			}

			//Write all rendered pixels to the file output.ppm
			for (int i = settings.resolution.x * settings.resolution.y - 1; i >= 0; i--) {
				uint16_t x = pixels[i].x * maxValuePerChannel;
				uint16_t y = pixels[i].y * maxValuePerChannel;
				uint16_t z = pixels[i].z * maxValuePerChannel;

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

			file.close();
			t.endTimer();
			std::cout << "File output.ppm written with successs" << std::endl;
			t.printlnSeconds();
			t.printlnMinutes();
			std::cout << '\a';
		}
	};

}