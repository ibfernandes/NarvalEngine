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

vec2 uv = gl_FragCoord.xy/textureSize(backgroundDepth, 0); //TODO change this textureSize here

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
uniform sampler2D previousFrame;
uniform float invMaxDensity;
uniform float time;
uniform int maxBounces = 6;
uniform int SPP;
uniform int frameCount;
float seedSum = 0;
int trackingMaxDepth = 50000; //If not high enough generates a lot of "light artifacts"

/* Light properties */
// 0 = rect, 1 = infAreaLight
struct Light {    
    vec3 position;
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
	vec3 scale;
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

float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

float max3 (vec3 v) {
  return max (max (v.x, v.y), v.z);
}

bool isBlack(vec3 v) {
	return (v.x == 0 && v.y == 0 && v.z == 0) ? true : false;
}

bool isAllOne(vec3 v) {
	return (v.x == 1 && v.y == 1 && v.z == 1) ? true : false;
}

vec3 sphericalToCartesianPre(float sinTheta, float cosTheta, float phi) {
	return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float powerHeuristic(float pdf0, float pdf1) {
	return (pdf0*pdf0) / (pdf0*pdf0 + pdf1 * pdf1);
}

float distanceSquared(vec3 a, vec3 b){
	vec3 dist = a - b;
	return dot(dist, dist);
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
	vec3 sizeWCS = light.scale * light.size;
	float area = 1;
	
	for (int i = 0; i < 3; i++)
		if (sizeWCS[i] != 0)
			area = area * sizeWCS[i];

	//return 1.0f / area;
	return convertAreaToSolidAngle(1.0f / area, normal, intersecP, pointOnLight);
}

vec3 getPointOnLight(Light light){
	vec3 e = vec3(randomUniform(uv), randomUniform(uv), randomUniform(uv));
	vec3 v0 = light.minVertex;
	vec3 size = light.size;
	vec3 pointOnSurface = vec3(v0.x + size.x * e[0], v0.y + size.y * e[1], v0.z + size.z * e[2]);

	pointOnSurface = vec3(light.transformWCS * vec4(pointOnSurface, 1.0f));
	
	return pointOnSurface;
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

	return interpolatedDensity(texGridPoint);
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

/*
	Calculates the Probability Density Function(PDF) of sampling this scattered direction
*/
float pdfBSDF(vec3 incoming, vec3 scattered, vec3 normal) {
	//if ((!sameHemisphere(-incoming, normal) || !sameHemisphere(scattered, normal)))
		//return 0;

	vec3 ss, ts;
	generateOrthonormalCS(normal, ss, ts);
	incoming = toLCS(incoming, normal, ss, ts);
	scattered = toLCS(scattered, normal, ss, ts);

	return pdfHG(incoming, scattered);
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

vec3 sampleBSDF(vec3 incoming, vec3 normal){
	vec3 ss, ts;
	normal = vec3(0,1,0);//TODO temp
	generateOrthonormalCS(normal, ss, ts);
	incoming = toLCS(incoming, normal, ss, ts);
	vec3 scattered = sampleHG(incoming, normal);

	scattered = toWorld(scattered, normal, ss, ts);

	return scattered;
}

// Perform ratio tracking to estimate the transmittance value
vec3 ratioTrackingTr(Ray incoming, float tNear, float tFar) {
	incoming.origin = getPointAt(incoming, tNear);
	
	float Tr = 1, t = tNear;
	for (int i = 0; i < trackingMaxDepth; i++){
		
		t -= log(1 - randomUniform(uv)) * invMaxDensity / (avgExtinction * densityCoef);
		if (t >= tFar) 
			break;
		float sampledDensity = sampleVolume(getPointAt(incoming, t));
		Tr *= 1 - max(0.0f, sampledDensity * invMaxDensity);
		const float rrThreshold = .1;
		
		if (Tr < rrThreshold) {
			float q = max(0.05f, 1.0f - Tr);
			if (randomUniform(uv) < q) 
				return vec3(0.0f);
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
		t -= log(1 - r) * invMaxDensity / (avgExtinction * densityCoef);
		if (t >= tFar) {
			scattered.origin = getPointAt(incoming, tFar + 0.0001f);
			break;
		}
		float sampledDensity = sampleVolume(getPointAt(incoming, t));
		float ra = randomUniform(uv);
		if (sampledDensity * invMaxDensity > ra) {
			scattered.origin = getPointAt(incoming, t);
			scattered.direction = sampleBSDF(incoming.direction, -incoming.direction);
			return scattering / extinction;
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
	
		if (didMiss || bothNegative)
			return Tr;
		
		if(thit.x < 0)
			thit.x = 0;

		Tr *= ratioTrackingTr(ray, thit.x, thit.y);

		ray.origin = getPointAt(ray, thit.y + 0.001);
		ray.direction = lightPoint - ray.origin;
	}
	
	return Tr;
}

vec3 estimateDirect(Ray scattered, Light light) {
	vec3 Ld = vec3(0, 0, 0);

	// Sample light source with multiple importance sampling
	Ray scatteredWo;
	float lightPdf = 0, scatteringPdf = 0;
	
	//vec3 Li = light.Li / distanceSquared(hitPoint, light.position); //if point light
	vec3 Li = light.Li;
	
	//scatteredWo points from the surface to the light source
	//light->sampleLi()
	vec3 pointOnLight = getPointOnLight(light);
	scatteredWo.origin = scattered.origin;
	scatteredWo.direction = pointOnLight - scattered.origin;
	//TODO it was -normalize(scatteredWo.direction) and it was giving brigther iamges...
	lightPdf = getLightPdf(scattered.origin, -normalize(scattered.direction), light, pointOnLight);
	//vec2 th = intersectBox(scatteredWo.origin, scatteredWo.direction);
	//if(th.x <= 0 && th.y > 0)
	//	return vec3(0,1,0);
	
	if (lightPdf > 0 && !isBlack(Li)) {
		// Compute BSDF or phase function's value for light sample
		vec3 f;

		// Evaluate phase function for light sampling strategy
		if(false /*isSurfaceInteraction*/){
		}else{
			f = evalBSDF(scattered.direction, scatteredWo.direction, -scattered.direction);
			scatteringPdf = f.x;
		}
	

		if (!isBlack(f)) {
			vec3 pointOnLight = getPointOnLight(light);
			Li *= visibilityTr(scattered.origin, pointOnLight);

			// Add light's contribution to reflected radiance
			if (!isBlack(Li)) {
				if (false /*IsDeltaLight(light.flags)*/)
					Ld += f * Li / lightPdf;
				else {
					float weight = powerHeuristic(lightPdf, scatteringPdf);
					Ld += f * Li * weight / lightPdf;
				}
			}
		}
	}

	// Sample BSDF with multiple importance sampling
	if (true  /*!IsDeltaLight(light.flags)*/) {
		vec3 f;
		
		if(false /*isSurfaceInteraction*/){
		}else{
			// Sample scattered direction for medium interactions
			f = evalBSDF(scattered.direction, scatteredWo.direction, -scattered.direction);
			scatteredWo.direction = sampleBSDF(scattered.direction, -scattered.direction);
			scatteringPdf = f.x;
		}
		
		if (!isBlack(f) && scatteringPdf > 0) {
			// Account for light contributions along sampled direction _wi_
			float weight = 1;
			if (true /*!sampledSpecular*/) {
				lightPdf = getLightPdf(scattered.origin, normalize(scattered.direction), light, pointOnLight);
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

	Light light = lights[i];

//TODO getPointAt near should be LBVH NEAR...
	return estimateDirect(scattered, light) / lightPdf;
}

vec3 integrate(Ray incoming){
	vec3 L = vec3(0, 0, 0);
	vec3 transmittance = vec3(1);
	
	for (int b = 0; b < maxBounces; b++) {
		vec2 t = intersectBox(incoming.origin, incoming.direction);
		if(t.x < 0)
			t.x = 0;
		
		//if missed
		if ((t.x > t.y || (t.x < 0 && t.y < 0)) || isBlack(transmittance))
			break;
			
		Ray scattered;
		
		//Move ray origin to volume's AABB boundary
		incoming.origin = getPointAt(incoming, t.x);
		
		float tFar = t.y - t.x;
		float tNear = 0;

		//Samples the Media for a scattered direction and point. Returns the transmittance from the incoming ray up to that point.
		vec3 sampledTransmittance = sampleDeltaTracking(incoming, scattered, tNear, tFar);
		
		if (isAllOne(sampledTransmittance)) {
			incoming.origin = getPointAt(incoming, tFar + 0.01f);
			b--;
			continue;
		}
		
		transmittance *= sampledTransmittance;

		//Evaluates the Phase Function BSDF
		vec3 phaseFr = evalBSDF(incoming.direction, scattered.direction, -incoming.direction);
		//Evaluates the Phase Function BSDF PDF
		float phasePdf = pdfBSDF(incoming.direction, scattered.direction, -incoming.direction);
		
		//If the BSDF or its PDF is zero, then it is not a valid scattered ray direction
		if (isBlack(phaseFr) || phasePdf == 0.f )
			break;

		//Samples the Radiance arriving from a light source
		vec3 lightSample = uniformSampleOneLight(scattered);

		transmittance *= phaseFr / phasePdf;

		if (isBlack(transmittance)) 
			break;

		//transmittance always less than 1
		//lightSample always less than its Li.

		L += transmittance * lightSample;
		
		incoming = scattered;
		
		float rrThreshold = 0.01;
		if (max3(transmittance) < rrThreshold && b > 3) {
            float q = max(0.05f, 1.0f - max3(transmittance));
            if (randomUniform(uv) < q) break;
            transmittance /= 1.0f - q;
        }
	}
	
	return L;
}

vec4 pathTracing(vec4 bg, vec3 origin, vec3 rayDirection, float depth){
	vec3 thisColor = vec3(0);
	float e1 = randomUniform(uv) - 0.5f;
	float e2 = randomUniform(uv) - 0.5f;
	vec2 frag = vec2(float(gl_FragCoord.x) + e1, float(gl_FragCoord.y) + e2);
	vec3 screenPosNDC = vec3((frag/textureSize(backgroundDepth, 0)), gl_FragCoord.z) * 2.0f - 1.0f;
	vec4 pixelModelCoord = vec4(screenPosNDC, 1.0f)/gl_FragCoord.w;
	pixelModelCoord = vec4(inverse( proj * cam ) * pixelModelCoord);
	vec3 newRay = vec3(pixelModelCoord) - origin;
	
	if(frameCount > SPP)
		return texture(previousFrame, uv);

	//raydir
	thisColor = integrate(Ray(origin, rayDirection));
	
	//clear color because the camera moved
	if(frameCount == 0)
		return vec4(thisColor.xyz, 1);
	
	vec4 previousColor = texture(previousFrame, uv);
	
	if(frameCount > 1){
		vec3 n1 = (1.0f / frameCount) * thisColor; 
		thisColor = vec3(previousColor) * 1.0f / (1.0f + 1.0f / (frameCount - 1.0f));
		thisColor += n1;
	}

	return vec4(thisColor, 1);
}

void main(){
	vec4 thisColor;
	vec3 colorResult;
	vec4 bg = texture(background, uv);
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 

	thisColor = pathTracing(bg, incomingRay.origin, incomingRay.direction, depth);

	color = thisColor;
}
