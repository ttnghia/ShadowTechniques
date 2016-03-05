//------------------------------------------------------------------------------------------
//
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <QtGui>
#include <QtWidgets>
#include <QOpenGLFunctions_4_0_Core>

#include "unitcube.h"
#include "unitsphere.h"
#include "unitplane.h"
#include "objloader.h"

//------------------------------------------------------------------------------------------
#define PRINT_ERROR(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
}

#define PRINT_AND_DIE(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
    exit(EXIT_FAILURE); \
}

#define TRUE_OR_DIE(_condition, _errStr) \
{ \
    if(!(_condition)) \
    { \
        qDebug()<< "Fatal error occured at line:" << __LINE__ << ", file:" << __FILE__; \
        qDebug()<< "Error message:" << _errStr; \
        exit(EXIT_FAILURE); \
    } \
}

#define SIZE_OF_MAT4 (4 * 4 *sizeof(GLfloat))
#define SIZE_OF_VEC4 (4 * sizeof(GLfloat))
//------------------------------------------------------------------------------------------
#define MOVING_INERTIA 0.9f
#define DEPTH_TEXTURE_SIZE 1024
#define DEFAULT_CAMERA_POSITION QVector3D(0.0f,  6.5f, 25.0f)
#define DEFAULT_CAMERA_FOCUS QVector3D(0.0f,  6.5f, 0.0f)
#define DEFAULT_LIGHT_POSITION QVector4D(-2.0f, 12.0f, 6.0f, 1.0f)
#define DEFAULT_CUBE_POSITION QVector3D(3.0f, 1.001f, -2.0f)
#define DEFAULT_MESH_OBJECT_POSITION QVector3D(4.5f, 3.0f, -3.0f)
#define DEFAULT_BILLBOARD_OBJECT_POSITION QVector3D(-1.0f, 1.001f, 0.0f)
#define DEFAULT_OCCLUDER_POSITION QVector3D(0.0f, 8.001f, 0.0f)

struct Light
{
    Light():
        position(10.0f, 10.0f, 10.0f, 1.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        intensity(1.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 1) * sizeof(GLfloat);
    }

    QVector4D position;
    QVector4D color;
    GLfloat intensity;
};

struct Material
{
    Material():
        diffuseColor(-10.0f, 1.0f, 0.0f, 1.0f),
        specularColor(1.0f, 1.0f, 1.0f, 1.0f),
        reflection(0.0f),
        shininess(10.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 2) * sizeof(GLfloat);
    }

    void setDiffuse(QVector4D _diffuse)
    {
        diffuseColor = _diffuse;
    }

    void setSpecular(QVector4D _specular)
    {
        specularColor = _specular;
    }

    void setReflection(float _reflection)
    {
        reflection = _reflection;
    }

    QVector4D diffuseColor;
    QVector4D specularColor;
    GLfloat reflection;
    GLfloat shininess;
};

enum FloorTexture
{
    CHECKERBOARD1 = 0,
    CHECKERBOARD2,
    STONE1,
    STONE2,
    WOOD1,
    WOOD2,
    NUM_FLOOR_TEXTURES
};

enum MeshObject
{
    TEAPOT_OBJ = 0,
    BUNNY_OBJ,
    DUCK_OBJ,
    MICKEY_OBJ,
    NUM_MESH_OBJECT
};

enum MouseTransformationTarget
{
    TRANSFORM_CAMERA = 0,
    TRANSFORM_OBJECTS,
    TRANSFORM_LIGHT,
    TRANSFORM_OCCLUDER,
    NUM_TRANSFORMATION_TARGET
};

enum ShadowModes
{
    NO_SHADOW = 0,
    PROJECTIVE_SHADOW,
    SHADOW_MAP,
    SHADOW_VOLUME,
    NUM_SHADOW_METHODS
};


enum ShadingProgram
{
    GOURAUD_SHADING = 0,
    PHONG_SHADING,
    LIGHT_SHADING,
    PROJECTED_OBJECT_SHADING,
    SHADOW_MAP_SHADING,
    SHADOW_VOLUME_SHADING,
    NUM_SHADING_MODE
};

enum LightingMode
{
    ALL_LIGHT = 0,
    AMBIENT_LIGHT = 1,
    DIFFUSE_SPECULAR = 2,
    NUM_LIGHTING_MODES
};

enum UBOBinding
{
    BINDING_MATRICES = 0,
    BINDING_LIGHT,
    BINDING_ROOM_MATERIAL,
    BINDING_CUBE_MATERIAL,
    BINDING_MESH_OBJECT_MATERIAL,
    BINDING_BILLBOARD_OBJECT_MATERIAL,
    BINDING_OCCLUDER_MATERIAL,
    NUM_BINDING_POINTS
};


//------------------------------------------------------------------------------------------
class Renderer : public QOpenGLWidget, QOpenGLFunctions_4_0_Core// QOpenGLFunctions
{
    Q_OBJECT
public:
    enum SpecialKey
    {
        NO_KEY,
        SHIFT_KEY,
        CTRL_KEY
    };

    enum MouseButton
    {
        NO_BUTTON,
        LEFT_BUTTON,
        RIGHT_BUTTON
    };

    Renderer(QWidget* parent = 0);
    ~Renderer();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void keyPressEvent(QKeyEvent* _event);
    void keyReleaseEvent(QKeyEvent* _event);
    void wheelEvent(QWheelEvent* _event);

    void setShadingMode(ShadingProgram _shadingMode);
    void setFloorTexture(FloorTexture _texture);
    void setFloorTextureFilteringMode(QOpenGLTexture::Filter _textureFiltering);
    void setRoomColor(float _r, float _g, float _b);
    void setCubeColor(float _r, float _g, float _b);
    void setMeshObjectColor(float _r, float _g, float _b);
    void setOccluderColor(float _r, float _g, float _b);

public slots:
    void enableDepthTest(bool _status);
    void enableZAxisRotation(bool _status);
    void enableTextureAnisotropicFiltering(bool _state);
    void enableShowShadowVolume(bool _state);
    void setMouseTransformationTarget(MouseTransformationTarget _mouseTarget);
    void setShadowMethod(ShadowModes _shadowMode = NO_SHADOW);
    void setRoomSize(int _roomSize);
    void setAmbientLight(int _ambientLight);
    void setLightIntensity(int _intensity);
    void setMeshObject(int _objectIndex);
    void resetCameraPosition();
    void resetObjectPositions();
    void resetLightPosition();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent* _event);
    void mouseMoveEvent(QMouseEvent* _event);
    void mouseReleaseEvent(QMouseEvent* _event);

private:
    void checkOpenGLVersion();
    void initTestScene();
    void initScene();
    bool initShaderPrograms();
    bool validateShaderPrograms(ShadingProgram _shadingMode);
    bool initProgram(ShadingProgram _shadingMode);
    bool initLightShadingProgram();
    bool initProjectedObjectShadingProgram();
    bool initShadowMapShadingProgram();
    bool initShadowVolumeShadingProgram();

    void initSharedBlockUniform();
    void initTexture();
    void initSceneMemory();
    void initLightObjectMemory();
    void initRoomMemory();
    void initCubeMemory();
    void initMeshObjectMemory();
    void initBillboardMemory();
    void initShadowVolumeMemory();
    void initVertexArrayObjects();
    void initLightVAO();
    void initRoomVAO(ShadingProgram _shadingMode);
    void initCubeVAO(ShadingProgram _shadingMode);
    void initMeshObjectVAO(ShadingProgram _shadingMode);
    void initBillboardVAO(ShadingProgram _shadingMode);
    void initShadowVolumeVAO();
    void initSceneMatrices();
    void initDepthBufferObject();

    void updateCamera();
    void translateCamera();
    void rotateCamera();
    void zoomCamera();

    void translateObjects();
    void rotateObjects();

    void translateLight();

    void calculateOccluderFaceVertices();
    void translateOccluder();
    void rotateOccluder();


    void renderTestScene();

    void renderScene();
    void renderObjectWithoutShadow(int _lightingMode);
    void renderObjectWithProjectiveShadow();

    void generateShadowMap();
    void renderObjectWithShadowMap();

    void generateShadowVolume();
    void renderShadowVolume();
    void renderObjectWithShadowVolume();

    void renderLight();
    void renderRoom();
    void renderRoom2DepthMap();

    void renderCube();
    void renderProjectedCube();
    void renderCube2DepthMap();

    void renderMeshObject();
    void renderProjectedMeshObject();
    void renderMeshObject2DepthMap();

    void renderBillboardObject();
    void renderBillboardObject2DepthMap();

    void renderOccluder();
    void renderProjectedOccluder();
    void renderOccluder2DepthMap();


    QOpenGLTexture* floorTextures[NUM_FLOOR_TEXTURES];
    QOpenGLTexture* ceilingTexture;
    QOpenGLTexture* meshObjectTexture;
    QOpenGLTexture* billboardTexture;
    QOpenGLTexture* decalTexture;
    UnitPlane* planeObject;
    UnitCube* cubeObject;
    OBJLoader* objLoader;

    QMap<ShadingProgram, QString> vertexShaderSourceMap;
    QMap<ShadingProgram, QString> fragmentShaderSourceMap;
    QOpenGLShaderProgram* glslPrograms[NUM_SHADING_MODE];
    QOpenGLShaderProgram* currentShadingProgram;
    QOpenGLShaderProgram* projectedShadowProgram;
    QOpenGLShaderProgram* shadowMapProgram;
    QOpenGLShaderProgram* shadowVolumeProgram;
    GLuint UBOBindingIndex[NUM_BINDING_POINTS];
    GLuint UBOMatrices;
    GLuint UBOLight;
    GLuint UBORoomMaterial;
    GLuint UBOCubeMaterial;
    GLuint UBOMeshObjectMaterial;
    GLuint UBOBillboardObjectMaterial;
    GLuint UBOOccluderMaterial;
    GLint attrVertex[NUM_SHADING_MODE];
    GLint attrNormal[NUM_SHADING_MODE];
    GLint attrTexCoord[NUM_SHADING_MODE];

    GLint uniMatrices[NUM_SHADING_MODE];
    GLint uniCameraPosition[NUM_SHADING_MODE];
    GLint uniLight[NUM_SHADING_MODE];
    GLint uniLightingMode[NUM_SHADING_MODE];
    GLint uniAmbientLight[NUM_SHADING_MODE];
    GLint uniMaterial[NUM_SHADING_MODE];
    GLint uniObjTexture[NUM_SHADING_MODE];
    GLint uniDepthTexture[NUM_SHADING_MODE];
    GLint uniHasObjTexture[NUM_SHADING_MODE];
    GLint uniHasDepthTexture[NUM_SHADING_MODE];
    GLint uniPlaneVector;
    GLint uniShadowIntensity;

    QOpenGLFramebufferObject* FBODepthMap;
    QOpenGLTexture* depthTexture;

    QOpenGLVertexArrayObject vaoLight;
    QOpenGLVertexArrayObject vaoShadowVolume;
    QOpenGLVertexArrayObject vaoRoom[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoCube[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoMeshObject[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoBillboard[NUM_SHADING_MODE];
    QOpenGLBuffer vboLight;
    QOpenGLBuffer vboRoom;
    QOpenGLBuffer vboCube;
    QOpenGLBuffer vboMeshObject;
    QOpenGLBuffer vboBillboard;
    QOpenGLBuffer vboShadowVolume;
    QOpenGLBuffer iboMeshObject;
    QOpenGLBuffer iboRoom;
    QOpenGLBuffer iboCube;
    QOpenGLBuffer iboBillboard;

    Material roomMaterial;
    Material cubeMaterial;
    Material meshObjectMaterial;
    Material billboardObjectMaterial;
    Material occluderMaterial;
    Light light;


    // data for shadow volume construction
    QVector<QVector3D> shadowVolume;
    UnitCube::CubeFaceTriangle* occluderFaces[12];


    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewProjectionMatrix;

    QMatrix4x4 roomModelMatrix;
    QMatrix4x4 roomNormalMatrix;
    QMatrix4x4 cubeModelMatrix;
    QMatrix4x4 cubeProjectedModelMatrix;
    QMatrix4x4 cubeNormalMatrix;
    QMatrix4x4 meshObjectModelMatrix;
    QMatrix4x4 meshObjectNormalMatrix;
    QMatrix4x4 billboardObjectModelMatrix;
    QMatrix4x4 occluderModelMatrix;
    QMatrix4x4 occluderNormalMatrix;

    QMatrix4x4 lightViewMatrix;
    QMatrix4x4 lightProjectionMatrix;
    QMatrix4x4 shadowMatrix;

    qreal retinaScale;
    float zooming;
    QVector3D cameraPosition;
    QVector3D cameraFocus;
    QVector3D cameraUpDirection;

    QVector2D lastMousePos;
    QVector3D translation;
    QVector3D translationLag;
    QVector3D rotation;
    QVector3D rotationLag;
    QVector3D scalingLag;
    SpecialKey specialKeyPressed;
    MouseButton mouseButtonPressed;

    ShadingProgram currentShadingMode;
    FloorTexture currentFloorTexture;
    MeshObject currentMeshObject;
    MouseTransformationTarget currentMouseTransTarget;
    ShadowModes currentShadowMode;
    float ambientLight;
    float roomSize;
    bool enabledZAxisRotation;
    bool enabledTextureAnisotropicFiltering;
//    bool enabledShadowMap;
    bool enabledShowShadowVolume;

    bool initializedScene;
    bool initializedTestScene;
    bool initializedDepthBuffer;
};

#endif // GLRENDERER_H
