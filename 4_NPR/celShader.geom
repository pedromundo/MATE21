#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec2 teTexCoord[3];
in vec3 teLightDirection_tangentspace[3];
in vec3 teEyeDirection_tangentspace[3];

out vec2 gTexCoord;
out vec3 gLightDirection_tangentspace;
out vec3 gEyeDirection_tangentspace;

void main() {

	gTexCoord = teTexCoord[0];	
	gLightDirection_tangentspace = teLightDirection_tangentspace[0];
	gEyeDirection_tangentspace = teEyeDirection_tangentspace[0];       
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gTexCoord = teTexCoord[1];	  
	gLightDirection_tangentspace = teLightDirection_tangentspace[1];
	gEyeDirection_tangentspace = teEyeDirection_tangentspace[1];       
    gl_Position = gl_in[1].gl_Position; EmitVertex();

	gTexCoord = teTexCoord[2];	      
	gLightDirection_tangentspace = teLightDirection_tangentspace[2];
	gEyeDirection_tangentspace = teEyeDirection_tangentspace[2];       
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}