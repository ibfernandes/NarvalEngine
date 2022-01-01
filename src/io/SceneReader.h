#pragma once
#include "core/ResourceManager.h"
#include "core/Camera.h"
#include "core/Settings.h"
#include "core/VolumeBSDF.h"
#include "materials/Material.h"
#include "materials/GridMedia.h"
#include "materials/HomogeneousMedia.h"
#include "primitives/BucketLBVH.h"
#include "primitives/Sphere.h"
#include "primitives/Rectangle.h"
#include "primitives/Point.h"
#include "core/Scene.h"
#include "core/GlossyBSDF.h"
#include "core/Microfacet.h"
#include "lights/DiffuseLight.h"
#include "lights/DirectionalLight.h"
#include "lights/InfiniteAreaLight.h"
#include "utils/Math.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <glog/logging.h>

namespace narvalengine {
	class SceneReader {
	private:
		SceneSettings settings;
		Camera mainCamera;
		Scene *scene = nullptr;
		glm::vec3 getVec3(rapidjson::Value& v);
		void processMaterial(rapidjson::Value& material);
		void processPrimitives(rapidjson::Value& primitive);
		void processCameraAndRenderer(rapidjson::Value& camera, rapidjson::Value& renderer);

	public:
		/**
		 * Initiates the SceneReader with given scene.
		 * 
		 * @param path to json file.
		 * @param absolutePath if true, uses the absolute path contained in {@code filePath}. Otherwise the path contained in {@code filePath} is relative to RESOURCES_DIR.
		 */
		SceneReader(std::string filePath, bool absolutePath = false);
		/**
		 * Loads a scene from a json file.
		 *
		 * @param path to json file.
		 * @param absolutePath if true, uses the absolute path contained in {@code filePath}. Otherwise the path contained in {@code filePath} is relative to RESOURCES_DIR.
		 */
		void loadScene(std::string filePath, bool absolutePath = false);
		Camera getMainCamera();
		SceneSettings getSettings();
		Scene *getScene();

		SceneReader();
		~SceneReader();
	};
}