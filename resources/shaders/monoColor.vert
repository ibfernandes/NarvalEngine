#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec3 fragColor;

uniform mat4 cam;
uniform mat4 model;
uniform mat4 projection;
uniform vec3 rgbColor;

void main(){
	gl_Position = projection * cam * model *vec4(vertex.xyz , 1.0);
	fragColor = rgbColor;
}
