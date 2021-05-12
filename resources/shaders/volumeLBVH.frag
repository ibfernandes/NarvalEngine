#version 430 core
#define PI 3.1415926535897932384626433832795
#define INV4PI 0.07957747154594766788
#define FLOAT_MAX 999999
#define MAX_LEVELS 1000
#define MAX_LIGHTS 32

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
flat in vec3 translation;
flat in vec3 scale;

//LBVH
//interleaved as node, min, max for LBVH1
// and min, max for LBVH2randomUniform
layout(std140, binding = 1) readonly buffer nodeBlock
{
    ivec4 node[];
};

layout(std140, binding = 2) readonly buffer offsetsBlock
{
    ivec4 offsets[];
};

layout(std140, binding = 3) readonly buffer mortonCodesBlock
{
    ivec4 mortonCodes[];
};

uniform int numberOfNodes;
uniform int levels;
uniform ivec3 lbvhSize;
uniform isampler2D nodeTex;
uniform ivec2 nodeTexSize;
//0 = binary, 1 = quad, 2 = oct
uniform int treeMode = 0;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;
uniform sampler2D previousCloud;

uniform float time;
uniform vec2 screenRes;
uniform int renderingMode;

uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform float densityCoef;
uniform float numberOfSteps;
uniform float shadowSteps;

struct LightPoint {    
    vec3 position;
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
    vec3 Li;
};

uniform LightPoint lightPoints[MAX_LIGHTS];
uniform int numberOfLights = 1; //TODO hard coded for now

uniform float ambientStrength;
uniform float Kc;
uniform float Kl;
uniform float Kq;

uniform float phaseFunctionOption;
uniform float g;
uniform int SPP;
uniform int frameCount;
uniform float enableShadow;

uniform vec3 cameraPosition;
uniform mat4 model;
uniform mat4 invmodel;
uniform mat4 cam;
uniform mat4 proj;

uniform float invMaxDensity;
uniform float densityMc;
vec3 scatteringMc = scattering * densityMc;
vec3 absorptionMc = absorption * densityMc;
vec3 extinctionMc = absorptionMc + scatteringMc;
float avgExtinctionMc = (extinctionMc.x + extinctionMc.y + extinctionMc.z) / 3.0f;

struct Ray{
	vec3 origin;
	vec3 direction;
};

float seedSum = 0;
vec2 uv = gl_FragCoord.xy/screenRes.xy;
int maxBounces = 4;
//vec3 boxMin = (model * vec4(0,0,0,1)).xyz;
vec3 boxMin = (model * vec4(-0.5f, -0.5f, -0.5f,1)).xyz;
//vec3 boxMax = (model * vec4(1,1,1,1)).xyz;
vec3 boxMax = (model * vec4(0.5, 0.5, 0.5, 1)).xyz;
float transmittanceThreshold = 0.001f;

vec3 getPointAt(Ray r, float t){
	return r.origin + t * r.direction;
}

float distanceSquared(vec3 a, vec3 b){
	vec3 dist = a - b;
	return dot(dist, dist);
}
/*
uint seed;
uint wang_hash()
{
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float randomUniform(vec2 uv){    
    return float(wang_hash()) / 4294967296.0;
}*/

/*
	Returns a random number between [0,1)
*/
float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
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

vec3 getPointOnLight(LightPoint light){
	vec3 e = vec3(randomUniform(uv), randomUniform(uv), randomUniform(uv));
	vec3 v0 = light.minVertex;
	vec3 size = light.size;
	vec3 pointOnSurface = vec3(v0.x + size.x * e[0], v0.y + size.y * e[1], v0.z + size.z * e[2]);

	pointOnSurface = vec3(light.transformWCS * vec4(pointOnSurface, 1.0f));
	
	return pointOnSurface;
}

float getLightPdf(vec3 intersecP, vec3 normal, LightPoint light){
	vec3 sizeWCS = vec3(light.transformWCS[0][0], light.transformWCS[1][1], light.transformWCS[2][2]) * light.size;
	float area = 1;
	
	for (int i = 0; i < 3; i++)
		if (sizeWCS[i] != 0)
			area = area * sizeWCS[i];

	//return 1.0f / area;
	return convertAreaToSolidAngle(1.0f / area, normal, intersecP, getPointOnLight(light));
}

/*
	Decodes morton code to 3D point. Assumes ZYX order.
*/
ivec3 decodeMorton3D(int value) {
	ivec3 res;
	res.z = value >> 2;
	res.z = res.z & 0x9249249;
	res.z = (res.z | (res.z >> 2)) & 0x30C30C3;
	res.z = (res.z | (res.z >> 4)) & 0x300F00F;
	res.z = (res.z | (res.z >> 8)) & 0x30000FF;
	res.z = (res.z | (res.z >> 16)) & 0X3FF;

	res.y = value >> 1;
	res.y = res.y & 0x9249249;
	res.y = (res.y | (res.y >> 2)) & 0x30C30C3;
	res.y = (res.y | (res.y >> 4)) & 0x300F00F;
	res.y = (res.y | (res.y >> 8)) & 0x30000FF;
	res.y = (res.y | (res.y >> 16)) & 0X3FF;

	res.x = value;
	res.x = res.x & 0x9249249;
	res.x = (res.x | (res.x >> 2)) & 0x30C30C3;
	res.x = (res.x | (res.x >> 4)) & 0x300F00F;
	res.x = (res.x | (res.x >> 8)) & 0x30000FF;
	res.x = (res.x | (res.x >> 16)) & 0X3FF;

	return res;
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

vec3 decodeSimple3D(int value) {
	uvec3 res;
	uint v = value & 0x3FFFFFFF;
	res.z = v >> 20;
	res.y = (v << 12) >> 22;
	res.x = (v << 22) >> 22;
	return res;
}

vec3 convertToWorldCoordinates(vec3 vec){
	return (vec / lbvhSize) * scale + translation;
}

vec3 getWCS(vec3 v){
	return convertToWorldCoordinates(v);	
}

vec3 getWCSNode(int simpleEncoded){
	vec3 r = decodeSimple3D(simpleEncoded);
	return convertToWorldCoordinates(r);
}

vec3 getWCS(int morton) {
	vec3 r = decodeMorton3D(morton);
	return convertToWorldCoordinates(r);
}

int setEmptyBit(int mortonCode) {
	return (mortonCode | 0x80000000) & 0x80000000;
}

bool isEmpty(int mortonCode) {
  return ((mortonCode & 0x80000000) == 0x80000000) ? true : false;
}

vec3 get3DTexAs1D(int index, isampler3D tex){
	ivec3 coord;
	coord.z = index / (lbvhSize.x * lbvhSize.y);
	index -= (coord.z * lbvhSize.x * lbvhSize.y);
	coord.y = index / lbvhSize.x;
	coord.x = index % lbvhSize.x;

	//Node, min, max
	return texture(tex, coord).rgb;
}

int powBase8(int exponent) {
	return 1 << (exponent * 3);
}

int powBase2(int exponent) {
	return 1 << (exponent);
}

int sumOfBase2(int exponent) {
	int sum = 1;
	for (int i = 1; i <= exponent; i++)
		sum += powBase2(i);
	return sum;
}

int sumOfBase8(int exponent) {
	int sum = 1;
	for (int i = 1; i <= exponent; i++)
		sum += powBase8(i);
	return sum;
}

int sumOfBaseN(int exponent) {
	if(treeMode == 0){
		return sumOfBase2(exponent);
	}else if(treeMode == 2){
		return sumOfBase8(exponent);
	}
	return -1;
}

int getLeftmostChild(int node, int leftmost, int rightmost) {
	if(treeMode == 0)
		return node + (rightmost - node) + 2 * (node - leftmost) + 1;
	else if(treeMode == 2)
		return node + (rightmost - node) + 8 * (node - leftmost) + 1;
}

int getRightmostChild(int node, int leftmost, int rightmost) {
	if(treeMode == 0)
		return getLeftmostChild(node, leftmost, rightmost) + 1;
	else if(treeMode == 2)
		return getLeftmostChild(node, leftmost, rightmost) + 7;
}

int getLeftmosChild(int node, int lvl) {
	int leftmost;
	int rightmost;
	if(treeMode == 0){
		leftmost = sumOfBase2(lvl) - powBase2(lvl) + 1;
		rightmost = sumOfBase2(lvl);
	}else if(treeMode == 2){
		leftmost = sumOfBase8(lvl) - powBase8(lvl) + 1;
		rightmost = sumOfBase8(lvl);
	}
	return getLeftmostChild(node, leftmost, rightmost);
}

int getRightmostChild(int node, int lvl) {
	if(treeMode == 0)
		return getLeftmosChild(node, lvl) + 1;
	else if(treeMode == 2)
		return getLeftmosChild(node, lvl) + 7;
}

int getParent(int node) {
	float div;
	float res;

	if (treeMode == 0) {
		div = node / 2.0f;
		res = floor(div);
	}else if(treeMode == 2) {
		div = node / 8.0f;
		res = ceil(div - 0.125f);
	}
	return int(div);
}

int getNode(int index){
	int arrIndex = index/4;
	int coord = index % 4;
	return node[arrIndex][coord];

	int y = index/nodeTexSize.x;
	int x = index % nodeTexSize.x;
	
	return texelFetch(nodeTex, ivec2(x, y), 0).r;
}

int getOffset(int index){
	int arrIndex = index/4;
	int coord = index % 4;
	return offsets[arrIndex][coord];
}

int getMorton(int index){
	int arrIndex = index/4;
	int coord = index % 4;
	return mortonCodes[arrIndex][coord];
}

vec3 traverseTree(Ray r) {
	int currentNode = 1;
	int currentLevel = 0;
	int offsetMinMax = 2;

	vec2 t = intersectBox(r.origin, r.direction, getWCSNode(getNode(currentNode * offsetMinMax - 1)), getWCSNode(getNode(currentNode * offsetMinMax)));
	vec2 finalt = vec2(99999, -99999);

	if (t.x > t.y)
		return vec3(finalt, 0);

	float accumulated = 0;
	int firstNodeAtDeepestLevel;
	if (treeMode == 0) 
		firstNodeAtDeepestLevel = numberOfNodes - powBase2(levels - 1) + 1;
	else if (treeMode == 2)
		firstNodeAtDeepestLevel = numberOfNodes - powBase8(levels - 1) + 1;

	while (currentNode != numberOfNodes) {
		bool miss = false;

		//If current node is empty, automatically misses
		if (isEmpty(getNode(currentNode * offsetMinMax)))
			miss = true;
		else {
			t = intersectBox(r.origin, r.direction, getWCSNode(getNode(currentNode * offsetMinMax - 1)), getWCSNode(getNode(currentNode * offsetMinMax)));
			if (t.x > t.y)
				miss = true;
		}

		//if miss
		if (miss) {
			//If it's the rightmost node current level, end
			if (currentNode == sumOfBaseN(currentLevel))
				break;

			//if this node is the rightmost child of its parent
			int parent = getParent(currentNode);
			int rightmostChild = getRightmostChild(parent, currentLevel - 1);
			if (rightmostChild == currentNode) {
				currentNode = getParent(currentNode) + 1;
				currentLevel--;
			}else if (getRightmostChild(currentNode, currentLevel) == currentNode) {
				currentNode = getParent(currentNode) + 1;
				currentLevel--;
			}else {
				currentNode = currentNode + 1;
			}

			continue;
		}

		//If we are checking a leaf node
		if (currentNode >= firstNodeAtDeepestLevel) {

			int offsetsPosition = currentNode - firstNodeAtDeepestLevel;
			int startingIndex;
			int elementsOnThisBucket = 0;

			if (offsetsPosition == 0) {
				startingIndex = 0;
				elementsOnThisBucket = getOffset(offsetsPosition);
			}else {
				startingIndex = getOffset(offsetsPosition - 1);
				elementsOnThisBucket = getOffset(offsetsPosition) - getOffset(offsetsPosition - 1);
			}

			//for each voxel on this bucket (leaf node), check which ones does in fact intersect this ray.
			// here we check only mortonCodes that represent non-empty voxels
			for (int i = 0; i < elementsOnThisBucket; i++) {
				int morton = getMorton(startingIndex + i);
				vec3 minVar, maxVar;
				minVar = decodeMorton3D(morton);
				maxVar = minVar + 1.0f;

				vec2 t2 = intersectBox(r.origin, r.direction, getWCS(minVar), getWCS(maxVar));

				//if intersects this voxel at current bucket, accumulate density and update intersection t's
				if (t2.x <= t2.y) {
					//accumulated += texture(volume, getWCS(minVar)).r;
					accumulated += 1;
					if (t2.x < finalt.x)
						finalt.x = t2.x;
					if (t2.y >= finalt.y)
						finalt.y = t2.y;

					float distance = finalt.y - max(finalt.x, 0.0f);
					if (distance > 99999)
						return vec3(finalt, accumulated);
				}
			}

			if (getRightmostChild(getParent(currentNode), currentLevel - 1) == currentNode) {
				currentNode = getParent(currentNode) + 1;
				currentLevel--;
			}else {
				currentNode = currentNode + 1;
			}
		}else {
			currentNode = getLeftmosChild(currentNode, currentLevel);
			currentLevel++;
		}

		if (currentNode == numberOfNodes)
			break;
	}

	return vec3(finalt, accumulated);
}

 float remap(float originalValue, float originalMin, float originalMax, float newMin, float newMax){
	return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float saturate(float value){
	return clamp(value, 0.0, 1.0);
}

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5; //added 0.5

	return densityCoef * texture(volume, tex).r;
}

float sampleVolumeMC(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5; //added 0.5

	return texture(volume, tex).r;
}

float isotropicPhaseFunction(){
	return 1 / (4 * PI);
}

float rayleighPhaseFunction(float theta){
	float cosAngle = cos(theta);
	return (3 * (1 + cosAngle*cosAngle)) / (16 * PI);
}

float henyeyGreensteinPhaseFunction(float cosTheta, float g){
	float g2 = g * g;
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

//geometric term
float evaluateLight(LightPoint light, in vec3 pos){
    float d = length(light.position - pos);
    return (1.0f / (Kc + Kl * d + Kq * d * d));
}

vec3 volumetricShadow(in vec3 cubePos, in vec3 lightPos){
	if(enableShadow==0)
		return vec3(1,1,1);

    vec3 transmittance = vec3(1.0);
    float distance = length(lightPos-cubePos) / shadowSteps;
	vec3 lightDir = normalize(lightPos - cubePos);
	float stepSizeShadow = 1.0f / shadowSteps;

    for(float tshadow = stepSizeShadow; tshadow < distance; tshadow += stepSizeShadow){
        vec3 cubeShadowPos = cubePos + tshadow * lightDir;
		

        float density = sampleVolume(cubeShadowPos);
		if(density == 0)
			continue;
		

		// e ^(-ext * density[i] + -ext * density[i+1] + ...)
        transmittance *= exp(-extinction * density);
		if(transmittance.x < transmittanceThreshold)
			break;
    }
    return transmittance;
}

bool isBlack(vec3 v) {
	return (v.x == 0 && v.y == 0 && v.z == 0) ? true : false;
}

bool isAllOne(vec3 v) {
	return (v.x == 1 && v.y == 1 && v.z == 1) ? true : false;
}

vec3 sphericalToCartesianPre(float cosTheta, float sinTheta, float phi) {
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float powerHeuristic(float pdf0, float pdf1) {
	return (pdf0*pdf0) / (pdf0*pdf0 + pdf1 * pdf1);
}

vec3 sampleHG(vec3 incomingDir) {
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

float pdfHG(vec3 incoming, vec3 scattered) {
	float cosTheta = dot(normalize(incoming), normalize(scattered));
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	return INV4PI * (1.0f - g * g) / (denom * sqrt(denom));
}

vec3 evalHG(vec3 incoming, vec3 scattered) {
	float cosTheta = dot(normalize(incoming), normalize(scattered));
	float denom = 1.0f + g * g + 2.0f * g * cosTheta;
	
	float res = INV4PI * (1.0f - g * g) / (denom * sqrt(denom));
	return vec3(res, res, res);
}

int trackingMaxDepth = 500; //If not high enough generates a lot of "light artifacts"

// Perform ratio tracking to estimate the transmittance value
vec3 ratioTrackingTr(Ray incoming, float tNear, float tFar) {

	incoming.origin = getPointAt(incoming, tNear);
	tFar = tFar - tNear;
	tNear = 0;
	
	float Tr = 1, t = tNear;
	for (int i = 0; i < trackingMaxDepth; i++){
		t -= log(1 - randomUniform(uv)) * invMaxDensity / avgExtinctionMc;
		if (t >= tFar) break;
		float sampledDensity = sampleVolumeMC(getPointAt(incoming, t));
		Tr *= 1 - max(0.0f, sampledDensity * invMaxDensity);
		const float rrThreshold = .1;
		if (Tr < rrThreshold) {
			float q = max(0.05f, 1.0f - Tr);
			if (randomUniform(uv) < q) return vec3(0.0f);
			Tr /= 1 - q;
		}
	}

	return vec3(Tr);
}

//Samples ray scattering using Delta Tracking
//tNear and tFar refer to the incoming ray
vec3 sampleDeltaTracking(Ray incoming, inout Ray scattered, float tNear, float tFar) {
	scattered.origin = incoming.origin;
	scattered.direction = incoming.direction;

	// Run delta-tracking iterations to sample a medium interaction
	float t = tNear;
	
	for (int i = 0; i < trackingMaxDepth; i++){
		float r = randomUniform(uv);
		t -= log(1 - r) * invMaxDensity / avgExtinctionMc;
		if (t >= tFar) {
			scattered.origin = getPointAt(incoming, tFar + 0.0001f);
			break;
		}
		float sampledDensity = sampleVolumeMC(getPointAt(incoming, t));
		float ra = randomUniform(uv);
		if (sampledDensity * invMaxDensity > ra) {
			scattered.origin = getPointAt(incoming, t);
			scattered.direction = sampleHG(incoming.direction);
			return scatteringMc / extinctionMc;
		}
	}

	return vec3(1.0f);
}

//p0 Intersection Point and p1 point on Light
vec3 visibilityTr(vec3 intersecPoint, vec3 lightPoint){
	Ray ray;
	ray.origin = intersecPoint;
	ray.direction = lightPoint - intersecPoint;

	vec3 Tr = vec3(1, 1, 1);
	//TODO this here really should be a while...
	for (int i = 0; i < trackingMaxDepth; i++){
		vec2 thit = intersectBox(ray.origin, ray.direction);
		bool didMiss = (thit.x > thit.y);
		bool bothNegative = (thit.x < 0 && thit.y < 0);
	 
		//if(thit.x <= 0 && thit.y > 0)
			//return vec3(0,1,0);
		if (didMiss || bothNegative)
			return Tr;

		Tr *= ratioTrackingTr(ray, thit.x, thit.y);

		ray.origin = getPointAt(ray, thit.y + 0.001);
		//ray.direction = normalize(lightPoint - ray.origin);
	}
	
	return Tr;
}

vec3 estimateDirect(Ray scattered, LightPoint light) {
	vec3 Ld = vec3(0, 0, 0);

	// Sample light source with multiple importance sampling
	Ray scatteredWo;
	float lightPdf = 0, scatteringPdf = 0;
	//vec3 Li = light.Li / distanceSquared(hitPoint, light.position); //if point light
	vec3 Li = light.Li;
	
	
	//scatteredWo points from the surface to the light source
	//light->sampleLi()
	scatteredWo.origin = scattered.origin;
	scatteredWo.direction = getPointOnLight(light) - scattered.origin;
	lightPdf = getLightPdf(scattered.origin, normalize(scatteredWo.direction), light);
	
	//vec2 th = intersectBox(scatteredWo.origin, scatteredWo.direction);
	//if(th.x <= 0 && th.y > 0)
	//	return vec3(0,1,0);
	

	if (lightPdf > 0 && !isBlack(Li)) {
		// Compute BSDF or phase function's value for light sample
		vec3 f;

		// Evaluate phase function for light sampling strategy
		f = evalHG(scattered.direction, scatteredWo.direction);
		scatteringPdf = f.x;
	

		if (!isBlack(f)) {
			vec3 pointOnLight = getPointOnLight(light);
			Li *= visibilityTr(scattered.origin, pointOnLight);

			// Add light's contribution to reflected radiance
			if (!isBlack(Li)) {
				if (true /*IsDeltaLight(light.flags)*/)
					Ld += f * Li / lightPdf;
				else {
					float weight = powerHeuristic(lightPdf, scatteringPdf);
					Ld += f * Li * weight / lightPdf;
				}
			}
		}
	}

	// Sample BSDF with multiple importance sampling
	if (true) {
		vec3 f;

		// Sample scattered direction for medium interactions
		f = evalHG(scattered.direction, scatteredWo.direction);
		scatteredWo.direction = sampleHG(scattered.direction);
		scatteringPdf = f.x;
		
		if (!isBlack(f) && scatteringPdf > 0) {
			// Account for light contributions along sampled direction _wi_
			float weight = 1;
			if (true) {
				lightPdf = getLightPdf(scattered.origin, normalize(scatteredWo.direction), light);
				if (lightPdf == 0) return Ld;
				weight = powerHeuristic(scatteringPdf, lightPdf);
			}

			// Find intersection and compute transmittance
			Ray ray;
			ray.origin = scattered.origin;
			ray.direction = scatteredWo.direction;

			vec3 Tr = vec3(1, 1, 1);
			//always handle media
			//Li = light.Li / distanceSquared(hitPoint, light.position); //changed from Le to Li
			Li = light.Li; //changed from Le to Li
			if (!isBlack(Li)) 
				Ld += f * Li * Tr * weight / scatteringPdf;
		}
	}
	return Ld;
}

vec3 uniformSampleOneLight(Ray scattered) {
	float r = randomUniform(uv);
	int i = int(numberOfLights * r);

	float lightPdf = 1.0f / numberOfLights;

	LightPoint light = lightPoints[i];

//TODO getPointAt near should be LBVH NEAR...
	return estimateDirect(scattered, light) / lightPdf;
}

vec3 integrateMC(Ray incoming){
	vec3 L = vec3(0, 0, 0);
	vec3 transmittance = vec3(1, 1, 1);
	
	for (int b = 0; b < maxBounces; b++) {
		vec2 t = intersectBox(incoming.origin, incoming.direction);
		
		//if missed
		if ((t.x > t.y || (t.x < 0 && t.y < 0)) || isBlack(transmittance))
			break;
			
		Ray scattered;
		float tFar = t.y - t.x;
		float tNear = 0;

		//Move ray origin to volume's AABB boundary
		incoming.origin = getPointAt(incoming, t.x);

		//Samples the Media for a scattered direction and point. Returns the transmittance from the incoming ray up to that point.
		vec3 sampledTransmittance = sampleDeltaTracking(incoming, scattered, tNear, tFar);
		
		if (isAllOne(sampledTransmittance)) {
			incoming.origin = getPointAt(incoming, tFar + 0.01f);
			b--; //TODO causing crash
			continue;
		}
		
		transmittance *= sampledTransmittance;

		//Evaluates the Phase Function BSDF
		vec3 phaseFr = evalHG(incoming.direction, scattered.direction);
		//Evaluates the Phase Function BSDF PDF
		float phasePdf = pdfHG(incoming.direction, scattered.direction);
		
		//If the BSDF or its PDF is zero, then it is not a valid scattered ray direction
		if (isBlack(phaseFr) || phasePdf == 0.f )
			break;

		//Samples the Radiance arriving from a light source
		vec3 lightSample = uniformSampleOneLight(scattered);

		transmittance *= phaseFr / phasePdf;

		if (isBlack(transmittance)) 
			break;

		L += transmittance * lightSample;
		incoming = scattered;
	}
	
	return L;
}

bool integrate(Ray incomingRay, LightPoint light, vec2 tHit, out vec3 transmittance, out vec3 inScattering){
	if (tHit.x > tHit.y) 
		return false;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(incomingRay.direction);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	float lightDotEye = dot(normalize(incomingRay.direction), normalize(light.position));
	vec3 cubePos = incomingRay.origin + tHit.x * incomingRay.direction;
	vec3 totalTr = vec3(1.0f);
	vec3 sum = vec3(0.0f);
	vec3 currentTr = vec3(0);

	for(float t = tHit.x; t < tHit.y; t += dt){
		float density = sampleVolume(cubePos);
		if(density == 0){
			cubePos += incomingRay.direction * dt;
			continue;
		}
		
		//If accumulated transmittance is near zero, there's no point in continue calculating transmittance
		if(totalTr.x > transmittanceThreshold){
			currentTr = exp(-extinction * density);
			totalTr *= currentTr;
		}
		
		vec3 Ls = ambientStrength * evaluateLight(light, cubePos) * volumetricShadow(cubePos, light.position) * phaseFunction(lightDotEye, g);
		//Integrate Ls from 0 to d
		Ls =  (Ls - Ls * currentTr) / extinction; 

		sum += totalTr * scattering * Ls;
		cubePos += incomingRay.direction * dt;
	}

	transmittance = totalTr;
	inScattering = sum;
	return true;
}

/*
	Sample unit sphere
*/
vec3 sampleSphere(vec2 uv) {
    float e1 = randomUniform(uv);
    float e2 = randomUniform(uv);

	float theta = 2 * PI * e1;
	float phi = acos(1 - 2 * e2);
    vec3 res = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	
    return res;
}

//Came from the neutron transport community in the 60s and has various names (delta tracking etc), but was developed by Woodcock. Source: http://www.cs.cornell.edu/courses/cs6630/2012sp/notes/09volpath.pdf
float sampleDistance(){
	float t = -log(1.0f - randomUniform(uv))/extinction.x;
	return t;
}

bool scatter(Ray incoming, vec2 tHit, inout Ray scattered, inout vec3 transmittance, inout vec3 scatteringOut){
	scattered.origin = incoming.origin + tHit.x * incoming.direction;
	scattered.direction = sampleSphere(uv);
	return true;
}

vec3 calculateColor(Ray incoming, int depth, vec3 background){
	Ray scattered;
	vec3 transmittance = vec3(1.0f);
	vec3 scatteringOut;
	vec3 transmittanceAcc = vec3(1.0f);
	vec3 scatteringAcc = vec3(0.0f);
	int maxBounces = 4;

	for(int i = 0; i < maxBounces; i++){
		vec2 tHit = intersectBox(incoming.origin, incoming.direction);
		
		if (tHit.x > tHit.y) 
			break;
		tHit.y -= 0.00001f;
		tHit.x = max(tHit.x, 0.0f);

		vec3 endPos = incoming.origin + tHit.y * incoming.direction;
		float totalDistance = length(incoming.origin - endPos);
		float scatterDistance = sampleDistance();

		//if(scatterDistance < totalDistance)
		//	tHit.y *= scatterDistance / totalDistance;

		//TODO using first light only as a test...
		integrate(incoming, lightPoints[0], tHit, transmittance, scatteringOut);

		if(transmittance.x < 1.0f)
			scatter(incoming, tHit, scattered, transmittance, scatteringOut);
		else
			continue;
		incoming = scattered;
		transmittanceAcc *= transmittance;
		scatteringAcc += scatteringOut;
	}

	return background * transmittanceAcc + scatteringAcc;
}

vec4 pathTracing(vec4 background, vec3 origin, vec3 rayDirection, float depth){
	vec3 thisColor = vec3(0);
	float e1 = randomUniform(uv) - 0.5f;
	float e2 = randomUniform(uv) - 0.5f;
	vec2 frag = vec2(float(gl_FragCoord.x) + e1, float(gl_FragCoord.y) + e2);
	vec3 screenPosNDC = vec3((frag/screenRes.xy), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;

	//raydir
	thisColor = calculateColor(Ray(origin, rayDirection), 0, background.xyz);
	
	//clear color because the camera moved
	if(frameCount == 0)
		return vec4(thisColor.xyz, 1);
	
	vec4 previousColor = texture(previousCloud, uv);
	
	if(frameCount > 1){
		vec3 n1 = (1.0f / frameCount) * thisColor; 
		thisColor = vec3(previousColor) * 1.0f / (1.0f + 1.0f / (frameCount - 1.0f));
		thisColor += n1;
	}

	return vec4(thisColor, 1);
}

vec4 pathTracingMC	(vec4 background, vec3 origin, vec3 rayDirection, float depth){
	vec3 thisColor = vec3(0);
	float e1 = randomUniform(uv) - 0.5f;
	float e2 = randomUniform(uv) - 0.5f;
	vec2 frag = vec2(float(gl_FragCoord.x) + e1, float(gl_FragCoord.y) + e2);
	vec3 screenPosNDC = vec3((frag/screenRes.xy), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;

	//raydir
	thisColor = integrateMC(Ray(origin, rayDirection));
	
	//clear color because the camera moved
	if(frameCount == 0)
		return vec4(thisColor.xyz, 1);
	
	vec4 previousColor = texture(previousCloud, uv);
	
	if(frameCount > 1){
		vec3 n1 = (1.0f / frameCount) * thisColor; 
		thisColor = vec3(previousColor) * 1.0f / (1.0f + 1.0f / (frameCount - 1.0f));
		thisColor += n1;
	}

	return vec4(thisColor, 1);
}

vec3 calculateLBVHColor(vec3 origin, vec3 rayDirection, vec3 background){
	vec3 res = traverseTree(Ray(origin, rayDirection)); 

	if(res.z > 0){
		vec3 tr = background.xyz * exp(-extinction * res.z);
		return vec3(tr.x, tr.y, tr.z);
	}else
		return background.xyz;
}

vec4 lbvhPathTracing(vec4 background, vec3 origin, vec3 rayDirection, float depth){
	vec3 thisColor = vec3(0);
	float e1 = randomUniform(uv) - 0.5f;
	float e2 = randomUniform(uv) - 0.5f;
	vec2 frag = vec2(float(gl_FragCoord.x) + e1, float(gl_FragCoord.y) + e2);
	vec3 screenPosNDC = vec3((frag/screenRes.xy), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;

	vec4 previousColor = texture(previousCloud, uv);

	//clear color because the camera moved
	if(previousColor.a == 0)
		return vec4(calculateLBVHColor(origin, rayDirection, background.xyz), 1);
	
	if(frameCount > 1){
		int steps = 64;
		thisColor = previousColor.xyz;

		for(int i = 0; i < steps; i++)
			if(frameCount % steps == i && int(gl_FragCoord.x) % steps == i){
				thisColor = calculateLBVHColor(origin, rayDirection, background.xyz);
				break;
			}
		
		vec3 n1 = (1.0f / frameCount) * thisColor; 
		thisColor = vec3(previousColor) * 1.0f / (1.0f + 1.0f / (frameCount - 1.0f));
		thisColor += n1;
	}

	return vec4(thisColor, 1);
}


void main(){
	vec4 thisColor;
	vec3 transmittance;
	vec3 inScattering;
	vec4 bg = texture(background, uv);
	bg = vec4(1, 1, 1, 1); //TODO for test purposes
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	Ray incomingRayNN = {cameraPosition, rayDirection};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 
	//seed = uint(uint(uv.x) * uint(1973) + uint(uv.y) * uint(9277) + uint(frameCount) * uint(26699)) | uint(1);
	
	switch(renderingMode){
		//Ray tracing
		case 0:
			if(integrate(incomingRay, lightPoints[0], tHit, transmittance, inScattering))
				thisColor = vec4(bg.xyz * transmittance + inScattering, 1.0f);
			break;
		//Path tracing Monte carlo
		case 1:
			thisColor = pathTracing(bg, incomingRay.origin, incomingRay.direction, depth); 
			break;
		//LBVH Monte carlo
		case 2:
			thisColor = lbvhPathTracing(bg, incomingRay.origin, incomingRay.direction, depth); 
			break;
		case 3:
			thisColor = pathTracingMC(bg, incomingRayNN.origin, incomingRayNN.direction, depth);  
			break;
	}
	
	color = thisColor;
	//color = vec4(1,0,0,1);
}
