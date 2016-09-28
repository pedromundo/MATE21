#version 400
in vec4 vColor;
in vec2 fragTexCoord;
uniform sampler2D tex; //this is the texture

void main() {	
	gl_FragColor = (0.001*vColor) + texture(tex,fragTexCoord);    
}