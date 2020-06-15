#pragma once
#include "BRDF.h"
#include "Math.h"

class RoughConductorBRDF : public BRDF {
public:
	float roughness;
	float alphax, alphay;

	float ggxD(float NdotH) {
		float a2 = roughness * roughness;
		float d = ((NdotH * a2 - NdotH) * NdotH + 1);
		return a2 / (d * d * PI);
	}

	float schlickMaskingTermG(float NdotV, float NdotL) {
		// Karis notes they use alpha / 2 (or roughness^2 / 2)
		float k = roughness * roughness / 2;

		// Compute G(v) and G(l).  These equations directly from Schlick 1994
		//     (Though note, Schlick's notation is cryptic and confusing.)
		float g_v = NdotV / (NdotV*(1 - k) + k);
		float g_l = NdotL / (NdotL*(1 - k) + k);
		return g_v * g_l;
	}

	glm::vec3 schlickFresnelF(glm::vec3 f0, float lDotH){
		return f0 + (glm::vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - lDotH, 5.0f);
	}

	// From "PHYSICALLY BASED LIGHTING CALCULATIONS FOR COMPUTER GRAPHICS" by Peter Shirley
	// http://www.cs.virginia.edu/~jdl/bib/globillum/shirley_thesis.pdf
	//conductorReflectance


	float fresnel(float eta, float k, float cosThetaI){
		float cosThetaISq = cosThetaI * cosThetaI;
		float sinThetaISq = glm::max(1.0f - cosThetaISq, 0.0f);
		float sinThetaIQu = sinThetaISq * sinThetaISq;

		float innerTerm = eta * eta - k * k - sinThetaISq;
		float aSqPlusBSq = std::sqrt(glm::max(innerTerm*innerTerm + 4.0f*eta*eta*k*k, 0.0f));
		float a = std::sqrt(glm::max((aSqPlusBSq + innerTerm)*0.5f, 0.0f));

		float Rs = ((aSqPlusBSq + cosThetaISq) - (2.0f*a*cosThetaI)) /
			((aSqPlusBSq + cosThetaISq) + (2.0f*a*cosThetaI));
		float Rp = ((cosThetaISq*aSqPlusBSq + sinThetaIQu) - (2.0f*a*cosThetaI*sinThetaISq)) /
			((cosThetaISq*aSqPlusBSq + sinThetaIQu) + (2.0f*a*cosThetaI*sinThetaISq));

		return 0.5f*(Rs + Rs * Rp);
	}

	glm::vec3 conductorReflectance(glm::vec3 eta, glm::vec3 k, float cosTheta) {
		return glm::vec3(
			fresnel(eta.x, k.x, cosTheta),
			fresnel(eta.y, k.y, cosTheta),
			fresnel(eta.z, k.z, cosTheta)
		);
	}

	float beckmannD(glm::vec3 surfaceNormal) {
		if (surfaceNormal.z <= 0.0f)
			return 0.0f;

		float alphaSq = alphax * alphay;
		float cosThetaSq = surfaceNormal.z*surfaceNormal.z;
		float tanThetaSq = glm::max(1.0f - cosThetaSq, 0.0f) / cosThetaSq;
		float cosThetaQu = cosThetaSq * cosThetaSq;
		return  std::exp(-tanThetaSq / alphaSq) / (PI * alphaSq* cosThetaQu);
	}

	float g1(glm::vec3 v, glm::vec3 m) {
		if (glm::dot(v, m) * v.z <= 0.0f)
			return 0.0f;
		float cosThetaSq = v.z*v.z;
		float tanTheta = std::abs(std::sqrt(glm::max(1.0f - cosThetaSq, 0.0f)) / v.z);
		float a = 1.0f / (alphax*tanTheta);
		if (a < 1.6f)
			return (3.535f*a + 2.181f*a*a) / (1.0f + 2.276f*a + 2.577f*a*a);
		else
			return 1.0f;
	}

	float g(glm::vec3 wi, glm::vec3 wo, glm::vec3 m) {
		return g1(wi, m) * g1(wo, m);
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
		return glm::vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
	}

	float pdf(glm::vec3 m) {
		return beckmannD(m) * m.z;
	}

	glm::vec3 eta = glm::vec3(0.200438f, 0.924033f, 1.10221f);
	glm::vec3 k = glm::vec3(3.91295f, 2.45285f, 2.14219f);

	//The one i was using before to test out
	bool sampleScattered(Ray &wi, Ray &wo, glm::vec3 hitpoint, glm::vec3 normal,glm::vec3 &attenuation, float &pdfVal) {
		if (wi.d.z <= 0.0f)
			return false;

		float sampleRoughness = roughness;

		glm::vec3 halfwayVec = microfacetSample();
		glm::vec3 target = hitpoint + halfwayVec + sampleUnitSphere();
		wo.d = target - hitpoint;
		return true;
	}

	//first you sample, then you eval.
	//based on https://github.com/tunabrain/tungsten/blob/74214128e08741af35615ac4832423d150ef88ca/src/core/bsdfs/RoughConductorBsdf.cpp
	bool sample(Ray &rayIn, Ray &scattered, glm::vec3 hitpoint, glm::vec3 normal, glm::vec3 albedo, float &pdfVal) {
		if (rayIn.d.z <= 0.0f)
			return false;

		glm::vec3 microfacetNormal = microfacetSample();
		glm::vec3 u, v;
		glm::mat3 orthonormalCS = generateOrthonormalCS(normal, u, v);
		scattered.o = hitpoint;
		scattered.d = toWorld(microfacetNormal, normal, u, v);

		float rayInDotMCNormal = glm::dot(rayIn.d, scattered.d);

		glm::vec3 halfwayVector = glm::normalize(rayIn.d + scattered.d);
		float G = g(rayIn.d, scattered.d, halfwayVector);
		float D = beckmannD(halfwayVector);
		float microfacetPDF = pdf(microfacetNormal);
		float weight = rayInDotMCNormal * G * D / rayIn.d.z * microfacetPDF;
		glm::vec3 F = conductorReflectance(eta, k, rayInDotMCNormal);


		pdfVal = (microfacetPDF * 0.25f) / rayInDotMCNormal;
		glm::vec3 finalWeight = albedo * F * weight;

		return true;
	}



	/*
		Calculates the microfacet BRDF fr(wi, wo, theta) equation and returns it
		based: https://github.com/tunabrain/tungsten/blob/74214128e08741af35615ac4832423d150ef88ca/src/core/bsdfs/RoughConductorBsdf.cpp
	*/
	/*glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, glm::vec3 attenuation) {

		if (rayIn.d.z <= 0.0f || scattered.d.z <= 0.0f)
			return glm::vec3(0);

		glm::vec3 halfwayVector = glm::normalize(rayIn.d + scattered.d);
		float cosTheta = glm::dot(rayIn.d, halfwayVector);
		glm::vec3 F = conductorReflectance(eta, k, cosTheta);
		float G = g(rayIn.d, scattered.d, halfwayVector);
		float D = beckmannD(halfwayVector);
		
		glm::vec3 fr = (F*G*D*0.25f) / rayIn.d.z;
		return fr;
	}*/

	void test() {
		this->roughness = 0.01f;
		glm::vec3 rayIn = glm::normalize(glm::vec3(1,-1,0));
		glm::vec3 microfacetNormal = glm::normalize(glm::vec3(-1,1,0));
		glm::vec3 outgoing = glm::reflect(rayIn, microfacetNormal);
		glm::vec3 normal = glm::normalize(glm::vec3(0,1,0));

		glm::vec3 H = glm::normalize(-rayIn + outgoing);

		float NdotL = saturate(glm::dot(normal, -rayIn));
		float NdotH = saturate(glm::dot(normal, H));
		float LdotH = saturate(glm::dot(-rayIn, H));
		float NdotV = saturate(glm::dot(normal, outgoing));

		// Evaluate terms for our GGX BRDF model
		float  D = ggxD(NdotH);
		float  G = schlickMaskingTermG(NdotL, NdotV);
		glm::vec3 f0 = glm::vec3(0.2f);
		glm::vec3 F = schlickFresnelF(f0, LdotH);

		glm::vec3 ggxTerm = D * G * F / (4 * NdotL * NdotV);
		float  ggxPdf = D * NdotH / (4 * LdotH);
		glm::vec3 res =  NdotL * ggxTerm / ggxPdf;


	}

	//based on http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
	glm::vec3 eval(Ray rayIn, Ray scattered, Hit hit, glm::vec3 attenuation) {
		//test();
		glm::vec3 H = glm::normalize(-rayIn.d + scattered.d);

		float NdotL = saturate(glm::dot(hit.normal, -rayIn.d));
		float NdotH = saturate(glm::dot(hit.normal, H));
		float LdotH = saturate(glm::dot(-rayIn.d, H));
		float NdotV = saturate(glm::dot(hit.normal, scattered.d));

		// Evaluate terms for our GGX BRDF model
		float  D = ggxD(NdotH);
		float  G = schlickMaskingTermG(NdotL, NdotV);
		glm::vec3 f0 = glm::vec3(0.2f);
		glm::vec3 F = schlickFresnelF(f0, LdotH);

		glm::vec3 ggxTerm = D * G * F / (4 * NdotL * NdotV);
		float  ggxPdf = D * NdotH / (4 * LdotH);
		return NdotL * attenuation * ggxTerm /*/ ggxPdf*/;
	}

	static float roughnessToAlpha(float roughness) {
		roughness = glm::max(roughness, 0.001f);
		float x = std::log(roughness);
		return 1.62142f + 0.819955f * x + 0.1734f * x * x +
			0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
	}

	RoughConductorBRDF(float roughness) {
		this->roughness = roughness;
		float a = roughnessToAlpha(roughness);
		this->alphax = a;
		this->alphay = a;
	}

	RoughConductorBRDF();
	~RoughConductorBRDF();
};

