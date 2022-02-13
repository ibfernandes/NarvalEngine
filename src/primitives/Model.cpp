#include "primitives/Model.h"

namespace narvalengine {
	Model::Model(const aiScene* scene, std::string path, std::string materialName) {
		processScene(scene, path, materialName);
		bvh.init(this);
	}

	Model::Model(MemoryBuffer vertexData, MemoryBuffer indexData,
		MemoryBuffer memoryBufferPrimitives,
		std::vector<Primitive*> primitives,
		MemoryBuffer memoryBufferLights,
		std::vector<Primitive*> lights,
		std::vector<Material*> materials,
		std::vector<Mesh> meshes,
		VertexLayout vertexLayout) {

		this->vertexData = (float*)vertexData.data;
		this->vertexDataLength = vertexData.size / sizeof(float);

		this->indexData = (uint32_t*)indexData.data;
		this->indexDataLength = indexData.size / sizeof(uint32_t);

		this->memoryBufferPrimitives = memoryBufferPrimitives;
		this->primitives = primitives;

		this->memoryBufferLights = memoryBufferLights;
		this->lights = lights;

		this->materials = materials;
		this->meshes = meshes;

		this->vertexLayout = vertexLayout;
	}

	Model::Model(MemoryBuffer vertexDataMB, MemoryBuffer indexDataMB, VertexLayout vertexLayout) {
		this->vertexData = (float*)malloc(vertexDataMB.size);
		memcpy(this->vertexData, vertexDataMB.data, vertexDataMB.size);

		this->indexData = (uint32_t*)malloc(indexDataMB.size);
		memcpy(this->indexData, indexDataMB.data, indexDataMB.size);

		this->vertexLayout = vertexLayout;

		uint32_t nIndices = indexDataMB.size / sizeof(uint32_t);
		uint32_t nVertices = 0;
		if (vertexLayout.contains(VertexAttrib::Position))
			nVertices = vertexDataMB.size / vertexLayout.getVertexAttribTypeSize(VertexAttrib::Position, RendererAPIName::CPU);
		this->vertexDataLength = nVertices;
		nVertices = nVertices / vertexLayout.qtt[VertexAttrib::Position];
		
		int nTriangles = nIndices / 3;
		primitives.resize(nTriangles);

		//We create a memory buffer to allocate all triangles so we force the memory to be sequentially allocated.
		memoryBufferPrimitives.size = sizeof(Triangle) * nTriangles;
		memoryBufferPrimitives.data = new uint8_t[sizeof(Triangle) * nTriangles];

		//Each triangle is made of three indices.
		for (int i = 0; i < nIndices; i += 3) {
			Triangle* t = new (&((Triangle*)(memoryBufferPrimitives.data))[(i/3) * sizeof(Triangle)])
				Triangle(&vertexData[indexData[i] * 3],
					&vertexData[indexData[i + 1] * 3],
					&vertexData[indexData[i + 2] * 3]);
			primitives[i/3] = t;

			//Each index points to a vertex.
			//Compare the three vertex points agaisnt the global min and max to generate the AABB later.
			for (int k = 0; k < 3; k++) {
				glm::vec3 vertex = glm::vec3(vertexData[indexData[i + k] * 3],
					vertexData[indexData[i + k] * 3 + 1],
					vertexData[indexData[i + k] * 3 + 2]);
				aabbMin = glm::min(vertex, aabbMin);
				aabbMax = glm::max(vertex, aabbMax);
			}
		}

		boundingBox = AABB(&aabbMin[0], &aabbMax[0]);
	}

	Model::~Model() {
		delete []vertexData;
		delete []indexData;

		if(primitives.size() > 0)
			delete []memoryBufferPrimitives.data;
		if (lights.size() > 0)
			delete []memoryBufferLights.data;

		primitives.clear();
		lights.clear();
	}

	bool Model::assimpHasMaterial(aiMaterial* mat, aiTextureType type) {
		return (mat->GetTextureCount(type) > 0);
	}

	std::vector<TextureInfo> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName, TextureName texName, const aiScene* scene) {
		std::vector<TextureInfo> loadingTextures;

		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
			aiString texture_file;
			mat->Get(AI_MATKEY_TEXTURE(type, i), texture_file);

			aiString str;
			mat->GetTexture(type, i, &str);
			std::string string = str.C_Str();
			string = string.substr(string.find_last_of("\\") + 1);
			StringID texID = NE_INVALID_STRING_ID;

			//If the texture is Embedded within the model format.
			if (const aiTexture* texture = scene->GetEmbeddedTexture(texture_file.C_Str())) {
				//Implement
			}else {
				texID = ResourceManager::getSelf()->loadTexture(string, relativePath + string);
				ResourceManager::getSelf()->getTexture(texID)->textureName = texName;
			}

			TextureInfo ti;
			ti.texID = texID;
			ti.texName = texName;
			ti.bindName = typeName;
			ti.rmTexName = string;
			loadingTextures.push_back(ti);
		}
		return loadingTextures;
	}

	std::vector<TextureInfo> Model::loadMaterialTextures(Material* m, std::string materialName) {
		std::vector<TextureInfo> loadingTextures;

		for (Texture* t : m->textures) {
			if (t == nullptr)
				continue;

			TextureInfo ti;
			if (t->textureName == ALBEDO) {
				ti.texID = genStringID(std::string(materialName + ".albedo").c_str());
				ti.texName = TextureName::ALBEDO;
				ti.bindName = "material.diffuse";
			}else if (t->textureName == METALLIC) {
				ti.texID = genStringID(std::string(materialName + ".metallic").c_str());
				ti.texName = TextureName::METALLIC;
				ti.bindName = "material.metallic";
			}else if (t->textureName == ROUGHNESS) {
				ti.texID = genStringID(std::string(materialName + ".roughness").c_str());
				ti.texName = TextureName::ROUGHNESS;
				ti.bindName = "material.roughness";
			}else {
				LOG(WARNING) << "The texture " << magic_enum::enum_name(t->textureName) << " is not yet supported when loading material textures.";
				continue;
			}
			loadingTextures.push_back(ti);
		}

		return loadingTextures;
	}

	Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, unsigned int vertexStartIndex, unsigned int indicesStartIndex, Material* m, std::string materialName) {
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			glm::vec3 v(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			aabbMin = glm::min(aabbMin, v);
			aabbMax = glm::max(aabbMax, v);

			this->vertexData[vertexStartIndex + i * stride] = mesh->mVertices[i].x;
			this->vertexData[vertexStartIndex + i * stride + 1] = mesh->mVertices[i].y;
			this->vertexData[vertexStartIndex + i * stride + 2] = mesh->mVertices[i].z;

			// normals
			this->vertexData[vertexStartIndex + i * stride + 3] = mesh->mNormals[i].x;
			this->vertexData[vertexStartIndex + i * stride + 4] = mesh->mNormals[i].y;
			this->vertexData[vertexStartIndex + i * stride + 5] = mesh->mNormals[i].z;

			//tangent
			this->vertexData[vertexStartIndex + i * stride + 6] = mesh->mTangents[i].x;
			this->vertexData[vertexStartIndex + i * stride + 7] = mesh->mTangents[i].y;
			this->vertexData[vertexStartIndex + i * stride + 8] = mesh->mTangents[i].z;

			//texture coordinates (UV)
			if (mesh->mTextureCoords[0]) {
				//For whatever reason, Assimp sometimes process negative UVs.
				vertexData[vertexStartIndex + i * stride + 9] = mesh->mTextureCoords[0][i].x;
				vertexData[vertexStartIndex + i * stride + 10] = mesh->mTextureCoords[0][i].y;
			}
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		//In order to correctly import OBJ materials for PBR, the following definitions must be met:
		// Kd = Albedo
		// Ks = Roughness
		// Ns = Metalness
		// Ka = AO
		// Bump = Normal
		std::vector<TextureInfo> textures;

		// 1. Diffuse map (Albedo)
		std::vector<TextureInfo> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "material.diffuse", TextureName::ALBEDO, scene);
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		
		// 2. Specular map (Roughness)
		std::vector<TextureInfo> specularMaps;
		if (assimpHasMaterial(material, aiTextureType_SHININESS))
			specularMaps  = loadMaterialTextures(material, aiTextureType_SPECULAR, "material.roughness", TextureName::ROUGHNESS, scene);
		else if (assimpHasMaterial(material, aiTextureType_DIFFUSE_ROUGHNESS))
			specularMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "material.roughness", TextureName::ROUGHNESS, scene);
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		
		// 3. Metalness map
		std::vector<TextureInfo> metallicMaps;
		if (assimpHasMaterial(material, aiTextureType_SHININESS))
			metallicMaps = loadMaterialTextures(material, aiTextureType_SHININESS, "material.metallic", TextureName::METALLIC, scene);
		else if (assimpHasMaterial(material, aiTextureType_METALNESS))
			metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "material.metallic", TextureName::METALLIC, scene);
		textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
		
		// 4. Normal map (Normal || Height)
		std::vector<TextureInfo> normalMaps;
		if(assimpHasMaterial(material, aiTextureType_NORMALS))
			normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "material.normal", TextureName::NORMAL, scene);
		else
			normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "material.normal", TextureName::NORMAL, scene);
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

		//TODO Treat cases where the type is undefined.
		//An example is when the metalness and roughness are packed into a single texture, as in GLTF. Assimp doesn't detect it and attributes aiTextureType_UNKNOWN.
		if (assimpHasMaterial(material, aiTextureType_UNKNOWN))
			LOG(WARNING) << "Unknown type material was identified for Assimp Material " << material->GetName().C_Str();

		Mesh thismesh;

		//Material specified via .json scene descriptor.
		if (m != nullptr) {
			thismesh.modelMaterialIndex = 0;
			std::vector<TextureInfo> texs = loadMaterialTextures(m, materialName);
			textures.insert(textures.end(), texs.begin(), texs.end());
		}else { 
			// Generate Material from .obj descriptors.
			for (int i = 0; i < diffuseMaps.size(); i++) {
				int matIndex = materials.size();
				this->materials.push_back(new Material());
				//Each material contains a pack of textures related to the BSDF, such as ALBEDO + METTALIC + SPECULAR etc
				Material* objMat = this->materials.at(matIndex); 

				GGXDistribution* ggxD = new GGXDistribution();
				ggxD->alpha = roughnessToAlpha(0.65f);
				FresnelSchilck* fresnel = new FresnelSchilck();
				GlossyBSDF* glossybsdf = new GlossyBSDF(ggxD, fresnel);

				objMat->bsdf = new BSDF();
				objMat->bsdf->addBxdf(glossybsdf);

				Texture* albedoTex = ResourceManager::getSelf()->getTexture(diffuseMaps.at(i).texID);
				objMat->addTexture(TextureName::ALBEDO, albedoTex);

				if (i < specularMaps.size()) {
					Texture* roughnessTex = ResourceManager::getSelf()->getTexture(specularMaps.at(i).texID);
					objMat->addTexture(TextureName::ROUGHNESS, roughnessTex);
				}
				if (i < metallicMaps.size()) {
					Texture* metallicTex = ResourceManager::getSelf()->getTexture(metallicMaps.at(i).texID);
					objMat->addTexture(TextureName::METALLIC, metallicTex);
				}
				if (i < normalMaps.size()) {
					Texture* normalTex = ResourceManager::getSelf()->getTexture(normalMaps.at(i).texID);
					objMat->addTexture(TextureName::NORMAL, normalTex);
				}

				ResourceManager::getSelf()->replaceMaterial(std::to_string(diffuseMaps.at(i).texID), objMat);
				m = objMat;
				thismesh.modelMaterialIndex = this->materials.size();
				this->materials.push_back(objMat);
			}
		}

		//Since we are setting a constraint of dealing always with triangles, aiFace->mNumIndices will always be 3 (vertices).
		//Layout v.x | v.y | v.z | n.x | n.y | n.z ...
		int vertexIndicesCurrentIndex = 0;
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];

			float* vertexAddress[3];

			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				unsigned int index = face.mIndices[j];
				this->indexData[indicesStartIndex + vertexIndicesCurrentIndex++] = index;
				vertexAddress[j] = &(this->vertexData[vertexStartIndex + index * stride]);
			}

			int primitiveIndex = (indicesStartIndex/3) + i;

			Triangle *triangle = new (&((Triangle*)(memoryBufferPrimitives.data))[primitiveIndex])
			Triangle(vertexAddress[0],
				vertexAddress[1], 
				vertexAddress[2], 
				m, 
				vertexAddress[0] + 3);

			triangle->material = m;
			triangle->vertexLayout = &vertexLayout;
			primitives.push_back(triangle);
		}

		thismesh.strideLength = stride;
		thismesh.vertexDataPointer = &(this->vertexData[vertexStartIndex]);
		thismesh.vertexDataPointerLength = mesh->mNumVertices * stride;

		thismesh.vertexIndicesPointer = new uint32_t[mesh->mNumFaces * 3];
		for (int i = 0; i < mesh->mNumFaces; i++) {
			thismesh.vertexIndicesPointer[i * 3] = mesh->mFaces[i].mIndices[0];
			thismesh.vertexIndicesPointer[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
			thismesh.vertexIndicesPointer[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
		}
		//Each face is made of 3 vertices (triangle), we force it using triangulate on ASSIMP.
		thismesh.vertexIndicesPointerLength = mesh->mNumFaces * 3;

		thismesh.material = m;
		thismesh.vertexLayout = vertexLayout;

		return thismesh;
	}

	void Model::processNode(aiNode* node, const aiScene* scene, std::string path, Material* m, std::string materialName) {
		relativePath = path;

		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene, startIndexVertex, startIndexIndices, m, materialName));
			startIndexVertex += scene->mMeshes[node->mMeshes[i]]->mNumVertices * stride;
			startIndexIndices += scene->mMeshes[node->mMeshes[i]]->mNumFaces * 3;
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			processNode(node->mChildren[i], scene, path, m, materialName);
	}

	void Model::processScene(const aiScene* scene, std::string path, std::string materialName) {
		Material* mat = nullptr;

		if (materialName.compare("") != 0)
			mat = ResourceManager::getSelf()->getMaterial(genStringID(materialName.c_str()));
		else {
			//If no name was given, set the name as the file path.
			materialName = path;
		}

		if (mat != nullptr)
			materials.push_back(mat);

		for (int i = 0; i < scene->mNumMeshes; i++) {
			vertexDataLength += scene->mMeshes[i]->mNumVertices * stride;
			indexDataLength += scene->mMeshes[i]->mNumFaces * 3;
		}

		vertexData = new float[vertexDataLength];
		indexData = new uint32_t[indexDataLength];

		uint32_t nPrimitives = indexDataLength / 3;
		memoryBufferPrimitives = {new Triangle[nPrimitives], nPrimitives * (uint32_t)sizeof(Triangle)};

		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::Tangent, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
		vertexLayout.end();

		processNode(scene->mRootNode, scene, path, mat, materialName);
		boundingBox = AABB(&aabbMin[0], &aabbMax[0]);
	}

	bool Model::intersect(Ray ray, RayIntersection& hit, float& tMin, float& tMax) {
		bool ditIntersect = false;
		RayIntersection tempIntersec;

		if (boundingBox.vertexData[0] != nullptr && boundingBox.vertexData[1] != nullptr)
			if (!boundingBox.intersect(ray, tempIntersec))
				return false;

		if (bvh.nodeCount > 0) {
			bool localIntersec =  bvh.intersect(ray, tempIntersec);
			if (localIntersec && tempIntersec.tNear > tMin && tempIntersec.tNear < tMax) {
				tMax = tempIntersec.tNear;
				hit = tempIntersec;
				ditIntersect = true;
			}
			return ditIntersect;
		}

		//If no LBVH is set, iterate linearly over all primitives
		for (int i = 0; i < primitives.size(); i++) {

			//If it is a participating medium like grid, check if the ray intersects any voxel.
			if (primitives.at(i)->material != nullptr && primitives.at(i)->material->medium != nullptr)
				if (GridMedia* gridMedia = dynamic_cast<GridMedia*>(primitives.at(i)->material->medium)) {
					bool localIntersec = primitives.at(i)->intersect(ray, tempIntersec);
					glm::vec3 res = glm::vec3(glm::max(0.0f, tempIntersec.tNear), tempIntersec.tFar, 0);

					//If intersected any non-empty voxel
					if (res.x <= res.y && !glm::isinf(res.y) && !glm::isinf(res.x)) {
						tempIntersec.primitive = primitives.at(i);
						tempIntersec.tNear = res.x;
						tempIntersec.tFar = res.y;
						tempIntersec.hitPoint = ray.getPointAt(res.x);
						tempIntersec.normal = -ray.d;
						tMax = tempIntersec.tNear;
						hit = tempIntersec;
						ditIntersect = true;
					}

					//A model can only have one Grid Media
					continue;
				}


			bool localIntersec = primitives.at(i)->intersect(ray, tempIntersec);
			//If we are inside a volume
			if (localIntersec && tempIntersec.tNear != tempIntersec.tFar && tempIntersec.tNear < 0 && tempIntersec.tFar > 0) {
				tempIntersec.tNear = 0;
				tempIntersec.hitPoint = ray.o;
				tMax = tempIntersec.tNear;
				hit = tempIntersec;
				ditIntersect = true;
			}

			if (localIntersec && tempIntersec.tNear > tMin && tempIntersec.tNear < tMax) {
				tMax = tempIntersec.tNear;
				hit = tempIntersec;
				ditIntersect = true;
			}
		}

		for (int i = 0; i < lights.size(); i++) {
			//If infinite area light, ignore intersection
			if (!isBlack(lights.at(i)->material->light->Le(ray, glm::mat4(1))))
				continue;

			bool localIntersec = lights.at(i)->intersect(ray, tempIntersec);
			if (localIntersec && tempIntersec.tNear > tMin && tempIntersec.tNear < tMax) {
				tMax = tempIntersec.tNear;
				hit = tempIntersec;
				ditIntersect = true;
			}
		}

		return ditIntersect;
	}

	AABB* Model::getAABB() {
		glm::vec3 min(INFINITY);
		glm::vec3 max(-INFINITY);

		for (int i = 0; i < vertexDataLength; i = i + 3) {
			glm::vec3 vertex = glm::vec3(vertexData[i], vertexData[i + 1], vertexData[i + 2]);
			min = glm::min(min, vertex);
			max = glm::max(max, vertex);
		}

		return new AABB(min, max);
	}

	void Model::centralize() {
		AABB *aabb = getAABB();
		glm::vec3 aabbCenter = aabb->getCenter();

		glm::mat4 transform(1.0f);
		transform = glm::translate(transform, -aabbCenter);

		for (int i = 0; i < vertexDataLength; i = i + 3) {
			glm::vec3 vertex = glm::vec3(vertexData[i], vertexData[i + 1], vertexData[i + 2]);
			vertex = transform * glm::vec4(vertex, 1.0f);
			vertexData[i] = vertex.x;
			vertexData[i + 1] = vertex.y;
			vertexData[i + 2] = vertex.z;
		}
	}

	Primitive* Model::getRandomLightPrimitive() {
		if (lights.size() == 0)
			return nullptr;
		float r = random();
		int i = lights.size() * r;

		return lights.at(i);
	}
}
