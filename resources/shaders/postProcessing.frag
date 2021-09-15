#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex;
uniform int mode = 0;
uniform float gamma = 2.2;
uniform float exposure = 0.5;

vec3 exposureToneMapping(vec3 hdrColor, float exposure) {
	return vec3(1.0) - exp(-hdrColor * exposure);
}

vec3 gammaCorrection(vec3 hdrColor) {
	return pow(hdrColor, vec3(1.0f / gamma));
}

vec3 postProcessing(vec3 hdrColor) {
	vec3 mapped = exposureToneMapping(hdrColor, exposure);
	mapped = gammaCorrection(mapped);

	return mapped;
}

void main(){
    vec4 texSample = texture(tex, texCoords);
   
   color = vec4(postProcessing(texSample.xyz).xyz, 1);
}
