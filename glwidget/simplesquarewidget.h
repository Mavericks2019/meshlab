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

class SimpleSquareWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SimpleSquareWidget(QWidget *parent = nullptr);
    virtual ~SimpleSquareWidget();

    void setBackgroundColor(const QColor& color);
    void setSquareColor(const QColor& color);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void setupSquare();

    QOpenGLShaderProgram squareProgram;
    QOpenGLBuffer squareVbo;
    QOpenGLBuffer squareEbo;
    QOpenGLVertexArrayObject squareVao;

    QMatrix4x4 projection;
    QColor squareColor;
    QColor bgColor;
    float squareSize;
};

#endif // SIMPLESQUAREWIDGET_H