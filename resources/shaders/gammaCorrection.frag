#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex;
uniform float gamma = 2.2;
uniform float exposure = 1.0;


void main(){
    vec4 sp = texture(tex, texCoords);
    float invGamma = 1.0/gamma;

    sp = vec4(1.0) - exp(-sp * exposure);

    color = pow(sp, vec4(invGamma, invGamma, invGamma, 1));
}
