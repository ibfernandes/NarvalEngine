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
#include "utils/Math.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>


namespace narvalengine {
	class SceneReader {
	private:
		Settings *settings = nullptr;
		Camera *mainCamera = nullptr;
		Scene *scene = nullptr;

	public:
		SceneReader(std::string filePath, bool absolutePath);
		void loadScene(std::string filePath, bool absolutePath);
		glm::vec3 getVec3(rapidjson::Value &v);
		void processMaterial(rapidjson::Value &material);
		void processPrimitives(rapidjson::Value &primitive);
		void processCameraAndRenderer(rapidjson::Value &camera, rapidjson::Value &renderer);
		Camera *getMainCamera();
		Settings *getSettings();
		Scene *getScene();

		SceneReader();
		~SceneReader();
	};
}