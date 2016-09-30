#version 400
in vec2 fragTexCoord;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec3 Normal_cameraspace;
in vec3 Position_worldspace;
in vec3 LightDirection_tangentspace;
in vec3 EyeDirection_tangentspace;
//this is the texture
uniform sampler2D tex; 
uniform sampler2D nor;


void main() {
	int LightPower = 100;
	int Distance = 10;
	vec4 LightColor = vec4(1.0,1.0,1.0,1.0);

    vec3 TextureNormal_tangentspace = normalize(texture( nor, fragTexCoord ).rgb*2.0 - 1.0);

	vec3 n = normalize( TextureNormal_tangentspace );	 
	vec3 l = normalize( LightDirection_tangentspace );
	float cosTheta = clamp( dot( n,l ), 0,1 );

	vec3 E = -normalize(EyeDirection_tangentspace);
	vec3 R = reflect(-l,n);
	float cosAlpha = clamp( dot( E,R ), 0,1 );

	vec4 MaterialDiffuseColor = texture(tex,fragTexCoord);
	vec4 MaterialAmbientColor = vec4(0.1,0.1,0.1,1.0) * MaterialDiffuseColor;

    vec4 color = MaterialAmbientColor +  
	MaterialDiffuseColor * vec4(1.0,1.0,1.0,1.0) * LightPower * cosTheta / (Distance*Distance) +
    vec4(1.0,1.0,1.0,1.0) * vec4(1.0,1.0,1.0,1.0) * LightPower * pow(cosAlpha,1000) / (Distance*Distance);


    gl_FragColor = vec4(color.rgb,1);
}