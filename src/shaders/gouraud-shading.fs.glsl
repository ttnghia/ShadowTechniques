#version 410 core
//------------------------------------------------------------------------------------------
// fragment shader, gouraud shading
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// uniforms
layout(std140) uniform Material
{
    vec4 diffuseColor;
    vec4 specularColor;
    float reflection;
    float shininess;
} material;

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
    vec3 f_ambientLight;
    vec3 f_diffuseLight;
    vec3 f_specularLight;
    vec2 f_texCoord;
};

//------------------------------------------------------------------------------------------
// out variables
out vec4 fragColor;

//------------------------------------------------------------------------------------------
// If an object uses texture, it must set "GL_TRUE" to hasObjTex
//------------------------------------------------------------------------------------------
void main()
{
    vec3 surfaceColor = vec3(0.0f);
    float alpha = 0.0f;
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

    float isNoShadow = 1.0f;
    if(hasDepthTex)
    {
        if(any(lessThan(vec3(f_shadowCoord), vec3(0.0))) )
            isNoShadow = 1.0f;
        else
            isNoShadow = textureProj(depthTex, f_shadowCoord);
    }

    /////////////////////////////////////////////////////////////////
    // output
    fragColor = vec4(f_ambientLight * surfaceColor + isNoShadow * (f_diffuseLight * surfaceColor +
                f_specularLight * vec3(material.specularColor)), alpha);
}
