#pragma once
#include "Material.h"
#include "BeckmannBRDF.h"
#include "RoughConductorBRDF.h"
#include "Math.h"

class MaterialBRDF : public Material {
public:
	glm::vec3 albedo;

	MaterialBRDF(BRDF *brdf, glm::vec3 albedo) {
		this->brdf = brdf;
		this->albedo = albedo;
	}

	bool scatter(Ray &rayIn, Hit &hit, glm::vec3 &attenuation, Ray &scattered, float &pdf) {

		//bool r = ((RoughConductorBRDF*)brdf)->sample(rayIn, scattered, hit.p, hit.normal, attenuation, pdf);
		glm::vec3 microfacetNormal = ((RoughConductorBRDF*)brdf)->microfacetSample();
		glm::vec3 u, v;
		glm::mat3 orthonormalCS = generateOrthonormalCS(hit.normal, u, v);
		glm::vec3 scatteredDir = toWorld(microfacetNormal, hit.normal, u, v);
		
		scatteredDir = glm::reflect(glm::normalize(rayIn.d), glm::normalize(scatteredDir));

		scattered = Ray(hit.p, scatteredDir);


		attenuation = albedo;

		return true;
		/*glm::vec3 fr = brdf->eval(rayIn, scattered, hit, attenuation, pdf);
		if (pdf == 0) {
			attenuation = glm::vec3(1);
		}
		else {
			glm::vec3 brdfVal = fr * (1.0f / pdf);
			//std::cout << pdf << "\n";
			//std::cout << brdfVal.x << ", " << brdfVal.y <<", " << brdfVal.z << "\n";
			//std::cout << fr.x << ", " << fr.y <<", " << fr.z << "\n";
			//std::cout << scattered.d.x << ", " << scattered.d.y << ", " << scattered.d.z << "\n";
			attenuation = albedo ;
		}*/
		/*float r = random();
		glm::vec3 v, u;
		glm::mat3 toWorld = generateOrthonormalCS(hit.normal, u, v);

		glm::vec3 n = ((BeckmannBRDF*)brdf)->sampleNormalDirection(glm::vec2(r, 1.0f - r));
		glm::vec3 nwp = toWorld * n;
		nwp = glm::normalize(nwp);

		attenuation = albedo;
		scattered.o = hit.p;
		scattered.d = -nwp;*/



		//scattered.d = ((BeckmannBRDF*)brdf)->sampleNormalDirection(glm::vec2(r, 1.0f -r));


		return true;
	}

	MaterialBRDF();
	~MaterialBRDF();
};