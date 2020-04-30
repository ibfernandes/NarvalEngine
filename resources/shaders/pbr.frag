#version 430 core
#define MAX_LIGHTS 32

out vec4 color; 

in vec2 texCoord;
in vec3 normalCoord;
in vec3 vertexCoord;

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

vec3 calculateLightPoint(LightPoint light, vec3 normal, vec3 fragPos, vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1.0);
    
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + 
  			     light.quadratic * (distance * distance));    
    
    vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, texCoord));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, texCoord));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, texCoord));

    return attenuation * (ambient + diffuse + specular);
}

void main(){
    for(int i=0; i < numberOfLights; i++)
        color += vec4(calculateLightPoint(lightPoints[i], normalCoord, vertexCoord, normalize(cameraPosition - vertexCoord)), 1);
    
	//color = texture(diffuse, texCoord);
}
