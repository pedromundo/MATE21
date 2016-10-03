#version 400
layout(vertices = 3) out;

in vec2 vertTexCoord[];
in vec3 vertPosition_modelspace[];
in vec3 vertEyeDirection_cameraspace[];
in vec3 vertLightDirection_cameraspace[];
in vec3 vertNormal_cameraspace[];
in vec3 vertLightDirection_tangentspace[];
in vec3 vertEyeDirection_tangentspace[];

out vec2 tcTexCoord[];
out vec3 tcPosition_modelspace[];
out vec3 tcEyeDirection_cameraspace[];
out vec3 tcLightDirection_cameraspace[];
out vec3 tcNormal_cameraspace[];
out vec3 tcLightDirection_tangentspace[];
out vec3 tcEyeDirection_tangentspace[];

void main(){

    float inTess  = 30;
    float outTess = 30;

	tcTexCoord[gl_InvocationID] = vertTexCoord[gl_InvocationID];                        
	tcPosition_modelspace[gl_InvocationID] = vertPosition_modelspace[gl_InvocationID];
	tcEyeDirection_cameraspace[gl_InvocationID] = vertEyeDirection_cameraspace[gl_InvocationID];
	tcLightDirection_cameraspace[gl_InvocationID] = vertLightDirection_cameraspace[gl_InvocationID];      
	tcNormal_cameraspace[gl_InvocationID] = vertNormal_cameraspace[gl_InvocationID];              
	tcLightDirection_tangentspace[gl_InvocationID] = vertLightDirection_tangentspace[gl_InvocationID];     
	tcEyeDirection_tangentspace[gl_InvocationID] = vertEyeDirection_tangentspace[gl_InvocationID];           
    if(gl_InvocationID == 0) {
        gl_TessLevelInner[0] = inTess;
        gl_TessLevelInner[1] = inTess;
        gl_TessLevelOuter[0] = outTess;
        gl_TessLevelOuter[1] = outTess;
        gl_TessLevelOuter[2] = outTess;
        gl_TessLevelOuter[3] = outTess;
    }
}