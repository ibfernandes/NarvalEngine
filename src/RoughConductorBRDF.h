#pragma once
#include "BRDF.h"
#include "Math.h"

class RoughConductorBRDF : public BRDF {
public:
	float roughness, metallic, a;
	glm::vec3 albedo;
	glm::vec3 f0;

	//Eq. 4 http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	//Also from LearnOpenGL https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.1.lighting/1.1.pbr.fs
	float ggxD(float NoH){
		float a2 = a * a;
		float NdotH = glm::max(NoH, 0.0f);
		float NdotH2 = NdotH * NdotH;

		float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
		denom = glm::max(EPSILON, PI * denom * denom);

		return a2 / denom;
	}

	//From eq Shlick-Beckmam k is not roghness, http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	//also opengl
	float geometrySchlickGGX(float NdotV){
		float k = a / 2.0f;

		float nom = NdotV;
		float denom = glm::max(EPSILON, NdotV * (1.0 - k) + k); //TODO case k = 0

		return nom / denom;
	}

	float geometrySmithG(float NoV, float NoL){
		float NdotV = glm::max(NoV, 0.0f);
		float NdotL = glm::max(NoL, 0.0f);
		float ggx1 = geometrySchlickGGX(NdotV);
		float ggx2 = geometrySchlickGGX(NdotL);

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
		float a2 = a * a;
		float theta = std::acos(std::sqrt((1.0f - r.x) / glm::max(float(EPSILON), (r.x * (a2 - 1.0f) + 1.0f))));
		float phi = TWO_PI * r.y;

		// Get our GGX NDF sample (i.e., the half vector)
		return glm::vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
	}

	float pdf() {
		return 1;
	}

	float pdf(float D, glm::vec3 H, glm::vec3 N, glm::vec3 V) {
		float NdotH = glm::dot(N, H);
		//float sinTheta= std::sin(std::acos(NdotH));
		float VdotH = glm::dot(V, H);

		float D_ggxpdf = D * NdotH;

		return D_ggxpdf / (4 * VdotH);
	}

	//Samples scattered direction given this BRDF
	//based on https://github.com/tunabrain/tungsten/blob/74214128e08741af35615ac4832423d150ef88ca/src/core/bsdfs/RoughConductorBsdf.cpp
	bool sample(Ray rayIn, Ray &scattered, Hit h) {

		//in spherical coordinates
		glm::vec3 microfacetNormal = microfacetSample();
		glm::vec3 v, u;
		glm::mat3 orthonormalCS = generateOrthonormalCS(h.normal, v, u);
		//local to world
		microfacetNormal = glm::normalize(toWorld(microfacetNormal, h.normal, v, u));

		glm::vec3 scatteredDir = glm::reflect(glm::normalize(rayIn.d), glm::normalize(microfacetNormal));

		scattered = Ray(h.p, scatteredDir);
		return true;
	}

	//Evaluates GGX term
	//based on http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
	glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, float &pdf) {
		glm::vec3 H = glm::normalize(-rayIn.d + scattered.d);

		float NdotL = saturate(glm::dot(hit.normal, scattered.d));
		float NdotH = saturate(glm::dot(hit.normal, H));
		float HdotV = saturate(glm::dot(H, -rayIn.d));
		float NdotV = saturate(glm::dot(hit.normal, -rayIn.d));

		// Evaluate terms for our GGX BRDF model
		float  D = ggxD(NdotH);
		float  G = geometrySmithG(NdotV, NdotL);
		glm::vec3 F = schlickF(HdotV);

		glm::vec3 numerator = D * G * F;
		float denominator = 4.0 * glm::max(NdotV, 0.0f) * glm::max(NdotL, 0.0f);
		glm::vec3 specular = numerator / glm::max(denominator, float(EPSILON));
		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0) - kS;

		if (NdotH == 0 || HdotV == 0)
			pdf = 0;
		else 
			pdf = this->pdf(D, H, hit.normal, -rayIn.d);

		if (metallic == 0)
			pdf = 1 / TWO_PI;
		

		if (pdf < EPSILON)
			pdf = 0;

		kD *= 1.0 - metallic;
		return ((albedo / float(PI)) * kD + specular) * NdotL;
	}

	static float roughnessToAlpha(float roughness) {
		//Clamping is necessary for the D term where we have 1 / roughness^4
		roughness = glm::max(roughness, 0.045f);
		return roughness * roughness;
	}

	RoughConductorBRDF(float roughness, float metallic, glm::vec3 albedo) {
		this->roughness = roughness;
		this->a = roughnessToAlpha(roughness);
		this->metallic = metallic;
		this->albedo = albedo;
		this->f0 = mix(glm::vec3(0.04f), albedo, metallic);
	}

	RoughConductorBRDF();
	~RoughConductorBRDF();
};

