// simplesquarewidget.cpp
#include "simplesquarewidget.h"
#include <QDebug>

SimpleSquareWidget::SimpleSquareWidget(QWidget *parent) : QOpenGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    squareSize(1.0f)
{
    setFocusPolicy(Qt::StrongFocus);
    
    // 设置背景色为深蓝色，与BaseGLWidget一致
    bgColor = QColor(0, 85, 127);
    // 设置正方形颜色为白色
    squareColor = QColor(255, 255, 255, 255);
    
    // 设置抗锯齿格式
    QSurfaceFormat format;
    format.setSamples(4);  // 4倍多重采样
    setFormat(format);
}

SimpleSquareWidget::~SimpleSquareWidget() {
    makeCurrent();
    squareVao.destroy();
    squareVbo.destroy();
    squareEbo.destroy();
    doneCurrent();
}

void SimpleSquareWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void SimpleSquareWidget::setSquareColor(const QColor& color) {
    squareColor = color;
    update();
}

void SimpleSquareWidget::initializeGL() {
    initializeOpenGLFunctions();
    
    // 设置背景颜色
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // 启用多重采样抗锯齿
    glEnable(GL_MULTISAMPLE);

    // Initialize square
    squareVao.create();
    squareVbo.create();
    squareEbo.create();
    
    setupSquare();
}

void SimpleSquareWidget::setupSquare() {
    float vertices[] = {
        -squareSize, -squareSize, 0.0f,
         squareSize, -squareSize, 0.0f,
         squareSize,  squareSize, 0.0f,
        -squareSize,  squareSize, 0.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    squareVao.bind();
    squareVbo.bind();
    squareVbo.allocate(vertices, sizeof(vertices));
    
    squareEbo.bind();
    squareEbo.allocate(indices, sizeof(indices));
    
    squareProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n");
    
    squareProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 squareColor;\n"
        "void main() {\n"
        "   FragColor = squareColor;\n"
        "}\n");
    squareProgram.link();
    
    squareProgram.bind();
    int posLoc = squareProgram.attributeLocation("aPos");
    squareProgram.enableAttributeArray(posLoc);
    squareProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    
    squareVao.release();
}

void SimpleSquareWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    
    // 创建保持宽高比的投影矩阵
    projection.setToIdentity();
    float aspect = static_cast<float>(w) / h;
    if (aspect > 1.0f) {
        projection.ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        projection.ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -1.0f, 1.0f);
    }
}

void SimpleSquareWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Draw square
    squareProgram.bind();
    squareVao.bind();
    squareEbo.bind();
    
    squareProgram.setUniformValue("projection", projection);
    squareProgram.setUniformValue("squareColor", 
                                 QVector4D(squareColor.redF(), squareColor.greenF(), 
                                           squareColor.blueF(), squareColor.alphaF()));
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    squareEbo.release();
    squareVao.release();
    squareProgram.release();
}