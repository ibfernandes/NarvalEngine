#version 430 core
#define PI 3.1415926535897932384626433832795

out vec4 color; 

uniform float time;
uniform vec2 screenRes;
float slices[] = {0.0, 0.25, 0.5f};
int spread = 1000;
//buckets must be less than spread.
int buckets = 10;

/*
	Returns a random number between [0,1)
    NOTE: fract() removes the signal. So -x becomes x.
*/
float seedSum = 0;
float randomUniform(vec2 uv) {
	vec2 seed = uv + fract(time) * 0.08f + (seedSum++);
	return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

/*
	Sample unit sphere
    highp 32 bits
    mediump 16 bits
    lowp 10 bits
*/
vec3 sampleSphere(vec2 uv) {
    float e1 = randomUniform(uv);
    float e2 = randomUniform(vec2(e1, uv.x));

	float theta = 2 * PI * e1;
	float phi = acos(1 - 2 * e2);
    vec3 res = vec3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));
	
    return res;
}

float selectRandom(vec2 uv, int selection){
    if(selection == 0)
        return  randomUniform(uv);
    else if (selection == 1)
        return sampleSphere(uv).x;
    return 0;
}

float remap (float value, vec2 interval, vec2 newInterval){
    return newInterval.x + (value - interval.x) * (newInterval.y - newInterval.x) / (interval.y - interval.x);
}

//NOTE: Not quite accurate...
float histogram(vec2 uv, vec2 interval, int selection){
    float count = 0.0f;
    float bucketIndex = remap(uv.x, interval, vec2(0.0f, buckets));

    for(int i = 0; i < spread; i++){
        float rand = selectRandom(vec2(uv.x, 0.25f) , selection);

        if(int(rand * buckets) == int(bucketIndex))
            count++;
    }

    if(remap(uv.y, vec2(0, 0.25f), vec2(0, 1.0f)) < (count/spread) * 0.1f / (1.0f/buckets))
        return 1;
    else
        return 0;
}

void main(){
    float noiseValue;
    float finalColor = 0;
    vec2 uv = gl_FragCoord.xy/screenRes.xy;

    for(int i = 0; i < slices.length() - 1; i++){
        if(uv.x > slices[i] && uv.x <= slices[i+1]){
            finalColor = selectRandom(uv.xy, i);
            if(uv.y < 0.25f)
                finalColor = histogram(uv, vec2(slices[i], slices[i+1]), i);
        }
    }
    
    if(abs(uv.x - 0.25f) < 0.001f)
        finalColor = 0; 

    color = vec4(finalColor, finalColor, finalColor, 1.0f);
}
