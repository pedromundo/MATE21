#version 400
in vec3 appPosition_modelspace;
in vec3 appNormal_modelspace;
in vec2 appTexCoord;
in vec3 appTangent_modelspace;
in vec3 appBinormal_modelspace;

out vec2 vertTexCoord;
out vec3 vertPosition_modelspace;
out vec3 vertNormal_cameraspace;
out vec3 vertLightDirection_tangentspace;
out vec3 vertEyeDirection_tangentspace;

uniform mat4 MVP;
uniform mat3 MV;
uniform mat3 M;
uniform vec3 lightPos;
uniform vec3 eyePos;

void main() {      
	vec3 Position_cameraspace = ( MV * appPosition_modelspace).xyz;
	vec3 vertEyeDirection_cameraspace;
	vec3 vertLightDirection_cameraspace;

	vertEyeDirection_cameraspace = normalize(eyePos - Position_cameraspace);

	// Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
	vec3 LightPosition_cameraspace = ( MV * lightPos).xyz;
	vertLightDirection_cameraspace = LightPosition_cameraspace + vertEyeDirection_cameraspace;

	// Normal of the the vertex, in camera space
	vertNormal_cameraspace = ( MV * appNormal_modelspace).xyz;

	vec3 vertexTangent_cameraspace = MV * normalize(appTangent_modelspace);
    vec3 vertexBinormal_cameraspace = MV * normalize(appBinormal_modelspace);

	mat3 TBN = transpose(mat3(
		vertexTangent_cameraspace,
		vertexBinormal_cameraspace,
		vertNormal_cameraspace
	));

	vertPosition_modelspace = appPosition_modelspace;
	vertLightDirection_tangentspace = TBN * vertLightDirection_cameraspace;
	vertEyeDirection_tangentspace =  TBN * vertEyeDirection_cameraspace;	
    vertTexCoord = vec2(appTexCoord.x,1-appTexCoord.y);
}