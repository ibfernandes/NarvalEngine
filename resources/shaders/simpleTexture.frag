#version 430 core

out vec4 color; 
in vec2 texCoords;

uniform sampler2D tex; //isampler FOR INTEGER!!
uniform int mode = 0;

void main(){
    vec4 texSample = texture(tex, texCoords);
    switch(mode){
        //Simply output texture
        case 0:
		//color = vec4(texCoords.xy,0,1); break;
			color = vec4(texSample.xyz, 1); break;
        //Output red channel only and opacity as 1
        case 1:
            color = vec4(texSample.r, texSample.r, texSample.r, 1); break;
        // Depth map with transparency
        case 4:
            if(texSample.r == 1)
                discard;
            color = vec4(texSample.r, texSample.r, texSample.r, 1); 
            break;
        //TODO: depth map with linear depth function
    }
	
	//color = vec4(1,0,0,1);
}
