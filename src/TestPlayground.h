#pragma once
#include <glm/glm.hpp>
#include "RoughConductorBRDF.h"
#include "Math.h"
#include "MaterialBRDF.h"
#include "OBB.h"
#include "GridMaterial.h"

class TestPlayground
{
public:

	void test() {
		glm::vec3 albedo = glm::vec3(0.722, 0.451, 0.2);
		RoughConductorBRDF *brdf = new RoughConductorBRDF(0.01f, 1.0f, albedo);
		glm::vec3 incomingDir = glm::normalize(glm::vec3(1, -1, 0));
		Ray incoming{ glm::vec3(-100,100,0), incomingDir };
		Ray scattered;
		Hit hit;
		hit.normal = glm::vec3(0, 1, 0);
		hit.p = glm::vec3(-10, 10, 0);
		float pdf = 0;

		brdf->sample(incoming, scattered, hit);
		//scattered.d = glm::reflect(glm::normalize(incoming.d), glm::normalize(hit.normal));
		//scattered.d = glm::normalize(glm::vec3(1, 0.01, 0));
		glm::vec3 scatteredDir = scattered.direction;
		glm::vec3 norm = glm::normalize(scatteredDir);
		glm::vec3 res = brdf->eval(incoming, scattered, hit, pdf);
		glm::vec3 resWithPdf = res * 1.0f / pdf;
		std::cout << "incoming: \t";
		printVec3(-incoming.direction);
		std::cout << "scattered: \t";
		printVec3(scattered.direction);
		std::cout << "albedo: \t";
		printVec3(albedo);
		std::cout << "BRDF: \t\t";
		printVec3(res);
		std::cout << "PDF: \t\t";
		std::cout << pdf << std::endl;
		std::cout << "BRDF * PDF: \t";
		printVec3(res*pdf);
		std::cout << "BRDF * 1/PDF: \t";
		printVec3(res * 1.0f / pdf);
		float angle = glm::dot(-incoming.direction, scatteredDir);
		int stop = 0;

		while (true)
			continue;
	}

	void testCS() {
		float theta, phi, r = 1;
		glm::vec3 w = glm::normalize(glm::vec3(1, 0, 0));
		glm::vec3 v, u;
		generateOrthonormalCS(w, v, u);

		theta = glm::radians(45.0f);
		phi = glm::radians(0.0f);
		glm::vec3 dir = sphericalToCartesian(r, theta, phi);
		glm::vec3 finalDir = toWorld(dir, w, u, v);
		glm::vec3 fromRHtoLH;
		fromRHtoLH.x = dir.y;
		fromRHtoLH.y = dir.z;
		fromRHtoLH.z = -dir.x;

		std::cout << "w: ";
		printVec3(w);
		std::cout << "u: ";
		printVec3(u);
		std::cout << "v: ";
		printVec3(v);
		std::cout << "dirCart: ";
		printVec3(dir);
		std::cout << "dirFinal: ";
		printVec3(finalDir);
		std::cout << "fromRHtoLH: ";
		printVec3(fromRHtoLH);

		std::cout << "--------------------------------" << std::endl;
		//RH
		w = glm::vec3(0, 1, 0);
		u = glm::vec3(1, 0, 0);
		v = glm::vec3(0, 0, -1);
		dir = glm::vec3(1, 1, 1);
		finalDir = toWorld(dir, w, u, v); //converts to LH
		std::cout << "w: ";
		printVec3(w);
		std::cout << "u: ";
		printVec3(u);
		std::cout << "v: ";
		printVec3(v);
		std::cout << "dir: ";
		printVec3(dir);
		std::cout << "finalDir: ";
		printVec3(finalDir);

		float len = glm::length2(glm::vec2(-4, 1));
		float len2 = glm::length2(glm::vec2(-1, 2));
		std::cout << len << std::endl;
		std::cout << len2;

		while (true)
			continue;
	}


	//TUDO OK pros 4 quadrantes
	void testLightDirSample() {
		glm::vec3 spherePos[8];
		spherePos[1] = glm::vec3(-3, 3, 1);
		spherePos[0] = glm::vec3(3, 3, 1);
		spherePos[2] = glm::vec3(-3, -3, 1);
		spherePos[3] = glm::vec3(3, -3, 1);
		spherePos[4] = glm::vec3(-3, 3, -1);
		spherePos[5] = glm::vec3(3, 3, -1);
		spherePos[6] = glm::vec3(-3, -3, -1);
		spherePos[7] = glm::vec3(3, -3, -1);

		glm::vec3 surfaceP = glm::vec3(0, 0, 0);
		float r = 1.0f;

		for (int i = 0; i < 8; i++) {
			//spherePos[i].x *= 101;
			//spherePos[i].y *= 101;
			//spherePos[i].z *= 303;
			glm::vec4 sphereDir = sampleSphere(surfaceP, spherePos[i], r);

			std::cout << "sphr - surf: ";
			printVec3(glm::normalize(spherePos[i] - surfaceP));
			std::cout << "sample:   ";
			printVec3(sphereDir);
			std::cout << "PDF: " << sphereDir.w << std::endl;
			std::cout << "----------------------------" << std::endl;
		}

		while (true)
			continue;
	}

	void testBRDFPDF() {
		glm::vec3 albedo = glm::vec3(0.722, 0.451, 0.2);
		RoughConductorBRDF *brdf = new RoughConductorBRDF(0.01f, 1.0f, albedo);
		Hit hit;
		glm::vec3 hitpoint = glm::vec3(1, 0, 0);
		glm::vec3 incomingPoint[6];
		incomingPoint[0] = glm::vec3(0, 0, 0);
		incomingPoint[1] = glm::vec3(0, 1, 0);
		incomingPoint[2] = glm::vec3(0, 0.5, 0);
		incomingPoint[3] = glm::vec3(0.5, 0.5, 0);
		incomingPoint[4] = glm::vec3(0.5, 1.0, 0);
		incomingPoint[5] = glm::vec3(0.75, 1.0, 0);

		for (int i = 0; i < 6; i++) {
			Ray incoming;
			incoming.o = incomingPoint[i];
			incoming.d = glm::normalize(hitpoint - incomingPoint[i]);
			hit.p = hitpoint;
			hit.normal = glm::vec3(0, 1, 0);
			Ray scattered;
			scattered.o = hitpoint;
			scattered.d = glm::normalize(glm::vec3(1, 1, 0));
			float pdf;
			glm::vec3 res = brdf->eval(incoming, scattered, hit, pdf);

			std::cout << "in.d: ";
			printVec3(incoming.d);
			std::cout << "sc.d: ";
			printVec3(scattered.d);
			std::cout << "pdf: " << pdf << std::endl;
			std::cout << "---------------" << std::endl;
		}

		while (true)
			continue;
	}

	void testPlate1() {
		glm::vec3 albedo = glm::vec3(0.722, 0.451, 0.2);
		RoughConductorBRDF *brdf = new RoughConductorBRDF(0.01f, 1.0f, albedo);
		MaterialBRDF *m = new MaterialBRDF(brdf, albedo);
		glm::vec3 lightPos(0, 7.5, 8.1);

		glm::vec3 platePos(0, 4.5, 10.5);
		glm::vec3 scale(12, 0.3, 2);
		glm::vec3 rotate(65, 0, 0);
		OBB *obb = new OBB(platePos, scale, rotate, m);

		Ray incoming{ glm::vec3(0.1f, 4.5, 0.1), glm::vec3(0, 0, 1) };
		Ray scattered;
		Hit hit;
		bool didHit = obb->hit(incoming, 0.0f, 99999.0f, hit);

		float pdf;
		brdf->sample(incoming, scattered, hit);
		glm::vec3 brdfRes = brdf->eval(incoming, scattered, hit, pdf);

		//printMat4(obb->model);
		glm::vec3 H = glm::normalize(-incoming.d + scattered.d);
		float NdotH = glm::dot(hit.normal, H);
		float D = brdf->ggxD(NdotH);

		std::cout << didHit << std::endl;
		std::cout << "in.d: ";
		printVec3(incoming.d);
		std::cout << "sct.d: ";
		printVec3(scattered.d);
		std::cout << "N: ";
		printVec3(hit.normal);
		std::cout << "NdotH: " << NdotH << std::endl;
		std::cout << "D: " << D << std::endl;
		std::cout << "H: ";
		printVec3(H);
		std::cout << "brdf: ";
		printVec3(brdfRes);
		while (true)
			continue;
	}

	void testVolume() {
		glm::vec3 platePos(0, 4.5, 10.5);
		glm::vec3 scale(3, 3, 3);
		glm::vec3 rotate(0, 0, 0);
		GridMaterial *gm = dynamic_cast<GridMaterial*>(ResourceManager::getSelf()->getMaterial("volumeMat"));
		OBB *obb = new OBB(platePos, scale, rotate, gm);

		Ray incoming{ glm::vec3(0.75f, 4.5, -10), glm::normalize(glm::vec3(0.0001f, 0.0001f, 1)) };
		Ray scattered;
		Hit hit;
		bool didHit = obb->hit(incoming, 0.0f, 99999.0f, hit);

		//	float t = 0.6f;
		float t = -std::log(1 - random()) / gm->extinctionAvg;
		incoming.o = incoming.o + t * incoming.d;
		glm::vec3 thit = gm->lbvh->traverseTreeUntil(incoming, t);
		bool didHitLBVH = false;
		if (thit.x > thit.y)
			didHitLBVH = false;
		else
			didHitLBVH = true;

		incoming.o = glm::vec3(0.75f, 4.5, -10);
		incoming.o = incoming.o + (hit.tFar - 0.1f) * incoming.d;
		//incoming.o = platePos + 100.1f;
		didHit = obb->hit(incoming, -0.001f, 99999.0f, hit);

		float k = 0;
		while (true)
			continue;
	}

	void testNDFIntegral() {
		int samples = 10000;
		float sum = 0;
		glm::vec3 albedo = glm::vec3(0.1, 0.1, 0.1);
		RoughConductorBRDF *brdf = new RoughConductorBRDF(0.01f, 1.0f, albedo);
		MaterialBRDF *m = new MaterialBRDF(brdf, albedo);
		glm::vec3 platePos(0, 0, 0);
		glm::vec3 scale(1, 0.1, 1);
		glm::vec3 rotate(0, 0, 0);
		OBB *obb = new OBB(platePos, scale, rotate, m);

		for (int i = 0; i < samples; i++) {
			glm::vec3 hemispherePoint = sampleUnitHemisphere();
			glm::vec3 v, u;
			glm::vec3 yup = glm::vec3(0, 1, 0);
			glm::mat3 orthonormalCS = generateOrthonormalCS(yup, v, u);
			hemispherePoint = glm::normalize(toWorld(hemispherePoint, yup, v, u));

			Ray incoming{ hemispherePoint, -hemispherePoint };
			Ray scattered;
			Hit hit;
			bool didHit = obb->hit(incoming, 0.0f, 99999.0f, hit);

			// GGX NDF sampling
			glm::vec2 r = glm::vec2(random(), random());
			float a2 = brdf->a * brdf->a;
			float theta = std::acos(std::sqrt((1.0f - r.x) / glm::max(float(EPSILON), (r.x * (a2 - 1.0f) + 1.0f))));
			float phi = TWO_PI * r.y;
			float cosTheta = cos(theta);
			float sinTheta = sin(theta);
			glm::vec3 microfacet = glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			orthonormalCS = generateOrthonormalCS(hit.normal, v, u);
			microfacet = glm::normalize(toWorld(microfacet, hit.normal, v, u));

			glm::vec3 scatteredDir = glm::reflect(glm::normalize(incoming.d), glm::normalize(microfacet));
			scattered.o = hit.p;
			scattered.d = scatteredDir;

			glm::vec3 halfway = glm::normalize(-incoming.d + scattered.d);
			float NdotH = glm::max(0.0f, glm::dot(hit.normal, halfway));
			float sinThetaInv = std::sin(std::acos(NdotH));

			if (!didHit)
				std::cout << "didNotHit" << std::endl;

			if (hemispherePoint.y < 0)
				std::cout << "hemisphere wrong" << std::endl;

			if (NdotH < 0)
				std::cout << "dot negative" << std::endl;

			float denom = cosTheta * cosTheta * (a2 - 1.0f) + 1.0f;
			denom = glm::max(EPSILON, PI * denom * denom);
			float D_ggx = a2 / (denom);

			float numerator = (D_ggx * cosTheta * sinTheta);
			//PDF for spherical coordinates is this:
			float pdf = glm::max(float(EPSILON), D_ggx * cosTheta * sinTheta);

			//Integral over spherical coordinates of D_ggx * cos(theta) * sin(theta) = 1
			sum += numerator / pdf;
		}

		float result = sum / samples;

		std::cout << "D(h)x(HoN) = " << result;
		while (true)
			continue;

	}
	TestPlayground();
	~TestPlayground();
};

