#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 tex;


out vec4 fragColor;

uniform mat4 cam;
uniform mat4 model;
uniform mat4 proj;

void main(){
	gl_Position = proj * cam * model * vec4(vertex.xyz , 1.0);
}
