// relasticwidget.cpp
#include "relasticwidget.h"
#include <QDebug>
#include <QVector3D>
#include <QMatrix4x4>
#include <cmath>
#include <QPainter>
#include <QFile>
#include <QTextStream>
#include <algorithm>

RelasticGLWidget::RelasticGLWidget(QWidget *parent)
    : BaseGLWidget(parent)
    , velocity(0, 0, 0)
    , gamma(1.0f)
    , visualizationMode(0) // 默认为原始模型
    , stretchFactor(1.0f)  // 拉伸因子
{
    // 不需要初始化变换缓冲区
}

RelasticGLWidget::~RelasticGLWidget()
{
    makeCurrent();
    doneCurrent();
}

void RelasticGLWidget::initializeGL()
{
    BaseGLWidget::initializeGL();
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
    else if (visualizationMode == 1 || visualizationMode == 2) {
        // 测量形象或视觉形象 - 显示变换后的模型
        if (modelLoaded && !originalVertices.empty()) {
            // 准备当前变换
            BaseGLWidget::paintGL();
        } else {
            BaseGLWidget::paintGL();
        }
    }
}

void RelasticGLWidget::loadOBJ(const QString &path)
{
    BaseGLWidget::loadOBJ(path);

    OpenMesh::IO::Options opt = OpenMesh::IO::Options::Default;
    OpenMesh::IO::read_mesh(openMesh_measure, path.toStdString(), opt);
    OpenMesh::IO::read_mesh(openMesh_look, path.toStdString(), opt);

    // 保存原始顶点数据
    if (modelLoaded) {
        originalVertices.clear();
        originalVertices.reserve(openMesh.n_vertices() * 3);
        
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            originalVertices.push_back(p[0]);
            originalVertices.push_back(p[1]);
            originalVertices.push_back(p[2]);
        }
        
        // 计算原始包围盒
        computeOriginalBoundingBox();
    }
    
    // 发射模型加载状态改变信号
    emit modelLoadedChanged(modelLoaded);
}

void RelasticGLWidget::setVelocity(const QVector3D& vel)
{
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

void RelasticGLWidget::setStretchFactor(float factor)
{
    stretchFactor = factor;
    update();
}

void RelasticGLWidget::computeGamma()
{
    float v2 = velocity.lengthSquared();
    if (v2 >= 1.0f) {
        gamma = 1000.0f;
    } else {
        gamma = 1.0f / sqrt(1.0f - v2);
    }
}

void RelasticGLWidget::updateBuffersFromOpenMesh()
{
    BaseGLWidget::updateBuffersFromOpenMesh();
}

void RelasticGLWidget::resetView()
{
    BaseGLWidget::resetView();
    velocity = QVector3D(0, 0, 0);
    gamma = 1.0f;
    visualizationMode = 0;
}

void RelasticGLWidget::centerView()
{
    BaseGLWidget::centerView();
}

// 计算原始包围盒
void RelasticGLWidget::computeOriginalBoundingBox()
{
    if (originalVertices.empty()) return;
    
    originalMin = QVector3D(FLT_MAX, FLT_MAX, FLT_MAX);
    originalMax = QVector3D(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    for (size_t i = 0; i < originalVertices.size(); i += 3) {
        QVector3D vertex(originalVertices[i], originalVertices[i+1], originalVertices[i+2]);
        originalMin.setX(qMin(originalMin.x(), vertex.x()));
        originalMin.setY(qMin(originalMin.y(), vertex.y()));
        originalMin.setZ(qMin(originalMin.z(), vertex.z()));
        
        originalMax.setX(qMax(originalMax.x(), vertex.x()));
        originalMax.setY(qMax(originalMax.y(), vertex.y()));
        originalMax.setZ(qMax(originalMax.z(), vertex.z()));
    }
    
    originalCenter = (originalMin + originalMax) * 0.5f;
    originalSize = originalMax - originalMin;
    originalMaxSize = qMax(originalSize.x(), qMax(originalSize.y(), originalSize.z()));
}

// 应用洛伦兹变换（测量形象）
std::vector<float> RelasticGLWidget::applyObserverLorentzTransform(const std::vector<float>& vertices)
{
    std::vector<float> result = vertices;
    
    float speed = velocity.length();
    if (speed < 1e-6f) {
        return result; // 速度为零，不需要变换
    }
    
    QVector3D n = velocity.normalized();
    float gamma = 1.0f / sqrt(1.0f - speed * speed);
    
    for (size_t i = 0; i < vertices.size(); i += 3) {
        QVector3D r_object(vertices[i], vertices[i+1], vertices[i+2]);
        
        // 分解为平行和垂直分量
        float parallel_dot = QVector3D::dotProduct(r_object, n);
        QVector3D r_parallel = parallel_dot * n;
        QVector3D r_perp = r_object - r_parallel;
        
        // 洛伦兹变换：平行分量收缩
        QVector3D r_observer = r_perp + r_parallel / gamma;
        
        result[i] = r_observer.x();
        result[i+1] = r_observer.y();
        result[i+2] = r_observer.z();
    }
    
    return result;
}

// 应用光锥变换（视觉形象）
std::vector<float> RelasticGLWidget::applyLightConeTransform(const std::vector<float>& vertices)
{
    std::vector<float> result = vertices;
    
    float vx = velocity.x();
    float vy = velocity.y();
    float vz = velocity.z();
    float speed = velocity.length();
    
    if (speed < 1e-6f) {
        return result; // 速度为零，不需要变换
    }
    
    // 先应用洛伦兹变换
    std::vector<float> lorentzVerts = applyObserverLorentzTransform(vertices);
    
    // 光锥变换参数
    float Vt = 1.0f;  // vt = 1
    float Vx = vx;
    float Vy = vy;
    float Vz = vz;
    
    for (size_t i = 0; i < lorentzVerts.size(); i += 3) {
        float x0 = lorentzVerts[i];
        float y0 = lorentzVerts[i+1];
        float z0 = lorentzVerts[i+2];
        float t0 = 0.0f; // t0 = 0
        
        // 二次方程系数
        float A = Vt * Vt - Vx * Vx - Vy * Vy - Vz * Vz;
        float B = 2.0f * (t0 * Vt - x0 * Vx - y0 * Vy - z0 * Vz);
        float C = t0 * t0 - x0 * x0 - y0 * y0 - z0 * z0;
        
        float s = 0.0f;
        
        if (qAbs(A) < 1e-6f) {
            // A = 0 的特殊情况
            if (qAbs(B) > 1e-6f) {
                s = -C / B;
            }
        } else {
            // 计算判别式
            float Delta = B * B - 4.0f * A * C;
            
            if (Delta >= 0) {
                float sqrtDelta = sqrt(Delta);
                float s1 = (-B + sqrtDelta) / (2.0f * A);
                float s2 = (-B - sqrtDelta) / (2.0f * A);
                
                // 选择负的s值（过去的光锥）
                if (s1 < 0 && s2 < 0) {
                    s = qMin(s1, s2);
                } else if (s1 < 0) {
                    s = s1;
                } else if (s2 < 0) {
                    s = s2;
                } else {
                    s = qMin(s1, s2); // 没有负值，取较小的
                }
            }
        }
        
        // 计算变换后的坐标
        float t = t0 + s * Vt;
        float x = x0 + s * Vx;
        float y = y0 + s * Vy;
        float z = z0 + s * Vz;
        
        result[i] = x;
        result[i+1] = y;
        result[i+2] = z;
    }
    
    return result;
}

// 准备当前显示的变换数据
void RelasticGLWidget::prepareCurrentTransformation()
{
    if (originalVertices.empty()) return;
    
    if (visualizationMode == 1) {
        // 测量形象：只应用洛伦兹变换
        transformedVertices = applyObserverLorentzTransform(originalVertices);
        lorentzVertices = transformedVertices; // 保存洛伦兹变换结果
    } else if (visualizationMode == 2) {
        // 视觉形象：应用完整的洛伦兹+光锥变换
        // 先计算洛伦兹变换
        lorentzVertices = applyObserverLorentzTransform(originalVertices);
        // 再应用光锥变换
        transformedVertices = applyLightConeTransform(lorentzVertices);
    }
}

// 导出变换后的OBJ文件
bool RelasticGLWidget::exportTransformedOBJ(const QString& filePath)
{
    if (!modelLoaded || originalVertices.empty()) {
        qWarning() << "No model loaded";
        return false;
    }
    
    std::vector<float> verticesToExport;
    
    if (visualizationMode == 1) {
        // 测量形象：导出洛伦兹变换结果
        verticesToExport = applyObserverLorentzTransform(originalVertices);
    } else if (visualizationMode == 2) {
        // 视觉形象：导出完整变换结果
        verticesToExport = applyLightConeTransform(originalVertices);
    } else {
        // 原始模型：导出原始顶点
        verticesToExport = originalVertices;
    }
    
    // 打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    
    // 写入顶点
    for (size_t i = 0; i < verticesToExport.size(); i += 3) {
        out << "v " 
            << verticesToExport[i] << " "
            << verticesToExport[i+1] << " "
            << verticesToExport[i+2] << "\n";
    }
    
    // 写入面（使用原始面索引）
    // 注意：OBJ文件索引从1开始
    for (size_t i = 0; i < faces.size(); i += 3) {
        out << "f "
            << faces[i] + 1 << " "
            << faces[i+1] + 1 << " "
            << faces[i+2] + 1 << "\n";
    }
    
    file.close();
    
    qDebug() << "Exported transformed OBJ to:" << filePath
             << "Mode:" << visualizationMode
             << "Velocity:" << velocity;
    
    return true;
}

// 重写绘制线框的方法
void RelasticGLWidget::drawWireframe(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!modelLoaded) return;
    
    // 创建一个变换后的模型矩阵，将 x 坐标拉伸
    QMatrix4x4 stretchedModel = model;
    stretchedModel.scale(stretchFactor, 1.0f, 1.0f);
    
    // 调用基类方法绘制线框
    BaseGLWidget::drawWireframe(stretchedModel, view, projection);
}

// 重写绘制线框覆盖的方法
void RelasticGLWidget::drawWireframeOverlay(const QMatrix4x4& model, const QMatrix4x4& view, const QMatrix4x4& projection)
{
    if (!modelLoaded) return;
    
    // 创建一个变换后的模型矩阵，将 x 坐标拉伸
    QMatrix4x4 stretchedModel = model;
    stretchedModel.scale(stretchFactor, 1.0f, 1.0f);
    
    // 调用基类方法绘制线框
    BaseGLWidget::drawWireframeOverlay(stretchedModel, view, projection);
}