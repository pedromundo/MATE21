#version 400
in vec2 aPosition;
in vec4 aColor;
out vec4 vColor;
uniform mat4 uMVP;

void main() {
	float glo_x = 2.0 * (aPosition.x - 1.0 / 2.0) / 1.0;
	float glo_y = -2.0 * (aPosition.y - 1.0 / 2.0) / 1.0;
    gl_Position = vec4(glo_x,glo_y,0.0,1.0);
    vColor = aColor;
}
