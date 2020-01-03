#version 430 core

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 eyeObjectSpace;
flat in vec3 translation;
flat in vec3 scale;

uniform sampler3D visibility;
uniform sampler3D light;

#define PI 3.1415926535897932384626433832795

uniform float densityCoef = 1.0;
uniform float numberOfSteps = 128;

uniform vec3 lightPosition = vec3(0.9);
uniform vec3 absorption = vec3(0.001);
uniform float bands = 3;

uniform vec3 gradientUp = vec3(0.0, 0.0, 0.0);
uniform vec3 gradientDown = vec3(0.0, 0.0, 0.0);
uniform vec2 screenRes = vec2(1024, 512);
vec3 lightPosInObjectSpace = (lightPosition - translation)/scale;

uniform int lod = 4;

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

//TODO: this
float sampleLightCoeff(vec3 pos, int l){
	//mod

	vec3 finalPos;
	finalPos.x = pos.x + l;
	finalPos.x /= 128 * bands * bands;
	finalPos.y = pos.y / 128;
	finalPos.z = pos.z / 128;

	finalPos.x /= lod;
	finalPos.y /= lod;
	finalPos.z /= lod;



	return texture(light, finalPos).r;
}

float sampleVisibility(vec3 pos){
	return densityCoef*texture(visibility, pos).r;
}

float isotropicPhaseFunction(){
	return 1 / (4 * PI);
}

vec3 gs(vec3 x){
	vec3 sum = vec3(0);
	ivec3 coeffPos;

	coeffPos.x = int(x.x * 128 * bands * bands);
	coeffPos.y = int(x.y * 128 );
	coeffPos.z = int(x.x * 128);

	coeffPos.x /= lod;
	coeffPos.y /= lod;
	coeffPos.z /= lod;

	int offset = coeffPos.x % int(bands);
	coeffPos.x -= offset; 


	for(int l = 0; l < bands * bands; l++){
		sum += sampleLightCoeff(coeffPos, l) * sampleVisibility(x);
	}
	return isotropicPhaseFunction() * sum; 
}


vec4 refinedVersion(vec4 color, vec3 rayDirection){
	vec2 tHit = intersectBox(eyeObjectSpace, rayDirection);

	if (tHit.x > tHit.y) 
		discard;
	
	tHit.x = max(tHit.x, 0.0);
	vec3 dtVec = 1.0 / (vec3(numberOfSteps) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = eyeObjectSpace + (tHit.x) * rayDirection;

	float currentDensity = 0.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 currentTransmittance = vec3(0.0, 0.0, 0.0);
	float lightDotEye = dot(normalize(rayDirection), normalize(lightPosInObjectSpace));

	for (float t = tHit.x; t < tHit.y; t += dt) {
		currentTransmittance = exp(- absorption);
		
		scatteredLight += gs(cubePos) * totalTransmittance; 
		
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


void main(){
	vec4 down = vec4( gradientUp,1);
    vec4 up = vec4( gradientDown,1);
	
	vec4 gradient = vec4(mix(up, down, gl_FragCoord.y/screenRes.y));
	vec4 thisColor;

	thisColor = refinedVersion(gradient, normalize(rayDirection));

	color = thisColor;
}
