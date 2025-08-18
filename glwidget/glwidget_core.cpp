#include "glwidget.h"
#include <QFile>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QtMath>
#include <QResource>
#include <algorithm>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <QPainter>
#include <QFont>

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    texCoordBuffer(QOpenGLBuffer::VertexBuffer),
    showWireframeOverlay(false),
    hideFaces(false)
{
    QSurfaceFormat format;
    format.setSamples(4);
    setFormat(format);
    
    setFocusPolicy(Qt::StrongFocus);
    rotationX = rotationY = 0;
    zoom = 1.0f;
    modelLoaded = false;
    isDragging = false;
    bgColor = QColor(0, 0, 0);
    currentRenderMode = BlinnPhong;
    wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
}

void GLWidget::setHideFaces(bool hide) {
    hideFaces = hide;
    update();
}

void GLWidget::setShowWireframeOverlay(bool show) {
    showWireframeOverlay = show;
    update();
}

void GLWidget::setWireframeColor(const QVector4D& color) {
    wireframeColor = color;
    update();
}

GLWidget::~GLWidget() {
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
    faceEbo.destroy();
    texCoordBuffer.destroy();
    doneCurrent();
}

void GLWidget::resetView() {
    rotationX = rotationY = 0;
    zoom = 1.0f;
    update();
}

void GLWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void GLWidget::setRenderMode(RenderMode mode) {
    currentRenderMode = mode;
    if (modelLoaded) {
        calculateCurvatures();
        makeCurrent();
        initializeShaders();
        doneCurrent();
    }
    update();
}

void GLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    vao.create();
    vaoTexture.create();
    vbo.create();
    ebo.create();
    faceEbo.create();
    texCoordBuffer.create();

    generateCheckerboardTexture();
    initializeShaders();
}

void GLWidget::initializeShaders() {
    wireframeProgram.removeAllShaders();
    blinnPhongProgram.removeAllShaders();
    curvatureProgram.removeAllShaders();
    textureProgram.removeAllShaders();

    // Wireframe shader
    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/wireframe.vert");
    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/wireframe.frag");
    wireframeProgram.link();
    
    // Blinn-Phong shader
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/blinnphong.vert");
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/blinnphong.frag");
    blinnPhongProgram.link();
    
    // Curvature shader
    curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/curvature.vert");
    curvatureProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/curvature.frag");
    curvatureProgram.link();
    
    // Texture shader
    textureProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/texture.vert");
    textureProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/texture.frag");
    textureProgram.link();

    if (modelLoaded) {
        updateBuffersFromOpenMesh();
    }
}

void GLWidget::updateBuffersFromOpenMesh() {
    if (openMesh.n_vertices() == 0) return;
    
    std::vector<float> vertices(openMesh.n_vertices() * 3);
    std::vector<float> normals(openMesh.n_vertices() * 3);
    std::vector<float> curvatures(openMesh.n_vertices());
    
    for (auto vh : openMesh.vertices()) {
        int idx = vh.idx();
        const auto& p = openMesh.point(vh);
        vertices[idx*3]   = p[0];
        vertices[idx*3+1] = p[1];
        vertices[idx*3+2] = p[2];
        
        const auto& n = openMesh.normal(vh);
        normals[idx*3]   = n[0];
        normals[idx*3+1] = n[1];
        normals[idx*3+2] = n[2];
        
        curvatures[idx] = openMesh.data(vh).curvature;
    }
    
    vao.bind();
    vbo.bind();
    
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    int curvatureSize = curvatures.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize + curvatureSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    vbo.write(vertexSize + normalSize, curvatures.data(), curvatureSize);
    
    // Set up wireframe shader
    wireframeProgram.bind();
    int posLoc = wireframeProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        wireframeProgram.enableAttributeArray(posLoc);
        wireframeProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    // Set up Blinn-Phong shader
    blinnPhongProgram.bind();
    posLoc = blinnPhongProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        blinnPhongProgram.enableAttributeArray(posLoc);
        blinnPhongProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    int normalLoc = blinnPhongProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        blinnPhongProgram.enableAttributeArray(normalLoc);
        blinnPhongProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    // Set up curvature shader
    curvatureProgram.bind();
    posLoc = curvatureProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        curvatureProgram.enableAttributeArray(posLoc);
        curvatureProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = curvatureProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        curvatureProgram.enableAttributeArray(normalLoc);
        curvatureProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }
    
    int curvatureLoc = curvatureProgram.attributeLocation("aCurvature");
    if (curvatureLoc != -1) {
        curvatureProgram.enableAttributeArray(curvatureLoc);
        curvatureProgram.setAttributeBuffer(curvatureLoc, GL_FLOAT, vertexSize + normalSize, 1, sizeof(float));
    }

    ebo.bind();
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int));
    
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    
    vao.release();
    
    // Set up texture shader
    vaoTexture.bind();
    vbo.bind();
    
    textureProgram.bind();
    posLoc = textureProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        textureProgram.enableAttributeArray(posLoc);
        textureProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = textureProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        textureProgram.enableAttributeArray(normalLoc);
        textureProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }

    updateTextureCoordinates();
    
    texCoordBuffer.bind();
    texCoordBuffer.allocate(texCoords.data(), texCoords.size() * sizeof(float));
    
    int texCoordLoc = textureProgram.attributeLocation("aTexCoord");
    if (texCoordLoc != -1) {
        textureProgram.enableAttributeArray(texCoordLoc);
        textureProgram.setAttributeBuffer(texCoordLoc, GL_FLOAT, 0, 2, 2 * sizeof(float));
    }

    faceEbo.bind();
    
    vaoTexture.release();
    textureProgram.release();
}

void GLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || openMesh.n_vertices() == 0) {
        return;
    }

    QMatrix4x4 model, view, projection;
    model.translate(0, 0, -2.5);
    model.rotate(rotationX, 1, 0, 0);
    model.rotate(rotationY, 0, 1, 0);
    model.scale(zoom);
    
    view.lookAt(QVector3D(0, 0, 5), QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    if (hideFaces) {
        drawWireframe(model, view, projection);
    } else {
        switch (currentRenderMode) {
        case TextureMapping:
            drawTextureMapping(model, view, projection, normalMatrix);
            break;
        case GaussianCurvature:
        case MeanCurvature:
        case MaxCurvature:
            drawCurvature(model, view, projection, normalMatrix);
            break;
        default:
            drawBlinnPhong(model, view, projection, normalMatrix);
            break;
        }

        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
    }
    
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Left:
        rotationY -= 5;
        break;
    case Qt::Key_Right:
        rotationY += 5;
        break;
    case Qt::Key_Up:
        rotationX -= 5;
        break;
    case Qt::Key_Down:
        rotationX += 5;
        break;
    case Qt::Key_Plus:
        zoom *= 1.1;
        break;
    case Qt::Key_Minus:
        zoom /= 1.1;
        break;
    case Qt::Key_R:
        resetView();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }
    update();
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging) {
        QPoint currentPos = event->pos();
        QPoint delta = currentPos - lastMousePos;
        
        rotationY += delta.x() * 0.5f;
        rotationX += delta.y() * 0.5f;
        
        rotationX = qBound(-90.0f, rotationX, 90.0f);
        
        lastMousePos = currentPos;
        update();
    }
}

void GLWidget::wheelEvent(QWheelEvent *event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
        zoom *= delta;
        zoom = qBound(0.1f, zoom, 10.0f);
        update();
    }
}

void GLWidget::setSurfaceColor(const QVector3D& color) {
    surfaceColor = color;
    update();
}

void GLWidget::setSpecularEnabled(bool enabled) {
    specularEnabled = enabled;
    update();
}

void GLWidget::centerView() {
    if (openMesh.n_vertices() == 0) return;
    
    Mesh::Point min, max;
    min = max = openMesh.point(*openMesh.vertices_begin());
    for (auto vh : openMesh.vertices()) {
        min.minimize(openMesh.point(vh));
        max.maximize(openMesh.point(vh));
    }
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    zoom = 1.0f;
    if (maxSize > 0.1f) {
        zoom = 1.0f / maxSize;
    }
    
    rotationX = 0;
    rotationY = 0;
    update();
}

void GLWidget::generateCheckerboardTexture() {
    const int size = 512;
    const int tileSize = 32;
    QImage image(size, size, QImage::Format_RGB32);
    
    const QColor brownColor(139, 69, 19);
    const QColor whiteColor(255, 255, 255);
    
    QPainter painter(&image);
    QFont font;
    font.setFamily("Arial");
    font.setPixelSize(tileSize / 2);
    font.setBold(true);
    painter.setFont(font);
    
    const QString letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    for (int y = 0; y < size; y += tileSize) {
        for (int x = 0; x < size; x += tileSize) {
            bool isBrown = ((x / tileSize) % 2) ^ ((y / tileSize) % 2);
            QColor tileColor = isBrown ? brownColor : whiteColor;
            
            for (int dy = 0; dy < tileSize && y + dy < size; dy++) {
                for (int dx = 0; dx < tileSize && x + dx < size; dx++) {
                    image.setPixel(x + dx, y + dy, tileColor.rgb());
                }
            }
            
            int tileIndex = (y / tileSize) * (size / tileSize) + (x / tileSize);
            char currentChar = letters.at(tileIndex % letters.length()).toLatin1();
            
            painter.setPen(isBrown ? whiteColor : brownColor);
            QRect tileRect(x, y, tileSize, tileSize);
            painter.drawText(tileRect, Qt::AlignCenter, QString(currentChar));
        }
    }
    
    painter.end();
}

void GLWidget::updateTextureCoordinates() {
    texCoords.clear();
    texCoords.reserve(openMesh.n_vertices() * 2);
    
    for (auto vh : openMesh.vertices()) {
        const auto& p = openMesh.point(vh);
        texCoords.push_back((p[0] + 1.0f) * 0.5f);
        texCoords.push_back((p[1] + 1.0f) * 0.5f);
    }
}

void GLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
    wireframeProgram.bind();
    vao.bind();
    ebo.bind();

    glLineWidth(1.5f);
    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    vao.release();
    wireframeProgram.release();
}

void GLWidget::drawTextureMapping(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    textureProgram.bind();
    vaoTexture.bind();
    faceEbo.bind();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    textureProgram.setUniformValue("model", model);
    textureProgram.setUniformValue("view", view);
    textureProgram.setUniformValue("projection", projection);
    textureProgram.setUniformValue("normalMatrix", normalMatrix);
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    textureProgram.release();
}

void GLWidget::drawCurvature(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    curvatureProgram.bind();
    vao.bind();
    faceEbo.bind();
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    curvatureProgram.setUniformValue("model", model);
    curvatureProgram.setUniformValue("view", view);
    curvatureProgram.setUniformValue("projection", projection);
    curvatureProgram.setUniformValue("normalMatrix", normalMatrix);
    curvatureProgram.setUniformValue("curvatureType", static_cast<int>(currentRenderMode));
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    curvatureProgram.release();
}

void GLWidget::drawBlinnPhong(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection, const QMatrix3x3& normalMatrix) {
    blinnPhongProgram.bind();
    vao.bind();
    faceEbo.bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    blinnPhongProgram.setUniformValue("model", model);
    blinnPhongProgram.setUniformValue("view", view);
    blinnPhongProgram.setUniformValue("projection", projection);
    blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
    blinnPhongProgram.setUniformValue("lightPos", QVector3D(2.0f, 2.0f, 2.0f));
    blinnPhongProgram.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
    blinnPhongProgram.setUniformValue("lightColor", QVector3D(1.0f, 1.0f, 1.0f));
    blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
    blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);

    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

    faceEbo.release();
    vao.release();
    blinnPhongProgram.release();
}

void GLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0, -1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.5f);
    
    wireframeProgram.bind();
    vao.bind();
    ebo.bind();

    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    vao.release();
    wireframeProgram.release();
    glDisable(GL_POLYGON_OFFSET_LINE);
}