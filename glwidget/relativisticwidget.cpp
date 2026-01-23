// relativisticwidget.cpp
#include "relativisticwidget.h"
#include <QFile>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QSurfaceFormat>
#include <QVector3D>
#include <QtMath>
#include <OpenMesh/Core/IO/MeshIO.hh>
#include <algorithm>
#include <set>

RelativisticGLWidget::RelativisticGLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer)
{
    QSurfaceFormat format;
    format.setSamples(4);
    format.setDepthBufferSize(24);  // 确保深度缓冲区大小
    setFormat(format);
    
    setFocusPolicy(Qt::StrongFocus);
    rotation = QQuaternion();
    
    // 禁用鼠标拖动旋转
    rotationSensitivity = 0.0f;
}

RelativisticGLWidget::~RelativisticGLWidget() {
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
    faceEbo.destroy();
    doneCurrent();
}

void RelativisticGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    
    // 启用深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    glEnable(GL_MULTISAMPLE);

    vao.create();
    vbo.create();
    ebo.create();
    faceEbo.create();
    
    // 线框着色器
    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/wireframe.vert");
    wireframeProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/wireframe.frag");
    wireframeProgram.link();
    
    // Flat Shading着色器
    faceProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/flat.vert");
    faceProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/flat.frag");
    faceProgram.link();
    
    // Blinn-Phong着色器
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/blinnphong.vert");
    blinnPhongProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/blinnphong.frag");
    blinnPhongProgram.link();
}

void RelativisticGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void RelativisticGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || openMesh.n_vertices() == 0) {
        return;
    }

    QMatrix4x4 model, view, projection;
    
    // 模型矩阵：先旋转，再缩放
    model.rotate(rotation);
    model.scale(zoom);
    
    // 视图矩阵：相机在(0,0,0)，看向(0,0,-1)，使用正交投影
    view.lookAt(QVector3D(0, 0, 0), QVector3D(0, 0, -1), QVector3D(0, 1, 0));
    
    // 投影矩阵：正交投影，范围(-50,50)在XY平面，深度(-50,50)
    float aspect = width() / float(height());
    float viewWidth = 100.0f;
    float viewHeight = viewWidth / aspect;
    projection.ortho(-viewWidth/2, viewWidth/2, -viewHeight/2, viewHeight/2, 0.1f, 100.0f);
    
    QMatrix3x3 normalMatrix = model.normalMatrix();

    // 定义三个光源的位置和颜色（与BaseGLWidget相同）
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
        // 隐藏面片，只绘制线框
        drawWireframe(model, view, projection);
    } else {
        // 根据渲染模式绘制面片
        if (currentRenderMode == FlatShading || currentRenderMode == FacesOnly) {
            // Flat Shading渲染
            faceProgram.bind();
            vao.bind();
            faceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            faceProgram.setUniformValue("model", model);
            faceProgram.setUniformValue("view", view);
            faceProgram.setUniformValue("projection", projection);
            faceProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置三个光源的位置和颜色
            for (int i = 0; i < 3; i++) {
                faceProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                faceProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            faceProgram.setUniformValue("viewPos", QVector3D(0, 0, 0));
            faceProgram.setUniformValue("objectColor", surfaceColor);
            faceProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            faceProgram.release();
        } else if (currentRenderMode == BlinnPhong) {
            // Blinn-Phong渲染
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
            
            blinnPhongProgram.setUniformValue("viewPos", QVector3D(0, 0, 0));
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
            blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);

            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);

            faceEbo.release();
            vao.release();
            blinnPhongProgram.release();
        }

        // 如果显示线框叠加且不是线框模式，绘制线框叠加
        if (showWireframeOverlay && currentRenderMode != WireframeOnly) {
            drawWireframeOverlay(model, view, projection);
        }
        
        // 如果当前是线框模式，绘制线框
        if (currentRenderMode == WireframeOnly) {
            drawWireframe(model, view, projection);
        }
    }
}

void RelativisticGLWidget::keyPressEvent(QKeyEvent *event) {
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
    default:
        QOpenGLWidget::keyPressEvent(event);
    }
    update();
}

void RelativisticGLWidget::mousePressEvent(QMouseEvent *event) {
    // 禁用鼠标拖动旋转
    // if (event->button() == Qt::LeftButton) {
    //     isDragging = true;
    //     lastMousePos = event->pos();
    //     setCursor(Qt::ClosedHandCursor);
    // }
    QOpenGLWidget::mousePressEvent(event);
}

void RelativisticGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    // 禁用鼠标拖动旋转
    // if (event->button() == Qt::LeftButton) {
    //     isDragging = false;
    //     setCursor(Qt::ArrowCursor);
    // }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void RelativisticGLWidget::mouseMoveEvent(QMouseEvent *event) {
    // 禁用鼠标拖动旋转
    // if (isDragging) {
    //     QPoint currentPos = event->pos();
    //     
    //     QVector3D lastPos3D = projectToTrackball(lastMousePos);
    //     QVector3D currentPos3D = projectToTrackball(currentPos);
    //     
    //     QVector3D axis = QVector3D::crossProduct(lastPos3D, currentPos3D).normalized();
    //     float angle = acos(qMin(1.0f, QVector3D::dotProduct(lastPos3D, currentPos3D))) 
    //                 * 180.0f / M_PI * rotationSensitivity;
    //     
    //     QQuaternion newRot = QQuaternion::fromAxisAndAngle(axis, angle);
    //     rotation = newRot * rotation;
    //     
    //     lastMousePos = currentPos;
    //     update();
    // }
    QOpenGLWidget::mouseMoveEvent(event);
}

void RelativisticGLWidget::wheelEvent(QWheelEvent *event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
        zoom *= delta;
        zoom = qBound(0.1f, zoom, 10.0f);
        update();
    }
    event->accept();
}

void RelativisticGLWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void RelativisticGLWidget::setRenderMode(RenderMode mode) {
    currentRenderMode = mode;
    update();
}

void RelativisticGLWidget::setShowWireframeOverlay(bool show) {
    showWireframeOverlay = show;
    update();
}

void RelativisticGLWidget::setHideFaces(bool hide) {
    hideFaces = hide;
    update();
}

void RelativisticGLWidget::resetView() {
    rotation = QQuaternion();
    zoom = 1.0f;
    update();
}

QVector3D RelativisticGLWidget::projectToTrackball(const QPoint& screenPos) {
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

bool RelativisticGLWidget::loadOBJToOpenMesh(const QString &path) {
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    return OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt);
}

void RelativisticGLWidget::computeBoundingBox(Mesh::Point& min, Mesh::Point& max) {
    if (openMesh.n_vertices() == 0) return;
    
    min = max = openMesh.point(*openMesh.vertices_begin());
    for (auto vh : openMesh.vertices()) {
        min.minimize(openMesh.point(vh));
        max.maximize(openMesh.point(vh));
    }
}

void RelativisticGLWidget::prepareFaceIndices() {
    faces.clear();
    for (auto fh : openMesh.faces()) {
        auto fv_it = openMesh.fv_ccwbegin(fh);
        auto fv_end = openMesh.fv_ccwend(fh);
        int vertexCount = openMesh.valence(fh);
        
        if (vertexCount < 3) continue;
        
        if (vertexCount == 3) {
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx()); ++fv_it;
            faces.push_back((*fv_it).idx());
        } else {
            unsigned int centerIdx = (*fv_it).idx();
            ++fv_it;
            unsigned int prevIdx = (*fv_it).idx();
            ++fv_it;
            
            for (int i = 2; i < vertexCount; i++) {
                unsigned int currentIdx = (*fv_it).idx();
                faces.push_back(centerIdx);
                faces.push_back(prevIdx);
                faces.push_back(currentIdx);
                prevIdx = currentIdx;
                ++fv_it;
            }
        }
    }
}

void RelativisticGLWidget::prepareEdgeIndices() {
    edges.clear();
    std::set<std::pair<unsigned int, unsigned int>> uniqueEdges;
    for (auto heh : openMesh.halfedges()) {
        if (openMesh.is_boundary(heh) || heh.idx() < openMesh.opposite_halfedge_handle(heh).idx()) {
            unsigned int from = openMesh.from_vertex_handle(heh).idx();
            unsigned int to = openMesh.to_vertex_handle(heh).idx();
            
            if (from > to) std::swap(from, to);
            uniqueEdges.insert({from, to});
        }
    }
    
    for (const auto& edge : uniqueEdges) {
        edges.push_back(edge.first);
        edges.push_back(edge.second);
    }
}

void RelativisticGLWidget::scaleAndPositionMesh() {
    if (openMesh.n_vertices() == 0) return;
    
    Mesh::Point min, max;
    computeBoundingBox(min, max);
    
    // 计算物体的包围盒大小
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 修改：放大包围盒到20×20×20（原为10×10×10）
    float scale = 20.0f / maxSize;
    
    // 计算物体的中心
    Mesh::Point center = (min + max) * 0.5f;
    
    // 将物体放置在最左侧的中心位置 (-20, 0, 0)
    float targetX = -20.0f;
    float targetY = 0.0f;
    float targetZ = 0.0f;
    
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        
        // 先平移到原点，然后缩放，再平移到目标位置
        p = (p - center) * scale;
        p[0] += targetX;
        p[1] += targetY;
        p[2] += targetZ;
        
        openMesh.set_point(vh, p);
    }
}

void RelativisticGLWidget::updateBuffersFromOpenMesh() {
    if (openMesh.n_vertices() == 0) return;
    
    // 计算顶点和法线
    std::vector<float> vertices(openMesh.n_vertices() * 3);
    std::vector<float> normals(openMesh.n_vertices() * 3);
    
    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    openMesh.update_normals();
    
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
    
    faceProgram.bind();
    posLoc = faceProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        faceProgram.enableAttributeArray(posLoc);
        faceProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    int normalLoc = faceProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        faceProgram.enableAttributeArray(normalLoc);
        faceProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }
    
    blinnPhongProgram.bind();
    posLoc = blinnPhongProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        blinnPhongProgram.enableAttributeArray(posLoc);
        blinnPhongProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = blinnPhongProgram.attributeLocation("aNormal");
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

void RelativisticGLWidget::loadOBJ(const QString &path) {
    openMesh.clear();
    faces.clear();
    edges.clear();
    
    if (!loadOBJToOpenMesh(path)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    // 缩放并定位物体
    scaleAndPositionMesh();
    
    // 准备索引
    prepareFaceIndices();
    prepareEdgeIndices();
    
    modelLoaded = true;
    
    makeCurrent();
    updateBuffersFromOpenMesh();
    doneCurrent();
    
    resetView();
    update();
}

void RelativisticGLWidget::drawFaces(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
    faceProgram.bind();
    vao.bind();
    faceEbo.bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    faceProgram.setUniformValue("model", model);
    faceProgram.setUniformValue("view", view);
    faceProgram.setUniformValue("projection", projection);
    
    // 设置光源和颜色
    faceProgram.setUniformValue("objectColor", surfaceColor);
    faceProgram.setUniformValue("lightPos", QVector3D(0, 0, 10));
    faceProgram.setUniformValue("viewPos", QVector3D(0, 0, 0));

    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    faceProgram.release();
}

void RelativisticGLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void RelativisticGLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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