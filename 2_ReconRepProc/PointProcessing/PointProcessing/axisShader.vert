#version 400
in vec3 aPosition;
in vec4 aColor;
out vec4 vColor;
uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPosition,1.0);
    vColor = aColor;
}