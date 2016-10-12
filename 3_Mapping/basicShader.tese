#version 400

layout(triangles, equal_spacing, ccw) in;

in vec2 tcTexCoord[];
in vec3 tcPosition_modelspace[];
/*in vec3 tcEyeDirection_cameraspace[];
in vec3 tcLightDirection_cameraspace[];*/
in vec3 tcNormal_cameraspace[];
in vec3 tcLightDirection_tangentspace[];
in vec3 tcEyeDirection_tangentspace[];

out vec2 teTexCoord;
out vec3 tePosition_cameraspace;
/*out vec3 teEyeDirection_cameraspace;
out vec3 teLightDirection_cameraspace;*/
out vec3 teNormal_cameraspace;
out vec3 teLightDirection_tangentspace;
out vec3 teEyeDirection_tangentspace;

uniform sampler2D hei;
uniform mat4 MVP;
uniform mat3 MV;
uniform float displacementStrength;

void main(){
	vec2 tc0 = gl_TessCoord.x * tcTexCoord[0];
    vec2 tc1 = gl_TessCoord.y * tcTexCoord[1];
    vec2 tc2 = gl_TessCoord.z * tcTexCoord[2];  
    teTexCoord = tc0 + tc1 + tc2;

    vec3 p0 = gl_TessCoord.x * tcPosition_modelspace[0];
    vec3 p1 = gl_TessCoord.y * tcPosition_modelspace[1];
    vec3 p2 = gl_TessCoord.z * tcPosition_modelspace[2];
    vec3 pos = p0 + p1 + p2;	

    vec3 n0 = gl_TessCoord.x * tcNormal_cameraspace[0];
    vec3 n1 = gl_TessCoord.y * tcNormal_cameraspace[1];
    vec3 n2 = gl_TessCoord.z * tcNormal_cameraspace[2];
    vec3 normal = normalize(n0 + n1 + n2);    

	vec3 lt0 = gl_TessCoord.x * tcLightDirection_tangentspace[0];
    vec3 lt1 = gl_TessCoord.y * tcLightDirection_tangentspace[1];
    vec3 lt2 = gl_TessCoord.z * tcLightDirection_tangentspace[2];
    vec3 lighttan = normalize(lt0 + lt1 + lt2);

	vec3 et0 = gl_TessCoord.x * tcEyeDirection_tangentspace[0];
    vec3 et1 = gl_TessCoord.y * tcEyeDirection_tangentspace[1];
    vec3 et2 = gl_TessCoord.z * tcEyeDirection_tangentspace[2];
    vec3 eyetan = normalize(et0 + et1 + et2);

    float height = texture(hei, teTexCoord).x;
    pos += normal * (height * displacementStrength);

    gl_Position = MVP * vec4(pos, 1);    
    tePosition_cameraspace = MV * pos;		
	teNormal_cameraspace = normal;
	teLightDirection_tangentspace = lighttan;
	teEyeDirection_tangentspace = eyetan;
}