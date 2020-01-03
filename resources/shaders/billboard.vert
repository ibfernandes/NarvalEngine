#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec2 texCoord;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;

void main(){
    texCoord = tex;

    vec3 camRight = vec3(cam[0][0], cam[1][0], cam[2][0]);
    vec3 camUp = vec3(cam[0][1], cam[1][1], cam[2][1]);

    vec3 position = vec3(model[3][0], model[3][1], model[3][2]);
    vec2 size = vec2(model[0][0], model[1][1]);

    vec3 vPos = position + camRight * vertex.x * size.x + camUp * vertex.y * size.y;

    gl_Position = proj * cam * vec4(vPos, 1.0f);
}
