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


uniform float param1 = 0;
uniform float param2 = 0;
uniform float param3 = 0;

vec3 sunDirection = normalize( vec3(-1.0,0.75,1.0) );

float hash( float n ) { return fract(sin(n)*753.5453123); }

float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);
	
    float n = p.x + p.y*157.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+157.0), hash(n+158.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
}

#define NUM_OCTAVES 5

float fbm(vec3 x) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(100);
    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(x);
        x = x * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

float snoise2(vec4 x) {
    /*x *= 4.0;
    vec3 y = x.xyz;
    y += x.w;
    return noise(y);
	*/
    x *= 0.4;
    vec3 y = x.xyz;
    y += x.w;
    return fbm(y);
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

float sampleVolumeR(vec3 pos){
	return densityCoef*texture(volume, pos).r;
}

float sampleVolume(vec3 pos){
	float radius = 0.4;
	float center = 0.5;
	float x = pos.x - center;
	x *= x;
	float y = pos.y - center;
	y *= y;
	float z = pos.z - center;
	z *= z;

	
	vec4 sampleNoise = texture(volume, (pos + continuosTime)*0.5);

	float shapeNoise = sampleNoise.g * 0.625 + sampleNoise.b * 0.25 + sampleNoise.a * 0.125;
	shapeNoise = -(1 - shapeNoise);
	shapeNoise = remap(sampleNoise.r, shapeNoise, 1.0, 0.0, 1.0);

	float distance = length (pos - vec3(center)) + 1;

	if(x+y+z <= radius*radius)
		return shapeNoise * densityCoef ;

	if(x+y+z > radius*radius &&  + x+y+z < radius*radius + noise(pos*param1)*param2/param3){
		if(enableAnimation == 1)
			return densityCoef * smoothstep(0.1, 0.4, noise((pos + continuosTime)*4)*0.6);
		else
		return densityCoef * smoothstep(0.1, 0.4, noise(pos*4)*0.6);
	}
	//return texture(volume, pos + continuosTime).g * densityCoef ;
	return 0;
}

float isotropicPhaseFunction(){
	return 1 / (4 * PI);
}

float rayleighPhaseFunction(float theta){
	float cosAngle = cos(theta);
	return (3 * (1 + cosAngle*cosAngle)) / (16 * PI);
}

float henyeyGreensteinPhaseFunction(float theta, float g){
	float cosAngle = cos(theta);
	float g2 = g * g;
	return (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5);
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
    return lightColor * 100.0 / dot(L, L);
}

vec3 volumetricShadow(in vec3 from, in vec3 to){

    const float numStep = shadowSteps; // quality control. Bump to avoid shadow alisaing
    vec3 shadow = vec3(1.0);
    float sigmaS = 0.0;
    float sigmaT = 0.0;
    float dd = length(to-from) / numStep;

    for(float s=0.5; s<(numStep-0.1); s+=1.0){
        vec3 pos = from + (to-from)*(s/(numStep));
        shadow *= exp(-extinction * dd);
    }
    return shadow;
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

	float d = 1.0;
	float dd = 0.0;
	vec3 scatteredLight = vec3(0.0, 0.0, 0.0);
	vec3 transmittance = vec3(1.0, 1.0, 1.0);

	for (float t = tHit.x; t < tHit.y; t += dt) {
		float theta = 1.5f;
		vec3 S = scattering * albedo * phaseFunction(theta, g) * volumetricShadow(cubePos, lightPosInObjectSpace);
		vec3 Sglobal = ambientStrength *  S;
		S = evaluateLight(cubePos) * S;
		
		vec3 Sint = (S - S * exp(-extinction * dd)) / extinction; 

		vec3 SintGlobal = (Sglobal - Sglobal * exp(-extinction * dd)) / extinction; 
		scatteredLight += transmittance * (Sint + SintGlobal); 
		
		transmittance *= exp(-extinction * dd);

		cubePos += rayDirection * dt;
		dd = sampleVolumeR(cubePos);
		d += dd;
	}

    //lighting
    vec3 finalColor = color.xyz;

    // Apply scattering/transmittance
    finalColor = vec3(finalColor * transmittance + scatteredLight);

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
