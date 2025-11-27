// simplesquarewidget.cpp
#include "simplesquarewidget.h"
#include <QDebug>

SimpleSquareWidget::SimpleSquareWidget(QWidget *parent) : QOpenGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    meshVbo(QOpenGLBuffer::VertexBuffer),
    meshEbo(QOpenGLBuffer::IndexBuffer),
    squareSize(1.0f)  // 保持为1.0，在resizeGL中动态调整
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
    meshVao.destroy();
    meshVbo.destroy();
    meshEbo.destroy();
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

void SimpleSquareWidget::setMeshData(const std::vector<float>& vertices, const std::vector<unsigned int>& faces) {
    qDebug() << "Setting mesh data - Vertices:" << vertices.size() << "Faces:" << faces.size();
    
    meshVertices = vertices;
    meshFaces = faces;
    meshLoaded = !vertices.empty() && !faces.empty();
    
    qDebug() << "Mesh loaded:" << meshLoaded;
    
    if (meshLoaded) {
        makeCurrent();
        setupMesh();
        doneCurrent();
    }
    
    update();
}

void SimpleSquareWidget::clearMeshData() {
    qDebug() << "Clearing mesh data";
    meshVertices.clear();
    meshFaces.clear();
    meshLoaded = false;
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
    
    // Initialize mesh
    meshVao.create();
    meshVbo.create();
    meshEbo.create();
    
    setupSquare();
    
    if (meshLoaded) {
        setupMesh();
    }
}

void SimpleSquareWidget::setupSquare() {
    // 先移除旧的着色器
    squareProgram.removeAllShaders();
    
    // 使用动态的正方形尺寸，在paintGL中通过投影矩阵控制大小
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  // 左下角
         1.0f, -1.0f, 0.0f,  // 右下角
         1.0f,  1.0f, 0.0f,  // 右上角
        -1.0f,  1.0f, 0.0f   // 左上角
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
    if (posLoc != -1) {
        squareProgram.enableAttributeArray(posLoc);
        squareProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    squareVao.release();
    squareProgram.release();
}

void SimpleSquareWidget::setupMesh() {
    if (meshVertices.empty() || meshFaces.empty()) {
        qDebug() << "Mesh data is empty, skipping setup";
        return;
    }
    
    qDebug() << "Setting up mesh with" << meshVertices.size() << "vertices and" << meshFaces.size() << "faces";
    
    // 先移除旧的着色器
    meshProgram.removeAllShaders();
    
    meshVao.bind();
    meshVbo.bind();
    meshVbo.allocate(meshVertices.data(), meshVertices.size() * sizeof(float));
    
    meshEbo.bind();
    meshEbo.allocate(meshFaces.data(), meshFaces.size() * sizeof(unsigned int));
    
    meshProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos, 1.0);\n"
        "}\n");
    
    meshProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec4 meshColor;\n"
        "void main() {\n"
        "   FragColor = meshColor;\n"
        "}\n");
    
    if (!meshProgram.link()) {
        qDebug() << "Mesh program link failed:" << meshProgram.log();
        return;
    }
    
    meshProgram.bind();
    int posLoc = meshProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        meshProgram.enableAttributeArray(posLoc);
        meshProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    meshVao.release();
    meshProgram.release();
    
    qDebug() << "Mesh setup completed successfully";
}

void SimpleSquareWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    
    // 创建保持宽高比的投影矩阵
    projection.setToIdentity();
    
    if (w > h) {
        // 宽屏 - 正方形适应高度
        float aspect = static_cast<float>(w) / h;
        projection.ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    } else {
        // 竖屏 - 正方形适应宽度
        float aspect = static_cast<float>(h) / w;
        projection.ortho(-1.0f, 1.0f, -aspect, aspect, -1.0f, 1.0f);
    }
}

void SimpleSquareWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (meshLoaded && !meshVertices.empty() && !meshFaces.empty()) {
        // Draw parameterized mesh with fill
        meshProgram.bind();
        meshVao.bind();
        meshEbo.bind();
        
        meshProgram.setUniformValue("projection", projection);
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(0.8f, 0.8f, 0.8f, 1.0f)); // 灰色填充
        
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        
        // Draw wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.5f);
        meshProgram.setUniformValue("meshColor", 
                                   QVector4D(0.0f, 0.0f, 0.0f, 1.0f)); // 黑色线框
        glDrawElements(GL_TRIANGLES, meshFaces.size(), GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        meshEbo.release();
        meshVao.release();
        meshProgram.release();
    } else {
        // Draw square (fallback)
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
}