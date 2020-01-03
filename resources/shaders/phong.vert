#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex;

out vec2 texCoord;
out vec3 normalCoord;
out vec3 vertexCoord;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform vec3 cameraPosition;

void main(){
    texCoord = tex;
    normalCoord = normal;
    vertexCoord = vec3(model * vec4(vertex, 1.0));
    gl_Position = proj * cam * model * vec4(vertex, 1.0);
}
