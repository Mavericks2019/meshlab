#include "shortestpathglwidget.h"
#include <QMouseEvent>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>

ShortestPathGLWidget::ShortestPathGLWidget(QWidget *parent) 
    : BaseGLWidget(parent), pickingFBO(nullptr)
{
    // 初始化父类后，可以添加特定于ShortestPathGLWidget的初始化
}

ShortestPathGLWidget::~ShortestPathGLWidget()
{
    makeCurrent();
    if (pickingFBO) {
        delete pickingFBO;
    }
    pickingProgram.removeAllShaders();
    doneCurrent();
}

void ShortestPathGLWidget::initializeGL()
{
    BaseGLWidget::initializeGL();
    initializePickingShaders();
}

void ShortestPathGLWidget::initializePickingShaders()
{
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/picking.vert");
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/picking.frag");
    pickingProgram.link();
    
    // 创建帧缓冲对象用于颜色编码拾取
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    pickingFBO = new QOpenGLFramebufferObject(width(), height(), format);
}

void ShortestPathGLWidget::paintGL()
{
    BaseGLWidget::paintGL();
    
    // 如果有选中的顶点，绘制高亮
    if (!selectedVertices.empty() || !pathVertices.empty()) {
        glPointSize(10.0f);
        glEnable(GL_POINT_SMOOTH);
        
        // 使用简单的着色器绘制高亮点
        QMatrix4x4 model, view, projection;
        
        model.rotate(rotation);
        model.scale(zoom);
        
        QVector3D eyePosition(0, 0, viewDistance * viewScale);
        view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
        
        projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
        
        // 绘制选中的顶点
        if (!selectedVertices.empty()) {
            wireframeProgram.bind();
            vao.bind();
            
            wireframeProgram.setUniformValue("model", model);
            wireframeProgram.setUniformValue("view", view);
            wireframeProgram.setUniformValue("projection", projection);
            wireframeProgram.setUniformValue("lineColor", QVector4D(highlightColor, 1.0f));
            
            glDrawElements(GL_POINTS, selectedVertices.size(), GL_UNSIGNED_INT, selectedVertices.data());
            
            vao.release();
            wireframeProgram.release();
        }
        
        // 绘制路径顶点（如果有）
        if (!pathVertices.empty()) {
            wireframeProgram.bind();
            vao.bind();
            
            wireframeProgram.setUniformValue("model", model);
            wireframeProgram.setUniformValue("view", view);
            wireframeProgram.setUniformValue("projection", projection);
            wireframeProgram.setUniformValue("lineColor", QVector4D(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色路径
            
            glDrawElements(GL_POINTS, pathVertices.size(), GL_UNSIGNED_INT, pathVertices.data());
            
            vao.release();
            wireframeProgram.release();
        }
        
        glDisable(GL_POINT_SMOOTH);
    }
}

void ShortestPathGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && modelLoaded) {
        int vertexId = pickVertexAtPosition(event->x(), event->y());
        if (vertexId != -1) {
            selectedVertices.push_back(vertexId);
            update();
            
            // 如果已经选择了两个点，计算路径
            if (selectedVertices.size() == 2) {
                calculateShortestPath();
            }
        }
    }
    
    QOpenGLWidget::mouseDoubleClickEvent(event);
}

int ShortestPathGLWidget::pickVertexAtPosition(int x, int y)
{
    makeCurrent();
    
    // 保存当前的清除颜色
    GLfloat oldClearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);
    
    // 绑定拾取FBO
    pickingFBO->bind();
    glViewport(0, 0, width(), height());
    
    // 使用黑色背景进行清除
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 设置拾取着色器
    pickingProgram.bind();
    vao.bind();
    
    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    pickingProgram.setUniformValue("model", model);
    pickingProgram.setUniformValue("view", view);
    pickingProgram.setUniformValue("projection", projection);
    
    // 绘制所有顶点，每个顶点使用其ID作为颜色
    glDrawArrays(GL_POINTS, 0, openMesh.n_vertices());
    
    // 读取像素颜色
    GLubyte pixel[4];
    glReadPixels(x, height() - y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    
    // 将颜色转换回顶点ID
    int vertexId = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
    
    vao.release();
    pickingProgram.release();
    pickingFBO->release();
    
    // 恢复默认FBO和视口
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, width(), height());
    
    // 恢复清除颜色到BaseGLWidget中设置的值
    QColor bgColor = this->bgColor; // 从基类获取背景颜色
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    
    // 如果读取的ID有效且不超过顶点数量，则返回
    if (vertexId >= 0 && vertexId < static_cast<int>(openMesh.n_vertices())) {
        return vertexId;
    }
    
    return -1;
}

void ShortestPathGLWidget::clearSelectedPoints()
{
    selectedVertices.clear();
    pathVertices.clear();
    update();
}

void ShortestPathGLWidget::calculateShortestPath()
{
    // 占位函数 - 后续实现最短路径算法
    // 这里只是简单地将两个点添加到路径中作为示例
    if (selectedVertices.size() == 2) {
        pathVertices = selectedVertices;
    }
    update();
}