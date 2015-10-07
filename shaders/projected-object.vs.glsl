#version 410 core
//------------------------------------------------------------------------------------------
// vertex shader, projected object shading
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

uniform vec4 planeVector;

//------------------------------------------------------------------------------------------
// input
in vec3 v_coord;

//------------------------------------------------------------------------------------------
// output
out float f_keepFragment;

//------------------------------------------------------------------------------------------
void main()
{
    vec4 worldCoord = modelMatrix * vec4(v_coord, 1.0);

    vec3 objectPos = vec3(worldCoord);
    vec3 lightPos = vec3(light.position);
    vec3 planeNormal = vec3(planeVector);
    vec3 dirLight2Object = objectPos - lightPos;

    vec3 projectedObjectPos = lightPos - (planeVector.w + dot(planeNormal, lightPos)) / dot(planeNormal, dirLight2Object) * dirLight2Object;
    vec3 dirLight2ProjectedPos = projectedObjectPos - lightPos;
    float distObjectPos = length(dirLight2Object);
    float distProjectedPos = length(dirLight2ProjectedPos);

    /////////////////////////////////////////////////////////////////
    // output
    f_keepFragment = 0.0;
    if((dot(dirLight2Object, dirLight2ProjectedPos) > 0) && (distProjectedPos > distObjectPos))
        f_keepFragment = 1.0;

    gl_Position = viewProjectionMatrix * vec4(projectedObjectPos, 1.0);
}
