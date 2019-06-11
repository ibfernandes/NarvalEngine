#version 330 core

in vec3 fragColor;
out vec4 color;

void main(){
	color = vec4(fragColor.xyz, 1);
}
