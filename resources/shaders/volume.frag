#version 430 core

out vec4 color; 
in vec3 rayDirection;
flat in vec3 eyeObjectSpace;

uniform sampler3D volume;
uniform sampler3D perlinWorley3;
uniform float time;
uniform vec3 scattering = 1 * vec3(0.4, 0.4, 0.1);
uniform vec3 absorption = vec3(0.001);
uniform vec3 lightPosition = vec3(0.9);
uniform vec3 lightColor = vec3(0.5, 0.2 , 0.2);
uniform float lightMaxRadius = 0.5;
uniform vec3 materialColor = vec3(1.0, 0.0, 0.0);
uniform vec3 sunLightColor = vec3(252.0/255, 186.0/255, 3.0/255);
uniform float ambientStrength = 0.1;
uniform float densityCoef = 0.75;
uniform float powderCoef = 1.3;
uniform float HGCoef = 0.01;

#define extinction (absorption + scattering)

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


// Get transmittance from a direction and distance onto a point (volume shadows)
vec3 getShadowTransmittance(vec3 cubePos, float sampledDistance, float stepSizeShadow){
    vec3 shadow = vec3(1.0, 1.0, 1.0);
    vec3 lDir = normalize(lightPosition - cubePos);

    for(float tshadow=0.0; tshadow<sampledDistance; tshadow+=stepSizeShadow){

        vec3 cubeShadowPos = cubePos + tshadow*lDir;
        float densityShadow = texture(volume, cubeShadowPos).r;
		//exp(− integral [a,b] extinction )
        shadow += -extinction * densityShadow * stepSizeShadow;
    }
	
    return exp(shadow);
}

// Returns the light distance attenuation
float distanceAttenuation(float distance){
    float linAtt = clamp((lightMaxRadius-distance)/lightMaxRadius,0.0,1.0);
    linAtt*=linAtt;	// some "fake artistic" attenuation
    return linAtt/(distance*distance);
}

vec3 sunDirection = normalize( vec3(-1.0,0.75,1.0) );

float HGPhase(float cosAngle, float g){
	float g2 = g * g;
	return (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5);
}

float beerLaw(float densitySample, float densityCoef){
   return exp( -densitySample * densityCoef);
}
         
float powderEffect(float densitySample, float cosAngle, float densityCoef, float powderCoef){
   float powder = 1.0 - exp(-densitySample * densityCoef * 2.0);
   powder = clamp(powder * powderCoef, 0.0, 1.0);
   return mix(1.0, powder, smoothstep(0.5, -0.5, cosAngle));
}

 float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax){
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float phaseFunction(){
    return 1.0/(4.0*3.14);
}

float sampleVolume(vec3 pos){
	return texture(volume, pos).r *0.3;
}

vec4 bunnyVersion(vec4 color){
	vec3 rayDirection = normalize(rayDirection);
	vec2 tHit = intersectBox(eyeObjectSpace, rayDirection);

	if (tHit.x > tHit.y) {
		discard;
	}
	
	tHit.x = max(tHit.x, 0.0);

	vec3 dtVec = 1.0 / (vec3(64) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = eyeObjectSpace + (tHit.x) * rayDirection;

	vec3 scatteredLuminance = vec3(0.0);
	vec3 transmittance = vec3(1.0);
	float density = 0;

	vec4 sum = vec4(0.0f);

	vec3 lightPosInObjectSpace = (lightPosition - vec3(0,0,4))/vec3(1,1,1);
	vec3 ambientColor = ambientStrength * sunLightColor;

	for (float t = tHit.x; t < tHit.y; t += dt) {
		density += sampleVolume(cubePos);
		float currentDensity = texture(volume, cubePos).r;
		
		float stepSizeShadow = 0.1;
		vec3 shadow = getShadowTransmittance(cubePos, 1.0, 1.0/10.0);
		
		float Ldist = length(lightPosInObjectSpace-cubePos);
		float Lattenuation = distanceAttenuation(Ldist);

		//working one?
		scatteredLuminance += Lattenuation * shadow * transmittance * density * scattering * dt * phaseFunction();
		transmittance *= exp(-density * extinction * dt);

		//VolumetricIntegration - SebH
        /*vec3 S = evaluateLight(p) * sigmaS * phaseFunction();// incoming light
        vec3 Sint = (S - S * exp(-sigmaE * dd)) / sigmaE; // integrate along the current step segment
        scatteredLight += transmittance * Sint; // accumulate and also take into account the transmittance from previous steps
        transmittance *= exp(-sigmaE * dd);*/
		

		//horizon try
		/*float cosAngle = dot(normalize(cubePos), normalize(lightPosInObjectSpace)); //wrong
		vec4 col = vec4(materialColor,1.0) * vec4(2.0 * beerLaw(currentDensity, densityCoef) * powderEffect(currentDensity, cosAngle, densityCoef, powderCoef) * HGPhase(cosAngle,HGCoef));
		sum += col;*/

		cubePos += rayDirection * dt;
	}

	//return vec4(sum.xyzw * 0.1);
	return vec4( ambientColor + transmittance*(color.xyz) + scatteredLuminance, 1.0);
}

void main(){
	color = vec4(0,1,0,1);
	vec4 thisColor = bunnyVersion(color);
	color = thisColor;
	//color = vec4(pow(color.xyz, vec3(1.0/2.2)),1);
}
