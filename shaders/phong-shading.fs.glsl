#version 410 core
//------------------------------------------------------------------------------------------
// fragment shader, phong shading
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// uniforms
layout(std140) uniform Light
{
    vec4 position;
    vec4 color;
    float intensity;
} light;

layout(std140) uniform Material
{
    vec4 diffuseColor;
    vec4 specularColor;
    float reflection;
    float shininess;
} material;

// lightingMode: 1 = ambient only, 2 = diffuse+spec only, 0 = all light
uniform int lightingMode;
uniform float ambientLight;
uniform sampler2DShadow depthTex;
uniform sampler2D objTex;
uniform bool hasObjTex;
uniform bool hasDepthTex;
uniform bool discardTransparentPixel;

//------------------------------------------------------------------------------------------
// in variables
in VS_OUT
{
    vec4 f_shadowCoord;
    vec3 f_color;
    vec3 f_normal;
    vec3 f_lightDir;
    vec3 f_viewDir;
    vec2 f_texCoord;
};

//----------------------------------------------------------`--------------------------------
// out variables
out vec4 fragColor;

//------------------------------------------------------------------------------------------
// If an object uses texture, it must set "GL_TRUE" to hasObjTex
// If it use vertex color, it must set material.diffuseColor.x to a number < 0.0f
//------------------------------------------------------------------------------------------
void main()
{
    vec3 normal = normalize(f_normal);
    vec3 lightDir = normalize(f_lightDir);
    vec3 viewDir = normalize(f_viewDir);
    vec3 reflectionDir = reflect(-viewDir, normal);

    float alpha = 0.0f;
    vec3 surfaceColor = vec3(0.0f);

    if(hasObjTex)
    {
        vec4 texVal = texture(objTex, f_texCoord);
        if(discardTransparentPixel && (texVal.w < 0.5f)) discard;

        surfaceColor = texVal.xyz;
        alpha = texVal.w;
    }

    if(material.diffuseColor.x > -0.001f)
    {
        surfaceColor = mix(vec3(material.diffuseColor), surfaceColor, alpha);
    }
    else
    {
        surfaceColor = mix(f_color, surfaceColor, alpha);
    }

    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);
    float isNoShadow = 1.0f;

    if(lightingMode == 1 || lightingMode == 0)
    {
        ambient = ambientLight * surfaceColor;
    }

    if(lightingMode == 2 || lightingMode == 0)
    {
        diffuse = vec3(max(dot(normal, lightDir), 0.0f)) * surfaceColor;
        vec3 halfDir = normalize(lightDir + viewDir);
        specular = pow(max(dot(halfDir, normal), 0.0f), material.shininess) * vec3(material.specularColor);
    }

    if(hasDepthTex)
    {
        if(any(lessThan(vec3(f_shadowCoord), vec3(0.0))) )
            isNoShadow = 1.0f;
        else
            isNoShadow = textureProj(depthTex, f_shadowCoord);
    }

    /////////////////////////////////////////////////////////////////
    // output
    fragColor = vec4(ambient + isNoShadow * light.intensity * (diffuse + specular), alpha);
}
