//------------------------------------------------------------------------------------------
//
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "renderer.h"

//------------------------------------------------------------------------------------------
Renderer::Renderer(QWidget* _parent):
    QOpenGLWidget(_parent),
    initializedScene(false),
    initializedTestScene(false),
    initializedDepthBuffer(false),
    enabledZAxisRotation(false),
    enabledTextureAnisotropicFiltering(true),
    enabledShowShadowVolume(false),
    currentShadowMode(NO_SHADOW),
    iboRoom(QOpenGLBuffer::IndexBuffer),
    iboCube(QOpenGLBuffer::IndexBuffer),
    iboMeshObject(QOpenGLBuffer::IndexBuffer),
    iboBillboard(QOpenGLBuffer::IndexBuffer),
    specialKeyPressed(Renderer::NO_KEY),
    mouseButtonPressed(Renderer::NO_BUTTON),
    translation(0.0f, 0.0f, 0.0f),
    translationLag(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    rotationLag(0.0f, 0.0f, 0.0f),
    zooming(0.0f),
    planeObject(NULL),
    cubeObject(NULL),
    objLoader(NULL),
    depthTexture(NULL),
    FBODepthMap(NULL),
    cameraPosition(DEFAULT_CAMERA_POSITION),
    cameraFocus(DEFAULT_CAMERA_FOCUS),
    cameraUpDirection(0.0f, 1.0f, 0.0f),
    currentShadingMode(PHONG_SHADING),
    currentFloorTexture(WOOD1),
    currentMeshObject(TEAPOT_OBJ),
    currentMouseTransTarget(TRANSFORM_CAMERA),
    ambientLight(0.4)
{
    retinaScale = devicePixelRatio();
    setFocusPolicy(Qt::StrongFocus);
}

//------------------------------------------------------------------------------------------
Renderer::~Renderer()
{
}

//------------------------------------------------------------------------------------------
void Renderer::checkOpenGLVersion()
{
    QString verStr = QString((const char*)glGetString(GL_VERSION));
    int major = verStr.left(verStr.indexOf(".")).toInt();
    int minor = verStr.mid(verStr.indexOf(".") + 1, 1).toInt();

    if(!(major >= 4 && minor >= 0))
    {
        QMessageBox::critical(this, "Error",
                              QString("Your OpenGL version is %1.%2, which does not satisfy this program requirement (OpenGL >= 4.1)")
                              .arg(major).arg(minor));
        close();
    }

//    qDebug() << major << minor;
//    qDebug() << verStr;
    //    TRUE_OR_DIE(major >= 4 && minor >= 1, "OpenGL version must >= 4.1");
}

//------------------------------------------------------------------------------------------
GLfloat triangle_vertices[] =
{
    0.0,  0.8, 0,
    -0.8, -0.8, 0,
    0.8, -0.8, 0
};

GLuint vbo = 0;
GLuint vao = 0;
QOpenGLShaderProgram* testProgram = new QOpenGLShaderProgram;

void Renderer::initTestScene()
{

    bool success;

    success = testProgram ->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                    ":/shaders/test.vs.glsl");
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = testProgram ->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                    ":/shaders/test.fs.glsl");
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = testProgram ->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    glUseProgram(testProgram ->programId());

    const char* attribute_name = "v_coord";
    GLint attribute_coord2d = glGetAttribLocation(testProgram->programId(), attribute_name);

    if (attribute_coord2d == -1)
    {
        qDebug() << "Could not bind attribute " << attribute_name;
        return;
    }

    glEnableVertexAttribArray(attribute_coord2d);


    glGenBuffers (1, &vbo);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glBufferData (GL_ARRAY_BUFFER, 9 * sizeof (float), triangle_vertices, GL_STATIC_DRAW);


    glGenVertexArrays (1, &vao);
//    glBindVertexArray (vao);
    glEnableVertexAttribArray(0);
    glBindBuffer (GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    initializedTestScene = true;
}

//------------------------------------------------------------------------------------------
void Renderer::initScene()
{
    TRUE_OR_DIE(initShaderPrograms(), "Cannot initialize shaders. Exit...");

    initTexture();
    initSceneMemory();
    initVertexArrayObjects();
    initSharedBlockUniform();
    initSceneMatrices();

    glEnable(GL_DEPTH_TEST);
    setShadingMode(PHONG_SHADING);
}
//------------------------------------------------------------------------------------------
bool Renderer::initProgram(ShadingProgram _shadingMode)
{
    QOpenGLShaderProgram* program;
    GLint location;

    /////////////////////////////////////////////////////////////////
    glslPrograms[_shadingMode] = new QOpenGLShaderProgram;
    program = glslPrograms[_shadingMode];
    bool success;

    success = program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                               vertexShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                               fragmentShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = program->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[_shadingMode] = location;

    location = program->attributeLocation("v_normal");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex normal.");
    attrNormal[_shadingMode] = location;

    location = program->attributeLocation("v_texCoord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute texture coordinate.");
    attrTexCoord[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Light");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniLight[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Material");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMaterial[_shadingMode] = location;

    location = program->uniformLocation("cameraPosition");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform cameraPosition.");
    uniCameraPosition[_shadingMode] = location;


    location = program->uniformLocation("lightingMode");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform lightingMode.");
    uniLightingMode[_shadingMode] = location;

    location = program->uniformLocation("ambientLight");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform ambientLight.");
    uniAmbientLight[_shadingMode] = location;

    location = program->uniformLocation("objTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform objTex.");
    uniObjTexture[_shadingMode] = location;

    location = program->uniformLocation("depthTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform depthTex.");
    uniDepthTexture[_shadingMode] = location;

    location = program->uniformLocation("hasObjTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform hasObjTex.");
    uniHasObjTexture[_shadingMode] = location;

    location = program->uniformLocation("hasDepthTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform hasDepthTex.");
    uniHasDepthTexture[_shadingMode] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initLightShadingProgram()
{
    GLint location;
    glslPrograms[LIGHT_SHADING] = new QOpenGLShaderProgram;
    QOpenGLShaderProgram* program = glslPrograms[LIGHT_SHADING];
    bool success;

    success = program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                               vertexShaderSourceMap.value(LIGHT_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                               fragmentShaderSourceMap.value(LIGHT_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = glGetUniformBlockIndex(program->programId(), "Matrices");

    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[LIGHT_SHADING] = location;

    location = glGetUniformBlockIndex(program->programId(), "Light");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniLight[LIGHT_SHADING] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initProjectedObjectShadingProgram()
{
    GLint location;
    glslPrograms[PROJECTED_OBJECT_SHADING] = new QOpenGLShaderProgram;
    projectedShadowProgram = glslPrograms[PROJECTED_OBJECT_SHADING];
    bool success;

    success = projectedShadowProgram->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                              vertexShaderSourceMap.value(PROJECTED_OBJECT_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = projectedShadowProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                              fragmentShaderSourceMap.value(PROJECTED_OBJECT_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = projectedShadowProgram->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = projectedShadowProgram->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[PROJECTED_OBJECT_SHADING] = location;

    location = glGetUniformBlockIndex(projectedShadowProgram->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[PROJECTED_OBJECT_SHADING] = location;

    location = glGetUniformBlockIndex(projectedShadowProgram->programId(), "Light");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniLight[PROJECTED_OBJECT_SHADING] = location;

    location = glGetUniformLocation(projectedShadowProgram->programId(), "planeVector");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform.");
    uniPlaneVector = location;

    location = glGetUniformLocation(projectedShadowProgram->programId(), "shadowIntensity");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform.");
    uniShadowIntensity = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initShadowMapShadingProgram()
{
    GLint location;
    glslPrograms[SHADOW_MAP_SHADING] = new QOpenGLShaderProgram;
    shadowMapProgram = glslPrograms[SHADOW_MAP_SHADING];
    bool success;

    success = shadowMapProgram->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                        vertexShaderSourceMap.value(SHADOW_MAP_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = shadowMapProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                        fragmentShaderSourceMap.value(SHADOW_MAP_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = shadowMapProgram->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = shadowMapProgram->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[SHADOW_MAP_SHADING] = location;

    location = shadowMapProgram->attributeLocation("v_texCoord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute texture coordinate.");
    attrTexCoord[SHADOW_MAP_SHADING] = location;

    location = glGetUniformBlockIndex(shadowMapProgram->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[SHADOW_MAP_SHADING] = location;

    location = shadowMapProgram->uniformLocation("objTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform objTex.");
    uniObjTexture[SHADOW_MAP_SHADING] = location;

    location = shadowMapProgram->uniformLocation("hasObjTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform hasObjTex.");
    uniHasObjTexture[SHADOW_MAP_SHADING] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initShadowVolumeShadingProgram()
{
    GLint location;
    glslPrograms[SHADOW_VOLUME_SHADING] = new QOpenGLShaderProgram;
    shadowVolumeProgram = glslPrograms[SHADOW_VOLUME_SHADING];
    bool success;

    success = shadowVolumeProgram->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                                           vertexShaderSourceMap.value(SHADOW_VOLUME_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = shadowVolumeProgram->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                                           fragmentShaderSourceMap.value(SHADOW_VOLUME_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = shadowVolumeProgram->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = shadowVolumeProgram->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[SHADOW_VOLUME_SHADING] = location;

    location = glGetUniformBlockIndex(shadowVolumeProgram->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[SHADOW_VOLUME_SHADING] = location;


    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initShaderPrograms()
{
    vertexShaderSourceMap.insert(GOURAUD_SHADING, ":/shaders/gouraud-shading.vs.glsl");
    vertexShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.vs.glsl");
    vertexShaderSourceMap.insert(LIGHT_SHADING, ":/shaders/light.vs.glsl");
    vertexShaderSourceMap.insert(PROJECTED_OBJECT_SHADING,
                                 ":/shaders/projected-object.vs.glsl");
    vertexShaderSourceMap.insert(SHADOW_MAP_SHADING,
                                 ":/shaders/shadow-map.vs.glsl");
    vertexShaderSourceMap.insert(SHADOW_VOLUME_SHADING,
                                 ":/shaders/shadow-volume.vs.glsl");

    fragmentShaderSourceMap.insert(GOURAUD_SHADING, ":/shaders/gouraud-shading.fs.glsl");
    fragmentShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.fs.glsl");
    fragmentShaderSourceMap.insert(LIGHT_SHADING, ":/shaders/light.fs.glsl");
    fragmentShaderSourceMap.insert(PROJECTED_OBJECT_SHADING,
                                   ":/shaders/projected-object.fs.glsl");
    fragmentShaderSourceMap.insert(SHADOW_MAP_SHADING,
                                   ":/shaders/shadow-map.fs.glsl");
    fragmentShaderSourceMap.insert(SHADOW_VOLUME_SHADING,
                                   ":/shaders/shadow-volume.fs.glsl");

    return (initLightShadingProgram() &&
            initProjectedObjectShadingProgram() &&
            initShadowMapShadingProgram() &&
            initShadowVolumeShadingProgram() &&
            initProgram(GOURAUD_SHADING) &&
            initProgram(PHONG_SHADING));
}

//------------------------------------------------------------------------------------------
bool Renderer::validateShaderPrograms(ShadingProgram _shadingMode)
{
    GLint status;
    GLint logLen;
    GLchar log[1024];

    glValidateProgram(glslPrograms[_shadingMode]->programId());
    glGetProgramiv(glslPrograms[_shadingMode]->programId(), GL_VALIDATE_STATUS, &status);

    glGetProgramiv(glslPrograms[_shadingMode]->programId(), GL_INFO_LOG_LENGTH, &logLen);

    if(logLen > 0)
    {
        glGetProgramInfoLog(glslPrograms[_shadingMode]->programId(), logLen, &logLen, log);

        if(QString(log).trimmed().length() != 0)
        {
            qDebug() << "ShadingMode: " << _shadingMode << ", log: " << log;
        }
    }

    return (status == GL_TRUE);
}

//------------------------------------------------------------------------------------------
void Renderer::initSharedBlockUniform()
{
    /////////////////////////////////////////////////////////////////
    // setup the light and material
    light.position = DEFAULT_LIGHT_POSITION;
    light.intensity = 0.8f;

    roomMaterial.setDiffuse(QVector4D(0.02f, 0.45f, 1.0f, 1.0f));
    roomMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
    roomMaterial.shininess = 150.0f;

    cubeMaterial.setDiffuse(QVector4D(0.0f, 1.0f, 0.2f, 1.0f));
    cubeMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
    cubeMaterial.shininess = 50.0f;

    meshObjectMaterial.setDiffuse(QVector4D(2.0f / 3.0f, 1.0f / 3.0f, 0.0f, 1.0f));
    meshObjectMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
    meshObjectMaterial.shininess = 50.0f;

    billboardObjectMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 0.0f));
    billboardObjectMaterial.shininess = 50.0f;

    occluderMaterial.setDiffuse(QVector4D(1.0f, 0.1f, 0.6f, 0.0f));
    occluderMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 0.0f));
    occluderMaterial.shininess = 50.0f;

    /////////////////////////////////////////////////////////////////
    // setup binding points for block uniform
    for(int i = 0; i < NUM_BINDING_POINTS; ++i)
    {
        UBOBindingIndex[i] = i + 1;
    }

    /////////////////////////////////////////////////////////////////
    // setup data for block uniform
    glGenBuffers(1, &UBOMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 4 * SIZE_OF_MAT4, NULL,
                 GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    glGenBuffers(1, &UBOLight);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 &light, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBORoomMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBORoomMaterial);
    glBufferData(GL_UNIFORM_BUFFER, roomMaterial.getStructSize(),
                 &roomMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOCubeMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOCubeMaterial);
    glBufferData(GL_UNIFORM_BUFFER, cubeMaterial.getStructSize(),
                 &cubeMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOMeshObjectMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMeshObjectMaterial);
    glBufferData(GL_UNIFORM_BUFFER, meshObjectMaterial.getStructSize(),
                 &meshObjectMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOBillboardObjectMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOBillboardObjectMaterial);
    glBufferData(GL_UNIFORM_BUFFER, billboardObjectMaterial.getStructSize(),
                 &billboardObjectMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOOccluderMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOOccluderMaterial);
    glBufferData(GL_UNIFORM_BUFFER, occluderMaterial.getStructSize(),
                 &occluderMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
void Renderer::initTexture()
{
    ////////////////////////////////////////////////////////////////////////////////
    // mesh object texture
//    meshObjectTexture = new QOpenGLTexture(QImage(":/textures/earth.jpg").mirrored());
//    meshObjectTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
//    meshObjectTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

    ////////////////////////////////////////////////////////////////////////////////
    // decal texture
    decalTexture = new QOpenGLTexture(
        QImage(":/textures/minion.png").mirrored().convertToFormat(QImage::Format_RGBA8888));
    decalTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    decalTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    decalTexture->setWrapMode(QOpenGLTexture::DirectionS,
                              QOpenGLTexture::ClampToEdge);
    decalTexture->setWrapMode(QOpenGLTexture::DirectionT,
                              QOpenGLTexture::ClampToEdge);

    ////////////////////////////////////////////////////////////////////////////////
    // billboard texture
    billboardTexture = new QOpenGLTexture(
        QImage(":/textures/billboardblueflowers.png").convertToFormat(QImage::Format_RGBA8888));
    billboardTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    billboardTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    billboardTexture->setWrapMode(QOpenGLTexture::DirectionS,
                                  QOpenGLTexture::ClampToEdge);
    billboardTexture->setWrapMode(QOpenGLTexture::DirectionT,
                                  QOpenGLTexture::ClampToEdge);

    ////////////////////////////////////////////////////////////////////////////////
    // ceiling
    ceilingTexture = new QOpenGLTexture(QImage(":/textures/ceiling.png").mirrored());
//    TRUE_OR_DIE(ceilingTexture
//                && ceilingTexture->isCreated(), "Cannot load texture from file.");

    ceilingTexture->setMinificationFilter(QOpenGLTexture::Linear);
    ceilingTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    ceilingTexture->setWrapMode(QOpenGLTexture::Repeat);

    ////////////////////////////////////////////////////////////////////////////////
    // floor texture
    QMap<FloorTexture, QString> floorTexture2StrMap;
    floorTexture2StrMap[CHECKERBOARD1] = "checkerboard1.jpg";
    floorTexture2StrMap[CHECKERBOARD2] = "checkerboard2.jpg";
    floorTexture2StrMap[STONE1] = "stone1.jpg";
    floorTexture2StrMap[STONE2] = "stone2.jpg";
    floorTexture2StrMap[WOOD1] = "wood1.jpg";
    floorTexture2StrMap[WOOD2] = "wood2.jpg";

    TRUE_OR_DIE(floorTexture2StrMap.size() == NUM_FLOOR_TEXTURES,
                "Ohh, you forget to initialize some floor texture...");

    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        FloorTexture tex = static_cast<FloorTexture>(i);

        QString texFile = QString(":/textures/%1").arg(floorTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(texFile), "Cannot load texture from file.");
        floorTextures[tex] = new QOpenGLTexture(QImage(texFile).mirrored());
        floorTextures[tex]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setWrapMode(QOpenGLTexture::Repeat);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::initSceneMemory()
{
    initLightObjectMemory();
    initRoomMemory();
    initCubeMemory();
    initMeshObjectMemory();
    initBillboardMemory();
    initShadowVolumeMemory();
}

//------------------------------------------------------------------------------------------
void Renderer::initLightObjectMemory()
{
    if(vboLight.isCreated())
    {
        vboLight.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for cube
    vboLight.create();
    vboLight.bind();
    vboLight.allocate(3 * sizeof(GLfloat));
    QVector3D lightPos(DEFAULT_LIGHT_POSITION);
    vboLight.write(0, &lightPos, 3 * sizeof(GLfloat));
    vboLight.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initRoomMemory()
{
    if(!cubeObject)
    {
        cubeObject = new UnitCube;
    }

    if(vboRoom.isCreated())
    {
        vboRoom.destroy();
    }

    if(iboRoom.isCreated())
    {
        iboRoom.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for cube
    vboRoom.create();
    vboRoom.bind();
    vboRoom.allocate(2 * cubeObject->getVertexOffset() + cubeObject->getTexCoordOffset());
    vboRoom.write(0, cubeObject->getVertices(), cubeObject->getVertexOffset());
    vboRoom.write(cubeObject->getVertexOffset(), cubeObject->getNegativeNormals(),
                  cubeObject->getVertexOffset());
    vboRoom.write(2 * cubeObject->getVertexOffset(), cubeObject->getTexureCoordinates(1.0f),
                  cubeObject->getTexCoordOffset());
    vboRoom.release();
    // indices
    iboRoom.create();
    iboRoom.bind();
    iboRoom.allocate(cubeObject->getIndices(), cubeObject->getIndexOffset());
    iboRoom.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initCubeMemory()
{
    if(!cubeObject)
    {
        cubeObject = new UnitCube;
    }

    if(vboCube.isCreated())
    {
        vboCube.destroy();
    }

    if(iboCube.isCreated())
    {
        iboCube.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for cube
    vboCube.create();
    vboCube.bind();
    vboCube.allocate(2 * cubeObject->getVertexOffset() + cubeObject->getTexCoordOffset());
    vboCube.write(0, cubeObject->getVertices(), cubeObject->getVertexOffset());
    vboCube.write(cubeObject->getVertexOffset(), cubeObject->getNormals(),
                  cubeObject->getVertexOffset());
    vboCube.write(2 * cubeObject->getVertexOffset(), cubeObject->getTexureCoordinates(1.0f),
                  cubeObject->getTexCoordOffset());
    vboCube.release();
    // indices
    iboCube.create();
    iboCube.bind();
    iboCube.allocate(cubeObject->getIndices(), cubeObject->getIndexOffset());
    iboCube.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initMeshObjectMemory()
{
//    qDebug() << QString(QDir::currentPath())+QString("/../obj/teapot.obj");
    if(!objLoader)
    {
        objLoader = new OBJLoader;
    }

    bool result = false;

    switch (currentMeshObject)
    {
    case TEAPOT_OBJ:
        result = objLoader->loadObjFile(":/obj/teapot.obj");
        break;

    case BUNNY_OBJ:
        result = objLoader->loadObjFile(":/obj/bunny.obj");
        break;

    case DUCK_OBJ:
        result = objLoader->loadObjFile(":/obj/duck.obj");
        break;

    case MICKEY_OBJ:
        result = objLoader->loadObjFile(":/obj/mickey.obj");
        break;

    default:
        break;
    }

    if(!result)
    {
        QMessageBox::critical(NULL, "Error", "Could not load OBJ file!");
        return;
    }

    if(vboMeshObject.isCreated())
    {
        vboMeshObject.destroy();
    }

    if(iboMeshObject.isCreated())
    {
        iboMeshObject.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for sphere
    vboMeshObject.create();
    vboMeshObject.bind();
    vboMeshObject.allocate(2 * objLoader->getVertexOffset() +
                           objLoader->getTexCoordOffset());
    vboMeshObject.write(0, objLoader->getVertices(), objLoader->getVertexOffset());
    vboMeshObject.write(objLoader->getVertexOffset(), objLoader->getNormals(),
                        objLoader->getVertexOffset());
    vboMeshObject.write(2 * objLoader->getVertexOffset(), objLoader->getTexureCoordinates(),
                        objLoader->getTexCoordOffset());
    vboMeshObject.release();
    // indices
    iboMeshObject.create();
    iboMeshObject.bind();
    iboMeshObject.allocate(objLoader->getIndices(), objLoader->getIndexOffset());
    iboMeshObject.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initBillboardMemory()
{
    if(!planeObject)
    {
        planeObject = new UnitPlane;
    }

    if(vboBillboard.isCreated())
    {
        vboBillboard.destroy();
    }

    if(iboBillboard.isCreated())
    {
        iboBillboard.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for billboard object
    vboBillboard.create();
    vboBillboard.bind();
    vboBillboard.allocate(2 * planeObject->getVertexOffset() +
                          planeObject->getTexCoordOffset());
    vboBillboard.write(0, planeObject->getVertices(), planeObject->getVertexOffset());
    vboBillboard.write(planeObject->getVertexOffset(), planeObject->getNormals(),
                       planeObject->getVertexOffset());
    vboBillboard.write(2 * planeObject->getVertexOffset(),
                       planeObject->getTexureCoordinates(1.0f),
                       planeObject->getTexCoordOffset());
    vboBillboard.release();
    // indices
    iboBillboard.create();
    iboBillboard.bind();
    iboBillboard.allocate(planeObject->getIndices(), planeObject->getIndexOffset());
    iboBillboard.release();
}

//------------------------------------------------------------------------------------------
#define MAX_NUM_SHADOW_VOLUME_VERTICES 1024

void Renderer::initShadowVolumeMemory()
{
    if(vboShadowVolume.isCreated())
    {
        vboShadowVolume.destroy();
    }


    ////////////////////////////////////////////////////////////////////////////////
    // init memory for shadow volume object
    vboShadowVolume.create();
    vboShadowVolume.bind();
    vboShadowVolume.allocate(3 * MAX_NUM_SHADOW_VOLUME_VERTICES * sizeof(GLfloat));
    vboShadowVolume.release();

    for(int i = 0; i < cubeObject->getNumFaceTriangles(); ++i)
    {
        occluderFaces[i] = new UnitCube::CubeFaceTriangle;
    }
}

//------------------------------------------------------------------------------------------
// record the buffer state by vertex array object
//------------------------------------------------------------------------------------------
void Renderer::initVertexArrayObjects()
{
    initLightVAO();

    initRoomVAO(GOURAUD_SHADING);
    initRoomVAO(PHONG_SHADING);
    initRoomVAO(SHADOW_MAP_SHADING);

    initCubeVAO(GOURAUD_SHADING);
    initCubeVAO(PHONG_SHADING);
    initCubeVAO(PROJECTED_OBJECT_SHADING);
    initCubeVAO(SHADOW_MAP_SHADING);

    initMeshObjectVAO(GOURAUD_SHADING);
    initMeshObjectVAO(PHONG_SHADING);
    initMeshObjectVAO(PROJECTED_OBJECT_SHADING);
    initMeshObjectVAO(SHADOW_MAP_SHADING);

    initBillboardVAO(GOURAUD_SHADING);
    initBillboardVAO(PHONG_SHADING);
    initBillboardVAO(SHADOW_MAP_SHADING);

    initShadowVolumeVAO();
}

//------------------------------------------------------------------------------------------
void Renderer::initLightVAO()
{
    if(vaoLight.isCreated())
    {
        vaoLight.destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[LIGHT_SHADING];

    vaoLight.create();
    vaoLight.bind();

    vboLight.bind();
    program->enableAttributeArray(attrVertex[LIGHT_SHADING]);
    program->setAttributeBuffer(attrVertex[LIGHT_SHADING], GL_FLOAT, 0, 3);

    // release vao before vbo and ibo
    vaoLight.release();
    vboLight.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initRoomVAO(ShadingProgram _shadingMode)
{
    if(vaoRoom[_shadingMode].isCreated())
    {
        vaoRoom[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoRoom[_shadingMode].create();
    vaoRoom[_shadingMode].bind();

    vboRoom.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                cubeObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * cubeObject->getVertexOffset(), 2);

    iboRoom.bind();

    // release vao before vbo and ibo
    vaoRoom[_shadingMode].release();
    vboRoom.release();
    iboRoom.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initCubeVAO(ShadingProgram _shadingMode)
{
    if(vaoCube[_shadingMode].isCreated())
    {
        vaoCube[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoCube[_shadingMode].create();
    vaoCube[_shadingMode].bind();

    vboCube.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    if(_shadingMode == GOURAUD_SHADING || _shadingMode == PHONG_SHADING)
    {
        program->enableAttributeArray(attrNormal[_shadingMode]);
        program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                    cubeObject->getVertexOffset(), 3);

        program->enableAttributeArray(attrTexCoord[_shadingMode]);
        program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                    2 * cubeObject->getVertexOffset(), 2);
    }

    iboCube.bind();

    // release vao before vbo and ibo
    vaoCube[_shadingMode].release();
    vboCube.release();
    iboCube.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initMeshObjectVAO(ShadingProgram _shadingMode)
{
    if(vaoMeshObject[_shadingMode].isCreated())
    {
        vaoMeshObject[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoMeshObject[_shadingMode].create();
    vaoMeshObject[_shadingMode].bind();

    vboMeshObject.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    if(_shadingMode == GOURAUD_SHADING || _shadingMode == PHONG_SHADING)
    {
        program->enableAttributeArray(attrNormal[_shadingMode]);
        program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                    objLoader->getVertexOffset(), 3);

        program->enableAttributeArray(attrTexCoord[_shadingMode]);
        program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                    2 * objLoader->getVertexOffset(), 2);
    }

    iboMeshObject.bind();

    // release vao before vbo and ibo
    vaoMeshObject[_shadingMode].release();
    vboMeshObject.release();
    iboMeshObject.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initBillboardVAO(ShadingProgram _shadingMode)
{
    if(vaoBillboard[_shadingMode].isCreated())
    {
        vaoBillboard[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoBillboard[_shadingMode].create();
    vaoBillboard[_shadingMode].bind();

    vboBillboard.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    if(_shadingMode == GOURAUD_SHADING || _shadingMode == PHONG_SHADING)
    {
        program->enableAttributeArray(attrNormal[_shadingMode]);
        program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                    planeObject->getVertexOffset(), 3);
    }

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * planeObject->getVertexOffset(), 2);

    iboBillboard.bind();

    // release vao before vbo and ibo
    vaoBillboard[_shadingMode].release();
    vboBillboard.release();
    iboBillboard.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initShadowVolumeVAO()
{
    if(vaoShadowVolume.isCreated())
    {
        vaoShadowVolume.destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[SHADOW_VOLUME_SHADING];

    vaoShadowVolume.create();
    vaoShadowVolume.bind();

    vboShadowVolume.bind();
    program->enableAttributeArray(attrVertex[SHADOW_VOLUME_SHADING]);
    program->setAttributeBuffer(attrVertex[SHADOW_VOLUME_SHADING], GL_FLOAT, 0, 3);

    vaoShadowVolume.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initSceneMatrices()
{
    /////////////////////////////////////////////////////////////////
    // room
    setRoomSize(10);

    /////////////////////////////////////////////////////////////////
    // cube
    cubeModelMatrix.setToIdentity();
    cubeModelMatrix.scale(1.5);
    cubeModelMatrix.translate(DEFAULT_CUBE_POSITION);
    cubeNormalMatrix = QMatrix4x4(cubeModelMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // mesh object
    TRUE_OR_DIE(objLoader, "OBJLoader must be initialized first");
    meshObjectModelMatrix.setToIdentity();
    meshObjectModelMatrix.translate(DEFAULT_MESH_OBJECT_POSITION);

    if(currentMeshObject != TEAPOT_OBJ)
        meshObjectModelMatrix.translate(QVector3D(0, -2.0f * objLoader->getLowestYCoordinate(),
                                                  0));

    meshObjectModelMatrix.scale(2.0f / objLoader->getScalingFactor());

    if(currentMeshObject == TEAPOT_OBJ)
    {
        meshObjectModelMatrix.rotate(-90, 1, 0, 0);
    }

    meshObjectNormalMatrix = QMatrix4x4(meshObjectModelMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // billboard object
    billboardObjectModelMatrix.setToIdentity();
    billboardObjectModelMatrix.scale(4.0f);
    billboardObjectModelMatrix.translate(DEFAULT_BILLBOARD_OBJECT_POSITION);

    /////////////////////////////////////////////////////////////////
    // occluder object
    occluderModelMatrix.setToIdentity();
    occluderModelMatrix.translate(DEFAULT_OCCLUDER_POSITION);
    occluderModelMatrix.scale(0.5f);
    occluderNormalMatrix = QMatrix4x4(occluderModelMatrix.normalMatrix());

    calculateOccluderFaceVertices();
}

//------------------------------------------------------------------------------------------
void Renderer::initDepthBufferObject()
{
    if(depthTexture)
    {
        depthTexture->destroy();
        delete depthTexture;
    }

    depthTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    depthTexture->create();
    depthTexture->setSize(DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
    depthTexture->setFormat(QOpenGLTexture::D32);
    depthTexture->allocateStorage();
    depthTexture->setMinificationFilter(QOpenGLTexture::Linear);
    depthTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    depthTexture->setWrapMode(QOpenGLTexture::DirectionS,
                              QOpenGLTexture::ClampToEdge);
    depthTexture->setWrapMode(QOpenGLTexture::DirectionT,
                              QOpenGLTexture::ClampToEdge);

    depthTexture->bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    // frame buffer
    FBODepthMap = new QOpenGLFramebufferObject(DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
    FBODepthMap->bind();
//    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
//                         dTex, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         depthTexture->textureId(), 0);
    TRUE_OR_DIE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "Framebuffer is imcomplete!");
    FBODepthMap->release();

    // shadow matrix
    lightProjectionMatrix.setToIdentity();
    lightProjectionMatrix.perspective(90, 1.0f, 0.1f, 100.0f);
    lightViewMatrix.setToIdentity();
    lightViewMatrix.lookAt(QVector3D(light.position), QVector3D(0, 0, 0), QVector3D(0, 0,
                                                                                    -1));
    shadowMatrix = lightProjectionMatrix * lightViewMatrix;
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    shadowMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    initializedDepthBuffer = true;
}

//------------------------------------------------------------------------------------------
void Renderer::setRoomSize(int _roomSize)
{
    if(isValid())
    {
        makeCurrent();
    }

    roomSize = (float)_roomSize;
    roomModelMatrix.setToIdentity();
    roomModelMatrix.scale(roomSize);
    roomModelMatrix.translate(0.0, 1.0, 0.0);
    roomNormalMatrix = QMatrix4x4(roomModelMatrix.normalMatrix());

    vboRoom.bind();
    vboRoom.write(2 * cubeObject->getVertexOffset(),
                  cubeObject->getTexureCoordinates(roomSize),
                  cubeObject->getTexCoordOffset());
    vboRoom.release();
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::setAmbientLight(int _ambientLight)
{
    ambientLight = (float) _ambientLight / 100.0f;
}

//------------------------------------------------------------------------------------------
void Renderer::setLightIntensity(int _intensity)
{
    if(!isValid())
    {
        return;
    }

    makeCurrent();
    light.intensity = (GLfloat)_intensity / 100.0f;
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 &light, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::resetObjectPositions()
{
    makeCurrent();
    initSceneMatrices();
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::resetLightPosition()
{
    makeCurrent();
    light.position = DEFAULT_LIGHT_POSITION;
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 &light, GL_STREAM_DRAW);

    lightViewMatrix.setToIdentity();
    lightViewMatrix.lookAt(QVector3D(light.position), QVector3D(0, 0, 0), QVector3D(0, 0,
                                                                                    -1));
    shadowMatrix = lightProjectionMatrix * lightViewMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    shadowMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::setMeshObject(int _objectIndex)
{
    if(!isValid())
    {
        return;
    }

    if(_objectIndex < 0 || _objectIndex >= NUM_MESH_OBJECT)
    {
        return;
    }

    currentMeshObject = static_cast<MeshObject>(_objectIndex);
    makeCurrent();
    initMeshObjectMemory();
    initMeshObjectVAO(GOURAUD_SHADING);
    initMeshObjectVAO(PHONG_SHADING);
    initMeshObjectVAO(PROJECTED_OBJECT_SHADING);
    initMeshObjectVAO(SHADOW_MAP_SHADING);

    resetObjectPositions();

    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::setFloorTexture(FloorTexture _texture)
{
    currentFloorTexture = _texture;
}

//------------------------------------------------------------------------------------------
void Renderer::setFloorTextureFilteringMode(QOpenGLTexture::Filter
                                            _textureFiltering)
{
    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        floorTextures[i]->setMinMagFilters(_textureFiltering, _textureFiltering);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::setRoomColor(float _r, float _g, float _b)
{
    if(!isValid())
    {
        return;
    }

    roomMaterial.setDiffuse(QVector4D(_r, _g, _b, 1.0f));
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBORoomMaterial);
    glBufferData(GL_UNIFORM_BUFFER, roomMaterial.getStructSize(),
                 &roomMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::setCubeColor(float _r, float _g, float _b)
{
    if(!isValid())
    {
        return;
    }

    cubeMaterial.setDiffuse(QVector4D(_r, _g, _b, 1.0f));
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBOCubeMaterial);
    glBufferData(GL_UNIFORM_BUFFER, cubeMaterial.getStructSize(),
                 &cubeMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::setMeshObjectColor(float _r, float _g, float _b)
{
    if(!isValid())
    {
        return;
    }

    meshObjectMaterial.setDiffuse(QVector4D(_r, _g, _b, 1.0f));
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMeshObjectMaterial);
    glBufferData(GL_UNIFORM_BUFFER, meshObjectMaterial.getStructSize(),
                 &meshObjectMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::setOccluderColor(float _r, float _g, float _b)
{
    if(!isValid())
    {
        return;
    }

    occluderMaterial.setDiffuse(QVector4D(_r, _g, _b, 1.0f));
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBOOccluderMaterial);
    glBufferData(GL_UNIFORM_BUFFER, occluderMaterial.getStructSize(),
                 &occluderMaterial, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::updateCamera()
{
    zoomCamera();

    /////////////////////////////////////////////////////////////////
    // flush camera data to uniform buffer
    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPosition, cameraFocus, cameraUpDirection);

    viewProjectionMatrix = projectionMatrix * viewMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    viewProjectionMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
QSize Renderer::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize Renderer::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void Renderer::initializeGL()
{
    initializeOpenGLFunctions();
    checkOpenGLVersion();

    if(!initializedScene)
    {
        initScene();
        initializedScene = true;
    }
}

//------------------------------------------------------------------------------------------
void Renderer::resizeGL(int w, int h)
{
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45, (float)w / (float)h, 0.1f, 1000.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::paintGL()
{
    if(!initializedScene)
    {
        return;
    }

    switch(currentMouseTransTarget)
    {
    case TRANSFORM_CAMERA:
    {
        translateCamera();
        rotateCamera();
    }
    break;

    case TRANSFORM_LIGHT:
    {
        translateLight();
    }
    break;

    case TRANSFORM_OBJECTS:
    {
        translateObjects();
        rotateObjects();
    }
    break;

    case TRANSFORM_OCCLUDER:
    {
        translateOccluder();
        rotateOccluder();
        calculateOccluderFaceVertices();
    }
    break;
    }


    updateCamera();


    // render scene
    renderScene();

}

//-----------------------------------------------------------------------------------------
void Renderer::mousePressEvent(QMouseEvent* _event)
{
    lastMousePos = QVector2D(_event->localPos());

    if(_event->button() == Qt::RightButton)
    {
        mouseButtonPressed = RIGHT_BUTTON;
    }
    else
    {
        mouseButtonPressed = LEFT_BUTTON;

        if(currentMouseTransTarget == TRANSFORM_LIGHT)
        {
            mouseButtonPressed = RIGHT_BUTTON;
        }
    }
}

//-----------------------------------------------------------------------------------------
void Renderer::mouseMoveEvent(QMouseEvent* _event)
{
    QVector2D mouseMoved = QVector2D(_event->localPos()) - lastMousePos;

    switch(specialKeyPressed)
    {
    case Renderer::NO_KEY:
    {

        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            translation.setX(translation.x() + mouseMoved.x() / 50.0f);
            translation.setY(translation.y() - mouseMoved.y() / 50.0f);
        }
        else
        {
            rotation.setX(rotation.x() - mouseMoved.x() / 5.0f);
            rotation.setY(rotation.y() - mouseMoved.y() / 5.0f);
        }

    }
    break;

    case Renderer::SHIFT_KEY:
    {
        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            QVector2D dir = mouseMoved.normalized();
            zooming += mouseMoved.length() * dir.x() / 500.0f;
        }
        else
        {
            rotation.setX(rotation.x() + mouseMoved.x() / 5.0f);
            rotation.setZ(rotation.z() + mouseMoved.y() / 5.0f);
        }
    }
    break;

    case Renderer::CTRL_KEY:
        break;

    }

    lastMousePos = QVector2D(_event->localPos());
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::mouseReleaseEvent(QMouseEvent* _event)
{
    mouseButtonPressed = NO_BUTTON;
}

//------------------------------------------------------------------------------------------
void Renderer::wheelEvent(QWheelEvent* _event)
{
    if(!_event->angleDelta().isNull())
    {
        zooming +=  (_event->angleDelta().x() + _event->angleDelta().y()) / 500.0f;
    }

    update();
}

//------------------------------------------------------------------------------------------
void Renderer::setShadingMode(ShadingProgram _shadingMode)
{
    currentShadingMode = _shadingMode;
    currentShadingProgram = glslPrograms[currentShadingMode];

    update();
}

//------------------------------------------------------------------------------------------
void Renderer::resetCameraPosition()
{
    cameraPosition = DEFAULT_CAMERA_POSITION;
    cameraFocus = DEFAULT_CAMERA_FOCUS;
    cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);

    update();
}

//------------------------------------------------------------------------------------------
void Renderer::enableDepthTest(bool _status)
{
    if(!isValid())
    {
        return;
    }

    makeCurrent();

    if(_status)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    doneCurrent();
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::enableZAxisRotation(bool _status)
{
    enabledZAxisRotation = _status;

    if(!enabledZAxisRotation)
    {
        cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::enableTextureAnisotropicFiltering(bool _state)
{
    enabledTextureAnisotropicFiltering = _state;
}

//------------------------------------------------------------------------------------------
void Renderer::enableShowShadowVolume(bool _state)
{
    enabledShowShadowVolume = _state;
}

//------------------------------------------------------------------------------------------
void Renderer::setMouseTransformationTarget(MouseTransformationTarget _mouseTarget)
{
    currentMouseTransTarget = _mouseTarget;
}

//------------------------------------------------------------------------------------------
void Renderer::setShadowMethod(ShadowModes _shadowMode)
{
    currentShadowMode = _shadowMode;
}

//------------------------------------------------------------------------------------------
void Renderer::keyPressEvent(QKeyEvent* _event)
{
    switch(_event->key())
    {
    case Qt::Key_Shift:
        specialKeyPressed = Renderer::SHIFT_KEY;
        break;

    case Qt::Key_Plus:
        zooming -= 0.1f;
        break;

    case Qt::Key_Minus:
        zooming += 0.1f;
        break;

    case Qt::Key_Up:
        translation += QVector3D(0.0f, 0.3f, 0.0f);
        break;

    case Qt::Key_Down:
        translation -= QVector3D(0.0f, 0.3f, 0.0f);
        break;

    case Qt::Key_Left:
        translation -= QVector3D(0.3f, 0.0f, 0.0f);
        break;

    case Qt::Key_Right:
        translation += QVector3D(0.3f, 0.0f, 0.0f);
        break;

    default:
        QOpenGLWidget::keyPressEvent(_event);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::keyReleaseEvent(QKeyEvent* _event)
{
    specialKeyPressed = Renderer::NO_KEY;
}

//------------------------------------------------------------------------------------------
void Renderer::translateCamera()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.01f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    cameraPosition -= scale * (translation.x() * v + translation.y() * u);
    cameraFocus -= scale * (translation.x() * v + translation.y() * u);

}

//------------------------------------------------------------------------------------------
void Renderer::rotateCamera()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;

    float scale = sqrt(nEyeVector.length()) * 0.02f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0),
                                                          rotation.y() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z() * scale);
    nEyeVector = qRotation.rotatedVector(nEyeVector);

    cameraPosition = cameraFocus + nEyeVector;

    if(enabledZAxisRotation)
    {
        cameraUpDirection = qRotation.rotatedVector(cameraUpDirection);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::zoomCamera()
{
    zooming *= MOVING_INERTIA;

    if(fabs(zooming) < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;
    float len = nEyeVector.length();
    nEyeVector.normalize();

    len += sqrt(len) * zooming * 0.3f;

    if(len < 0.5f)
    {
        len = 0.5f;
    }

    cameraPosition = len * nEyeVector + cameraFocus;

}

//------------------------------------------------------------------------------------------
void Renderer::translateObjects()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.05f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    QVector3D objectTrans = scale * (translation.x() * v + translation.y() * u);
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.translate(objectTrans);

    cubeModelMatrix = translationMatrix * cubeModelMatrix;
    meshObjectModelMatrix = translationMatrix * meshObjectModelMatrix;
}

//------------------------------------------------------------------------------------------
void Renderer::rotateObjects()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D currentPos(0.0f, 0.0f, 0.0f);
    currentPos = cubeModelMatrix * currentPos;

    float scale = -0.2f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f),
                                                          rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), rotation.y() * scale);
    //*
    //                      QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z()*scale);

    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(qRotation);

    QMatrix4x4 invTranslationMatrix, translationMatrix;
    invTranslationMatrix.setToIdentity();
    invTranslationMatrix.translate(-1.0f * currentPos);
    translationMatrix.setToIdentity();
    translationMatrix.translate(currentPos);

    cubeModelMatrix = translationMatrix * rotationMatrix * invTranslationMatrix *
                      cubeModelMatrix;
    meshObjectModelMatrix = translationMatrix * rotationMatrix *
                            invTranslationMatrix *
                            meshObjectModelMatrix;

    cubeNormalMatrix = QMatrix4x4(cubeModelMatrix.normalMatrix());
    meshObjectNormalMatrix = QMatrix4x4(meshObjectModelMatrix.normalMatrix());
}

//------------------------------------------------------------------------------------------
void Renderer::translateLight()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.05f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    QVector3D objectTrans = scale * (translation.x() * v + translation.y() * u);
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.translate(objectTrans);

    light.position = translationMatrix * light.position;
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 &light, GL_STREAM_DRAW);

    lightViewMatrix.setToIdentity();
    lightViewMatrix.lookAt(QVector3D(light.position), QVector3D(0, 0, 0), QVector3D(0, 0,
                                                                                    -1));
    shadowMatrix = lightProjectionMatrix * lightViewMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 3 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    shadowMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
void Renderer::calculateOccluderFaceVertices()
{
    for(int i = 0; i < cubeObject->getNumFaceTriangles(); ++i)
    {
        UnitCube::CubeFaceTriangle* face = occluderFaces[i];
        face->faceNormal = occluderNormalMatrix * cubeObject->getFace(i).faceNormal;

        for(int j = 0; j < 3; ++j)
        {
            QVector4D transformedVertex = occluderModelMatrix *
                                          QVector4D((cubeObject->getFace(i)).vertices[j], 1.0);
            face->vertices[j] = QVector3D(transformedVertex);
            face->indices[j] = cubeObject->getFace(i).indices[j];
        }
    }
}

//------------------------------------------------------------------------------------------
void Renderer::translateOccluder()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.05f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    QVector3D objectTrans = scale * (translation.x() * v + translation.y() * u);
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.translate(objectTrans);

    occluderModelMatrix = translationMatrix * occluderModelMatrix;
}

//------------------------------------------------------------------------------------------
void Renderer::rotateOccluder()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D currentPos(0.0f, 0.0f, 0.0f);
    currentPos = occluderModelMatrix * currentPos;

    float scale = -0.2f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f),
                                                          rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), rotation.y() * scale);
    //*
    //                      QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z()*scale);

    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(qRotation);

    QMatrix4x4 invTranslationMatrix, translationMatrix;
    invTranslationMatrix.setToIdentity();
    invTranslationMatrix.translate(-1.0f * currentPos);
    translationMatrix.setToIdentity();
    translationMatrix.translate(currentPos);

    occluderModelMatrix = translationMatrix * rotationMatrix * invTranslationMatrix *
                          occluderModelMatrix;
    occluderNormalMatrix = QMatrix4x4(occluderModelMatrix.normalMatrix());
}

//------------------------------------------------------------------------------------------
void Renderer::renderTestScene()
{
    if(!initializedTestScene)
    {
        initTestScene();
        return;
    }

    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(testProgram->programId());
    glBindVertexArray (vao);
    // draw points 0-3 from the currently bound VAO with current in-use shader
    glDrawArrays (GL_TRIANGLES, 0, 3);
    glUseProgram(0);
}

//------------------------------------------------------------------------------------------
void Renderer::renderScene()
{
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    glClearColor(0.8f, 0.8f, 0.8f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderLight();

    switch(currentShadowMode)
    {
    case NO_SHADOW:
        renderObjectWithoutShadow(ALL_LIGHT);
        break;

    case PROJECTIVE_SHADOW:
        renderObjectWithProjectiveShadow();
        break;

    case SHADOW_MAP:
        renderObjectWithShadowMap();
        break;

    case SHADOW_VOLUME:
        renderObjectWithShadowVolume();
        break;

    default:
        break;
    }

}

//------------------------------------------------------------------------------------------
void Renderer::renderObjectWithoutShadow(int _lightingMode)
{
    currentShadingProgram->bind();
    currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                           cameraPosition);
    currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
    currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
    currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_FALSE);
    currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
    currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode],
                                           _lightingMode);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    renderRoom();
    renderCube();
    renderMeshObject();
    renderOccluder();
    renderBillboardObject();

    currentShadingProgram->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderObjectWithProjectiveShadow()
{
    renderLight();

    QVector4D planeNormals[6] =
    {
        QVector4D(0, 0, -1, roomSize),
        QVector4D(-1, 0, 0, roomSize),
        QVector4D(0, 0, 1, roomSize),
        QVector4D(1, 0, 0, roomSize),
        QVector4D(0, 1, 0, 0),
        QVector4D(0, -1, 0, 2 * roomSize)
    };

    for(int i = 0; i < 4; ++i)
    {

        ////////////////////////////////////////////////////////////////////////////////
        // render the 4 faces of the room
        currentShadingProgram->bind();
        currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                               cameraPosition);
        currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
        currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
        currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_FALSE);
        currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_FALSE);
        currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
        currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode], 0);

        glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                              UBOBindingIndex[BINDING_MATRICES]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                         UBOMatrices);

        glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                              UBOBindingIndex[BINDING_LIGHT]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                         UBOLight);

        glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                              UBOBindingIndex[BINDING_ROOM_MATERIAL]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_ROOM_MATERIAL],
                         UBORoomMaterial);

        /////////////////////////////////////////////////////////////////
        // flush the model and normal matrices
        glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                        roomModelMatrix.constData());
        glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                        roomNormalMatrix.constData());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        vaoRoom[currentShadingMode].bind();

        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_ZERO, GL_ZERO, GL_INCR);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(6 * i * sizeof(GLushort) ));
        glDisable(GL_CULL_FACE);


        vaoRoom[currentShadingMode].release();
        currentShadingProgram->release();


        ////////////////////////////////////////////////////////////////////////////////
        // render projected shadow
        glDisable(GL_DEPTH_TEST);
        glStencilFunc(GL_EQUAL, 1, 0xFF);

        glEnable(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        projectedShadowProgram->bind();

        /////////////////////////////////////////////////////////////////
        // set the uniform
        glUniformBlockBinding(projectedShadowProgram->programId(),
                              uniMatrices[PROJECTED_OBJECT_SHADING],
                              UBOBindingIndex[BINDING_MATRICES]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                         UBOMatrices);

        glUniformBlockBinding(projectedShadowProgram->programId(),
                              uniLight[PROJECTED_OBJECT_SHADING],
                              UBOBindingIndex[BINDING_LIGHT]);
        glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                         UBOLight);

        projectedShadowProgram->setUniformValue(uniShadowIntensity,
                                                (GLfloat)(1.0 - ambientLight));
        projectedShadowProgram->setUniformValue(uniPlaneVector, planeNormals[i]);

        renderProjectedCube();
        renderProjectedMeshObject();
        renderProjectedOccluder();

        projectedShadowProgram->release();
        glDisable(GL_BLEND);

    }


    /////////////////////////////////////////////////////////////////
    // render the floor
    currentShadingProgram->bind();
    currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                           cameraPosition);
    currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
    currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
    currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_FALSE);
    currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
    currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode], 0);


    glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_ROOM_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_ROOM_MATERIAL],
                     UBORoomMaterial);

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    roomModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    roomNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoRoom[currentShadingMode].bind();

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_ZERO, GL_ZERO, GL_INCR);

    glEnable(GL_DEPTH_TEST);
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_TRUE);
    floorTextures[currentFloorTexture]->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * 24));
    floorTextures[currentFloorTexture]->release();

    vaoRoom[currentShadingMode].release();
    currentShadingProgram->release();


    ////////////////////////////////////////////////////////////////////////////////
    // render projected shadow
    glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    projectedShadowProgram->bind();

    /////////////////////////////////////////////////////////////////
    // set the uniform
    glUniformBlockBinding(projectedShadowProgram->programId(),
                          uniMatrices[PROJECTED_OBJECT_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(projectedShadowProgram->programId(),
                          uniLight[PROJECTED_OBJECT_SHADING],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    projectedShadowProgram->setUniformValue(uniShadowIntensity,
                                            (GLfloat)(1.0 - ambientLight));
    projectedShadowProgram->setUniformValue(uniPlaneVector, planeNormals[4]);

    renderProjectedCube();
    renderProjectedMeshObject();
    renderProjectedOccluder();

    projectedShadowProgram->release();

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);


    /////////////////////////////////////////////////////////////////
    // render the ceiling
    currentShadingProgram->bind();
    currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                           cameraPosition);
    currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
    currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
    currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_FALSE);
    currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
    currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode], 0);


    glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_ROOM_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_ROOM_MATERIAL],
                     UBORoomMaterial);

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    roomModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    roomNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoRoom[currentShadingMode].bind();

    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_ZERO, GL_ZERO, GL_INCR);

    glEnable(GL_DEPTH_TEST);
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_TRUE);
    ceilingTexture->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * 30));
    glDisable(GL_CULL_FACE);
    ceilingTexture->release();



    vaoRoom[currentShadingMode].release();
    currentShadingProgram->release();


    ////////////////////////////////////////////////////////////////////////////////
    // render projected shadow
    glDisable(GL_DEPTH_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);

    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    projectedShadowProgram->bind();

    /////////////////////////////////////////////////////////////////
    // set the uniform
    glUniformBlockBinding(projectedShadowProgram->programId(),
                          uniMatrices[PROJECTED_OBJECT_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(projectedShadowProgram->programId(),
                          uniLight[PROJECTED_OBJECT_SHADING],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    projectedShadowProgram->setUniformValue(uniShadowIntensity,
                                            (GLfloat)(1.0 - ambientLight));
    projectedShadowProgram->setUniformValue(uniPlaneVector, planeNormals[5]);
    projectedShadowProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_FALSE);

    renderProjectedCube();
    renderProjectedMeshObject();
    renderProjectedOccluder();

    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);

    ////////////////////////////////////////////////////////////////////////////////
    // render the shadow casting
    currentShadingProgram->bind();
    currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                           cameraPosition);
    currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
    currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
    currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_FALSE);
    currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
    currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode], 0);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    renderCube();
    renderMeshObject();
    renderOccluder();
    renderBillboardObject();
    currentShadingProgram->release();
}

//------------------------------------------------------------------------------------------
void Renderer::generateShadowMap()
{
    /////////////////////////////////////////////////////////////////
    // render scene to shadow map
    FBODepthMap->bind();
    glViewport(0, 0, DEPTH_TEXTURE_SIZE, DEPTH_TEXTURE_SIZE);
    glDrawBuffer(GL_NONE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.0f, 4.0f);

    shadowMapProgram->bind();
    glUniformBlockBinding(shadowMapProgram->programId(), uniMatrices[SHADOW_MAP_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    renderCube2DepthMap();
    renderMeshObject2DepthMap();
    renderOccluder2DepthMap();
    renderBillboardObject2DepthMap();

    glDisable(GL_POLYGON_OFFSET_FILL);
    shadowMapProgram->release();

    FBODepthMap->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderObjectWithShadowMap()
{
    if(!initializedDepthBuffer)
    {
        initDepthBufferObject();
    }

    generateShadowMap();

    /////////////////////////////////////////////////////////////////
    // render scene with shadow map
    makeCurrent();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    renderLight();

    currentShadingProgram->bind();
    currentShadingProgram->setUniformValue(uniCameraPosition[currentShadingMode],
                                           cameraPosition);
    currentShadingProgram->setUniformValue(uniObjTexture[currentShadingMode], 0);
    currentShadingProgram->setUniformValue(uniDepthTexture[currentShadingMode], 1);
    currentShadingProgram->setUniformValue(uniHasDepthTexture[currentShadingMode], GL_TRUE);
    currentShadingProgram->setUniformValue(uniAmbientLight[currentShadingMode], ambientLight);
    currentShadingProgram->setUniformValue(uniLightingMode[currentShadingMode],
                                           (int) ALL_LIGHT);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMatrices[currentShadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentShadingProgram->programId(), uniLight[currentShadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    depthTexture->bind(1);

    renderRoom();
    renderCube();
    renderMeshObject();
    renderOccluder();
    renderBillboardObject();

    depthTexture->release();
    currentShadingProgram->release();
}


//------------------------------------------------------------------------------------------
void Renderer::generateShadowVolume()
{
    UnitCube::CubeFaceTriangle* face1;
    UnitCube::CubeFaceTriangle* face2;
    QVector3D lightDir1, lightDir2;
    QVector3D sharedVertices[2];
    float dot1, dot2;
    shadowVolume.clear();

    for(int i = 0; i < cubeObject->getNumFaceTriangles(); ++i)
    {
        face1 = occluderFaces[i];
        lightDir1 = QVector3D(light.position) - face1->vertices[0];
        dot1 = QVector3D::dotProduct(face1->faceNormal, lightDir1);

        for(int j = 0; j < cubeObject->getNumFaceTriangles(); ++j)
        {
            if(i == j)
            {
                continue;
            }

            face2 = occluderFaces[j];
            lightDir2 = QVector3D(light.position) - face2->vertices[0];
            dot2 = QVector3D::dotProduct(face2->faceNormal, lightDir2);

            if(dot1 * dot2 < 0) // silhouette
            {
                if(dot1 < 0)
                {
                    if(face1->findSharedVertices(face2->vertices, sharedVertices))
                    {
                        // first triangle
                        shadowVolume.append(sharedVertices[0]);
                        shadowVolume.append(sharedVertices[1]);
                        shadowVolume.append(sharedVertices[0] + 100 * (sharedVertices[0] - QVector3D(
                                                                           light.position)));

                        // second triangle
                        shadowVolume.append(sharedVertices[0] + 100 * (sharedVertices[0] - QVector3D(
                                                                           light.position)));
                        shadowVolume.append(sharedVertices[1]);
                        shadowVolume.append(sharedVertices[1] + 100 * (sharedVertices[1] - QVector3D(
                                                                           light.position)));

                    }
                }
                else
                {
                    if(face2->findSharedVertices(face1->vertices, sharedVertices))
                    {
                        // first triangle
                        shadowVolume.append(sharedVertices[0]);
                        shadowVolume.append(sharedVertices[1]);
                        shadowVolume.append(sharedVertices[0] + 100 * (sharedVertices[0] - QVector3D(
                                                                           light.position)));

                        // second triangle
                        shadowVolume.append(sharedVertices[0] + 100 * (sharedVertices[0] - QVector3D(
                                                                           light.position)));
                        shadowVolume.append(sharedVertices[1]);
                        shadowVolume.append(sharedVertices[1] + 100 * (sharedVertices[1] - QVector3D(
                                                                           light.position)));
                    }

                }

            }
        }


        vboShadowVolume.bind();
        vboShadowVolume.write(0, shadowVolume.constData(),
                              sizeof(GLfloat) * 3 * shadowVolume.size());
        vboShadowVolume.release();
    }

}

//------------------------------------------------------------------------------------------
void Renderer::renderShadowVolume()
{
    shadowVolumeProgram->bind();
    glUniformBlockBinding(shadowVolumeProgram->programId(),
                          uniMatrices[SHADOW_VOLUME_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    vaoShadowVolume.bind();
    glDrawArrays(GL_TRIANGLES, 0, shadowVolume.size());
    vaoShadowVolume.release();
    shadowVolumeProgram->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderObjectWithShadowVolume()
{
    generateShadowVolume();
    renderObjectWithoutShadow(AMBIENT_LIGHT);

    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glStencilMask(0xFF);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    renderShadowVolume();
    glCullFace(GL_FRONT);
    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    renderShadowVolume();
    glDisable(GL_CULL_FACE);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glStencilMask(0xFF);
    glStencilFunc(GL_EQUAL, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    if(enabledShowShadowVolume)
    {
        glDisable(GL_CULL_FACE);
        glDisable(GL_STENCIL_TEST);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        renderShadowVolume();
        glEnable(GL_STENCIL_TEST);
    }

    glEnable(GL_STENCIL_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_ONE, GL_ONE);
    renderObjectWithoutShadow(DIFFUSE_SPECULAR);


    glDisable(GL_BLEND);
    glDisable(GL_STENCIL_TEST);
}

//------------------------------------------------------------------------------------------
void Renderer::renderLight()
{
    if(!vaoLight.isCreated())
    {
        qDebug() << "vaoLight is not created!";
        return;
    }

    QOpenGLShaderProgram* program = glslPrograms[LIGHT_SHADING];

    program->bind();

    /////////////////////////////////////////////////////////////////
    // set the uniform
    glUniformBlockBinding(program->programId(), uniMatrices[LIGHT_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(program->programId(), uniLight[LIGHT_SHADING],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);
    program->setUniformValue("pointDistance",
                             (cameraPosition - cameraFocus).length());

    vaoLight.bind();
    glEnable (GL_POINT_SPRITE);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable (GL_DEPTH_TEST);
    glDrawArrays(GL_POINTS, 0, 1);
    glDisable(GL_POINT_SPRITE);

    vaoLight.release();
    program->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderRoom()
{
    if(!vaoRoom[currentShadingMode].isCreated())
    {
        qDebug() << "vaoRoom is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    roomModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    roomNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_ROOM_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_ROOM_MATERIAL],
                     UBORoomMaterial);

    /////////////////////////////////////////////////////////////////
    // render the floor
    vaoRoom[currentShadingMode].bind();

    // 4 sides
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_FALSE);
    glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_SHORT, 0);
    glDisable(GL_CULL_FACE);


    // floor
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_TRUE);
    floorTextures[currentFloorTexture]->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }


    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * 24));
    floorTextures[currentFloorTexture]->release();

    // ceiling
    ceilingTexture->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (GLvoid*)(sizeof(GLushort) * 30));
    glDisable(GL_CULL_FACE);
    ceilingTexture->release();

    vaoRoom[currentShadingMode].release();
}


//------------------------------------------------------------------------------------------
void Renderer::renderRoom2DepthMap()
{
    if(!vaoRoom[currentShadingMode].isCreated())
    {
        qDebug() << "vaoRoom is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    roomModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    vaoRoom[SHADOW_MAP_SHADING].bind();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    glDisable(GL_CULL_FACE);
    vaoRoom[SHADOW_MAP_SHADING].release();
}


//------------------------------------------------------------------------------------------
void Renderer::renderCube()
{
    if(!vaoCube[currentShadingMode].isCreated())
    {
        qDebug() << "vaoCube is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    cubeModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    cubeNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_TRUE);
    currentShadingProgram->setUniformValue("discardTransparentPixel", GL_FALSE);
    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_CUBE_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_CUBE_MATERIAL],
                     UBOCubeMaterial);

    /////////////////////////////////////////////////////////////////
    // render the cube
    vaoCube[currentShadingMode].bind();
    decalTexture->bind(0);
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    decalTexture->release();
    vaoCube[currentShadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderProjectedCube()
{
    if(!vaoCube[PROJECTED_OBJECT_SHADING].isCreated())
    {
        qDebug() << "vaoCube is not created!";
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    cubeModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoCube[PROJECTED_OBJECT_SHADING].bind();
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoCube[PROJECTED_OBJECT_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderCube2DepthMap()
{
    if(!vaoCube[SHADOW_MAP_SHADING].isCreated())
    {
        qDebug() << "vaoCube is not created!";
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    cubeModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoCube[SHADOW_MAP_SHADING].bind();
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoCube[SHADOW_MAP_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderMeshObject()
{
    if(!vaoMeshObject[currentShadingMode].isCreated())
    {
        qDebug() << "vaoMeshObject is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    meshObjectModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    meshObjectNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_FALSE);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_MESH_OBJECT_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MESH_OBJECT_MATERIAL],
                     UBOMeshObjectMaterial);

    /////////////////////////////////////////////////////////////////
    // render the mesh object
    vaoMeshObject[currentShadingMode].bind();
//    meshObjectTexture->bind(0);
    glDrawElements(GL_TRIANGLES, objLoader->getNumIndices(), GL_UNSIGNED_SHORT, 0);
//    meshObjectTexture->release();
    vaoMeshObject[currentShadingMode].release();


}

//------------------------------------------------------------------------------------------
void Renderer::renderProjectedMeshObject()
{
    if(!vaoMeshObject[PROJECTED_OBJECT_SHADING].isCreated())
    {
        qDebug() << "vaoMeshObject is not created!";
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    meshObjectModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoMeshObject[PROJECTED_OBJECT_SHADING].bind();
    glDrawElements(GL_TRIANGLES, objLoader->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoMeshObject[PROJECTED_OBJECT_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderMeshObject2DepthMap()
{
    if(!vaoMeshObject[SHADOW_MAP_SHADING].isCreated())
    {
        qDebug() << "vaoMeshObject is not created!";
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    meshObjectModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    vaoMeshObject[SHADOW_MAP_SHADING].bind();
    glDrawElements(GL_TRIANGLES, objLoader->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoMeshObject[SHADOW_MAP_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderBillboardObject()
{
    if(!vaoBillboard[currentShadingMode].isCreated())
    {
        qDebug() << "vaoBillboardObject is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // rotate the billboard to face the camera
    QVector3D billboardPos = DEFAULT_BILLBOARD_OBJECT_POSITION;
    QVector3D cameraDir = cameraPosition - cameraFocus;
    float angle = atan2(billboardPos.x() - cameraDir.x(), billboardPos.z() - cameraDir.z()) ;

    QMatrix4x4 rotationMatrix = billboardObjectModelMatrix;
    rotationMatrix.rotate(angle * 180 / M_PI, QVector3D(0.0f, 1.0f, 0.0f));
    rotationMatrix.rotate(90, QVector3D(1.0f, 0.0f, 0.0f));
    QMatrix4x4 normalMatrix = -QMatrix4x4(rotationMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    rotationMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    normalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_TRUE);
    currentShadingProgram->setUniformValue("discardTransparentPixel", GL_TRUE);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_BILLBOARD_OBJECT_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_BILLBOARD_OBJECT_MATERIAL],
                     UBOBillboardObjectMaterial);

    /////////////////////////////////////////////////////////////////
    // render the billboard
    vaoBillboard[currentShadingMode].bind();
    billboardTexture->bind(0);
//    glEnable(GL_BLEND);
//    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawElements(GL_TRIANGLES, planeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
//    glDisable(GL_BLEND);
    billboardTexture->release();
    vaoBillboard[currentShadingMode].release();


}

//------------------------------------------------------------------------------------------
void Renderer::renderBillboardObject2DepthMap()
{
    if(!vaoBillboard[SHADOW_MAP_SHADING].isCreated())
    {
        qDebug() << "vaoBillboardObject is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // rotate the billboard to face the camera
    QVector3D billboardPos = DEFAULT_BILLBOARD_OBJECT_POSITION;
    QVector3D cameraDir = cameraPosition - cameraFocus;
    float angle = atan2(billboardPos.x() - cameraDir.x(), billboardPos.z() - cameraDir.z()) ;

    QMatrix4x4 rotationMatrix = billboardObjectModelMatrix;
    rotationMatrix.rotate(angle * 180 / M_PI, QVector3D(0.0f, 1.0f, 0.0f));
    rotationMatrix.rotate(90, QVector3D(1.0f, 0.0f, 0.0f));

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    rotationMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    shadowMapProgram->setUniformValue(uniHasObjTexture[SHADOW_MAP_SHADING], GL_TRUE);

    /////////////////////////////////////////////////////////////////
    // render the billboard
    vaoBillboard[SHADOW_MAP_SHADING].bind();
    billboardTexture->bind(0);
    glDrawElements(GL_TRIANGLES, planeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    billboardTexture->release();
    vaoBillboard[SHADOW_MAP_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderOccluder()
{
    if(!vaoCube[currentShadingMode].isCreated())
    {
        qDebug() << "vaoOccluder is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    occluderModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    occluderNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentShadingProgram->setUniformValue(uniHasObjTexture[currentShadingMode], GL_FALSE);

    glUniformBlockBinding(currentShadingProgram->programId(), uniMaterial[currentShadingMode],
                          UBOBindingIndex[BINDING_OCCLUDER_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_OCCLUDER_MATERIAL],
                     UBOOccluderMaterial);

    /////////////////////////////////////////////////////////////////
    // render the occluder
    glDisable(GL_CULL_FACE);
    vaoCube[currentShadingMode].bind();
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoCube[currentShadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderProjectedOccluder()
{
    if(!vaoCube[PROJECTED_OBJECT_SHADING].isCreated())
    {
        qDebug() << "vaoOccluder is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    occluderModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    /////////////////////////////////////////////////////////////////
    // render the cube
    vaoCube[PROJECTED_OBJECT_SHADING].bind();
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoCube[PROJECTED_OBJECT_SHADING].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderOccluder2DepthMap()
{
    if(!vaoCube[SHADOW_MAP_SHADING].isCreated())
    {
        qDebug() << "vaoOccluder is not created!";
        return;
    }

    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    occluderModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    /////////////////////////////////////////////////////////////////
    // render the cube
    vaoCube[SHADOW_MAP_SHADING].bind();
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    vaoCube[SHADOW_MAP_SHADING].release();

}

