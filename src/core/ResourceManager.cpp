#include "core/ResourceManager.h"
#include "primitives/Model.h"

namespace narvalengine {
	ResourceManager *ResourceManager::self = nullptr;

	ResourceManager* ResourceManager::getSelf() {
		if (self == 0) {
			openvdb::initialize();
			self = new ResourceManager();
		}

		return self;
	}

	ResourceManager::ResourceManager() {
	}


	ResourceManager::~ResourceManager() {
		//TODO delete all resources/
	}


	StringID ResourceManager::setModel(std::string name, Model *model) {
		StringID id = genStringID(name.c_str());

		if (models.count(id) > 0) {
			LOG(INFO) << "Model with name " << name << " was already set. Ignoring new insertion and returning previous ID.";
			return id;
		}

		models.insert({ id, model });

		return id;
	}

	StringID ResourceManager::replaceModel(std::string name, Model* model) {
		StringID id = genStringID(name.c_str());

		if (models.count(id) > 0)
			models[id] = model;
		else
			models.insert({ id, model });

		return id;
	}

	StringID ResourceManager::loadModel(std::string name, std::string path, std::string fileName) {
		StringID id = genStringID(name.c_str());

		if (models.count(id) > 0) {
			LOG(WARNING) << "Model with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		std::string finalPath = RESOURCES_DIR + path + fileName;
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(finalPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			LOG(ERROR) << "ASSIMP couldn't load the model " << name << "." << importer.GetErrorString();
			return NE_INVALID_STRING_ID;
		}

		Model* model = new Model(scene, path, "");
		models.insert({ id, model });
		return id;
	}

	StringID ResourceManager::loadModel(std::string name, std::string path, std::string fileName, std::string materialName) {
		StringID id = genStringID(name.c_str());

		if (models.count(id) > 0) {
			LOG(WARNING) << "Model with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		std::string finalPath = RESOURCES_DIR + path + fileName;
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(finalPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			LOG(FATAL) << "ASSIMP couldn't load the model " << name << "." << importer.GetErrorString();
			return NE_INVALID_STRING_ID;
		}

		Model* model = new Model(scene, path, materialName);
		models.insert({ id, model });
		return id;
	}

	Model* ResourceManager::getModel(std::string name) {
		StringID id = genStringID(name.c_str());

		if (models.count(id) == 0) 
			LOG(FATAL) << "Model with name " << name << " does not exist.";

		return models.at(id);
	}

	Model* ResourceManager::getModel(StringID id) {
		if (models.count(id) == 0)
			LOG(FATAL) << "Model with id " << id << " does not exist.";

		return models.at(id);
	}

	StringID ResourceManager::setMaterial(std::string name, Material *material) {
		StringID id = genStringID(name.c_str());

		if (materials.count(id) > 0) {
			LOG(INFO) << "Material with name " << name << " was already set. Ignoring new insertion and returning previous ID.";
			return id;
		}

		materials.insert({ id, material });

		return id;
	}

	StringID ResourceManager::replaceMaterial(std::string name, Material* material) {
		StringID id = genStringID(name.c_str());

		if (materials.count(id) > 0) 
			materials[id] = material;
		else
			materials.insert({ id, material });

		return id;
	}

	Material * ResourceManager::getMaterial(std::string name) {
		if (materials.count(genStringID(name.c_str())) == 0)
			LOG(FATAL) << "Material " << name << " doesn't exist.";
		return materials.at(genStringID(name.c_str()));
	}

	Material* ResourceManager::getMaterial(StringID id) {
		if (materials.count(id) == 0)
			LOG(FATAL) << "Material " << id << " doesn't exist.";
		return materials.at(id);
	}

	StringID ResourceManager::setTexture(std::string name, Texture *t) {
		StringID id = genStringID(name.c_str());
		if (textures.count(id) > 0) {
			LOG(INFO) << "Texture with name " << name << " was already set. Ignoring new insertion and returning previous ID.";
			return id;
		}
		t->resourceID = id;
		textures.insert({ id, t });
		return id;
	}

	StringID ResourceManager::replaceTexture(std::string name, Texture* t) {
		StringID id = genStringID(name.c_str());
		if (textures.count(id) > 0)
			textures[id] = t;
		else
			textures.insert({ id, t });
		return id;
	}

	StringID ResourceManager::loadVDBasTexture(std::string name, std::string path) {
		StringID id = genStringID(name.c_str());

		if (textures.count(id) > 0) {
			LOG(INFO) << "VDB Texture with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		openvdb::io::File file(RESOURCES_DIR + path);
		openvdb::GridBase::Ptr baseGrid;
		file.open();
		for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
			if (nameIter.gridName() == "density") {
				baseGrid = file.readGrid(nameIter.gridName());
				break;
			}
		}
		file.close();

		openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

		openvdb::CoordBBox bb = grid->evalActiveVoxelBoundingBox();
		openvdb::Coord minbb = bb.min();
		openvdb::Coord maxbb = bb.max();
		float resX = abs(minbb.x()) + abs(maxbb.x());
		float resY = abs(minbb.y()) + abs(maxbb.y());
		float resZ = abs(minbb.z()) + abs(maxbb.z());

		//Centered at origin.
		if (minbb.x() < 0 && maxbb.x() >= 0)
			resX += 1;
		if (minbb.y() < 0 && maxbb.y() >= 0)
			resY += 1;
		if (minbb.z() < 0 && maxbb.z() >= 0)
			resZ += 1;

		openvdb::Coord dim(resX, resY, resZ);
		openvdb::Coord originvdb(-abs(bb.min().x()), -abs(bb.min().y()), -abs(bb.min().z()));
		openvdb::tools::Dense<float> dense(dim, originvdb);

		openvdb::tools::copyToDense<openvdb::tools::Dense<float>, openvdb::FloatGrid>(*grid, dense);

		int sizeBytes = sizeof(float) * resZ * resY * resX;
		int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_NEAREST;

		MemoryBuffer mem;
		mem.data = (uint8_t*)dense.data();
		mem.size = sizeBytes;

		Texture* tex = new Texture(resZ, resY, resX, TextureLayout::R32F, flags, mem);
		tex->textureName = TextureName::TEX_1;

		textures.insert({ id, tex });

		return id;
	}

	StringID ResourceManager::loadVolasTexture(std::string name, std::string path) {
		StringID id = genStringID(name.c_str());

		if (textures.count(id) > 0) {
			LOG(INFO) << "VOL Texture with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		std::ifstream file(RESOURCES_DIR + path);
		std::string buffer;

		if (!file) {
			LOG(FATAL) << "Couldn't read the file at " << RESOURCES_DIR + path << ".";
			return NE_INVALID_STRING_ID;
		}

		getline(file, buffer);
		std::string resolutionString = buffer;
		std::string delimitator = " ";
		glm::vec3 resolution;

		int count = 0;
		int start = 0;
		int end = resolutionString.find(delimitator);
		while (end != std::string::npos)
		{
			resolution[count] = std::stof(resolutionString.substr(start, end - start));
			start = end + delimitator.length();
			end = resolutionString.find(delimitator, start);
			count++;
		}

		float* grid = new float[resolution.x * resolution.y * resolution.z];
		std::string gridStringData;
		getline(file, buffer);
		while (file) {
			getline(file, buffer);
			gridStringData = gridStringData + buffer + "\n";
		}
		count = 0;
		start = 0;
		end = gridStringData.find(delimitator);
		while (end != std::string::npos)
		{
			grid[count] = std::stof(gridStringData.substr(start, end - start));
			start = end + delimitator.length();
			end = gridStringData.find(delimitator, start);
			count++;
		}


		int sizeBytes = sizeof(float) * resolution.x * resolution.y * resolution.z;
		int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_NEAREST;

		MemoryBuffer mem;
		mem.data = (uint8_t*)grid;
		mem.size = sizeBytes;

		Texture* tex = new Texture(resolution.x, resolution.y, resolution.z, TextureLayout::R32F, flags, mem);
		tex->textureName = TextureName::TEX_1;

		textures.insert({ id, tex });

		return id;
	}

	StringID ResourceManager::loadTexture(std::string name, std::string path, bool absolutePath) {
		StringID id = genStringID(name.c_str());
		if (!absolutePath)
			path = (RESOURCES_DIR + path);

		if (textures.count(id) > 0) {
			LOG(INFO) << "Texture with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		int width, height, channels;
		unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (data == nullptr) {
			LOG(FATAL) << "Couldn't read the file at " << path << ".";
			return NE_INVALID_STRING_ID;
		}

		uint32_t sizeBytes = sizeof(char) * width * height * 4;
		
		int flags = NE_TEX_SAMPLER_UVW_MIRROR| NE_TEX_SAMPLER_MIN_MAG_NEAREST;
		Texture *texture = new Texture(width, height, RGBA8, flags, { data, sizeBytes });
		texture->resourceID = id;
		textures.insert({ id, texture });
		stbi_image_free(data);

		return id;
	}

	Texture* ResourceManager::getTexture(std::string name) {
		StringID id = genStringID(name.c_str());
		return textures.at(id);
	}

	Texture* ResourceManager::getTexture(StringID id) {
		if (textures.count(id) > 0)
			return textures.at(id);
		else
			return nullptr;
	}

	StringID ResourceManager::loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath) {
		StringID id = genStringID(name.c_str());

		if (shaders.count(id) > 0) {
			LOG(INFO) << "Shader with name " << name << " was already loaded. Ignoring loading again and returning previous ID.";
			return id;
		}

		std::string  vertexCode = "", fragmentCode = "", geometryCode = "";
		std::string buffer;
		std::ifstream vc(RESOURCES_DIR + vertexShaderPath);

		if (!vc) {
			LOG(FATAL) << "Couldn't read the file at " << RESOURCES_DIR + vertexShaderPath << ".";
			return NE_INVALID_STRING_ID;
		}

		while (getline(vc, buffer)) {
			vertexCode = vertexCode + buffer + "\n";
		}

		std::ifstream fc(RESOURCES_DIR + fragmentShaderPath);

		if (!fc) {
			LOG(FATAL) << "Couldn't read the file at " << RESOURCES_DIR + fragmentShaderPath << ".";
			return NE_INVALID_STRING_ID;
		}

		while (fc) {
			getline(fc, buffer);
			fragmentCode = fragmentCode + buffer + "\n";
		}

		Shader* shader = new Shader;
		shader->addSourceCode(vertexCode, fragmentCode);

		shaders.insert({ id, shader });

		return id;
	}

	Shader* ResourceManager::getShader(std::string name) {
		StringID id = genStringID(name.c_str());
		return shaders.at(id);
	}

	Shader* ResourceManager::getShader(StringID id) {
		return shaders.at(id);
	}
}