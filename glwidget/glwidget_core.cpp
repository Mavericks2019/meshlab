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
#include <cfloat> // 用于FLT_MAX和FLT_MIN

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    axisVbo(QOpenGLBuffer::VertexBuffer), // 初始化坐标轴VBO
    axisEbo(QOpenGLBuffer::IndexBuffer),  // 初始化坐标轴EBO
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
    
    // 初始化模型中心和视图距离
    modelCenter = QVector3D(0, 0, 0);
    viewDistance = 5.0f;
    viewScale = 1.0f; // 默认缩放因子为1.0
    
    // 初始化初始视图状态
    initialRotationX = 0;
    initialRotationY = 0;
    initialZoom = 1.0f;
    initialModelCenter = QVector3D(0, 0, 0);
    initialViewDistance = 5.0f;
    initialViewScale = 1.0f;
    
    // 默认显示坐标轴
    showAxis = true;
}

// 新增：设置视角缩放因子
void GLWidget::setViewScale(float scale) {
    viewScale = scale;
    update(); // 触发重绘
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
    axisVbo.destroy();  // 销毁坐标轴VBO
    axisEbo.destroy();  // 销毁坐标轴EBO
    doneCurrent();
}

void GLWidget::resetView() {
    // 使用保存的初始视图状态
    rotationX = initialRotationX;
    rotationY = initialRotationY;
    zoom = initialZoom;
    modelCenter = initialModelCenter;
    viewDistance = initialViewDistance;
    viewScale = initialViewScale; // 重置视角缩放因子
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
    vbo.create();
    ebo.create();
    faceEbo.create();
    
    // 初始化坐标轴缓冲区
    axisVbo.create();
    axisEbo.create();
    
    // 坐标轴数据
    float axisVertices[] = {
        // 位置              // 颜色
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X轴起点 (红色)
        2.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // X轴终点 - 长度从1.0f增加到2.0f
        
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y轴起点 (绿色)
        0.0f, 2.0f, 0.0f,  0.0f, 1.0f, 0.0f, // Y轴终点 - 长度从1.0f增加到2.0f
        
        0.0f, 0.0f, 0.0f,  0.0f, 0.5f, 1.0f, // Z轴起点 (蓝色)
        0.0f, 0.0f, 2.0f,  0.0f, 0.5f, 1.0f  // Z轴终点 - 长度从1.0f增加到2.0f
    };
    
    unsigned int axisIndices[] = {
        0, 1, // X轴
        2, 3, // Y轴
        4, 5  // Z轴
    };
    
    axisVbo.bind();
    axisVbo.allocate(axisVertices, sizeof(axisVertices));
    
    axisEbo.bind();
    axisEbo.allocate(axisIndices, sizeof(axisIndices));
    
    // 初始化坐标轴着色器
    axisProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/axis.vert");
    axisProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/axis.frag");
    axisProgram.link();

    initializeShaders();
}

void GLWidget::initializeShaders() {
    wireframeProgram.removeAllShaders();
    blinnPhongProgram.removeAllShaders();
    curvatureProgram.removeAllShaders();

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
    
    // 模型变换：以模型中心为原点进行旋转缩放
    model.rotate(rotationX, 1, 0, 0);
    model.rotate(rotationY, 0, 1, 0);
    model.scale(zoom);
    
    // 视图变换：相机看向模型中心
    // 应用视角缩放因子
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    // 投影变换
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    if (hideFaces) {
        drawWireframe(model, view, projection);
    } else {
        switch (currentRenderMode) {
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
    
    // 绘制XYZ坐标轴
    if (showAxis) {
        drawXYZAxis(view, projection);
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
    case Qt::Key_A: // 按A键切换坐标轴显示
        showAxis = !showAxis;
        update();
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
    
    // 设置视图参数（与模型加载时相同的计算逻辑）
    modelCenter = QVector3D(center[0], center[1], center[2]);
    viewDistance = 2.0f * maxSize; // 基础视图距离
    
    rotationX = 0;
    rotationY = 0;
    zoom = 1.0f;
    update();
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
    
    // 注意：这里也需要使用缩放后的眼位置
    blinnPhongProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
    
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

void GLWidget::drawXYZAxis(const QMatrix4x4& view, const QMatrix4x4& projection) {
    // 保存当前深度测试状态
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    // 暂时禁用深度测试，确保坐标轴始终可见
    glDisable(GL_DEPTH_TEST);
    
    axisProgram.bind();
    
    // 设置模型矩阵 - 坐标轴位置在原点，大小固定
    QMatrix4x4 model;
    model.translate(modelCenter);
    model.scale(0.8f); // 稍微增大缩放比例
    
    // 应用当前旋转
    model.rotate(rotationX, 1, 0, 0);
    model.rotate(rotationY, 0, 1, 0);
    
    axisProgram.setUniformValue("model", model);
    axisProgram.setUniformValue("view", view);
    axisProgram.setUniformValue("projection", projection);
    
    axisVbo.bind();
    axisEbo.bind();
    
    // 设置顶点属性指针
    int posLoc = axisProgram.attributeLocation("aPos");
    axisProgram.enableAttributeArray(posLoc);
    axisProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 6 * sizeof(float));
    
    int colorLoc = axisProgram.attributeLocation("aColor");
    axisProgram.enableAttributeArray(colorLoc);
    axisProgram.setAttributeBuffer(colorLoc, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    
    // 绘制坐标轴 - 增加线宽
    glLineWidth(3.0f); // 增加线宽到3.0f
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, 0);
    
    // 恢复之前的深度测试状态
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    
    // 清理
    axisProgram.disableAttributeArray(posLoc);
    axisProgram.disableAttributeArray(colorLoc);
    axisEbo.release();
    axisVbo.release();
    axisProgram.release();
}