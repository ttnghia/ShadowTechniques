#version 410 core
//------------------------------------------------------------------------------------------
// vertex shader, phong shading
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

layout(std140) uniform Light
{
    vec4 position;
    vec4 color;
    float intensity;
} light;

uniform vec3 cameraPosition;

//------------------------------------------------------------------------------------------
// const
const mat4 scaleMatrix = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f),
                              vec4(0.0f, 0.5f, 0.0f, 0.0f),
                              vec4(0.0f, 0.0f, 0.5f, 0.0f),
                              vec4(0.5f, 0.5f, 0.5f, 1.0f));

//------------------------------------------------------------------------------------------
// in variables
in vec3 v_coord;
in vec3 v_color;
in vec3 v_normal;
in vec2 v_texCoord;

//------------------------------------------------------------------------------------------
// out variables
out VS_OUT
{
    vec4 f_shadowCoord;
    vec3 f_color;
    vec3 f_normal;
    vec3 f_lightDir;
    vec3 f_viewDir;
    vec2 f_texCoord;
};

//------------------------------------------------------------------------------------------
void main()
{
    vec4 worldCoord = modelMatrix * vec4(v_coord, 1.0);

    /////////////////////////////////////////////////////////////////
    // output
    f_shadowCoord = scaleMatrix * shadowMatrix * worldCoord;
    f_color = v_color;
    f_normal = mat3(normalMatrix) * v_normal;
    f_lightDir = vec3(light.position) - vec3(worldCoord);
    f_viewDir = vec3(cameraPosition) - vec3(worldCoord);
    f_texCoord = v_texCoord;

    gl_Position = viewProjectionMatrix * worldCoord;
}
