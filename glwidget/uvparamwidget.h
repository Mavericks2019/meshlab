// uvparamwidget.h
#ifndef UVPARAMWIDGET_H
#define UVPARAMWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QColor>
#include <vector>
#include <QFile>
#include <QTextStream>
#include <unordered_map>
#include <unordered_set>
#include "../meshutils/my_traits.h"  // 添加头文件

class UVParamWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit UVParamWidget(QWidget *parent = nullptr);
    virtual ~UVParamWidget();

    void loadOBJ(const QString &path);
    void clearData();
    bool hasUVData() const { return hasUV; }
    void setAntialiasing(bool enabled);  // 添加抗锯齿设置函数
    
    // 添加获取mesh信息的函数
    int getVertexCount() const { return mesh.n_vertices(); }
    int getConnectedComponentsCount() const;
    void printMeshInfo() const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void parseOBJ(const QString &path);
    void setupSquare();
    void setupUVPoints();
    void analyzeTopology();
    void setupFaces();

    bool hasUV;
    std::vector<QVector2D> uvCoords;
    std::vector<QVector3D> vertices;
    std::vector<QVector2D> textureCoords;
    
    Mesh mesh;  // 添加Mesh变量

    // 拓扑分析相关
    std::vector<std::vector<int>> faceIndices;
    std::vector<int> faceColors; // 每个面对应的颜色索引
    std::vector<QVector3D> colorPalette; // 颜色调色板

    // OpenGL objects for square
    QOpenGLShaderProgram squareProgram;
    QOpenGLBuffer squareVbo;
    QOpenGLBuffer squareEbo;
    QOpenGLVertexArrayObject squareVao;

    // OpenGL objects for UV points
    QOpenGLShaderProgram uvProgram;
    QOpenGLBuffer uvVbo;
    QOpenGLVertexArrayObject uvVao;

    // OpenGL objects for UV lines
    QOpenGLShaderProgram lineProgram;
    QOpenGLBuffer lineVbo;
    QOpenGLBuffer lineEbo;
    QOpenGLVertexArrayObject lineVao;

    // OpenGL objects for UV faces
    QOpenGLShaderProgram faceProgram;
    QOpenGLBuffer faceVbo;
    QOpenGLBuffer faceColorVbo;
    QOpenGLVertexArrayObject faceVao;

    // 抗锯齿相关
    bool useAntialiasing;
    QOpenGLShaderProgram antialiasedLineProgram;
    QOpenGLShaderProgram antialiasedPointProgram;

    QMatrix4x4 projection;
    QColor squareColor;
    QColor pointColor;
    QColor lineColor;
    float squareSize;
    bool showPoints;
    bool showWireframe;
    bool showFaces;
    int lineVertexCount;
    int faceVertexCount;

public slots:
    void setShowPoints(bool show);
    void setShowWireframe(bool show);
    void setShowFaces(bool show);
};

#endif // UVPARAMWIDGET_H