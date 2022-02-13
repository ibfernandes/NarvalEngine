#version 430 core
#define MAX_LIGHTS 32
#define PI 3.14159265359
#define DIFFUSE_TERM DIFFUSE_TERM_BURLEY
#define SSS_SAMPLE_COUNT 128
#define EPSILON 0.001

//Notations:
// L = Light vector pointing towards the light source.
// V = View vector pointing towards the viewer eye.
// H = Halfway vector. Calculated as V+L.
// LoH = Dot product between L and H.
// NoH = Dot product between N and H.
// SM_ = Shadow Masking.
// D_ = Normal Distribution Function, a.k.a. NDF.
// V_ = Visibility function.
// G_ = Geometry shadowing function.
// F_ = Fresnel term.
// Fd = Diffuse term.
// Fs = Specular term.
// Fr = Fd + Fs

out vec4 color; 

in VERTOUT{
    vec2 texCoord;
    vec3 normalCoord;
    vec3 worldCoord;
    vec4 fragPosLightSpace;
    mat3 TBN;
} fragIn;

struct Material{
    sampler2D diffuse;
    sampler2D metallic;
    sampler2D normal;
    sampler2D roughness;
    sampler2D subsurface;
    sampler2D ao;
};

struct MaterialIsSet{
    bool diffuse;
    bool metallic;
    bool normal;
    bool roughness;
    bool subsurface;
    bool ao;
    bool isLight;
};

struct LightPoint {    
    vec3 position;
    vec3 color;
};

uniform LightPoint lightPoints[MAX_LIGHTS];
uniform int numberOfLights = 0;

uniform Material material;
uniform MaterialIsSet materialIsSet;
uniform vec3 cameraPosition;
uniform sampler2D shadowMap;
uniform float time;

//Parameters
uniform sampler2D preIntegratedSSSBRDFLUT;
uniform sampler2D preIntegratedShadowSSSBRDFLUT;
uniform vec3 sheenColor = vec3(1, 1, 1); 
uniform float sheenFactor = 0.0f;
uniform float sssCurvatureFactor = 1.0f;


//----------------------------------------------
// Commons
//----------------------------------------------
/**
* Normals are stored assuming the Z-axis as up.
*/
vec3 getNormalFromMap(){
    vec3 tangentNormal = texture(material.normal, fragIn.texCoord).xyz * 2.0 - 1.0;

    vec3 q1  = dFdx(fragIn.worldCoord);
    vec3 q2  = dFdy(fragIn.worldCoord);
    vec2 st1 = dFdx(fragIn.texCoord);
    vec2 st2 = dFdy(fragIn.texCoord);

    //normal, tangent and bitangent vectors
    vec3 n   = normalize(fragIn.normalCoord);
    vec3 t  = normalize(q1 * st2.t - q2 * st1.t);
    vec3 b  = -normalize(cross(n, t));

    mat3 tbn = mat3(t, b, n);

    return normalize(tbn * tangentNormal);
}

float random (vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec3 fwidth(vec3 p){
     return abs(dFdx(p)) + abs(dFdy(p));
}

//----------------------------------------------
// Fresnel terms
//----------------------------------------------
//Fresnel term by [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
//where cosTheta is usually the View dot Halfway.
vec3 F_Schlick(float cosTheta, vec3 F0, vec3 F90){
    return F0 + (F90 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

//----------------------------------------------
// Visibility/Geometry terms
//----------------------------------------------
// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float NoV, float NoL) {
    return 1.0f / (4.0f * (NoL + NoV - NoL * NoV));
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float geometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float V_Smith(float NoV, float NoL, float roughness){
    float ggx2 = geometrySchlickGGX(NoV, roughness);
    float ggx1 = geometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

//----------------------------------------------
// Distribution (NDF) terms
//----------------------------------------------
//A.k.a. Velvet Distribution. Ashikhmin 2007, "Distribution-based BRDFs".
float D_Ashikhmin(float roughness, float NoH) {
    float a2 = roughness * roughness;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
    float sin4h = sin2h * sin2h;
    float cot2 = -cos2h / (a2 * sin2h);
    return 1.0 / (PI * (4.0 * a2 + 1.0) * sin4h) * (4.0 * exp(cot2) + sin4h);
}

// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF". Original Shadowing term omitted and using [Neubelt13] instead.
float D_Charlie(float roughness, float NoH) {
    float invAlpha  = 1.0 / max(roughness, EPSILON);
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

/**
* halfway is defined as the microfacet's normal.
*/
float D_GGX(float NoH, float roughness){
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(NoH, 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

//----------------------------------------------
// Diffuse terms
//----------------------------------------------
//Lambert diffuse term.
float Fd_Lambert(){
    return 1.0/ PI;
}

//Burley diffuse term. [Burley 2012, "Physically-Based Shading at Disney"]
vec3 Fd_Burley(float roughness, float NoV, float NoL, float LoH){
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    vec3 lightScatter = F_Schlick(NoL, vec3(1.0), vec3(f90));
    vec3 viewScatter = F_Schlick(NoV, vec3(1.0), vec3(f90));
    return lightScatter * viewScatter * (1.0 / PI);
}

vec3 Fd(float roughness, float NoV, float NoL, float LoH){
#if DIFFUSE_TERM == DIFFUSE_TERM_LAMBERT
    return vec3(Fd_Lambert());
#elif DIFFUSE_TERM == DIFFUSE_TERM_BURLEY
    return Fd_Burley(roughness, NoV, NoL, LoH);
#endif
}
//----------------------------------------------
// Cloth BRDF (a.k.a. sheen)
//----------------------------------------------

vec3 sheenBRDF(float NoH, float NoV, float NoL, float roughness, LightPoint light){
  // specular BRDF
  float a2 = roughness * roughness;
  float D = D_Charlie(a2, NoH); 
  float V = V_Neubelt(NoV, NoL);
  vec3 F = sheenColor;
  vec3 Fs = (D * V) * F;

  return Fs;
}

//----------------------------------------------
// Cook-Torrance BRDF
//----------------------------------------------
vec3 cookTorranceBRDF(float NoH, float NoV, float NoL, float HoL, float roughness, vec3 albedo, vec3 F0){
    float NDF = D_GGX(NoH, roughness);   
	float G = V_Smith(NoV, NoL, roughness);      
	vec3 F = F_Schlick(max(HoL, 0.0), F0, vec3(1.0f));

    vec3 numerator = NDF * G * F; 
	float denominator = 4 * max(NoV, 0.0) * max(NoL, 0.0) + 0.001; // 0.001 to prevent divide by zero.
	vec3 Fs = numerator / denominator;

    vec3 Fd = albedo * Fd(roughness, NoV, NoL, HoL);

    return Fd + Fs;
}

//----------------------------------------------
// Sub Surface Scattering (SSS)
//----------------------------------------------
vec3 skinDiffuse(float curvature, float NoL){
    return texture(preIntegratedSSSBRDFLUT, vec2(NoL, curvature)).xyz;
}

vec3 skinShadow(float shadow, float width){ 
    return texture(preIntegratedShadowSSSBRDFLUT, vec2(shadow, width)).xyz;
}

vec3 skinBRDF(vec3 normal, vec3 worldPos, float NoL){
    float curvature = length(fwidth(normal)) / length(fwidth(worldPos)); 
    curvature *= sssCurvatureFactor;
    curvature = clamp(curvature, 0.0f, 1.0f);
    return skinDiffuse(curvature, NoL);
}

//----------------------------------------------
// Bringing it all together
//----------------------------------------------

float calculateShadow(vec3 normal, vec3 lightDir){
    //divide by w to get NDC [-1,1]
    vec3 projCoords = fragIn.fragPosLightSpace.xyz / fragIn.fragPosLightSpace.w;
    //and now transform NDC to [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float bias;
    bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float currentDepth = projCoords.z;  
    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	
    //Pecentage-closer filtering PCF 
    for(int x = -1; x <= 1; x++)
        for(int y = -1; y <= 1; y++){
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias) > pcfDepth  ? 1.0 : 0.0;        
        }    
    
    shadow /= 9.0;

    return shadow;
}

vec3 calculateLightPoint(LightPoint light){
	vec3 albedo     = pow(materialIsSet.diffuse ? texture(material.diffuse, fragIn.texCoord).rgb : vec3(1,1,1), vec3(2.2));
    float metallic  = materialIsSet.metallic ? texture(material.metallic, fragIn.texCoord).r : 0.0f;
    float roughness =  materialIsSet.roughness ? texture(material.roughness, fragIn.texCoord).r : 0.0f;
    float subsurface =  materialIsSet.subsurface ? texture(material.subsurface, fragIn.texCoord).r : 0.0f;
    float ao        = materialIsSet.ao ? texture(material.ao, fragIn.texCoord).r : 1.0f;

    vec3 normal;
    //If the normal texture is set, use it. If not, use vertex normals.
    if(materialIsSet.normal)
        normal = getNormalFromMap();
    else
        normal = fragIn.normalCoord;
    vec3 toLight = normalize(light.position - fragIn.worldCoord);
    vec3 toCam = normalize(cameraPosition - fragIn.worldCoord);

    vec3 L = normalize(light.position - fragIn.worldCoord);
    vec3 N = normal;
    vec3 H = normalize(toCam + toLight);
    vec3 V = toCam;
    float NoL = max(0, dot(N, L));
    float NoH = max(0, dot(N, H));
    float NoV = max(0, dot(N, V));
    float LoH = max(0, dot(L, H));

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    //BRDF/BSDF terms
    vec3 diffuseTerm = cookTorranceBRDF(NoH, NoV, NoL, LoH, roughness, albedo, F0);
    vec3 sheenTerm = sheenBRDF(NoH, NoV, NoL, roughness, light);
    vec3 sssTerm = skinBRDF(normal, fragIn.worldCoord, NoL);
    vec3 mettalicTerm = vec3(metallic);

    //Weights
    float diffuseWeight = 1.0f - metallic;

    vec3 finalTerm = vec3(0, 0, 0);
    //finalTerm += diffuseWeight * diffuseTerm;
    //finalTerm += sheenFactor * sheenColor * sheenTerm;
    finalTerm += sssTerm;

    float distance = length(light.position - fragIn.worldCoord);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = (light.color) * attenuation;

	vec3 Lo = finalTerm * radiance * max(0.0, NoL);
	
    float shadow = calculateShadow(normal, toLight);
    return (1.0 - shadow) * Lo;
}

void main(){
	vec3 Lo = vec3(0.0);

    if(materialIsSet.isLight)
        color = vec4(pow(materialIsSet.diffuse ? texture(material.diffuse, fragIn.texCoord).rgb : vec3(1,1,1), vec3(2.2)), 1);
	else{
	    for(int i=0; i < numberOfLights; i++)
		    Lo += calculateLightPoint(lightPoints[i]);
        vec3 finalColor = Lo;
        color = vec4(finalColor, 1.0);
    }
}
