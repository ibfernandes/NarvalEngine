#version 430 core
#define MAX_LIGHTS 32

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
    sampler2D specular;
    sampler2D normal;
    sampler2D roughness;
    sampler2D ao;
};

struct MaterialIsSet{
    bool diffuse;
    bool metallic;
    bool specular;
    bool normal;
    bool roughness;
    bool ao;
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

const float PI = 3.14159265359;

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

/**
* halfway is defined as the microfacet's normal.
*/
float distributionGGX(vec3 normal, vec3 halfway, float roughness){
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(normal, halfway), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}


float geometrySchlickGGX(float NdotV, float roughness){
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}


/**  
*   toCam    N    toLight    
*    ^       ^       ^
*     \      |      /
*      \     |     /
*       \    |    /
*/

float geometrySmith(vec3 normal, vec3 toCam, vec3 toLight, float roughness){
    float NdotV = max(dot(normal, toCam), 0.0);
    float NdotL = max(dot(normal, toLight), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0){
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

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
	vec3 toCam      = normalize(cameraPosition - fragIn.worldCoord);
	vec3 albedo     = pow(materialIsSet.diffuse ? texture(material.diffuse, fragIn.texCoord).rgb : vec3(1,1,1), vec3(2.2));
    float metallic  = materialIsSet.metallic ? texture(material.metallic, fragIn.texCoord).r : 0.0f;
    float roughness =  materialIsSet.roughness ? texture(material.roughness, fragIn.texCoord).r : 0.0f;
    float ao        = materialIsSet.ao ? texture(material.ao, fragIn.texCoord).r : 1.0f;

    vec3 normal;
    //If the normal texture is set, use it. If not, use vertex normals.
    if(materialIsSet.normal)
        normal = getNormalFromMap();
    else
        normal = fragIn.normalCoord;

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);

	// calculate per-light radiance
	vec3 toLight = normalize(light.position - fragIn.worldCoord);
	vec3 halfway = normalize(toCam + toLight);
	float distance = length(light.position - fragIn.worldCoord);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = (light.color/100) * attenuation;

	// Cook-Torrance BRDF
	float NDF = distributionGGX(normal, halfway, roughness);   
	float G   = geometrySmith(normal, toCam, toLight, roughness);      
	vec3 F    = fresnelSchlick(max(dot(halfway, toLight), 0.0), F0);
	   
	vec3 numerator = NDF * G * F; 
	float denominator = 4 * max(dot(normal, toCam), 0.0) * max(dot(normal, toLight), 0.0) + 0.001; // 0.001 to prevent divide by zero.
	vec3 specular = numerator / denominator;
	
	// kS is equal to Fresnel
	vec3 kS = F;
	// for energy conservation, the diffuse and specular light can't
	// be above 1.0 (unless the surface emits light); to preserve this
	// relationship the diffuse component (kD) should equal 1.0 - kS.
	vec3 kD = vec3(1.0) - kS;
	// multiply kD by the inverse metalness such that only non-metals 
	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// have no diffuse light).
	kD *= 1.0 - metallic;	  

	// scale light by NdotL
	float NdotL = max(dot(normal, toLight), 0.0);

	// add to outgoing radiance Lo
	Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	
    //return Lo;
    float shadow = calculateShadow(normal, toLight);
    //return (1.0) * Lo;
	return (1.0 - shadow) * Lo;
}

void main(){
	vec3 Lo = vec3(0.0);
	
	for(int i=0; i < numberOfLights; i++)
		Lo += calculateLightPoint(lightPoints[i]);
    
    vec3 finalColor = Lo;
    color = vec4(finalColor, 1.0);
}
