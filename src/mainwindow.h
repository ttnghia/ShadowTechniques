//------------------------------------------------------------------------------------------
// mainwindow.h
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSPinbox>
#include <QtGui>
#include <QtWidgets>

#include "renderer.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

protected:
    void keyPressEvent(QKeyEvent*);

    void setupGUI();

public slots:
    void changeShadingMode(bool _mouseClicked = true);
    void changeFloorTexture(bool _mouseClicked = true);
    void changeTextureFilteringMode();
    void resetObjectPositions();
    void changeRoomColor();
    void changeCubeColor();
    void changeMeshObjectColor();
    void changeOccluderColor();
    void changeMouseTransformTarget(bool _state);
    void changeShadowMethod(bool _state);

private:
    void setRoomColor(QColor _color);
    void setCubeColor(QColor _color);
    void setMeshObjectColor(QColor _color);
    void setOccluderColor(QColor _color);

    Renderer* renderer;

    QMap<QRadioButton*, ShadingProgram> rdb2ShadingMap;
    QList<QRadioButton*> shadingRDBList;

    QMap<QRadioButton*, FloorTexture> rdb2FloorTextureMap;
    QList<QRadioButton*> floorTextureRDBList;

    QMap<QString, QOpenGLTexture::Filter> str2TextureFilteringMap;
    QComboBox* cbTextureFiltering;
    QComboBox* cbMeshObject;

    QMap<QRadioButton*, ShadowModes> rdb2ShadowMethodMap;
    QCheckBox* chkShowShadowVolume;

    QWidget* wgRoomColor;
    QWidget* wgCubeColor;
    QWidget* wgMeshObjectColor;
    QWidget* wgOccluderColor;
    QSlider* sldRoomSize;

    QMap<QRadioButton*, MouseTransformationTarget> rdb2MouseTransTargetMap;

};

#endif // MAINWINDOW_H
