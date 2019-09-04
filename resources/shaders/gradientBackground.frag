#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler3D volume;
uniform float time;
uniform vec2 screenRes = vec2(1024, 512);

void main(){
        //color = renderNoiseSlices(vec3(texCoords.xy, 1.0));

        vec4 up = vec4( 236/255.0f , 240/255.0f, 241/255.0f,1);
        vec4 down = vec4( 186/255.0f , 199/255.0f, 200/255.0f,1);
        color = vec4(mix(down, up, gl_FragCoord.y/screenRes.y));

}
