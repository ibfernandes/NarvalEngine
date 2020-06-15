#pragma once
#include "BRDF.h"
#include "Math.h"

//Implementation based on sources :
//www.pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models.html
class BeckmannBRDF: public BRDF{
public:

	//TODO what?
	glm::vec3 R = glm::vec3(1.0f);

	//Wh = micro surface normal
	glm::vec3 sampleWh(glm::vec3 &wo, glm::vec2 &u) {
		// Sample full distribution of normals for Beckmann distribution

		// Compute $\tan^2 \theta$ and $\phi$ for Beckmann distribution sample
		float tan2Theta, phi;
		if (alphax == alphay) {
			float logSample = std::log(1 - u[0]);
			//if(!std::isinf(logSample));
			tan2Theta = -alphax * alphax * logSample;
			phi = u[1] * 2 * PI;
		}else {
			// Compute _tan2Theta_ and _phi_ for anisotropic Beckmann
			// distribution
			float logSample = std::log(1 - u[0]);
			//if(!std::isinf(logSample));
			phi = std::atan(alphay / alphax * std::tan(2 * PI * u[1] + 0.5f * PI));
			if (u[1] > 0.5f) phi += PI;
			float sinPhi = std::sin(phi), cosPhi = std::cos(phi);
			float alphax2 = alphax * alphax, alphay2 = alphay * alphay;
			tan2Theta = -logSample /
				(cosPhi * cosPhi / alphax2 + sinPhi * sinPhi / alphay2);
		}

		// Map sampled Beckmann angles to normal direction _wh_
		float cosTheta = 1 / std::sqrt(1 + tan2Theta);
		float sinTheta = std::sqrt(std::max((float)0, 1 - cosTheta * cosTheta));
		glm::vec3 wh = sampleUnitSphere(cosTheta, sinTheta, phi);
		if (!sameHemisphere(wo, wh)) wh = -wh;
		return wh;
	}

	//samples direction and its pdf
	//sample_f pbrt equivalent
	//wi is wrong for my case. wo is the outgoing ray.
	glm::vec3 sample_direction(glm::vec3 &wo, glm::vec3 &wi, glm::vec2 &u, float &pdfValue) {
		// Sample microfacet orientation $\wh$ and reflected direction $\wi$
		if (wo.z == 0) return glm::vec3(0.0f);

		glm::vec3 wh = sampleWh(wo, u);
		if (glm::dot(wo, wh) < 0) return glm::vec3(0.0f);   // Should be rare
		wi = glm::reflect(wo, wh);

		if (sameHemisphere(wo, wi)) return glm::vec3(0.f);
		// Compute PDF of _wi_ for microfacet reflection
		pdfValue = pdf(wo, wh) / (4.0f * glm::dot(wo, wh));
		return bsdf(wo, wi);
	}

	glm::vec3 sampleNormalDirection(glm::vec2 xi){
		float phi = xi.y * PI * 2.0F;
		float cosTheta = 0.0f;

		float tanThetaSq = -alphax * alphay *std::log(1.0f - xi.x);
		cosTheta = 1.0f / std::sqrt(1.0f + tanThetaSq);
	

		float r = std::sqrt(glm::max(1.0f - cosTheta * cosTheta, 0.0f));
		return glm::vec3(std::cos(phi)*r, std::sin(phi)*r, cosTheta);
	}


	glm::vec3 eval(Ray &rayIn, Ray &scattered, Hit &hit, glm::vec3 &attenuation, float &pdf) {
		return sample_direction(rayIn.d, scattered.d, glm::vec2(random(), random()), pdf);
	}






	float distribution(glm::vec3 surfaceNormal) {
		float tan = tan2Theta(surfaceNormal);
		if (std::isinf(tan)) return 0;
		float cos4Theta = cos2Theta(surfaceNormal);
		cos4Theta *= cos4Theta;

		float dividend = std::exp(-tan * (cos2Phi(surfaceNormal) / (alphax * alphax) + sin2Phi(surfaceNormal) / (alphay * alphay)));
		float divisor = (PI * alphax * alphay * cos4Theta);

		return dividend / divisor;
	}

	float lambda(glm::vec3 w) {
		float alpha = std::sqrt(cos2Phi(w) * alphax * alphax + sin2Phi(w) * alphay * alphay);

		float absTanTheta = glm::abs(tanTheta(w));
		if (std::isinf(absTanTheta)) return 0;
		float a = 1 / (alpha * absTanTheta);

		if (a >= 1.6f)
			return 0;
		else
			return (1.0f - 1.259f * a + 0.396f * a *a) / (3.535f * a + 2.181f * a * a);
	}

	float g(glm::vec3 wi, glm::vec3 wo) {
		return 1.0f / (1.0f + lambda(wo) + lambda(wi));
	}

	float g1(glm::vec3 w) {
		return 1.0f / (1.0f + lambda(w));
	}

	//fresnelSchlick
	glm::vec3 fresnel(float cosTheta) {
		glm::vec3 F0 = glm::vec3(0.04);
		//F0      = mix(F0, surfaceColor.rgb, metalness);
		return F0 + (1.0f - F0) * std::pow(1.0f - cosTheta, 5.0f);
	}

	//called MicrofacetReflection::f on pbrt
	glm::vec3 bsdf(glm::vec3 &wo, glm::vec3 &wi) {
		float cosThetaO = absCosTheta(wo), cosThetaI = absCosTheta(wi);
		glm::vec3 wh = wi + wo;
		// Handle degenerate cases for microfacet reflection
		if (cosThetaI == 0 || cosThetaO == 0) return glm::vec3(0.);
		if (wh.x == 0 && wh.y == 0 && wh.z == 0) return glm::vec3(0.);
		wh = glm::normalize(wh);
		glm::vec3 F = fresnel(glm::dot(wi, wh));

		return R * distribution(wh) * g(wo, wi) * F / (4 * cosThetaI * cosThetaO);
	}

	float pdf(glm::vec3 &wo, glm::vec3 &wh){
		return distribution(wh) * absCosTheta(wh);
	}

	static float roughnessToAlpha(float roughness) {
		roughness = glm::max(roughness, 0.001f);
		float x = std::log(roughness);
		return 1.62142f + 0.819955f * x + 0.1734f * x * x +
			0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
	}

	BeckmannBRDF(float roughness) {
		float a = roughnessToAlpha(roughness);
		this->alphax = a;
		this->alphay = a;
	}

	BeckmannBRDF();
	~BeckmannBRDF();

private:
	float alphax, alphay;
};

