#version 400
in vec3 aPosition;
in vec2 vertTexCoord;
out vec2 fragTexCoord;
uniform mat4 MVP;
uniform mat3 MV;

void main() {        
    fragTexCoord = vec2(vertTexCoord.x,1-vertTexCoord.y);
	gl_Position = MVP * vec4(aPosition,1);
}