#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex1; //isampler FOR INTEGER!!
uniform sampler2D tex2; //isampler FOR INTEGER!!

void main(){
    vec4 texSample1 = texture(tex1, texCoords);
    vec4 texSample2 = texture(tex2, texCoords);
	
	vec4 res = abs(texSample1 - texSample2);
	
	color = vec4(res.xyz, 1.0f);
}
