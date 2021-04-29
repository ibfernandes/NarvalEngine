#include "io/SceneReader.h"

namespace narvalengine {
	SceneReader::SceneReader() {
	}

	SceneReader::~SceneReader() {
	}

	SceneReader::SceneReader(std::string filePath, bool absolutePath) {
		settings = new Settings();
		mainCamera = new Camera();
		scene = new Scene();

		std::string  text = "";
		std::string buffer;
		std::ifstream vc;
		if(absolutePath)
			vc = std::ifstream(filePath);
		else
			vc = std::ifstream(RESOURCES_DIR + filePath);

		if (!vc) {
			std::cerr << "Couldn't read " << filePath << " JSON file." << std::endl;
			exit(1);
		}

		while (getline(vc, buffer)) {
			text = text + buffer + "\n";
		};
		vc.close();

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
	
	void SceneReader::loadScene(std::string filePath, bool absolutePath) {
		if(settings)
			delete settings;
		if (mainCamera)
			delete mainCamera;
		if (scene)
			delete scene;

		settings = new Settings();
		mainCamera = new Camera();
		scene = new Scene();

		std::string  text = "";
		std::string buffer;
		std::ifstream vc;
		if (absolutePath)
			vc = std::ifstream(filePath);
		else
			vc = std::ifstream(RESOURCES_DIR + filePath);

		if (!vc) {
			std::cerr << "Couldn't read " << filePath << " JSON file." << std::endl;
			exit(1);
		}

		while (getline(vc, buffer)) {
			text = text + buffer + "\n";
		};
		vc.close();

		rapidjson::Document doc;
		doc.Parse(text.c_str());
		assert("ERROR: invalid JSON" && doc.IsObject());

		assert("ERROR: file must contain a version" && doc.HasMember("version"));
		std::string version = doc["version"].GetString();

		rapidjson::Value& materials = doc["materials"];
		for (rapidjson::SizeType i = 0; i < materials.Size(); i++)
			processMaterial(materials[i]);

		rapidjson::Value& primitives = doc["primitives"];
		for (rapidjson::SizeType i = 0; i < primitives.Size(); i++)
			processPrimitives(primitives[i]);

		processCameraAndRenderer(doc["camera"], doc["renderer"]);
	}

	glm::vec3 SceneReader::getVec3(rapidjson::Value &v) {
		glm::vec3 vec;
		for (rapidjson::SizeType i = 0; i < v.Size(); i++)
			vec[i] = v[i].GetFloat();
		return vec;
	}

	void SceneReader::processMaterial(rapidjson::Value &material) {
		//TODO replace asserts
		assert("ERROR: Missing material type" && material.HasMember("type"));
		assert("ERROR: Missing material name" && material.HasMember("name"));

		std::string type = material["type"].GetString();
		std::string name = material["name"].GetString();

		if (type.compare("microfacet") == 0) {
			std::string albedoPath = "";
			glm::vec3 albedo = glm::vec3(1, 0, 1);
			glm::vec3 normalMap = glm::vec3(0, 0, 1.0);

			if (material["albedo"].IsString())
				albedoPath = material["albedo"].GetString();
			else
				albedo = getVec3(material["albedo"]);
			
			if(material.HasMember("normalMap"))
				normalMap = getVec3(material["normalMap"]); //Must be already converted on range from [-1, 1] to [0, 1]

			float roughness = material["roughness"].GetFloat();
			float metallic = material["metallic"].GetFloat();
			int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR;

			Texture *metallicTex;
			metallicTex = new Texture(1, 1, R32F, flags, {(uint8_t*)&metallic, sizeof(float)});
			metallicTex->textureName = TextureName::METALLIC;
			//metallicTex = new Texture(TextureName::METALLIC, TextureChannelFormat::R_METALLIC, glm::ivec2(1, 1), R32F, &metallic);
			ResourceManager::getSelf()->setTexture(name + ".metallic", metallicTex);

			Texture *albedoTex = nullptr;
			//If it should load a texture file
			if (material["albedo"].IsString()) {
				ResourceManager::getSelf()->loadTexture(name + ".albedo", albedoPath);
				albedoTex = ResourceManager::getSelf()->getTexture(name + ".albedo");
				albedoTex->textureName = TextureName::ALBEDO;
			}
			else { //If it is a simple 3 float color
				albedoTex = new Texture(1, 1, RGB32F, flags, { (uint8_t*)&albedo[0], sizeof(float) * 3 });
				albedoTex->textureName = TextureName::ALBEDO;
				//albedoTex = new Texture2D(TextureName::ALBEDO, TextureChannelFormat::RGB_ALBEDO, glm::ivec2(1, 1), RGB32F, &albedo[0]);
				ResourceManager::getSelf()->setTexture(name + ".albedo", albedoTex);
			}

			Texture* roughnessTex;
			roughnessTex = new Texture(1, 1, R32F, flags, { (uint8_t*)&roughness, sizeof(float) });
			roughnessTex->textureName = TextureName::ROUGHNESS;
			ResourceManager::getSelf()->setTexture(name + ".roughness", roughnessTex);

			Texture* normalMapTex;
			normalMapTex = new Texture(1, 1, RGB32F, flags, { (uint8_t*)&normalMap[0], sizeof(float) * 3 });
			normalMapTex->textureName = TextureName::NORMAL_MAP;
			ResourceManager::getSelf()->setTexture(name + ".normal", normalMapTex);


			Material *mat = new Material();
			mat->addTexture(TextureName::ALBEDO, albedoTex);
			mat->addTexture(TextureName::METALLIC, metallicTex);
			mat->addTexture(TextureName::ROUGHNESS, roughnessTex);
			mat->addTexture(TextureName::NORMAL_MAP, normalMapTex);

			GGXDistribution *ggxD = new GGXDistribution();
			ggxD->alpha = roughnessToAlpha(0.65f); //TODO double check
			FresnelSchilck *fresnel = new FresnelSchilck();
			GlossyBSDF *glossybsdf = new GlossyBSDF(ggxD, fresnel);

			mat->bsdf = new BSDF();
			mat->bsdf->addBxdf(glossybsdf);

			ResourceManager::getSelf()->replaceMaterial(name, mat);
		} else if (type.compare("emitter") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);

			DiffuseLight *diffLight = new DiffuseLight();
			diffLight->li = albedo;

			Material *lightMaterial = new Material();
			lightMaterial->light = diffLight;

			ResourceManager::getSelf()->replaceMaterial(name, lightMaterial);
		}else if (type.compare("directionalLight") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);
			glm::vec3 position = getVec3(material["position"]);
			glm::vec3 direction = glm::normalize(glm::vec3(0) - position);

			DirectionalLight* dirLight = new DirectionalLight();
			dirLight->le = albedo;
			dirLight->direction = direction;

			Material* lightMaterial = new Material();
			lightMaterial->light = dirLight;

			ResourceManager::getSelf()->replaceMaterial(name, lightMaterial);
		}else if (type.compare("volume") == 0) {
			glm::vec3 scattering = getVec3(material["scattering"]);
			glm::vec3 absorption = getVec3(material["absorption"]);
			std::string phaseFunction = material["phaseFunction"].GetString();
			PhaseFunction *pf;

			if (phaseFunction.compare("isotropic") == 0)
				pf = new IsotropicPhaseFunction();
			else if (phaseFunction.compare("hg") == 0 || phaseFunction.compare("henyey-greenstein") == 0) {
				float g = material["g"].GetFloat();
				pf = new HG(g);
			}

			Medium *medium;
			if (material.HasMember("path")) {
				std::string path = material["path"].GetString();
				if (path.find(".vdb") != std::string::npos)
					ResourceManager::getSelf()->loadVDBasTexture(name, path);
				else if (path.find(".vol") != std::string::npos)
					ResourceManager::getSelf()->loadVolasTexture(name, path);
				medium = new GridMedia(scattering, absorption, name, material["density"].GetFloat());
			} else {
				medium = new HomogeneousMedia(scattering, absorption, material["density"].GetFloat());
			}

			Material *mediumMaterial = new Material();
			mediumMaterial->medium = medium;
			VolumeBSDF *volumebsdf = new VolumeBSDF(pf);
			mediumMaterial->bsdf = new BSDF();
			mediumMaterial->bsdf->addBxdf(volumebsdf);
			mediumMaterial->addTexture(TextureName::TEX_1, ResourceManager::getSelf()->getTexture(name));
			//TODO falta definir A BSDF

			ResourceManager::getSelf()->replaceMaterial(name, mediumMaterial);
		} else {
			assert("ERROR: material must have a valid type" && false);
		}
	}

	void SceneReader::processPrimitives(rapidjson::Value &primitive) {
		assert("ERROR: Missing model type" && primitive.HasMember("type"));
		assert("ERROR: Missing model name" && primitive.HasMember("name"));

		std::string name = primitive["name"].GetString();
		std::string type = primitive["type"].GetString();
		glm::vec3 pos = getVec3(primitive["transform"]["position"]);

		if (type.compare("obj") == 0) {
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);
			std::string materialName;
			if (primitive.HasMember("materialName"))
				materialName = primitive["materialName"].GetString();
			std::string path = primitive["path"].GetString();

			int count = path.find_last_of("/") + 1;
			std::string fileName = path.substr(count);
			std::string folder = path.substr(0, count);

			StringID modelID;
			if (primitive.HasMember("materialName")){
				Material* material = ResourceManager::getSelf()->getMaterial(materialName);
				modelID = ResourceManager::getSelf()->loadModel(name, folder, fileName, materialName);
			}else {
				modelID = ResourceManager::getSelf()->loadModel(name, folder, fileName);
			}
			
			InstancedModel *instancedModel = new InstancedModel(ResourceManager::getSelf()->getModel(modelID), modelID, getTransform(pos, rotate, scale));
			scene->instancedModels.push_back(instancedModel);
		} 
		else if (type.compare("point") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

			Model* model = new Model();
			model->vertexDataLength = 1;
			model->vertexData = new float[3];
			model->vertexData[0] = pos[0];
			model->vertexData[1] = pos[1];
			model->vertexData[2] = pos[2];

			int numOfIndices = 1; // Only one vertex
			model->faceVertexIndices = new int[numOfIndices];
			model->faceVertexIndicesLength = numOfIndices;
			model->faceVertexIndices[0] = 0;

			Mesh m;
			m.strideLength = 3;
			m.vertexDataPointer = &model->vertexData[0];
			m.vertexDataPointerLength = model->vertexDataLength;
			m.vertexIndicesPointer = &model->faceVertexIndices[0];
			m.vertexIndicesPointerLength = model->faceVertexIndicesLength;
			m.modelMaterialIndex = 0;

			m.vertexLayout.init();
			m.vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			m.vertexLayout.end();

			/*for (Texture* t : ResourceManager::getSelf()->getMaterial(materialName)->textures) {
				if (t->textureName == TextureName::METALLIC) {
					m.textures.push_back({ genStringID(materialName + ".metallic"), "material.metallic" });
				}
				else if (t->textureName == TextureName::ALBEDO) {
					m.textures.push_back({ genStringID(materialName + ".albedo"), "material.diffuse" });
				}
			}*/

			model->meshes.push_back(m);

			Point* point = new Point();
			point->vertexData[0] = &model->vertexData[0];
			Material* material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				model->lights.push_back(point);
				material->light->primitive = point;
			}
			else
				model->primitives.push_back(point);

			model->materials.push_back(material);
			point->material = material;

			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel* instancedModel = new InstancedModel(model, modelID, getTransform(pos, rotate, scale));
			if (material->light != nullptr)
				scene->lights.push_back(instancedModel);
			else
				scene->instancedModels.push_back(instancedModel);

		}
		else if (type.compare("sphere") == 0) {
			float radius = primitive["radius"].GetFloat();
			std::string materialName = primitive["materialName"].GetString();
			Model *model = new Model();
			model->vertexDataLength = 3;
			model->vertexData = new float[3];
			glm::vec3 point1(0, 0, 0);
			model->vertexData[0] = point1[0];
			model->vertexData[1] = point1[1];
			model->vertexData[2] = point1[2];
			model->centralize();

			//TODO: missing mesh?
			Sphere *sphere = new Sphere();
			sphere->vertexData[0] = &model->vertexData[0];
			sphere->radius = radius;
			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				model->lights.push_back(sphere);
				material->light->primitive = sphere;
			} else
				model->primitives.push_back(sphere);

			model->materials.push_back(material);
			sphere->material = material;

			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel *instancedModel = new InstancedModel(model, modelID, getTransform(pos, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));
			if (material->light != nullptr)
				scene->lights.push_back(instancedModel);
			else
				scene->instancedModels.push_back(instancedModel);

		} else if (type.compare("rectangle") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

			Model *model = new Model();
			model->vertexDataLength = 4 * 3 + 4 * 3 + 4 * 3 + 4 * 2; //vert, norm, tan and UV
			model->vertexData = new float[model->vertexDataLength];
			glm::vec3 point1(-0.5f, -0.5f, 0); //0: bottom left
			glm::vec3 point2(0.5f, -0.5f, 0); //1: bottom right
			glm::vec3 point3(0.5f, 0.5f, 0);  //2: top right
			glm::vec3 point4(-0.5f, 0.5f, 0); //3: top left
			glm::vec2 uv1(0, 0);
			glm::vec2 uv2(1, 0);
			glm::vec2 uv3(1, 1);
			glm::vec2 uv4(0, 1);
			glm::vec3 normal(0, 0, -1.0f);
			glm::vec3 tan(1.0f, 0, 0);
			model->vertexData[0] = point1[0];
			model->vertexData[1] = point1[1];
			model->vertexData[2] = point1[2];
			model->vertexData[3] = normal[0];
			model->vertexData[4] = normal[1];
			model->vertexData[5] = normal[2];
			model->vertexData[6] = tan[0];
			model->vertexData[7] = tan[1];
			model->vertexData[8] = tan[2];
			model->vertexData[9] = uv1[0];
			model->vertexData[10] = uv1[1];
			model->vertexData[11] = point2[0];
			model->vertexData[12] = point2[1];
			model->vertexData[13] = point2[2];
			model->vertexData[14] = normal[0];
			model->vertexData[15] = normal[1];
			model->vertexData[16] = normal[2];
			model->vertexData[17] = tan[0];
			model->vertexData[18] = tan[1];
			model->vertexData[19] = tan[2];
			model->vertexData[20] = uv2[0];
			model->vertexData[21] = uv2[1];
			model->vertexData[22] = point3[0];
			model->vertexData[23] = point3[1];
			model->vertexData[24] = point3[2];
			model->vertexData[25] = normal[0];
			model->vertexData[26] = normal[1];
			model->vertexData[27] = normal[2];
			model->vertexData[28] = tan[0];
			model->vertexData[29] = tan[1];
			model->vertexData[30] = tan[2];
			model->vertexData[31] = uv3[0];
			model->vertexData[32] = uv3[1];
			model->vertexData[33] = point4[0];
			model->vertexData[34] = point4[1];
			model->vertexData[35] = point4[2];
			model->vertexData[36] = normal[0];
			model->vertexData[37] = normal[1];
			model->vertexData[38] = normal[2];
			model->vertexData[39] = tan[0];
			model->vertexData[40] = tan[1];
			model->vertexData[41] = tan[2];
			model->vertexData[42] = uv4[0];
			model->vertexData[43] = uv4[1];
			//model->centralize();

			int numOfIndices = 1 * 2 * 3; // 1 face comprised of 2 triangles, each triangle needs 3 indices
			model->faceVertexIndices = new int[numOfIndices];
			model->faceVertexIndicesLength = numOfIndices;
			model->faceVertexIndices[0] = 0;
			model->faceVertexIndices[1] = 1;
			model->faceVertexIndices[2] = 2;
			model->faceVertexIndices[3] = 0;
			model->faceVertexIndices[4] = 2;
			model->faceVertexIndices[5] = 3;

			Mesh m;
			m.strideLength = 11;
			m.vertexDataPointer = &model->vertexData[0];
			m.vertexDataPointerLength = model->vertexDataLength;
			m.vertexIndicesPointer = &model->faceVertexIndices[0];
			m.vertexIndicesPointerLength = model->faceVertexIndicesLength;
			m.modelMaterialIndex = 0;

			m.vertexLayout.init();
			m.vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			m.vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
			m.vertexLayout.add(VertexAttrib::Tangent, VertexAttribType::Float, 3);
			m.vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
			m.vertexLayout.end();

			m.material = ResourceManager::getSelf()->getMaterial(materialName);
			/*for (Texture *t : ResourceManager::getSelf()->getMaterial(materialName)->textures) {
				if (t->textureName == TextureName::METALLIC) {
					m.textures.push_back({ genStringID(materialName + ".metallic"), "material.metallic" });
				}else if (t->textureName == TextureName::ALBEDO) {
					m.textures.push_back({ genStringID(materialName + ".albedo"), "material.diffuse"});
				}
			}*/

			model->meshes.push_back(m);

			Rectangle *rectangle = new Rectangle();
			rectangle->vertexLayout = &model->meshes.at(0).vertexLayout;
			rectangle->vertexData[0] = &model->vertexData[0];
			rectangle->vertexData[1] = &model->vertexData[m.strideLength * 2];
			rectangle->normal = normal;
			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				model->lights.push_back(rectangle);
				material->light->primitive = rectangle;
			} else
				model->primitives.push_back(rectangle);

			model->materials.push_back(material);
			rectangle->material = material;

			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);

			InstancedModel *instancedModel = new InstancedModel(model, modelID, getTransform(pos, rotate, scale));
			
			if (material->light != nullptr)
				scene->lights.push_back(instancedModel);
			else
				scene->instancedModels.push_back(instancedModel);

		}
		else if (type.compare("volume") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

			//TODO Not the best solution.
			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			if (GridMedia *gridMedia = dynamic_cast<GridMedia *>(material->medium)) {
				gridMedia->lbvh->scale = scale;
				gridMedia->lbvh->translate = pos - scale / 2.0f; //centers
			}

			Model *model = new Model();
			model->vertexDataLength = 8 * 3; // 8 points of 3 coordinates each
			model->vertexData = new float[8 * 3];
			glm::vec3 point1f(-0.5f); //0: bottom left
			glm::vec3 point2f(0.5f, point1f.y, point1f.z); //1: bottom right
			glm::vec3 point3f(0.5f, 0.5f, point1f.z); //2: top right
			glm::vec3 point4f(point1f.x, 0.5f, point1f.z); //3: top left
			glm::vec3 point1b(0.5f); //4: top right
			glm::vec3 point2b(-0.5f, point1b.y, point1b.z); //5: top left
			glm::vec3 point3b(-0.5f, -0.5f, point1b.z); //6: bottom left
			glm::vec3 point4b(point1b.x, -0.5f, point1b.z); //7: bottom right
			//front points						 back points
			model->vertexData[0] = point1f[0];	 model->vertexData[12] = point1b[0];
			model->vertexData[1] = point1f[1];	 model->vertexData[13] = point1b[1];
			model->vertexData[2] = point1f[2];	 model->vertexData[14] = point1b[2];
			model->vertexData[3] = point2f[0];	 model->vertexData[15] = point2b[0];
			model->vertexData[4] = point2f[1];	 model->vertexData[16] = point2b[1];
			model->vertexData[5] = point2f[2];	 model->vertexData[17] = point2b[2];
			model->vertexData[6] = point3f[0];	 model->vertexData[18] = point3b[0];
			model->vertexData[7] = point3f[1];	 model->vertexData[19] = point3b[1];
			model->vertexData[8] = point3f[2];	 model->vertexData[20] = point3b[2];
			model->vertexData[9] = point4f[0];	 model->vertexData[21] = point4b[0];
			model->vertexData[10] = point4f[1];	 model->vertexData[22] = point4b[1];
			model->vertexData[11] = point4f[2];	 model->vertexData[23] = point4b[2];

			int numOfIndices = 6 * 2 * 3; // 6 faces comprised of 2 triangles each, each triangle needs 3 indices
			model->faceVertexIndices = new int[numOfIndices];
			model->faceVertexIndicesLength = numOfIndices;
			//front face
			model->faceVertexIndices[0] = 0;
			model->faceVertexIndices[1] = 1;
			model->faceVertexIndices[2] = 3;
			model->faceVertexIndices[3] = 1;
			model->faceVertexIndices[4] = 2;
			model->faceVertexIndices[5] = 3;

			//right face
			model->faceVertexIndices[6] = 1;
			model->faceVertexIndices[7] = 2;
			model->faceVertexIndices[8] = 4;
			model->faceVertexIndices[9] = 4;
			model->faceVertexIndices[10] = 1;
			model->faceVertexIndices[11] = 7;

			//back face
			model->faceVertexIndices[12] = 6;
			model->faceVertexIndices[13] = 7;
			model->faceVertexIndices[14] = 5;
			model->faceVertexIndices[15] = 5;
			model->faceVertexIndices[16] = 4;
			model->faceVertexIndices[17] = 7;

			//left face
			model->faceVertexIndices[18] = 0;
			model->faceVertexIndices[19] = 3;
			model->faceVertexIndices[20] = 6;
			model->faceVertexIndices[21] = 3;
			model->faceVertexIndices[22] = 5;
			model->faceVertexIndices[23] = 6;

			//top face
			model->faceVertexIndices[24] = 3;
			model->faceVertexIndices[25] = 5;
			model->faceVertexIndices[26] = 2;
			model->faceVertexIndices[27] = 2;
			model->faceVertexIndices[28] = 5;
			model->faceVertexIndices[29] = 4;

			//bottom face
			model->faceVertexIndices[30] = 0;
			model->faceVertexIndices[31] = 6;
			model->faceVertexIndices[32] = 1;
			model->faceVertexIndices[33] = 1;
			model->faceVertexIndices[34] = 7;
			model->faceVertexIndices[35] = 6;

			AABB *aabb = new AABB();
			aabb->vertexData[0] = &model->vertexData[0];
			aabb->vertexData[1] = &model->vertexData[12];

			model->materials.push_back(material);
			aabb->material = material;
			model->primitives.push_back(aabb);

			Mesh m;
			m.strideLength = 3;
			m.vertexDataPointer = &model->vertexData[0];
			m.vertexDataPointerLength = model->vertexDataLength;
			m.vertexIndicesPointer = &model->faceVertexIndices[0];
			m.vertexIndicesPointerLength = model->faceVertexIndicesLength;
			m.modelMaterialIndex = 0;

			m.vertexLayout.init();
			m.vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			m.vertexLayout.end();
			m.material = material;
			//m.textures.push_back({genStringID(materialName), "volume"});

			model->meshes.push_back(m);
			
			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel *instancedModel = new InstancedModel(model, modelID, getTransform(pos, rotate, scale));
			scene->instancedModels.push_back(instancedModel);

		} else {
			assert("ERROR: primitive must have a valid type");
		}
	}

	void SceneReader::processCameraAndRenderer(rapidjson::Value &camera, rapidjson::Value &renderer) {
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
		} else
			focus = camera["focus"].GetFloat();

		mainCamera = new Camera(position, lookAt, glm::vec3(0, 1, 0), vfov, float(resolution.x) / float(resolution.y), 0.0001f, focus);
		settings->resolution = resolution;
		settings->spp = renderer["spp"].GetInt();
		settings->bounces = renderer["bounces"].GetInt();
		settings->hdr = renderer["HDR"].GetBool();
		std::string mode = renderer["mode"].GetString();
		if (mode.compare("offline") == 0)
			settings->renderMode = OFFLINE_RENDERING_MODE;
		else if (mode.compare("realtime") == 0)
			settings->renderMode = REALTIME_RENDERING_MODE;
		scene->settings = *settings; //TODO this here is being a source of confusion and problem.
	}

	Scene *SceneReader::getScene() {
		return scene;
	}

	Camera *SceneReader::getMainCamera() {
		return mainCamera;
	}

	Settings *SceneReader::getSettings() {
		return settings;
	}
}