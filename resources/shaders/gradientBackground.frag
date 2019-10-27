#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler3D volume;
uniform float time;
uniform vec2 screenRes = vec2(1024, 512);
uniform vec3 gradientUp = vec3(0.0, 0.0, 0.0);
uniform vec3 gradientDown = vec3(0.0, 0.0, 0.0);

void main(){
        //color = renderNoiseSlices(vec3(texCoords.xy, 1.0));

        vec4 up = vec4( gradientUp,1);
        vec4 down = vec4( gradientDown,1);
        color = vec4(mix(down, up, gl_FragCoord.y/screenRes.y));

}
