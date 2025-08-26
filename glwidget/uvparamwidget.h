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

class UVParamWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit UVParamWidget(QWidget *parent = nullptr);
    virtual ~UVParamWidget();

    void loadOBJ(const QString &path);
    void clearData();
    bool hasUVData() const { return hasUV; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void parseOBJ(const QString &path);
    void setupSquare();
    void setupUVPoints();

    bool hasUV;
    std::vector<QVector2D> uvCoords;
    std::vector<QVector3D> vertices;
    std::vector<QVector2D> textureCoords;

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

    QMatrix4x4 projection;
    QColor squareColor;
    QColor pointColor;
    QColor lineColor;
    float squareSize;
};

#endif // UVPARAMWIDGET_H