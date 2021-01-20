#version 430 core

out vec4 color; 
out float gl_FragDepth;
in vec2 texCoords;

uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D depth1;
uniform sampler2D depth2;

void main(){
    vec4 texSample1 = texture(tex1, texCoords);
    vec4 texSample2 = texture(tex2, texCoords);
	float depth1 = texture(depth1, texCoords).r;
	float depth2 = texture(depth2, texCoords).r;
	
	if(depth1 < depth2){
		color = texSample1;
		gl_FragDepth = depth1;
	}else{
		color = texSample2;
		gl_FragDepth = depth2;
	}
}
