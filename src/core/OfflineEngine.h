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
#define NE_MULTI_THREAD_MODE true

namespace narvalengine {
	/**
	 * Class responsible for executing the Path Tracing algorithm using the CPU.
	 */
	class OfflineEngine {
	public:
		Camera camera;
		Scene *scene;
		SceneSettings settings;
		Integrator *pathIntegrator;
		const int bitsPerChannel = 16;
		const int maxValuePerChannel = std::pow(2, bitsPerChannel) - 1;
		std::atomic<bool>* isThreadDone;
		std::thread* threadPool;
		int numberOfThreads;
		glm::ivec2 numberOfTiles;
		glm::ivec2 tileSize;
		glm::vec3 *pixels;

		~OfflineEngine();
		OfflineEngine(Camera camera, SceneSettings settings, Scene *scene);
		void updateOfflineEngine(Camera camera, SceneSettings settings, Scene *scene);
		glm::vec3 exposureToneMapping(glm::vec3 hdrColor, float exposure);
		glm::vec3 reinhardToneMapping(glm::vec3 hdrColor);
		glm::vec3 gammaCorrection(glm::vec3 hdrColor);
		glm::vec3 postProcessing(glm::vec3 hdrColor);
		void renderTile(Camera cam, int index, std::atomic<bool>& finished);
		void coreLoop();
	};

}