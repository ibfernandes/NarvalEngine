#version 430 core
#define CLOUDS_COLOR vec3(0.6, 0.71, 0.75)
#define MAX_STEPS 100

in vec3 fragPos;
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

bool cube(vec3 org, vec3 dir, out float near, out float far){
	// compute intersection of ray with all six bbox planes
	vec3 invR = 1.0/dir;
	vec3 tbot = invR * (-0.5 - org);
	vec3 ttop = invR * (0.5 - org);
	
	// re-order intersections to find smallest and largest on each axis
	vec3 tmin = min (ttop, tbot);
	vec3 tmax = max (ttop, tbot);
	
	// find the largest tmin and the smallest tmax
	vec2 t0 = max(tmin.xx, tmin.yz);
	near = max(t0.x, t0.y);
	t0 = min(tmax.xx, tmax.yz);
	far = min(t0.x, t0.y);

	// check for hit
	return near < far && far > 0.0;
}

vec4 raymarch(vec3 ro, vec3 rd) {
	float dt = 1;
	float stepSize = 1.0/MAX_STEPS;
	float accumulatedDensity = 0; 


	for(int i=0; i<MAX_STEPS; i++) {

		vec3 pos = ro + dt*rd;

	   	float density =  texture(perlinWorley3, pos).x*0.9;
		accumulatedDensity += density * stepSize;
		dt += stepSize;
	}

	return vec4(accumulatedDensity);
}

void main(){

	vec4 res = raymarch(cameraPosition, fragPos - cameraPosition);

    color = res;
}
