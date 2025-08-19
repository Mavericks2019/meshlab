// modelglwidget.h
#ifndef MODELGLWIDGET_H
#define MODELGLWIDGET_H

#include "baseglwidget.h"
#include <QOpenGLShaderProgram>

class ModelGLWidget : public BaseGLWidget
{
    Q_OBJECT

public:
    explicit ModelGLWidget(QWidget *parent = nullptr);
    ~ModelGLWidget() override = default;

    void setRenderMode(RenderMode mode) ;
    void updateBuffersFromOpenMesh();
    void calculateCurvatures();
    void drawCurvature(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix);

public:
    void initializeShaders();
    void paintGL() override;
    void loadOBJ(const QString &path);

private:
    QOpenGLShaderProgram curvatureProgram;
};

#endif // MODELGLWIDGET_H