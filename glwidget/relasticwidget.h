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
    
    // 变换后的网格数据
    Mesh transformedMesh;
    std::vector<float> transformedVertices;
    std::vector<float> originalVertices; // 保存原始顶点数据
    
    // 用于绘制变换后网格的缓冲区
    QOpenGLVertexArrayObject transformedVao;
    QOpenGLBuffer transformedVbo;
    QOpenGLBuffer transformedFaceEbo;
    
    QOpenGLShaderProgram transformedProgram;
    
    // 计算洛伦兹因子
    void computeGamma();
    
    // 应用洛伦兹变换到顶点
    QVector3D lorentzTransform(const QVector3D& point) const;
    
    // 更新变换后的缓冲区
    void updateTransformedBuffers();
    
    // 计算包围盒并调整相机
    void adjustCameraForTransformedMesh();
};

#endif // RELASTICWIDGET_H