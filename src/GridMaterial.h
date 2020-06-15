#pragma once
#include "Material.h"
#include "LBVH.h"
#include "LBVH2.h"
#include "Math.h"
#include <vector>
#include <math.h>

enum PhaseFunction {
	ISOTROPIC,
	RAYLEIGH,
	HENYEY_GREENSTEIN
};

class GridMaterial : public Material {
public:
	glm::vec3 albedo;

	/*GridMaterial(LBVH2 *lbvh) {
		this->lbvh = lbvh;
	};*/

	GridMaterial(glm::vec3 scattering, glm::vec3 absorption, PhaseFunction phaseFunction, std::string material) {
		this->scattering = scattering;
		this->absorption = absorption;
		this->extinction = absorption + scattering;
		extinctionAvg = (extinction.x + extinction.y + extinction.z) / 3.0f;
		absorptionAvg = (absorption.x + absorption.y + absorption.z) / 3.0f;
		phaseFunctionOption = phaseFunction;

		lbvh = new LBVH2(ResourceManager::getSelf()->getTexture3D(material)->floatData, ResourceManager::getSelf()->getTexture3D(material)->getResolution());
	}

	LBVH2 *lbvh;
	glm::vec3 scattering = glm::vec3(0.45f, 0.25f, 0.25f);
	glm::vec3 absorption = glm::vec3(1.801f, 0.801f, 0.801f);
	glm::vec3 extinction = absorption + scattering;
	float extinctionAvg = (extinction.x + extinction.y + extinction.z) / 3.0f;
	float absorptionAvg = (absorption.x + absorption.y + absorption.z) / 3.0f;
	glm::vec3 lightPosition = glm::vec3(0, 1, 2);
	float ambientStrength = 300.0;
	float Kc = 1.0, Kl = 0.7, Kq = 1.8;
	float transmittanceThreshold = 0.05f;
	float shadowSteps = 5;
	float numberOfSteps = 120;
	PhaseFunction phaseFunctionOption = ISOTROPIC;
	float g = 0.9f;

	float sampleVolume(glm::vec3 pos) {
		return lbvh->grid[to1D(lbvh->gridSize.x, lbvh->gridSize.y, pos.x, pos.y, pos.z)];
	}

	float isotropicPhaseFunction() {
		return 1 / (4 * PI);
	}

	float rayleighPhaseFunction(float theta) {
		float cosAngle = cos(theta);
		return (3 * (1 + cosAngle * cosAngle)) / (16 * PI);
	}

	float henyeyGreensteinPhaseFunction(float cosTheta, float g) {
		float g2 = g * g;
		return  (0.25 / PI) * (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
	}

	float phaseFunction(float theta, float g) {
		if (phaseFunctionOption == ISOTROPIC)
			return isotropicPhaseFunction();

		if (phaseFunctionOption == RAYLEIGH)
			return rayleighPhaseFunction(theta);

		if (phaseFunctionOption == HENYEY_GREENSTEIN)
			return henyeyGreensteinPhaseFunction(theta, g);

		return 1;
	}

	glm::vec3 volumetricShadow(glm::vec3 cubePos, glm::vec3 lightPos) {

		glm::vec3 transmittance = glm::vec3(1.0f);
		float distance = length(lightPos - cubePos) / shadowSteps;
		glm::vec3 lightDir = normalize(lightPos - cubePos);
		float stepSizeShadow = 1.0f / shadowSteps;

		for (float tshadow = stepSizeShadow; tshadow < distance; tshadow += stepSizeShadow) {
			glm::vec3 cubeShadowPos = cubePos + tshadow * lightDir;

			float density = sampleVolume(cubeShadowPos);

			transmittance *= exp(-extinction * density);
			if (transmittance.x < transmittanceThreshold)
				break;
		}
		return transmittance;
	}

	float evaluateLight(glm::vec3 pos) {
		float d = length(lightPosition - pos);
		return (1.0f / (Kc + Kl * d + Kq * d * d));
	}


	void rayMarching(Ray incomingRay, glm::vec2 tHit, glm::vec3 &transmittance, glm::vec3 &inScattering) {

		tHit.x = glm::max(tHit.x, 0.0f);
		glm::vec3 absDir = glm::abs(incomingRay.direction);
		float dt = 1.0f / (128 * glm::max(absDir.x, glm::max(absDir.y, absDir.z)));
		float lightDotEye = dot(normalize(incomingRay.direction), normalize(lightPosition));
		glm::vec3 cubePos = incomingRay.origin + tHit.x * incomingRay.direction;
		glm::vec3 totalTr = glm::vec3(1.0f);
		glm::vec3 sum = glm::vec3(0.0f);
		glm::vec3 currentTr = glm::vec3(0);

		for (float t = tHit.x; t < tHit.y; t += dt) {
			float density = sampleVolume(cubePos);
			if (density == 0) {
				cubePos += incomingRay.direction * dt;
				continue;
			}

			//If accumulated transmittance is near zero, there's no point in continue calculating transmittance
			if (totalTr.x > transmittanceThreshold) {
				currentTr = exp(-extinction * density);
				totalTr *= currentTr;
			}

			glm::vec3 Ls = ambientStrength * evaluateLight(cubePos) * volumetricShadow(cubePos, lightPosition) * phaseFunction(lightDotEye, g);
			//Integrate Ls from 0 to d
			Ls = (Ls - Ls * currentTr) / extinction;

			sum += totalTr * scattering * Ls;
			cubePos += incomingRay.direction * dt;
		}

		transmittance = totalTr;
		inScattering = sum;
	}

	bool integrate(Ray incomingRay, glm::vec2 tHit, glm::vec3 &transmittance, glm::vec3 &inScattering, float expp) {

		tHit.x = glm::max(tHit.x, 0.0f);
		glm::vec3 absDir = abs(incomingRay.direction);
		float dt = 1.0f / (numberOfSteps * glm::max(absDir.x, glm::max(absDir.y, absDir.z)));
		float lightDotEye = glm::dot(glm::normalize(incomingRay.direction), glm::normalize(lightPosition));
		glm::vec3 cubePos = incomingRay.origin + tHit.x * incomingRay.direction;
		glm::vec3 totalTr = glm::vec3(1.0f);
		glm::vec3 sum = glm::vec3(0.0f);
		glm::vec3 currentTr = glm::vec3(0);


			//float density = sampleVolume(cubePos);

			//If accumulated transmittance is near zero, there's no point in continue calculating transmittance
			//if (totalTr.x > transmittanceThreshold) {
				//currentTr = exp(-extinction * density);
				//totalTr *= currentTr;
			//}
		totalTr = exp(-extinction * expp);

		//glm::vec3 Ls = glm::vec3(phaseFunction(lightDotEye, g));
		//glm::vec3 Ls = 1.0f * volumetricShadow(cubePos, lightPosition) * phaseFunction(lightDotEye, g);
		//Integrate Ls from 0 to d
		//Ls = (Ls - Ls * currentTr) / extinction;

		//	sum += totalTr * scattering * Ls;
		//	cubePos += incomingRay.direction * dt;

		transmittance = totalTr;
		inScattering = sum;
		return true;
	}

	//Source: Production Volume Rendering - SIGGRAPH 2017 pg. 14.
	bool deltaTracking(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered) {
		float t = -std::log(1 - random()) / 0;
		float sigma_n = 0.2f; //fictitious medium
		glm::vec3 thit = lbvh->traverseTreeUntil(rayIn, t);
		
		//boundary has been hit
		if (t > thit.x)
			return true;

		glm::vec3 x = rayIn.o - t * rayIn.d;
		if (random() < absorptionAvg / extinctionAvg)
			return true;
		else if (random() < 1 - 0)
			return false;
	}

	//Also know as closed-form tracking
	//Source: Production Volume Rendering - SIGGRAPH 2017 pg. 12.
	bool regularTracking(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered) {
		float t = -std::log(1 - random()) / extinctionAvg;
		glm::vec3 thit = lbvh->traverseTreeUntil(rayIn, t);

		if (t > thit.x ){
			scattered = Ray(rayIn.o + t * rayIn.d , rayIn.d);
		}

		glm::vec3 x = rayIn.o - t * rayIn.d;
		// An absorption/emission collision has occured
		if (t < absorptionAvg / extinctionAvg) {
			attenuation = exp(-extinction * thit.z);
		}else if (thit.z != 0) {
			//sample phase function (in this case, isotropic)
			scattered = Ray(rayIn.o + t * rayIn.d, sampleUnitSphere());
		}
		else if(thit.z == 0) {
			scattered = Ray(rayIn.o + t * rayIn.d, rayIn.d);
		}

		//p46
		return true;
	}

	bool regularTracking2(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered) {
		float t = -std::log(1 - random()) / extinctionAvg;
		glm::vec3 thit = lbvh->traverseTreeUntil(rayIn, t);


		glm::vec3 x = rayIn.o - t * rayIn.d;

		// An absorption/emission collision has occured
		if (t < absorptionAvg / extinctionAvg) {
			attenuation = exp(-extinction * thit.z);
		}
		scattered = Ray(rayIn.o + t * rayIn.d, sampleUnitSphere());


		return true;
	}

	bool test(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered) {
		attenuation = exp(-extinction * 0.4f);
		scattered = Ray(hit.p, sampleUnitSphere());
		return true;
	}

	bool workingOne(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered) {
		glm::vec3 transmittance(1.0f);
		glm::vec3 inScattering(0.0f);

		glm::vec3 thit = lbvh->traverseTreeUntil(rayIn, hit.t);

		integrate(rayIn, thit, transmittance, inScattering, thit.z);
		//rayMarching(rayIn, thit, transmittance, inScattering);
		if (thit.z == 0)
			scattered = Ray(hit.p, rayIn.d);
		else
			//scattered = Ray(hit.p, sampleUnitSphere());
			scattered = Ray(rayIn.o + hit.t * rayIn.d, sampleUnitSphere());

		attenuation = transmittance + inScattering;
		return true;
	}

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {
		//return test(rayIn, hit, attenuation, scattered);
		return workingOne(rayIn, hit, attenuation, scattered);
	};
};