#version 430 core
#define PI 3.1415926535897932384626433832795
#define FLOAT_MAX 999999
#define MAX_LEVELS 1000

out vec4 color; 
in vec3 rayDirection;
in vec3 vertCoord;

//LBVH
uniform ivec3 lbvhSize;

//interleaved as node, min, max for LBVH1
// and min, max for LBVH2
layout(std430, binding = 10) readonly buffer nodeBlock
{
    int node[];
};

layout(std430, binding = 11) readonly buffer offsetsBlock
{
    int offsets[];
};

layout(std430, binding = 12) readonly buffer mortonCodesBlock
{
    int mortonCodes[];
};

uniform int levels;
uniform int nodesSize;
uniform isampler3D nodes;

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

vec3 getWCS(int morton) {
	vec3 r = decodeMorton3D(morton);
	return vec3(r.x, r.y, r.z) / lbvhSize;
}

vec3 getWCS(float x, float y, float z) {
	return vec3(x, y, z) / lbvhSize;
}

vec3 getWCS(vec3 m) {
	return getWCS(m.x, m.y, m.z);
}

int setEmptyBit(int mortonCode) {
	return (mortonCode | 0x80000000) & 0x80000000;
}

bool isEmpty(int mortonCode) {
  return ((mortonCode & 0x80000000) == 0x80000000) ? true : false;
}

vec3 traverseTree2(Ray r) {
	int currentNode = 1;
	int currentLevel = 0;

	vec2 t = intersectBox(r.origin, r.direction, getWCS(node[currentNode * 2]), getWCS(node[currentNode * 2 + 1]));
	vec2 finalt = vec2(FLOAT_MAX, -FLOAT_MAX);
	if (t.x > t.y)
		return vec3(finalt, 0);


	float accumulated = 0;
	int firstNodeAtDeepestLevel = int(pow(2, levels - 1));

	int marker[MAX_LEVELS];
	for (int i = 0; i < levels; i++)
		marker[i] = 0;


	while (currentNode != nodesSize) {
		bool miss = false;

		if (isEmpty(node[currentNode * 2]))
			miss = true;
		else {
			t = intersectBox(r.origin, r.direction, getWCS(node[currentNode * 2]), getWCS(node[currentNode * 2 + 1]));
			if (t.x > t.y)
				miss = true;
		}

		marker[currentLevel]++;

		//if miss
		if (miss) {

			int tempNode = currentNode;

			//find rightmost leaf
			while (tempNode * 2 + 1 <= nodesSize)
				tempNode = 2 * tempNode + 1;

			//if rightmost leaf is the last one of the tree, which means we're on root's right child, then this miss means a break 
			if (tempNode == nodesSize)
				break;

			int newLevel;
			//Starts at parent level and checks the first one with an odd marker
			for (int i = currentLevel; i >= 0; i--)
				//if odd, means that we should go to the right child of the parent node of current node at this level
				if (marker[i] % 2 == 1) {
					newLevel = i;
					break;
				}

			int newNode = currentNode;
			for (int i = 0; i <= (currentLevel - newLevel); i++)
				newNode = newNode / 2;

			currentNode = 2 * newNode + 1;
			currentLevel = newLevel;
			continue;
		}

		//If we are checking a leaf node
		if (currentNode >= firstNodeAtDeepestLevel) {
			int offsetsPosition = currentNode - firstNodeAtDeepestLevel;
			int startingIndex;
			int elementsOnThisBucket = 0;

			if (offsetsPosition == 0) {
				startingIndex = 0;
				elementsOnThisBucket = offsets[offsetsPosition];
			}
			else {
				startingIndex = offsets[offsetsPosition - 1];
				elementsOnThisBucket = offsets[offsetsPosition] - offsets[offsetsPosition - 1];
			}

			//for each voxel on this bucket (leaf node), check which ones does in fact intersect this ray.
			for (int i = 0; i < elementsOnThisBucket; i++) {
				int morton = mortonCodes[startingIndex + i];
				vec3 min, max;
				min = decodeMorton3D(morton);
				max = min + 1.0f;

				vec2 t2 = intersectBox(r.origin, r.direction, getWCS(min), getWCS(max));

				//if intersects this voxel at current bucket, accumulate density and update intersection t's
				if (t2.x <= t2.y) {

					accumulated += texture(volume, getWCS(min.x, min.y, min.z)).r;
					if (t2.x < finalt.x)
						finalt.x = t2.x;
					if (t2.y >= finalt.y)
						finalt.y = t2.y;
					//intersectedMorton.push_back(morton);
				}
			}

			//if this leaft node is the left child of its parent, then the next one to test is the right child of its parent.
			if (currentNode % 2 == 0)
				currentNode++;
			else {
				int newLevel;
				//Starts at parent level and checks the first one with an odd marker
				for (int i = currentLevel - 1; i >= 0; i--)
					//if odd, means that we should go to the right child of the parent node of current node at this level
					if (marker[i] % 2 == 1) {
						newLevel = i;
						break;
					}

				int newNode = currentNode;
				for (int i = 0; i <= (currentLevel - newLevel); i++)
					newNode = newNode / 2;

				currentNode = 2 * newNode + 1;
				currentLevel = newLevel;
			}
		}
		else {
			currentNode = 2 * currentNode;
			currentLevel++;

			if (currentNode > nodesSize) {
				currentNode = currentNode / 2;
				currentLevel--;
				currentNode++;
			}


		}

		if (currentNode == nodesSize)
			break;
	}

	return vec3(finalt, accumulated);
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

vec3 traverseTree(Ray r) {
	int currentNode = 1;
	int side = 0;
	vec2 t = intersectBox(r.origin, r.direction, getWCS(node[currentNode * 3 + 1]), getWCS(node[currentNode * 3 + 2]));
	vec2 finalt = vec2(FLOAT_MAX, -FLOAT_MAX);
	int currentMorton = 0;
	float accumulated = 0;
	int vectorSize = int(lbvhSize.x * lbvhSize.y * lbvhSize.z);

	if (t.x > t.y) {
		return vec3(9999.0f, -9999.0f, 0.0f);
	}

	//Three ways of ignoring a sub tree:
	// 1 - empty bit is set to 1, which means there's no density.
	// 2 - Already visited
	// 3 - Doesn't intersect
	//while (currentMorton != (vectorSize - 2)) {
	while (true) {
		currentMorton = node[currentNode * 3];
		t = intersectBox(r.origin, r.direction, getWCS(node[currentNode * 3 + 1]), getWCS(node[currentNode * 3 + 2]));
		
		//if miss
		if (t.x > t.y) {
			int tempNode = currentNode;

			//find leaf
			while (tempNode * 2 + 1 < vectorSize)
				tempNode = 2 * tempNode + 1;

			if (node[tempNode * 3 ] == (vectorSize - 2))
				break;
			else
				currentNode = tempNode;

			int nextParent = node[currentNode * 3] + 1;
			currentMorton = node[currentNode * 3];
			int n = currentNode;

			while (currentMorton != nextParent) {
				n = (n - 0) / 2;
				currentMorton = node[n * 3];
			}
			currentNode = 2 * n + 1;
			side = 0;

			continue;
		}

		vec3 v = decodeMorton3D(node[currentNode * 3]);
		vec2 t2 = intersectBox(r.origin, r.direction, getWCS(v.x, v.y, v.z), getWCS(v.x + 1, v.y + 1, v.z + 1));

		//bool test = false;
		if (t2.x <= t2.y) 
			accumulated += texture(volume, getWCS(v.x, v.y, v.z)).r;

		if (t.x < finalt.x)
			finalt.x = t.x;
		if (t.y >= finalt.y)
			finalt.y = t.y;

		if (currentMorton == (vectorSize - 2))
			break;
 
		//if not a leaf then keep going down
		if (currentNode * 2 + side < vectorSize) 
			currentNode = 2 * currentNode + side;
		else {
			if (side == 1) {
				currentNode = (currentNode - 0) / 2;
				currentNode = 2 * currentNode + 1;
				int nextParent = node[currentNode * 3] + 1;
				currentMorton = node[currentNode * 3];
				int n = currentNode;
				while (currentMorton != nextParent) {
					n = (n - 0) / 2;
					currentMorton = node[n * 3];
				}
				currentNode = 2 * n + 1;
			}
			else {
				currentNode = (currentNode - 0) / 2;
				currentNode = 2 * currentNode + 1;
			}

			side = ++side % 2;
		}

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
			if( int(gl_FragCoord.x) % 2 == 0 && int(gl_FragCoord.y) % 2 == 0){
				float t = traverseTree(incomingRay).z; 
				thisColor = vec4(t, t, t, 1.0f);
			}/*else if (frameCount % 2 == 1 && int(gl_FragCoord.x) % 2 == 1 && int(gl_FragCoord.y) % 1 == 0){
				float t = traverseTree(incomingRay).z; 
				thisColor = vec4(t, t, t, 1.0f);
			}*/
			break;
	}
	
	color = thisColor;
}
