#version 400
in vec4 vColor;
uniform vec4 aChromaKeyingBase;
uniform vec4 aChromaKeyingDest;

void main() {
    vec4 colorDiff = vColor - aChromaKeyingBase;
    gl_FragColor = vColor;
	if(length(colorDiff) < 0.6f){
        gl_FragColor = aChromaKeyingDest;
    }
}