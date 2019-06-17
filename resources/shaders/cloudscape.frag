#version 330 core
#define HIGH_FREQUENCY_MODE true

in vec3 texCoords;
out vec4 color;

uniform sampler3D perlinWorley3;
uniform sampler3D worley3;
uniform sampler2D curl3;
uniform sampler2D weather;
uniform float time;
uniform float screenRes;

//Fractional value for sample position in the cloud layer
float getHeightFractionForPoint(vec3 inPosition, vec2 inCloudMinMax){
    //Get global fractional position in cloud zone
    float heightFraction = (inPosition.z - inCloudMinMax.x) / (inCloudMinMax.y - inCloudMinMax.x);
    return clamp(heightFraction, 0.0, 1.0);
}

float changeRange( float x, float xmin, float xmax, float a, float b) {
    return (b - a) * (x - xmin) / (xmax - xmin) + a;
}

//TODO: wtf is this f supposed to do?
//WeatherData R: cloud coverage
//            G: precipitation
//            B: cloud type (0 stratus, 0.5 stratocumulus, 1.0 cumulus)
float getDensityHeightGradientForPoint(vec3 p, vec3 weatherData){
    return 1;
}

float sampleCloudDensity(vec3 p, vec3 weatherData){
    vec2 inCloudMinMax = vec2(0.0, 1.0);

    float heightFraction = getHeightFractionForPoint(p, inCloudMinMax);

    vec4 lowFrequencyNoises = texture(perlinWorley3, p).xyzw;
    float lowFrequencyFBM = (lowFrequencyNoises.y * 0.625)
                            + (lowFrequencyNoises.z * 0.25)
                            + (lowFrequencyNoises.w * 0.125);
    
    //define the base cloud shapre by dilating it with the low-frequency 
    //FBM made of Worley noise
    float baseCloud = changeRange(lowFrequencyNoises.r, - (1.0 - lowFrequencyFBM), 1.0, 0.0, 1.0);

    //get the density-height gradient using the desnity height function
    //i.e heights.png
    float densityHeightGradient = getDensityHeightGradientForPoint(p, weatherData); 

    //Apply the height function to the base cloud shape
    baseCloud *= densityHeightGradient;

    float cloudCoverage = weatherData.r;

    float baseCloudWithCoverage = changeRange(baseCloud, cloudCoverage, 1.0, 0.0, 1.0);

    baseCloudWithCoverage *= cloudCoverage;

    //add some turbulence to bottom of clouds.
    p.xy += texture(curl3, p.xy).xy * (1.0 - heightFraction);

    vec3 highFreqNoises = texture(perlinWorley3, p*0.1).xyz;
    float highFreqFBM = ( highFreqNoises.x * 0.625)
                        + ( highFreqNoises.y * 0.25 )
                        + ( highFreqNoises.z * 0.125);

    //transition from wispy shapes to billowy shapes over height
    float highFreqNoiseModifier = mix(highFreqFBM, 1.0 - highFreqFBM, clamp(heightFraction * 10.0, 0.0, 1.0));

    //erode the base cloud shape with the distorted high freq worley noise
    float finalCloud = changeRange(baseCloudWithCoverage, highFreqNoiseModifier*0.2,1.0, 0.0, 1.0);

    return finalCloud;   
}

vec3 calculateWind(vec3 p, float heightFraction){
    vec3 windDirection = vec3(1.0, 0.0, 0.0);
    float cloudSpeed = 10.0;
    float cloudTopOffset = 500.0;

    //skew in wind direction
    p += heightFraction * windDirection * cloudTopOffset;
    //animate clouds in wind direction and add a small upward bias
    p+= (windDirection + vec3(0.0, 0.1, 0.0)) * time * cloudSpeed;

    return p;
}

void main(){
    //textureLod
    //sampleCloudDensity(calculateWind(p), weatherTexSample)

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

    noiseValue = sampleCloudDensity(newTexCoords.xyz, texture(weather, newTexCoords.xy).xyz);

    color = vec4(noiseValue,noiseValue,noiseValue,noiseValue);
}
