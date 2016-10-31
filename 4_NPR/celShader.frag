#version 400
in vec2 gTexCoord;
in vec3 gLightDirection_tangentspace;
in vec3 gEyeDirection_tangentspace;
in vec4 gPosition_clipspace;
//Texture and normal
uniform sampler2D tex, nor; 
uniform uint lightDiffusePower, lightSpecularPower, lightDistance, shaderOption;
uniform bool binarize;


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

	if(shaderOption == 1){
		if(sin(mix(-200,200,gPosition_clipspace.y)) < 0.1){
			discard;
		}
		if(sin(mix(-200,200,gPosition_clipspace.x)) < 0.1){
			discard;
		}
	}else if(shaderOption == 2){
		if(sin(mix(-200,200,gPosition_clipspace.y)) < 0.1){
			MaterialDiffuseColor = vec4(0,0,0,1);
		}

		if(sin(mix(-200,200,gPosition_clipspace.x)) < 0.1){
			MaterialDiffuseColor = vec4(0,0,0,1);
		}
	}else if(shaderOption == 3){
		MaterialDiffuseColor = vec4(vec3(0.2126 * MaterialDiffuseColor.r + 0.7152 * MaterialDiffuseColor.g + 0.0722 * MaterialDiffuseColor.b),1);
	}else if(shaderOption == 4){
		MaterialDiffuseColor = vec4(0.5-MaterialDiffuseColor.r,0.5-MaterialDiffuseColor.g,0.5-MaterialDiffuseColor.b,1);
	}

	if(binarize){
		MaterialDiffuseColor = vec4((0.2126 * MaterialDiffuseColor.r + 0.7152 * MaterialDiffuseColor.g + 0.0722 * MaterialDiffuseColor.b)>0.18?1.0:0.0);
	}

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