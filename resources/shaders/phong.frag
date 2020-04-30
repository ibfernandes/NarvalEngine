#version 430 core
#define MAX_LIGHTS 32

out vec4 color; 

in VERTOUT{
    vec2 texCoord;
    vec3 normalCoord;
    vec3 vertexCoord;
    vec4 fragPosLightSpace;
    mat3 TBN;
} fragIn;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
};

struct LightPoint {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Material material;
uniform LightPoint lightPoints[MAX_LIGHTS];
uniform int numberOfLights = 0;
uniform vec3 cameraPosition;
uniform int viewMode;
uniform sampler2D shadowMap;
uniform int normalMapping;

float calculateShadow(vec3 normal, vec3 lightDir){
    //divide by w to get NDC [-1,1]
    vec3 projCoords = fragIn.fragPosLightSpace.xyz / fragIn.fragPosLightSpace.w;
    //and now transform NDC to [0,1]
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float bias;
    if(normalMapping==1)
        bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    else
        bias = 0.005;

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

vec3 calculateLightPoint(LightPoint light, vec3 fragPos, vec3 viewDir){
    vec3 normal;
    if(normalMapping==1){
        normal = texture(material.normal, fragIn.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(fragIn.TBN * normal);
    }
    if(normal.x ==0 && normal.y ==0 && normal.z ==0)
        normal = fragIn.normalCoord;

    vec4 diffuseTexSample = texture(material.diffuse, fragIn.texCoord);
    if(diffuseTexSample.a == 0)
        discard; 

    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    //phong
    vec3 reflectDir = reflect(-lightDir, normal);
    //blinn-phong
    reflectDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
    
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    
    vec3 ambient  = light.ambient  * vec3(diffuseTexSample);
    vec3 diffuse  = light.diffuse  * diff * vec3(diffuseTexSample);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fragIn.texCoord));

    float shadow = calculateShadow(normal, lightDir);
    return (ambient + (1.0 - shadow)*(diffuse + specular));
}

void main(){

    switch(viewMode){
        case 0:
            for(int i=0; i < numberOfLights; i++)
                color += vec4(calculateLightPoint(lightPoints[i], fragIn.vertexCoord, normalize(cameraPosition - fragIn.vertexCoord)), 1);
            break;
        case 1:
            color = texture(material.diffuse, fragIn.texCoord);
            break;
        case 2:
            color = texture(material.normal, fragIn.texCoord);
            break;
    }
}
