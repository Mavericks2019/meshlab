#include "shortestpathglwidget.h"
#include <QMouseEvent>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <limits>
#include <cmath>
#include <set>
#include <chrono>


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
    // 确保OpenGL上下文正确
    if (!context()->isValid()) {
        qDebug() << "OpenGL context is not valid during initialization!";
        return;
    }
    
    // 顶点拾取着色器
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/picking.vert");
    pickingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/picking.frag");
    if (!pickingProgram.link()) {
        qDebug() << "Failed to link picking shader program:" << pickingProgram.log();
    }
    
    // 面元拾取着色器
    facePickingProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/glwidget/shaders/picking.vert");
    facePickingProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/glwidget/shaders/face_picking.frag");
    if (!facePickingProgram.link()) {
        qDebug() << "Failed to link face picking shader program:" << facePickingProgram.log();
    }
    
    // 延迟创建FBO，直到widget有有效尺寸
    // 这里只创建空指针，实际在pickVertexAtPosition中创建
    pickingFBO = nullptr;
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

void ShortestPathGLWidget::resizeGL(int w, int h)
{
    BaseGLWidget::resizeGL(w, h);
    
    // 当widget尺寸变化时，重新创建FBO
    makeCurrent();
    
    if (pickingFBO && (pickingFBO->width() != w || pickingFBO->height() != h)) {
        delete pickingFBO;
        pickingFBO = nullptr;
    }
    
    doneCurrent();
}

void ShortestPathGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && modelLoaded) {
        qDebug() << "Mouse click at:" << event->x() << event->y();
        qDebug() << "Widget size:" << width() << height();
        qDebug() << "OpenGL context valid:" << context()->isValid();
        
        int vertexId = pickVertexAtPosition(event->x(), event->y());
        qDebug() << "Picked vertex ID:" << vertexId;
        
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
    
    // 确保widget有有效尺寸
    if (width() <= 0 || height() <= 0) {
        return -1;
    }
    
    // 确保OpenGL上下文正确
    if (!context()->isValid()) {
        qDebug() << "OpenGL context is not valid!";
        return -1;
    }
    
    // 检查是否需要重新创建FBO（尺寸变化或未初始化）
    if (!pickingFBO || 
        pickingFBO->width() != width() || 
        pickingFBO->height() != height()) {
        
        // 清理旧的FBO
        if (pickingFBO) {
            delete pickingFBO;
            pickingFBO = nullptr;
        }
        
        // 创建新的FBO
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        pickingFBO = new QOpenGLFramebufferObject(width(), height(), format);
        
        if (!pickingFBO->isValid()) {
            qDebug() << "Failed to create picking FBO!";
            delete pickingFBO;
            pickingFBO = nullptr;
            return -1;
        }
    }
    
    // 保存当前状态
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    
    // 禁用混合
    glDisable(GL_BLEND);
    
    // 绑定FBO
    pickingFBO->bind();
    
    // 设置视口
    glViewport(0, 0, width(), height());
    
    // 清除颜色和深度缓冲
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 设置矩阵（与正常渲染一致）
    QMatrix4x4 model, view, projection;
    
    model.rotate(rotation);
    model.scale(zoom);
    
    QVector3D eyePosition(0, 0, viewDistance * viewScale);
    view.lookAt(eyePosition, modelCenter, QVector3D(0, 1, 0));
    
    projection.perspective(45.0f, width() / float(height()), 0.1f, 100.0f);
    
    // 绘制面元
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
    
    // 绘制顶点
    pickingProgram.bind();
    vao.bind();
    
    pickingProgram.setUniformValue("model", model);
    pickingProgram.setUniformValue("view", view);
    pickingProgram.setUniformValue("projection", projection);
    
    glDrawArrays(GL_POINTS, 0, openMesh.n_vertices());
    
    // 转换鼠标坐标：Qt坐标（左上角为原点）到OpenGL坐标（左下角为原点）
    int glY = height() - y - 1;
    
    // 读取像素
    GLubyte pixel[4];
    glReadPixels(x, glY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    
    int vertexId = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
    
    // 如果顶点ID有效
    if (vertexId >= 0 && vertexId < static_cast<int>(openMesh.n_vertices())) {
        vao.release();
        pickingProgram.release();
        pickingFBO->release();
        
        // 恢复默认FBO
        glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
        glViewport(0, 0, width(), height());
        
        // 恢复状态
        if (blendEnabled) glEnable(GL_BLEND);
        if (!depthTestEnabled) glDisable(GL_DEPTH_TEST);
        
        glClearColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF());
        
        return vertexId;
    }
    
    // 如果直接拾取失败，尝试搜索周围的像素
    const int searchRadius = 10;
    int bestVertexId = -1;
    float bestDepth = 1.0f;
    
    for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
        for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
            int px = x + dx;
            int py = glY + dy;
            
            if (px < 0 || px >= width() || py < 0 || py >= height()) {
                continue;
            }
            
            glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            int currentVertexId = pixel[0] + (pixel[1] << 8) + (pixel[2] << 16);
            
            if (currentVertexId >= 0 && currentVertexId < static_cast<int>(openMesh.n_vertices())) {
                GLfloat depth;
                glReadPixels(px, py, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
                
                if (depth < bestDepth && depth > 0.0f) {
                    bestDepth = depth;
                    bestVertexId = currentVertexId;
                }
            }
        }
    }
    
    vao.release();
    pickingProgram.release();
    pickingFBO->release();
    
    // 恢复默认FBO
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, width(), height());
    
    // 恢复状态
    if (blendEnabled) glEnable(GL_BLEND);
    if (!depthTestEnabled) glDisable(GL_DEPTH_TEST);
    
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

// 计算启发式函数（欧几里得距离）
double ShortestPathGLWidget::heuristic(unsigned int from, unsigned int to)
{
    OpenMesh::VertexHandle vh_from(from);
    OpenMesh::VertexHandle vh_to(to);
    OpenMesh::Vec3d from_pos = openMesh.point(vh_from);
    OpenMesh::Vec3d to_pos = openMesh.point(vh_to);
    return (from_pos - to_pos).norm();
}

// 新增：A*算法计算最短路径
std::vector<unsigned int> ShortestPathGLWidget::aStarShortestPath(unsigned int start, unsigned int end)
{
    auto startTime = std::chrono::high_resolution_clock::now(); // 开始计时
    
    if (start >= openMesh.n_vertices() || end >= openMesh.n_vertices()) {
        return {};
    }
    
    // 初始化距离和前驱数组
    std::vector<double> g_score(openMesh.n_vertices(), std::numeric_limits<double>::max());
    std::vector<double> f_score(openMesh.n_vertices(), std::numeric_limits<double>::max());
    std::vector<int> prev(openMesh.n_vertices(), -1);
    
    g_score[start] = 0.0;
    f_score[start] = heuristic(start, end);
    
    // 使用优先队列实现A*算法
    using VertexScore = std::pair<double, unsigned int>;
    std::priority_queue<VertexScore, std::vector<VertexScore>, std::greater<VertexScore>> open_set;
    open_set.push({f_score[start], start});
    
    while (!open_set.empty()) {
        auto [current_f, u] = open_set.top();
        open_set.pop();
        
        // 如果到达终点，提前退出
        if (u == end) {
            break;
        }
        
        // 如果当前f_score不是最优的，跳过
        if (current_f > f_score[u]) {
            continue;
        }
        
        // 遍历所有邻接顶点
        OpenMesh::VertexHandle vh(u);
        for (auto vv_it = openMesh.vv_begin(vh); vv_it != openMesh.vv_end(vh); ++vv_it) {
            unsigned int v = vv_it->idx();
            
            // 计算边的权重（欧几里得距离）
            OpenMesh::Vec3d u_pos = openMesh.point(vh);
            OpenMesh::Vec3d v_pos = openMesh.point(*vv_it);
            double weight = (u_pos - v_pos).norm();
            
            // 计算临时g_score
            double tentative_g_score = g_score[u] + weight;
            
            // 如果找到更短的路径
            if (tentative_g_score < g_score[v]) {
                prev[v] = u;
                g_score[v] = tentative_g_score;
                f_score[v] = g_score[v] + heuristic(v, end);
                open_set.push({f_score[v], v});
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
    
    auto endTime = std::chrono::high_resolution_clock::now(); // 结束计时
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << "A* algorithm took " << duration.count() << " microseconds" << std::endl;
    
    return path;
}

std::vector<unsigned int> ShortestPathGLWidget::dijkstraShortestPath(unsigned int start, unsigned int end)
{
    auto startTime = std::chrono::high_resolution_clock::now(); // 开始计时
    
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
            OpenMesh::Vec3d u_pos = openMesh.point(vh);
            OpenMesh::Vec3d v_pos = openMesh.point(*vv_it);
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
    
    auto endTime = std::chrono::high_resolution_clock::now(); // 结束计时
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    std::cout << "Dijkstra algorithm took " << duration.count() << " microseconds" << std::endl;
    
    return path;
}

// 修改calculateAllShortestPaths方法，根据选择的算法计算路径
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
        
        std::vector<unsigned int> path;
        
        // 根据选择的算法计算路径
        if (currentAlgorithm == Dijkstra) {
            path = dijkstraShortestPath(start, end);
        } else if (currentAlgorithm == AStar) {
            path = aStarShortestPath(start, end);
        }
        
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