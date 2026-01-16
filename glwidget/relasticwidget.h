// relasticwidget.h
#ifndef RELASTICWIDGET_H
#define RELASTICWIDGET_H

#include "baseglwidget.h"
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class RelasticGLWidget : public BaseGLWidget
{
    Q_OBJECT

public:
    explicit RelasticGLWidget(QWidget *parent = nullptr);
    ~RelasticGLWidget();

    // 相对论变换相关方法
    void setVelocity(const QVector3D& velocity);
    QVector3D getVelocity() const { return velocity; }
    
    void setVisualizationMode(int mode);
    int getVisualizationMode() const { return visualizationMode; }
    
    void setStretchFactor(float factor);
    float getStretchFactor() const { return stretchFactor; }
    
    // 重载基类方法
    void loadOBJ(const QString &path);
    void resetView();
    void centerView();
    
    // 洛伦兹变换计算
    void applyLorentzTransformation();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void updateBuffersFromOpenMesh();

private:
    // 相对论参数
    QVector3D velocity;          // 速度 (以光速为单位)
    float gamma;                 // 洛伦兹因子
    int visualizationMode;       // 0:原始模型, 1:测量形象, 2:视觉形象
    float stretchFactor;         // 拉伸因子
    
    // 原始模型数据
    std::vector<float> originalVertices; // 原始顶点数据
    std::vector<unsigned int> originalFaces; // 原始面索引
    QVector3D originalMin, originalMax, originalCenter, originalSize;
    float originalMaxSize;
    
    // 拉伸变换后的数据
    std::vector<float> stretchedVertices; // 拉伸后的顶点数据
    
    // 变换后的网格数据
    Mesh transformedMesh;
    
    // 用于绘制变换后网格的缓冲区
    QOpenGLVertexArrayObject transformedVao;
    QOpenGLBuffer transformedVbo;
    QOpenGLBuffer transformedFaceEbo;
    
    // 计算洛伦兹因子
    void computeGamma();
    
    // 计算原始包围盒
    void computeOriginalBoundingBox();
    
    // 计算变换后的包围盒
    void computeTransformedBoundingBox(Mesh::Point& min, Mesh::Point& max);
    
    // 应用拉伸变换
    void applyStretchTransformation();
    
    // 渲染拉伸后的模型
    void renderStretchedModel();
    
    // 更新变换后的缓冲区
    void updateTransformedBuffers();
    
    // 计算包围盒并调整相机
    void adjustCameraForTransformedMesh();
    
    // 重置到原始状态
    void resetToOriginal();
    
    void drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
};

#endif // RELASTICWIDGET_H