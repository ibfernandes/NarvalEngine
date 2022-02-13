#include "io/SceneReader.h"

namespace narvalengine {
	SceneReader::SceneReader() {
	}

	SceneReader::~SceneReader() {
	}

	void SceneReader::loadScene(std::string filePath, bool absolutePath) {

		scene = new Scene();

		std::string  text = "";
		std::string buffer;
		std::ifstream vc;
		if (absolutePath)
			vc = std::ifstream(filePath);
		else
			vc = std::ifstream(RESOURCES_DIR + filePath);

		if (!vc) {
			if (absolutePath)
				LOG(FATAL) << "Couldn't read " << filePath << ".";
			else
				LOG(FATAL) << "Couldn't read " << RESOURCES_DIR + filePath << ".";
		}

		while (getline(vc, buffer)) 
			text = text + buffer + "\n";
	
		vc.close();

		rapidjson::Document doc;
		doc.Parse(text.c_str());

		if(!doc.IsObject())
			LOG(FATAL) << "Malformatted json file: " << filePath << ". Not a valid json object.";

		if(!doc.HasMember("version"))
			LOG(FATAL) << "Incomplete json file: " << filePath << ". Version is not present.";

		std::string version = doc["version"].GetString();

		rapidjson::Value& materials = doc["materials"];
		for (rapidjson::SizeType i = 0; i < materials.Size(); i++)
			processMaterial(materials[i]);

		rapidjson::Value& primitives = doc["primitives"];
		for (rapidjson::SizeType i = 0; i < primitives.Size(); i++)
			processPrimitives(primitives[i]);

		processCameraAndRenderer(doc["camera"], doc["renderer"]);
	}

	SceneReader::SceneReader(std::string filePath, bool absolutePath) {
		loadScene(filePath, absolutePath);
	}

	glm::vec3 SceneReader::getVec3(rapidjson::Value &v) {
		glm::vec3 vec;
		for (rapidjson::SizeType i = 0; i < v.Size(); i++)
			vec[i] = v[i].GetFloat();
		return vec;
	}

	void SceneReader::processMaterial(rapidjson::Value &material) {
		if(!material.HasMember("name"))
			LOG(FATAL) << "Material is missing the property name.";
		std::string name = material["name"].GetString();

		if(!material.HasMember("type"))
			LOG(FATAL) << "Material " << name << " is missing the property type.";
		std::string type = material["type"].GetString();

		if (type.compare("microfacet") == 0) {
			std::string albedoPath = "";
			glm::vec3 albedo = glm::vec3(1, 0, 1);
			glm::vec3 normalMap = glm::vec3(0, 0, 1.0);

			if (material["albedo"].IsString())
				albedoPath = material["albedo"].GetString();
			else
				albedo = getVec3(material["albedo"]);
			
			if(material.HasMember("normalMap"))
				//Must be already converted from the range [-1, 1] to [0, 1].
				normalMap = getVec3(material["normalMap"]); 

			float roughness = material["roughness"].GetFloat();
			float metallic = material["metallic"].GetFloat();
			int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR;

			Texture *metallicTex;
			metallicTex = new Texture(1, 1, R32F, flags, {(uint8_t*)&metallic, sizeof(float)});
			metallicTex->textureName = TextureName::METALLIC;
			ResourceManager::getSelf()->setTexture(name + ".metallic", metallicTex);

			Texture *albedoTex = nullptr;
			//If it should load from a texture file.
			if (material["albedo"].IsString()) {
				ResourceManager::getSelf()->loadTexture(name + ".albedo", albedoPath);
				albedoTex = ResourceManager::getSelf()->getTexture(name + ".albedo");
				albedoTex->textureName = TextureName::ALBEDO;
			}else { 
				//If it is a simple 3 float color.
				albedoTex = new Texture(1, 1, RGB32F, flags, { (uint8_t*)&albedo[0], sizeof(float) * 3 });
				albedoTex->textureName = TextureName::ALBEDO;
				ResourceManager::getSelf()->setTexture(name + ".albedo", albedoTex);
			}

			Texture* roughnessTex;
			roughnessTex = new Texture(1, 1, R32F, flags, { (uint8_t*)&roughness, sizeof(float) });
			roughnessTex->textureName = TextureName::ROUGHNESS;
			ResourceManager::getSelf()->setTexture(name + ".roughness", roughnessTex);

			Texture* normalMapTex;
			if (material.HasMember("normalMap")) {
				normalMapTex = new Texture(1, 1, RGB32F, flags, { (uint8_t*)&normalMap[0], sizeof(float) * 3 });
				normalMapTex->textureName = TextureName::NORMAL;
				ResourceManager::getSelf()->setTexture(name + ".normal", normalMapTex);
			}


			Material *mat = new Material();
			mat->addTexture(TextureName::ALBEDO, albedoTex);
			mat->addTexture(TextureName::METALLIC, metallicTex);
			mat->addTexture(TextureName::ROUGHNESS, roughnessTex);
			if (material.HasMember("normalMap"))
				mat->addTexture(TextureName::NORMAL, normalMapTex);

			GGXDistribution *ggxD = new GGXDistribution();
			ggxD->alpha = roughnessToAlpha(roughness);
			FresnelSchilck *fresnel = new FresnelSchilck();
			GlossyBSDF *glossybsdf = new GlossyBSDF(ggxD, fresnel);

			mat->bsdf = new BSDF();
			mat->bsdf->addBxdf(glossybsdf);

			ResourceManager::getSelf()->replaceMaterial(name, mat);
		} else if (type.compare("emitter") == 0) {
			glm::vec3 albedo = getVec3(material["albedo"]);

			DiffuseLight *diffLight = new DiffuseLight();
			diffLight->li = albedo;

			Texture *albedoTex = new Texture(1, 1, RGB32F, NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR, { &albedo[0], sizeof(float) * 3 });
			albedoTex->textureName = TextureName::ALBEDO;
			ResourceManager::getSelf()->setTexture(name + ".albedo", albedoTex);

			Material *lightMaterial = new Material();
			lightMaterial->light = diffLight;
			lightMaterial->addTexture(TextureName::ALBEDO, albedoTex);

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
		}else if (type.compare("infiniteAreaLight") == 0) {
			Texture* tex;

			if (material.HasMember("path")) {
				std::string path = material["path"].GetString();

				ResourceManager::getSelf()->loadTexture(name + ".tex_1", path);
				tex = ResourceManager::getSelf()->getTexture(name + ".tex_1");
				tex->textureName = TextureName::TEX_1;
			}

			InfiniteAreaLight* infLight = new InfiniteAreaLight(tex);

			Material* lightMaterial = new Material();
			lightMaterial->light = infLight;
			lightMaterial->addTexture(TextureName::TEX_1, ResourceManager::getSelf()->getTexture(name + ".tex_1"));

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
				medium = new GridMedia(scattering, absorption, ResourceManager::getSelf()->getTexture(name), material["density"].GetFloat());
			} else {
				medium = new HomogeneousMedia(scattering, absorption, material["density"].GetFloat());
			}

			Material *mediumMaterial = new Material();
			mediumMaterial->medium = medium;
			VolumeBSDF *volumebsdf = new VolumeBSDF(pf);
			mediumMaterial->bsdf = new BSDF();
			mediumMaterial->bsdf->addBxdf(volumebsdf);
			mediumMaterial->addTexture(TextureName::TEX_1, ResourceManager::getSelf()->getTexture(name));

			ResourceManager::getSelf()->replaceMaterial(name, mediumMaterial);
		} else 
			LOG(FATAL) << "Invalid material type while reading scene from json file.";
	}

	void SceneReader::processPrimitives(rapidjson::Value &primitive) {
		if (!primitive.HasMember("name"))
			LOG(FATAL) << "Primitive is missing the property name.";
		std::string name = primitive["name"].GetString();

		if (!primitive.HasMember("type"))
			LOG(FATAL) << "Primitive " << name << " is missing the property type.";
		std::string type = primitive["type"].GetString();

		glm::vec3 pos = getVec3(primitive["transform"]["position"]);
		bool collision = true;
		if(primitive.HasMember("collision"))
			collision = primitive["collision"].GetBool();

		Model* model;
		std::vector<Primitive*> primitives;
		std::vector<Primitive*> lights;
		std::vector<Material*> materials;
		std::vector<Mesh> meshes;
		VertexLayout vertexLayout;

		if (type.compare("obj") == 0 || type.compare("gltf") == 0) {
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

			vertexLayout.init();
			vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			vertexLayout.end();

			const uint32_t vertexDataLength = 3;
			float* vertexData = new float[vertexDataLength];
			MemoryBuffer vertexDataMem = {(uint8_t*)vertexData, vertexDataLength * sizeof(float)};

			const uint32_t indexDataLength = 1;
			uint32_t* indexData = new uint32_t[indexDataLength];
			MemoryBuffer indexDataMem = {(uint8_t*)indexData, indexDataLength * sizeof(uint32_t) };

			vertexData[0] = pos[0];
			vertexData[1] = pos[1];
			vertexData[2] = pos[2];

			indexData[0] = 0;

			Mesh mesh;
			mesh.strideLength = 3;
			mesh.vertexDataPointer = &vertexData[0];
			mesh.vertexDataPointerLength = vertexDataLength;
			mesh.vertexIndicesPointer = &indexData[0];
			mesh.vertexIndicesPointerLength = indexDataLength;
			mesh.modelMaterialIndex = 0;
			mesh.vertexLayout = vertexLayout;
			meshes.push_back(mesh);

			Point* point = new Point[1];
			point[0].vertexData[0] = &vertexData[0];

			Material* material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				lights.push_back(&point[0]);
				material->light->primitive = &point[0];
			}else
				primitives.push_back(&point[0]);

			materials.push_back(material);
			point->material = material;

			model = new Model(vertexDataMem, indexDataMem,
				primitives.size() > 0 ? MemoryBuffer{(uint8_t*)&point[0], sizeof(Point)} : MemoryBuffer{},
				primitives,
				lights.size() > 0 ? MemoryBuffer{(uint8_t*)&point[0], sizeof(Point)} : MemoryBuffer{},
				lights,
				materials,
				meshes,
				vertexLayout
			);

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

			vertexLayout.init();
			vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			vertexLayout.end();

			const uint32_t vertexDataLength = 3;
			float* vertexData = new float[vertexDataLength];
			MemoryBuffer vertexDataMem = { (uint8_t*)vertexData, vertexDataLength * sizeof(float) };

			const uint32_t indexDataLength = 1;
			uint32_t* indexData = new uint32_t[indexDataLength];
			MemoryBuffer indexDataMem = { (uint8_t*)indexData, indexDataLength * sizeof(uint32_t) };

			vertexData[0] = 0;
			vertexData[1] = 0;
			vertexData[2] = 0;

			indexData[0] = 0;

			Sphere *sphere = new Sphere[1];
			sphere[0].vertexData[0] = &vertexData[0];
			sphere[0].radius = radius;

			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				lights.push_back(&sphere[0]);
				material->light->primitive = &sphere[0];
			} else
				primitives.push_back(&sphere[0]);
			materials.push_back(material);
			sphere[0].material = material;

			model = new Model(vertexDataMem, indexDataMem,
				primitives.size() > 0 ? MemoryBuffer{ (uint8_t*)&sphere[0], sizeof(Sphere) } : MemoryBuffer{},
				primitives,
				lights.size() > 0 ? MemoryBuffer{ (uint8_t*)&sphere[0], sizeof(Sphere) } : MemoryBuffer{},
				lights,
				materials,
				meshes,
				vertexLayout
			);

			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel *instancedModel = new InstancedModel(model, modelID, getTransform(pos, glm::vec3(0, 0, 0), glm::vec3(1, 1, 1)));
			instancedModel->isCollisionEnabled = collision;
			if (material->light != nullptr)
				scene->lights.push_back(instancedModel);
			else
				scene->instancedModels.push_back(instancedModel);

		} else if (type.compare("rectangle") == 0) {
			std::string materialName = primitive["materialName"].GetString();
			glm::vec3 scale = getVec3(primitive["transform"]["scale"]);
			glm::vec3 rotate = getVec3(primitive["transform"]["rotation"]);

			vertexLayout.init();
			vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::Tangent, VertexAttribType::Float, 3);
			vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
			vertexLayout.end();

			//Position, Normal, Tangent and UV
			const uint32_t vertexDataLength = 4 * 3 + 4 * 3 + 4 * 3 + 4 * 2;
			float* vertexData = new float[vertexDataLength];
			MemoryBuffer vertexDataMem = { (uint8_t*)vertexData, vertexDataLength * sizeof(float) };

			//2 triangles, each triangle requiring 3 indices
			const uint32_t indexDataLength = 2 * 3; 
			uint32_t* indexData = new uint32_t[indexDataLength];
			MemoryBuffer indexDataMem = { (uint8_t*)indexData, indexDataLength * sizeof(uint32_t) };

			glm::vec3 point1(-0.5f, -0.5f, 0);	//0: bottom left
			glm::vec3 point2(0.5f, -0.5f, 0);	//1: bottom right
			glm::vec3 point3(0.5f, 0.5f, 0);	//2: top right
			glm::vec3 point4(-0.5f, 0.5f, 0);	//3: top left
			glm::vec2 uv1(0, 0);
			glm::vec2 uv2(1, 0);
			glm::vec2 uv3(1, 1);
			glm::vec2 uv4(0, 1);
			glm::vec3 normal(0, 0, -1.0f);
			glm::vec3 tan(1.0f, 0, 0);

			vertexData[0] = point1[0];
			vertexData[1] = point1[1];
			vertexData[2] = point1[2];
			vertexData[3] = normal[0];
			vertexData[4] = normal[1];
			vertexData[5] = normal[2];
			vertexData[6] = tan[0];
			vertexData[7] = tan[1];
			vertexData[8] = tan[2];
			vertexData[9] = uv1[0];
			vertexData[10] = uv1[1];
			vertexData[11] = point2[0];
			vertexData[12] = point2[1];
			vertexData[13] = point2[2];
			vertexData[14] = normal[0];
			vertexData[15] = normal[1];
			vertexData[16] = normal[2];
			vertexData[17] = tan[0];
			vertexData[18] = tan[1];
			vertexData[19] = tan[2];
			vertexData[20] = uv2[0];
			vertexData[21] = uv2[1];
			vertexData[22] = point3[0];
			vertexData[23] = point3[1];
			vertexData[24] = point3[2];
			vertexData[25] = normal[0];
			vertexData[26] = normal[1];
			vertexData[27] = normal[2];
			vertexData[28] = tan[0];
			vertexData[29] = tan[1];
			vertexData[30] = tan[2];
			vertexData[31] = uv3[0];
			vertexData[32] = uv3[1];
			vertexData[33] = point4[0];
			vertexData[34] = point4[1];
			vertexData[35] = point4[2];
			vertexData[36] = normal[0];
			vertexData[37] = normal[1];
			vertexData[38] = normal[2];
			vertexData[39] = tan[0];
			vertexData[40] = tan[1];
			vertexData[41] = tan[2];
			vertexData[42] = uv4[0];
			vertexData[43] = uv4[1];

			indexData[0] = 0;
			indexData[1] = 1;
			indexData[2] = 2;
			indexData[3] = 0;
			indexData[4] = 2;
			indexData[5] = 3;

			Mesh mesh;
			mesh.strideLength = 11;
			mesh.vertexDataPointer = &vertexData[0];
			mesh.vertexDataPointerLength = vertexDataLength;
			mesh.vertexIndicesPointer = &indexData[0];
			mesh.vertexIndicesPointerLength = indexDataLength;
			mesh.modelMaterialIndex = 0;
			mesh.vertexLayout = vertexLayout;

			Rectangle *rectangle = new Rectangle[1];
			rectangle[0].vertexData[0] = &vertexData[0];
			rectangle[0].vertexData[1] = &vertexData[mesh.strideLength * 2];
			rectangle[0].normal = normal;

			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			mesh.material = material;
			rectangle->material = material;
			if (material->light != nullptr) {
				lights.push_back(&rectangle[0]);
				material->light->primitive = &rectangle[0];
			} else
				primitives.push_back(&rectangle[0]);

			materials.push_back(material);
			meshes.push_back(mesh);

			model = new Model(vertexDataMem, indexDataMem,
				primitives.size() > 0 ? MemoryBuffer{ (uint8_t*)&rectangle[0], sizeof(Rectangle) } : MemoryBuffer{},
				primitives,
				lights.size() > 0 ? MemoryBuffer{ (uint8_t*)&rectangle[0], sizeof(Rectangle) } : MemoryBuffer{},
				lights,
				materials,
				meshes,
				vertexLayout
			);
			rectangle[0].vertexLayout = &model->vertexLayout;

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

			VertexLayout vertexLayout;
			vertexLayout.init();
			vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			vertexLayout.end();

			//8 Vertices of glm::vec3
			const uint32_t vertexDataLength = 8 * 3;
			float* vertexData = new float[vertexDataLength];
			MemoryBuffer vertexDataMem = { (uint8_t*)vertexData, vertexDataLength * sizeof(float) };

			// 6 faces comprised of 2 triangles each, each triangle needs 3 indices
			const uint32_t indexDataLength = 6 * 2 * 3;
			uint32_t* indexData = new uint32_t[indexDataLength];
			MemoryBuffer indexDataMem = { (uint8_t*)indexData, indexDataLength * sizeof(uint32_t) };

			Material *material = ResourceManager::getSelf()->getMaterial(materialName);
			if (GridMedia *gridMedia = dynamic_cast<GridMedia *>(material->medium)) {
				//gridMedia->lbvh->scale = scale;
				//gridMedia->lbvh->translate = pos - scale / 2.0f; //centralizes the LBVH
			}

			glm::vec3 point1f(-0.5f);						//0: bottom left
			glm::vec3 point2f(0.5f, point1f.y, point1f.z);	//1: bottom right
			glm::vec3 point3f(0.5f, 0.5f, point1f.z);		//2: top right
			glm::vec3 point4f(point1f.x, 0.5f, point1f.z);	//3: top left
			glm::vec3 point1b(0.5f);						//4: top right
			glm::vec3 point2b(-0.5f, point1b.y, point1b.z); //5: top left
			glm::vec3 point3b(-0.5f, -0.5f, point1b.z);		//6: bottom left
			glm::vec3 point4b(point1b.x, -0.5f, point1b.z);	//7: bottom right

			//front points				  back points
			vertexData[0] = point1f[0];	  vertexData[12] = point1b[0];
			vertexData[1] = point1f[1];	  vertexData[13] = point1b[1];
			vertexData[2] = point1f[2];	  vertexData[14] = point1b[2];
			vertexData[3] = point2f[0];	  vertexData[15] = point2b[0];
			vertexData[4] = point2f[1];	  vertexData[16] = point2b[1];
			vertexData[5] = point2f[2];	  vertexData[17] = point2b[2];
			vertexData[6] = point3f[0];	  vertexData[18] = point3b[0];
			vertexData[7] = point3f[1];	  vertexData[19] = point3b[1];
			vertexData[8] = point3f[2];	  vertexData[20] = point3b[2];
			vertexData[9] = point4f[0];	  vertexData[21] = point4b[0];
			vertexData[10] = point4f[1];  vertexData[22] = point4b[1];
			vertexData[11] = point4f[2];  vertexData[23] = point4b[2];

			//front face
			indexData[0] = 0;
			indexData[1] = 1;
			indexData[2] = 3;
			indexData[3] = 1;
			indexData[4] = 2;
			indexData[5] = 3;

			//right face
			indexData[6] = 1;
			indexData[7] = 2;
			indexData[8] = 4;
			indexData[9] = 4;
			indexData[10] = 1;
			indexData[11] = 7;

			//back face
			indexData[12] = 6;
			indexData[13] = 7;
			indexData[14] = 5;
			indexData[15] = 5;
			indexData[16] = 4;
			indexData[17] = 7;

			//left face
			indexData[18] = 0;
			indexData[19] = 3;
			indexData[20] = 6;
			indexData[21] = 3;
			indexData[22] = 5;
			indexData[23] = 6;

			//top face
			indexData[24] = 3;
			indexData[25] = 5;
			indexData[26] = 2;
			indexData[27] = 2;
			indexData[28] = 5;
			indexData[29] = 4;

			//bottom face
			indexData[30] = 0;
			indexData[31] = 6;
			indexData[32] = 1;
			indexData[33] = 1;
			indexData[34] = 7;
			indexData[35] = 6;

			AABB *aabb = new AABB[1];
			aabb[0].vertexData[0] = &vertexData[0];
			aabb[0].vertexData[1] = &vertexData[12];
			aabb[0].material = material;

			materials.push_back(material);
			primitives.push_back(&aabb[0]);

			Mesh mesh;
			mesh.strideLength = 3;
			mesh.vertexDataPointer = &vertexData[0];
			mesh.vertexDataPointerLength = vertexDataLength;
			mesh.vertexIndicesPointer = &indexData[0];
			mesh.vertexIndicesPointerLength = indexDataLength;
			mesh.modelMaterialIndex = 0;
			mesh.vertexLayout = vertexLayout;
			mesh.material = material;

			meshes.push_back(mesh);

			model = new Model(vertexDataMem, indexDataMem,
				MemoryBuffer{ (uint8_t*)&aabb[0], sizeof(AABB) },
				primitives,
				MemoryBuffer{},
				lights,
				materials,
				meshes,
				vertexLayout
			);
			
			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel *instancedModel = new InstancedModel(model, modelID, getTransform(pos, rotate, scale));
			scene->instancedModels.push_back(instancedModel);

		} else
			LOG(FATAL) << "Invalid primitive type while reading scene from json file.";
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

		mainCamera = Camera(position, lookAt, glm::vec3(0, 1, 0), vfov, float(resolution.x) / float(resolution.y), 0.0001f, focus);
		settings.resolution = resolution;
		settings.spp = renderer["spp"].GetInt();
		settings.bounces = renderer["bounces"].GetInt();
		settings.hdr = renderer["HDR"].GetBool();
		std::string mode = renderer["mode"].GetString();
		scene->settings = settings;
	}

	Scene *SceneReader::getScene() {
		return scene;
	}

	Camera SceneReader::getMainCamera() {
		return mainCamera;
	}

	SceneSettings SceneReader::getSettings() {
		return settings;
	}
}