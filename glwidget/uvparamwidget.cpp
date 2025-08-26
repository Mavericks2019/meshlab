// uvparamwidget.cpp
#include "uvparamwidget.h"
#include <QDebug>
#include <QFileInfo>
#include <QtMath>

UVParamWidget::UVParamWidget(QWidget *parent) : QOpenGLWidget(parent),
    squareVbo(QOpenGLBuffer::VertexBuffer),
    squareEbo(QOpenGLBuffer::IndexBuffer),
    uvVbo(QOpenGLBuffer::VertexBuffer),
    lineVbo(QOpenGLBuffer::VertexBuffer),
    lineEbo(QOpenGLBuffer::IndexBuffer),
    hasUV(false),
    squareSize(1.0f),
    showLines(true),
    lineVertexCount(0)
{
    setFocusPolicy(Qt::StrongFocus);
    
    // Set colors
    squareColor = QColor(100, 100, 100, 100);  // Semi-transparent gray
    pointColor = QColor(255, 0, 0);            // Red points
    lineColor = QColor(0, 0, 255);             // Blue lines
}

UVParamWidget::~UVParamWidget() {
    makeCurrent();
    squareVao.destroy();
    squareVbo.destroy();
    squareEbo.destroy();
    uvVao.destroy();
    uvVbo.destroy();
    lineVao.destroy();
    lineVbo.destroy();
    lineEbo.destroy();
    doneCurrent();
}

void UVParamWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize square
    squareVao.create();
    squareVbo.create();
    squareEbo.create();
    
    setupSquare();

    // Initialize UV points
    uvVao.create();
    uvVbo.create();
    
    uvProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   gl_PointSize = 5.0;\n"
        "}\n");
    
    uvProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 pointColor;\n"
        "void main() {\n"
        "   FragColor = vec4(pointColor, 1.0);\n"
        "}\n");
    uvProgram.link();

    // Initialize lines
    lineVao.create();
    lineVbo.create();
    lineEbo.create();
    
    lineProgram.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "   gl_Position = projection * vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "}\n");
    
    lineProgram.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 lineColor;\n"
        "void main() {\n"
        "   FragColor = vec4(lineColor, 1.0);\n"
        "}\n");
    lineProgram.link();
}

void UVParamWidget::setupSquare() {
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

void UVParamWidget::setupUVPoints() {
    if (uvCoords.empty()) return;
    
    // 转换UV坐标到[-1,1]范围
    std::vector<QVector2D> transformedUV;
    for (const auto& uv : uvCoords) {
        // 将UV从[0,1]映射到[-1,1]
        float x = uv.x() * 2.0f - 1.0f;
        float y = uv.y() * 2.0f - 1.0f;
        transformedUV.emplace_back(x, y);
    }
    
    // Setup points
    uvVao.bind();
    uvVbo.bind();
    uvVbo.allocate(transformedUV.data(), transformedUV.size() * sizeof(QVector2D));
    
    uvProgram.bind();
    int posLoc = uvProgram.attributeLocation("aPos");
    uvProgram.enableAttributeArray(posLoc);
    uvProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    
    uvVao.release();
    
    // Setup lines (connect points based on face indices)
    std::vector<QVector2D> lineVertices;
    for (const auto& face : faceIndices) {
        for (int i = 0; i < face.size(); i++) {
            int idx1 = face[i];
            int idx2 = face[(i + 1) % face.size()];
            if (idx1 < transformedUV.size() && idx2 < transformedUV.size()) {
                lineVertices.push_back(transformedUV[idx1]);
                lineVertices.push_back(transformedUV[idx2]);
            }
        }
    }
    
    lineVao.bind();
    lineVbo.bind();
    lineVbo.allocate(lineVertices.data(), lineVertices.size() * sizeof(QVector2D));
    
    lineProgram.bind();
    posLoc = lineProgram.attributeLocation("aPos");
    lineProgram.enableAttributeArray(posLoc);
    lineProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    
    lineVao.release();
    
    // Store the number of line vertices for drawing
    lineVertexCount = lineVertices.size();
}

void UVParamWidget::resizeGL(int w, int h) {
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

void UVParamWidget::paintGL() {
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
    
    if (hasUV) {
        // Draw lines if enabled
        if (showLines) {
            lineProgram.bind();
            lineVao.bind();
            
            lineProgram.setUniformValue("projection", projection);
            lineProgram.setUniformValue("lineColor", 
                                       QVector3D(lineColor.redF(), lineColor.greenF(), lineColor.blueF()));
            
            glDrawArrays(GL_LINES, 0, lineVertexCount);
            
            lineVao.release();
            lineProgram.release();
        }
        
        // Draw points
        uvProgram.bind();
        uvVao.bind();
        
        uvProgram.setUniformValue("projection", projection);
        uvProgram.setUniformValue("pointColor", 
                                 QVector3D(pointColor.redF(), pointColor.greenF(), pointColor.blueF()));
        
        glDrawArrays(GL_POINTS, 0, uvCoords.size());
        
        uvVao.release();
        uvProgram.release();
    }
}

void UVParamWidget::parseOBJ(const QString &path) {
    clearData();
    
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << path;
        return;
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;
        
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;
        
        QString type = parts[0];
        
        if (type == "v") {  // Vertex
            if (parts.size() >= 4) {
                float x = parts[1].toFloat();
                float y = parts[2].toFloat();
                float z = parts[3].toFloat();
                vertices.emplace_back(x, y, z);
            }
        } else if (type == "vt") {  // Texture coordinate
            if (parts.size() >= 3) {
                float u = parts[1].toFloat();
                float v = parts[2].toFloat();
                textureCoords.emplace_back(u, v);
                hasUV = true;
            }
        } else if (type == "f") {  // Face
            if (hasUV && parts.size() >= 4) {
                std::vector<int> faceTexIndices;
                for (int i = 1; i < parts.size(); i++) {
                    QStringList indices = parts[i].split("/");
                    if (indices.size() >= 2 && !indices[1].isEmpty()) {
                        int texIdx = indices[1].toInt() - 1;  // OBJ indices start at 1
                        if (texIdx >= 0 && texIdx < textureCoords.size()) {
                            uvCoords.push_back(textureCoords[texIdx]);
                            faceTexIndices.push_back(uvCoords.size() - 1);
                        }
                    }
                }
                faceIndices.push_back(faceTexIndices);
            }
        }
    }
    
    file.close();
    
    if (hasUV) {
        makeCurrent();
        setupUVPoints();
        doneCurrent();
    }
    
    update();
}

void UVParamWidget::loadOBJ(const QString &path) {
    parseOBJ(path);
}

void UVParamWidget::clearData() {
    hasUV = false;
    vertices.clear();
    textureCoords.clear();
    uvCoords.clear();
    faceIndices.clear();
    lineVertexCount = 0;
    
    makeCurrent();
    uvVbo.bind();
    uvVbo.allocate(0, 0);
    lineVbo.bind();
    lineVbo.allocate(0, 0);
    doneCurrent();
    
    update();
}

void UVParamWidget::setShowLines(bool show) {
    showLines = show;
    update();
}