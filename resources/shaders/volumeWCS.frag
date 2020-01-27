	#version 430 core

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 translation;
flat in vec3 scale;
flat in mat4 mvp;

uniform sampler3D volume;
uniform sampler3D perlinWorley3;
uniform sampler2D background;
uniform sampler2D backgroundDepth;
uniform sampler2D previousCloud;
uniform float time;

#define PI 3.1415926535897932384626433832795
uniform vec2 screenRes = vec2(1600.0f, 900.0f);

uniform vec3 scattering = vec3(0.4, 0.4, 0.4);
uniform vec3 absorption = vec3(0.001);
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform vec3 lightPosition = vec3(0.9);
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

uniform vec3 cameraPosition;
uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform int SPP = 1;

vec2 seed = vec2(0);

struct Ray{
	vec3 origin;
	vec3 direction;
};

float rand() {
	seed += vec2(gl_FragCoord.xy/screenRes.xy) + ( 1.0f + time/10000);
	return fract(sin(dot(seed, vec2(12.9898, 4.1414))) * 43758.5453);
}

bool intersectAABB(vec3 origin, vec3 dir){
	float tmin, tmax, tymin, tymax, tzmin, tzmax; 
	const vec3 boxMin = vec3(0);
	const vec3 boxMax = vec3(1);
	vec3 bounds[2];
	bounds[0] = boxMin;
	bounds[1] = boxMax;
	const vec3 invdir = 1/origin;
	const ivec3 sign = ivec3((invdir.x < 0), (invdir.y < 0), (invdir.z < 0)); 
 
    tmin = (bounds[sign.x].x - origin.x) * invdir.x; 
    tmax = (bounds[1-sign.x].x - origin.x) * invdir.x; 
    tymin = (bounds[sign.y].y - origin.y) * invdir.y; 
    tymax = (bounds[1-sign.y].y - origin.y) * invdir.y; 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
    if (tymin > tmin) 
        tmin = tymin; 
    if (tymax < tmax) 
        tmax = tymax; 
 
    tzmin = (bounds[sign.z].z - origin.z) * invdir.z; 
    tzmax = (bounds[1-sign.z].z - origin.z) * invdir.z; 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
    if (tzmin > tmin) 
        tmin = tzmin; 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    return true; 
}

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

//geometric term
vec3 evaluateLight(in vec3 pos){
    vec3 L = lightPosition - pos;
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
		vec3 texCoord = vec3(inverse(model) * vec4(cubeShadowPos, 1)) ;
        float density = sampleVolume(texCoord);
		//if(density==0)
		//	break;	//TODO not quite right...
        transmittance *= exp(-extinction * density);
		if(transmittance.x < 0.05)
			break;
    }
    return transmittance;
}

//sample unit sphere to generate random direction 
vec3 randomDir() {
	float e1 = rand();
	float e2 = rand();
	float z = 1.0 - 2.0 * e1;
	float sint = sqrt(1.0 - z * z);
	return vec3(cos(2.0 * PI * e2) * sint, sin(2.0 * PI * e2) * sint, z);
}

//Came from the neutron transport community in the 60s and has various names (delta tracking etc), but was developed by Woodcock. Source: http://www.cs.cornell.edu/courses/cs6630/2012sp/notes/09volpath.pdf
float sampleDistance(){
	float t = -log(1 - rand())/extinction.x;
	return t;
}

vec4 MCPathTracing(vec4 color, vec3 origin, vec3 rayDirection, float depth){
	vec2 tHit = intersectBox(origin, rayDirection);

	if (tHit.x > tHit.y) 
		return color;
	
	tHit.x = max(tHit.x, 0.0);
	vec3 dtVec = 1.0 / (vec3(numberOfSteps) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = cameraPosition + (tHit.x) * rayDirection;

	float currentDensity = 0.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 currentTransmittance = vec3(0.0, 0.0, 0.0);
	float lightDotEye = dot(normalize(rayDirection), normalize(lightPosition));
	float rayLength = 0;

	//for(int b=0; b<4; b++){
		float t = sampleDistance();

		for (float t = tHit.x; t < tHit.y; t += dt) {
			vec3 texCoord = vec3(inverse(model) * vec4(cubePos, 1)) ;
			currentDensity = sampleVolume(texCoord);
			currentTransmittance = exp(-extinction * currentDensity);

			vec3 S = scattering * albedo * volumetricShadow(cubePos, lightPosition);
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

			if(enablePathTracing==1 && rand() > currentTransmittance.x) {
				//rayDirection = randomDir();
				break;
			}
			vec4 cubePosNDC = proj * cam * vec4(cubePos,1);
			if(cubePosNDC.z > depth)
				break;
		}

		rayDirection = randomDir();
		tHit = intersectBox(cubePos, rayDirection);

		//if (tHit.x > tHit.y) 
		//	break;
		
		tHit.x = max(tHit.x, 0.0);
		dtVec = 1.0 / (vec3(numberOfSteps) * abs(rayDirection));
		dt = min(dtVec.x, min(dtVec.y, dtVec.z));
		
	//}

    vec3 finalColor = color.xyz;
    finalColor = vec3(finalColor * totalTransmittance + scatteredLight);

	return vec4(finalColor, 1.0);
}

vec4 singleScattering(vec4 background, vec3 origin, vec3 rayDirection, float depth){
	vec2 tHit = intersectBox(origin, rayDirection); 
	if (tHit.x > tHit.y) 
		return background;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(rayDirection);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));

	vec3 cubePos = cameraPosition + tHit.x * rayDirection;
	float lightDotEye = dot(normalize(rayDirection), normalize(lightPosition));
	vec3 totalTr = vec3(1.0f);
	vec3 sum = vec3(0.0f);

	for(float t = tHit.x; t < tHit.y; t += dt){
		float density = sampleVolume( vec3(inverse(model) * vec4(cubePos, 1)) );
		vec3 currentTr = exp(-extinction * density);
		totalTr *= currentTr;
		
		vec3 Ls = evaluateLight(cubePos) * volumetricShadow(cubePos, lightPosition) * phaseFunction(lightDotEye, g);
		//Integrate Ls from 0 to d
		Ls =  (Ls - Ls * currentTr) / extinction; 

		sum += totalTr * scattering * Ls;
		cubePos += rayDirection * dt;
	}

	return vec4(background.xyz * totalTr + sum, 1.0f); 
}

vec4 refinedVersion(vec4 color, vec3 origin, vec3 rayDirection, float depth){
	vec2 tHit = intersectBox(origin, rayDirection);

	if (tHit.x > tHit.y) 
		return color;
	//if (!intersectAABB(origin, rayDirection)) 
	//	return vec4(0,0,0,0);
	
	tHit.x = max(tHit.x, 0.0);
	vec3 dtVec = 1.0 / (vec3(numberOfSteps) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = cameraPosition + (tHit.x) * rayDirection;

	float currentDensity = 0.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 currentTransmittance = vec3(0.0, 0.0, 0.0);
	float theta = 1.5f;
	float lightDotEye = dot(normalize(rayDirection), normalize(lightPosition));

	for (float t = tHit.x; t < tHit.y; t += dt) {
		vec3 texCoord = vec3(inverse(model) * vec4(cubePos, 1)) ;
		currentDensity = sampleVolume(texCoord);
		currentTransmittance = exp(-extinction * currentDensity);

		vec3 S = scattering * albedo * volumetricShadow(cubePos, lightPosition);
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

		if(enablePathTracing==1 && rand() > currentTransmittance.x) {
			rayDirection = randomDir();
		}
		vec4 cubePosNDC = proj * cam * vec4(cubePos,1);
		if(cubePosNDC.z > depth)
			break;
	}

    //lighting
    vec3 finalColor = color.xyz;

    // Apply scattering/transmittance
    finalColor = vec3(finalColor * totalTransmittance + scatteredLight);

	return vec4(finalColor, 1.0);
}
/*
---------------------------------------
PATH TRACING
---------------------------------------
*/ 

vec3 sampleHemisphere(float e1, float e2){
	float theta = 2*acos(sqrt(1-e1));
	float phi = 2*PI*e2;
	float x = sin(theta)*cos(phi);
	float y = sin(theta)*sin(phi);
	float z = cos(theta);

	return vec3(x,y,z);
}

vec3 randomDirectionInHemisphere(vec3 nl, float e1, float e2){
	float up = e1; // uniform distribution in hemisphere
    float over = sqrt(max(0.0, 1.0 - up * up));
	float around = e2 * PI;
	// from "Building an Orthonormal Basis, Revisited" http://jcgt.org/published/0006/01/01/
	float signf = nl.z >= 0.0 ? 1.0 : -1.0;
	float a = -1.0 / (signf + nl.z);
	float b = nl.x * nl.y * a;
	vec3 T = vec3( 1.0 + signf * nl.x * nl.x * a, signf * b, -signf * nl.x );
	vec3 B = vec3( b, signf + nl.y * nl.y * a, -nl.y );
	return normalize(cos(around) * over * T + sin(around) * over * B + up * nl);
}

vec4 pathTracing(vec4 color, vec3 origin, vec3 rayDirection, float depth){
	//origin is in model space
	vec2 tHit = intersectBox(origin, rayDirection);
	vec4 thisColor = vec4(0);
   
    vec2 frag = vec2(float(gl_FragCoord.x) + rand(), float(gl_FragCoord.y) + rand());
	vec3 screenPosNDC = vec3((frag/screenRes.xy), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam * model ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;
	newRay = normalize(newRay);

	/*float aspectRatio = screenRes.x/screenRes.y;
	float radians45deg = 0.785398f;
	float tan45 = tan(radians45deg / 2.0f);
	vec2 pixelNDC = vec2((gl_FragCoord.x + rand())/screenRes.x, (gl_FragCoord.y + rand())/screenRes.y);
	vec2 pixelScreen = pixelNDC * 2 - 1;
	pixelScreen.x *= aspectRatio;
	pixelScreen *= tan45;
	vec3 cameraSpaceP = vec3(pixelScreen.xy, 1);
	vec3 originCameraSpace = vec3(cam * model * vec4(origin, 1));
	vec3 newRay = cameraSpaceP - originCameraSpace;*/

	//return vec4(((rayDirection+1)/2 - (newRay+1)/2), 1.0);

	for(int i = 0; i < SPP; i++){
		thisColor += MCPathTracing(color, origin, normalize(newRay), depth);
		//thisColor += refinedVersion(color, origin, normalize(newRay), depth);
	}
	
	thisColor = thisColor/float(SPP);
	vec4 previousColor = texture(previousCloud, vec2(gl_FragCoord.xy/screenRes.xy));
	//return vec4(rand(), rand(), rand(), 1);
	//return thisColor;
	if(previousColor.a == 0)
		return thisColor;
	return mix(thisColor, previousColor, 0.98);
}

void main(){
	
	vec4 bg = texture(background, vec2((gl_FragCoord.x)/screenRes.x, gl_FragCoord.y/screenRes.y));
	vec4 thisColor;
	//screenRes.x - gl_FragCoord.x
	float depth = texture(backgroundDepth, vec2((gl_FragCoord.x)/screenRes.x, gl_FragCoord.y/screenRes.y)).r;
	//depth = 2 * depth - 1;
	//depth = (inverse(mvp) * vec4(1, 1, depth, 1)).z;
	vec3 origin = vec3(inverse(model) * vec4(cameraPosition, 1));

	if(enablePathTracing==1)
		thisColor = pathTracing(bg, origin, normalize(rayDirection), depth);
	else
		//thisColor = refinedVersion(bg, origin, normalize(rayDirection), depth);
		thisColor = singleScattering(bg, origin, normalize(rayDirection), depth);

	color = thisColor;
}
