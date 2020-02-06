#version 430 core
#define PI 3.1415926535897932384626433832795

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;
uniform sampler2D previousCloud;

uniform float time;
uniform vec2 screenRes;

uniform vec3 resolution;
uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform float densityCoef;
uniform float showbbox;
uniform float numberOfSteps;
uniform float shadowSteps;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float Kc;
uniform float Kl;
uniform float Kq;

uniform float phaseFunctionOption;
uniform float g;
uniform float enablePathTracing;
uniform int SPP;
uniform int frameCount;
uniform float enableShadow;

uniform vec3 cameraPosition;
uniform mat4 model;
uniform mat4 invmodel;
uniform mat4 cam;
uniform mat4 proj;

struct Ray{
	vec3 origin;
	vec3 direction;
};

float seedSum = 0;
vec2 uv = gl_FragCoord.xy/screenRes.xy;
int maxBounces = 4;
vec3 boxMin = (model * vec4(0,0,0,1)).xyz;
vec3 boxMax = (model * vec4(1,1,1,1)).xyz;
float transmittanceThreshold = 0.001f;

/*
	Returns a random number between [0,1)
*/
float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

/*
	if(t.x == 0)
		origin inside AABB
	if(t.x > t.y)
		miss
*/
vec2 intersectBox(vec3 orig, vec3 dir) {
	//Line's/Ray's equation
	// o + t*d = y
	// t = (y - o)/d
	//when t is negative, the box is behind the ray origin
	vec3 tMinTemp = (boxMin - orig) / dir;
	vec3 tmaxTemp = (boxMax - orig) / dir;

	vec3 tMin = min(tMinTemp, tmaxTemp);
	vec3 tMax = max(tMinTemp, tmaxTemp);

	float t0 = max(tMin.x, max(tMin.y, tMin.z));
	float t1 = min(tMax.x, min(tMax.y, tMax.z));

	return vec2(t0, t1);
}

 float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax){
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float saturate(float value){
	return clamp(value, 0.0, 1.0);
}

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz;

	return densityCoef * texture(volume, tex).r;
}

float isotropicPhaseFunction(){
	return 1 / (4 * PI);
}

float rayleighPhaseFunction(float theta){
	float cosAngle = cos(theta);
	return (3 * (1 + cosAngle*cosAngle)) / (16 * PI);
}

float henyeyGreensteinPhaseFunction(float cosTheta, float g){
	float g2 = g * g;
	return  (0.25 / PI) * (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
}

float phaseFunction(float theta, float g){
	if(phaseFunctionOption == 0)
		return isotropicPhaseFunction();

	if(phaseFunctionOption == 1)
		return rayleighPhaseFunction(theta);

	if(phaseFunctionOption == 2)
		return henyeyGreensteinPhaseFunction(theta, g);

	return 1;
}

//geometric term
float evaluateLight(in vec3 pos){
    float d = length(lightPosition - pos);
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

        transmittance *= exp(-extinction * density);
		if(transmittance.x < transmittanceThreshold)
			break;
    }
    return transmittance;
}

bool integrate(Ray incomingRay, vec2 tHit, out vec3 transmittance, out vec3 inScattering){
	if (tHit.x > tHit.y) 
		return false;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(incomingRay.direction);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	float lightDotEye = dot(normalize(incomingRay.direction), normalize(lightPosition));
	vec3 cubePos = incomingRay.origin + tHit.x * incomingRay.direction;
	vec3 totalTr = vec3(1.0f);
	vec3 sum = vec3(0.0f);
	vec3 currentTr = vec3(0);

	for(float t = tHit.x; t < tHit.y; t += dt){
		float density = sampleVolume(cubePos);
		
		//If accumulated transmittance is near zero, there's no point in continue calculating transmittance
		if(totalTr.x > transmittanceThreshold){
			currentTr = exp(-extinction * density);
			totalTr *= currentTr;
		}
		
		vec3 Ls = ambientStrength * evaluateLight(cubePos) * volumetricShadow(cubePos, lightPosition) * phaseFunction(lightDotEye, g);
		//Integrate Ls from 0 to d
		Ls =  (Ls - Ls * currentTr) / extinction; 

		sum += totalTr * scattering * Ls;
		cubePos += incomingRay.direction * dt;
	}

	transmittance = totalTr;
	inScattering = sum;
	return true;
}

/*
	Sample unit sphere
*/
vec3 sampleSphere(vec2 uv) {
    float e1 = randomUniform(uv);
    float e2 = randomUniform(uv);

	float theta = 2 * PI * e1;
	float phi = acos(1 - 2 * e2);
    vec3 res = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	
    return res;
}

//Came from the neutron transport community in the 60s and has various names (delta tracking etc), but was developed by Woodcock. Source: http://www.cs.cornell.edu/courses/cs6630/2012sp/notes/09volpath.pdf
float sampleDistance(){
	float t = -log(1.0f - randomUniform(uv))/extinction.x;
	return t;
}

bool scatter(Ray incoming, vec2 tHit, inout Ray scattered, inout vec3 transmittance, inout vec3 scatteringOut){
	integrate(incoming, tHit, transmittance, scatteringOut);
	scattered.origin = incoming.origin + tHit.x * incoming.direction;
	scattered.direction = sampleSphere(uv);
	return true;
}

vec3 calculateColor(Ray incoming, int depth, vec3 background){
	Ray scattered;
	vec3 transmittance;
	vec3 scatteringOut;
	vec3 transmittanceAcc = vec3(1.0f);
	vec3 scatteringAcc = vec3(0.0f);
	int maxBounces = 4;

	for(int i = 0; i < maxBounces; i++){
		vec2 tHit = intersectBox(incoming.origin, incoming.direction);
		
		if (tHit.x > tHit.y) 
			break;
		tHit.y -= 0.00001f;
		tHit.x = max(tHit.x, 0.0f);

		vec3 endPos = incoming.origin + tHit.y * incoming.direction;
		float totalDistance = length(incoming.origin - endPos);
		float scatterDistance = sampleDistance();

		if(scatterDistance < totalDistance)
			tHit.y *= scatterDistance / totalDistance;

		scatter(incoming, tHit, scattered, transmittance, scatteringOut);
		incoming = scattered;
		transmittanceAcc *= transmittance;
		scatteringAcc += scatteringOut;
	}

	return background * transmittanceAcc + scatteringAcc;
}

vec4 pathTracing(vec4 background, vec3 origin, vec3 rayDirection, float depth){
	vec3 thisColor = vec3(0);
	float e1 = randomUniform(uv) - 0.5f;
	float e2 = randomUniform(uv) - 0.5f;
	vec2 frag = vec2(float(gl_FragCoord.x) + e1, float(gl_FragCoord.y) + e2);
	vec3 screenPosNDC = vec3((frag/screenRes.xy), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;

	thisColor = calculateColor(Ray(origin, newRay), 0, background.xyz);
	
	vec4 previousColor = texture(previousCloud, uv);
	
	//clear color because the camera moved
	if(previousColor.a == 0)
		return vec4(thisColor.xyz, 1);
	
	if(frameCount > 1){
		vec3 n1 = (1.0f / frameCount) * thisColor; 
		thisColor = vec3(previousColor) * 1.0f / (1.0f + 1.0f / (frameCount - 1.0f));
		thisColor += n1;
	}

	return vec4(thisColor, 1);
}

void main(){
	vec4 thisColor;
	vec3 transmittance;
	vec3 inScattering;
	vec4 bg = texture(background, uv);
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 

	if(enablePathTracing==1)
		thisColor = pathTracing(bg, incomingRay.origin, incomingRay.direction, depth);
	else
		if(integrate(incomingRay, tHit, transmittance, inScattering))
			thisColor = vec4(bg.xyz * transmittance + inScattering, 1.0f);
	if(showbbox == 1)
		thisColor = vec4(1,0,0,1);
	
	color = thisColor;
}
