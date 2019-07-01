#version 430 core
#define CLOUDS_COLOR vec3(0.6, 0.71, 0.75)
#define MAX_STEPS 100

in vec3 fragPos;
in vec3 vray_dir;
flat in vec3 transformed_eye;
flat in vec3 transformed_sunLight;
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
uniform vec3 sunLightPosition = vec3(0,10,15);
uniform vec3 sphereCenter;
uniform float sphereRadius;

bool intersectSphere(vec3 ro, vec3 rd, out float t0, out float t1){
    float t = dot(sphereCenter - ro, rd);
	vec3 p = ro + rd*t;
	float y = length (sphereCenter - p);

	if(y<sphereRadius){
		float x = sqrt(sphereRadius * sphereRadius - y * y);
		t0 = t-x;
		t1 = t+x;
		return true;
	}

	return false;
}

vec2 getUVProjection(vec3 p){
	return p.xz/sphereRadius + 0.5;
}

vec4 raymarch(vec3 ro, vec3 rd) {
	float dt = 1;
	float accumulatedDensity = 0; 

	float t0, t1;

	/*if(!intersectSphere(ro,rd,t0,t1))
		return vec4(1,0,0,0);	*/	

	float stepSize = 0.05;

	for(float i=0; i<1; i+=stepSize) {
		vec3 pos = ro + dt*rd;
		float density =  texture(perlinWorley3, pos).x*0.9;
		accumulatedDensity += density * stepSize;
		dt += stepSize;
	}

	return vec4(accumulatedDensity);
}

vec2 intersect_box(vec3 orig, vec3 dir) {
	const vec3 box_min = vec3(0);
	const vec3 box_max = vec3(1);
	vec3 inv_dir = 1.0 / dir;
	vec3 tmin_tmp = (box_min - orig) * inv_dir;
	vec3 tmax_tmp = (box_max - orig) * inv_dir;
	vec3 tmin = min(tmin_tmp, tmax_tmp);
	vec3 tmax = max(tmin_tmp, tmax_tmp);
	float t0 = max(tmin.x, max(tmin.y, tmin.z));
	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	return vec2(t0, t1);
}

vec4 rayMarchCube(){
// Step 1: Normalize the view ray
	vec3 ray_dir = normalize(vray_dir);

	// Step 2: Intersect the ray with the volume bounds to find the interval
	// along the ray overlapped by the volume.
	vec2 t_hit = intersect_box(transformed_eye, ray_dir);
	if (t_hit.x > t_hit.y) {
		discard;
	}
	// We don't want to sample voxels behind the eye if it's
	// inside the volume, so keep the starting point at or in front
	// of the eye
	t_hit.x = max(t_hit.x, 0.0);

	vec3 volume_dims = vec3(64,64,64);
	// Step 3: Compute the step size to march through the volume grid
	vec3 dt_vec = 1.0 / (vec3(volume_dims) * abs(ray_dir));
	float dt = min(dt_vec.x, min(dt_vec.y, dt_vec.z));

	// Step 4: Starting from the entry point, march the ray through the volume
	// and sample it
	vec4 color = vec4(0,0,0,0);
	vec3 p = transformed_eye + t_hit.x * ray_dir;
	for (float t = t_hit.x; t < t_hit.y; t += dt) {
		// Step 4.1: Sample the volume, and color it by the transfer function.
		// Note that here we don't use the opacity from the transfer function,
		// and just use the sample value as the opacity
		float val = texture(perlinWorley3, p).g;
		vec4 val_color = vec4(val);

		// Step 4.2: Accumulate the color and opacity using the front-to-back
		// compositing equation
		color.rgb += (1.0 - color.a) * val_color.a * val_color.rgb;
		color.a += (1.0 - color.a) * val_color.a;

		// Optimization: break out of the loop when the color is near opaque
		if (color.a >= 0.95) {
			break;
		}
		p += ray_dir * dt;
	}
	return color;
}


void main(){

	vec4 res = raymarch(cameraPosition, normalize(fragPos - cameraPosition));
	res = rayMarchCube();

    color = res;
}
