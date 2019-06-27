#version 430 core
#define CLOUDS_COLOR vec3(0.6, 0.71, 0.75)
#define MAX_STEPS 100

in vec2 texCoords;
in vec3 rayDirection;
in vec3 eye;
out vec4 color;

uniform sampler3D perlinWorley3;
uniform sampler3D worley3;
uniform sampler2D curl3;
uniform sampler2D weather;
uniform float time;
uniform mat4 inverseProj;
uniform mat4 inverseCam;
uniform mat4 cam;
uniform vec2 screenRes;
uniform vec3 cameraPosition;
uniform vec3 sunDirection = normalize( vec3(-1.0,0.0,-1.0) );

vec4 raymarch(vec3 ro, vec3 rd) {
	float dt = 0;
	float stepSize = 10.0/MAX_STEPS;
	float accumulatedDensity = 0; 

	for(int i=0; i<MAX_STEPS; i++) {
		vec3 distance = ro + dt*rd;
	   	float density = texture(perlinWorley3, distance).x*0.9;
		accumulatedDensity += density * stepSize;
		dt += stepSize;
	}

	return vec4(accumulatedDensity);
}

vec2 convertScreenToNDC(in vec2 screenCoords){
	return 2.0 * (screenCoords.xy)  / screenRes.xy - 1.0;
}

void main(){

	vec4 res = raymarch(eye, rayDirection);

	if(res.x<0.01)
		discard;

    color = res;
}
