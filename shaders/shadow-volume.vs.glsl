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

//------------------------------------------------------------------------------------------
void main()
{
    /////////////////////////////////////////////////////////////////
    // output
    gl_Position = viewProjectionMatrix * vec4(v_coord, 1.0);
}
