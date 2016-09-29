#version 400
in vec2 fragTexCoord;
uniform sampler2D tex; //this is the texture

void main() {	
	gl_FragColor = texture(tex,fragTexCoord);    	
}