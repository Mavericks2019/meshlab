// simplesquarewidget.h
#ifndef SIMPLESQUAREWIDGET_H
#define SIMPLESQUAREWIDGET_H

#include "baseglwidget.h"
#include <vector>
#include <map>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/SparseLU>
#include <Eigen/IterativeLinearSolvers>
#include <cmath>
#include <tuple>

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
    void solveParameterizationFloater();
    
    // 重写参数化相关方法
    void performParameterization(BoundaryType boundaryType = Rectangle, 
                                 ParameterizationMethod method = OriginalMethod) override;
    std::vector<float> getParameterizedVertices() const override { return paramVertices; }
    std::vector<unsigned int> getParameterizedFaces() const override { return paramFaces; }
    bool isParameterized() const override { return parameterized; }
    
    // 设置参数化方法
    void setParameterizationMethod(ParameterizationMethod method) { 
        currentParamMethod = method; 
        qDebug() << "Parameterization method set to:" << method;
    }

    // 添加检测翻转的方法
    int checkForFlips() const;
    std::vector<int> getFlippedTriangles() const { return flippedTriangles; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

public:
    void setupSquare();
    void setupMesh();
    
    // 参数化相关方法
    void mapBoundaryToCircle();
    void mapBoundaryToRectangle();
    void normalizeMesh();
    void solveParameterizationOriginal();
    void solveParameterization();
    
    // 参数化权重计算方法
    std::map<int, float> computeWeightsForVertex(Mesh::VertexHandle vh, ParameterizationMethod method);
    float computeCotangentWeight(Mesh::HalfedgeHandle heh);
    void solveParameterizationInternal(ParameterizationMethod method);
    
    // 数学辅助函数
    float computeAngle(const Mesh::Point& a, const Mesh::Point& b) const;
    float computeVectorAngle(const Mesh::Point& a, const Mesh::Point& b) const;
    std::tuple<float, float, float> computeBarycentric(float px, float py,
                                                      float ax, float ay,
                                                      float bx, float by,
                                                      float cx, float cy) const;

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
    QColor meshColor;
    float squareSize;
    Mesh original;
    
    // 网格数据
    std::vector<float> meshVertices;
    std::vector<unsigned int> meshFaces;
    bool meshLoaded = false;
    
    // 参数化结果
    std::vector<float> paramVertices;
    std::vector<unsigned int> paramFaces;
    bool parameterized = false;
    
    // 当前参数化方法
    ParameterizationMethod currentParamMethod = OriginalMethod;
    
    // 存储翻转的三角形索引
    mutable std::vector<int> flippedTriangles;
    void outputDebugFiles() const;
private:
    // 添加私有辅助方法
    float computeDeterminant2D(float x1, float y1, float x2, float y2, float x3, float y3) const;
};

#endif // SIMPLESQUAREWIDGET_H