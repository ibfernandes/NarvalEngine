#pragma once
#include "BRDF.h"
#include "Math.h"

class RoughConductorBRDF : public BRDF {
public:
	float roughness, metallic, a;
	glm::vec3 albedo;
	glm::vec3 f0;

	float ggxD(float NoH){
		float a2 = a * a;
		float NdotH = glm::max(NoH, 0.0f);
		float NdotH2 = NdotH * NdotH;

		float nom = a2;
		float denom = (NdotH2 * (a2 - 1.0) + 1.0);
		denom = PI * denom * denom;

		return nom / denom;
	}

	float geometrySchlickGGX(float NdotV, float k){
		float nom = NdotV;
		float denom = NdotV * (1.0 - k) + k;

		return nom / denom;
	}

	float g(float NoV, float NoL){
		float NdotV = glm::max(NoV, 0.0f);
		float NdotL = glm::max(NoL, 0.0f);
		float ggx1 = geometrySchlickGGX(NdotV, a);
		float ggx2 = geometrySchlickGGX(NdotL, a);

		return ggx1 * ggx2;
	}

	glm::vec3 schlickF(float cosTheta){
		return f0 + (1.0f - f0) * std::pow(1.0f - cosTheta, 5.0f);
	}

	//samples random microfacet normal direction, i.e the half vector
	//src: http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
	//samples GGX
	glm::vec3 microfacetSample() {
		glm::vec2 r = glm::vec2(random(), random());

		// GGX NDF sampling
		float a2 = roughness * roughness;
		float cosTheta = sqrt( glm::max(0.0f, (1.0f - r.x) / ((a2 - 1.0f)*r.x + 1.0f)) );
		float sinTheta = sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
		float phi = r.y * TWO_PI;

		// Get our GGX NDF sample (i.e., the half vector)
		//z is up
		return glm::vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
	}

	float pdf() {
		return 1;
		//return D * NdotH / (4 * HdotV);
	}

	//Samples scattered direction given this BRDF
	//based on https://github.com/tunabrain/tungsten/blob/74214128e08741af35615ac4832423d150ef88ca/src/core/bsdfs/RoughConductorBsdf.cpp
	bool sample(Ray rayIn, Ray &scattered, Hit h) {

		glm::vec3 microfacetNormal = microfacetSample();
		glm::vec3 u, v;
		glm::mat3 orthonormalCS = generateOrthonormalCS(h.normal, u, v);
		microfacetNormal = toWorld(microfacetNormal, h.normal, u, v);

		glm::vec3 scatteredDir = glm::reflect(glm::normalize(rayIn.d), glm::normalize(microfacetNormal));

		scattered = Ray(h.p, scatteredDir);
		return true;
	}

	//Evaluates GGX term
	//based on http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
	glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, float &pdf) {
		glm::vec3 H = glm::normalize(-rayIn.d + scattered.d);

		float NdotL = saturate(glm::dot(hit.normal, -rayIn.d));
		float NdotH = saturate(glm::dot(hit.normal, H));
		float LdotH = saturate(glm::dot(-rayIn.d, H));
		float HdotV = saturate(glm::dot(H, scattered.d));
		float NdotV = saturate(glm::dot(hit.normal, scattered.d));

		// Evaluate terms for our GGX BRDF model
		float  D = ggxD(NdotH);
		float  G = g(NdotV, NdotL);
		glm::vec3 F = schlickF(LdotH);

		glm::vec3 numerator = D * G * F;
		float denominator = 4.0 * glm::max(NdotV, 0.0f) * glm::max(NdotL, 0.0f);
		glm::vec3 specular = numerator / glm::max(denominator, float(EPSILON));
		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0) - kS;

		pdf = D * NdotH / (4 * HdotV);
		if (pdf < EPSILON)
			pdf = 0;

		kD *= 1.0 - metallic;
		return ((albedo / float(PI)) * kD + specular) * NdotL;
	}

	static float roughnessToAlpha(float roughness) {
		roughness = glm::max(roughness, 0.001f);
		return roughness * roughness;
	}

	RoughConductorBRDF(float roughness, float metallic, glm::vec3 albedo) {
		this->roughness = roughness;
		this->a = roughnessToAlpha(roughness);
		this->metallic = metallic;
		this->albedo = albedo;
		this->f0 = mix(glm::vec3(0.05f), albedo, metallic);
	}

	RoughConductorBRDF();
	~RoughConductorBRDF();
};

