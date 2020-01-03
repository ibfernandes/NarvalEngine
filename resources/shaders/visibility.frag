#version 430 core
#define PI 3.14159265358979323846264338327950288

in vec2 texCoord;
out vec4 color; 


uniform sampler3D volume;
uniform layout(binding = 1, rgba8) image3D visibility;
uniform float currentSlice;
uniform float time;
uniform float numberOfSamples;
uniform float density = 5.0f;
uniform float steps = 32.0f;
uniform vec3 scattering = vec3(0.4, 0.4, 0.1);
uniform vec3 absorption = vec3(0.01);
#define extinction (absorption + scattering)

uniform int lod = 4;

float sampleVolume(vec3 pos){
	return density * texture(volume, pos).r;
}

vec3 sampleAbsorptionOverDirection(vec3 x, vec3 omega){
	float stepSize = 16.0f / 128.0f;

	float currentDensity;
	vec3 cubePos = x;
	vec3 totalTransmittance = vec3(1.0, 1.0, 1.0);
	vec3 currentTransmittance = vec3(0.0, 0.0, 0.0);

	for(int t = 0; t < 32; t++){
		currentDensity = sampleVolume(cubePos);
		currentTransmittance = exp(-absorption * currentDensity);
		totalTransmittance *= currentTransmittance;

		cubePos += omega * stepSize;
	}

	return totalTransmittance;
}

float rand(float n){return fract(sin(n) * 43758.5453123);}

float noise(float p){
	float fl = floor(p);
  float fc = fract(p);
	return mix(rand(fl), rand(fl + 1.0), fc);
}

vec2 convertCartesianCoordToSpherical(float x, float y) {
	vec2 sphericalCoord;
	sphericalCoord.x = 2 * acos(sqrt(1 - x));
	sphericalCoord.y = 2 * PI * y;

	return sphericalCoord;
}

vec3 convertSphericalCoordToCartesian(float theta, float phi) {
	vec3 cartCoord;
	cartCoord.x = sin(theta) * cos(phi);
	cartCoord.y = sin(theta) * sin(phi);
	cartCoord.z = cos(theta);

	return cartCoord;
}

void main(){
	vec3 thisColor = vec3(0);
	if(sampleVolume(vec3(texCoord, currentSlice/128)) != 0){
		for(int i = 0; i < numberOfSamples; i++){
			float a = noise(time);
			float b = noise(time + 1000000);
			vec2 spherical = convertCartesianCoordToSpherical(a, b);
			vec3 cartesian = convertSphericalCoordToCartesian(spherical.x, spherical.y);
			thisColor += sampleAbsorptionOverDirection(vec3(texCoord, currentSlice/128), cartesian);
		}
	}else{
		thisColor = vec3(0);
	}
	color = vec4(thisColor/numberOfSamples, 1.0);

	//imageStore(visibility, ivec3(texCoord, currentSlice), vec4(thisColor/numberOfSamples, 1));
	//imageStore uses pixel coordinates
	imageStore(visibility, ivec3(texCoord * 128, currentSlice)/lod, color);
}
