#version 400
in vec3 aPosition;
in vec4 aColor;
in vec2 vertTexCoord;
out vec4 vColor;
out vec2 fragTexCoord;
uniform mat4 MVP;

void main() {    
    vColor = aColor;
    fragTexCoord = vertTexCoord;
	gl_Position = MVP * vec4(aPosition,1);
}