// relativisticwidget.h
#ifndef RELATIVISTICWIDGET_H
#define RELATIVISTICWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <QColor>
#include <vector>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include "../meshutils/my_traits.h"


class RelativisticGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum RenderMode {
        FlatShading,
        BlinnPhong,
        WireframeOnly,
        FacesOnly
    };

    explicit RelativisticGLWidget(QWidget *parent = nullptr);
    virtual ~RelativisticGLWidget();

    void setBackgroundColor(const QColor& color);
    void loadOBJ(const QString &path);
    void resetView();
    
    // 新增：设置渲染模式
    void setRenderMode(RenderMode mode);
    
    // 新增：设置显示选项
    void setShowWireframeOverlay(bool show);
    void setHideFaces(bool hide);
    
    QColor bgColor = QColor(0, 85, 127);  // 深蓝色背景
    
    // 新增：渲染模式
    RenderMode currentRenderMode = FlatShading;
    bool showWireframeOverlay = true;
    bool hideFaces = false;
    
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    bool loadOBJToOpenMesh(const QString &path);
    void computeBoundingBox(Mesh::Point& min, Mesh::Point& max);
    void prepareFaceIndices();
    void prepareEdgeIndices();
    void updateBuffersFromOpenMesh();
    void scaleAndPositionMesh();
    
    QVector3D projectToTrackball(const QPoint& screenPos);
    void drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawFaces(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    void drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection);
    
    Mesh openMesh;
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
    
    bool modelLoaded = false;
    
    QQuaternion rotation;
    float zoom = 1.0f;
    
    QVector3D modelCenter = QVector3D(-20, 0, 0);  // 左侧中心位置
    
    // 禁用鼠标拖动旋转
    bool isDragging = false;
    QPoint lastMousePos;
    float rotationSensitivity = 0.0f;  // 设置为0禁用旋转
    
    // 光照参数
    QVector3D surfaceColor = QVector3D(0.88f, 0.84f, 0.76f);  // 米白色
    QVector4D wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);  // 红色线框
    bool specularEnabled = true;
    
    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram faceProgram;
    QOpenGLShaderProgram blinnPhongProgram;  // 新增：Blinn-Phong着色器
    
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QOpenGLBuffer faceEbo;
};

#endif // RELATIVISTICWIDGET_H