#version 330 core

in vec3 texCoords;
out vec4 color;

uniform sampler3D perlinWorley3;
uniform sampler3D worley3;
uniform float time;
uniform float screenRes;

void main(){
    vec3 newTexCoords = vec3(texCoords.x,texCoords.y, time);
    float noiseValue = 0;

    if(gl_FragCoord.x > 0 && gl_FragCoord.x <= screenRes*0.25 )
        noiseValue = texture(perlinWorley3, newTexCoords).x;
    if(gl_FragCoord.x > screenRes*0.25 && gl_FragCoord.x <= screenRes*0.5 )
        noiseValue = texture(perlinWorley3, newTexCoords).y;
    if(gl_FragCoord.x > screenRes*0.5 && gl_FragCoord.x <= screenRes*0.75 )
        noiseValue = texture(perlinWorley3, newTexCoords).z;
    if(gl_FragCoord.x > screenRes*0.75 && gl_FragCoord.x <= screenRes )
        noiseValue = texture(perlinWorley3, newTexCoords).a;

    vec4 c = texture(perlinWorley3, texCoords);
    c = vec4(noiseValue,noiseValue,noiseValue,noiseValue);

    color = c;
}
