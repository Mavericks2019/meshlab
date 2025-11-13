// simplesquarewidget.h
#ifndef SIMPLESQUAREWIDGET_H
#define SIMPLESQUAREWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QColor>
#include <vector>

class SimpleSquareWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SimpleSquareWidget(QWidget *parent = nullptr);
    virtual ~SimpleSquareWidget();

    void setBackgroundColor(const QColor& color);
    void setSquareColor(const QColor& color);
    void setMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& faces);
    void clearMeshData();
    bool hasMeshData() const { return meshLoaded; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void setupSquare();
    void setupMesh();

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
    QColor bgColor;
    float squareSize;

    // 网格数据
    std::vector<float> meshVertices;
    std::vector<unsigned int> meshFaces;
    bool meshLoaded = false;
};

#endif // SIMPLESQUAREWIDGET_H