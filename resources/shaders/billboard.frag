#version 430 core
#define MAX_LIGHTS 32

out vec4 color; 

in vec2 texCoord;

struct Material{
    sampler2D diffuse;
};
uniform Material material;

uniform vec3 cameraPosition;

void main(){
	color = texture(material.diffuse, texCoord);
    if(color.a == 0.0f)
        gl_FragDepth = 1.0;
}
