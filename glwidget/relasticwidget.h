// relasticwidget.h
#ifndef RELASTICWIDGET_H
#define RELASTICWIDGET_H

#include "baseglwidget.h"
#include <QVector3D>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <vector>
#include <string>

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
    
    // 导出变换后的模型
    bool exportTransformedOBJ(const QString& filePath);
    
    // 重载基类方法
    void loadOBJ(const QString &path);
    void resetView();
    void centerView();
    void paintGL();

signals:
    void modelLoadedChanged(bool loaded);  // 添加这个信号

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void updateBuffersFromOpenMesh();

private:
    // 相对论参数
    Mesh openMesh_measure;
    Mesh openMesh_look;
    QVector3D velocity;          // 速度 (以光速为单位)
    float gamma;                 // 洛伦兹因子
    int visualizationMode;       // 0:原始模型, 1:测量形象, 2:视觉形象
    float stretchFactor;         // 拉伸因子
    
    // 原始模型数据
    std::vector<float> originalVertices; // 原始顶点数据
    QVector3D originalMin, originalMax, originalCenter, originalSize;
    float originalMaxSize;
    
    // 变换后的数据
    std::vector<float> transformedVertices; // 变换后的顶点数据
    std::vector<float> lorentzVertices;     // 洛伦兹变换后的顶点
    
    // 计算洛伦兹因子
    void computeGamma();
    
    // 计算原始包围盒
    void computeOriginalBoundingBox();
    
    // 应用洛伦兹变换（测量形象）
    std::vector<float> applyObserverLorentzTransform(const std::vector<float>& vertices);
    
    // 应用光锥变换（视觉形象）
    std::vector<float> applyLightConeTransform(const std::vector<float>& vertices);
    
    // 准备当前显示的变换数据
    void prepareCurrentTransformation();
    
    // 更新变换后的缓冲区
    void updateTransformedBuffers();
    
    // 重写绘制方法（去掉 override 关键字）
    void drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
};

#endif // RELASTICWIDGET_H