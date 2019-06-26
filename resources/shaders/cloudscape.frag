#version 430 core
#define PI 3.1415
#define EARTH_RADIUS 600000.0
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + 5000.0)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + 17000.0)
#define BAYER_FACTOR 1.0/16.0
#define CLOUDS_COLOR vec3(0.6, 0.71, 0.75)
#define SUN_COLOR vec3(1,0,0)
#define SKY_COLOR vec3(0.1,0.1,0.3)
#define CLOUDS_MIN_TRANSMITTANCE 1e-1
#define CLOUDS_TRANSMITTANCE_THRESHOLD 1.0 - CLOUDS_MIN_TRANSMITTANCE
#define CLOUD_TOP_OFFSET 600.0
#define SATURATE(x) clamp(x, 0.0, 1.0)
#define CLOUD_SCALE 3.0
#define CLOUD_SPEED 1.0
#define STRATUS_GRADIENT vec4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT vec4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT vec4(0.00, 0.1625, 0.88, 0.98)
#define MAX_STEPS 100

in vec3 texCoords;
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

vec4 integrate( in vec4 sum, in float dif, in float den, in vec3 bgcol, in float t )
{
    // lighting
    vec3 lin = vec3(0.65,0.7,0.75)*1.4 + vec3(1.0, 0.6, 0.3)*dif;        
    vec4 col = vec4( mix( vec3(1.0,0.95,0.8), vec3(0.25,0.3,0.35), den ), den );
    col.xyz *= lin;
    col.xyz = mix( col.xyz, bgcol, 1.0-exp(-0.003*t*t) );
    // front to back blending    
    col.a *= 0.4;
    col.rgb *= col.a;
    return sum + col*(1.0-sum.a);
}

vec4 march(vec3 ro, vec3 rd) {
	vec4 sum = vec4(0.0);
	float t = 0;
	float distance = 1.0/MAX_STEPS;

	for(int i=0; i<MAX_STEPS; i++) {
		vec3  pos = ro + t*rd;

		if( pos.y<-3.0 || pos.y>2.0 || sum.a > 0.99 ) break;
	   	float density = texture(perlinWorley3, pos).x-0.7;

	    if( density>0.01 ) {
			pos = pos+0.3*sunDirection;
			float dif =  clamp((density - texture(perlinWorley3, pos).x)/0.6, 0.0, 1.0 );
			sum = integrate( sum, dif, density, CLOUDS_COLOR, t ); 
		} 

		t += distance;
	}

	return sum;
}

vec4 raymarch( in vec3 ro, in vec3 rd, in vec2 px ){
	
	vec4 sum = vec4(0.0);

    sum += march(ro, rd);

    return clamp( sum, 0.0, 1.0 );
}

vec2 convertScreenToNDC(in vec2 screenCoords){
	return 2.0 * screenCoords.xy / screenRes.xy - 1.0;
}

void main(){
	vec4 ndcPoint = vec4(convertScreenToNDC( vec2(gl_FragCoord.xy) ).xy, 1.0, 1.0);

	vec4 rayCamSpace = inverseProj * ndcPoint;
	rayCamSpace = vec4(rayCamSpace.xy, -1.0, 0.0);
	vec3 worldDir = (inverseCam * rayCamSpace).xyz;

	vec4 rayOriginWCS = vec4(cameraPosition.xyz,1);
	vec4 rayDirectionWCS = vec4(worldDir, 1.0);
	
	vec4 res = raymarch( rayOriginWCS.xyz, rayDirectionWCS.xyz, gl_FragCoord.xy );
    color = vec4(CLOUDS_COLOR*(1.0-res.w) + res.xyz, 1.0);
}

/*
#version 430 core
#define PI 3.1415
#define EARTH_RADIUS 600000.0
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + 5000.0)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + 17000.0)
#define BAYER_FACTOR 1.0/16.0
#define CLOUDS_COLOR vec3(0.6, 0.71, 0.75)
#define SUN_COLOR vec3(1,0,0)
#define SKY_COLOR vec3(0.1,0.1,0.3)
#define CLOUDS_MIN_TRANSMITTANCE 1e-1
#define CLOUDS_TRANSMITTANCE_THRESHOLD 1.0 - CLOUDS_MIN_TRANSMITTANCE
#define CLOUD_TOP_OFFSET 600.0
#define SATURATE(x) clamp(x, 0.0, 1.0)
#define CLOUD_SCALE 3.0
#define CLOUD_SPEED 1.0
#define STRATUS_GRADIENT vec4(0.0, 0.1, 0.2, 0.3)
#define STRATOCUMULUS_GRADIENT vec4(0.02, 0.2, 0.48, 0.625)
#define CUMULUS_GRADIENT vec4(0.00, 0.1625, 0.88, 0.98)
#define MAX_STEPS 100

in vec3 texCoords;
in vec4 fragPos;
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
	float stepSize = 1.0/MAX_STEPS;
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
	vec2 uv = (gl_FragCoord.xy - 0.5*screenRes.xy)/screenRes.y;

	vec4 res = raymarch(normalize(cameraPosition.xyz), normalize(vec3(uv, 1)));
    color = res;
}
*/