#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

void main(){
    gl_Position = proj * view * model * vec4(vertex, 1.0);
}
