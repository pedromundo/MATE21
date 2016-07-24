#version 400
in vec3 aPosition;
in vec4 aColor;

out vec4 vColor;

uniform int uAngle = 0;
uniform mat4 uMVP;
#define M_PI 3.14159265359

float rotation = uAngle * (M_PI/180);
mat4 RotationMatrix = mat4( cos(rotation), -sin(rotation), 0.0, 0.0,
                            sin(rotation),  cos(rotation), 0.0, 0.0,
                                0.0,       0.0,  1.0, 0.0,
                                0.0,       0.0,  0.0, 1.0);

void main() {
    if((aPosition.x <= -1.0 || aPosition.x >= 1.0 || aPosition.y <= -1.0 || aPosition.y >= 1.0 )){
        gl_Position = uMVP * vec4(aPosition,1);
    }else{
        gl_Position = RotationMatrix * uMVP * vec4(aPosition,1);
    }
    vColor = aColor;
}