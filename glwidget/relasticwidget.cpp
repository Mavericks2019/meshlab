// relasticwidget.cpp
#include "relasticwidget.h"
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>
#include <cmath>
#include <QPainter>

RelasticGLWidget::RelasticGLWidget(QWidget *parent)
    : BaseGLWidget(parent)
    , velocity(0, 0, 0)
    , gamma(1.0f)
    , visualizationMode(0) // 默认为原始模型
{
    // 初始化变换后的缓冲区
    transformedVao.create();
    transformedVbo.create();
    transformedFaceEbo.create();
}

RelasticGLWidget::~RelasticGLWidget()
{
    makeCurrent();
    transformedVao.destroy();
    transformedVbo.destroy();
    transformedFaceEbo.destroy();
    doneCurrent();
}

void RelasticGLWidget::initializeGL()
{
    BaseGLWidget::initializeGL();
    
    // 初始化变换后网格的着色器
    transformedProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/relastic.vert");
    transformedProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/relastic.frag");
    transformedProgram.link();
    
    // 初始化变换后的缓冲区
    updateTransformedBuffers();
}

void RelasticGLWidget::resizeGL(int w, int h)
{
    BaseGLWidget::resizeGL(w, h);
}

void RelasticGLWidget::paintGL()
{
    // 如果选择了测量形象且速度不为零，应用洛伦兹变换
    if (visualizationMode == 1 && !velocity.isNull()) {
        applyLorentzTransformation();
    }
    
    // 根据可视化模式选择渲染内容
    if (visualizationMode == 0) {
        // 原始模型 - 调用基类渲染
        BaseGLWidget::paintGL();
    } 
    else if (visualizationMode == 1) {
        // 测量形象 - 渲染变换后的网格
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        if (!modelLoaded || transformedMesh.n_vertices() == 0) {
            return;
        }
        
        QMatrix4x4 model, view, projection;
        
        model.rotate(rotation);
        model.scale(zoom);
        
        QVector3D eyePosition(0, 0, viewDistance * viewScale);
        view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
        
        projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
        
        // 设置三个光源的位置和颜色
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
        
        // 渲染变换后的网格
        transformedProgram.bind();
        transformedVao.bind();
        transformedFaceEbo.bind();
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        transformedProgram.setUniformValue("model", model);
        transformedProgram.setUniformValue("view", view);
        transformedProgram.setUniformValue("projection", projection);
        transformedProgram.setUniformValue("normalMatrix", model.normalMatrix());
        
        // 设置光源
        for (int i = 0; i < 3; i++) {
            transformedProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
            transformedProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
        }
        
        transformedProgram.setUniformValue("viewPos", QVector3D(0, 0, viewDistance * viewScale));
        transformedProgram.setUniformValue("objectColor", QVector3D(0.5f, 0.8f, 1.0f)); // 浅蓝色表示测量形象
        transformedProgram.setUniformValue("specularEnabled", specularEnabled);
        
        glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
        
        transformedFaceEbo.release();
        transformedVao.release();
        transformedProgram.release();
        
        // 绘制线框覆盖
        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
        
        if (showAxis) {
            drawXYZAxis(view, projection);
        }
    }
    else if (visualizationMode == 2) {
        // 视觉形象 - 暂时显示占位符
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // 显示提示文本
        QPainter painter(this);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 16));
        painter.drawText(rect(), Qt::AlignCenter, "视觉形象 - 功能开发中");
        painter.end();
    }
}

void RelasticGLWidget::loadOBJ(const QString &path)
{
    BaseGLWidget::loadOBJ(path);
    
    // 保存原始顶点数据
    if (modelLoaded) {
        originalVertices.clear();
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            originalVertices.push_back(p[0]);
            originalVertices.push_back(p[1]);
            originalVertices.push_back(p[2]);
        }
        
        // 复制原始网格到变换后的网格
        transformedMesh = openMesh;
    }
}

void RelasticGLWidget::setVelocity(const QVector3D& vel)
{
    // 确保速度小于光速（不能等于1）
    velocity.setX(qMin(0.999f, qMax(-0.999f, vel.x())));
    velocity.setY(qMin(0.999f, qMax(-0.999f, vel.y())));
    velocity.setZ(qMin(0.999f, qMax(-0.999f, vel.z())));
    
    computeGamma();
    update();
}

void RelasticGLWidget::setVisualizationMode(int mode)
{
    visualizationMode = mode;
    update();
}

void RelasticGLWidget::computeGamma()
{
    float v2 = velocity.lengthSquared();
    if (v2 >= 1.0f) {
        gamma = 1000.0f; // 接近光速时的近似
    } else {
        gamma = 1.0f / sqrt(1.0f - v2);
    }
}

QVector3D RelasticGLWidget::lorentzTransform(const QVector3D& point) const
{
    if (velocity.isNull()) {
        return point;
    }
    
    // 四维位置向量 (ct, x, y, z)
    float ct = 0; // 假设t=0
    float x = point.x();
    float y = point.y();
    float z = point.z();
    
    // 速度方向单位向量
    QVector3D v_hat = velocity.normalized();
    float v = velocity.length();
    
    // 平行于速度方向的分量
    float x_parallel = QVector3D::dotProduct(point, v_hat);
    QVector3D x_perp = point - v_hat * x_parallel;
    
    // 洛伦兹变换
    float ct_prime = gamma * (ct - v * x_parallel);
    float x_parallel_prime = gamma * (x_parallel - v * ct);
    
    // 组合变换后的位置
    QVector3D transformed = x_perp + v_hat * x_parallel_prime;
    
    return transformed;
}

void RelasticGLWidget::applyLorentzTransformation()
{
    if (!modelLoaded || originalVertices.empty()) {
        return;
    }
    
    // 应用洛伦兹变换到每个顶点
    int vertexCount = openMesh.n_vertices();
    auto vh_it = transformedMesh.vertices_begin();
    
    for (int i = 0; i < vertexCount; ++i, ++vh_it) {
        // 从原始顶点数据获取位置
        float x = originalVertices[i * 3];
        float y = originalVertices[i * 3 + 1];
        float z = originalVertices[i * 3 + 2];
        
        QVector3D originalPoint(x, y, z);
        QVector3D transformedPoint = lorentzTransform(originalPoint);
        
        // 更新变换后的网格
        transformedMesh.set_point(*vh_it, 
            Mesh::Point(transformedPoint.x(), transformedPoint.y(), transformedPoint.z()));
    }
    
    // 更新法线
    transformedMesh.request_vertex_normals();
    transformedMesh.request_face_normals();
    transformedMesh.update_normals();
    
    // 更新OpenGL缓冲区
    updateTransformedBuffers();
    
    // 调整相机
    adjustCameraForTransformedMesh();
}

void RelasticGLWidget::updateTransformedBuffers()
{
    if (transformedMesh.n_vertices() == 0) return;
    
    // 准备变换后的顶点数据
    std::vector<float> vertices(transformedMesh.n_vertices() * 3);
    std::vector<float> normals(transformedMesh.n_vertices() * 3);
    
    for (auto vh : transformedMesh.vertices()) {
        int idx = vh.idx();
        const auto& p = transformedMesh.point(vh);
        vertices[idx*3]   = p[0];
        vertices[idx*3+1] = p[1];
        vertices[idx*3+2] = p[2];
        
        const auto& n = transformedMesh.normal(vh);
        normals[idx*3]   = n[0];
        normals[idx*3+1] = n[1];
        normals[idx*3+2] = n[2];
    }
    
    // 更新变换后的缓冲区
    transformedVao.bind();
    transformedVbo.bind();
    
    int vertexSize = vertices.size() * sizeof(float);
    int normalSize = normals.size() * sizeof(float);
    transformedVbo.allocate(vertexSize + normalSize);
    transformedVbo.write(0, vertices.data(), vertexSize);
    transformedVbo.write(vertexSize, normals.data(), normalSize);
    
    transformedProgram.bind();
    int posLoc = transformedProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        transformedProgram.enableAttributeArray(posLoc);
        transformedProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    int normalLoc = transformedProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        transformedProgram.enableAttributeArray(normalLoc);
        transformedProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }
    
    transformedFaceEbo.bind();
    transformedFaceEbo.allocate(faces.data(), faces.size() * sizeof(unsigned int));
    
    transformedVao.release();
}

void RelasticGLWidget::adjustCameraForTransformedMesh()
{
    if (transformedMesh.n_vertices() == 0) return;
    
    // 计算变换后网格的包围盒
    Mesh::Point min, max;
    min = max = transformedMesh.point(*transformedMesh.vertices_begin());
    
    for (auto vh : transformedMesh.vertices()) {
        min.minimize(transformedMesh.point(vh));
        max.maximize(transformedMesh.point(vh));
    }
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 更新相机参数
    modelCenter = QVector3D(center[0], center[1], center[2]);
    viewDistance = 2.0f * maxSize;
    
    update();
}

void RelasticGLWidget::resetView()
{
    BaseGLWidget::resetView();
    
    // 重置速度
    velocity = QVector3D(0, 0, 0);
    gamma = 1.0f;
    visualizationMode = 0;
    
    // 重置变换后的网格
    if (modelLoaded) {
        transformedMesh = openMesh;
        updateTransformedBuffers();
    }
    
    update();
}

void RelasticGLWidget::centerView()
{
    if (visualizationMode == 0) {
        BaseGLWidget::centerView();
    } else if (visualizationMode == 1) {
        adjustCameraForTransformedMesh();
    }
}

void RelasticGLWidget::updateBuffersFromOpenMesh()
{
    BaseGLWidget::updateBuffersFromOpenMesh();
    
    // 同时更新变换后的缓冲区
    if (modelLoaded) {
        transformedMesh = openMesh;
        updateTransformedBuffers();
    }
}