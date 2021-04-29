#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex1; //isampler FOR INTEGER!!
uniform sampler2D tex2; //isampler FOR INTEGER!!

/* 
p29 from
https://www.cg.tuwien.ac.at/research/publications/2018/glanz-2017-pbrcomparison/glanz-2017-pbrcomparison-thesis.pdf */

vec3 colorDistance(vec3 c1, vec3 c2){
	float r = abs(c1.r = c2.r);
	float g = abs(c1.g = c2.g);
	float b = abs(c1.b = c2.b);
	
	float dist = (r + g + b) / 3.0f;
	
	return vec3(dist, dist, dist);
}

vec3 RMSE(vec3 c1, vec3 c2){
	float r = c1.r - c2.r;
	r *= r;
	float g = c1.g - c2.g;
	g *= g;
	float b = c1.b - c2.b;
	b *= b;
	
	float dist = sqrt((r + g + b) / 3.0f);

	return vec3(dist, dist, dist);
}

//RGB to CIE L*C*h (Lch)
vec3 rgbToCIE(vec3 inputrgb) {
	float num = 0;
	vec3 rgb = vec3(0,0,0);

	for (int i = 0; i < 3; i++) {
		if (inputrgb[i] > 0.04045f)
			inputrgb[i] = pow(((inputrgb[i] + 0.055f) / 1.055f), 2.4f);
		else
			inputrgb[i] = inputrgb[i] / 12.92f;


		rgb[i] = inputrgb[i] * 100.0f;
	}

	float xyz[3] = { 0, 0, 0 };


	xyz[0] = rgb[0] * 0.4124f + rgb[1] * 0.3576f + rgb[2] * 0.1805f;
	xyz[1] = rgb[0] * 0.2126f + rgb[1] * 0.7152f + rgb[2] * 0.0722f;
	xyz[2] = rgb[0] * 0.0193f + rgb[1] * 0.1192f + rgb[2] * 0.9505f;


	// Observer= 2°, Illuminant= D65
	xyz[0] = xyz[0] / 95.047f;         // ref_X =  95.047
	xyz[1] = xyz[1] / 100.0f;          // ref_Y = 100.000
	xyz[2] = xyz[2] / 108.883f;        // ref_Z = 108.883


	for (int i = 0; i < 3; i++) {
		if (xyz[i] > 0.008856f)
			xyz[i] = pow(xyz[i], 0.3333333333333333);
		else
			xyz[i] = (7.787f * xyz[i]) + (16.0f / 116.0f);
	}

	vec3 lab;
	lab[0] = (116.0f * xyz[1]) - 16.0f;
	lab[1] = 500 * (xyz[0] - xyz[1]);
	lab[2] = 200 * (xyz[1] - xyz[2]);
	
	return lab;
}

//currently using distance metric deltaE CIE94
//could be using CIEDE2000(?)
float deltaE(vec3 labA, vec3 labB) {
	float deltaL = labA[0] - labB[0];
	float deltaA = labA[1] - labB[1];
	float deltaB = labA[2] - labB[2];
	float c1 = sqrt(labA[1] * labA[1] + labA[2] * labA[2]);
	float c2 = sqrt(labB[1] * labB[1] + labB[2] * labB[2]);
	float deltaC = c1 - c2;
	float deltaH = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
	deltaH = deltaH < 0 ? 0 : sqrt(deltaH);
	float sc = 1.0 + 0.045 * c1;
	float sh = 1.0 + 0.015 * c1;
	float deltaLKlsl = deltaL / (1.0);
	float deltaCkcsc = deltaC / (sc);
	float deltaHkhsh = deltaH / (sh);
	float i = deltaLKlsl * deltaLKlsl + deltaCkcsc * deltaCkcsc + deltaHkhsh * deltaHkhsh;
	return i < 0 ? 0 : sqrt(i);
}

vec3 CIELAB(vec3 c1, vec3 c2){
	vec3 lab1 = rgbToCIE(c1); //TODO multiply c1 by 255?
	vec3 lab2 = rgbToCIE(c2);
	
	float deltE = deltaE(lab1, lab2);
	
	return vec3(deltE, deltE, deltE)/100.0f; //TODO is that correct, what is the max range here?
}

vec3 overlap(vec3 i1, vec3 i2){
	return mix(i1, i2, 0.5);
}

void main(){
    vec4 texSample1 = texture(tex1, texCoords);
    vec4 texSample2 = texture(tex2, texCoords);
		
	color = vec4(RMSE(texSample1.xyz, texSample2.xyz), 1.0f);
}
