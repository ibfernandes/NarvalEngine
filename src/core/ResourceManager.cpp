#include "core/ResourceManager.h"
#include "primitives/Model.h"

namespace narvalengine {
	ResourceManager *ResourceManager::self = 0;

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
	}


	StringID ResourceManager::setModel(std::string name, Model *model) {
		StringID id = genStringID(name);

		if (models.count(id) > 0)
			throw "Model with this name already exists";

		models.insert({ id, model });

		return id;
	}

	StringID ResourceManager::replaceModel(std::string name, Model* model) {
		StringID id = genStringID(name);

		if (models.count(id) > 0)
			models[id] = model;
		else
			models.insert({ id, model });

		return id;
	}

	StringID ResourceManager::loadModel(std::string name, std::string path, std::string fileName) {
		StringID id = genStringID(name);

		Model *model = new Model();
		std::string finalPath = RESOURCES_DIR + path + fileName;
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(finalPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "Error loadModel ASSIMP" << importer.GetErrorString() << std::endl;
		}

		model->processScene(scene, path, "");
		if (models.count(id) > 0)
			throw "Model with this name already exists";

		models.insert({ id, model });
		return id;
	}

	StringID ResourceManager::loadModel(std::string name, std::string path, std::string fileName, std::string materialName) {
		StringID id = genStringID(name);

		Model *model = new Model();
		std::string finalPath = RESOURCES_DIR + path + fileName;
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(finalPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "Error loadModel ASSIMP" << importer.GetErrorString() << std::endl;
		}

		model->processScene(scene, path, materialName);

		if (models.count(id) > 0) {
			return id;
			//throw "Model with this name already exists";
			//return NE_INVALID_STRING_ID;
		}

		models.insert({ id, model });
		return id;
	}

	Model* ResourceManager::getModel(std::string name) {
		StringID id = genStringID(name);

		if (models.count(id) == 0)
			throw "Model doesn't exist";

		return models.at(id);
	}

	Model* ResourceManager::getModel(StringID id) {
		if (models.count(id) == 0)
			throw "Model doesn't exist";

		return models.at(id);
	}

	StringID ResourceManager::setMaterial(std::string name, Material *material) {
		StringID id = genStringID(name);

		if (materials.count(id) > 0)
			throw "Material with this name already loaded";

		materials.insert({ id, material });

		return id;
	}

	StringID ResourceManager::replaceMaterial(std::string name, Material* material) {
		StringID id = genStringID(name);

		if (materials.count(id) > 0) 
			materials[id] = material;
		else
			materials.insert({ id, material });

		return id;
	}

	Material * ResourceManager::getMaterial(std::string name) {
		return materials.at(genStringID(name));
	}

	Material* ResourceManager::getMaterial(StringID id) {
		return materials.at(id);
	}

	StringID ResourceManager::setTexture(std::string name, Texture *t) {
		StringID id = genStringID(name);
		textures.insert({ id, t });
		return id;
	}

	StringID ResourceManager::replaceTexture(std::string name, Texture* t) {
		StringID id = genStringID(name);
		if (textures.count(id) > 0)
			textures[id] = t;
		else
			textures.insert({ id, t });
		return id;
	}

	StringID ResourceManager::loadVDBasTexture(std::string name, std::string path) {
		StringID id = genStringID(name);

		if (textures.count(id) > 0)
			return id;
			//throw "VDB Texture with this name already loaded";

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

		//centered at origin
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

		textures.insert({ id, tex });

		return id;
	}

	StringID ResourceManager::loadVolasTexture(std::string name, std::string path) {
		StringID id = genStringID(name);

		if (textures.count(id) > 0)
			return id;

		std::ifstream file(RESOURCES_DIR + path);
		std::string buffer;

		if (!file) {
			std::cerr << "Couldn't read " << path << " vol file." << std::endl;
			exit(1);
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

		textures.insert({ id, tex });

		return id;
	}

	/*
		Supports only:
		JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
		PNG 1/2/4/8/16-bit-per-channel
	*/
	StringID ResourceManager::loadTexture(std::string name, std::string path) {
		StringID id = genStringID(name);

		if (textures.count(id) > 0)
			return id;
			//throw "Texture with this name already loaded";

		int width, height, channels;
		unsigned char *data = stbi_load((RESOURCES_DIR + path).c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if(data == nullptr)
			throw "Error loading texture";

		uint32_t sizeBytes = sizeof(char) * width * height * 4;
		
		int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_NEAREST;

		textures.insert({ id, new Texture(width, height, RGBA8, flags, {data, sizeBytes}) });
		stbi_image_free(data);

		return id;
	}

	Texture* ResourceManager::getTexture(std::string name) {
		StringID id = genStringID(name);
		return textures.at(id);
	}

	Texture* ResourceManager::getTexture(StringID id) {
		if (textures.count(id) > 0)
			return textures.at(id);
		else
			return nullptr;
	}

	StringID ResourceManager::loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath) {
		StringID id = genStringID(name);

		if (shaders.count(id) > 0)
			throw "Shader with this name already loaded";

		std::string  vertexCode = "", fragmentCode = "", geometryCode = "";
		std::string buffer;
		std::ifstream vc(RESOURCES_DIR + vertexShaderPath);

		if (!vc) {
			std::cerr << "Couldn't read " << vertexShaderPath << "vertex shader file." << std::endl;
			exit(1);
		}

		while (getline(vc, buffer)) {
			vertexCode = vertexCode + buffer + "\n";
		}

		std::ifstream fc(RESOURCES_DIR + fragmentShaderPath);

		if (!fc) {
			std::cerr << "Couldn't read fragment shader file." << std::endl;
			exit(1);
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
		StringID id = genStringID(name);
		return shaders.at(id);
	}

	Shader* ResourceManager::getShader(StringID id) {
		return shaders.at(id);
	}
}