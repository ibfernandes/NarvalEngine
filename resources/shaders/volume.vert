#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec3 rayDirection;
flat out vec3 eyeObjectSpace;
flat out vec3 translation;
flat out vec3 scale;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform vec3 cameraPosition;

void main(){
    scale.x = model[0][0];
    scale.y = model[1][1];
    scale.z = model[2][2];

    translation.x = model[3][0];
    translation.y = model[3][1];
    translation.z = model[3][2];

    gl_Position = proj * cam * model * vec4(vertex, 1.0);
    eyeObjectSpace = (cameraPosition - translation)/scale;
    rayDirection = vertex - eyeObjectSpace;
}
