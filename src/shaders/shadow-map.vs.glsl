#version 410 core
//------------------------------------------------------------------------------------------
// vertex shader, shadow map shading
//------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------
// uniforms
layout(std140) uniform Matrices
{
    mat4 modelMatrix;
    mat4 normalMatrix;
    mat4 viewProjectionMatrix;
    mat4 shadowMatrix;
};

//------------------------------------------------------------------------------------------
// in variables
in vec3 v_coord;
in vec2 v_texCoord;
//------------------------------------------------------------------------------------------
// out variables
out vec2 f_texCoord;

//------------------------------------------------------------------------------------------
void main()
{
    vec4 worldCoord = modelMatrix * vec4(v_coord, 1.0);

    /////////////////////////////////////////////////////////////////
    // output
    f_texCoord = v_texCoord;
    gl_Position = shadowMatrix * worldCoord;
}
