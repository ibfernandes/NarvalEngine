#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex;
uniform float totalSamples = 1;

void main(){
    color = vec4(texture(tex, texCoords).rgb, 1);
}
