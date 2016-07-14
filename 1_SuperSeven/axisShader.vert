attribute vec3 aPosition;
attribute vec4 aColor;

varying vec4 vColor;

void main() {        
    gl_Position = ftransform();
    vColor = aColor;
}
