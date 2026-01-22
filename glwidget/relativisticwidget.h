// relativisticwidget.h
#ifndef RELATIVISTICWIDGET_H
#define RELATIVISTICWIDGET_H

#include "baseglwidget.h"
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>
#include <vector>

class RelativisticGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit RelativisticGLWidget(QWidget *parent = nullptr);
    virtual ~RelativisticGLWidget();

    void setBackgroundColor(const QColor& color);
    void loadOBJ(const QString &path);
    void resetView();
    
    QColor bgColor = QColor(0, 85, 127);  // 与BaseGLWidget相同的深蓝色背景
    
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
    
    Mesh openMesh;
    std::vector<unsigned int> faces;
    std::vector<unsigned int> edges;
    
    bool modelLoaded = false;
    
    QQuaternion rotation;
    float zoom = 1.0f;
    
    QVector3D modelCenter = QVector3D(-20, 0, 0);  // 左侧中心位置
    
    bool isDragging = false;
    QPoint lastMousePos;
    float rotationSensitivity = 2.0f;
    
    QOpenGLShaderProgram wireframeProgram;
    QOpenGLShaderProgram faceProgram;
    
    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ebo;
    QOpenGLBuffer faceEbo;
};

#endif // RELATIVISTICWIDGET_H