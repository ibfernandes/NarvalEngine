#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec2 tex;

out vec2 texCoords;
out vec3 fragPos;
flat out vec3 transformed_eye ;
flat out vec3 transformed_sunLight ;
out vec3 vray_dir ;

uniform mat4 cam;
uniform mat4 staticCam;
uniform mat4 model;
uniform mat4 projection;
uniform vec3 cameraPosition;
uniform vec3 sunLightPosition = vec3(0,150,150);
uniform float time;

void main(){
	mat4 mvp = projection * staticCam * model;

	gl_Position = mvp * vec4(vertex.xyz , 1.0);

	fragPos = vec3(model * vec4(vertex.xyz, 1.0));

	transformed_eye = (cameraPosition - vec3(0,0,5)) / vec3(1,1,1);
	transformed_sunLight = (vec3(sunLightPosition.x, time*sunLightPosition.y, sunLightPosition.z) - vec3(0,0,0)) / vec3(1,1,1);
	vray_dir = vertex - transformed_eye;

	texCoords = tex.xy;
}
