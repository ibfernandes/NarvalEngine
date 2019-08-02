#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler3D volume;
uniform float time;
uniform vec2 screenRes = vec2(1024, 512);

vec4 renderNoiseSlices(vec3 texCoords){
    float noiseValue;

    if(texCoords.x > 0 && texCoords.x <= 0.25 )
        noiseValue = texture(volume, texCoords).x;
    if(texCoords.x > 0.25 && texCoords.x <= 0.5 )
        noiseValue = texture(volume, texCoords).y;
    if(texCoords.x > 0.5 && texCoords.x <= 0.75 )
        noiseValue = texture(volume, texCoords).z;
    if(texCoords.x > 0.75 && texCoords.x <= 1.0 )
        noiseValue = texture(volume, texCoords).w;

    return vec4(noiseValue,noiseValue,noiseValue,1.0);
}

void main(){
	color = renderNoiseSlices(vec3(texCoords.xy, 1.0));
}
