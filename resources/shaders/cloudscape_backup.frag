#version 430 core
#define PI 3.1415
#define EARTH_RADIUS 600000.0
#define SPHERE_INNER_RADIUS (EARTH_RADIUS + 5000.0)
#define SPHERE_OUTER_RADIUS (SPHERE_INNER_RADIUS + 17000.0)
#define BAYER_FACTOR 1.0/16.0
#define CLOUDS_COLOR vec3(0.5,0.5,0.5)
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

in vec3 texCoords;
out vec4 color;

uniform sampler3D perlinWorley3;
uniform sampler3D worley3;
uniform sampler2D curl3;
uniform sampler2D weather;
uniform float time;

uniform vec2 screenRes;
uniform mat4 inverseProj;
uniform mat4 inverseCam;
uniform vec3 sunlightDirection;
uniform float densityFactor = 0.2;
uniform float absorption = 0.0035;
uniform vec3 cameraPosition;
uniform float coverage_multiplier = 0.4;
uniform float curliness = 1.002;

vec3 sphereCenter = vec3(0.0, EARTH_RADIUS, 0.0);
const vec3 windDirection = normalize(vec3(0.5, 0.0, 0.1));

vec3 noiseKernel[6] = vec3[](
    vec3( 0.38051305,  0.92453449, -0.02111345),
	vec3(-0.50625799, -0.03590792, -0.86163418),
	vec3(-0.32509218, -0.94557439,  0.01428793),
	vec3( 0.09026238, -0.27376545,  0.95755165),
	vec3( 0.28128598,  0.42443639, -0.86065785),
	vec3(-0.16852403,  0.14748697,  0.97460106)
);

float bayerFilter[16] = float[]
(
	0.0*BAYER_FACTOR, 8.0*BAYER_FACTOR, 2.0*BAYER_FACTOR, 10.0*BAYER_FACTOR,
	12.0*BAYER_FACTOR, 4.0*BAYER_FACTOR, 14.0*BAYER_FACTOR, 6.0*BAYER_FACTOR,
	3.0*BAYER_FACTOR, 11.0*BAYER_FACTOR, 1.0*BAYER_FACTOR, 9.0*BAYER_FACTOR,
	15.0*BAYER_FACTOR, 7.0*BAYER_FACTOR, 13.0*BAYER_FACTOR, 5.0*BAYER_FACTOR
);


float changeRange( float x, float xmin, float xmax, float a, float b) {
    return (b - a) * (x - xmin) / (xmax - xmin) + a;
}

vec2 getUVProjection(vec3 p){
	return p.xy/SPHERE_INNER_RADIUS + 0.5;
}

float getDensityForCloud(float heightFraction, float cloudType)
{
	float stratusFactor = 1.0 - clamp(cloudType * 2.0, 0.0, 1.0);
	float stratoCumulusFactor = 1.0 - abs(cloudType - 0.5) * 2.0;
	float cumulusFactor = clamp(cloudType - 0.5, 0.0, 1.0) * 2.0;

	vec4 baseGradient = stratusFactor * STRATUS_GRADIENT + stratoCumulusFactor * STRATOCUMULUS_GRADIENT + cumulusFactor * CUMULUS_GRADIENT;

	return smoothstep(baseGradient.x, baseGradient.y, heightFraction) - smoothstep(baseGradient.z, baseGradient.w, heightFraction);
}

 float getHeightFraction(vec3 inPos){
	return (length(inPos - sphereCenter) - SPHERE_INNER_RADIUS)/(SPHERE_OUTER_RADIUS - SPHERE_INNER_RADIUS);
 }

float sampleCloudDensity(vec3 p, float lod, bool expensive){

	float heightFraction = getHeightFraction(p);
	vec3 animation = heightFraction * windDirection * CLOUD_TOP_OFFSET + windDirection * time * CLOUD_SPEED;
	vec2 uv = getUVProjection(p);
	vec2 moving_uv = getUVProjection(p + animation);

	if(heightFraction < 0.0 || heightFraction > 1.0){
		return 0.0;
	}

	vec4 low_frequency_noise = textureLod(perlinWorley3, vec3(uv*CLOUD_SCALE, heightFraction), lod);
	float lowFreqFBM = dot(low_frequency_noise.gba, vec3(0.625, 0.25, 0.125));
	float base_cloud = changeRange(low_frequency_noise.r, -(1.0 - lowFreqFBM), 1., 0.0 , 1.0);
	
	float density = getDensityForCloud(heightFraction, 1.0);
	base_cloud *= (density/heightFraction);

	vec3 weather_data = texture(weather, moving_uv).rgb;
	float cloud_coverage = weather_data.r*coverage_multiplier;
	float base_cloud_with_coverage = changeRange(base_cloud , cloud_coverage , 1.0 , 0.0 , 1.0);
	base_cloud_with_coverage *= cloud_coverage;

	if(expensive)
	{
		vec3 erodeCloudNoise = textureLod(worley3, vec3(moving_uv*CLOUD_SCALE, heightFraction)*curliness, lod).rgb;
		float highFreqFBM = dot(erodeCloudNoise.rgb, vec3(0.625, 0.25, 0.125));//(erodeCloudNoise.r * 0.625) + (erodeCloudNoise.g * 0.25) + (erodeCloudNoise.b * 0.125);
		float highFreqNoiseModifier = mix(highFreqFBM, 1.0 - highFreqFBM, clamp(heightFraction * 10.0, 0.0, 1.0));

		base_cloud_with_coverage = base_cloud_with_coverage - highFreqNoiseModifier * (1.0 - base_cloud_with_coverage);

		base_cloud_with_coverage = changeRange(base_cloud_with_coverage*2.0, highFreqNoiseModifier * 0.2, 1.0, 0.0, 1.0);
	}

	return clamp(base_cloud_with_coverage, 0.0, 1.0);
}


float beerLaw(float densitySample, float p){
    return exp( - densitySample * p);
}

float powderSugarEffect(float densitySample){
    return 1.0 - exp( - densitySample * 2.0);
}

float henyeyGreenstein(float cosAngle, float g){
    return ( (1.0 - g * g) / pow((1.0 + g * g - 2.0 * g * cosAngle), 3.0/2.0)) / 4.0 * PI;
}

float calculateLightEnergy(float densitySample, vec3 lightVector, vec3 viewVector, float g, float p){
     float cosAngle = dot(normalize(lightVector), normalize(viewVector));

    return 2.0 * beerLaw(densitySample, p) * powderSugarEffect(densitySample) * henyeyGreenstein(cosAngle, g);
}


vec3 calculateWind(vec3 p, float heightFraction){
    vec3 windDirection = vec3(1.0, 0.0, 0.0);
    float cloudSpeed = 10.0;
    float cloudTopOffset = 500.0;

    //skew in wind direction
    p += heightFraction * windDirection * cloudTopOffset;
    //animate clouds in wind direction and add a small upward bias
    p+= (windDirection + vec3(0.0, 0.1, 0.0)) * time * cloudSpeed;

    return p;
}

float renderNoiseSlices(vec3 texCoords){
    float noiseValue;

    if(gl_FragCoord.x > 0 && gl_FragCoord.x <= screenRes.x*0.25 )
        noiseValue = texture(perlinWorley3, texCoords).x;
    if(gl_FragCoord.x > screenRes.x*0.25 && gl_FragCoord.x <= screenRes.x*0.5 )
        noiseValue = texture(perlinWorley3, texCoords).y;
    if(gl_FragCoord.x > screenRes.x*0.5 && gl_FragCoord.x <= screenRes.x*0.75 )
        noiseValue = texture(perlinWorley3, texCoords).z;
    if(gl_FragCoord.x > screenRes.x*0.75 && gl_FragCoord.x <= screenRes.x )
        noiseValue = texture(perlinWorley3, texCoords).a;

    return noiseValue;
}

vec3 convertPixelSpaceToNDC(vec2 fragCoord){
    vec2 ndc = 2.0 * fragCoord.xy / screenRes.xy - 1.0;
    return vec3(ndc.xy, 1.0);
}

bool raySphereintersectionSkyMap(vec3 rd, float radius, out vec3 startPos){
	float t;
	vec3 sphereCenter = vec3(0.0);
	float radius2 = radius*radius;

	vec3 l = - sphereCenter;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, l);
	float c = dot(l,l) - radius2;

	float discr = b * b - 4.0 * a * c;
	t = max(0.0, (-b + sqrt(discr))/2);

	startPos = rd * t;

	return true;
}

 float raymarchToLight(vec3 o, float stepSize, vec3 lightDir, float originalDensity, float lightDotEye){

	vec3 startPos = o;
	float ds = stepSize * 6.0;
	vec3 rayStep = lightDir * ds;
	const float CONE_STEP = 1.0/6.0;
	float coneRadius = 1.0; 
	float density = 0.0;
	float coneDensity = 0.0;
	float invDepth = 1.0/ds;
	float sigma_ds = -ds*absorption;
	vec3 pos;

	float T = 1.0;

	for(int i = 0; i < 6; i++){
		pos = startPos + coneRadius*noiseKernel[i]*float(i);

		float heightFraction = getHeightFraction(pos);
		if(heightFraction >= 0){
			
			float cloudDensity = sampleCloudDensity(pos, i/16, density > 0.3);
			if(cloudDensity > 0.0){
				float Ti = exp(cloudDensity*sigma_ds);
				T *= Ti;
				density += cloudDensity;
			}
		}
		startPos += rayStep;
		coneRadius += CONE_STEP;
	}

	//return 2.0*T*powder((originalDensity));//*powder(originalDensity, 0.0);
	return T;
}

vec4 raymarchToCloud(vec3 startPos, vec3 endPos, vec3 bg, out vec4 cloudPos){
	vec3 path = endPos - startPos;
	float len = length(path);

	const int nSteps = 64;
	
	float ds = len/nSteps;
	vec3 dir = path/len;
	dir *= ds;
	vec4 col = vec4(0.0);
	vec2 fragCoord = vec2(gl_FragCoord.x, gl_FragCoord.y);
	int a = int(fragCoord.x) % 4;
	int b = int(fragCoord.y) % 4;
	//startPos += dir * bayerFilter[a * 4 + b];
	startPos += dir * 1;
	vec3 pos = startPos;

	float density = 0.0;

	float lightDotEye = dot(normalize(sunlightDirection), normalize(dir));

	float T = 1.0;
	float sigma_ds = -ds*densityFactor;
	bool expensive = true;
	bool entered = false;

	for(int i = 0; i < nSteps; ++i){	

		float density_sample = sampleCloudDensity(pos, i/16, true);
		if(density_sample > 0.){
			if(!entered){
				cloudPos = vec4(pos,1.0);
				entered = true;	
			}
			
			float height = getHeightFraction(pos);
			vec3 ambientLight = CLOUDS_COLOR;
			float light_density = raymarchToLight(pos, ds*0.1, sunlightDirection, density_sample, lightDotEye);
			float scattering = mix(henyeyGreenstein(lightDotEye, -0.08), henyeyGreenstein(lightDotEye, 0.08), clamp(lightDotEye*0.5 + 0.5, 0.0, 1.0));
			scattering = max(scattering, 1.0);
			float powderTerm =  powderSugarEffect(density_sample);
			
			vec3 S = 0.6*( mix( mix(ambientLight*1.8, bg, 0.2), scattering*SUN_COLOR, powderTerm*light_density)) * density_sample;
			float dTrans = exp(density_sample*sigma_ds);
			vec3 Sint = (S - S * dTrans) * (1. / density_sample);
			col.rgb += T * Sint;
			T *= dTrans;

		}
		
		if( T <= CLOUDS_MIN_TRANSMITTANCE ) break;
		pos += dir;
	}

	col.a = 1.0 - T;

	return col;
}


float threshold(const float v, const float t){
	return v > t ? v : 0.0;
}

bool raySphereintersection(vec3 ro, vec3 rd, float radius, out vec3 startPos){
	
	float t;

	sphereCenter.xz = cameraPosition.xz;

	float radius2 = radius*radius;

	vec3 L = ro - sphereCenter;
	float a = dot(rd, rd);
	float b = 2.0 * dot(rd, L);
	float c = dot(L,L) - radius2;

	float discr = b*b - 4.0 * a * c;
	if(discr < 0.0) return false;
	t = max(0.0, (-b + sqrt(discr))/2);
	if(t == 0.0){
		return false;
	}
	startPos = ro + rd*t;

	return true;
}

void main(){
    vec2 fragCoord = gl_FragCoord.xy;
    vec4 rayNDC = vec4(convertPixelSpaceToNDC(fragCoord), 1.0);
    vec4 rayWCS = inverseProj * rayNDC;
    rayWCS = vec4(rayWCS.xy, -1.0, 0.0);
    vec3 worldDir = (inverseCam * rayWCS).xyz;
    worldDir = normalize(worldDir);

    vec3 startPos, endPos;
    vec4 v = vec4(1.0);
	
	vec3 cubeMapEndPos;
	bool hit = raySphereintersectionSkyMap(worldDir, 0.5, cubeMapEndPos);

	vec4 bg = vec4(SKY_COLOR,1);
	bg = mix( mix( vec4(1.0), vec4(1.0), sunlightDirection.y), bg, pow( max(cubeMapEndPos.y+0.1, .0), 0.2));

    if(cameraPosition.y < SPHERE_INNER_RADIUS - EARTH_RADIUS){
		raySphereintersection(cameraPosition, worldDir, SPHERE_INNER_RADIUS, startPos);
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, endPos);
	}else if(cameraPosition.y > SPHERE_INNER_RADIUS - EARTH_RADIUS && cameraPosition.y < SPHERE_OUTER_RADIUS - EARTH_RADIUS){
		startPos = cameraPosition;
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, endPos);
	}else{
		raySphereintersection(cameraPosition, worldDir, SPHERE_OUTER_RADIUS, startPos);
		raySphereintersection(cameraPosition, worldDir, SPHERE_INNER_RADIUS, endPos);
	}

	vec4 cloudDistance;
    v = raymarchToCloud(startPos,endPos, bg.rgb, cloudDistance);
	//v = vec4(normalize(cloudDistance));
	//color = v;
    cloudDistance = vec4(distance(cameraPosition, cloudDistance.xyz), 0.0, 0.0, 0.0);

	v.rgb = v.rgb * 1.8 - 0.1;

    //v.rgb = mix(v.rgb, bg.rgb*v.a, clamp(0.3,0.,1.));

    float sun = clamp( dot(sunlightDirection,normalize(endPos - startPos)), 0.0, 1.0 );
	vec3 s = 0.8*vec3(1.0,0.4,0.2)*pow( sun, 256.0 );
	//v.rgb += s*v.a;

    bg.rgb = bg.rgb*(1.0 - v.a) + v.rgb;
	bg.a = 1.0;

    color = bg;

}
