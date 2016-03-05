//------------------------------------------------------------------------------------------
//
//
// Created on: 2/20/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------
#include <QtWidgets>
#include "objloader.h"
#include "cyPoint.h"

OBJLoader::OBJLoader():
    objObject(NULL)
{
}

//------------------------------------------------------------------------------------------
bool OBJLoader::loadObjFile(const char* _fileName)
{
    if(!objObject)
    {
        objObject = new cyTriMesh;
    }
    else
    {
        objObject->Clear();
    }

    if(!objObject->LoadFromFileObj(_fileName, false))
    {
        return false;
    }

//    qDebug() << objObject->NV();
    clearData();
    objObject->ComputeNormals();
    objObject->ComputeBoundingBox();
    boxMin = objObject->GetBoundMin();
    boxMax = objObject->GetBoundMax();

    for(int i = 0; i < objObject->NV(); ++i)
    {
        cyPoint3f vertex = objObject->V(i) ;
        verticesList.append(vertex.x);
        verticesList.append(vertex.y);
        verticesList.append(vertex.z);


        cyPoint3f normal = objObject->VN(i);
        normalsList.append(normal.x);
        normalsList.append(normal.y);
        normalsList.append(normal.z);

        if(objObject->NVT() == objObject->NV())
        {
            cyPoint3f texCoor = objObject->VT(i);
            texCoordList.append(texCoor.x);
            texCoordList.append(texCoor.y);
        }
    }

    for(int i = 0; i < objObject->NF(); ++i)
    {
        cyTriMesh::cyTriFace face = objObject->F(i);
        indicesList.append((GLushort)face.v[0]);
        indicesList.append((GLushort)face.v[1]);
        indicesList.append((GLushort)face.v[2]);
    }

    return true;
}

//------------------------------------------------------------------------------------------
OBJLoader::~OBJLoader()
{
    clearData();
    delete objObject;
}

//------------------------------------------------------------------------------------------
int OBJLoader::getNumVertices()
{

    return objObject->NV();
}

//------------------------------------------------------------------------------------------
int OBJLoader::getNumIndices()
{
    return indicesList.size();
}

//------------------------------------------------------------------------------------------
int OBJLoader::getVertexOffset()
{
    return (sizeof(GLfloat) * getNumVertices() * 3);
}

//------------------------------------------------------------------------------------------
int OBJLoader::getIndexOffset()
{
    return (sizeof(GLushort) * getNumIndices());
}

//------------------------------------------------------------------------------------------
float OBJLoader::getScalingFactor()
{
    float diff = fmax(fmax(boxMax.x - boxMin.x, boxMax.y - boxMin.y), boxMax.z - boxMin.z);
    return 0.5f * diff;

}

//------------------------------------------------------------------------------------------
float OBJLoader::getLowestYCoordinate()
{
    return (boxMin.y / getScalingFactor());
}

//------------------------------------------------------------------------------------------
int OBJLoader::getTexCoordOffset()
{
    return (sizeof(GLfloat) * getNumVertices() * 2);
}

//------------------------------------------------------------------------------------------
GLfloat* OBJLoader::getVertices()
{
    return (GLfloat*)verticesList.data();
}

//------------------------------------------------------------------------------------------
GLfloat* OBJLoader::getNormals()
{
    return (GLfloat*)normalsList.data();
}

//------------------------------------------------------------------------------------------
GLfloat* OBJLoader::getTexureCoordinates()
{
    return (GLfloat*)texCoordList.data();
}

//------------------------------------------------------------------------------------------
GLushort* OBJLoader::getIndices()
{
    return (GLushort*)indicesList.data();
}

//------------------------------------------------------------------------------------------
void OBJLoader::clearData()
{
    verticesList.clear();
    normalsList.clear();
    texCoordList.clear();
    indicesList.clear();
}

