#pragma once
#define NOMINMAX //necessary for nanovdb
#define GLM_FORCE_ALIGNED_GENTYPES true
#define GLM_FORCE_INTRINSICS true
#define GLM_FORCE_SIMD_AVX2 true
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>
#include <glm/simd/geometric.h>
#include <xmmintrin.h>
#include <immintrin.h>

#include "core/Scene.h"
#include "integrators/VolumetricPathIntegrator.h"
#include "core/Microfacet.h"
#include "io/SceneReader.h"
#include <sstream>
//#include <openvdb/openvdb.h>
//#include <openvdb/tools/Dense.h>
#include <nanovdb/util/OpenToNanoVDB.h> // converter from OpenVDB to NanoVDB (includes NanoVDB.h and GridManager.h)
#include <nanovdb/util/IO.h>
#include "utils/Timer.h"

#include "oidn/include/OpenImageDenoise/oidn.hpp"

#include "oidn/include/OpenImageDenoise/oidn.hpp"
//#include <OpenImageDenoise/oidn.hpp>

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

			//TODO updated to TextureName channels
			for (Texture* t : ResourceManager::getSelf()->getMaterial(materialName)->textures) {
				if (t->textureName == TextureName::METALLIC) {
					//m.textures.push_back({ genStringID(materialName + ".metallic"), "material.metallic" });
				}
				else if (t->textureName == TextureName::ALBEDO) {
					//m.textures.push_back({ genStringID(materialName + ".albedo"), "material.diffuse" });
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

		void convertToNanoVDB() {
			try {
				openvdb::io::File file(RESOURCES_DIR + std::string("vdb/cube.vdb"));
				openvdb::GridBase::Ptr baseGrid;
				file.open();
				for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
					if (nameIter.gridName() == "density") {
						baseGrid = file.readGrid(nameIter.gridName());
						break;
					}
				}
				file.close();

				openvdb::FloatGrid::Ptr srcGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
				nanovdb::GridHandle handle = nanovdb::openToNanoVDB(srcGrid);

				auto* dstGrid = handle.grid<float>();

				if (!dstGrid)
					throw std::runtime_error("GridHandle does not contain a grid with value type float");

				// Get accessors for the two grids. Note that accessors only accelerate repeated access!
				auto dstAcc = dstGrid->getAccessor();
				auto srcAcc = srcGrid->getAccessor();

				nanovdb::io::writeGrid(RESOURCES_DIR + std::string("scenes/pbrt/cube.nvdb"), handle); // Write the NanoVDB grid to file and throw if writing fails
			}catch (const std::exception& e) {
				std::cerr << "An exception occurred: \"" << e.what() << "\"" << std::endl;
			}
		}

		void generateCubeVDB() {
			glm::ivec3 res = glm::vec3(50,50,50);
			float *fgrid = new float[res.x * res.y * res.z];
			for (int i = 0; i < res.x * res.y * res.z; i++)
				fgrid[i] = 0.02;

			openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
			openvdb::FloatGrid::Accessor accessor = grid->getAccessor();

			/*
			for(int x = 0; x < res.x; x++)
				for(int y = 0; y < res.y; y++)
					for (int z = 0; z < res.z; z++) {
						openvdb::Coord xyz(x, y, z);
						accessor.setValue(xyz, 1.0); // Should be fgrid[i] value
					}
			*/

			//centered
			for (int x = -res.x/2; x < glm::ceil(res.x/2.0f); x++)
				for (int y = -res.y/2; y < glm::ceil(res.y/2.0f); y++)
					for (int z = -res.z/2; z < glm::ceil(res.z/2.0f); z++) {
						openvdb::Coord xyz(x, y, z);
						accessor.setValue(xyz, 0.02); // Should be fgrid[i] value
					}
			
			//grid->insertMeta("radius", openvdb::FloatMetadata(50.0));
			//grid->setTransform(openvdb::math::Transform::createLinearTransform(/*voxel size=*/0.5));
			//grid->setGridClass(openvdb::GRID_LEVEL_SET);
			grid->setName("density");

			// Create a VDB file object.
			openvdb::io::File file(RESOURCES_DIR + std::string("vdb/cubeBig.vdb"));
			openvdb::GridPtrVec grids;
			grids.push_back(grid);
			file.write(grids);
			file.close();
		}

		void floatRoundError() {
			Efloat a = 1.34343434;
			Efloat b = 1.32323232;
			Efloat c = a + b;
			c = 0;
			double d = 0;

			for (int i = 0; i < 200; i++) {
				c = c + 1.11111111;
				d = d + 1.11111111;
			}

			float expected = 222.222222f;

			std::cout << std::setprecision(64);
			std::cout << "Result Double: " << d << std::endl;
			std::cout << "Result: " << (float)c << std::endl;
			std::cout << "Result expected: " << expected << std::endl;
			std::cout << "Diff obt and expec: " << expected - (float)c << std::endl;
			std::cout << "Result + error: " << (float)c + c.getAbsoluteError() << std::endl;
			std::cout << "Result - error: " << (float)c - c.getAbsoluteError() << std::endl;
			std::cout << "Error: " << c.getAbsoluteError() << std::endl;
			std::cout << "UpperBound: " << c.upperBound() << std::endl;
			std::cout << "lowerBound: " << c.lowerBound() << std::endl;
			std::cout << "Machine Epsilon: " << machineEpsilon << std::endl;
		}

		void testEXR();

		void glmSIMD() {
			Timer t;
			int ops = 100000000;

			std::cout << "glm_vec4 glm_vec4_dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm_vec4 t1 = { 1,1,1,1 };
				glm_vec4 t2 = { 2,2,2,2 };

				glm_vec4 k = glm_vec4_dot(t1, t2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "glm::vec4 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec4 v1 = glm::vec4(1, 1, 1, 1);
				glm::vec4 v2 = glm::vec4(2, 2, 2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "aligned_highp_vec4 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::aligned_highp_vec4 v1 = glm::aligned_highp_vec4(1, 1, 1, 1);
				glm::aligned_highp_vec4 v2 = glm::aligned_highp_vec4(2, 2, 2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "glm::vec3 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec3 v1 = glm::vec3(1, 1, 1);
				glm::vec3 v2 = glm::vec3(2, 2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "aligned_highp_vec3 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::aligned_highp_vec3 v1 = glm::aligned_highp_vec3(1, 1, 1);
				glm::aligned_highp_vec3 v2 = glm::aligned_highp_vec3(2, 2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "glm::vec2 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec2 v1 = glm::vec2(1, 1);
				glm::vec2 v2 = glm::vec2(2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "aligned_highp_vec2 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::aligned_highp_vec2 v1 = glm::aligned_highp_vec2(1, 1);
				glm::aligned_highp_vec2 v2 = glm::aligned_highp_vec2(2, 2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "glm::vec1 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec1 v1 = glm::vec1(1);
				glm::vec1 v2 = glm::vec1(2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			std::cout << "aligned_highp_vec1 glm::dot" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::aligned_highp_vec1 v1 = glm::aligned_highp_vec1(1);
				glm::aligned_highp_vec1 v2 = glm::aligned_highp_vec1(2);

				float k = glm::dot(v1, v2);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;
		}

		glm::vec4 intersectBoxSIMDv3(__m256 o, __m256 d, __m256 bmi, __m256 bma) {
			__m256 t1 = _mm256_mul_ps(_mm256_sub_ps(bmi, o), d);
			__m256 t2 = _mm256_mul_ps(_mm256_sub_ps(bma, o), d);
			__m256 vmax4 = _mm256_max_ps(t1, t2);
			__m256 vmin4 = _mm256_min_ps(t1, t2);
			float* vmax = (float*)&vmax4;
			float* vmin = (float*)&vmin4;
			float tmax1 = glm::min(vmax[0], glm::min(vmax[1], vmax[2]));
			float tmin1 = glm::max(vmin[0], glm::max(vmin[1], vmin[2]));
			float tmax2 = glm::min(vmax[3], glm::min(vmax[4], vmax[5]));
			float tmin2 = glm::max(vmin[3], glm::max(vmin[4], vmin[5]));

			return glm::vec4(tmin1, tmax1, tmin2, tmax2);
		}

		glm::vec2 intersectBoxSIMDv2(__m128 o, __m128 d, __m128 bmi, __m128 bma) {
			__m128 t1 = _mm_mul_ps(_mm_sub_ps(bmi, o), d);
			__m128 t2 = _mm_mul_ps(_mm_sub_ps(bma, o), d);
			__m128 vmax4 = _mm_max_ps(t1, t2);
			__m128 vmin4 = _mm_min_ps(t1, t2);
			float* vmax = (float*)&vmax4;
			float* vmin = (float*)&vmin4;
			float tmax = glm::min(vmax[0], glm::min(vmax[1], vmax[2]));
			float tmin = glm::max(vmin[0], glm::max(vmin[1], vmin[2]));

			return glm::vec2(tmin, tmax);
		}

		glm::vec2 intersectBoxSIMD(glm::vec4 orig, glm::vec4 dir, glm::vec4 bmin, glm::vec4 bmax) {
			__m128 o = _mm_load_ps(&orig[0]);
			__m128 d = _mm_load_ps(&dir[0]);
			__m128 bmi = _mm_load_ps(&bmin[0]);
			__m128 bma = _mm_load_ps(&bmax[0]);

			__m128 t1 = _mm_mul_ps(_mm_sub_ps(bmi, o), d);
			__m128 t2 = _mm_mul_ps(_mm_sub_ps(bma, o), d);
			__m128 vmax4 = _mm_max_ps(t1, t2);
			__m128 vmin4 = _mm_min_ps(t1, t2);
			float* vmax = (float*)&vmax4;
			float *vmin = (float*)&vmin4;
			float tmax = glm::min(vmax[0], glm::min(vmax[1], vmax[2]));
			float tmin = glm::max(vmin[0], glm::max(vmin[1], vmin[2]));

			return glm::vec2(tmin, tmax);
		}

		inline glm::aligned_highp_vec2 intersectBoxhighp(glm::aligned_highp_vec3 orig, glm::aligned_highp_vec3 dir, glm::aligned_highp_vec3 bmin, glm::aligned_highp_vec3 bmax) {
			//Line's/Ray's equation
			// o + t*d = y
			// t = (y - o)/d
			//when t is negative, the box is behind the ray origin
			glm::aligned_highp_vec3 tMinTemp = (bmin - orig) / dir; //TODO: div by 0
			glm::aligned_highp_vec3 tmaxTemp = (bmax - orig) / dir;

			tmaxTemp.x *= 1 + 2 * gamma(3);
			tmaxTemp.y *= 1 + 2 * gamma(3);
			tmaxTemp.z *= 1 + 2 * gamma(3);

			glm::aligned_highp_vec3 tMin = glm::min(tMinTemp, tmaxTemp);
			glm::aligned_highp_vec3 tMax = glm::max(tMinTemp, tmaxTemp);

			float t0 = glm::max(tMin.x, glm::max(tMin.y, tMin.z));
			float t1 = glm::min(tMax.x, glm::min(tMax.y, tMax.z));

			//if t0 > t1: miss
			return glm::aligned_highp_vec2(t0, t1);
		}

		void aabbSIMD() {
			glm::vec3 origin = glm::vec3(0.5, 0.5, -0.5);
			glm::vec3 direction = glm::vec3(0,0,1);
			glm::vec3 min = glm::vec3(0,0,0);
			glm::vec3 max = glm::vec3(1,1,1);
			int ops = 100000000;
			Timer t;

			std::cout << "normal" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec2 res = intersectBox(origin, direction, min, max);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			//just doing vec4 outside the loop reduces the cost by half!!!!!
			//TODO d should the invDir!
			glm::vec4 o4 = glm::vec4(origin, 0);
			glm::vec4 d4 = glm::vec4(direction, 0);
			glm::vec4 mi4 = glm::vec4(min, 0);
			glm::vec4 ma4 = glm::vec4(max, 0);
			std::cout << "SIMD" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec2 res = intersectBoxSIMD(o4, d4, mi4, ma4);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			//if i do the load once per AABB and ray, it is faster
			__m128 o = _mm_load_ps(&o4[0]);
			__m128 d = _mm_load_ps(&d4[0]);
			__m128 bmi = _mm_load_ps(&mi4[0]);
			__m128 bma = _mm_load_ps(&ma4[0]);
			std::cout << "SIMD v2" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec2 res = intersectBoxSIMDv2(o, d, bmi, bma);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			float o8[8] = { origin.x, origin.y, origin.z, 0, origin.x, origin.y, origin.z, 0};
			float d8[8] = {direction.x, direction.y, direction.z, 0, direction.x, direction.y, direction.z, 0};
			float mi8[8];
			float ma8[8];
			__m256 o256 = _mm256_load_ps(&o8[0]);
			__m256 d256 = _mm256_load_ps(&d8[0]);
			std::cout << "SIMD v3" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i+=2) {
				mi8[0] = min.x; mi8[1] = min.y; mi8[2] = min.z; mi8[3] = 0;
				mi8[4] = min.x; mi8[5] = min.y; mi8[6] = min.z; mi8[7] = 0;

				ma8[0] = max.x; ma8[1] = max.y; ma8[2] = max.z; ma8[3] = 0;
				ma8[4] = max.x; ma8[5] = max.y; ma8[6] = max.z; ma8[7] = 0;
				__m256 bmi256 = _mm256_load_ps(&mi8[0]);
				__m256 bma256 = _mm256_load_ps(&ma8[0]);

				glm::vec2 res = intersectBoxSIMDv3(o256, d256, bmi256, bma256);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

			glm::aligned_highp_vec3 ohp = glm::aligned_highp_vec3(origin);
			glm::aligned_highp_vec3 dhp = glm::aligned_highp_vec3(direction);
			glm::aligned_highp_vec3 mihp = glm::aligned_highp_vec3(min);
			glm::aligned_highp_vec3 mahp = glm::aligned_highp_vec3(max);
			std::cout << "highp" << std::endl;
			t.startTimer();
			for (int i = 0; i < ops; i++) {
				glm::vec2 res = intersectBoxhighp(ohp, dhp, mihp, mahp);
			}
			t.endTimer();
			t.printlnNanoSeconds();
			std::cout << std::endl;

		}

		void oidnTest();

		void testMSFactorapproach() {
			SceneReader sceneReader;
			sceneReader.loadScene("scenes/testing.json", false);
			Scene *scene = sceneReader.getScene();
			scene->settings.bounces = 1;

			VolumetricPathIntegrator *volPath = new VolumetricPathIntegrator();

			Ray r = { glm::vec3(0.0f, 1.5f,-0.0f), glm::vec3(0, 0, 1) };

			const int maxBouncesToTest = 100;
			const int samples = 1000;
			const int test = 5;

			if (test == 0) {
				/*
					Avg Li radiance 
				*/
				glm::vec3 LiAvg[maxBouncesToTest];
				for (int i = 0; i < maxBouncesToTest; i++)
					LiAvg[i] = glm::vec3(0);

				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/Li.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				std::cout << "Samples per Bounce N" << samples << std::endl;
				for (int k = 0; k < maxBouncesToTest; k++) {
					for (int i = 0; i < samples; i++) {
						glm::vec3 Li = volPath->Li(r, scene);
						LiAvg[k] += Li;
					}
					LiAvg[k] = LiAvg[k] / float(samples);
					std::cout << "Bounces: " << k + 1 << std::endl;
					printVec3(LiAvg[k], "Li ");
					file << LiAvg[k].x << " " << LiAvg[k].y << " " << LiAvg[k].z << " ";

					scene->settings.bounces++;
				}

				file.close();
			}else if (test == 1) {
				/*
					Avg Path Depth (should correlate to mean free path?)
				*/
				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/pathDepth.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				GridMedia* m = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;
				float avgPathDepth[maxBouncesToTest];
				for (int i = 0; i < maxBouncesToTest; i++)
					avgPathDepth[i] = 0;

				for (int b = 0; b < maxBouncesToTest; b++) {
					for (int i = 0; i < samples; i++) {
						m->t = 0;
						Ray incoming = r;

						for (int k = 0; k < b+1; k++) {
							RayIntersection ri;
							Ray scattered;

							bool didHit = scene->intersectScene(incoming, ri, 0, 9999);
							if (!didHit)
								break;

							m->sample(incoming, scattered, ri);
							incoming = scattered;
						}

						avgPathDepth[b] += m->t;
					}

					avgPathDepth[b] /= samples;
					std::cout << "Bounces: " << b + 1 << std::endl;
					std::cout << avgPathDepth[b] << std::endl;
					file << avgPathDepth[b] << " ";
				}
				file.close();
			}else if (test == 2) {
				/*
					Avg Transmittance Tr
				*/
				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/Tr.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				GridMedia* m = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;
				glm::vec3 avgTr[maxBouncesToTest];
				for (int i = 0; i < maxBouncesToTest; i++)
					avgTr[i] = glm::vec3(1);

				for (int b = 0; b < maxBouncesToTest; b++) {
					for (int i = 0; i < samples; i++) {
						m->tr = glm::vec3(1);
						Ray incoming = r;

						for (int k = 0; k < b + 1; k++) {
							RayIntersection ri;
							Ray scattered;

							bool didHit = scene->intersectScene(incoming, ri, 0, 9999);
							if (!didHit)
								break;

							m->sample(incoming, scattered, ri);
							incoming = scattered;
						}

						avgTr[b] += m->tr;
					}

					avgTr[b] /= samples;
					std::cout << "Bounces: " << b + 1 << std::endl;
					printVec3(avgTr[b]);
					file << avgTr[b].x << " " << avgTr[b].y << " " << avgTr[b].z << " ";
				}
				file.close();
			}else if (test == 3) {
				/*
					Avg Theta from HG
				*/
				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/avgHGCos.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				GridMedia* m = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;

				const int numberOfgs = 21;
				float increment = 0.1;
				float g = -1.0;

				float avgTheta[numberOfgs];
				for (int i = 0; i < numberOfgs; i++)
					avgTheta[i] = 0;

				for (int i = 0; i < numberOfgs; i++) {
					for (int k = 0; k < samples; k++) {
						Ray incoming = r;
						RayIntersection ri;
						Ray scattered;

						bool didHit = scene->intersectScene(incoming, ri, 0, 9999);
						if (!didHit)
							break;

						VolumeBSDF* volbsdf = (VolumeBSDF*)ri.instancedModel->model->materials.at(0)->bsdf->bxdf[0];
						HG* hg = (HG*)volbsdf->phaseFunction;
						hg->g = g;

						m->sample(incoming, scattered, ri);
						avgTheta[i] += hg->theta;

						incoming = scattered;
					}
					avgTheta[i] /= samples;

					std::cout << "Avg Theta for g = " << g << ": " << std::endl;
					std::cout << avgTheta[i] << std::endl;
					file << avgTheta[i] << " ";

					g += increment;
				}
				file.close();
			}else if (test == 4) {
				/*
					Ray paths
				*/
				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/paths.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				GridMedia* m = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;
				const int numberOfPaths = 15;

				for (int i = 0; i < numberOfPaths; i++) {
					Ray incoming = r;

					for (int k = 0; k < maxBouncesToTest; k++) {
						file << incoming.o.x << " " << incoming.o.y << " " << incoming.o.z << " ";
						RayIntersection intersection;
						Ray scattered;

						bool didHit = scene->intersectScene(incoming, intersection, 0, 9999);
						if (!didHit)
							break;

						incoming.o = incoming.getPointAt(intersection.tNear);
						intersection.tFar = intersection.tFar - intersection.tNear;
						intersection.tNear = 0;

						//Samples the Media for a scattered direction and point. Returns the transmittance from the incoming ray up to that point.
						Ray rayOCS = transformRay(incoming, intersection.instancedModel->invTransformToWCS);
						glm::vec3 sampledTransmittance = intersection.primitive->material->medium->sample(rayOCS, scattered, intersection);
						scattered = transformRay(scattered, intersection.instancedModel->transformToWCS);

						VolumeBSDF* volbsdf = (VolumeBSDF*)intersection.instancedModel->model->materials.at(0)->bsdf->bxdf[0];
						HG* hg = (HG*)volbsdf->phaseFunction;
						//scattered.d = hg->sample(rayOCS.d);
						//scattered.d = glm::normalize(scattered.d);

						incoming = scattered;
					}

					file << std::endl;
				}
				file.close();
			}else if (test == 5) {
			/*
				Plot influence per bounces
			*/
				std::ofstream file;
				file.open(std::string(RESOURCES_DIR) + "tests/bsdf.txt", std::ios::out | std::ios::ate | std::ios::trunc);

				GridMedia* m = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;

				float *theta = new float[maxBouncesToTest*samples];
				float *phi = new float[maxBouncesToTest*samples];
				float *bsdfValue = new float[maxBouncesToTest*samples];
				float *tr = new float[maxBouncesToTest*samples];
				glm::vec3 *Li = new glm::vec3[maxBouncesToTest*samples*3];

				for(int k = 0; k < maxBouncesToTest; k++)
					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						theta[pos] = 0;
						phi[pos] = 0;
						bsdfValue[pos] = 0;
						tr[pos] = 1;
						Li[pos] = glm::vec3(0);
					}


				// 1 bounces, n samples
				// 2 bounces, n samples
				//...
				for (int k = 0; k < maxBouncesToTest; k++) {
					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						Ray incoming = r;

						for (int b = 0; b < k + 1; b++) {
							RayIntersection ri;
							Ray scattered;

							bool didHit = scene->intersectScene(incoming, ri, 0, 9999);
							if (!didHit || ri.instancedModel->model->lights.size() > 0)
								break;

							VolumeBSDF* volbsdf = (VolumeBSDF*)ri.instancedModel->model->materials.at(0)->bsdf->bxdf[0];
							HG* hg = (HG*)volbsdf->phaseFunction;

							glm::vec3 sampledTransmittance = m->sample(incoming, scattered, ri);
							glm::vec3 fr = volbsdf->eval(incoming.d, scattered.d, ri);
							float pdf = volbsdf->pdf(incoming.d, scattered.d);

							glm::vec3 lightSample = volPath->uniformSampleOneLight(scattered, ri, scene);

							theta[pos] = hg->theta;
							phi[pos] = hg->phi;
							bsdfValue[pos] += (fr.x/pdf);
							tr[pos] *= sampledTransmittance.x;
							Li[pos] += tr[pos] * lightSample;

							incoming = scattered;
						}

						//theta[k][i] /= (k+1);
						//phi[k][i] /= (k+1);
						//Li[k][i] /= (k+1);
						//bsdfValue[k][i] /= (k+1);
					}
					std::cout << "Bounce " << k + 1 << " finished" << std::endl;

					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						file << theta[pos] << " ";
					}
					file << std::endl;

					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						file << phi[pos] << " ";
					}
					file << std::endl;

					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						file << bsdfValue[pos] << " ";
					}
					file << std::endl;

					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						file << tr[pos] << " ";
					}
					file << std::endl;

					for (int i = 0; i < samples; i++) {
						int pos = to1D(maxBouncesToTest, samples, k, i);
						file << Li[pos].x << " ";
					}
					file << std::endl;
				}
				file.close();
			}

			float k = 0;
		}

		void compareEXR(std::string img1, std::string img2, std::string writeTo);

		void convertVDBToFloatFile() {

			SceneReader sceneReader;
			sceneReader.loadScene("scenes/testing.json", false);
			Scene* scene = sceneReader.getScene();
			GridMedia* gm = (GridMedia*)scene->instancedModels.at(0)->model->materials.at(0)->medium;

			std::ofstream file;
			file.open(std::string(RESOURCES_DIR) + "vdb/cloud.json", std::ios::out | std::ios::ate | std::ios::trunc);
			int size = gm->lbvh->gridSize.x * gm->lbvh->gridSize.y * gm->lbvh->gridSize.z;

			file << "{ ";
			file << "\"grid\": [";
			for(int i = 0; i < size-1; i++)
				file << gm->lbvh->grid[i] << ", ";
			file << gm->lbvh->grid[size - 1];
			file << "] ";
			file << "}";
			file.close();
		}

		TestPlayground() {
			//testRandomGenRange();
			//testBasisChange();
			//unitTest();
			//testMicrofacetBSDF();
			//testUVSampling();
			//testDot();
			generateCubeVDB();
			//convertToNanoVDB();
			//floatRoundError();
			//testEXR();
			//glmSIMD();
			//aabbSIMD();
			//oidnTest();
			convertVDBToFloatFile();
			//testMSFactorapproach();

			//std::string img1 = RESOURCES_DIR + std::string("scenes/tests/10000spp 1000b quarter.exr");
			//std::string img2 = RESOURCES_DIR + std::string("scenes/tests/10000spp 35b quarter.exr");
			//std::string writeTo = RESOURCES_DIR + std::string("scenes/tests/RED1000bVS35b.exr");
			//compareEXR(img1, img2, writeTo);

			float t = 0;
		}

		~TestPlayground();
	};
}

