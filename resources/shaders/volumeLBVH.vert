#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec3 rayDirection;
out vec3 vertCoord;
flat out vec3 eyeObjectSpace;
flat out vec3 translation;
flat out vec3 scale;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform vec3 cameraPosition;

void main(){
    vertCoord = vertex;
    vec3 posWCS = vec3(model * vec4(vertex, 1.0));
    rayDirection = posWCS - cameraPosition;
    gl_Position = proj * cam * model * vec4(vertex, 1.0);
}
