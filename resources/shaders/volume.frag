#version 430 core
#define STEPS 64

out vec4 color; 
in vec3 rayDirection;
flat in vec3 eyeObjectSpace;

uniform sampler3D volume;
uniform float time;

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

vec3 scattering = vec3(0.2, 0.5, 1.0);
//the higher the absorption
vec3 absorption = vec3(0.4);
vec3 lightPosition = vec3(time);
vec3 L = vec3(0.5, 0.5, 0.5);
#define extinction (absorption + scattering)
#define tr 3
#define PI 3.14

/*
that Li(c, −v) = Lo(p, v), where c
is the camera position, p is the intersection point of the closest surface with the view
ray, and v is the unit view vector pointing from p to c
*/
//the closer to 0 the more it abosordbs light.
//the closer to 1 the more it scatters light.
vec3 calculateAlbedo(vec3 color){
	vec3 albedo = scattering / (scattering + absorption);
	return color * albedo;
}

//calculates transmittance using beer-lambert law.
//Tr (c, p)
vec3 calculateTransmittance(vec3 p1, vec3 p2){
	float depth = length(p1 - p2);
	vec3 t = depth * extinction;
	return exp(-t);
}

//Henyey-Greenstein phase function
//g in [-1,1]
float phaseFunction(float g, float theta){
	return (1 - g*g)/(4*PI*pow(1 + g*g - 2*g*cos(theta), 1.5));
}

vec4 bookVersion(){
 	vec3 rayDirection = normalize(rayDirection);
	vec2 t_hit = intersectBox(eyeObjectSpace, rayDirection);

	if (t_hit.x > t_hit.y) {
		discard;
	}

	t_hit.x = max(t_hit.x, 0.0);
	vec3 dt_vec = 1.0 / (vec3(100) * abs(rayDirection));
	float dt = 1 * min(dt_vec.x, min(dt_vec.y, dt_vec.z));
	vec3 cubePos = eyeObjectSpace + (t_hit.x) * rayDirection;
	vec4 finalColor = vec4(0,0,0,0);

	//c = camera
	//p intersection point closest surface
	// v unit ray from p to c (p - c)
	for (float t = t_hit.x; t < t_hit.y; t += dt) {
		float density = texture(volume, cubePos).r;
		
		cubePos += rayDirection * dt;
	}
	
	//finalColor = calculateTransmittance()*Lo(p,v) + integral o -> p-c Tr * Lscat* scattering * dt
	return finalColor;
}


vec4 original(vec4 color){
	vec3 rayDirection = normalize(rayDirection);
	vec2 t_hit = intersectBox(eyeObjectSpace, rayDirection);
	if (t_hit.x > t_hit.y) {
		discard;
	}
	t_hit.x = max(t_hit.x, 0.0);
	vec3 dt_vec = 1.0 / (vec3(100) * abs(rayDirection));
	float dt = 1 * min(dt_vec.x, min(dt_vec.y, dt_vec.z));
	vec3 cubePos = eyeObjectSpace + (t_hit.x) * rayDirection;
	vec3 scatteredLuminance = vec3(0.0,0.0,0.0);
	vec3 transmittance = vec3(1.0);

	for (float t = t_hit.x; t < t_hit.y; t += dt) {
		float density = texture(volume, cubePos).r;
		vec4 val_color = vec4(vec3(1,1,1), density);

		// Opacity correction
		val_color.a = 1.0 - pow(1.0 - val_color.a, 1);
		color.rgb += (1.0 - color.a) * val_color.a * val_color.rgb;
		color.a += (1.0 - color.a) * val_color.a;

		if (color.a >= 1) {
			break;
		}
		
		cubePos += rayDirection * dt;
	}
	return color;
}

// Get transmittance from a direction and distance onto a point (volume shadows)
vec3 getShadowTransmittance(vec3 cubePos, float sampledDistance, float stepSizeShadow){
    vec3 shadow = vec3(1.0, 1.0, 1.0);
    vec3 lDir = normalize(lightPosition - cubePos);

    for(float tshadow=0.0; tshadow<sampledDistance; tshadow+=stepSizeShadow){

        vec3 cubeShadowPos = cubePos + tshadow*lDir;
        float densityShadow = texture(volume, cubeShadowPos).r;
		//exp(− integral [a,b] extinction )
        shadow += -densityShadow * extinction * stepSizeShadow;

    }
	
    return exp(shadow);
}

// Returns the light distance attenuation
float distanceAttenuation(float distance)
{
    float lightMaxRadius = 3.0;
    float linAtt = clamp((lightMaxRadius-distance)/lightMaxRadius,0.0,1.0);
    linAtt*=linAtt;	// some "fake artistic" attenuation
    return linAtt/(distance*distance);
}

vec4 bunnyVersion(){
	vec3 rayDirection = normalize(rayDirection);
	vec2 tHit = intersectBox(eyeObjectSpace, rayDirection);

	if (tHit.x > tHit.y) {
		discard;
	}
	
	tHit.x = max(tHit.x, 0.0);

	vec3 dtVec = 1.0 / (vec3(64) * abs(rayDirection));
	float dt = min(dtVec.x, min(dtVec.y, dtVec.z));
	vec3 cubePos = eyeObjectSpace + (tHit.x) * rayDirection;

	vec3 scatteredLuminance = vec3(0.0,0.0,0.0);
	vec3 transmittance = vec3(1.0);

	float density = 0;
	for (float t = tHit.x; t < tHit.y; t += dt) {
		density += texture(volume, cubePos).r;
		
		float stepSizeShadow = 0.1;
		vec3 shadow = getShadowTransmittance(cubePos, 1.0, 0.1);
		
		float Ldist = length(lightPosition-cubePos);
		float Lattenuation = distanceAttenuation(Ldist);
		// Scattered luminance ignores phase function (assumes L has it baked in)
		// This is not energy conservative.
		scatteredLuminance += Lattenuation * shadow * transmittance * density *scattering * dt * L;       
		transmittance *= exp(-density * extinction * dt);
		
		cubePos += rayDirection * dt;
	}

	return vec4(transmittance + scatteredLuminance, density);
}

void main(){
        
	vec4 thisColor = bunnyVersion();
	color = vec4(thisColor);

	//color = bookVersion();
	//color = original(color);

	//float density = rayMarch(eyeObjectSpace, normalize(rayDirection));
	//color = vec4(1,1,1,density);
}
