#version 400
in vec2 gTexCoord;
in vec3 gPosition_cameraspace;
in vec3 gEyeDirection_cameraspace;
in vec3 gLightDirection_cameraspace;
in vec3 gNormal_cameraspace;
in vec3 gLightDirection_tangentspace;
in vec3 gEyeDirection_tangentspace;
//this is the texture
uniform sampler2D tex; 
uniform sampler2D nor;


void main() {
	int LightPower = 100;
	int Distance = 10;
	vec4 LightColor = vec4(0.9,1.0,0.69,1.0);

    vec3 TextureNormal_tangentspace = normalize(texture( nor, gTexCoord ).rgb*2.0 - 1.0);

	vec3 n = normalize( TextureNormal_tangentspace );	 
	vec3 l = normalize( gLightDirection_tangentspace );
	float cosTheta = clamp( dot( n,l ), 0,1 );

	vec3 E = -normalize(gEyeDirection_tangentspace);
	vec3 R = reflect(-l,n);
	float cosAlpha = clamp( dot( E,R ), 0,1 );

	vec4 MaterialDiffuseColor = texture(tex,gTexCoord);
	vec4 MaterialAmbientColor = vec4(0.02,0.02,0.02,1.0) * MaterialDiffuseColor;
	vec4 MaterialSpecularColor = vec4(1.0,1.0,1.0,1.0);

    vec4 color = MaterialAmbientColor +  
	MaterialDiffuseColor * LightColor * LightPower * cosTheta / (Distance*Distance) +
    MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,1000) / (Distance*Distance);


    gl_FragColor = vec4(color.rgb,1);
}