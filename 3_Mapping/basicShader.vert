#version 400
in vec3 Position_modelspace;
in vec3 vertNormal_modelspace;
in vec2 vertTexCoord;
in vec3 vertTangent_modelspace;
in vec3 vertBinormal_modelspace;
out vec2 fragTexCoord;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;
out vec3 Normal_cameraspace;
out vec3 Position_worldspace;
out vec3 LightDirection_tangentspace;
out vec3 EyeDirection_tangentspace;
uniform mat4 MVP;
uniform mat3 MV;
uniform mat3 M;
uniform vec3 lightPos;

void main() {      
	gl_Position = MVP * vec4(Position_modelspace,1);
	Position_worldspace = (M * Position_modelspace).xyz;

	vec3 Position_cameraspace = ( MV * Position_modelspace).xyz;
	EyeDirection_cameraspace = vec3(0,0,0) - Position_cameraspace;

	// Vector that goes from the vertex to the light, in camera space. M is ommited because it's identity.
	vec3 LightPosition_cameraspace = ( MV * lightPos).xyz;
	LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;

	// Normal of the the vertex, in camera space
	Normal_cameraspace = ( MV * vertNormal_modelspace).xyz;

	vec3 vertexTangent_cameraspace = MV * normalize(vertTangent_modelspace);
    vec3 vertexBinormal_cameraspace = MV * normalize(vertBinormal_modelspace);

	mat3 TBN = transpose(mat3(
		vertexTangent_cameraspace,
		vertexBinormal_cameraspace,
		Normal_cameraspace
	));

	LightDirection_tangentspace = TBN * LightDirection_cameraspace;
	EyeDirection_tangentspace =  TBN * EyeDirection_cameraspace;
	
    fragTexCoord = vec2(vertTexCoord.x,1-vertTexCoord.y);
}