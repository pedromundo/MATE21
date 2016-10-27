#version 400
in vec2 gTexCoord;
in vec3 gLightDirection_tangentspace;
in vec3 gEyeDirection_tangentspace;
//Texture and normal
uniform sampler2D tex, nor; 
uniform uint lightDiffusePower, lightSpecularPower, lightDistance;

out vec4 fragColor;

void main() {	
	vec4 LightColor = vec4(1.0,1.0,1.0,1.0);

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
	MaterialDiffuseColor * LightColor * lightDiffusePower * cosTheta / (lightDistance*lightDistance) +
    MaterialSpecularColor * LightColor * lightSpecularPower * pow(cosAlpha,10) / (lightDistance*lightDistance);

	color = vec4(color.xyz*2,1);

	float physicalIntensity = 0.2126 * color.r + 0.7152 * color.g + 0.0722 * color.b;

	if (physicalIntensity > 0.75){
        color = vec4(1.0,1.0,1.0,1.0) * color;
	} else if (physicalIntensity > 0.5){
        color = vec4(0.7,0.7,0.7,1.0) * color;
    } else if (physicalIntensity > 0.25){
        color = vec4(0.35,0.35,0.35,1.0) * color;
	} else {
        color = vec4(0.1,0.1,0.1,1.0) * color;
	}

    fragColor = vec4(color.rgb,1);
}