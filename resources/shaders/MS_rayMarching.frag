#version 430 core
#define PI 3.1415926535897932384626433832795
#define INV4PI 0.07957747154594766788
#define MAX_LIGHTS 32

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 translation;
flat in vec3 scale;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform mat4 invmodel;
uniform vec3 cameraPosition;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;

vec2 uv = gl_FragCoord.xy/textureSize(background, 0);

/* Volume properties */
uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)
uniform float densityCoef;

/* Phase function properties */
uniform float g;

/* Method Params */
uniform int numberOfSteps;
uniform int numberOfShadowSteps;
const float transmittanceThreshold = 0.01;

/* Light properties */
// 0 = rect, 1 = infAreaLight
struct Light {    
    vec3 position;
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
    vec3 Li;
	int type;
};

uniform Light lights[MAX_LIGHTS];
uniform int numberOfLights = 1;

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

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5;
	vec3 texGridPoint = tex * textureSize(volume, 0);

	return interpolatedDensity(texGridPoint);
}

vec3 evalHG(vec3 incoming, vec3 scattered) {
	float cosTheta = dot(normalize(incoming), normalize(scattered));
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	
	float res = INV4PI * (1.0f - g * g) / (denom * sqrt(denom));
	return vec3(res, res, res);
}

//geometric term
vec3 evaluateLight(Light light, in vec3 pos){
    float d = length(light.position - pos);
    return (light.Li) / (d*d); //TODO this number 100 here is a pure gambiarra
}

vec3 volumetricShadow(in vec3 cubePos, in vec3 lightPos){
    vec3 transmittance = vec3(1.0);
    float distance = length(lightPos-cubePos) / numberOfShadowSteps;
	vec3 lightDir = normalize(lightPos - cubePos);
	float stepSizeShadow = 1.0f / numberOfShadowSteps;

    for(float tshadow = stepSizeShadow; tshadow < distance; tshadow += stepSizeShadow){
        vec3 cubeShadowPos = cubePos + tshadow * lightDir;
		

        float density = sampleVolume(cubeShadowPos);
		if(density == 0)
			continue;
		

		// e ^(-ext * density[i] + -ext * density[i+1] + ...)
        transmittance *= exp(-extinction * density * stepSizeShadow);
		if(transmittance.x < transmittanceThreshold)
			break;
    }
    return transmittance;
}

bool integrate(Ray ray, Light light, vec2 tHit, out vec3 transmittance, out vec3 inScattering){
	if (tHit.x > tHit.y) 
		return false;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(ray.direction);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	ray.origin = getPointAt(ray, tHit.x);
	vec3 totalTr = vec3(1.0f);
	vec3 scatteredLuminance = vec3(0.0f);
	vec3 currentTr = vec3(0);

	for(float t = tHit.x; t < tHit.y; t += dt){
		float density = sampleVolume(ray.origin);
		
		vec3 S = evaluateLight(light, ray.origin) 
				* volumetricShadow(ray.origin, light.position) 
				* density * (scattering*densityCoef)
				* evalHG(ray.direction, light.position - ray.origin);
		vec3 sampleExtinction = max(vec3(0.0000000001), density * extinction * densityCoef);
		vec3 Sint = (S - S * exp(-sampleExtinction * dt)) / sampleExtinction;
		scatteredLuminance += totalTr * Sint;

		// Evaluate transmittance to view independentely
		totalTr *= exp(-sampleExtinction * dt);
		
		ray.origin = getPointAt(ray, dt);
	}

	transmittance = totalTr;
	inScattering = scatteredLuminance;
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

	if(integrate(incomingRay, lights[0], tHit, transmittance, inScattering))
		thisColor = vec4(bg.xyz * transmittance + inScattering, 1.0f);

	color = thisColor;
}
