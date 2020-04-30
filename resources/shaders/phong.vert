#version 430 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 tex;
layout (location = 3) in vec3 tangent;

out VERTOUT{
    vec2 texCoord;
    vec3 normalCoord;
    vec3 vertexCoord;
    vec4 fragPosLightSpace;
    mat3 TBN;
} vertOut;

uniform mat4 model;
uniform mat4 cam;
uniform mat4 proj;
uniform vec3 cameraPosition;

uniform mat4 lightProj;
uniform mat4 lightCam;

void main(){
    vertOut.texCoord = tex;
    vertOut.normalCoord = mat3(transpose(inverse(model))) * normal;
    vertOut.vertexCoord = vec3(model * vec4(vertex, 1.0));
    vertOut.fragPosLightSpace = lightProj * lightCam * vec4(vertOut.vertexCoord, 1.0);

    vec3 T = normalize(vec3(model * vec4(tangent, 0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0)));
    vec3 B = cross(N, T);
    vertOut.TBN = mat3(T, B, N);

    gl_Position = proj * cam * model * vec4(vertex, 1.0);
}
