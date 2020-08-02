#version 430 core
#define PI 3.1415926535897932384626433832795
#define FLOAT_MAX 999999
#define MAX_LEVELS 1000

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;
in vec3 translation;
in vec3 scale;

//LBVH
//interleaved as node, min, max for LBVH1
// and min, max for LBVH2
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

uniform int nodesSize;
uniform int numberOfNodes;
uniform int levels;
uniform ivec3 lbvhSize;
uniform isampler2D nodeTex;
uniform ivec2 nodeTexSize;
uniform isampler2D offsetsTex;
uniform isampler2D mortonCodesTex;
//0 = binary, 1 = quad, 2 = oct
uniform int treeMode = 0;

uniform sampler3D volume;
uniform sampler2D background;
uniform sampler2D backgroundDepth;
uniform sampler2D previousCloud;

uniform float time;
uniform vec2 screenRes;
uniform int renderingMode;

uniform vec3 resolution;
uniform vec3 scattering;
uniform vec3 absorption;
#define extinction (absorption + scattering)
#define albedo (scattering/extinction)

uniform float densityCoef;
uniform float numberOfSteps;
uniform float shadowSteps;

uniform vec3 lightPosition;
uniform vec3 lightColor;
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

struct Ray{
	vec3 origin;
	vec3 direction;
};

float seedSum = 0;
vec2 uv = gl_FragCoord.xy/screenRes.xy;
int maxBounces = 4;
vec3 boxMin = (model * vec4(0,0,0,1)).xyz;
vec3 boxMax = (model * vec4(1,1,1,1)).xyz;
float transmittanceThreshold = 0.001f;

/*
	Returns a random number between [0,1)
*/
float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
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
	/*int arrIndex = index/4;
	int coord = index % 4;
	return node[arrIndex][coord];*/

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
	vec3 tex = ( invmodel * vec4(pos,1)).xyz;

	return densityCoef * texture(volume, tex).r;
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
float evaluateLight(in vec3 pos){
    float d = length(lightPosition - pos);
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
		

        transmittance *= exp(-extinction * density);
		if(transmittance.x < transmittanceThreshold)
			break;
    }
    return transmittance;
}

bool integrate(Ray incomingRay, vec2 tHit, out vec3 transmittance, out vec3 inScattering){
	if (tHit.x > tHit.y) 
		return false;

	tHit.x = max(tHit.x, 0.0f);
	vec3 absDir = abs(incomingRay.direction);
	float dt = 1.0f / ( numberOfSteps * max(absDir.x, max(absDir.y, absDir.z)));
	float lightDotEye = dot(normalize(incomingRay.direction), normalize(lightPosition));
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
		
		vec3 Ls = ambientStrength * evaluateLight(cubePos) * volumetricShadow(cubePos, lightPosition) * phaseFunction(lightDotEye, g);
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

		integrate(incoming, tHit, transmittance, scatteringOut);

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
	
	vec4 previousColor = texture(previousCloud, uv);
	
	//clear color because the camera moved
	if(previousColor.a == 0)
		return vec4(thisColor.xyz, 1);
	
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
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 

	
	switch(renderingMode){
		//Ray tracing
		case 0:
			if(integrate(incomingRay, tHit, transmittance, inScattering))
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
	}
	
	color = thisColor;
}
