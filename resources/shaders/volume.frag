#version 430 core

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 eyeObjectSpace;
flat in vec3 translation;
flat in vec3 scale;

uniform sampler3D volume;
uniform sampler3D perlinWorley3;
uniform float time;
uniform float continuosTime;

#define PI 3.1415926535897932384626433832795
uniform vec2 screenRes = vec2(1024, 512);

uniform vec3 scattering = vec3(0.4, 0.4, 0.1);
uniform vec3 absorption = vec3(0.001);
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform vec3 lightPosition = vec3(0.9);
vec3 lightPosInObjectSpace = (lightPosition - translation)/scale;
uniform vec3 lightColor = vec3(0.5, 0.2 , 0.2);
uniform vec3 materialColor = vec3(1.0, 0.0, 0.0);
uniform vec3 gradientUp = vec3(0.0, 0.0, 0.0);
uniform vec3 gradientDown = vec3(0.0, 0.0, 0.0);

uniform vec3 sunLightColor = vec3(252.0/255, 186.0/255, 3.0/255);
uniform float ambientStrength = 0.1;

uniform float lightMaxRadius = 0.5;
uniform float densityCoef = 0.5;

uniform float numberOfSteps = 64;
uniform float shadowSteps = 16;
uniform float g = 1;
uniform float phaseFunctionOption = 0;
uniform float gammaCorrection = 0;
uniform float enablePathTracing = 0;
uniform float enableAnimation = 0;
uniform float enableShadow = 0;


uniform float param1 = 0;
uniform float param2 = 0;
uniform float param3 = 0;

vec3 sunDirection = normalize( vec3(-1.0,0.75,1.0) );

vec2 intersectBox(vec3 orig, vec3 dir) {
	const vec3 boxMin = vec3(0, 0, 0);
	const vec3 boxMax = vec3(1, 1, 1);

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
	return densityCoef*texture(volume, pos).r;
}

float isotropicPhaseFunction(){
	return 1 / (4 * PI);
}

float rayleighPhaseFunction(float theta){
	float cosAngle = cos(theta);
	return (3 * (1 + cosAngle*cosAngle)) / (16 * PI);
}

float henyeyGreensteinPhaseFunction(float cosTheta, float g){
	//float cosAngle = cos(theta);
	float g2 = g * g;
	// (0.25 / PI) = 1 / (4 * PI)
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

vec3 evaluateLight(in vec3 pos){
    vec3 L = lightPosInObjectSpace - pos;
    return lightColor/ (1.2 + dot(L, L));
}

vec3 volumetricShadow(in vec3 cubePos, in vec3 lightPos){
	if(enableShadow==0)
		return vec3(1,1,1);

    vec3 transmittance = vec3(1.0);
    float distance = length(lightPos-cubePos) / shadowSteps;
	vec3 lightDir = normalize(lightPos - cubePos);
	float stepSizeShadow = 1 / shadowSteps;

    for(float tshadow= stepSizeShadow; tshadow<3.0; tshadow+=stepSizeShadow){
        vec3 cubeShadowPos = cubePos + tshadow*lightDir;
        float density = sampleVolume(cubeShadowPos);
		if(density==0)
			break;	
        transmittance *= exp(-extinction * density);
		if(transmittance.x < 0.05)
			break;
    }
    return transmittance;
}

vec4 refinedVersion(vec4 color, vec3 rayDirection){
	vec2 tHit = intersectBox(eyeObjectSpace, rayDirection);

	if (tHit.x > tHit.y) 
		discard;
	
	tHit.x = max(tHit.x, 0.0);
	vec3 dtVec = 1.0 / (vec3(numberOfSteps) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = eyeObjectSpace + (tHit.x) * rayDirection;

	float density = 0;

	float currentDensity = 0.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 currentTransmittance = vec3(0.0, 0.0, 0.0);
	float theta = 1.5f;
	float lightDotEye = dot(normalize(rayDirection), normalize(lightPosInObjectSpace));

	for (float t = tHit.x; t < tHit.y; t += dt) {
		currentDensity = sampleVolume(cubePos);
		currentTransmittance = exp(-extinction * currentDensity);

		vec3 S = scattering * albedo * volumetricShadow(cubePos, lightPosInObjectSpace);
		vec3 Sglobal = ambientStrength *  S;
		S = evaluateLight(cubePos) * phaseFunction(lightDotEye, g) * S;
		
		//solved integral for slide 28 SH
		vec3 Sint = (S - S * currentTransmittance) / extinction; 

		vec3 SintGlobal = (Sglobal - Sglobal * currentTransmittance) / extinction; 
		scatteredLight += totalTransmittance * (Sint + SintGlobal); 
		
		totalTransmittance *= currentTransmittance;

		if(totalTransmittance.x < 0.05)
			break;

		cubePos += rayDirection * dt;
	}

    //lighting
    vec3 finalColor = color.xyz;

    // Apply scattering/transmittance
    finalColor = vec3(finalColor * totalTransmittance + scatteredLight);

	return vec4(finalColor, 1.0);
}

uniform float SPP = 16;
#define BOUNCES 16

vec2 seed;

vec3 rand2n() {
    seed+=vec2(-1,1);
	// implementation based on: lumina.sourceforge.net/Tutorials/Noise.html
    return vec3(fract(sin(dot(seed.xy ,vec2(12.9898,78.233))) * 43758.5453),
		fract(cos(dot(seed.xy ,vec2(4.898,7.23))) * 23421.631), 1);
}

vec3 castRay(vec4 color, vec3 direction){
	for(int i=0; i < BOUNCES; i++){
		return refinedVersion(color, direction).xyz;
	}
}

vec4 pathTracing(vec4 color){
	vec3 finalColor = color.xyz;

	for(int i=0; i < SPP; i++){
		seed = gl_FragCoord.xy * (i + 1);
		vec3 rayDir = (vertCoord + rand2n()*0.2) - eyeObjectSpace ;
		rayDir = normalize(rayDir);

		vec2 tHit = intersectBox(eyeObjectSpace, rayDir);
		if (tHit.x > tHit.y){ 
			finalColor += color.xyz;
			continue;
		}

		finalColor +=  castRay(color, rayDir);
	}

	finalColor = finalColor / float(SPP);

	return vec4(finalColor.xyz, 1.0);
}

void main(){
	vec4 down = vec4( gradientUp,1);
    vec4 up = vec4( gradientDown,1);
	
	vec4 gradient = vec4(mix(up, down, gl_FragCoord.y/screenRes.y));
	vec4 thisColor;

	if(enablePathTracing == 1)
		thisColor = pathTracing(gradient);
	else
		thisColor = refinedVersion(gradient, normalize(rayDirection));

	if(gammaCorrection == 1 && thisColor!=gradient)
		thisColor = vec4(pow(thisColor.xyz, vec3(1.0/2.2)), 1.0); // simple linear to gamma, exposure of 1.0

	color = thisColor;
}
