#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec3 rayDirection;
flat out vec3 eyeObjectSpace;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform vec3 cameraPosition;

void main(){
    gl_Position = proj * cam * model * vec4(vertex, 1.0);

    eyeObjectSpace = (cameraPosition - vec3(0,0,4))/vec3(1,1,1);
    rayDirection = vertex - eyeObjectSpace;
}
