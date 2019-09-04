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

vec3 calculateIntegral(float a, float b, vec3 sigma_a){
    return (b-a) * sigma_a;
}

vec3 calculateAbsorption(vec3 radiance, float startT){
	//absorption values for each color channel (wavelength)
	vec3 sigma_a = vec3(0.19, 0.19, 0.19); 
	
	vec3 e = vec3(0, 0, 0);
	vec3 p = vec3(0, 0, 0);
	vec3 w = normalize(vec3(1, 0, 0));
	float stepSize = 0.001f;
	float d = 10.0;
	
	/*for(float t = startT; t <= d; t += stepSize){
		vec3 currentP = p + t*w;
		float volumeSample = 1.0;
	    e += calculateIntegral(t, t+stepSize, sigma_a);
	}*/

    vec3 integral = calculateIntegral(startT * d, d, sigma_a);
    e = exp(-integral);
	
	return e * radiance;
}


void main(){
	//color = renderNoiseSlices(vec3(texCoords.xy, 1.0));

    vec3 c = vec3(1,1,1);
    color = vec4(calculateAbsorption(c, gl_FragCoord.x/screenRes.x), 1.0);

}
