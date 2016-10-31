#version 400
in vec3 appPosition_modelspace;
in vec3 appNormal_modelspace;

out vec4 vertColor;

uniform mat4 MVP;

void main() {      
	 gl_Position = (MVP * vec4(appPosition_modelspace,1.0)) + (MVP * vec4(appNormal_modelspace,1.0)) * 0.01;
	 vertColor = vec4(0,0,0,1.0);
	 //vertColor = vec4(0.055,0.8,1.0,1.0);
}