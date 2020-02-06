#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;

void main(){
    gl_Position = proj * cam * model * vec4(vertex, 1.0);
}
