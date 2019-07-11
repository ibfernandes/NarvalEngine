#version 430 core
#define STEPS 64

out vec4 color; 
in vec3 rayDirection;
flat in vec3 eyeObjectSpace;

uniform sampler3D volume;

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

float rayMarch(vec3 eye, vec3 rayDirection){

    vec2 t_hit = intersect_box(eye, rayDirection);
	if (t_hit.x > t_hit.y) {
		discard;
	}

    t_hit.x = max(t_hit.x, 0.0);
    vec3 volumeDimensions = vec3(100);

	vec3 dt_vec = 1.0 / (vec3(volumeDimensions) * abs(rayDirection));
	float dt = min(dt_vec.x, min(dt_vec.y, dt_vec.z)) * 1;
	
    vec3 p = eye + t_hit.x * rayDirection;
    
	float density = 0;

	for (float t = t_hit.x; t < t_hit.y; t += dt) {
	
		density += (1.0 - density) * texture(volume, p).r;

		if (density>= 0.95) {
			break;
		}

		p += rayDirection * dt;
	}

	return density;
}

float wang_hash(int seed) {
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return float(seed % 2147483647) / float(2147483647);
}

void main(){
    vec3 rayDirectionn = normalize(rayDirection);
	vec2 t_hit = intersect_box(eyeObjectSpace, rayDirectionn);
	if (t_hit.x > t_hit.y) {
		discard;
	}
	t_hit.x = max(t_hit.x, 0.0);
	vec3 dt_vec = 1.0 / (vec3(100) * abs(rayDirectionn));
	float dt = 1 * min(dt_vec.x, min(dt_vec.y, dt_vec.z));
	float offset = wang_hash(int(gl_FragCoord.x + 512 * gl_FragCoord.y))/10;
	vec3 p = eyeObjectSpace + (t_hit.x) * rayDirectionn;
	for (float t = t_hit.x; t < t_hit.y; t += dt) {
		float val = texture(volume, p).r;
		vec4 val_color = vec4(vec3(1,1,1), val);
		// Opacity correction
		val_color.a = 1.0 - pow(1.0 - val_color.a, 1);
		color.rgb += (1.0 - color.a) * val_color.a * val_color.rgb;
		color.a += (1.0 - color.a) * val_color.a;
		if (color.a >= 0.95) {
			break;
		}
		p += rayDirectionn * dt;
	}

	//float density = rayMarch(eyeObjectSpace, normalize(rayDirection));
	//color = vec4(1,1,1,density);
}
