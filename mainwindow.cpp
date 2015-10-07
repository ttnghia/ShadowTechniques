//------------------------------------------------------------------------------------------
// mainwindow.cpp
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("[CS6610 - Spring 2015] Assignment 3 - Shader based Shadow Algorithms");

    setupGUI();

    setRoomColor(QColor(5, 115, 255));
    setCubeColor(QColor(0, 255, 50));
    setMeshObjectColor(QColor(170, 85, 0));
    setOccluderColor(QColor(255, 26, 153));

    // Update continuously
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), renderer, SLOT(update()));
    timer->start(10);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch(e->key())
    {
    case Qt::Key_Escape:
        close();
        break;

    case Qt::Key_V:
        changeShadingMode(false);
        break;

    case Qt::Key_R:
        renderer->resetCameraPosition();
        break;

    case Qt::Key_F:
        changeFloorTexture(false);
        break;

    default:
        renderer->keyPressEvent(e);
    }
}


//------------------------------------------------------------------------------------------
void MainWindow::setupGUI()
{
    renderer = new Renderer(this);

    ////////////////////////////////////////////////////////////////////////////////
    // shading modes
    QHBoxLayout* shadingLayout = new QHBoxLayout;


    QRadioButton* rdbGouraudShading = new QRadioButton("Gouraud");
    rdb2ShadingMap[rdbGouraudShading] = GOURAUD_SHADING;
    shadingRDBList.append(rdbGouraudShading);
    shadingLayout->addWidget(rdbGouraudShading);

    QRadioButton* rdbPhongShading = new QRadioButton("Phong");
    rdb2ShadingMap[rdbPhongShading] = PHONG_SHADING;
    shadingRDBList.append(rdbPhongShading);
    rdbPhongShading->setChecked(true);
    shadingLayout->addWidget(rdbPhongShading);

    foreach (QRadioButton * rdbShading, rdb2ShadingMap.keys())
    {
        connect(rdbShading, SIGNAL(clicked(bool)), this, SLOT(changeShadingMode(bool)));
    }

    QGroupBox* shadingGroup = new QGroupBox("Shading Mode");
    shadingGroup->setLayout(shadingLayout);

    ////////////////////////////////////////////////////////////////////////////////
    // floor textures
    QGridLayout* floorTextureLayout = new QGridLayout;

    QRadioButton* rdbWood1 = new QRadioButton("Wood1");
    rdbWood1->setChecked(true);
    rdb2FloorTextureMap[rdbWood1] = WOOD1;
    floorTextureRDBList.append(rdbWood1);
    floorTextureLayout->addWidget(rdbWood1, 0, 0);

    QRadioButton* rdbWood2 = new QRadioButton("Wood2");
    rdb2FloorTextureMap[rdbWood2] = WOOD2;
    floorTextureRDBList.append(rdbWood2);
    floorTextureLayout->addWidget(rdbWood2, 0, 1);


    QRadioButton* rdbCheckerboard1  = new QRadioButton("Checker1");
    rdb2FloorTextureMap[rdbCheckerboard1] = CHECKERBOARD1;
    floorTextureRDBList.append(rdbCheckerboard1);
    floorTextureLayout->addWidget(rdbCheckerboard1, 0, 2);

    QRadioButton* rdbCheckerboard2  = new QRadioButton("Checker2");
    rdb2FloorTextureMap[rdbCheckerboard2] = CHECKERBOARD2;
    floorTextureRDBList.append(rdbCheckerboard2);
    floorTextureLayout->addWidget(rdbCheckerboard2, 1, 0);


    QRadioButton* rdbStone1 = new QRadioButton("Stone1");
    rdb2FloorTextureMap[rdbStone1] = STONE1;
    floorTextureRDBList.append(rdbStone1);
    floorTextureLayout->addWidget(rdbStone1, 1, 1);


    QRadioButton* rdbStone2 = new QRadioButton("Stone2");
    rdb2FloorTextureMap[rdbStone2] = STONE2;
    floorTextureRDBList.append(rdbStone2);
    floorTextureLayout->addWidget(rdbStone2, 1, 2);


    QGroupBox* floorTextureGroup = new QGroupBox("Floor Texture");
    floorTextureGroup->setLayout(floorTextureLayout);

    foreach (QRadioButton * rdbTexture, rdb2FloorTextureMap.keys())
    {
        connect(rdbTexture, SIGNAL(clicked(bool)), this, SLOT(changeFloorTexture(bool)));
    }


    ////////////////////////////////////////////////////////////////////////////////
    // texture filtering mode
    cbTextureFiltering = new QComboBox;
    QString str;

    str = QString("Nearest");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Nearest;

    str = QString("Linear");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Linear;


    str = QString("Nearest_MipMap_Nearest");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapNearest;


    str = QString("Nearest_MipMap_Linear");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapLinear;


    str = QString("Linear_MipMap_Nearest");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapNearest;

    str = QString("Linear_MipMap_Linear");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapLinear;


    QVBoxLayout* textureFilteringLayout = new QVBoxLayout;
    textureFilteringLayout->addWidget(cbTextureFiltering);
    QGroupBox* textureFilteringGroup = new QGroupBox("Floor Texture MinMag Filtering Mode");
    textureFilteringGroup->setLayout(textureFilteringLayout);

    cbTextureFiltering->setCurrentIndex(cbTextureFiltering->count() - 1);
    connect(cbTextureFiltering, SIGNAL(currentIndexChanged(int)), this,
            SLOT(changeTextureFilteringMode()));


    ////////////////////////////////////////////////////////////////////////////////
    // object color picker
    wgRoomColor = new QWidget;
    wgRoomColor->setAutoFillBackground(true);

    wgCubeColor = new QWidget;
    wgCubeColor->setAutoFillBackground(true);

    wgMeshObjectColor = new QWidget;
    wgMeshObjectColor->setAutoFillBackground(true);

    wgOccluderColor = new QWidget;
    wgOccluderColor->setAutoFillBackground(true);


    QPushButton* btnChangeRoomColor = new QPushButton("Change...");
    QPushButton* btnChangeCubeColor = new QPushButton("Change...");
    QPushButton* btnChangeMeshObjectColor = new QPushButton("Change...");
    QPushButton* btnChangeOccluderColor = new QPushButton("Change...");

    connect(btnChangeRoomColor, &QPushButton::clicked, this, &MainWindow::changeRoomColor);
    connect(btnChangeCubeColor, &QPushButton::clicked, this, &MainWindow::changeCubeColor);
    connect(btnChangeMeshObjectColor, &QPushButton::clicked, this,
            &MainWindow::changeMeshObjectColor);
    connect(btnChangeOccluderColor, &QPushButton::clicked, this,
            &MainWindow::changeOccluderColor);


    QGridLayout* objectColorLayout = new QGridLayout;
    objectColorLayout->addWidget(new QLabel("Room:"), 0, 0, Qt::AlignRight);
    objectColorLayout->addWidget(wgRoomColor, 0, 1, 1, 2);
    objectColorLayout->addWidget(btnChangeRoomColor, 0, 3);
    objectColorLayout->addWidget(new QLabel("Cube:"), 1, 0, Qt::AlignRight);
    objectColorLayout->addWidget(wgCubeColor, 1, 1, 1, 2);
    objectColorLayout->addWidget(btnChangeCubeColor, 1, 3);
    objectColorLayout->addWidget(new QLabel("Mesh Object:"), 2, 0, Qt::AlignRight);
    objectColorLayout->addWidget(wgMeshObjectColor, 2, 1, 1, 2);
    objectColorLayout->addWidget(btnChangeMeshObjectColor, 2, 3);
    objectColorLayout->addWidget(new QLabel("Occluder Object:"), 3, 0, Qt::AlignRight);
    objectColorLayout->addWidget(wgOccluderColor, 3, 1, 1, 2);
    objectColorLayout->addWidget(btnChangeOccluderColor, 3, 3);

    QGroupBox* objectColorGroup = new QGroupBox("Objects' Color");
    objectColorGroup->setLayout(objectColorLayout);

    ////////////////////////////////////////////////////////////////////////////////
    // mesh object
    cbMeshObject = new QComboBox;

    str = QString("Teapot");
    cbMeshObject->addItem(str);

    str = QString("Bunny");
    cbMeshObject->addItem(str);

    str = QString("Duck");
    cbMeshObject->addItem(str);

    str = QString("Mickey mouse");
    cbMeshObject->addItem(str);



    QVBoxLayout* meshObjectLayout = new QVBoxLayout;
    meshObjectLayout->addWidget(cbMeshObject);
    QGroupBox* meshObjectGroup = new QGroupBox("Mesh Object");
    meshObjectGroup->setLayout(meshObjectLayout);

    cbMeshObject->setCurrentIndex(0);
    connect(cbMeshObject, SIGNAL(currentIndexChanged(int)), renderer,
            SLOT(setMeshObject(int)));


    ////////////////////////////////////////////////////////////////////////////////
    // room size
    sldRoomSize = new QSlider(Qt::Horizontal);
    sldRoomSize->setMinimum(4);
    sldRoomSize->setMaximum(100);
    sldRoomSize->setValue(10);

    connect(sldRoomSize, &QSlider::valueChanged, renderer,
            &Renderer::setRoomSize);

    QVBoxLayout* roomSizeLayout = new QVBoxLayout;
    roomSizeLayout->addWidget(sldRoomSize);
    QGroupBox* roomSizeGroup = new QGroupBox("Room Size");
    roomSizeGroup->setLayout(roomSizeLayout);


    ////////////////////////////////////////////////////////////////////////////////
    // light intensity
    QSlider* sldLightIntensity = new QSlider(Qt::Horizontal);
    sldLightIntensity->setMinimum(0);
    sldLightIntensity->setMaximum(100);
    sldLightIntensity->setValue(70);

    connect(sldLightIntensity, &QSlider::valueChanged, renderer,
            &Renderer::setLightIntensity);

    QVBoxLayout* lightIntensityLayout = new QVBoxLayout;
    lightIntensityLayout->addWidget(sldLightIntensity);
    QGroupBox* lightIntensityGroup = new QGroupBox("Point Light Intensity");
    lightIntensityGroup->setLayout(lightIntensityLayout);

    ////////////////////////////////////////////////////////////////////////////////
    // ambient light
    QSlider* sldAmbientLight = new QSlider(Qt::Horizontal);
    sldAmbientLight->setMinimum(0);
    sldAmbientLight->setMaximum(100);
    sldAmbientLight->setValue(40);

    connect(sldAmbientLight, &QSlider::valueChanged, renderer,
            &Renderer::setAmbientLight);

    QVBoxLayout* ambientLightLayout = new QVBoxLayout;
    ambientLightLayout->addWidget(sldAmbientLight);
    QGroupBox* ambientLightGroup = new QGroupBox("Ambient Light Intensity");
    ambientLightGroup->setLayout(ambientLightLayout);


    ////////////////////////////////////////////////////////////////////////////////
    // shadow generation
    QRadioButton* rdbNoShadow = new QRadioButton("No Shadow");
    rdb2ShadowMethodMap[rdbNoShadow] = NO_SHADOW;
    rdbNoShadow->setChecked(true);

    QRadioButton* rdbProjectiveShadow = new QRadioButton("Projective Shadow");
    rdb2ShadowMethodMap[rdbProjectiveShadow] = PROJECTIVE_SHADOW;

    QRadioButton* rdbShadowMap = new QRadioButton("Shadow Map");
    rdb2ShadowMethodMap[rdbShadowMap] = SHADOW_MAP;

    QRadioButton* rdbShadowVolume = new QRadioButton("Shadow Volume");
    rdb2ShadowMethodMap[rdbShadowVolume] = SHADOW_VOLUME;

    chkShowShadowVolume = new QCheckBox("Show Shadow Volume");

    QGridLayout* shadowLayout = new QGridLayout;
    shadowLayout->addWidget(rdbNoShadow, 0, 0);
    shadowLayout->addWidget(rdbProjectiveShadow, 0, 1);
    shadowLayout->addWidget(rdbShadowMap, 1, 0);
    shadowLayout->addWidget(rdbShadowVolume, 1, 1);
    shadowLayout->addWidget(chkShowShadowVolume, 2, 0, 1, 2);

    QGroupBox* shadowGroup = new QGroupBox("Shadow Generation");
    shadowGroup->setLayout(shadowLayout);


    connect(rdbNoShadow, &QRadioButton::toggled, this,
            &MainWindow::changeShadowMethod);
    connect(rdbProjectiveShadow, &QRadioButton::toggled, this,
            &MainWindow::changeShadowMethod);
    connect(rdbShadowMap, &QRadioButton::toggled, this,
            &MainWindow::changeShadowMethod);
    connect(rdbShadowVolume, &QRadioButton::toggled, this,
            &MainWindow::changeShadowMethod);
    connect(chkShowShadowVolume, &QCheckBox::toggled, renderer,
            &Renderer::enableShowShadowVolume);
    chkShowShadowVolume->setEnabled(false);

    ////////////////////////////////////////////////////////////////////////////////
    // mouse drag transformation
    QRadioButton* rdbMoveCamera;
    QRadioButton* rdbMoveCubeWithMeshObject;
    QRadioButton* rdbMoveLight;
    QRadioButton* rdbMoveOccluder;

    rdbMoveCamera = new QRadioButton("Camera");
    rdbMoveCamera->setChecked(true);
    rdb2MouseTransTargetMap[rdbMoveCamera] = TRANSFORM_CAMERA;
    connect(rdbMoveCamera, &QRadioButton::clicked, this,
            &MainWindow::changeMouseTransformTarget);

    rdbMoveCubeWithMeshObject = new QRadioButton(
        QString::fromUtf8("Cube && Mesh Object"));
    rdb2MouseTransTargetMap[rdbMoveCubeWithMeshObject] = TRANSFORM_OBJECTS;
    connect(rdbMoveCubeWithMeshObject, &QRadioButton::clicked, this,
            &MainWindow::changeMouseTransformTarget);

    rdbMoveLight = new QRadioButton( QString::fromUtf8("Light Source"));
    rdbMoveLight->setChecked(false);
    rdb2MouseTransTargetMap[rdbMoveLight] = TRANSFORM_LIGHT;
    connect(rdbMoveLight, &QRadioButton::clicked, this,
            &MainWindow::changeMouseTransformTarget);

    rdbMoveOccluder = new QRadioButton(
        QString::fromUtf8("Cube Occluder"));
    rdb2MouseTransTargetMap[rdbMoveOccluder] = TRANSFORM_OCCLUDER;
    rdbMoveOccluder->setChecked(false);
    connect(rdbMoveOccluder, &QRadioButton::clicked, this,
            &MainWindow::changeMouseTransformTarget);

    QGridLayout* mouseTransformationTargetLayout = new QGridLayout;
    mouseTransformationTargetLayout->addWidget(rdbMoveCamera, 0, 0);
    mouseTransformationTargetLayout->addWidget(rdbMoveCubeWithMeshObject, 0, 1);
    mouseTransformationTargetLayout->addWidget(rdbMoveLight, 1, 0);
    mouseTransformationTargetLayout->addWidget(rdbMoveOccluder, 1, 1);

    QGroupBox* mouseTransformationTargetGroup = new QGroupBox("Mouse Transformation Target");
    mouseTransformationTargetGroup->setLayout(mouseTransformationTargetLayout);


    ////////////////////////////////////////////////////////////////////////////////
    // others
    QCheckBox* chkEnableZAxisRotation = new QCheckBox("Enable Z Axis Rotation");
    chkEnableZAxisRotation->setChecked(false);
    connect(chkEnableZAxisRotation, &QCheckBox::toggled, renderer,
            &Renderer::enableZAxisRotation);

    QPushButton* btnResetObjects = new QPushButton("Reset Object Positions");
    connect(btnResetObjects, SIGNAL(clicked()), this,
            SLOT(resetObjectPositions()));

    QPushButton* btnResetCamera = new QPushButton("Reset Camera");
    connect(btnResetCamera, &QPushButton::clicked, renderer,
            &Renderer::resetCameraPosition);


    ////////////////////////////////////////////////////////////////////////////////
    // Add slider group to parameter group
    QVBoxLayout* parameterLayout = new QVBoxLayout;
    parameterLayout->addWidget(shadingGroup);
    parameterLayout->addWidget(floorTextureGroup);
    parameterLayout->addWidget(textureFilteringGroup);
    parameterLayout->addWidget(meshObjectGroup);
    parameterLayout->addWidget(objectColorGroup);
    parameterLayout->addWidget(roomSizeGroup);
    parameterLayout->addWidget(lightIntensityGroup);
    parameterLayout->addWidget(ambientLightGroup);
    parameterLayout->addWidget(shadowGroup);
    parameterLayout->addWidget(mouseTransformationTargetGroup);
    parameterLayout->addWidget(chkEnableZAxisRotation);

    parameterLayout->addWidget(btnResetObjects);
    parameterLayout->addWidget(btnResetCamera);


    parameterLayout->addStretch();

    QGroupBox* parameterGroup = new QGroupBox;
    parameterGroup->setFixedWidth(300);
    parameterGroup->setLayout(parameterLayout);

    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(renderer);
    hLayout->addWidget(parameterGroup);

    setLayout(hLayout);

}

//------------------------------------------------------------------------------------------
void MainWindow::changeShadingMode(bool _mouseClicked)
{
    QRadioButton* rdbShading;
    QRadioButton* rdbNextShading;

    for(int i = 0; i < shadingRDBList.size(); ++i)
    {
        rdbShading = shadingRDBList.at(i);

        if(rdbShading->isChecked())
        {
            if(!_mouseClicked)
            {
                rdbNextShading = shadingRDBList.at((i + 1) % shadingRDBList.size());
                rdbNextShading->setChecked(true);
                renderer->setShadingMode(rdb2ShadingMap.value(rdbNextShading));
            }
            else
            {
                renderer->setShadingMode(rdb2ShadingMap.value(rdbShading));
            }

            break;
        }
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeFloorTexture(bool _mouseClicked)
{
    QRadioButton* rdbTexture;
    QRadioButton* rdbNextTexture;

    for(int i = 0; i < floorTextureRDBList.size(); ++i)
    {
        rdbTexture = floorTextureRDBList.at(i);

        if(rdbTexture->isChecked())
        {
            if(!_mouseClicked)
            {
                rdbNextTexture = floorTextureRDBList.at((i + 1) % floorTextureRDBList.size());
                rdbNextTexture->setChecked(true);
                renderer->setFloorTexture(rdb2FloorTextureMap.value(rdbNextTexture));
            }
            else
            {
                renderer->setFloorTexture(rdb2FloorTextureMap.value(rdbTexture));
            }

            break;
        }
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeTextureFilteringMode()
{
    QOpenGLTexture::Filter filterMode =
        str2TextureFilteringMap[cbTextureFiltering->currentText()];
    renderer->setFloorTextureFilteringMode(filterMode);
}

//------------------------------------------------------------------------------------------
void MainWindow::resetObjectPositions()
{
    sldRoomSize->setValue(10);
    renderer->resetObjectPositions();
    renderer->resetLightPosition();
}

//------------------------------------------------------------------------------------------
void MainWindow::changeRoomColor()
{
    QColor color = QColorDialog::getColor(Qt::green, this);

    if(color.isValid())
    {
        setRoomColor(color);
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeCubeColor()
{
    QColor color = QColorDialog::getColor(Qt::green, this);

    if(color.isValid())
    {
        setCubeColor(color);
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeMeshObjectColor()
{
    QColor color = QColorDialog::getColor(Qt::green, this);

    if(color.isValid())
    {
        setMeshObjectColor(color);
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeOccluderColor()
{
    QColor color = QColorDialog::getColor(Qt::green, this);

    if(color.isValid())
    {
        setOccluderColor(color);
    }
}

//------------------------------------------------------------------------------------------
void MainWindow::changeMouseTransformTarget(bool _state)
{
    if(!_state)
    {
        return;
    }

    QRadioButton* rdbTransformTarget = qobject_cast<QRadioButton*>(QObject::sender());

    if(!rdbTransformTarget)
    {
        return;
    }

    renderer->setMouseTransformationTarget(rdb2MouseTransTargetMap.value(rdbTransformTarget));

}

//------------------------------------------------------------------------------------------
void MainWindow::changeShadowMethod(bool _state)
{
    if(!_state)
    {
        return;
    }

    QRadioButton* rdbShadowMethod = qobject_cast<QRadioButton*>(QObject::sender());

    if(!rdbShadowMethod)
    {
        return;
    }

    chkShowShadowVolume->setEnabled(rdb2ShadowMethodMap[rdbShadowMethod] == SHADOW_VOLUME);

    renderer->setShadowMethod(rdb2ShadowMethodMap[rdbShadowMethod]);
}

//------------------------------------------------------------------------------------------
void MainWindow::setRoomColor(QColor _color)
{
    QPalette palette = wgRoomColor->palette();
    palette.setColor(QPalette::Window, _color);
    wgRoomColor->setPalette(palette);
    renderer->setRoomColor((float) _color.red() / 255.0f, (float) _color.green() / 255.0f,
                           (float) _color.blue() / 255.0f);
}

//------------------------------------------------------------------------------------------
void MainWindow::setCubeColor(QColor _color)
{
    QPalette palette = wgCubeColor->palette();
    palette.setColor(QPalette::Window, _color);
    wgCubeColor->setPalette(palette);
    renderer->setCubeColor((float) _color.red() / 255.0f, (float) _color.green() / 255.0f,
                           (float) _color.blue() / 255.0f);
}

//------------------------------------------------------------------------------------------
void MainWindow::setMeshObjectColor(QColor _color)
{
    QPalette palette = wgMeshObjectColor->palette();
    palette.setColor(QPalette::Window, _color);
    wgMeshObjectColor->setPalette(palette);
    renderer->setMeshObjectColor((float) _color.red() / 255.0f,
                                 (float) _color.green() / 255.0f,
                                 (float) _color.blue() / 255.0f);
}

//------------------------------------------------------------------------------------------
void MainWindow::setOccluderColor(QColor _color)
{
    QPalette palette = wgOccluderColor->palette();
    palette.setColor(QPalette::Window, _color);
    wgOccluderColor->setPalette(palette);
    renderer->setOccluderColor((float) _color.red() / 255.0f,
                               (float) _color.green() / 255.0f,
                               (float) _color.blue() / 255.0f);
}

