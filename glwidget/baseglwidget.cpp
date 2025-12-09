// baseglwidget.cpp
#include "baseglwidget.h"
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
#include <cfloat>
#include <Eigen/Sparse>
#include <set>

BaseGLWidget::BaseGLWidget(QWidget *parent) : QOpenGLWidget(parent),
    vbo(QOpenGLBuffer::VertexBuffer),
    ebo(QOpenGLBuffer::IndexBuffer),
    faceEbo(QOpenGLBuffer::IndexBuffer),
    axisVbo(QOpenGLBuffer::VertexBuffer),
    axisEbo(QOpenGLBuffer::IndexBuffer),
    showWireframeOverlay(true),  // 修改为true，默认显示线框
    hideFaces(false)
{
    QSurfaceFormat format;
    format.setSamples(4);
    format.setSwapInterval(1); // 1 表示启用垂直同步，0 表示禁用
    setFormat(format);
    
    setFocusPolicy(Qt::StrongFocus);
    rotation = QQuaternion();
    zoom = 1.0f;
    modelLoaded = false;
    isDragging = false;
    bgColor = QColor(0, 85, 127);  // 修改为深蓝色
    currentRenderMode = FlatShading;  // 修改为FlatShading
    // 修改线框颜色为红色 (1.0, 0.0, 0.0)
    wireframeColor = QVector4D(1.0f, 0.0f, 0.0f, 1.0f);
    // 修改表面颜色为米白色 (0.88, 0.84, 0.76)
    surfaceColor = QVector3D(0.88f, 0.84f, 0.76f);
    
    modelCenter = QVector3D(0, 0, 0);
    viewDistance = 5.0f;
    viewScale = 1.0f;
    
    initialRotation = QQuaternion();
    initialZoom = 1.0f;
    initialModelCenter = QVector3D(0, 0, 0);
    initialViewDistance = 5.0f;
    initialViewScale = 1.0f;
    
    showAxis = false;  // 修改为false，默认不显示坐标轴
    specularEnabled = false;  // 添加这行，默认不显示高光
}

std::map<int, float> BaseGLWidget::computeWeightsForVertex(Mesh::VertexHandle vh, ParameterizationMethod method) {
    std::map<int, float> weights;
    int vertexId = vh.idx();
    
    // 获取邻居顶点
    std::vector<Mesh::VertexHandle> neighbors;
    for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
        neighbors.push_back(*vv_it);
    }
    
    int degree = neighbors.size();
    
    switch (method) {
    case UniformTutte:
        // Tutte均匀参数化：λ = 1/degree
        for (auto neighbor : neighbors) {
            weights[neighbor.idx()] = 1.0f / degree;
        }
        break;
        
    case WeightedTutte:
        // 加权Tutte参数化：基于三维距离的权重
        {
            float totalWeight = 0.0f;
            std::vector<float> rawWeights;
            auto centerPos = openMesh.point(vh);
            
            for (auto neighbor : neighbors) {
                auto neighborPos = openMesh.point(neighbor);
                float dist = (neighborPos - centerPos).norm();
                // 避免除零错误
                float w = 1.0f / (dist * dist + 0.0001f);
                rawWeights.push_back(w);
                totalWeight += w;
            }
            
            // 归一化权重
            for (size_t i = 0; i < neighbors.size(); i++) {
                weights[neighbors[i].idx()] = rawWeights[i] / totalWeight;
            }
        }
        break;
        
    case FloaterShapePreserving:
        // 使用余切权重（形状保持）
        {
            float totalWeight = 0.0f;
            
            // 遍历所有出边的一半边
            for (auto heh : openMesh.voh_range(vh)) {
                if (!openMesh.is_boundary(heh)) {
                    Mesh::VertexHandle vj = openMesh.to_vertex_handle(heh);
                    float weight = computeCotangentWeight(heh);
                    
                    if (weight > 0) {
                        weights[vj.idx()] = weight;
                        totalWeight += weight;
                    }
                }
            }
            
            // 归一化权重
            if (totalWeight > 0) {
                for (auto& kv : weights) {
                    kv.second /= totalWeight;
                }
            }
        }
        break;
        
    case OriginalMethod:
    default:
        // 使用余切权重（原来的方法）
        {
            float totalWeight = 0.0f;
            
            // 遍历所有出边的一半边
            for (auto heh : openMesh.voh_range(vh)) {
                if (!openMesh.is_boundary(heh)) {
                    Mesh::VertexHandle vj = openMesh.to_vertex_handle(heh);
                    float weight = computeCotangentWeight(heh);
                    
                    if (weight > 0) {
                        weights[vj.idx()] = weight;
                        totalWeight += weight;
                    }
                }
            }
            
            // 归一化权重
            if (totalWeight > 0) {
                for (auto& kv : weights) {
                    kv.second /= totalWeight;
                }
            }
        }
        break;
    }
    
    return weights;
}

float BaseGLWidget::computeCotangentWeight(Mesh::HalfedgeHandle heh) {
    // 获取半边对应的两个三角形并计算余切权重
    if (!openMesh.is_boundary(heh)) {
        // 主半边
        Mesh::HalfedgeHandle heh_opp = openMesh.opposite_halfedge_handle(heh);
        
        // 获取顶点
        Mesh::VertexHandle v0 = openMesh.from_vertex_handle(heh);
        Mesh::VertexHandle v1 = openMesh.to_vertex_handle(heh);
        
        // 第一个三角形：v0-v1-v2
        Mesh::HalfedgeHandle heh_next = openMesh.next_halfedge_handle(heh);
        Mesh::VertexHandle v2 = openMesh.to_vertex_handle(heh_next);
        
        // 第二个三角形：v1-v0-v3
        Mesh::HalfedgeHandle heh_opp_next = openMesh.next_halfedge_handle(heh_opp);
        Mesh::VertexHandle v3 = openMesh.to_vertex_handle(heh_opp_next);
        
        // 计算两个角度的余切值
        auto p0 = openMesh.point(v0);
        auto p1 = openMesh.point(v1);
        auto p2 = openMesh.point(v2);
        auto p3 = openMesh.point(v3);
        
        // 向量计算
        auto e01 = p1 - p0;
        auto e02 = p2 - p0;
        auto e10 = p0 - p1;
        auto e13 = p3 - p1;
        
        // 计算角度并避免数值问题
        float cos_alpha = (e01 | e02) / (sqrt((e01 | e01) * (e02 | e02)) + 1e-10);
        float cos_beta = (e10 | e13) / (sqrt((e10 | e10) * (e13 | e13)) + 1e-10);
        
        cos_alpha = std::max(-1.0f, std::min(1.0f, cos_alpha));
        cos_beta = std::max(-1.0f, std::min(1.0f, cos_beta));
        
        float sin_alpha = sqrt(1.0f - cos_alpha * cos_alpha);
        float sin_beta = sqrt(1.0f - cos_beta * cos_beta);
        
        // 余切权重
        float cot_alpha = cos_alpha / (sin_alpha + 1e-10);
        float cot_beta = cos_beta / (sin_beta + 1e-10);
        
        return (cot_alpha + cot_beta) / 2.0f;
    }
    
    return 0.0f;
}

void BaseGLWidget::solveParameterization(ParameterizationMethod method) {
    if (!modelLoaded || openMesh.n_vertices() == 0) return;
    
    // 标记边界顶点
    std::vector<bool> isBoundary(openMesh.n_vertices(), false);
    for (auto vh : openMesh.vertices()) {
        isBoundary[vh.idx()] = openMesh.is_boundary(vh);
    }
    
    // 构建线性方程组
    using namespace Eigen;
    using SpMat = SparseMatrix<float>;
    using Triplet = Triplet<float>;
    
    int n = openMesh.n_vertices();
    SpMat A(n, n);
    VectorXf b_u(n), b_v(n);
    VectorXf x(n), y(n);
    
    b_u.setZero();
    b_v.setZero();
    
    std::vector<Triplet> triplets;
    triplets.reserve(n * 10);
    
    for (auto vh : openMesh.vertices()) {
        int i = vh.idx();
        
        if (isBoundary[i]) {
            // 边界顶点：固定位置
            triplets.push_back(Triplet(i, i, 1.0f));
            b_u[i] = openMesh.point(vh)[0];
            b_v[i] = openMesh.point(vh)[1];
        } else {
            // 内部顶点：根据方法计算权重
            auto weights = computeWeightsForVertex(vh, method);
            
            float totalWeight = 0.0f;
            for (const auto& [j, w] : weights) {
                triplets.push_back(Triplet(i, j, w));
                totalWeight += w;
            }
            triplets.push_back(Triplet(i, i, -totalWeight));
            b_u[i] = 0.0f;
            b_v[i] = 0.0f;
        }
    }
    
    // 设置稀疏矩阵
    A.setFromTriplets(triplets.begin(), triplets.end());
    A.makeCompressed();
    
    // 使用SparseLU求解器
    Eigen::SparseLU<SpMat> solver;
    solver.analyzePattern(A);
    solver.factorize(A);
    
    if (solver.info() != Eigen::Success) {
        qWarning() << "Matrix factorization failed!";
        return;
    }
    
    // 求解坐标
    x = solver.solve(b_u);
    y = solver.solve(b_v);
    
    // 更新顶点位置
    for (int i = 0; i < n; i++) {
        Mesh::Point newPos(x[i], y[i], 0.0f);
        openMesh.set_point(Mesh::VertexHandle(i), newPos);
    }
}

void BaseGLWidget::setShowAxis(bool show) {
    showAxis = show;
    update();
}

void BaseGLWidget::setViewScale(float scale) {
    viewScale = scale;
    update();
}

void BaseGLWidget::setHideFaces(bool hide) {
    hideFaces = hide;
    update();
}

void BaseGLWidget::setShowWireframeOverlay(bool show) {
    showWireframeOverlay = show;
    update();
}

void BaseGLWidget::setWireframeColor(const QVector4D& color) {
    wireframeColor = color;
    update();
}

BaseGLWidget::~BaseGLWidget() {
    makeCurrent();
    vao.destroy();
    vbo.destroy();
    ebo.destroy();
    faceEbo.destroy();
    axisVbo.destroy();
    axisEbo.destroy();
    doneCurrent();
}

void BaseGLWidget::resetView() {
    rotation = initialRotation;
    zoom = initialZoom;
    modelCenter = initialModelCenter;
    viewDistance = initialViewDistance;
    viewScale = initialViewScale;
    update();
}

void BaseGLWidget::setBackgroundColor(const QColor& color) {
    bgColor = color;
    makeCurrent();
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    doneCurrent();
    update();
}

void BaseGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    qDebug() << "OpenGL initialized. Version:" << (const char*)glGetString(GL_VERSION);
    qDebug() << "OpenGL renderer:" << (const char*)glGetString(GL_RENDERER); // 添加这行
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

void BaseGLWidget::initializeShaders() {
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
        updateBuffersFromOpenMesh();
    }
}

void BaseGLWidget::updateBuffersFromOpenMesh() {
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

void BaseGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void BaseGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!modelLoaded || openMesh.n_vertices() == 0) {
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
        // 基类只实现BlinnPhong渲染，曲率渲染在派生类中实现
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

void BaseGLWidget::keyPressEvent(QKeyEvent *event) {
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

void BaseGLWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void BaseGLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void BaseGLWidget::mouseMoveEvent(QMouseEvent *event) {
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

QVector3D BaseGLWidget::projectToTrackball(const QPoint& screenPos) {
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

void BaseGLWidget::wheelEvent(QWheelEvent *event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        float delta = numDegrees.y() > 0 ? 1.1f : 0.9f;
        zoom *= delta;
        zoom = qBound(0.1f, zoom, 10.0f);
        update();
    }
    event->accept();
}

void BaseGLWidget::setSurfaceColor(const QVector3D& color) {
    surfaceColor = color;
    update();
}

void BaseGLWidget::setSpecularEnabled(bool enabled) {
    specularEnabled = enabled;
    update();
}

void BaseGLWidget::centerView() {
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
    
    modelCenter = QVector3D(center[0], center[1], center[2]);
    viewDistance = 2.0f * maxSize;
    
    rotation = QQuaternion();
    zoom = 1.0f;
    update();
}

void BaseGLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void BaseGLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void BaseGLWidget::drawXYZAxis(const QMatrix4x4& view, const QMatrix4x4& projection) {
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

void BaseGLWidget::clearMeshData() {
    openMesh.clear();
    faces.clear();
    edges.clear();
    modelLoaded = false;
    parameterized = false;
    paramVertices.clear();
    paramFaces.clear();
}

bool BaseGLWidget::loadOBJToOpenMesh(const QString &path) {
    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    return OpenMesh::IO::read_mesh(openMesh, path.toStdString(), opt);
}

void BaseGLWidget::computeBoundingBox(Mesh::Point& min, Mesh::Point& max) {
    if (openMesh.n_vertices() > 0) {
        min = max = openMesh.point(*openMesh.vertices_begin());
        for (auto vh : openMesh.vertices()) {
            min.minimize(openMesh.point(vh));
            max.maximize(openMesh.point(vh));
        }
    }
}

void BaseGLWidget::centerAndScaleMesh(const Mesh::Point& center, float maxSize) {
    float scaleFactor = 2.0f / maxSize;
    for (auto vh : openMesh.vertices()) {
        Mesh::Point p = openMesh.point(vh);
        p = (p - center) * scaleFactor;
        openMesh.set_point(vh, p);
    }
}

void BaseGLWidget::prepareFaceIndices() {
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

void BaseGLWidget::prepareEdgeIndices() {
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

void BaseGLWidget::saveOriginalMesh() {
    originalMesh = openMesh;
    hasOriginalMesh = true;
}

void BaseGLWidget::loadOBJ(const QString &path) {
    clearMeshData();
    if (!loadOBJToOpenMesh(path)) {
        qWarning() << "Failed to load mesh:" << path;
        return;
    }
    
    Mesh::Point min, max;
    computeBoundingBox(min, max);
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    centerAndScaleMesh(center, maxSize);
    
    Mesh::Point min_norm, max_norm;
    computeBoundingBox(min_norm, max_norm);
    Mesh::Point center_norm = (min_norm + max_norm) * 0.5f;
    Mesh::Point size_norm = max_norm - min_norm;
    float maxSize_norm = std::max({size_norm[0], size_norm[1], size_norm[2]});

    modelCenter = QVector3D(0, 0, 0);
    viewDistance = 2.0f * maxSize_norm;
    
    initialRotation = QQuaternion();
    initialZoom = 1.0f;
    initialModelCenter = modelCenter;
    initialViewDistance = viewDistance;
    initialViewScale = viewScale;

    openMesh.request_vertex_normals();
    openMesh.request_face_normals();
    openMesh.update_normals();
    
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