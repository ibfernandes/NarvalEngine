#pragma once
#include <glm/glm.hpp>
#include "core/Scene.h"
#include "core/Microfacet.h"
#include "io/SceneReader.h"
#include <sstream>

namespace narvalengine {
	class TestPlayground {
	public:
		//consider z-up
		void generateOrthonormalCS2(glm::vec3 normal, glm::vec3& v, glm::vec3& u) {
			//ns, ss, ts
			if (std::abs(normal.x) > std::abs(normal.y))
				v = glm::vec3(-normal.z, 0, normal.x) / std::sqrt(normal.x * normal.x + normal.z * normal.z);
			else
				v = glm::vec3(0, normal.z, -normal.y) / std::sqrt(normal.y * normal.y + normal.z * normal.z);

			u = glm::normalize(glm::cross(v, normal));
		}

		glm::vec3 toLCS2(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
			return glm::vec3(glm::dot(v, ss), glm::dot(v, ts), glm::dot(v, ns));
		}

		glm::vec3 toWorld2(glm::vec3 v, glm::vec3 ns, glm::vec3 ss, glm::vec3 ts) {
			return glm::vec3(
				ss.x * v.x + ts.x * v.y + ns.x * v.z,
				ss.y * v.x + ts.y * v.y + ns.y * v.z,
				ss.z * v.x + ts.z * v.y + ns.z * v.z);
		}

		//make Basis considering the left handed one with Y-up as the standart
		glm::mat4 makeBasis(glm::vec3 up, glm::vec3 forward, glm::vec3 right) {
			glm::mat4 res = glm::mat4(0);
			res[0] = glm::vec4(right, 0); // first column
			res[1] = glm::vec4(up, 0); // second column, Up/Normal
			res[2] = glm::vec4(forward, 0); // third column
			res[3] = glm::vec4(0, 0, 0, 1); // fourth column
			
			return res;
		}

		glm::vec3 changeBasis(glm::vec3 v, glm::mat4 basisChangeMat) {
			return glm::vec4(v, 0) * basisChangeMat;
		}

		glm::vec3 backFromBasis(glm::vec3 v, glm::vec3 up, glm::vec3 forward, glm::vec3 right) {
			return glm::vec3(glm::dot(v, right), glm::dot(v, up), glm::dot(v, right));
		}

		void testBasisChange() {
			glm::vec3 up = glm::vec3(0, 0, 1);
			glm::vec3 forward = glm::vec3(0, 1, 0);
			glm::vec3 right = glm::vec3(1, 0, 0);

			glm::mat4 basis = makeBasis(up, forward, right);

			//from left handed Y-up to Z-up
			glm::vec3 v(0, 1, 0);
			glm::vec3 toZUp = changeBasis(v, basis);
			glm::vec3 backToYUp = backFromBasis(toZUp, up, forward, right);

			float d;
		}

		void testMicrofacetBSDF() {
			int runs = 10000;
			int microfacetNotOnSameHemisphereAsNormal = 0;
			int scatteredDirNotOnSameHemisphereAsNormal = 0;
			bool printLog = false;

			for (int i = 0; i < runs; i++) {
				glm::vec3 cameraPos = glm::vec3(0, 2, -5);
				glm::vec3 pointTo = glm::vec3(0, 1, 0);

				//Both incoming and normal in WCS
				glm::vec3 incoming = glm::normalize(pointTo - cameraPos);
				glm::vec3 normal = glm::vec3(0, 0, 1);

				GGXDistribution* ggx = new GGXDistribution();
				ggx->alpha = 0.95;
				//Microfacet in SCS, tested and validated on a SCILAB plot
				//SCS has the z axis pointing up
				glm::vec3 microfacet = ggx->sampleMicrofacet();

				glm::vec3 up = glm::vec3(0, 0, 1);
				glm::vec3 forward = glm::vec3(0, 1, 0);
				glm::vec3 right = glm::vec3(1, 0, 0);
				glm::mat4 basis = makeBasis(up, forward, right);
				//now it has Z-up
				incoming = changeBasis(incoming, basis);

				glm::vec3 ss, ts;
				generateOrthonormalCS2(normal, ss, ts);

				//convert incoming and microfacet to the same LCS space
				microfacet = toLCS2(microfacet, normal, ss, ts);
				incoming = toLCS2(incoming, normal, ss, ts);

				//scatteredDir in LCS
				glm::vec3 scatteredDir = glm::reflect(glm::normalize(incoming), microfacet);

				//convert scatteredDir from LCS to WCS
				scatteredDir = toWorld2(scatteredDir, normal, ss, ts);
				//if (!sameHemisphere(scatteredDir, normal))
				//	scatteredDir = -scatteredDir;

				if (printLog) {
					printVec3(normal, "normal: ");
					printVec3(microfacet, "microfacet: ");
					printVec3(incoming, "incoming: ");
					printVec3(scatteredDir, "scattered Dir: ");
				}

				if (!sameHemisphere(microfacet, normal))
					microfacetNotOnSameHemisphereAsNormal++;

				if (!sameHemisphere(scatteredDir, normal)) {
					scatteredDirNotOnSameHemisphereAsNormal++;
					if (printLog)
						std::cout << "ERROR: scattered direction not in the same hemisphere as normal";
				}
				//if (!sameHemisphere(-incoming, normal))
				//	if (printLog)
				//		std::cout << "ERROR: -incoming direction not in the same hemisphere as normal";
			}

			std::cout << "microfacet missed " << microfacetNotOnSameHemisphereAsNormal << " out of " << runs << std::endl;
			//always half
			std::cout << "scatteredDir missed " << scatteredDirNotOnSameHemisphereAsNormal << " out of " << runs << std::endl;
			float d;
		}

		void unitTest() {
			glm::vec3 normal = glm::vec3(0, 0, -1);
			glm::vec3 v = glm::normalize(glm::vec3(0,1,0)); //in WCS
			glm::vec3 ss, ts;
			generateOrthonormalCS(normal, ss, ts);

			glm::vec3 vectorInLCS = toLCS2(v, normal, ss, ts);
			glm::vec3 vectorInWCS = toWorld2(vectorInLCS, normal, ss, ts);

			float d;
		}

		void testRandomGenRange() {
			for (int i = 0; i < 1000000000; i++) {
				float r = random();
				int k = 1 * r;

				if (k == 1) {
					std::stringstream ss;
					ss.precision(10);
					ss << std::fixed << "ERROR: k = " << k << ",  r = " << r << std::endl;
					std::cout << ss.str();
				}
			}

			std::cout << "done";
		}

		void genMaterial() {
			bool isFileTex = true;
			std::string name = "checkboard";
			std::string albedoPath = "";
			glm::vec3 albedo = glm::vec3(1, 0, 1);
			if (isFileTex)
				albedoPath = "imgs/checkboard.png";
			else
				albedo = glm::vec3(1,0,0);

			float roughness = 0.95;
			float metallic = 0;
			int flags = NE_TEX_SAMPLER_UVW_CLAMP | NE_TEX_SAMPLER_MIN_MAG_LINEAR;

			Texture* metallicTex;
			metallicTex = new Texture(1, 1, R32F, flags, { (uint8_t*)&metallic, sizeof(float) });
			metallicTex->textureName = TextureName::METALLIC;
			//metallicTex = new Texture(TextureName::METALLIC, TextureChannelFormat::R_METALLIC, glm::ivec2(1, 1), R32F, &metallic);
			ResourceManager::getSelf()->setTexture(name + ".metallic", metallicTex);

			Texture* albedoTex = nullptr;
			//If it should load a texture file
			if (isFileTex) {
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

			Material* mat = new Material();
			mat->addTexture(TextureName::ALBEDO, albedoTex);
			mat->addTexture(TextureName::METALLIC, metallicTex);

			GGXDistribution* ggxD = new GGXDistribution();
			ggxD->alpha = roughnessToAlpha(0.65f); //TODO double check
			FresnelSchilck* fresnel = new FresnelSchilck();
			GlossyBSDF* glossybsdf = new GlossyBSDF(ggxD, fresnel);

			mat->bsdf = new BSDF();
			mat->bsdf->addBxdf(glossybsdf);

			ResourceManager::getSelf()->replaceMaterial(name, mat);
		}

		void genRectangle(Scene *scene) {
			std::string materialName = "checkboard";
			std::string name = "texTest";
			std::string type = "rectangle";
			glm::vec3 pos = glm::vec3(0,0,0);
			glm::vec3 scale = glm::vec3(1);
			glm::vec3 rotate = glm::vec3(0);

			Model* model = new Model();
			model->vertexDataLength = 4 * 3 + 4 * 2;
			model->vertexData = new float[model->vertexDataLength];
			glm::vec3 point1(-0.5f, -0.5f, 0); //0: bottom left
			glm::vec3 point2(0.5f, -0.5f, 0); //1: bottom right
			glm::vec3 point3(0.5f, 0.5f, 0);  //2: top right
			glm::vec3 point4(-0.5f, 0.5f, 0); //3: top left
			glm::vec2 uv1(0, 0);
			glm::vec2 uv2(1, 0);
			glm::vec2 uv3(1, 1);
			glm::vec2 uv4(0, 1);
			model->vertexData[0] = point1[0];
			model->vertexData[1] = point1[1];
			model->vertexData[2] = point1[2];
			model->vertexData[3] = uv1[0];
			model->vertexData[4] = uv1[1];
			model->vertexData[5] = point2[0];
			model->vertexData[6] = point2[1];
			model->vertexData[7] = point2[2];
			model->vertexData[8] = uv2[0];
			model->vertexData[9] = uv2[1];
			model->vertexData[10] = point3[0];
			model->vertexData[11] = point3[1];
			model->vertexData[12] = point3[2];
			model->vertexData[13] = uv3[0];
			model->vertexData[14] = uv3[1];
			model->vertexData[15] = point4[0];
			model->vertexData[16] = point4[1];
			model->vertexData[17] = point4[2];
			model->vertexData[18] = uv4[0];
			model->vertexData[19] = uv4[1];
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
			m.strideLength = 5;
			m.vertexDataPointer = &model->vertexData[0];
			m.vertexDataPointerLength = model->vertexDataLength;
			m.vertexIndicesPointer = &model->faceVertexIndices[0];
			m.vertexIndicesPointerLength = model->faceVertexIndicesLength;

			m.vertexLayout.init();
			m.vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
			m.vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
			m.vertexLayout.end();

			for (Texture* t : ResourceManager::getSelf()->getMaterial(materialName)->textures) {
				if (t->textureName == TextureName::METALLIC) {
					m.textures.push_back({ genStringID(materialName + ".metallic"), "material.metallic" });
				}
				else if (t->textureName == TextureName::ALBEDO) {
					m.textures.push_back({ genStringID(materialName + ".albedo"), "material.diffuse" });
				}
			}

			model->meshes.push_back(m);

			Rectangle* rectangle = new Rectangle();
			rectangle->vertexLayout = &model->meshes.at(0).vertexLayout;
			rectangle->vertexData[0] = &model->vertexData[0];
			rectangle->vertexData[1] = &model->vertexData[m.strideLength * 2];
			rectangle->normal = glm::vec3(0, 0, -1);
			Material* material = ResourceManager::getSelf()->getMaterial(materialName);
			if (material->light != nullptr) {
				model->lights.push_back(rectangle);
				material->light->primitive = rectangle;
			}
			else
				model->primitives.push_back(rectangle);

			model->materials.push_back(material);
			rectangle->material = material;

			StringID modelID = ResourceManager::getSelf()->replaceModel(name, model);
			InstancedModel* instancedModel = new InstancedModel(model, modelID, getTransform(pos, rotate, scale));
			if (material->light != nullptr)
				scene->lights.push_back(instancedModel);
			else
				scene->instancedModels.push_back(instancedModel);
		}

		void testUVSampling() {
			Scene* scene = new Scene();
			genMaterial();
			genRectangle(scene);

			Ray r = { glm::vec3(0,0,-.1f), glm::vec3(0, 0, 1)};
			Ray scattered = { glm::vec3(0), glm::normalize(glm::vec3(0,1,-1)) };
			for(float x = -0.5; x < 0.5; x = x + 0.1f)
				for (float y = -0.5; y < 0.5; y = y + 0.1f) {
					r.o.x = x;
					r.o.y = y;

					RayIntersection ri;
					bool didhit = scene->intersectScene(r, ri, 0, 999);

					if (didhit) {
						std::cout << "----------------" << std::endl;
						std::cout << toString(ri.uv, "uv: ") << std::endl; // OK
						std::cout << toString(ri.hitPoint, "hitPoint: ") << std::endl; //OK
						glm::vec3 albedo = ri.primitive->material->sampleMaterial(ALBEDO, ri.uv.x, ri.uv.y);
						std::cout << toString(albedo, "albedo: ") << std::endl; //OK

						//here is the godamn problem.
						glm::vec3 fr = ri.primitive->material->bsdf->eval(r.d, scattered.d, ri);
						std::cout << toString(fr, "fr: ") << std::endl;
					}
				}
					
			float a = 0;
		}

		void testDot() {
			glm::vec3 v1 = glm::vec3(1,1,0);
			glm::vec3 v2 = glm::vec3(1,1,0);
			float dot = glm::dot(v1, v2);

		}

		TestPlayground() {
			//testRandomGenRange();
			//testBasisChange();
			//unitTest();
			//testMicrofacetBSDF();
			//testUVSampling();
			testDot();
			float t;
		}

		~TestPlayground();
	};
}

