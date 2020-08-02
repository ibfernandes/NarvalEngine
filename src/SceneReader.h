#pragma once
#include "ResourceManager.h"
#include "Camera.h"
#include "Settings.h"
#include "Material.h"
#include "GridMaterial.h"
#include "RoughConductorBRDF.h"
#include "MaterialBRDF.h"
#include "Volume.h"
#include "BucketLBVH.h"
#include "OBB.h"
#include "Sphere.h"
#include "DiffuseMaterial.h"
#include "MetalMaterial.h"
#include "DiffuseLight.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

class SceneReader
{
private:
	Settings settings;
	Camera mainCamera;

public:
	SceneReader(std::string filePath) {
		std::string  text = "";
		std::string buffer;
		std::ifstream vc(RESOURCES_DIR + filePath);

		if (!vc) {
			std::cerr << "Couldn't read " << filePath << " JSON file." << std::endl;
			exit(1);
		}

		while (getline(vc, buffer)) {
			text = text + buffer + "\n";
		};

		rapidjson::Document doc;
		doc.Parse(text.c_str());
		assert("ERROR: invalid JSON" && doc.IsObject());

		assert("ERROR: file must contain a version" && doc.HasMember("version"));
		std::string version = doc["version"].GetString();
		
		rapidjson::Value &materials = doc["materials"];
		for (rapidjson::SizeType i = 0; i < materials.Size(); i++) 
			processMaterial(materials[i]);

		rapidjson::Value &primitives = doc["primitives"];
		for (rapidjson::SizeType i = 0; i < primitives.Size(); i++)
			processPrimitives(primitives[i]);
		
		processCameraAndRenderer(doc["camera"], doc["renderer"]);
	}

	glm::vec3 getVec3(rapidjson::Value &v) {
		glm::vec3 vec;
		for (rapidjson::SizeType i = 0; i < v.Size(); i++)
			vec[i] = v[i].GetFloat();
		return vec;
	}

	void processMaterial(rapidjson::Value &material) {
		assert("ERROR: Missing material type" && material.HasMember("type"));
		assert("ERROR: Missing material name" && material.HasMember("name"));

		std::string type = material["type"].GetString();
		std::string name = material["name"].GetString();

		if (type.compare("diffuse") == 0){
			glm::vec3 albedo = getVec3(material["albedo"]);

			ResourceManager::getSelf()->addMaterial(name, new DiffuseMaterial(albedo));
		}
		else if (type.compare("metal") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);

			ResourceManager::getSelf()->addMaterial(name, new MetalMaterial(albedo));
			ResourceManager::getSelf()->getMaterial(name)->hasSpecularLobe = true;

		}else if (type.compare("microfacet") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);
			float roughness = material["roughness"].GetFloat();
			float metallic = material["metallic"].GetFloat();
			//BeckmannBRDF brdf;

			ResourceManager::getSelf()->addMaterial(name, new MaterialBRDF(new RoughConductorBRDF(roughness, metallic, albedo), albedo));
			ResourceManager::getSelf()->getMaterial(name)->hasSpecularLobe = false;

		}else if (type.compare("emitter") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);

			ResourceManager::getSelf()->addMaterial(name, new DiffuseLight(albedo));
		}else if (type.compare("volume") == 0) {
			glm::vec3 scattering = getVec3(material["scattering"]);
			glm::vec3 absorption = getVec3(material["absorption"]);
			std::string phaseFunction = material["phaseFunction"].GetString();
			PhaseFunction pf;
			if (phaseFunction.compare("isotropic") == 0)
				pf = ISOTROPIC;
			else if (phaseFunction.compare("rayleigh") == 0)
				pf = RAYLEIGH;
			else if (phaseFunction.compare("rayleigh") == 0)
				pf = HENYEY_GREENSTEIN;
			else
				assert("ERROR: invalid phase function" && false);

			ResourceManager::getSelf()->loadVDBasTexture3D(name, material["path"].GetString());

			GridMaterial *gm = new GridMaterial(scattering, absorption, pf, name);

			ResourceManager::getSelf()->addMaterial(name, gm);
		}else {
			assert("ERROR: material must have a valid type" && false);
		}
	}

	void processPrimitives(rapidjson::Value &primitive) {
		assert("ERROR: Missing model type" && primitive.HasMember("type"));
		assert("ERROR: Missing model name" && primitive.HasMember("name"));

		std::string name = primitive["name"].GetString();
		std::string type = primitive["type"].GetString();
		glm::vec3 pos = getVec3(primitive["transform"]["position"]);

		if (type.compare("obj") == 0) {
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

		}else if(type.compare("sphere") == 0){
			float radius = primitive["radius"].GetFloat();
			std::string materialName = primitive["materialName"].GetString();
			Sphere *s = new Sphere(pos, radius, ResourceManager::getSelf()->getMaterial(materialName));

			Model *m = new Model();
			m->addGeometry(s);

			ResourceManager::getSelf()->addModel(name, m);
		}else if (type.compare("volume") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			Volume *v = new Volume( (GridMaterial*)ResourceManager::getSelf()->getMaterial(materialName));

			ResourceManager::getSelf()->addModel(name , v);
		}else if (type.compare("box") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

			OBB *obb = new OBB(pos, scale, rotate, ResourceManager::getSelf()->getMaterial(materialName));
			Model *m = new Model();
			m->addGeometry(obb);
			ResourceManager::getSelf()->addModel(name, m);
		}else {
			assert("ERROR: primitive must have a valid type");
		}
	}

	void processCameraAndRenderer(rapidjson::Value &camera, rapidjson::Value &renderer) {
		glm::vec3 position = getVec3(camera["position"]);
		glm::vec3 lookAt = getVec3(camera["lookAt"]);
		glm::vec3 up = getVec3(camera["up"]);
		float speed = camera["speed"].GetFloat();
		float vfov = camera["vfov"].GetFloat();
		float aperture = camera["aperture"].GetFloat();
		glm::ivec2 resolution(renderer["resolution"][0].GetInt(), renderer["resolution"][1].GetInt());
		float focus;

		if (camera.HasMember("autoFocus")) {
			if (camera["autoFocus"].GetBool())
				focus = (position - lookAt).length();
			else
				focus = camera["focus"].GetFloat();
		}else
			focus = camera["focus"].GetFloat();

		mainCamera = Camera(position, lookAt, glm::vec3(0, 1, 0), 45.0f, float(resolution.x) / float(resolution.y), 0.0001f, focus);
		settings.resolution = resolution;
		settings.spp = renderer["spp"].GetInt();
		settings.bounces = renderer["bounces"].GetInt();
		std::string mode = renderer["mode"].GetString();
		if(mode.compare("offline") == 0)
			settings.renderMode = OFFLINE_RENDERING_MODE;
		else if(mode.compare("realtime") == 0)
			settings.renderMode = REALTIME_RENDERING_MODE;
	}

	Camera getMainCamera() {
		return mainCamera;
	}

	Settings getSettings() {
		return settings;
	}

	SceneReader();
	~SceneReader();
};

