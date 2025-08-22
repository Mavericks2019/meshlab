#include "shortestpathglwidget.h"
#include <QMouseEvent>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <limits>


ShortestPathGLWidget::ShortestPathGLWidget(QWidget *parent) 
    : BaseGLWidget(parent), pickingFBO(nullptr), pathEdgeEbo(QOpenGLBuffer::IndexBuffer)
{

}

// 在析构函数中销毁pathEdgeEbo
ShortestPathGLWidget::~ShortestPathGLWidget()
{
    makeCurrent();
    if (pickingFBO) {
        delete pickingFBO;
    }
    pickingProgram.removeAllShaders();
    pathEdgeEbo.destroy(); // 添加这行
    doneCurrent();
}

void ShortestPathGLWidget::initializeGL()
{
    BaseGLWidget::initializeGL();
    initializePickingShaders();
    
    // 创建路径边的EBO
    pathEdgeEbo.create();
}

void ShortestPathGLWidget::initializePickingShaders()
{
    // 顶点拾取着色器
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/picking.vert");
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/picking.frag");
    pickingProgram.link();
    
    // 面元拾取着色器
    facePickingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/picking.vert");
    facePickingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/face_picking.frag");
    facePickingProgram.link();
    
    // 创建帧缓冲对象用于颜色编码拾取
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    pickingFBO = new QOpenGLFramebufferObject(width(), height(), format);
}

void ShortestPathGLWidget::paintGL()
{
    BaseGLWidget::paintGL();
    
    // 如果有选中的顶点，绘制高亮
    if (!selectedVertices.empty() || !pathVertices.empty() || !pathEdges.empty()) {
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
        
        // 绘制路径边（如果有）
        if (!pathEdges.empty()) {
            renderPathEdges();
        }
        
        glDisable(GL_POINT_SMOOTH);
    }
}

void ShortestPathGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && modelLoaded) {
        int vertexId = pickVertexAtPosition(event->x(), event->y());
        std::cout << vertexId << std::endl;
        if (vertexId != -1) {
            selectedVertices.push_back(vertexId);
            update();
        }
    }
    
    QOpenGLWidget::mouseDoubleClickEvent(event);
}

int ShortestPathGLWidget::pickVertexAtPosition(int x, int y)
{
    makeCurrent();
    // 保存当前的清除颜色和深度测试状态
    GLfloat oldClearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    // 绑定拾取FBO
    pickingFBO->bind();
    glViewport(0, 0, width(), height());
    
    // 使用黑色背景进行清除，并启用深度测试
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnable(GL_DEPTH_TEST);  // 启用深度测试
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    // 首先绘制面元（黑色）
    facePickingProgram.bind();
    vao.bind();
    faceEbo.bind();
    
    facePickingProgram.setUniformValue("model", model);
    facePickingProgram.setUniformValue("view", view);
    facePickingProgram.setUniformValue("projection", projection);
    
    glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
    
    faceEbo.release();
    vao.release();
    facePickingProgram.release();
    
    // 然后绘制顶点（使用ID编码颜色）
    pickingProgram.bind();
    vao.bind();
    
    pickingProgram.setUniformValue("model", model);
    pickingProgram.setUniformValue("view", view);
    pickingProgram.setUniformValue("projection", projection);
    
    // 绘制所有顶点，每个顶点使用其ID作为颜色
    glDrawArrays(GL_POINTS, 0, openMesh.n_vertices());
    
    // 定义搜索半径
    const int searchRadius = 10;
    
    // 计算搜索区域的边界
    int startX = std::max(0, x - searchRadius);
    int startY = std::max(0, y - searchRadius);
    int endX = std::min(width() - 1, x + searchRadius);
    int endY = std::min(height() - 1, y + searchRadius);
    
    int bestVertexId = -1;
    float bestDepth = 1.0f; // 初始化为最大深度值
    
    // 遍历搜索区域内的所有像素
    for (int py = startY; py <= endY; ++py) {
        for (int px = startX; px <= endX; ++px) {
            // 读取像素颜色
            GLubyte pixel[4];
            glReadPixels(px, height() - py - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            
            // 将颜色转换回顶点ID
            int vertexId = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
            
            // 如果读取的ID有效且不超过顶点数量
            if (vertexId >= 0 && vertexId < static_cast<int>(openMesh.n_vertices())) {
                // 读取深度值
                GLfloat depth;
                glReadPixels(px, height() - py - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
                
                // 选择最前面的顶点（深度值最小的）
                if (depth < bestDepth) {
                    bestDepth = depth;
                    bestVertexId = vertexId;
                }
            }
        }
    }
    
    vao.release();
    pickingProgram.release();
    pickingFBO->release();
    
    // 恢复默认FBO和视口
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, width(), height());
    
    // 恢复深度测试状态
    if (!depthTestEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    
    // 恢复清除颜色到BaseGLWidget中设置的值
    QColor bgColor = this->bgColor; // 从基类获取背景颜色
    glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
    
    return bestVertexId;
}

void ShortestPathGLWidget::clearSelectedPoints()
{
    selectedVertices.clear();
    pathVertices.clear();
    pathEdges.clear();
    update();
}

void ShortestPathGLWidget::calculateShortestPath()
{
    // 如果已经选择了两个点，计算路径
    if (selectedVertices.size() >= 2) {
        calculateAllShortestPaths();
    }
    std::cout << "calculateShortestPath" << std::endl;
    update();
}

// 新增方法：保存拾取FBO中的图像
void ShortestPathGLWidget::savePickingImage(const QString& filename)
{
    makeCurrent();
    
    // 绑定拾取FBO
    pickingFBO->bind();
    
    // 读取FBO中的图像数据
    QImage image = pickingFBO->toImage();
    
    // 释放FBO
    pickingFBO->release();
    
    // 保存图像到文件
    if (!image.save(filename)) {
        qDebug() << "Failed to save picking image to" << filename;
    } else {
        qDebug() << "Picking image saved to" << filename;
    }
}

// 新增：Dijkstra算法计算最短路径
std::vector<unsigned int> ShortestPathGLWidget::dijkstraShortestPath(unsigned int start, unsigned int end)
{
    if (start >= openMesh.n_vertices() || end >= openMesh.n_vertices()) {
        return {};
    }
    
    // 初始化距离和前驱数组
    std::vector<double> dist(openMesh.n_vertices(), std::numeric_limits<double>::max());
    std::vector<int> prev(openMesh.n_vertices(), -1);
    dist[start] = 0.0;
    
    // 使用优先队列实现Dijkstra算法
    using VertexDist = std::pair<double, unsigned int>;
    std::priority_queue<VertexDist, std::vector<VertexDist>, std::greater<VertexDist>> pq;
    pq.push({0.0, start});
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        
        // 如果已经找到更短的路径，跳过
        if (d > dist[u]) {
            continue;
        }
        
        // 如果到达终点，提前退出
        if (u == end) {
            break;
        }
        
        // 遍历所有邻接顶点
        OpenMesh::VertexHandle vh(u);
        for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
            unsigned int v = vv_it->idx();
            
            // 计算边的权重（欧几里得距离）
            OpenMesh::Vec3f u_pos = openMesh.point(vh);
            OpenMesh::Vec3f v_pos = openMesh.point(*vv_it);
            double weight = (u_pos - v_pos).norm();
            
            // 松弛操作
            if (dist[v] > dist[u] + weight) {
                dist[v] = dist[u] + weight;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }
    // 从终点回溯构建路径
    std::vector<unsigned int> path;
    int current = end;
    while (current != -1) {
        path.push_back(current);
        current = prev[current];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

// 修改calculateAllShortestPaths方法，填充pathEdgeIndices
void ShortestPathGLWidget::calculateAllShortestPaths()
{
    pathVertices.clear();
    pathEdges.clear();
    pathEdgeIndices.clear(); // 清除之前的路径边索引

    if (selectedVertices.size() < 2) {
        return;
    }

    // 计算每对相邻选中点之间的最短路径
    for (size_t i = 0; i < selectedVertices.size() - 1; ++i) {
        unsigned int start = selectedVertices[i];
        unsigned int end = selectedVertices[i + 1];
        
        std::vector<unsigned int> path = dijkstraShortestPath(start, end);
        
        if (path.empty()) {
            continue;
        }
        
        // 添加路径顶点到总路径中
        pathVertices.insert(pathVertices.end(), path.begin(), path.end());
        
        // 添加路径边到总边列表中
        for (size_t j = 0; j < path.size() - 1; ++j) {
            unsigned int v0 = path[j];
            unsigned int v1 = path[j + 1];
            
            // 查找连接这两个顶点的边
            OpenMesh::VertexHandle vh0(v0);
            OpenMesh::VertexHandle vh1(v1);
            
            auto heh = openMesh.find_halfedge(vh0, vh1);
            if (heh.is_valid()) {
                OpenMesh::EdgeHandle eh = openMesh.edge_handle(heh);
                pathEdges.push_back(eh.idx());

                // 将两个顶点索引添加到pathEdgeIndices
                pathEdgeIndices.push_back(v0);
                pathEdgeIndices.push_back(v1);
            }
        }
    }

    // 更新EBO
    makeCurrent();
    pathEdgeEbo.bind();
    if (!pathEdgeIndices.empty()) {
        pathEdgeEbo.allocate(pathEdgeIndices.data(), pathEdgeIndices.size() * sizeof(unsigned int));
    } else {
        pathEdgeEbo.allocate(0);
    }
    pathEdgeEbo.release();
    doneCurrent();
}

// 修改renderPathEdges方法，使用正确的索引数量
void ShortestPathGLWidget::renderPathEdges()
{
    if (pathEdgeIndices.empty()) {
        return;
    }
    
    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    // 设置线宽
    glLineWidth(5.0f);
    glEnable(GL_LINE_SMOOTH);
    
    wireframeProgram.bind();
    vao.bind();
    pathEdgeEbo.bind(); // 绑定路径边的EBO
    
    wireframeProgram.setUniformValue("model", model);
    wireframeProgram.setUniformValue("view", view);
    wireframeProgram.setUniformValue("projection", projection);
    wireframeProgram.setUniformValue("lineColor", QVector4D(0.0f, 1.0f, 0.0f, 1.0f)); // 绿色路径
    
    // 使用正确的索引数量
    glDrawElements(GL_LINES, pathEdgeIndices.size(), GL_UNSIGNED_INT, 0);
    
    pathEdgeEbo.release();
    vao.release();
    wireframeProgram.release();
    
    glDisable(GL_LINE_SMOOTH);
}