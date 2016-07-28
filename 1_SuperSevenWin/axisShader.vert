#version 400
in vec2 aPosition;
in vec3 aColor;
out vec4 vColor;
uniform mat4 uMVP;
uniform vec4 aChromaKeyingBase;
uniform vec4 aChromaKeyingDest;

void main() {
    float glo_x = 2.0f * (aPosition.x - 1.0f / 2.0f) / 1.0f;
    float glo_y = -2.0f * (aPosition.y - 1.0f / 2.0f) / 1.0f;
    gl_Position = vec4(glo_x,glo_y,0.0f,1.0f);    
    vColor = vec4(aColor/255,1.0f);
}
