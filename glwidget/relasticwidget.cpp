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
    , stretchFactor(2.0f)  // 拉伸因子
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
    
    // 初始化变换后的缓冲区
    updateTransformedBuffers();
}

void RelasticGLWidget::resizeGL(int w, int h)
{
    BaseGLWidget::resizeGL(w, h);
}

void RelasticGLWidget::paintGL()
{
    // 根据可视化模式选择渲染内容
    if (visualizationMode == 0) {
        // 原始模型 - 调用基类渲染
        BaseGLWidget::paintGL();
    } 
    else if (visualizationMode == 1) {
        // 测量形象 - 应用拉伸变换
        if (modelLoaded && !originalVertices.empty()) {
            std::cout << "1" << std::endl;
            applyStretchTransformation();
            std::cout << "2" << std::endl;
            renderStretchedModel();
            std::cout << "3" << std::endl;
        } else {
            BaseGLWidget::paintGL();
        }
    }
    else if (visualizationMode == 2) {
        // 视觉形象 - 暂时显示占位符
        BaseGLWidget::paintGL();
    }
}

void RelasticGLWidget::loadOBJ(const QString &path)
{
    BaseGLWidget::loadOBJ(path);
    
    // 保存原始顶点数据
    if (modelLoaded) {
        originalVertices.clear();
        originalVertices.resize(openMesh.n_vertices() * 3);
        
        int idx = 0;
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            originalVertices[idx++] = p[0];
            originalVertices[idx++] = p[1];
            originalVertices[idx++] = p[2];
        }
        
        // 保存原始模型的面索引
        originalFaces = faces;
        
        // 计算原始包围盒
        computeOriginalBoundingBox();
        
        // 初始化变换后的网格
        if (openMesh.n_vertices() > 0) {
            transformedMesh = openMesh;
            updateTransformedBuffers();
        }
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
    if (visualizationMode == 1 && modelLoaded) {
        // 重置到原始状态
        resetToOriginal();
        // 应用拉伸变换
        applyStretchTransformation();
        // 重新计算包围盒和相机
        adjustCameraForTransformedMesh();
    }
    update();
}

void RelasticGLWidget::setStretchFactor(float factor)
{
    stretchFactor = factor;
    if (visualizationMode == 1 && modelLoaded) {
        applyStretchTransformation();
        adjustCameraForTransformedMesh();
        update();
    }
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

void RelasticGLWidget::computeOriginalBoundingBox()
{
    if (originalVertices.empty()) return;
    
    // 计算原始顶点的包围盒
    originalMin = originalMax = QVector3D(originalVertices[0], originalVertices[1], originalVertices[2]);
    
    for (size_t i = 0; i < originalVertices.size(); i += 3) {
        QVector3D point(originalVertices[i], originalVertices[i+1], originalVertices[i+2]);
        originalMin.setX(qMin(originalMin.x(), point.x()));
        originalMin.setY(qMin(originalMin.y(), point.y()));
        originalMin.setZ(qMin(originalMin.z(), point.z()));
        
        originalMax.setX(qMax(originalMax.x(), point.x()));
        originalMax.setY(qMax(originalMax.y(), point.y()));
        originalMax.setZ(qMax(originalMax.z(), point.z()));
    }
    
    originalCenter = (originalMin + originalMax) * 0.5f;
    originalSize = originalMax - originalMin;
    originalMaxSize = qMax(qMax(originalSize.x(), originalSize.y()), originalSize.z());
}

void RelasticGLWidget::applyStretchTransformation()
{
    if (!modelLoaded || originalVertices.empty()) {
        return;
    }
    
    // 创建拉伸变换后的顶点
    stretchedVertices.resize(originalVertices.size());
    
    for (size_t i = 0; i < originalVertices.size(); i += 3) {
        // 将x坐标乘以拉伸因子
        stretchedVertices[i] = originalVertices[i] * stretchFactor;  // x坐标拉伸
        stretchedVertices[i+1] = originalVertices[i+1] * stretchFactor;              // y坐标不变
        stretchedVertices[i+2] = originalVertices[i+2];              // z坐标不变
    }
    
    // 更新变换后的网格
    if (transformedMesh.n_vertices() != openMesh.n_vertices()) {
        transformedMesh = openMesh;
    }
    
    int vertexIdx = 0;
    for (auto vh : transformedMesh.vertices()) {
        if (vertexIdx * 3 + 2 < (int)stretchedVertices.size()) {
            Mesh::Point p(
                stretchedVertices[vertexIdx * 3],
                stretchedVertices[vertexIdx * 3 + 1],
                stretchedVertices[vertexIdx * 3 + 2]
            );
            transformedMesh.set_point(vh, p);
            vertexIdx++;
        }
    }
    
    // 更新法线
    transformedMesh.request_vertex_normals();
    transformedMesh.request_face_normals();
    transformedMesh.update_normals();
    
    // 更新OpenGL缓冲区
    updateTransformedBuffers();
}

void RelasticGLWidget::renderStretchedModel()
{
    if (!modelLoaded || transformedMesh.n_vertices() == 0) {
        return;
    }
    
    // 清除屏幕
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 计算变换后的包围盒
    Mesh::Point min, max;
    computeTransformedBoundingBox(min, max);
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 设置模型中心（移动到原点便于观察）
    QVector3D modelCenter(0, 0, 0);
    
    // 计算视图距离（基于变换后的尺寸）
    float viewDistance = 2.0f * maxSize * viewScale;
    
    // 构建变换矩阵
    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    // 设置相机位置
    QVector3D eyePosition(0, 0, viewDistance);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    // 透视投影
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    // 法线矩阵
    QMatrix3x3 normalMatrix = model.normalMatrix();
    
    // 定义光源
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
    
    // 绘制面片
    if (!hideFaces) {
        if (currentRenderMode == BlinnPhong) {
            blinnPhongProgram.bind();
            transformedVao.bind();
            transformedFaceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            blinnPhongProgram.setUniformValue("model", model);
            blinnPhongProgram.setUniformValue("view", view);
            blinnPhongProgram.setUniformValue("projection", projection);
            blinnPhongProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置光源
            for (int i = 0; i < 3; i++) {
                blinnPhongProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                blinnPhongProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            blinnPhongProgram.setUniformValue("viewPos", eyePosition);
            blinnPhongProgram.setUniformValue("objectColor", surfaceColor);
            blinnPhongProgram.setUniformValue("specularEnabled", specularEnabled);
            
            glDrawElements(GL_TRIANGLES, originalFaces.size(), GL_UNSIGNED_INT, 0);
            
            transformedFaceEbo.release();
            transformedVao.release();
            blinnPhongProgram.release();
        }
        else if (currentRenderMode == FlatShading) {
            flatProgram.bind();
            transformedVao.bind();
            transformedFaceEbo.bind();
            
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            flatProgram.setUniformValue("model", model);
            flatProgram.setUniformValue("view", view);
            flatProgram.setUniformValue("projection", projection);
            flatProgram.setUniformValue("normalMatrix", normalMatrix);
            
            // 设置光源
            for (int i = 0; i < 3; i++) {
                flatProgram.setUniformValue(QString("lightPositions[%1]").arg(i).toStdString().c_str(), lightPositions[i]);
                flatProgram.setUniformValue(QString("lightColors[%1]").arg(i).toStdString().c_str(), lightColors[i]);
            }
            
            flatProgram.setUniformValue("viewPos", eyePosition);
            flatProgram.setUniformValue("objectColor", surfaceColor);
            flatProgram.setUniformValue("specularEnabled", specularEnabled);
            
            glDrawElements(GL_TRIANGLES, originalFaces.size(), GL_UNSIGNED_INT, 0);
            
            transformedFaceEbo.release();
            transformedVao.release();
            flatProgram.release();
        }
        
        // 绘制线框叠加
        if (showWireframeOverlay) {
            drawWireframeOverlay(model, view, projection);
        }
    } else {
        // 只绘制线框
        drawWireframe(model, view, projection);
    }
    
    // 绘制坐标轴
    if (showAxis) {
        drawXYZAxis(view, projection);
    }
}

void RelasticGLWidget::computeTransformedBoundingBox(Mesh::Point& min, Mesh::Point& max)
{
    if (transformedMesh.n_vertices() == 0) {
        min = max = Mesh::Point(0, 0, 0);
        return;
    }
    
    min = max = transformedMesh.point(*transformedMesh.vertices_begin());
    
    for (auto vh : transformedMesh.vertices()) {
        min.minimize(transformedMesh.point(vh));
        max.maximize(transformedMesh.point(vh));
    }
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
    
    // 设置顶点属性
    wireframeProgram.bind();
    int posLoc = wireframeProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        wireframeProgram.enableAttributeArray(posLoc);
        wireframeProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    wireframeProgram.release();
    
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
    blinnPhongProgram.release();
    
    flatProgram.bind();
    posLoc = flatProgram.attributeLocation("aPos");
    if (posLoc != -1) {
        flatProgram.enableAttributeArray(posLoc);
        flatProgram.setAttributeBuffer(posLoc, GL_FLOAT, 0, 3, 3 * sizeof(float));
    }
    
    normalLoc = flatProgram.attributeLocation("aNormal");
    if (normalLoc != -1) {
        flatProgram.enableAttributeArray(normalLoc);
        flatProgram.setAttributeBuffer(normalLoc, GL_FLOAT, vertexSize, 3, 3 * sizeof(float));
    }
    flatProgram.release();
    
    // 更新面索引缓冲区
    transformedFaceEbo.bind();
    transformedFaceEbo.allocate(originalFaces.data(), originalFaces.size() * sizeof(unsigned int));
    
    transformedVao.release();
}

void RelasticGLWidget::adjustCameraForTransformedMesh()
{
    if (transformedMesh.n_vertices() == 0) return;
    
    // 计算变换后网格的包围盒
    Mesh::Point min, max;
    computeTransformedBoundingBox(min, max);
    
    Mesh::Point center = (min + max) * 0.5f;
    Mesh::Point size = max - min;
    float maxSize = std::max({size[0], size[1], size[2]});
    
    // 更新相机参数
    modelCenter = QVector3D(center[0], center[1], center[2]);
    viewDistance = 2.0f * maxSize;
}

void RelasticGLWidget::resetToOriginal()
{
    if (modelLoaded && !originalVertices.empty()) {
        // 重置拉伸因子
        stretchFactor = 2.0f;
        
        // 重置变换后的网格
        transformedMesh = openMesh;
        
        // 重置顶点数据
        int idx = 0;
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            transformedMesh.set_point(vh, p);
            idx++;
        }
        
        // 更新法线
        transformedMesh.request_vertex_normals();
        transformedMesh.request_face_normals();
        transformedMesh.update_normals();
        
        // 更新缓冲区
        updateTransformedBuffers();
    }
}

void RelasticGLWidget::resetView()
{
    BaseGLWidget::resetView();
    
    // 重置相对论参数
    velocity = QVector3D(0, 0, 0);
    gamma = 1.0f;
    visualizationMode = 0;
    
    // 重置变换后的网格
    if (modelLoaded) {
        resetToOriginal();
    }
    
    update();
}

void RelasticGLWidget::centerView()
{
    if (visualizationMode == 0) {
        BaseGLWidget::centerView();
    } else if (visualizationMode == 1) {
        adjustCameraForTransformedMesh();
        update();
    }
}

void RelasticGLWidget::updateBuffersFromOpenMesh()
{
    BaseGLWidget::updateBuffersFromOpenMesh();
    
    // 同时更新变换后的缓冲区
    if (modelLoaded) {
        originalVertices.clear();
        originalVertices.resize(openMesh.n_vertices() * 3);
        
        int idx = 0;
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            originalVertices[idx++] = p[0];
            originalVertices[idx++] = p[1];
            originalVertices[idx++] = p[2];
        }
        
        originalFaces = faces;
        computeOriginalBoundingBox();
        
        transformedMesh = openMesh;
        updateTransformedBuffers();
    }
}

// 重写绘制线框的方法，使用变换后的网格数据
void RelasticGLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    wireframeProgram.bind();
    transformedVao.bind();
    ebo.bind();

    glLineWidth(1.5f);
    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    transformedVao.release();
    wireframeProgram.release();
}

// 重写绘制线框覆盖的方法，使用变换后的网格数据
void RelasticGLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0, -1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.5f);
    
    wireframeProgram.bind();
    transformedVao.bind();
    ebo.bind();

    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", wireframeColor);

    glDrawElements(GL_LINES, edges.size(), GL_UNSIGNED_INT, 0);
    
    ebo.release();
    transformedVao.release();
    wireframeProgram.release();
    glDisable(GL_POLYGON_OFFSET_LINE);
}