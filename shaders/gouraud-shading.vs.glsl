#version 410 core
//------------------------------------------------------------------------------------------
// vertex shader, gouraud shading
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

layout(std140) uniform Material
{
    vec4 diffuseColor;
    vec4 specularColor;
    float reflection;
    float shininess;
} material;

uniform int lightingMode;
uniform float ambientLight;
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
    vec3 f_ambientLight;
    vec3 f_diffuseLight;
    vec3 f_specularLight;
    vec2 f_texCoord;
};

//------------------------------------------------------------------------------------------
// If it use vertex color, it must set material.diffuseColor.x to a number < 0.0f
//------------------------------------------------------------------------------------------
void main(void)
{
    vec4 worldCoord = modelMatrix * vec4(v_coord, 1.0);

    vec3 normal = mat3(normalMatrix) * v_normal;
    vec3 lightDir = vec3(light.position) - vec3(worldCoord);
    vec3 viewDir = vec3(cameraPosition) - vec3(worldCoord);

    normal = normalize(normal);
    lightDir = normalize(lightDir);
    viewDir = normalize(viewDir);


    vec3 ambient = vec3(0.0f);
    vec3 diffuse = vec3(0.0f);
    vec3 specular = vec3(0.0f);

    if(lightingMode == 1 || lightingMode == 0)
    {
        ambient = vec3(ambientLight);
    }

    if(lightingMode == 2 || lightingMode == 0)
    {
        diffuse = vec3(max(dot(normal, lightDir), 0.0f));

        vec3 halfDir = normalize(lightDir + viewDir);
        specular = pow(max(dot(halfDir, normal), 0.0f), material.shininess) * vec3(1.0f);
    }

    /////////////////////////////////////////////////////////////////
    // output
    f_shadowCoord = scaleMatrix * shadowMatrix * worldCoord;
    f_color = v_color;
    f_ambientLight = ambient;
    f_diffuseLight = light.intensity * diffuse;
    f_specularLight = light.intensity * specular;
    f_texCoord = v_texCoord;

    gl_Position = viewProjectionMatrix * worldCoord;
}
