#version 430 core
#define PI 3.1415926535897932384626433832795
#define INV4PI 0.07957747154594766788
#define MAX_LIGHTS 32

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 translation;
flat in vec3 scale;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform mat4 invmodel;
uniform vec3 cameraPosition;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;

vec2 uv = gl_FragCoord.xy/textureSize(backgroundDepth, 0);

/* Volume properties */
uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)
#define avgExtinction ((extinction.x + extinction.y + extinction.z) / 3.0f)
uniform float densityCoef;

/* Phase function properties */
uniform float g;

/* Method Params */
uniform float time;
uniform float invMaxDensity;
uniform float stepSize = 0.1;
uniform int numberOfSteps = 100;
uniform int nPointsToSample = 10;
uniform int meanPathMult = 20;
uniform vec3 lobePoints[100];
float seedSum = 0;
vec3 pointsToSample[100]; //should be [nPointsToSample], points on phase function lobe
vec3 sampledDirections[100]; //should be [nPointsToSample]

/* Light properties */
// 0 = rect, 1 = infAreaLight
struct Light {    
    vec3 position;
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
    vec3 Li;
	int type;
};

uniform Light lights[MAX_LIGHTS];
uniform int numberOfLights = 1;

struct Ray{
	vec3 origin;
	vec3 direction;
};

//Boundaries of a unit cube centered at origin
vec3 boxMin = (model * vec4(-0.5f, -0.5f, -0.5f,1)).xyz;
vec3 boxMax = (model * vec4(0.5, 0.5, 0.5, 1)).xyz;

vec3 getPointAt(Ray r, float t){
	return r.origin + t * r.direction;
}

/*
	Returns a random number between [0,1)
*/
float randomUniform(vec2 uv) {
	uv = vec2(0,0); //fixed
	vec2 seed = uv + fract(0) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 sphericalToCartesianPre(float sinTheta, float cosTheta, float phi) {
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float distanceSquared(vec3 a, vec3 b){
	vec3 dist = a - b;
	return dot(dist, dist);
}

/*
	Transforms from Local Coordinate System(LCS) where the normal vector v is "up" to World Coordinate System (WCS)
	from PBR:
	http://www.pbr-book.org/3ed-2018/Materials/BSDFs.html#BSDF::ss
*/
vec3 toWorld(vec3 v, vec3 ns, vec3 ss, vec3 ts) {
	return vec3(
		ss.x * v.x + ts.x * v.y + ns.x * v.z,
		ss.y * v.x + ts.y * v.y + ns.y * v.z,
		ss.z * v.x + ts.z * v.y + ns.z * v.z);
}

/*
	Transforms from World Coordinate System (WCS) to Local Coordinate System(LCS) where the normal vector v is "up"
*/
vec3 toLCS(vec3 v, vec3 ns, vec3 ss, vec3 ts) {
	return vec3(dot(v, ss), dot(v, ts), dot(v, ns));
}

void generateOrthonormalCS(vec3 normal, inout vec3 v, inout vec3 u) {
	//ns, ss, ts
	if (abs(normal.x) > abs(normal.y))
		v = vec3(-normal.z, 0, normal.x) / sqrt(normal.x * normal.x + normal.z * normal.z);
	else
		v = vec3(0, normal.z, -normal.y) / sqrt(normal.y * normal.y + normal.z * normal.z);

	u = normalize(cross(normal, v));
}


float convertAreaToSolidAngle(float pdfArea, vec3 normal, vec3 p1, vec3 p2) {
	vec3 wi = p1 - p2;

	if (dot(wi, wi) == 0)
		return 0;
	else {
		wi = normalize(wi);
		// Convert from area measure, as returned by the Sample() call
		// above, to solid angle measure.
		pdfArea *= distanceSquared(p1, p2) / abs(dot(normal, -wi));
		if (isinf(pdfArea)) return 0;
	}

	return pdfArea;
}

float getLightPdf(vec3 intersecP, vec3 normal, Light light, vec3 pointOnLight){
	vec3 sizeWCS = vec3(light.transformWCS[0][0], light.transformWCS[1][1], light.transformWCS[2][2]) * light.size;
	float area = 1;
	
	for (int i = 0; i < 3; i++)
		if (sizeWCS[i] != 0)
			area = area * sizeWCS[i];

	//return 1.0f / area;
	return convertAreaToSolidAngle(1.0f / area, normal, intersecP, pointOnLight);
}

/*
	Ray must be in WCS
*/
vec2 intersectBox(vec3 orig, vec3 dir, vec3 bmin, vec3 bmax) {
	//Line's/Ray's equation
	// o + t*d = y
	// t = (y - o)/d
	//when t is negative, the box is behind the ray origin
	vec3 tMinTemp = (bmin - orig) / dir;
	vec3 tmaxTemp = (bmax - orig) / dir;

	vec3 tMin = min(tMinTemp, tmaxTemp);
	vec3 tMax = max(tMinTemp, tmaxTemp);

	float t0 = max(tMin.x, max(tMin.y, tMin.z));
	float t1 = min(tMax.x, min(tMax.y, tMax.z));

	return vec2(t0, t1);
}

/*
	if(t.x == 0)
		origin inside AABB
	if(t.x > t.y)
		miss
*/
vec2 intersectBox(vec3 orig, vec3 dir) {
	return intersectBox(orig, dir, boxMin, boxMax);
}

float density(ivec3 gridPoint){
	return texelFetch( volume, gridPoint, 0).x;
}

float interpolatedDensity(vec3 gridPoint) {
	vec3 pSamples = vec3(gridPoint.x - .5f, gridPoint.y - .5f, gridPoint.z - .5f);
	ivec3 pi = ivec3(floor(pSamples.x), floor(pSamples.y), floor(pSamples.z));
	vec3 d = pSamples - vec3(pi);

	// Trilinearly interpolate density values to compute local density
	float d00 = mix(density(pi), density(pi + ivec3(1, 0, 0)), d.x);
	float d10 = mix(density(pi + ivec3(0, 1, 0)), density(pi + ivec3(1, 1, 0)), d.x);
	float d01 = mix(density(pi + ivec3(0, 0, 1)), density(pi + ivec3(1, 0, 1)), d.x);
	float d11 = mix(density(pi + ivec3(0, 1, 1)), density(pi + ivec3(1, 1, 1)), d.x);
	float d0 = mix(d00, d10, d.y);
	float d1 = mix(d01, d11, d.y);
	return mix(d0, d1, d.z);
}

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5;
	vec3 texGridPoint = tex * textureSize(volume, 0);
	
	//vec3 voxelCenter = floor(texGridPoint) + 0.5f;
	//float dist = distanceSquared(texGridPoint, voxelCenter) * 100;

	return interpolatedDensity(texGridPoint);
	//return density(ivec3(texGridPoint));
}

vec3 evalHG(vec3 incoming, vec3 scattered) {
	float cosTheta = dot(normalize(incoming), normalize(scattered));
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	
	float res = INV4PI * (1.0f - g * g) / (denom * sqrt(denom));
	return vec3(res, res, res);
}

vec3 sampleHG(vec3 incomingDir, vec3 normal) {
	float cosTheta;
	vec2 u = vec2(randomUniform(uv), randomUniform(uv));

	if (abs(g) < 1e-3f)
		cosTheta = 1.0f - 2.0f * u[0];
	else {
		float sqr = (1.0f - g * g) / (1.0f + g - 2.0f * g * u[0]);
		sqr = sqr * sqr;
		cosTheta = -1.0f / (2.0f * g) * (1.0f + g * g - sqr);
	}

	// Compute direction _wi_ for Henyey--Greenstein sample
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = 2.0f * PI * u[1];
	vec3 scattered = sphericalToCartesianPre(sinTheta, cosTheta, phi);
	return scattered;
}

vec3 frameZUp(vec3 scattered){
	vec3 ss, ts;
	vec3 normal = vec3(0,0,1);
	generateOrthonormalCS(normal, ss, ts);

	return toWorld(scattered, normal, ss, ts);
}

vec3 sampleBSDF(vec3 incoming, vec3 normal){
	vec3 scattered = sampleHG(incoming, normal);
	return frameZUp(scattered);
}

/*
	Evals BSDF function Fr(x, w_i, w_o)
*/
vec3 evalBSDF(vec3 incoming, vec3 scattered, vec3 normal) {
	//if ((!sameHemisphere(-incoming, ri.normal) || !sameHemisphere(scattered, ri.normal)))
		//return vec3(0);

	vec3 ss, ts;
	generateOrthonormalCS(normal, ss, ts);
	incoming = toLCS(incoming, normal, ss, ts);
	scattered = toLCS(scattered, normal, ss, ts);

	return evalHG(incoming, scattered);
}

vec3 calculateTr(vec3 A, vec3 B){
	vec3 dir = normalize(B - A);
	float dist = length(B - A);
	Ray r;
	r.origin = A;
	r.direction = dir;
	float Tr = 1;
	
	vec3 absDir = abs(dir);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	dt = dist / numberOfSteps;
	
	for(float i = 0; i < dist; i+=dt){
		float sampledDensity = sampleVolume(r.origin);
		Tr *= exp(-extinction.x * densityCoef * sampledDensity * invMaxDensity);
		if(Tr < 0.01)
			break;
		
		r.origin = getPointAt(r, dt);
	}
	
	return vec3(Tr, Tr, Tr);
}
vec3 sampleLight(Ray rayOnLobeSurface, Light light){
	vec3 Tr = calculateTr(rayOnLobeSurface.origin, light.position);
	//float lightPdf = getLightPdf(rayOnLobeSurface.origin, normalize(rayOnLobeSurface.direction), light, light.position);
	float d = length(light.position - rayOnLobeSurface.origin);
    return light.Li * Tr / (d*d);
	
	//return light.Li * Tr/lightPdf;
}

bool integrate(Ray incoming, Light light, vec2 tHit, out vec3 avgL, out vec3 trAvg){
	vec2 t = intersectBox(incoming.origin, incoming.direction);
	if(t.x < 0)
		t.x = 0;
	
	//if missed
	if ((t.x > t.y || (t.x < 0 && t.y < 0)))
		return false;
	
	incoming.origin = getPointAt(incoming, t.x);
	
	vec3 absDir = abs(incoming.direction);
	float dt = 1.0f / ( 100 * max(absDir.x, max(absDir.y, absDir.z)));
	float avgMeanPath = 1.0f / (avgExtinction * densityCoef);
	trAvg = vec3(0.0f);
	avgL = vec3(0,0,0);
	
	//test if we reach any volume at all (NOT IDEAL)
	float densityBounds = 0;
	for(float x = t.x; x < t.y; x += dt){
		densityBounds = sampleVolume(incoming.origin); //TODO i dont need trilinear interpolation here.
		if(densityBounds > 0)
			break;
		incoming.origin = getPointAt(incoming, dt);
	}
	
	if(densityBounds == 0)
		return false;

	//Stretch and shape the points into the lobe format
	for(int i = 0; i < nPointsToSample; i++){
		vec3 sampledDir = frameZUp(lobePoints[i]);
		Ray r;
		r.origin = incoming.origin;
		r.origin = getPointAt(incoming, avgMeanPath * meanPathMult);
		r.direction = sampledDir;
		
		sampledDirections[i] = r.direction;
		//pointsToSample[i] = getPointAt(r, avgMeanPath * meanPathMult);
		pointsToSample[i] = getPointAt(r, 1);
	}
	
	//TODO shape Lobe size to voxel NEAR AREA i.e. points of the lobe are getting outside in empty space
	//and still being sampled...
	for(int i = 0; i < nPointsToSample; i++){
		float sampledDensity = sampleVolume(pointsToSample[i]);
		float validPoint = (sampledDensity > 0)? 1 : 0; //TODO is that a good solution?
		if(validPoint == 0){
			//trAvg += 1;
			//continue;
		}
		trAvg += calculateTr(incoming.origin, pointsToSample[i]);
	}
	
	vec3 accL = vec3(0,0,0);
	for(int l = 0; l < numberOfLights; l++ ){
		for(int i = 0; i < nPointsToSample; i++){
			float sampledDensity = sampleVolume(pointsToSample[i]);
			float validPoint = (sampledDensity > 0)? 1 : 0; //TODO is that a good solution?
			if(validPoint == 0){
				//continue;
			}
			
			vec3 toLight = normalize(lights[l].position - pointsToSample[i]);
			
			accL += evalBSDF(sampledDirections[i], toLight, -sampledDirections[i]) 
					* sampleLight(Ray(pointsToSample[i], sampledDirections[i]), lights[l]);
		}
		avgL += (scattering /** densityCoef*/) * (accL / nPointsToSample);
		accL = vec3(0,0,0);
	}

	trAvg = trAvg / nPointsToSample;
	
	return true;
}

void main(){
	vec4 thisColor = vec4(0,0,0,0);
	vec3 colorResult;
	vec4 bg = texture(background, uv);
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 
	
	vec3 avgL;
	vec3 avgTr;
	if(integrate(incomingRay, lights[0], tHit, avgL, avgTr))
		thisColor += vec4(bg.xyz * avgTr + avgL , 1.0f);
	else
		thisColor += vec4(bg.xyz, 1);
	
	color = thisColor;
}
