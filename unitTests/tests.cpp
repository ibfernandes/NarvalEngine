#include "tests.h"

GTEST_API_ int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_stderrthreshold = 1; // Warning and above.
  printf("Running main() from gtest_main.cc\n");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

namespace narvalengine {
	void PrintTo(const Ray& ray, std::ostream* os) {
		*os << "Ray.origin: " << toString(ray.o) << std::endl
			<< "Ray.direction: " << toString(ray.d) << std::endl;
	}
	void PrintTo(const RayIntersection& ri, std::ostream* os) {
		*os << "RayIntersec.hitPoint: " << toString(ri.hitPoint) << std::endl
			<< "RayIntersec.normal: " << toString(ri.normal) << std::endl
			<< "RayIntersec.uv: " << toString(ri.uv) << std::endl
			<< "RayIntersec.tNear: " << ri.tNear << std::endl
			<< "RayIntersec.tFar: " << ri.tFar << std::endl;
	}

	/**
	*	Tests if two floats are equal tolerating epsilon as error.
	*/
	::testing::AssertionResult floatEqualsEpsilon(float f1, float f2, float epsilon) {

		if (f1 > (f2 - epsilon) && f1 < (f2 + epsilon))
			return ::testing::AssertionSuccess();
		else
			return ::testing::AssertionFailure() << f1 << " not equal to " << f2 << "± " << epsilon;
	}

	/**
	*	Tests if two vectors are equal tolerating epsilon as error.
	*/
	::testing::AssertionResult vec3EqualsEpsilon(glm::vec3 v1, glm::vec3 v2, float epsilon) {
		if (floatEqualsEpsilon(v1.x, v2.x, epsilon) && floatEqualsEpsilon(v1.y, v2.y, epsilon) && floatEqualsEpsilon(v1.z, v2.z, epsilon))
			return ::testing::AssertionSuccess();
		else
			return ::testing::AssertionFailure() << toString(v1) << " not equal to " << toString(v2);
	}

	/**
	 *	Validates the inverval of our uniform random number generator.
	 */
	TEST(Math, random) {
		const int nSamples = 10000000;
		float r = 0;

		//Validates the random interval
		for (int i = 0; i < nSamples; i++) {
			r = random();
			ASSERT_TRUE(r >= 0.0f && r <= 1.0f);
		}
	}

	/**
	 * Tests if the conversion between Local Cordinate System (LCS), where the z-axis is up, to World Coordinate System (WCS).
	 */
	TEST(Math, conversionBetweenLCSandWCS) {
		float precision = EPSILON3;
		//Where ns is the shading normal, usually retrieved from a normal map texture.
		//ns, ss and ts are in World Coordiante System (WCS) and define a orthonormal basis.
		//LCS is defined with z as up vector. As per the tangent space in normal's texture representation.
		glm::vec3 ns, ss, ts;
		ns = glm::vec3(0, 1, 0);
		ss = glm::vec3(1, 0, 0);
		ts = glm::vec3(0, 0, 1);

		//	WCS						 LCS
		//  (0, 1, 0)				 (0, 0, 1)
		//      |					     |
		//		|______      to:		 |_______
		//		 \						  \
		//		  \						   \

		EXPECT_TRUE(vec3EqualsEpsilon(toLCS(glm::vec3(0, 1, 0), ns, ss, ts), glm::vec3(0, 0, 1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(toWorld(glm::vec3(0, 0, 1), ns, ss, ts), glm::vec3(0, 1, 0), precision));
	}

	/**
	 * Tests the orthonormal coordinate system is generated accordingly.
	 */
	TEST(Math, generateOrthonormalCS) {
		float precision = EPSILON3;
		glm::vec3 v, u;

		generateOrthonormalCS(glm::vec3(0, 1, 0), v, u);
		EXPECT_TRUE(vec3EqualsEpsilon(u, glm::vec3(-1, 0, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(v, glm::vec3(0, 0, -1), precision));

		generateOrthonormalCS(glm::vec3(0, -1, 0), v, u);
		EXPECT_TRUE(vec3EqualsEpsilon(u, glm::vec3(-1, 0, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(v, glm::vec3(0, 0, 1), precision));
	}

	/**
	 *	Validates the vectors sampled in the unit sphere.
	 *	Note that in the Spherical Coordinate System (SCS) the Z-axis is up.
	 */
	TEST(Math, sampleUnitSphere) {
		//For this test we are using an epsilon of 10^-3.
		float precision = EPSILON3;

		//If the phi angle is fixed at 0 degrees, we are rotating parallel to the z axis (up).
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(0.0f, 0.0f / 180.0f), glm::vec3(0, 0, 1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(90.0f / 360.0f, 0.0f / 180.0f), glm::vec3(0, 0, 1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(180.0f / 360.0f, 0.0f / 180.0f), glm::vec3(0, 0, 1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(270.0f / 360.0f, 0.0f / 180.0f), glm::vec3(0, 0, 1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(360.0f / 360.0f, 0.0f / 180.0f), glm::vec3(0, 0, 1), precision));

		//If the phi angle is fixed at 90 degrees, we are rotating perpendicular to the z axis.
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(0.0f / 360.0f, 90.0f / 180.0f), glm::vec3(1, 0, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(90.0f / 360.0f, 90.0f / 180.0f), glm::vec3(0, 1, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(180.0f / 360.0f, 90.0f / 180.0f), glm::vec3(-1, 0, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(270.0f / 360.0f, 90.0f / 180.0f), glm::vec3(0, -1, 0), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(360.0f / 360.0f, 90.0f / 180.0f), glm::vec3(1, 0, 0), precision));

		//If the phi angle is fixed at 180 degrees, we are rotating parallel to the z axis, but pointing downwards.
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(0.0f / 360.0f, 180.0f / 180.0f), glm::vec3(0, 0, -1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(90.0f / 360.0f, 180.0f / 180.0f), glm::vec3(0, 0, -1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(180.0f / 360.0f, 180.0f / 180.0f), glm::vec3(0, 0, -1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(270.0f / 360.0f, 180.0f / 180.0f), glm::vec3(0, 0, -1), precision));
		EXPECT_TRUE(vec3EqualsEpsilon(sampleUnitSphere(360.0f / 360.0f, 180.0f / 180.0f), glm::vec3(0, 0, -1), precision));

		//Test boundaries.
		//Both arguments must be in the interval  [0, 1].
		ASSERT_DEATH(sampleUnitSphere(0.0f, -1.0f), "");
		ASSERT_DEATH(sampleUnitSphere(-1.0f, -1.0f), "");
		ASSERT_DEATH(sampleUnitSphere(-1.0f, 0.0f), "");
		ASSERT_DEATH(sampleUnitSphere(2.0f, 0.0f), "");
		ASSERT_DEATH(sampleUnitSphere(0.0f, 2.0f), "");
		ASSERT_DEATH(sampleUnitSphere(2.0f, 2.0f), "");
	}

	/**
	 *	Validates the g value of the Henyey-Greenstein (HG) phase function.
	 */
	TEST(BxDF, HGPhaseFunctionEvalG) {
		//From the paper DIFFUSE RADIATION IN THE GALAXY - L.G.HENYEY AND J.L.GREENSTEIN:
		//The phase angle is theta, defined as the deviation of the ray from the forward direction.
		//In our implementation the incomingRay represents the forward direction.
		glm::vec3 incomingRay = glm::vec3(0, 0, 1);
		const int nSamples = 100000;
		const float gStep = 0.3;
		const double spherePDF = INV4PI;

		for (float g = -0.9f; g < 0.9f; g += gStep) {
			HG* hgPhaseFunction = new HG(g);
			double result = 0.0f;

			for (int i = 0; i < nSamples; i++) {
				glm::vec3 scattered = sampleUnitSphere(random(), random());
				//From the same paper eq 3:
				//integral_{\omega} of phaseFunction(theta) * cos(theta) dw 
				//where \omega = 4 PI (unit sphere)
				result += hgPhaseFunction->eval(incomingRay, scattered) * glm::dot(incomingRay, scattered);
			}

			result = result / (nSamples * spherePDF);
			EXPECT_TRUE(floatEqualsEpsilon(result, g, 0.05f));
			delete hgPhaseFunction;
		}
	}

	/**
	 * Tests the Henyey-Greenstein phase function.
	 */
	TEST(BxDF, HGPhaseFunction) {
		HG* hgPhaseFunction = new HG(0.1f);

		//int{\omega} expands to int{0}{2PI} int{0}{PI}
		//int{0}{2PI} int{0}{PI} p(theta) * sin(theta) dTheta dPhi = 1

		const int nSamplesPerRay = 100;
		double result = 0;
		//Varying from 0 to 2PI.
		float thetaAngle = 0;
		//Varying from 0 to PI.
		float phiAngle = 0;

		const int nSamplesThetaAngle = 100;
		const int nSamplesPhiAngle = 50;
		const float thetaStep = TWO_PI / nSamplesThetaAngle;
		const float phiStep = PI / nSamplesPhiAngle;
		int totalSamples = nSamplesThetaAngle * nSamplesPhiAngle * nSamplesPerRay;
		double buckets[nSamplesThetaAngle][nSamplesPhiAngle];

		for (int i = 0; i < nSamplesThetaAngle; i++)
			for (int j = 0; j < nSamplesPhiAngle; j++)
				buckets[i][j] = 0;

		for (int i = 0; i < nSamplesThetaAngle; i++) {
			for (int j = 0; j < nSamplesPhiAngle; j++) {
				glm::vec3 incomingRay = sampleUnitSphere(cos(thetaAngle), sin(thetaAngle), phiAngle);

				for (int n = 0; n < nSamplesPerRay; n++) {
					glm::vec3 scattered = glm::vec3(1, 0, 0);

					//Validate if the same theta angle is used for sin and cos.
					float angleTheta = glm::orientedAngle(incomingRay, scattered, glm::vec3(0, 0, 1));
					float cosTheta = glm::dot(glm::normalize(incomingRay), glm::normalize(scattered));
					float angleThetaFromCos = std::acosf(cosTheta);
					floatEqualsEpsilon(angleTheta, angleThetaFromCos, EPSILON3);

					buckets[i][j] += hgPhaseFunction->eval(incomingRay, scattered);
				}
				phiAngle += phiStep;
			}
			thetaAngle += thetaStep;
		}

		for (int i = 0; i < nSamplesThetaAngle; i++)
			for (int j = 0; j < nSamplesPhiAngle; j++)
				result += buckets[i][j];

		result = result / totalSamples;

		EXPECT_TRUE(floatEqualsEpsilon(result, INV4PI, EPSILON3));
	}

	/**
	 * Tests the isotropic phase function.
	 */
	TEST(BxDF, IsotropicPhaseFunction) {
		//An isotropic phase function over the whole sphere must integrate to 4*PI*r^2, or simply 4*PI for the unit sphere.
		IsotropicPhaseFunction* isotropicPhaseFunction = new IsotropicPhaseFunction();

		const int nSamplesPerRay = 1000;
		//If result is defined as float, the error goes way up at ~= 13.89.
		double result = 0;
		//Varying from 0 to 2PI.
		float thetaAngle = 0;
		//Varying from 0 to PI.
		float phiAngle = 0;

		const int nSamplesThetaAngle = 100;
		const int nSamplesPhiAngle = 50;
		const float thetaStep = TWO_PI / nSamplesThetaAngle;
		const float phiStep = PI / nSamplesPhiAngle;
		int totalSamples = nSamplesThetaAngle * nSamplesPhiAngle * nSamplesPerRay;
		double buckets[nSamplesThetaAngle][nSamplesPhiAngle];

		for (int i = 0; i < nSamplesThetaAngle; i++)
			for (int j = 0; j < nSamplesPhiAngle; j++)
				buckets[i][j] = 0;

		for (int i = 0; i < nSamplesThetaAngle; i++) {
			for (int j = 0; j < nSamplesPhiAngle; j++) {
				glm::vec3 incomingRay = sampleUnitSphere(cos(thetaAngle), sin(thetaAngle), phiAngle);

				for (int n = 0; n < nSamplesPerRay; n++) {
					glm::vec3 scattered = isotropicPhaseFunction->sample(incomingRay);
					buckets[i][j] += 1.0f / isotropicPhaseFunction->pdf(incomingRay, scattered);
				}
				phiAngle += phiStep;
			}
			thetaAngle += thetaStep;
		}

		for (int i = 0; i < nSamplesThetaAngle; i++)
			for (int j = 0; j < nSamplesPhiAngle; j++)
				result += buckets[i][j];

		//The integral must result in Approx. 4*PI.
		result = result / totalSamples;

		EXPECT_TRUE(floatEqualsEpsilon(result, FOUR_PI, EPSILON3));
	}

	/**
	 *	Tests the basic functionality of a VertexLayout struct.
	 */
	TEST(Model, vertexLayout) {
		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
		vertexLayout.end();

		EXPECT_EQ(vertexLayout.stride, sizeof(float) * 6);
		EXPECT_EQ(vertexLayout.offset[VertexAttrib::Position], 0);
		EXPECT_EQ(vertexLayout.offset[VertexAttrib::Normal], sizeof(float) * 3);

		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Normal, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		EXPECT_EQ(vertexLayout.stride, sizeof(float) * 6);
		EXPECT_EQ(vertexLayout.offset[VertexAttrib::Normal], 0);
		EXPECT_EQ(vertexLayout.offset[VertexAttrib::Position], sizeof(float) * 3);
	}

	/**
	 *	Tests for ray-triangle intersection cases.
	 */
	TEST(Triangle, intersection) {
		float precision = EPSILON3;
		const int nTriangles = 1;
		//Each vertex has 6 elements, 3 floats for position and 3 floats for normal.
		const int elementsPerVertex = 6;
		//A triangle is defined by 3 vertices.
		const int nVertices = 3;

		float vertexData[elementsPerVertex * nVertices] = {
			0,0,0, 0,0,-1,
			0,1,0, 0,0,-1,
			1,0,0, 0,0,-1
		};

		Triangle* triangle;
		triangle = new Triangle(&vertexData[0], &vertexData[6], &vertexData[12]);

		//Origin at vertex.
		Ray ray = { glm::vec3(0, 0, 0), glm::vec3(0, 0, 1) };
		RayIntersection rayIntersection;
		EXPECT_TRUE(triangle->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, 0, 0), precision));
		EXPECT_TRUE(rayIntersection.tNear == 0.0f, true) << "tNear was: " << rayIntersection.tNear << " but expected was: " << 0.0f;
		EXPECT_TRUE(rayIntersection.tFar == 0.0f, true) << "tFar was: " << rayIntersection.tFar << " but expected was: " << 0.0f;

		//Shooting straight into the vertex.
		ray = { glm::vec3(0, 0, -1), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(triangle->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, 0, 0), precision));
		EXPECT_TRUE(rayIntersection.tNear == 1.0f, true) << "tNear was: " << rayIntersection.tNear << " but expected was: " << 1.0f;
		EXPECT_TRUE(rayIntersection.tFar == 1.0f, true) << "tFar was: " << rayIntersection.tFar << " but expected was: " << 1.0f;

		//Shooting parallel to the triangle axis.
		//tNear and tFar should be NaN, thus the intersection returns false.
		ray = { glm::vec3(0, -1, 0), glm::vec3(0, 1, 0) };
		EXPECT_FALSE(triangle->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);

		//Shooting in the middle of the triangle.
		ray = { glm::vec3(0.25f, 0.25f, -1), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(triangle->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0.25f, 0.25f, 0), precision));
		EXPECT_TRUE(rayIntersection.tNear == 1.0f, true) << "tNear was: " << rayIntersection.tNear << " but expected was: " << 1.0f;
		EXPECT_TRUE(rayIntersection.tFar == 1.0f, true) << "tFar was: " << rayIntersection.tFar << " but expected was: " << 1.0f;

		//Origin behind the triangle and pointing away from it.
		ray = { glm::vec3(0, 0, 1), glm::vec3(0, 0, 1) };
		EXPECT_FALSE(triangle->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
	}

	/**
	 *	Tests for ray-AABB intersection cases.
	 */
	TEST(AABB, intersection) {
		//All tests are perform in Object Coordinate System (OCS).
		float precision = EPSILON3;
		//3 elements per vertex: x, y and z.
		const int elementsPerVertex = 3;
		//An AABB is defined by 2 vertices: min and max.
		const int nVertices = 2;

		float vertexData[elementsPerVertex * nVertices] = {
			-0.5, -0.5, -0.5,
			0.5, 0.5, 0.5
		};

		AABB* aabb;
		aabb = new AABB(&vertexData[0], &vertexData[3]);

		Ray ray;
		RayIntersection rayIntersection;

		//Shooting in the middle.
		ray = { glm::vec3(0, 0, -1.0), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(aabb->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, 0, -0.5), precision));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tNear, 0.5f, EPSILON3));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tFar, 1.5f, EPSILON3));

		//Origin on one of AABB's face.
		ray = { glm::vec3(0, 0, -0.5), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(aabb->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, 0, -0.5), precision));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tNear, 0.0f, EPSILON3));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tFar, 1.0f, EPSILON3));

		//Parallel to the bottom.
		ray = { glm::vec3(0, -0.5, -0.5), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(aabb->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, -0.5, -0.5), precision));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tNear, 0.0f, EPSILON3));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tFar, 1.0f, EPSILON3));

		//From inside. Returns negative tNear, i.e. "behind" the ray origin.
		ray = { glm::vec3(0, 0.0, 0.0), glm::vec3(0, 0, 1) };
		EXPECT_TRUE(aabb->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(vec3EqualsEpsilon(rayIntersection.hitPoint, glm::vec3(0, 0, -0.5), precision));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tNear, -0.5f, EPSILON3));
		EXPECT_TRUE(floatEqualsEpsilon(rayIntersection.tFar, 0.5f, EPSILON3));

		//Miss test.
		ray = { glm::vec3(0, 0.55, 0.0), glm::vec3(0, 0, 1) };
		EXPECT_FALSE(aabb->intersect(ray, rayIntersection)) << testing::PrintToString(ray) << testing::PrintToString(rayIntersection);
		EXPECT_TRUE(rayIntersection.tNear == INFINITY);
		EXPECT_TRUE(rayIntersection.tFar == -INFINITY);
	}

	/**
	 *	Tests for ray-model intersection cases.
	 */
	TEST(Model, modelMadeOfTriangles) {
		// Cube indices mapping.
		// Centered at origin with width equals 1;
		//   6 ---- 7
		//  /      / |
		// 2 ---- 3  |
		// |  4   |  5
		// | /    | /
		// 0 ---- 1
		//Cube vertex data
		float cube[8 * 3] = { -0.5f, -0.5f, -0.5f,
								0.5f, -0.5f, -0.5f,
								0.5f, 0.5f, -0.5f,
							   -0.5f, 0.5f, -0.5f,
							   -0.5f, -0.5f, 0.5f,
								0.5f, -0.5f, 0.5f,
								0.5f, 0.5f, 0.5f,
							   -0.5f, 0.5f, 0.5f };
		uint32_t cubeNTriangles = 6 * 2;
		uint32_t cubeNVertices = 8;
		uint32_t cubeIndices[6 * 2 * 3] = { 0,1,3,//back face
											1,2,3,
											5,1,2,//right face
											2,5,6,
											4,5,6,//front face
											6,4,7,
											4,0,3,//left face
											3,7,4,
											7,6,2,//top face
											2,3,7,
											4,5,1,//bottom face
											1,0,4 };
		MemoryBuffer vertexData;
		vertexData = { (uint8_t*)&cube[0], cubeNVertices * 3 * (uint32_t)sizeof(float)};
		MemoryBuffer indexData;
		indexData = { (uint8_t*)&(cubeIndices[0]), cubeNTriangles * 3 * (uint32_t)sizeof(uint32_t) };

		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		Model* model;
		model = new Model(vertexData, indexData, vertexLayout);

		//Verify that all primitives were allocated sequentially.
		Primitive* address = model->primitives[0];
		for (int i = 1; i < model->primitives.size(); i++) {
			ASSERT_EQ((Triangle*)address + 1, model->primitives[i]) << "failed at i = " << i;
			address = model->primitives[i];
		}

		//Shoots ray right in the middle of the model.
		Ray ray = {glm::vec3(0, 0, -2), glm::vec3(0, 0, 1)};
		RayIntersection ri;
		float min = 0;
		float max = std::numeric_limits<float>::infinity();
		EXPECT_TRUE(model->intersect(ray, ri, min, max));

		//Shoots ray in the right side of the model.
		ray = { glm::vec3(1.5, 0, -2), glm::vec3(0, 0, 1) };
		min = 0;
		max = std::numeric_limits<float>::infinity();
		EXPECT_FALSE(model->intersect(ray, ri, min, max));

		//Shoots ray from inside the model.
		ray = { glm::vec3(0, 0, 0), glm::vec3(0, 0, 1) };
		min = 0;
		max = std::numeric_limits<float>::infinity();
		EXPECT_TRUE(model->intersect(ray, ri, min, max));

		delete model;
	}

	/**
	 * Tests if allocations and freeing of Handles in a HandleAllocator works.
	 */
	TEST(HandleAllocator, allocatorMemoryManagement) {
		const int bufferSize = 512;

		uint8_t* handleAllocSpace1 = memAlloc(sizeof(HandleAllocator) + 2 * bufferSize * sizeof(HandleID));
		uint8_t* handleAllocSpace2 = memAlloc(sizeof(HandleAllocator) + 2 * bufferSize * sizeof(HandleID));
		HandleAllocator* handleAllocatorA;
		HandleAllocator* handleAllocatorB;
		handleAllocatorA = createHandleAllocator(handleAllocSpace1, bufferSize);
		handleAllocatorB = createHandleAllocator(handleAllocSpace2, bufferSize);
		
		//Allocate all handles available and free them in sequence.
		//After all allocations handleAllocatorA->sparse should be [0, 1, 2, ... nHandles].
		for (int i = 0; i < bufferSize; i++) {
			uint16_t id = handleAllocatorA->alloc();
			ASSERT_EQ(id, i);
			ASSERT_TRUE(handleAllocatorA->isValid(id));
			ASSERT_EQ(handleAllocatorA->nHandles, i + 1);
		}
		ASSERT_EQ(handleAllocatorA->nHandles, bufferSize);

		for (int i = 0; i < bufferSize; i++) {
			uint16_t id = handleAllocatorB->alloc();
			ASSERT_EQ(id, i);
			ASSERT_TRUE(handleAllocatorB->isValid(id));
			ASSERT_EQ(handleAllocatorB->nHandles, i + 1);
		}
		ASSERT_EQ(handleAllocatorB->nHandles, bufferSize);

		//Free all handles.
		for (int i = 0; i < bufferSize; i++) {
			ASSERT_TRUE(handleAllocatorA->isValid(i));
			ASSERT_EQ(handleAllocatorA->nHandles, bufferSize - i);
			handleAllocatorA->free(i);
			ASSERT_FALSE(handleAllocatorA->isValid(i));
		}
		ASSERT_EQ(handleAllocatorA->nHandles, 0);

		for (int i = 0; i < bufferSize; i++) {
			ASSERT_TRUE(handleAllocatorB->isValid(i));
			ASSERT_EQ(handleAllocatorB->nHandles, bufferSize - i);
			handleAllocatorB->free(i);
			ASSERT_FALSE(handleAllocatorB->isValid(i));
		}
		ASSERT_EQ(handleAllocatorB->nHandles, 0);

		//Fill allocator again and try to allocate over the capacity.
		for (int i = 0; i < bufferSize; i++) {
			uint16_t id = handleAllocatorA->alloc();
			ASSERT_EQ(id, bufferSize - i - 1);
			ASSERT_EQ(handleAllocatorA->nHandles, i + 1);
		}
		ASSERT_EQ(handleAllocatorA->nHandles, bufferSize);

		//Always return an invalid handle when trying to overallocate.
		uint16_t id = handleAllocatorA->alloc();
		ASSERT_EQ(id, INVALID_HANDLE);

		id = handleAllocatorA->alloc();
		ASSERT_EQ(id, INVALID_HANDLE);

		//Try to free element in the middle.
		handleAllocatorA->free(bufferSize / 2);
		ASSERT_EQ(handleAllocatorA->nHandles, bufferSize - 1);
		ASSERT_FALSE(handleAllocatorA->isValid(bufferSize / 2));

		//Try to free element in the middle again, that is now not allocated.
		EXPECT_FALSE(handleAllocatorA->isValid(bufferSize / 2));
		handleAllocatorA->free(bufferSize / 2);
		ASSERT_FALSE(handleAllocatorA->isValid(bufferSize / 2));

		//Try to free when there is no element allocated.
		ASSERT_DEATH(handleAllocatorB->free(0), "");
		ASSERT_FALSE(handleAllocatorA->isValid(0));

		//Fill allocator B again and free out of order.
		for (int i = 0; i < bufferSize; i++) {
			uint16_t id = handleAllocatorB->alloc();
			ASSERT_EQ(id, bufferSize - i - 1);
			ASSERT_EQ(handleAllocatorB->nHandles, i + 1);
		}
		ASSERT_EQ(handleAllocatorB->nHandles, bufferSize);

		//Fill vector with random handles.
		int rIndices[bufferSize];
		for (int i = 0; i < bufferSize; i++)
			rIndices[i] = i;

		//Randomly shuffle vector.
		for (int i = 0; i < bufferSize; i++) {
			int rIndex = random() * bufferSize;
			int tempHandle = rIndices[i];

			rIndices[i] = rIndices[rIndex];
			rIndices[rIndex] = tempHandle;
		}

		//Free out of order.
		for (int i = 0; i < bufferSize; i++) {
			ASSERT_TRUE(handleAllocatorB->isValid(i));
			ASSERT_EQ(handleAllocatorB->nHandles, bufferSize - i);
			handleAllocatorB->free(i);
			ASSERT_FALSE(handleAllocatorB->isValid(i));
		}
	}

	TEST(VolumetricPathIntegrator, visibilityTr) {
		VolumetricPathIntegrator volPath;
		SceneReader sceneReader;
		sceneReader.loadScene("scenes/testing/singleVoxelVolume.json", false);
		Scene *scene = sceneReader.getScene();

		InstancedModel *imLight = scene->lights.at(0);
		Material *lightMaterial = scene->lights.at(0)->model->materials.at(0);
		GridMedia* gridMedia = dynamic_cast<GridMedia*>(lightMaterial->medium);
		//float density = gridMedia->invMaxDensity;

		glm::vec3 lightPoint;
		glm::vec3 origin;
		glm::vec3 visibilityTr;

		lightPoint = glm::vec3(-0.4, 2, 0);
		origin = glm::vec3(-0.4, 0, 0);
		visibilityTr = volPath.visibilityTr(origin, lightPoint, scene);
		//Passes through the medium and reaches the light.
		EXPECT_TRUE(vec3EqualsEpsilon(visibilityTr, glm::vec3(1), EPSILON3));

		lightPoint = glm::vec3(0.4, 2, 0);
		origin = glm::vec3(0.4, 0, 0);
		visibilityTr = volPath.visibilityTr(origin, lightPoint, scene);
		//Passes through the medium and reaches the surface above it.
		EXPECT_TRUE(vec3EqualsEpsilon(visibilityTr, glm::vec3(0), EPSILON3));
	}

	TEST(VolumetricPathIntegrator, estimateDirectSurfaceAndLight) {
		VolumetricPathIntegrator volPath;
		SceneReader sceneReader;
		sceneReader.loadScene("scenes/testing/surfaceAndLight.json", false);
		Scene* scene = sceneReader.getScene();

		Ray incoming;
		RayIntersection hit;
		//Instead of randomly, select the light we want to sample from. There is only one in the scene anyway.
		InstancedModel* lightIM = scene->lights.at(0);
		Light* light = scene->lights.at(0)->model->materials.at(0)->light;
		glm::vec3 lightLi = light->Li();


		//NOTE that we define the incoming direction pointing TOWARDS the hit point. But when this same incoming direction
		// is passed to the BxDF and converted to the Spherical Coordiante System (SCS) it is negated in order to be in the same
		// hemisphere as the normal.
		// incoming
		// \   ^
		//	\  |  normal
		//	 \ |
		//    v|
		//-----x--------------- surface
		incoming.origin = glm::vec3(0, EPSILON3, 0);
		incoming.direction = glm::vec3(0, -1, 0);
		EXPECT_TRUE(scene->intersectScene(incoming, hit, 0, INFINITY));
		glm::vec3 Li = volPath.estimateDirect(incoming, hit, light, lightIM, scene);
		EXPECT_TRUE(vec3EqualsEpsilon(Li, lightLi, EPSILON3));

	}

	TEST(Triangle, samplePointOntexture) {
		glm::vec3 v1 = { 0.00000000, -0.0685599968, -1.00000000 };
		glm::vec3 v2 = { -0.382234007, 8.78874969, -0.923610985 };
		glm::vec3 v3 = { -0.194640994, 8.78874969, -0.980517030 };
		glm::vec2 uv1 = { 0.00396300014, 0.866797984 };
		glm::vec2 uv2 = { 0.00000000, 0.587139964 };
		glm::vec2 uv3 = { 0.0143640004, 0.587136030 };
		float data[3 * 3 * 3 * 2] = {
			v1[0], v1[1], v1[2], uv1[0], uv1[1],
			v2[0], v2[1], v2[2], uv2[0], uv2[1],
			v3[0], v3[1], v3[2], uv3[0], uv3[1]
		};
		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.add(VertexAttrib::TexCoord0, VertexAttribType::Float, 2);
		vertexLayout.end();

		Triangle* triangle = new Triangle(&data[0], &data[3+2], &data[(3+2)*2]);
		triangle->vertexLayout = &vertexLayout;

		//Validate if point is in fact over the surface.
		glm::vec3 pointOnSurface = { -0.151023865, -0.0685615540, 0.108901978 };
		//EXPECT_TRUE(isPointInsideTriangle(pointOnSurface, v1, v2, v3));

		//Texture UV must be in the range: [0, 1].
		glm::vec2 uvSampled = triangle->samplePointOnTexture(pointOnSurface);
		EXPECT_TRUE(uvSampled.x < 1.0f);
		EXPECT_TRUE(uvSampled.y < 1.0f);
		EXPECT_TRUE(uvSampled.x >= 0.0f);
		EXPECT_TRUE(uvSampled.y >= 0.0f);
	}

	TEST(Triangle, area) {
		//Triangle Area = 1/2 * base * height
		//or half the Rectangle Area 
		float precision = EPSILON3;
		std::vector<float> data = {
			0, 0, 0,
			1, 0, 0,
			0, 1, 0
		};

		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		Triangle* triangle = new Triangle(&data[0], &data[3], &data[6]);
		triangle->vertexLayout = &vertexLayout;
		float calculatedArea;

		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 1.0/2.0, precision));

		data = {
			0, 0, 0,
			0, 2, 0,
			2, 0, 0
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 4.0/2.0, precision));

		data = {
			0, 0, 0,
			0, 2, 0,
			0, 0, 2
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 4.0 / 2.0, precision));

		data = {
			0, 0, 0,
			0, 2, 0,
			-2, 0, 0
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 4.0 / 2.0, precision));

		data = {
			0, 0, 0,
			0, 2, 0,
			0, 0, -2
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 4.0 / 2.0, precision));

		data = {
			15, 0, 15,
			15, 0, 16,
			15, 1, 15
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 1.0 / 2.0, precision));

		data = {
			-15, 0, -15,
			-15, 0, -16,
			-15, -1, -15
		};
		calculatedArea = triangle->calculateArea();
		EXPECT_TRUE(floatEqualsEpsilon(calculatedArea, 1.0 / 2.0, precision));
	}

	TEST(Math, isPointInsideTriangleRange) {
		glm::vec3 v1 = {0,0,0};
		glm::vec3 v2 = {1,0,0};
		glm::vec3 v3 = {0,1,0};
		//Inside the triangle.
		EXPECT_TRUE(isPointInsideTriangleRange(glm::vec3(0.4, 0.4, 0.0), v1, v2, v3));
		//Test on line segment
		EXPECT_TRUE(isPointInsideTriangleRange(glm::vec3(0.5, 0.5, 0.0), v1, v2, v3));
		//Test on vertices.
		EXPECT_TRUE(isPointInsideTriangleRange(v1, v1, v2, v3));
		EXPECT_TRUE(isPointInsideTriangleRange(v2, v1, v2, v3));
		EXPECT_TRUE(isPointInsideTriangleRange(v3, v1, v2, v3));
		//Just outside.
		EXPECT_FALSE(isPointInsideTriangleRange(glm::vec3(0.6, 0.6, 0.0), v1, v2, v3));
		EXPECT_FALSE(isPointInsideTriangleRange(glm::vec3(-0.1, -0.1, 0.0), v1, v2, v3));
		//Inside the range, but at different depth. Note that the test does not verify if the point is in the triangle's surface.
		EXPECT_TRUE(isPointInsideTriangleRange(glm::vec3(0.4, 0.4, 0.1), v1, v2, v3));
	}

	TEST(Triangle, contains) {
		float precision = EPSILON3;
		std::vector<float> data = {
			0, 0, 0,
			1, 0, 0,
			0, 1, 0
		};

		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		Triangle* triangle = new Triangle(&data[0], &data[3], &data[6]);
		triangle->vertexLayout = &vertexLayout;

		EXPECT_TRUE(triangle->contains(glm::vec3(0, 0, 0)));
		EXPECT_TRUE(triangle->contains(glm::vec3(1, 0, 0)));
		EXPECT_TRUE(triangle->contains(glm::vec3(0, 1, 0)));
		EXPECT_TRUE(triangle->contains(glm::vec3(0.3, 0.3, 0)));
		EXPECT_FALSE(triangle->contains(glm::vec3(0.6, 0.6, 0)));
		EXPECT_FALSE(triangle->contains(glm::vec3(0.3, 0.3, 0.1)));
		EXPECT_FALSE(triangle->contains(glm::vec3(-0.3, -0.3, 0.1)));
		EXPECT_FALSE(triangle->contains(glm::vec3(-0.3, -0.3, 0)));

		data = {
		   0, 0, 0,
		   1000, 0, 0,
		   0, 1000, 0
		};
		EXPECT_FALSE(triangle->contains(glm::vec3(0.3, 0.3, 0.1)));
		EXPECT_FALSE(triangle->contains(glm::vec3(0.3, 0.3, 0.01)));
		EXPECT_FALSE(triangle->contains(glm::vec3(0.3, 0.3, 0.001)));
		EXPECT_FALSE(triangle->contains(glm::vec3(0.3, 0.3, 0.0001)));
	}

	TEST(Triangle, barycentricCoordinates) {
		float precision = EPSILON3;
		std::vector<float> data = {
			0, 0, 0,
			1, 0, 0,
			0, 1, 0
		};

		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		Triangle* triangle = new Triangle(&data[0], &data[3], &data[6]);
		triangle->vertexLayout = &vertexLayout;
		glm::vec3 barycentricCoords;

		//Weights on each vertex.
		barycentricCoords = triangle->barycentricCoordinates(glm::vec3(0, 0, 0), triangle->getVertex(0), triangle->getVertex(1), triangle->getVertex(2));
		EXPECT_TRUE(vec3EqualsEpsilon(barycentricCoords, glm::vec3(1, 0, 0), precision));

		barycentricCoords = triangle->barycentricCoordinates(glm::vec3(1, 0, 0), triangle->getVertex(0), triangle->getVertex(1), triangle->getVertex(2));
		EXPECT_TRUE(vec3EqualsEpsilon(barycentricCoords, glm::vec3(0, 1, 0), precision));

		barycentricCoords = triangle->barycentricCoordinates(glm::vec3(0, 1, 0), triangle->getVertex(0), triangle->getVertex(1), triangle->getVertex(2));
		EXPECT_TRUE(vec3EqualsEpsilon(barycentricCoords, glm::vec3(0, 0, 1), precision));

		//Weight for a point outside the triangle.
		barycentricCoords = triangle->barycentricCoordinates(glm::vec3(1, 1, 0), triangle->getVertex(0), triangle->getVertex(1), triangle->getVertex(2));
		EXPECT_TRUE(vec3EqualsEpsilon(barycentricCoords, glm::vec3(-1, 1, 1), precision));
	}

	TEST(Triangle, samplePointOnSurface) {
		const int nRuns = 10000;
		std::vector<float> data = {
			0, 0, 0,
			1, 0, 0,
			0, 1, 0
		};

		VertexLayout vertexLayout;
		vertexLayout.init();
		vertexLayout.add(VertexAttrib::Position, VertexAttribType::Float, 3);
		vertexLayout.end();

		Triangle* triangle = new Triangle(&data[0], &data[3], &data[6]);
		triangle->vertexLayout = &vertexLayout;

		RayIntersection ri = {};
		glm::mat4 toWCS = glm::mat4(1);

		for (int i = 0; i < nRuns; i++) {
			glm::vec3 point = triangle->samplePointOnSurface(ri, toWCS);
			EXPECT_TRUE(triangle->contains(point));
		}
	}

	TEST(BVH, initAndIntersect) {
		float precision = EPSILON3;
		//Load cube model with range [-1, 1].
		ResourceManager::getSelf()->loadModel("cube", "models/", "cube.obj");
		Model* model = ResourceManager::getSelf()->getModel("cube");

		BVH bvh;
		bvh.init(model);
		EXPECT_TRUE(bvh.nodeCount, 5);

		Ray ray;
		RayIntersection ri;
		
		ray = { glm::vec3(0,0,-2), glm::vec3(0,0,1) };
		EXPECT_TRUE(bvh.intersect(ray, ri));
		EXPECT_TRUE(floatEqualsEpsilon(ri.tNear, 1, precision));
		//EXPECT_TRUE(floatEqualsEpsilon(ri.tFar, 1, precision));

		ray = { glm::vec3(0,0,0), glm::vec3(0,0,1) };
		EXPECT_TRUE(bvh.intersect(ray, ri));
		EXPECT_TRUE(floatEqualsEpsilon(ri.tNear, 1, precision));
		//EXPECT_TRUE(floatEqualsEpsilon(ri.tFar, 1, precision));
	}
}