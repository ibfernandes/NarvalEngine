#include "ResourceManager.h"
#include "Model.h"

ResourceManager *ResourceManager::self = 0;

ResourceManager* ResourceManager::getSelf(){
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


void ResourceManager::addModel(std::string name, Model model) {

	if (models.count(name) > 0)
		return;

	models.insert({ name, model });
}

Model ResourceManager::loadModel(std::string name, std::string path, std::string fileName) {
	Model model;
	std::string finalPath = RESOURCES_DIR + path + fileName;
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(finalPath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "Error loadModel ASSIMP" << importer.GetErrorString() << std::endl;
	}

	model.processNode(scene->mRootNode, scene, path);

	models.insert({ name, model });
	return model;
}

Model ResourceManager::getModel(std::string name) {
	return models.at(name);
}

Texture3D ResourceManager::getTexture3D(std::string name) {
	return textures3D.at(name);
}

Texture3D ResourceManager::loadVDBasTexture3D(std::string name, std::string path) {
	if (textures3D.count(name) > 0)
		return textures3D.at(name);

	Texture3D cloud;

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
	float resX = abs(bb.min().x()) + abs(bb.max().x());
	float resY = abs(bb.min().y()) + abs(bb.max().y());
	float resZ = abs(bb.min().z()) + abs(bb.max().z());

	openvdb::Coord dim(resX, resY , resZ);
	openvdb::Coord originvdb(-abs(bb.min().x()), -abs(bb.min().y()), -abs(bb.min().z()));
	openvdb::tools::Dense<float> dense(dim, originvdb);

	openvdb::tools::copyToDense<openvdb::tools::Dense<float>, openvdb::FloatGrid>(*grid, dense);

	cloud.generateWithData(resZ, resY , resX, 1, dense.data());
	textures3D.insert({ name, cloud });

	return textures3D.at(name);
}

Texture2D ResourceManager::getTexture2D(std::string name) {
	return textures2D.at(name);
}

void ResourceManager::setTexture2D(std::string name, Texture2D t){
	textures2D.insert({name, t});
}

/*
	Supports only:
	JPEG baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib)
	PNG 1/2/4/8/16-bit-per-channel
*/
Texture2D ResourceManager::loadTexture2D(std::string name, std::string path) {
	if (textures2D.count(name) > 0)
		return textures2D.at(name);

	int width, height, channels;
	unsigned char *data = stbi_load((RESOURCES_DIR + path).c_str(), &width, &height, &channels, STBI_rgb_alpha);
	Texture2D tex;
	tex.generateWithData(width, height, 4, data);
	textures2D.insert({ name, tex });
	stbi_image_free(data);

	return textures2D.at(name);
}

Shader ResourceManager::getShader(std::string name) {
	return shaders.at(name);
}

Shader ResourceManager::loadShader(std::string name, std::string vertexShaderPath, std::string fragmentShaderPath,
	std::string geometryShaderPath) {

	if (shaders.count(name) > 0)
		return shaders.at(name);

	shaders.insert({ name, loadShaderFromFile(vertexShaderPath, fragmentShaderPath, geometryShaderPath) });

	return shaders.at(name);
}

Shader ResourceManager::loadShaderFromFile(std::string vertexShaderPath, std::string fragmentShaderPath, std::string geometryShaderPath) {
	std::string  vertexCode = "", fragmentCode = "", geometryCode = "";
	std::string buffer;
	std::ifstream vc(RESOURCES_DIR + vertexShaderPath);

	if (!vc) {
		std::cerr << "Couldn't read " << vertexShaderPath << "vertex shader file." << std::endl;
		exit(1);
	}

	while (vc) {
		getline(vc, buffer);
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
	shader->compile(vertexCode, fragmentCode, (geometryCode.empty()) ? "" : geometryCode);
	return *shader;
}