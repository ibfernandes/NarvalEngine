#version 430 core

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 eyeObjectSpace;
flat in vec3 translation;
flat in vec3 scale;

uniform sampler3D volume;
uniform sampler3D perlinWorley3;
uniform sampler2D background;
uniform sampler2D backgroundDepth;
uniform float time;
uniform float continuosTime;

#define PI 3.1415926535897932384626433832795
uniform vec2 screenRes = vec2(1600, 900);

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
			break;	//TODO not quite right...
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

//----------------------------------------------------
//PVR 2017 multiple scattering (variant of closed-form tracking as described in algorithm 1, where the light integrator is responsible for computing Ld , Le , and \omega,)
// p45 onwards

struct Ray{
	vec3 origin;
	vec3 direction;
};

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

/*
	Generates a random number on interval [0,1]
*/
float random(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float avg(vec3 vec){
	return (vec.x + vec.y + vec.z)/3;
}

bool intersectBox(Ray r){
	vec2 tHit = intersectBox(r.origin, r.direction);
	if(tHit.y < 0)
		return false;
	else
		return true;
}

bool getNearestHit(Ray ray, out vec3 p){
	vec2 tHit = intersectBox(ray.origin, ray.direction);

	if (tHit.x > tHit.y) 
		return false;
	else{
		p = ray.origin + (tHit.x) * ray.direction;
		return true;
	}
}

//isotropic phase BSDF
void evaluateSample(inout vec3 sampleDirection, inout vec3 L, inout float pdf){
	pdf = 0.25 / PI;
	L = vec3(pdf);
}

void sampleHG(inout vec3 sampleDirection, out vec3 L, out float pdf ){
	float e1 = random(sampleDirection*2);
	float e2 = random(sampleDirection*3);
	float s = 1.0 - 2.0 * e1;
	float cost = (s + 2.0*g*g*g * (-1.0 + e1) * e1 + g*g*s + 2.0*g*(1.0 - e1+e1*e1))/((1.0+g*s)*(1.0+g*s));
	float sint = sqrt(1.0-cost*cost);
	pdf = 0.25 / PI;
	L = vec3(pdf);
	sampleDirection = vec3(cos(2.0 * PI * e2) * sint, sin(2.0 * PI * e2) * sint, cost);
}

void generateSample(inout vec3 sampleDirection, out vec3 L, out float pdf ){
	float r = random(sampleDirection*10);
	float cosTheta =  r * 2.0 - 1.0;
	float sinTheta = 1.0 - cosTheta * cosTheta; 

	sampleDirection.z = cosTheta;

	if(sinTheta > 0.0){
		sinTheta = sqrt(sinTheta);
		r = random(sampleDirection*20);
		float phi = r * 2.0 * PI;
		sampleDirection.x = sinTheta * cos(phi);
		sampleDirection.y = sinTheta * sin(phi);
	}else
		sampleDirection.x = sampleDirection.y = 0;

	sampleDirection = normalize(sampleDirection);
	pdf = 0.25 / PI;
	L = vec3(pdf);
}

/*
	Integrates over interval considering homogeneous absorption only media.
	GetP() = x0
	P = xi
	wo = incident
	wi = nextRay
*/
bool integrateBeerLaw(inout Ray incident, out Ray nextRay, in vec3 x0, in vec3 xi, inout vec3 L, out vec3 transmittance, inout vec3 weight){
	if(!intersectBox(incident))
		return false;
	L = vec3(0);
	float distance = length(xi - x0);
	float density = sampleVolume(xi);
	transmittance = exp(-absorption * distance * density);
	weight = vec3(1);
	nextRay = Ray(xi, incident.direction);
	return true;
}

vec3 scatteringAlbedo;
/*
	Responsible for calculating the next hitpoint x_i 
*/
bool integrateMultipleScattering(inout Ray incident, out Ray nextRay, in vec3 x0, in vec3 xi, inout vec3 L, out vec3 transmittance, inout vec3 weight){
	if(!intersectBox(incident))
		return false;

	float totalDistance = length (xi - x0);
	float r = random(x0);
	float scatterDistance = -log(1.0 - r) / avg(extinction);
	scatterDistance *= 0.05;
	vec3 pdf;
	float density;

	if(scatterDistance < totalDistance){
		xi = x0 + scatterDistance * incident.direction;
		density = sampleVolume(xi);
		transmittance = exp(-extinction * scatterDistance * density);
		vec3 pdf = extinction * transmittance;
		weight = scatteringAlbedo * extinction / pdf;
	}else{
		density = sampleVolume(xi);
		transmittance = exp(-extinction * scatterDistance * density);
		pdf = transmittance;
		weight = 1 / pdf;
	}

	L = vec3(0);
	nextRay = Ray(xi, incident.direction);
	return true;
}

int limit = 0;
/*
	Light integrator
*/
vec4 mainLoop(vec4 color){
	vec3 rayDir = normalize(rayDirection);
	vec3 x0;
	if(!getNearestHit(Ray(eyeObjectSpace, rayDir), x0))
		return color;
	
	vec3 xi =x0;
	vec3 L;
	vec3 transmittance = vec3(1);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 weight;
	Ray incident = Ray(x0, rayDir);
	Ray nextRay;
	
	/*while(limit < 800){

		if(!integrateBeerLaw(incident, nextRay, x0, xi, L, transmittance, weight))
			break;
		x0 = xi;
		xi += nextRay.direction	* 0.001;	
		incident = nextRay;
		totalTransmittance *= transmittance;
		limit++;
	}*/
	 
	while(limit < 500){

		if(!integrateMultipleScattering(incident, nextRay, x0, xi, L, transmittance, weight))
			break;
		x0 = xi;
		xi += nextRay.direction	* 0.001;	
		incident = nextRay;
		totalTransmittance *= transmittance;
		limit++;
	}

    // Apply scattering/transmittance
    return vec4(vec3(color.xyz * totalTransmittance), 1.0);
}

vec3 lightning(){
	return vec3(0);
}

vec4 msLoop(vec4 color){
	vec3 rayDir = normalize(rayDirection);
	vec3 x0;
	if(!getNearestHit(Ray(eyeObjectSpace, rayDir), x0))
		return color;
	
	vec3 xi =x0;
	vec3 L = vec3(0);//scatteredLight
	vec3 Lv = vec3(0);
	vec3 transmittance = vec3(1);
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0); // same as throughput
	vec3 weight;
	Ray incident = Ray(x0, rayDir);
	Ray nextRay;
	vec3 sampleDirection = rayDir;
	
	int j = 0;
	while(j < 800){
		L += totalTransmittance * lightning(); // * directLightning = calculate the whole light scatter thing L*extin * scatter etc p59

		float pdf;
		vec3 Ls;
	
		//generateSample(sampleDirection, Ls, pdf);
		sampleHG(sampleDirection, Ls, pdf);
		totalTransmittance *= Ls / pdf; // same as *= 1;
		incident = Ray(xi, sampleDirection);
		
		if(!integrateMultipleScattering(incident, nextRay, x0, xi, Lv, transmittance, weight)) //p29
			break;
		L += weight * totalTransmittance * Lv;
		totalTransmittance *= transmittance;

		x0 = xi;
		xi += nextRay.direction	* 0.001;	
		incident = nextRay;
		j++;
	}

	return vec4(vec3(color.xyz * totalTransmittance + L), 1.0);
}

/*
	integrates from t=0 until d using simplified VRE
	x_0 = origin of w (w is generated by generateSample at x_0)
	L = radiance along the ray L
	weight = weight of this radiance estimate
	wo = incident ray into the hitpoint (P)
	P = hitpoint
	transmittance = beam transmittance over integration interval
	wi = incident ray?
	//ShadingContext::GetP() = x0 = origin of ray (p.21)
*/
vec3 x0; // hitpoint position 
vec3 omega; //w_i = \omega_i initial ray from eye to hit point 
void integrate(inout Ray wi, inout vec3 L, inout vec3 transmittance, inout vec3 weight, inout vec3 P, inout Ray wo){
	//the volume integrator actually picks the distance d and returns xd in the output parameter P
	// P with omega direction
	//ray (p, dir)
	//getNearestHit(ray, point, geometry g)

	float totalDistance = length (P - x0);
	float xi = random(P);
	float scatterDistance = -log(1.0 - xi) / avg(extinction);
	vec3 pdf;
	//vec3 density = sampleVolume(cubePos);

	if(scatterDistance < totalDistance){
		P = x0 + scatterDistance * wi.direction;
		transmittance = exp(-extinction * scatterDistance);
		vec3 pdf = extinction * transmittance;
		weight = scatteringAlbedo * extinction / pdf;
	}else{
		transmittance = exp(-extinction * scatterDistance);
		pdf = transmittance;
		weight = 1 / pdf;
	}

	L = vec3(0);
	wo = wi;
}
//----------------------------------------------------

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
	vec4 bg = texture(background, vec2((screenRes.x - gl_FragCoord.x)/screenRes.x, gl_FragCoord.y/screenRes.y));
	vec4 thisColor;
	float depth = texture(backgroundDepth, vec2((screenRes.x - gl_FragCoord.x)/screenRes.x, gl_FragCoord.y/screenRes.y)).r;
	depth = 2 * depth - 1;

	if(enablePathTracing == 1)
		//thisColor = pathTracing(gradient);
		//thisColor = mainLoop(gradient);
		thisColor = msLoop(gradient);
	else
		thisColor = refinedVersion(bg, normalize(rayDirection));

	if(gammaCorrection == 1 && thisColor!=gradient)
		thisColor = vec4(pow(thisColor.xyz, vec3(1.0/2.2)), 1.0); // simple linear to gamma, exposure of 1.0

	color = thisColor;
}
