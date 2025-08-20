// cgalglwidget.cpp
#include "cgalglwidget.h"
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
#include <QPainter>
#include <QFont>
#include <cfloat>
#include <fstream>
#include <CGAL/IO/OBJ.h>
#include <CGAL/Polygon_mesh_processing/compute_normal.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/distance.h>
#include <CGAL/Side_of_triangle_mesh.h>

CGALGLWidget::CGALGLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    axisVbo(QOpenGLBuffer::VertexBuffer),
    axisEbo(QOpenGLBuffer::IndexBuffer),
    showWireframeOverlay(false),
    hideFaces(false)
{
    QSurfaceFormat format;
    format.setSamples(4);
    setFormat(format);
    
    setFocusPolicy(Qt::StrongFocus);
    rotation = QQuaternion();
    zoom = 1.0f;
    modelLoaded = false;
    isDragging = false;
    bgColor = QColor(0, 0, 0);
    currentRenderMode = BlinnPhong;
    wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
    
    modelCenter = QVector3D(0, 0, 0);
    viewDistance = 5.0f;
    viewScale = 1.0f;
    
    initialRotation = QQuaternion();
    initialZoom = 1.0f;
    initialModelCenter = QVector3D(0, 0, 0);
    initialViewDistance = 5.0f;
    initialViewScale = 1.0f;
    wireframeColor = QVector4D(0.0f, 0.0f, 0.25f, 1.0f);
    // 修改表面颜色为米白色 (0.88, 0.84, 0.76)
    surfaceColor = QVector3D(0.88f, 0.84f, 0.76f);
    showAxis = true;
}

void CGALGLWidget::setShowAxis(bool show) {
    showAxis = show;
    update();
}

void CGALGLWidget::setViewScale(float scale) {
    viewScale = scale;
    update();
}

void CGALGLWidget::setHideFaces(bool hide) {
    hideFaces = hide;
    update();
}

void CGALGLWidget::setShowWireframeOverlay(bool show) {
    showWireframeOverlay = show;
    update();
}

void CGALGLWidget::setWireframeColor(const QVector4D& color) {
    wireframeColor = color;
    update();
}

CGALGLWidget::~CGALGLWidget() {
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
    faceEbo.destroy();
    axisVbo.destroy();
    axisEbo.destroy();
    doneCurrent();
}

void CGALGLWidget::resetView() {
    rotation = initialRotation;
    zoom = initialZoom;
    modelCenter = initialModelCenter;
    viewDistance = initialViewDistance;
    viewScale = initialViewScale;
    update();
}

void CGALGLWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void CGALGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    vao.create();
    vbo.create();
    ebo.create();
    faceEbo.create();
    
    axisVbo.create();
    axisEbo.create();
    
    float axisVertices[] = {
        0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
        
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        0.0f, 2.0f, 0.0f,  0.0f, 1.0f, 0.0f,
        
        0.0f, 0.0f, 0.0f,  0.0f, 0.5f, 1.0f,
        0.0f, 0.0f, 2.0f,  0.0f, 0.5f, 1.0f
    };
    
    unsigned int axisIndices[] = {
        0, 1,
        2, 3,
        4, 5
    };
    
    axisVbo.bind();
    axisVbo.allocate(axisVertices, sizeof(axisVertices));
    
    axisEbo.bind();
    axisEbo.allocate(axisIndices, sizeof(axisIndices));
    
    axisProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/axis.vert");
    axisProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/axis.frag");
    axisProgram.link();

    initializeShaders();
}

void CGALGLWidget::computeNormals() {
    // 计算面法线
    face_normals.clear();
    face_normals.reserve(mesh.number_of_faces());
    
    for(auto face : mesh.faces()) {
        // 获取面的顶点
        std::vector<Point> points;
        for(auto vertex : CGAL::vertices_around_face(mesh.halfedge(face), mesh)) {
            points.push_back(mesh.point(vertex));
        }
        
        // 计算面法线
        if(points.size() >= 3) {
            Vector v1 = points[1] - points[0];
            Vector v2 = points[2] - points[0];
            Vector normal = CGAL::cross_product(v1, v2);
            normal = normal / std::sqrt(normal * normal); // 归一化
            face_normals.push_back(normal);
        } else {
            face_normals.push_back(Vector(0, 0, 0));
        }
    }
    
    // 计算顶点法线（平均相邻面的法线）
    vertex_normals.clear();
    vertex_normals.resize(mesh.number_of_vertices(), Vector(0, 0, 0));
    
    int face_idx = 0;
    for(auto face : mesh.faces()) {
        for(auto vertex : CGAL::vertices_around_face(mesh.halfedge(face), mesh)) {
            vertex_normals[vertex.idx()] = vertex_normals[vertex.idx()] + face_normals[face_idx];
        }
        face_idx++;
    }
    
    // 归一化顶点法线
    for(auto& normal : vertex_normals) {
        double length = std::sqrt(normal * normal);
        if(length > 0) {
            normal = normal / length;
        }
    }
}

void CGALGLWidget::initializeShaders() {
    wireframeProgram.removeAllShaders();
    blinnPhongProgram.removeAllShaders();
    flatProgram.removeAllShaders();  // 确保清除旧的着色器

    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/wireframe.vert");
    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/wireframe.frag");
    wireframeProgram.link();
    
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/blinnphong.vert");
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/blinnphong.frag");
    blinnPhongProgram.link();
    
    // 添加Flat Shading着色器
    flatProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/flat.vert");
    flatProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/flat.frag");
    flatProgram.link();

    if (modelLoaded) {
        updateBuffersFromCGALMesh();
    }
}

void CGALGLWidget::updateBuffersFromCGALMesh() {
    if (mesh.number_of_vertices() == 0) return;
    
    std::vector<float> vertices;
    std::vector<float> normals;
    
    // 提取顶点数据
    for(auto v : mesh.vertices()) {
        const Point& p = mesh.point(v);
        vertices.push_back(p.x());
        vertices.push_back(p.y());
        vertices.push_back(p.z());
        
        // 使用计算的法线
        if(v.idx() < vertex_normals.size()) {
            const Vector& n = vertex_normals[v.idx()];
            normals.push_back(n.x());
            normals.push_back(n.y());
            normals.push_back(n.z());
        } else {
            normals.push_back(0.0f);
            normals.push_back(1.0f);
            normals.push_back(0.0f);
        }
    }
    
    vao.bind();
    vbo.bind();
    
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    vbo.allocate(vertexSize + normalSize);
    vbo.write(0, vertices.data(), vertexSize);
    vbo.write(vertexSize, normals.data(), normalSize);
    
    wireframeProgram.bind();
    int posLoc = wireframeProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        wireframeProgram.enableAttributeArray(posLoc);
        wireframeProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
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

    ebo.bind();
    ebo.allocate(edges.data(), edges.size() * sizeof(unsigned int));
    
    faceEbo.bind();
    faceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    
    vao.release();
}

void CGALGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void CGALGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || mesh.number_of_vertices() == 0) {
        return;
    }

    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    GLint oldPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);

    // 定义三个光源的位置和颜色
    QVector3D lightPositions[3] = {
        QVector3D(10.0f, 10.0f, -10.0f),
        QVector3D(-10.0f, 10.0f, -10.0f),
        QVector3D(0.0f, 0.0f, 10.0f)
    };
    QVector3D lightColors[3] = {
        QVector3D(1.0f, 1.0f, 1.0f),
        QVector3D(1.0f, 1.0f, 1.0f),
        QVector3D(1.0f, 1.0f, 1.0f)
    };

    if (hideFaces) {
        drawWireframe(model, view, projection);
    } else {
        if (currentRenderMode == BlinnPhong) {
            blinnPhongProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            blinnPhongProgram.setUniformValue("model", model);
            blinnPhongProgram.setUniformValue("view", view);
            blinnPhongProgram.setUniformValue("projection", projection);
            blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置三个光源的位置和颜色
            for (int i = 0; i < 3; i++) {
                blinnPhongProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                blinnPhongProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            blinnPhongProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
            blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            blinnPhongProgram.release();
        } else if (currentRenderMode == FlatShading) {
            // Flat Shading渲染
            flatProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            flatProgram.setUniformValue("model", model);
            flatProgram.setUniformValue("view", view);
            flatProgram.setUniformValue("projection", projection);
            flatProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置三个光源的位置和颜色
            for (int i = 0; i < 3; i++) {
                flatProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                flatProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            flatProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
            flatProgram.setUniformValue("objectColor", surfaceColor);
            flatProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            flatProgram.release();
        }

        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
    }
    
    if (showAxis) {
        drawXYZAxis(view, projection);
    }
    
    glPolygonMode(GL_FRONT, oldPolygonMode[0]);
    glPolygonMode(GL_BACK, oldPolygonMode[1]);
}

void CGALGLWidget::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Left:
        rotation = QQuaternion::fromAxisAndAngle(0, 1, 0, 5) * rotation;
        break;
    case Qt::Key_Right:
        rotation = QQuaternion::fromAxisAndAngle(0, 1, 0, -5) * rotation;
        break;
    case Qt::Key_Up:
        rotation = QQuaternion::fromAxisAndAngle(1, 0, 0, -5) * rotation;
        break;
    case Qt::Key_Down:
        rotation = QQuaternion::fromAxisAndAngle(1, 0, 0, 5) * rotation;
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
    case Qt::Key_A:
        showAxis = !showAxis;
        update();
        break;
    default:
        QOpenGLWidget::keyPressEvent(event);
    }
    update();
}

void CGALGLWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void CGALGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void CGALGLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging) {
        QPoint currentPos = event->pos();
        
        QVector3D lastPos3D = projectToTrackball(lastMousePos);
        QVector3D currentPos3D = projectToTrackball(currentPos);
        
        QVector3D axis = QVector3D::crossProduct(lastPos3D, currentPos3D).normalized();
        float angle = acos(qMin(1.0f, QVector3D::dotProduct(lastPos3D, currentPos3D))) 
                    * 180.0f / M_PI * rotationSensitivity;
        
        QQuaternion newRot = QQuaternion::fromAxisAndAngle(axis, angle);
        rotation = newRot * rotation;
        
        lastMousePos = currentPos;
        update();
    }
}

QVector3D CGALGLWidget::projectToTrackball(const QPoint& screenPos) {
    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();
    float z = 0.0f;
    
    float lengthSquared = x * x + y * y;
    if (lengthSquared <= 1.0f) {
        z = sqrt(1.0f - lengthSquared);
    } else {
        float length = sqrt(lengthSquared);
        x /= length;
        y /= length;
    }
    
    return QVector3D(x, y, z);
}

void CGALGLWidget::wheelEvent(QWheelEvent *event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
        zoom *= delta;
        zoom = qBound(0.1f, zoom, 10.0f);
        update();
    }
    event->accept();
}

void CGALGLWidget::setSurfaceColor(const QVector3D& color) {
    surfaceColor = color;
    update();
}

void CGALGLWidget::setSpecularEnabled(bool enabled) {
    specularEnabled = enabled;
    update();
}

void CGALGLWidget::centerView() {
    if (mesh.number_of_vertices() == 0) return;
    
    Point min, max;
    computeBoundingBox(min, max);
    
    Point center = Point((min.x() + max.x()) * 0.5, 
                         (min.y() + max.y()) * 0.5, 
                         (min.z() + max.z()) * 0.5);
    double size_x = max.x() - min.x();
    double size_y = max.y() - min.y();
    double size_z = max.z() - min.z();
    double maxSize = std::max({size_x, size_y, size_z});
    
    modelCenter = QVector3D(center.x(), center.y(), center.z());
    viewDistance = 2.0f * maxSize;
    
    rotation = QQuaternion();
    zoom = 1.0f;
    update();
}

void CGALGLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void CGALGLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void CGALGLWidget::drawXYZAxis(const QMatrix4x4& view, const QMatrix4x4& projection) {
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    glDisable(GL_DEPTH_TEST);
    
    axisProgram.bind();
    
    QMatrix4x4 model;
    model.translate(modelCenter);
    model.scale(0.8f);
    model.rotate(rotation);
    
    axisProgram.setUniformValue("model", model);
    axisProgram.setUniformValue("view", view);
    axisProgram.setUniformValue("projection", projection);
    
    axisVbo.bind();
    axisEbo.bind();
    
    int posLoc = axisProgram.attributeLocation("aPos");
    axisProgram.enableAttributeArray(posLoc);
    axisProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 6 * sizeof(float));
    
    int colorLoc = axisProgram.attributeLocation("aColor");
    axisProgram.enableAttributeArray(colorLoc);
    axisProgram.setAttributeBuffer(colorLoc, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    
    glLineWidth(3.0f);
    glDrawElements(GL_LINES, 6, GL_UNSIGNED_INT, 0);
    
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    
    axisProgram.disableAttributeArray(posLoc);
    axisProgram.disableAttributeArray(colorLoc);
    axisEbo.release();
    axisVbo.release();
    axisProgram.release();
}

void CGALGLWidget::clearMeshData() {
    mesh.clear();
    faces.clear();
    edges.clear();
    vertex_normals.clear();
    face_normals.clear();
    modelLoaded = false;
}

bool CGALGLWidget::loadOBJToCGALMesh(const QString &path) {
    std::ifstream in(path.toStdString());
    if (!in) {
        return false;
    }
    
    return CGAL::IO::read_OBJ(in, mesh);
}

void CGALGLWidget::computeBoundingBox(Point& min, Point& max) {
    if (mesh.number_of_vertices() > 0) {
        auto v_it = mesh.vertices_begin();
        Point first = mesh.point(*v_it);
        min = max = first;
        
        for (auto v : mesh.vertices()) {
            const Point& p = mesh.point(v);
            min = Point(std::min(min.x(), p.x()), std::min(min.y(), p.y()), std::min(min.z(), p.z()));
            max = Point(std::max(max.x(), p.x()), std::max(max.y(), p.y()), std::max(max.z(), p.z()));
        }
    }
}

void CGALGLWidget::centerAndScaleMesh(const Point& center, float maxSize) {
    float scaleFactor = 2.0f / maxSize;
    for (auto v : mesh.vertices()) {
        Point p = mesh.point(v);
        p = Point((p.x() - center.x()) * scaleFactor, 
                  (p.y() - center.y()) * scaleFactor, 
                  (p.z() - center.z()) * scaleFactor);
        mesh.point(v) = p;
    }
}

void CGALGLWidget::prepareFaceIndices() {
    for (auto f : mesh.faces()) {
        std::vector<unsigned int> faceVertices;
        for (auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
            faceVertices.push_back(v.idx());
        }
        
        if (faceVertices.size() < 3) continue;
        
        if (faceVertices.size() == 3) {
            faces.insert(faces.end(), faceVertices.begin(), faceVertices.end());
        } else {
            // 简单三角化：使用扇形三角化
            unsigned int centerIdx = faceVertices[0];
            for (size_t i = 1; i < faceVertices.size() - 1; i++) {
                faces.push_back(centerIdx);
                faces.push_back(faceVertices[i]);
                faces.push_back(faceVertices[i + 1]);
            }
        }
    }
}

void CGALGLWidget::prepareEdgeIndices() {
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    
    for (auto e : mesh.edges()) {
        auto h = mesh.halfedge(e);
        unsigned int from = mesh.source(h).idx();
        unsigned int to = mesh.target(h).idx();
        
        if (from > to) std::swap(from, to);
        uniqueEdges.insert({from, to});
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
}

void CGALGLWidget::saveOriginalMesh() {
    // CGAL Surface_mesh 没有直接的复制构造函数，需要手动复制
    // 这里简单标记为有原始网格，实际应用中可能需要更复杂的实现
    hasOriginalMesh = true;
}

void CGALGLWidget::loadOBJ(const QString &path) {
    clearMeshData();
    if (!loadOBJToCGALMesh(path)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    Point min, max;
    computeBoundingBox(min, max);
    
    Point center = Point((min.x() + max.x()) * 0.5, 
                         (min.y() + max.y()) * 0.5, 
                         (min.z() + max.z()) * 0.5);
    double size_x = max.x() - min.x();
    double size_y = max.y() - min.y();
    double size_z = max.z() - min.z();
    double maxSize = std::max({size_x, size_y, size_z});
    
    centerAndScaleMesh(center, maxSize);
    
    Point min_norm, max_norm;
    computeBoundingBox(min_norm, max_norm);
    Point center_norm = Point((min_norm.x() + max_norm.x()) * 0.5, 
                              (min_norm.y() + max_norm.y()) * 0.5, 
                              (min_norm.z() + max_norm.z()) * 0.5);
    double size_x_norm = max_norm.x() - min_norm.x();
    double size_y_norm = max_norm.y() - min_norm.y();
    double size_z_norm = max_norm.z() - min_norm.z();
    double maxSize_norm = std::max({size_x_norm, size_y_norm, size_z_norm});

    modelCenter = QVector3D(0, 0, 0);
    viewDistance = 2.0f * maxSize_norm;
    
    initialRotation = QQuaternion();
    initialZoom = 1.0f;
    initialModelCenter = modelCenter;
    initialViewDistance = viewDistance;
    initialViewScale = viewScale;

    // 计算法线
    computeNormals();
    
    prepareFaceIndices();
    prepareEdgeIndices();
    
    saveOriginalMesh();
    modelLoaded = true;
    
    makeCurrent();
    initializeShaders();
    doneCurrent();
    
    rotation = QQuaternion();
    zoom = 1.0f;
    update();
}