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
#define avgExtinction (extinction.x + extinction.y + extinction.z) / 3.0f
#define albedo (scattering/extinction)

uniform float densityCoef;
uniform float invMaxDensity;
uniform float numberOfSteps;
uniform float shadowSteps;

struct Ray{
	vec3 origin;
	vec3 direction;
};

struct RectangleLight {    
	vec3 minVertex;
	vec3 maxVertex;
	vec3 size;
	mat4 transformWCS;
    vec3 Li;
};

uniform RectangleLight lights[MAX_LIGHTS];
uniform int numberOfLights = 1; //TODO hard coded for now

uniform float g;
uniform int SPP;
uniform int frameCount;
uniform float enableShadow;

uniform vec3 cameraPosition;
uniform mat4 model;
uniform mat4 invmodel;
uniform mat4 cam;
uniform mat4 proj;

float seedSum = 0;
vec2 uv = gl_FragCoord.xy/screenRes.xy;
int maxBounces = 4;
vec3 boxMin = (model * vec4(-0.5f, -0.5f, -0.5f,1)).xyz;
vec3 boxMax = (model * vec4(0.5, 0.5, 0.5, 1)).xyz;
float transmittanceThreshold = 0.001f;
int trackingMaxDepth = 200; //If not high enough generates a lot of "light artifacts"

vec3 getPointAt(Ray r, float t){
	return r.origin + t * r.direction;
}

/*
	Returns a random number between [0,1)
*/
float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 getPointOnLight(RectangleLight light){
	vec3 e = vec3(randomUniform(uv), randomUniform(uv), randomUniform(uv));
	vec3 v0 = light.minVertex;
	vec3 size = light.size;
	vec3 pointOnSurface = vec3(v0.x + size.x * e[0], v0.y + size.y * e[1], v0.z + size.z * e[2]);

	pointOnSurface = light.transformWCS * vec4(pointOnSurface, 1.0f);
	
	return pointOnSurface;
}

float getLightPdf(RectangleLight light){
	vec3 sizeWCS = vec3(light.transformWCS[0][0], light.transformWCS[1][1], light.transformWCS[2][2]) * light.size;
	float area = 1;
	
	for (int i = 0; i < 3; i++)
		if (sizeWCS[i] != 0)
			area = area * sizeWCS[i];

	return 1.0f / area;
}

/*	
	Line's/Ray's equation
	o + t*d = y
	t = (y - o)/d
	when t is negative, the box is behind the ray origin 
*/
vec2 intersectBox(vec3 orig, vec3 dir, vec3 bmin, vec3 bmax) {
	vec3 tMinTemp = (bmin - orig) / dir;
	vec3 tmaxTemp = (bmax - orig) / dir;

	vec3 tMin = min(tMinTemp, tmaxTemp);
	vec3 tMax = max(tMinTemp, tmaxTemp);

	float t0 = max(tMin.x, max(tMin.y, tMin.z));
	float t1 = min(tMax.x, min(tMax.y, tMax.z));

	return vec2(t0, t1);
}

vec2 intersectBox(vec3 orig, vec3 dir) {
	return intersectBox(orig, dir, boxMin, boxMax);
}

float sampleVolume(vec3 pos){
	vec3 tex = ( invmodel * vec4(pos,1)).xyz + 0.5; //added 0.5

	return densityCoef * texture(volume, tex).r;
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

/*	
	Perform ratio tracking to estimate the transmittance value
*/
vec3 ratioTrackingTr(Ray incoming, float tNear, float tFar) {

	incoming.origin = getPointAt(incoming, tNear);
	tFar = tFar - tNear;
	tNear = 0;

	float Tr = 1, t = tNear;
	for (int i = 0; i < trackingMaxDepth; i++){
		t -= log(1 - randomUniform(uv)) * invMaxDensity / avgExtinction;
		if (t >= tFar) break;
		float density = sampleVolume(getPointAt(incoming, t));
		Tr *= 1 - max(0.0f, density * invMaxDensity);
		const float rrThreshold = .1;
		if (Tr < rrThreshold) {
			float q = max(0.05f, 1.0f - Tr);
			if (randomUniform(uv) < q) return vec3(0.0f);
			Tr /= 1 - q;
		}
	}

	return vec3(Tr);
}

/*
	Samples a light/medium interaction using Delta Tracking
*/
vec3 sampleDeltaTracking(Ray incoming, inout Ray scattered, float tNear, float tFar) {
	scattered.origin = incoming.origin;
	scattered.direction = incoming.direction;

	float r = randomUniform(uv);

	// Run delta-tracking iterations to sample a medium interaction
	float t = tNear;
	
	for (int i = 0; i < trackingMaxDepth; i++){
		t -= log(1 - r) * invMaxDensity / avgExtinction;
		if (t >= tFar) {
			scattered.origin = getPointAt(incoming, tFar + 0.0001f);
			break;
		}
		float density = sampleVolume(getPointAt(incoming, t));
		float ra = randomUniform(uv);
		if (density * invMaxDensity > ra) {
			scattered.origin = getPointAt(incoming, t);
			scattered.direction = sampleHG(incoming.direction);
			return scattering / extinction;
		}
	}

	return vec3(1.0f);
}

//p0 Intersection Point and p1 point on Light
vec3 visibilityTr(vec3 intersecPoint, vec3 lightPoint){
	Ray ray;
	ray.origin = intersecPoint;
	ray.direction = normalize(lightPoint - intersecPoint);

	vec3 Tr = vec3(1, 1, 1);
	//TODO this here really should be a while...
	for (int i = 0; i < trackingMaxDepth; i++){
		vec2 thit = intersectBox(ray.origin, ray.direction);
		bool insideVolume = thit.x > thit.y ? false : true;

		if (!insideVolume)
			return Tr;

		Tr *= ratioTrackingTr(ray, thit.x, thit.y);

		ray.origin = getPointAt(ray, thit.y + 0.001);
		ray.direction = normalize(lightPoint - ray.origin);
	}
	return Tr;
}

float distanceSquared(vec3 a, vec3 b){
	vec3 dist = a - b;
	return dot(dist, dist);
}

vec3 estimateDirect(Ray incoming, RectangleLight light, vec3 hitPoint) {
	vec3 Ld = vec3(0, 0, 0);

	// Sample light source with multiple importance sampling
	Ray scatteredWo;
	float lightPdf = 0, scatteringPdf = 0;
	//vec3 Li = light.Li / distanceSquared(hitPoint, light.position);
	vec3 Li = light.Li;

	//scatteredWo points from the surface to the light source
	scatteredWo.origin = getPointOnLight(light);
	scatteredWo.direction = normalize(scatteredWo.origin - hitPoint);
	lightPdf = getLightPdf(light);
	

	if (lightPdf > 0 && !isBlack(Li)) {
		// Compute BSDF or phase function's value for light sample
		vec3 f;

		// Evaluate phase function for light sampling strategy
		f = evalHG(incoming.direction, scatteredWo.direction);
		scatteringPdf = f.x;
	

		if (!isBlack(f)) {
			vec3 pointOnLight = getPointOnLight(light);
			Li *= visibilityTr(hitPoint, pointOnLight);

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
	if (true) {
		vec3 f;

		// Sample scattered direction for medium interactions
		f = evalHG(incoming.direction, scatteredWo.direction);
		scatteredWo.direction = sampleHG(incoming.direction);
		scatteringPdf = f.x;
		
		if (!isBlack(f) && scatteringPdf > 0) {
			// Account for light contributions along sampled direction _wi_
			float weight = 1;
			if (true) {
				lightPdf = 1;
				if (lightPdf == 0) return Ld;
				weight = powerHeuristic(scatteringPdf, lightPdf);
			}

			// Find intersection and compute transmittance
			Ray ray;
			ray.origin = hitPoint;
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

vec3 uniformSampleOneLight(Ray incoming, float tNear, float tFar) {
	incoming.origin = getPointAt(incoming, tNear);
	tFar = tFar - tNear;
	tNear = 0;
	float r = randomUniform(uv);
	int i = int(numberOfLights * r);

	float lightPdf = 1.0f / numberOfLights;

	RectangleLight light = lights[i];

//TODO getPointAt near should be LBVH NEAR...
	return estimateDirect(incoming, light, getPointAt(incoming, tNear)) / lightPdf;
}

vec3 integrate(Ray incoming){
	vec3 L = vec3(0, 0, 0);
	vec3 transmittance = vec3(1, 1, 1);
	
	for (int b = 0; b < maxBounces; b++) {
		vec2 t = intersectBox(incoming.origin, incoming.direction);
		
		//if missed
		if ((t.x > t.y) || isBlack(transmittance))
			break;
			
		Ray scattered;
		float tFar = t.y - t.x;
		float tNear = 0;

		//Move ray origin to volume's AABB boundary
		incoming.origin = getPointAt(incoming, t.x);
		tFar = tFar - tNear;
		tNear = 0;

		//Samples the Media for a scattered direction and point. Returns the transmittance from the incoming ray up to that point.
		vec3 sampledTransmittance = sampleDeltaTracking(incoming, scattered, tNear, tFar);
		
		if (isAllOne(sampledTransmittance)) {
			incoming.origin = getPointAt(incoming, tFar + 0.01f);
			//b--; //TODO causing crash
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
		vec3 lightSample = uniformSampleOneLight(scattered, tNear, tFar);
		//return lightSample;

		transmittance *= phaseFr / phasePdf;

		if (isBlack(transmittance)) 
			break;

		L += transmittance * lightSample;
		incoming = scattered;
	}
	
	return L;
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
	thisColor = integrate(Ray(origin, rayDirection));
	
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

void main(){
	vec4 thisColor;
	vec4 bg = texture(background, uv);
	bg = vec4(1, 1, 1, 1); //TODO for test purposes
	float depth = texture(backgroundDepth, uv).r;
	Ray incomingRay = {cameraPosition, normalize(rayDirection)};
	vec2 tHit = intersectBox(incomingRay.origin, incomingRay.direction); 

	thisColor = pathTracing(bg, incomingRay.origin, incomingRay.direction, depth);  

	color = thisColor;
}
