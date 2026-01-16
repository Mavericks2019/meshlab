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
            // 临时处理：直接绘制拉伸后的线框
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            QMatrix4x4 model, view, projection;
            model.rotate(rotation);
            model.scale(zoom);
            
            QVector3D eyePosition(0, 0, viewDistance * viewScale);
            view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
            projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
            
            // 绘制拉伸后的线框
            drawWireframe(model, view, projection);
            
            // 绘制坐标轴
            if (showAxis) {
                drawXYZAxis(view, projection);
            }
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
        for (auto vh : openMesh.vertices()) {
            const auto& p = openMesh.point(vh);
            originalVertices.push_back(p[0]);
            originalVertices.push_back(p[1]);
            originalVertices.push_back(p[2]);
        }
    }
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