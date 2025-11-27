// simplesquarewidget.h
#ifndef SIMPLESQUAREWIDGET_H
#define SIMPLESQUAREWIDGET_H

#include "baseglwidget.h"
#include <vector>

class SimpleSquareWidget : public BaseGLWidget
{
    Q_OBJECT

public:
    explicit SimpleSquareWidget(QWidget *parent = nullptr);
    virtual ~SimpleSquareWidget();

    void setSquareColor(const QColor& color);
    void setMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& faces);
    void clearMeshData();
    bool hasMeshData() const { return meshLoaded; }

    // 重写参数化相关方法
    void performParameterization(BoundaryType boundaryType = Rectangle) override;
    std::vector<float> getParameterizedVertices() const override { return paramVertices; }
    std::vector<unsigned int> getParameterizedFaces() const override { return paramFaces; }
    bool isParameterized() const override { return parameterized; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void setupSquare();
    void setupMesh();
    
    // 参数化相关方法
    void mapBoundaryToCircle();
    void mapBoundaryToRectangle();
    void normalizeMesh();
    void solveParameterization();

    QOpenGLShaderProgram squareProgram;
    QOpenGLShaderProgram meshProgram;
    QOpenGLBuffer squareVbo;
    QOpenGLBuffer squareEbo;
    QOpenGLBuffer meshVbo;
    QOpenGLBuffer meshEbo;
    QOpenGLVertexArrayObject squareVao;
    QOpenGLVertexArrayObject meshVao;

    QMatrix4x4 projection;
    QColor squareColor;
    QColor meshColor;  // 添加网格颜色成员变量
    float squareSize;

    // 网格数据
    std::vector<float> meshVertices;
    std::vector<unsigned int> meshFaces;
    bool meshLoaded = false;
};

#endif // SIMPLESQUAREWIDGET_H