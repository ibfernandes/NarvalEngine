#version 430 core
#define PI 3.1415926535897932384626433832795
#define MAX_LIGHTS 32

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 translation;
flat in vec3 scale;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;

uniform vec2 screenRes;
uniform vec3 cameraPosition;
uniform mat4 model;
uniform mat4 invmodel;


/* Volume properties */
uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform float densityCoef;
uniform float numberOfSteps;
uniform float shadowSteps;

/* Phase function properties */
uniform float g;

/* Light properties */
// 0 = rect, 1 = infAreaLight
struct LightPoint {    
    vec3 position;
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
    vec3 Li;
	int type;
};

uniform LightPoint lightPoints[MAX_LIGHTS];
uniform int numberOfLights = 1;

uniform float Kc;
uniform float Kl;
uniform float Kq;

struct Ray{
	vec3 origin;
	vec3 direction;
};

//Boundaries of a unit cube centered at origin
vec3 boxMin = (model * vec4(-0.5f, -0.5f, -0.5f,1)).xyz;
vec3 boxMax = (model * vec4(0.5, 0.5, 0.5, 1)).xyz;

vec3 getPointAt(Ray r, float t){
	return r.origin + t * r.direction;
}

/*
	Ray must be in WCS
*/
vec2 intersectBox(vec3 orig, vec3 dir, vec3 bmin, vec3 bmax) {
	//Line's/Ray's equation
	// o + t*d = y
	// t = (y - o)/d
	//when t is negative, the box is behind the ray origin
	vec3 tMinTemp = (bmin - orig) / dir;
	vec3 tmaxTemp = (bmax - orig) / dir;

	vec3 tMin = min(tMinTemp, tmaxTemp);
	vec3 tMax = max(tMinTemp, tmaxTemp);

	float t0 = max(tMin.x, max(tMin.y, tMin.z));
	float t1 = min(tMax.x, min(tMax.y, tMax.z));

	return vec2(t0, t1);
}

/*
	if(t.x == 0)
		origin inside AABB
	if(t.x > t.y)
		miss
*/
vec2 intersectBox(vec3 orig, vec3 dir) {
	return intersectBox(orig, dir, boxMin, boxMax);
}

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5; //added 0.5

	return densityCoef * texture(volume, tex).r;
}

float density(ivec3 gridPoint){
	return texelFetch( volume, gridPoint, 0).x;
}

float interpolatedDensity(vec3 gridPoint) {
	vec3 pSamples = vec3(gridPoint.x - .5f, gridPoint.y - .5f, gridPoint.z - .5f);
	ivec3 pi = ivec3(floor(pSamples.x), floor(pSamples.y), floor(pSamples.z));
	vec3 d = pSamples - vec3(pi);

	// Trilinearly interpolate density values to compute local density
	float d00 = mix(density(pi), density(pi + ivec3(1, 0, 0)), d.x);
	float d10 = mix(density(pi + ivec3(0, 1, 0)), density(pi + ivec3(1, 1, 0)), d.x);
	float d01 = mix(density(pi + ivec3(0, 0, 1)), density(pi + ivec3(1, 0, 1)), d.x);
	float d11 = mix(density(pi + ivec3(0, 1, 1)), density(pi + ivec3(1, 1, 1)), d.x);
	float d0 = mix(d00, d10, d.y);
	float d1 = mix(d01, d11, d.y);
	return mix(d0, d1, d.z);
}

float henyeyGreensteinPhaseFunction(float cosTheta, float g){
	float g2 = g * g;
	return  (0.25 / PI) * (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
}

float phaseFunction(float theta, float g){
	return henyeyGreensteinPhaseFunction(theta, g);
}

//geometric term
float evaluateLight(LightPoint light, in vec3 pos){
    float d = length(light.position - pos);
    return (1.0f / (Kc + Kl * d + Kq * d * d));
}

vec3 volumetricShadow(in vec3 cubePos, in vec3 lightPos){
	if(enableShadow==0)
		return vec3(1,1,1);

    vec3 transmittance = vec3(1.0);
    float distance = length(lightPos-cubePos) / shadowSteps;
	vec3 lightDir = normalize(lightPos - cubePos);
	float stepSizeShadow = 1.0f / shadowSteps;

    for(float tshadow = stepSizeShadow; tshadow < distance; tshadow += stepSizeShadow){
        vec3 cubeShadowPos = cubePos + tshadow * lightDir;
		

        float density = sampleVolume(cubeShadowPos);
		if(density == 0)
			continue;
		

		// e ^(-ext * density[i] + -ext * density[i+1] + ...)
        transmittance *= exp(-extinction * density);
		if(transmittance.x < transmittanceThreshold)
			break;
    }
    return transmittance;
}

bool integrate(Ray incomingRay, LightPoint light, vec2 tHit, out vec3 transmittance, out vec3 inScattering){
	if (tHit.x > tHit.y) 
		return false;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(incomingRay.direction);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	float lightDotEye = dot(normalize(incomingRay.direction), normalize(light.position));
	vec3 cubePos = incomingRay.origin + tHit.x * incomingRay.direction;
	vec3 totalTr = vec3(1.0f);
	vec3 sum = vec3(0.0f);
	vec3 currentTr = vec3(0);

	for(float t = tHit.x; t < tHit.y; t += dt){
		float density = sampleVolume(cubePos);
		if(density == 0){
			cubePos += incomingRay.direction * dt;
			continue;
		}
		
		//If accumulated transmittance is near zero, there's no point in continue calculating transmittance
		if(totalTr.x > transmittanceThreshold){
			currentTr = exp(-extinction * density);
			totalTr *= currentTr;
		}
		
		vec3 Ls = ambientStrength * evaluateLight(light, cubePos) * volumetricShadow(cubePos, light.position) * phaseFunction(lightDotEye, g);
		//Integrate Ls from 0 to d
		Ls =  (Ls - Ls * currentTr) / extinction; 

		sum += totalTr * scattering * Ls;
		cubePos += incomingRay.direction * dt;
	}

	transmittance = totalTr;
	inScattering = sum;
	return true;
}

void main(){
	vec4 thisColor;
	vec3 transmittance;
	vec3 inScattering;
	vec4 bg = texture(background, uv);
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 

	if(integrate(incomingRay, lightPoints[0], tHit, transmittance, inScattering))
		thisColor = vec4(bg.xyz * transmittance + inScattering, 1.0f);

	color = thisColor;
}
